#pragma once
#include "../common/common.h"
#include "../common/schrift.h"

enum class E_TEXFORMAT
{
    RGBA8888_UNORM,
    RG88_UNORM,
    RGB565_UNORM
};

struct Vertex
{
    f32 pos[2];
    f32 tex_coord[2];
};

class GX2ShaderSet
{
public:
    explicit GX2ShaderSet(std::string_view name);

    void Prepare() const;
    void Activate() const;
    bool IsCompiledSuccessfully() const { return compiledSuccessfully; }

    static void InitDefaultFetchShader();

    void SerializeToFile(std::string path);

    //static GX2ShaderSet* Load(const char* name);
    //static void SwitchToShader();

private:
    bool compiledSuccessfully = false;
    GX2FetchShader* fetchShader;
    GX2VertexShader* vertexShader;
    GX2PixelShader* fragmentShader;

    static bool s_defaultFetchShaderInitialized;
    static GX2FetchShader s_defaultFetchShader;
};

// helper class to simplify shader loading and management
class ShaderSwitcher
{
public:
    explicit ShaderSwitcher(const std::string_view shaderName)
    {
        m_name = shaderName;
    }

    void CheckForReload()
    {
        if (m_lastRefresh < OSGetTime())
        {
            m_lastRefresh = OSGetTime() + (OSTime)OSMillisecondsToTicks(10 * 1000);
            auto reloadedShader = std::make_unique<GX2ShaderSet>(m_name);
            if (reloadedShader->IsCompiledSuccessfully())
            {
                m_shaderSet = std::move(reloadedShader);
            }
            else {
                WHBLogPrintf("Failed to reload shader %s", m_name.data());
            }
        }
    }

    void Activate()
    {
        if (m_shaderSet)
        {
 #ifdef DEBUG
             // check for reload in debug mode
             CheckForReload();
 #endif

            // activate and exit
            m_shaderSet->Activate();
            return;
        }
        m_shaderSet = std::make_unique<GX2ShaderSet>(m_name);
        if (!m_shaderSet->IsCompiledSuccessfully()) {
            CriticalErrorHandler("Failed to compile shader %s", m_name.data());
        }
#ifdef DEBUG
        // dump shader to file
        m_shaderSet->SerializeToFile(std::string(m_name));
#endif

        m_shaderSet->Activate();
    }

private:
    std::unique_ptr<GX2ShaderSet> m_shaderSet;
    std::string_view m_name;
    OSTime m_lastRefresh = 0;
};

class Framebuffer
{
    friend class Render;
    static constexpr u32 MAX_COLOR_BUFFERS = 8;
public:
    void SetColorBuffer(u32 index, u32 width, u32 height, E_TEXFORMAT texFormat, bool clear = false, bool allocateInMEM1 = false);
    void Apply();

    static void ApplyBackbuffer();
    static Framebuffer* GetActiveFramebuffer();

    GX2Texture* GetColorBufferTexture(u32 index)
    {
        if(index >= MAX_COLOR_BUFFERS)
            return nullptr;
        return m_colorBufferTexture[index];
    }

    static f32 GetCurrentPixelWidth() { return m_activePixelWidth; }
    static f32 GetCurrentPixelHeight() { return m_activePixelHeight; }

private:
    GX2Texture* m_colorBufferTexture[MAX_COLOR_BUFFERS]{};
    GX2ColorBuffer* m_colorBuffer[MAX_COLOR_BUFFERS]{};

    static inline f32 m_activePixelWidth{};
    static inline f32 m_activePixelHeight{};
};

class Sprite
{
    friend class Render;
public:
    Sprite(const char* path, bool hasTransparency = false);
    Sprite(u32 width, u32 height, bool hasTransparency, E_TEXFORMAT format = E_TEXFORMAT::RGBA8888_UNORM); // create sprite with uninitialized pixels
    struct GX2Texture* GetTexture() { return m_tex; }

    void SetupSampler(bool useLinearFiltering);

    void Clear(u32 color = 0x00000000);
    u32 GetPixelRGBA8888(u32 x, u32 y);
    void SetPixelRGBA8888(u32 x, u32 y, u32 color);
    void SetPixelRG88(u32 x, u32 y, u32 color);
    void FlushCache();

    u8* GetData() { return (u8*)m_tex->surface.image; }
    s32 GetBytesPerRow() { return (s32)m_tex->surface.width * 4; }

    s32 GetWidth() const { return m_width; }
    s32 GetHeight() const { return m_height; }
private:

    struct GX2Texture* m_tex;
    struct GX2Sampler* m_sampler;
    s32 m_width;
    s32 m_height;
    bool m_hasTransparency;
    E_TEXFORMAT m_format;
    bool m_isAlias{false};
};

class Render
{
    friend class Framebuffer;
public:
    static void Init();
    static void Shutdown();
    static void BeginFrame();
    static void DoPostProcessing();
    static bool IsRunning();
    static void SwapBuffers();

    static void SetStateForSpriteRendering();

    static void RenderSprite(Sprite* sprite, s32 x, s32 y);
    static void RenderSprite(Sprite* sprite, s32 x, s32 y, s32 pxWidth, s32 pxHeight);
    static void RenderSprite(Sprite* sprite, s32 x, s32 y, s32 pxWidth, s32 pxHeight, f32 angle);
    static void RenderSpritePortion(Sprite *sprite, s32 x, s32 y, u32 cx, u32 cy, u32 cw, u32 ch);
    static void RenderSpriteScreenRelative(Sprite* sprite, s32 x, s32 y);
    static void RenderSpriteScreenRelative(Sprite *sprite, s32 x, s32 y, s32 pxWidth, s32 pxHeight);
    static void RenderSpritePortionScreenRelative(Sprite*, s32, s32, u32, u32, u32, u32);

    static void SetCameraPosition(Vector2f pos);
    static Vector2f GetCameraPosition(); // returns camera position, pixel-aligned
    static Vector2f GetUnfilteredCameraPosition(); // returns the original vector passed to SetCameraPosition
    static Vector2i GetScreenSize();

    static void RenderText(u32 x, u32 y, u8 blackLevel, const char* text, ...);
    static Sprite* RenderTextSprite(u8 size, u32 color, const wchar_t* string);
protected:
    static void UpdateCameraCachedValues();

    static Sprite* sSmallFontTextureBlack;
    static Sprite* sSmallFontTextureWhite;
};

class RenderState
{
public:
    enum class E_TRANSPARENCY_MODE
    {
        OPAQUE,
        ADDITIVE,
    };

    static void Init();
    static void ReapplyState();
    static void SetTransparencyMode(E_TRANSPARENCY_MODE mode);
    static void SetShaderMode(GX2ShaderMode mode);

private:
    static E_TRANSPARENCY_MODE sRenderTransparencyMode;
    static GX2ColorControlReg sRenderColorControl_noTransparency;
    static GX2ColorControlReg sRenderColorControl_transparency;
    static GX2BlendControlReg sRenderBlendReg_transparency;
    static GX2ShaderMode sShaderMode;

};