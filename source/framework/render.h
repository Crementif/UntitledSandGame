#pragma once
#include "../common/common.h"

enum class E_TEXFORMAT
{
    RGBA8888_UNORM,
    RG88_UNORM
};

struct Vertex
{
    f32 pos[2];
    f32 tex_coord[2];
};

struct GX2ShaderSet
{
    static GX2ShaderSet* Load(const char* name);
    //static void SwitchToShader();

    void Prepare();
    void Activate();

    GX2FetchShader* fetchShader;
    GX2VertexShader* vertexShader;
    GX2PixelShader* fragmentShader;
};

constexpr uint64_t ct_fnv1a_hash(std::string_view str) noexcept
{
    uint64_t hash = 14695981039346656037u;
    for(auto c : str)
    {
        hash = (hash ^ static_cast<uint64_t>(c)) * (uint64_t)1099511628211u;
    }
    return hash;
}

// helper class to simplify shader loading and management
class ShaderSwitcher
{
public:
    ShaderSwitcher(const std::string_view shaderName)
    {
        m_name = shaderName;
    }

    void Activate()
    {
        if(m_shaderSet)
        {
            // activate and exit
            m_shaderSet->Activate();
            return;
        }
        m_shaderSet = GX2ShaderSet::Load(m_name.c_str());
        m_shaderSet->Activate();
    }

private:
    GX2ShaderSet* m_shaderSet{nullptr};
    std::string m_name;
};

class Framebuffer
{
    friend class Render;
    static constexpr u32 MAX_COLOR_BUFFERS = 8;
public:
    void SetColorBuffer(u32 index, u32 width, u32 height, E_TEXFORMAT texFormat, bool clear = false);
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
    void SetPixelRGBA8888(u32 x, u32 y, u32 color);
    void SetPixelRG88(u32 x, u32 y, u32 color);
    void FlushCache();

    u8* GetData() { return (u8*)m_tex->surface.image; };
    s32 GetBytesPerRow()
    {
        return m_tex->surface.width * 4;
    }

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

    static void RenderText(u32 x, u32 y, u8 textSize, u8 blackLevel, const char* text, ...);
protected:
    static void UpdateCameraCachedValues();

    static Sprite* sBigFontTextureBlack;
    static Sprite* sBigFontTextureWhite;
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

};