#include "render.h"

#include <coreinit/memdefaultheap.h>

#include <gx2/clear.h>
#include <gx2/draw.h>
#include <gx2/mem.h>
#include <gx2/registers.h>
#include <gx2/utils.h>
#include <gx2/event.h>
#include <gx2/surface.h>
#include <whb/gfx.h>

#include <whb/file.h>
#include <whb/sdcard.h>

#include "window.h"

#include "render_data.h"
#include "../framework/fileformat/TGAFile.h"
#include "../game/GameScene.h"

ShaderSwitcher Shader_CRT{"crt"};
Framebuffer* postProcessingBuffer = nullptr;

void _GX2InitTexture(GX2Texture* texturePtr, u32 width, u32 height, u32 depth, u32 numMips, GX2SurfaceFormat surfaceFormat, GX2SurfaceDim surfaceDim, GX2TileMode tileMode)
{
    texturePtr->surface.dim = surfaceDim;
    texturePtr->surface.width = width;
    texturePtr->surface.height = height;
    texturePtr->surface.depth = depth;
    texturePtr->surface.mipLevels = numMips;
    texturePtr->surface.format = surfaceFormat;
    texturePtr->surface.aa = GX2_AA_MODE1X;
    texturePtr->surface.use = GX2_SURFACE_USE_TEXTURE;
    texturePtr->surface.dim = surfaceDim;
    texturePtr->surface.tileMode = tileMode;
    texturePtr->surface.swizzle = 0;
    texturePtr->viewFirstMip = 0;
    texturePtr->viewNumMips = numMips;
    texturePtr->viewFirstSlice = 0;
    texturePtr->viewNumSlices = depth;
    texturePtr->compMap = 0x0010203;
    GX2CalcSurfaceSizeAndAlignment(&texturePtr->surface);
    GX2InitTextureRegs(texturePtr);
}

void _GX2AllocateTexture(GX2Texture* texture)
{
    texture->surface.image = MEMAllocFromDefaultHeapEx(texture->surface.imageSize, texture->surface.alignment);
}

void _GX2DeallocateTexture(GX2Texture* texture)
{
    if(texture->surface.image)
    {
        MEMFreeToDefaultHeap(texture->surface.image);
        texture->surface.image = nullptr;
    }
}

void _GX2InitColorBufferFromSurface(GX2ColorBuffer* colorBuffer, GX2Surface* surface)
{
    memset(colorBuffer, 0, sizeof(GX2ColorBuffer));
    colorBuffer->viewFirstSlice = 0;
    colorBuffer->viewNumSlices = 1;
    colorBuffer->viewMip = 0;
    colorBuffer->surface = *surface;
    GX2InitColorBufferRegs(colorBuffer);
}

GX2SurfaceFormat _GetGX2SurfaceFormat(E_TEXFORMAT texFormat)
{
    switch(texFormat)
    {
        case E_TEXFORMAT::RGBA8888_UNORM: return GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8;
        case E_TEXFORMAT::RG88_UNORM: return GX2_SURFACE_FORMAT_UNORM_R8_G8;
        default:
            CriticalErrorHandler("Unknown texture format");
    }
    return GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8;
}

void Framebuffer::SetColorBuffer(u32 index, u32 width, u32 height, E_TEXFORMAT texFormat, bool clear)
{
    if (index >= MAX_COLOR_BUFFERS)
        return;
    if(m_colorBufferTexture[index])
    {
        // destroy existing color buffer
        _GX2DeallocateTexture(m_colorBufferTexture[index]);
        free(m_colorBufferTexture[index]);
        m_colorBufferTexture[index] = nullptr;
        free(m_colorBuffer[index]);
        m_colorBuffer[index] = nullptr;
    }
    // allocate and initialize GX2Surface/GX2Texture
    m_colorBufferTexture[index] = (GX2Texture*)malloc(sizeof(GX2Texture));
    memset(m_colorBufferTexture[index], 0, sizeof(sizeof(GX2Texture)));
    _GX2InitTexture(m_colorBufferTexture[index], width, height, 1, 1, _GetGX2SurfaceFormat(texFormat), GX2_SURFACE_DIM_TEXTURE_2D, GX2_TILE_MODE_DEFAULT);
    _GX2AllocateTexture(m_colorBufferTexture[index]);
    m_colorBufferTexture[index]->surface.use = (GX2SurfaceUse)(m_colorBufferTexture[index]->surface.use | GX2_SURFACE_USE_TEXTURE_COLOR_BUFFER_TV);
    if(clear)
    {
        memset(m_colorBufferTexture[index]->surface.image, 0, m_colorBufferTexture[index]->surface.imageSize);
        GX2Invalidate(GX2_INVALIDATE_MODE_CPU_TEXTURE, m_colorBufferTexture[index]->surface.image, m_colorBufferTexture[index]->surface.imageSize);
    }
    // allocate and initialize GX2ColorBuffer
    m_colorBuffer[index] = (GX2ColorBuffer*)malloc(sizeof(GX2ColorBuffer));
    _GX2InitColorBufferFromSurface(m_colorBuffer[index], &m_colorBufferTexture[index]->surface);

    OSReport("Framebuffer::SetColorBuffer. Set texture with res %d/%d in slot %d\n", width, height, index);
}

void Framebuffer::Apply()
{
    s32 width=0, height=0;
    for (u32 i = 0; i < MAX_COLOR_BUFFERS; ++i)
    {
        if (m_colorBuffer[i])
        {
            GX2SetColorBuffer(m_colorBuffer[i], (GX2RenderTarget)i);
            if(width == 0)
            {
                width = m_colorBuffer[i]->surface.width;
                height = m_colorBuffer[i]->surface.height;
                m_activePixelWidth = 1.0f / (f32)width;
                m_activePixelHeight = 1.0f / (f32)height;
            }
        }
    }
    GX2SetViewport(0.0f, 0.0f, (float)width, (float)height, 0.0f, 1.0f);
    GX2SetScissor(0, 0, width, height);
    Render::UpdateCameraCachedValues();
}

void Framebuffer::ApplyBackbuffer()
{
    s32 width = WindowGetWidth();
    s32 height = WindowGetHeight();
    GX2SetColorBuffer(WindowGetColorBuffer(), GX2_RENDER_TARGET_0);
    GX2SetDepthBuffer(WindowGetDepthBuffer());

    GX2SetViewport(0, 0, width, height, 0.0f, 1.0f);
    GX2SetScissor(0, 0, width, height);

    m_activePixelWidth = 1.0f / (f32)width;
    m_activePixelHeight = 1.0f / (f32)height;
    Render::UpdateCameraCachedValues();
}

GX2Sampler sRenderBaseSampler1_linear;
GX2Sampler sRenderBaseSampler1_nearest;

struct
{
  Vertex* base;
  u32 currentWriteIndex;
  u32 size;

  Vertex* GetVertices(u32 n, u32& vertexBaseIndex)
  {
      vertexBaseIndex = currentWriteIndex;
      if((currentWriteIndex + n) >= size)
      {
          currentWriteIndex = n;
          vertexBaseIndex = 0;
      }
      else
      {
          currentWriteIndex += n;
      }
      return base + vertexBaseIndex;
  }
}sVtxRingbuffer;

void _InitVtxRingbuffer()
{
    sVtxRingbuffer.size = 1 * 1024 * 1024 / sizeof(Vertex); // 1 MiB of vertex ringbuffer. Can hold roughly 43k vertices
    sVtxRingbuffer.base = (Vertex*)MEMAllocFromDefaultHeapEx(1024 * 1024, 256);
    sVtxRingbuffer.currentWriteIndex = 0;
}

bool s_compilerInitialized = false;

void _InitShaderCompiler()
{
    if(!s_compilerInitialized)
    {
        if (GLSL_Init())
            s_compilerInitialized = true;
        else
        {
            OSFatal("Error: Place the glslcompiler.rpl in the /wiiu/libs/ folder on your SD card.");
        }
    }
}

void _InitBasicRenderResources()
{
    _InitShaderCompiler();

    // init linear sampler
    GX2InitSampler(&sRenderBaseSampler1_linear, GX2_TEX_CLAMP_MODE_CLAMP, GX2_TEX_XY_FILTER_MODE_LINEAR);
    GX2InitSamplerZMFilter(&sRenderBaseSampler1_linear, GX2_TEX_Z_FILTER_MODE_NONE, GX2_TEX_MIP_FILTER_MODE_NONE);

    // init nearest sampler
    GX2InitSampler(&sRenderBaseSampler1_nearest, GX2_TEX_CLAMP_MODE_CLAMP, GX2_TEX_XY_FILTER_MODE_POINT);
    GX2InitSamplerZMFilter(&sRenderBaseSampler1_nearest, GX2_TEX_Z_FILTER_MODE_NONE, GX2_TEX_MIP_FILTER_MODE_NONE);
}

void Render::Init()
{
    u32 fb_width, fb_height;
    WindowInit(1920, 1080, &fb_width, &fb_height);
    RenderState::Init();
    _InitVtxRingbuffer();
    _InitBasicRenderResources();
}

void Render::Shutdown()
{
    WindowExit();
}

bool Render::IsRunning()
{
    return WindowIsRunning();
}

void Render::BeginFrame()
{
    Framebuffer::ApplyBackbuffer();
    GX2ClearColor(WindowGetColorBuffer(), 0.2f, 0.3f, 0.3f, 1.0f); // we could skip this if we were to overdraw the full screen
    WindowMakeContextCurrent();
}

constexpr static __attribute__ ((aligned (32))) u16 s_idx_data[] =
{
    0, 1, 2, 2, 1, 3
};

void Render::DoPostProcessing() {
    if (GameScene::IsCrtFilterEnabled()) {
        Framebuffer::ApplyBackbuffer();

        Shader_CRT.Activate();

        GX2SetPixelTexture(WindowGetColorBufferTexture(), 0);
        GX2SetPixelSampler(&sRenderBaseSampler1_nearest, 0);

        GX2DrawIndexedEx(GX2_PRIMITIVE_MODE_TRIANGLES, 6, GX2_INDEX_TYPE_U16, (void*)s_idx_data, 0, 1);
    }
}

void Render::SwapBuffers()
{
    WindowSwapBuffers();
}

Vector2f sRenderCamUnfiltered{0.0, 0.0};
Vector2f sRenderCamPosition{0.0, 0.0};
Vector2f sRenderCamOffset{0.0, 0.0};

void Render::SetCameraPosition(Vector2f pos)
{
    sRenderCamUnfiltered = pos;
    UpdateCameraCachedValues();
}

void Render::UpdateCameraCachedValues()
{
    sRenderCamPosition = sRenderCamUnfiltered;
    sRenderCamPosition.x = (f32)(s32)sRenderCamPosition.x;
    sRenderCamPosition.y = (f32)(s32)sRenderCamPosition.y;

    sRenderCamOffset = sRenderCamPosition;

    const f32 PIXEL_WIDTH = Framebuffer::GetCurrentPixelWidth() * 2.0;
    const f32 PIXEL_HEIGHT = Framebuffer::GetCurrentPixelHeight() * 2.0;
    sRenderCamOffset.x *= PIXEL_WIDTH;
    sRenderCamOffset.y *= PIXEL_HEIGHT;
}

Vector2f Render::GetCameraPosition()
{
    return sRenderCamPosition;
}

Vector2f Render::GetUnfilteredCameraPosition()
{
    return sRenderCamUnfiltered;
}

Vector2i Render::GetScreenSize()
{
    return {1920, 1080};
}

ShaderSwitcher Shader_Sprite{"sprite"};

void Render::SetStateForSpriteRendering()
{
    RenderState::ReapplyState();

    GX2SetShaderModeEx(GX2_SHADER_MODE_UNIFORM_REGISTER, 48, 64, 0, 0, 200, 192);
    // setup GX2 state for sprite renderer
    Shader_Sprite.Activate();
    // use the sprite vertex ringbuffer
    GX2SetAttribBuffer(0, 1024 * 1024, sizeof(Vertex), sVtxRingbuffer.base);
}

template<bool TUseCamPos, bool TInvalidate = true>
u32 _SetupSpriteVertexData(f32 x, f32 y, f32 spriteWidth, f32 spriteHeight)
{
    const f32 PIXEL_WIDTH = Framebuffer::GetCurrentPixelWidth() * 2.0; // *2.0 is since window coordinates are -1.0 to 1.0
    const f32 PIXEL_HEIGHT = Framebuffer::GetCurrentPixelHeight() * 2.0;

    u32 baseVertex;
    Vertex* vtx = sVtxRingbuffer.GetVertices(4, baseVertex);

    f32 x1 = x * PIXEL_WIDTH - 1.0;
    f32 y1 = 1.0 - y * PIXEL_HEIGHT;
    if constexpr(TUseCamPos)
    {
        x1 -= sRenderCamOffset.x;
        y1 += sRenderCamOffset.y;
    }
    f32 x2 = x1 + PIXEL_WIDTH * spriteWidth;
    f32 y2 = y1 - PIXEL_HEIGHT * spriteHeight;


    vtx[0].tex_coord[0] = 0.0f;
    vtx[0].tex_coord[1] = 0.0f;
    vtx[1].tex_coord[0] = 1.0f;
    vtx[1].tex_coord[1] = 0.0f;
    vtx[2].tex_coord[0] = 0.0f;
    vtx[2].tex_coord[1] = 1.0f;
    vtx[3].tex_coord[0] = 1.0f;
    vtx[3].tex_coord[1] = 1.0f;

    vtx[0].pos[0] = x1;
    vtx[0].pos[1] = y1;
    vtx[1].pos[0] = x2;
    vtx[1].pos[1] = y1;
    vtx[2].pos[0] = x1;
    vtx[2].pos[1] = y2;
    vtx[3].pos[0] = x2;
    vtx[3].pos[1] = y2;

    if constexpr (TInvalidate)
        GX2Invalidate(GX2_INVALIDATE_MODE_CPU_ATTRIBUTE_BUFFER, (void*)vtx, (sizeof(Vertex) * 4));

    return baseVertex;
}

template<bool TUseCamPos, bool TInvalidate = true>
u32 _SetupSpriteVertexData(f32 x, f32 y, f32 spriteWidth, f32 spriteHeight, f32 cx, f32 cy, f32 cw, f32 ch)
{
    const f32 PIXEL_WIDTH = Framebuffer::GetCurrentPixelWidth() * 2.0; // *2.0 since window coordinates are -1.0 to 1.0
    const f32 PIXEL_HEIGHT = Framebuffer::GetCurrentPixelHeight() * 2.0;

    u32 baseVertex;
    Vertex* vtx = sVtxRingbuffer.GetVertices(4, baseVertex);

    f32 x1 = x * PIXEL_WIDTH - 1.0;
    f32 y1 = 1.0 - y * PIXEL_HEIGHT;
    if constexpr(TUseCamPos)
    {
        x1 -= sRenderCamOffset.x;
        y1 += sRenderCamOffset.y;
    }
    f32 x2 = x1 + PIXEL_WIDTH * cw*spriteWidth;
    f32 y2 = y1 - PIXEL_HEIGHT * ch*spriteHeight;

    vtx[0].tex_coord[0] = cx;
    vtx[0].tex_coord[1] = cy;
    vtx[1].tex_coord[0] = cx + cw;
    vtx[1].tex_coord[1] = cy;
    vtx[2].tex_coord[0] = cx;
    vtx[2].tex_coord[1] = cy + ch;
    vtx[3].tex_coord[0] = cx + cw;
    vtx[3].tex_coord[1] = cy + ch;

    vtx[0].pos[0] = x1;
    vtx[0].pos[1] = y1;
    vtx[1].pos[0] = x2;
    vtx[1].pos[1] = y1;
    vtx[2].pos[0] = x1;
    vtx[2].pos[1] = y2;
    vtx[3].pos[0] = x2;
    vtx[3].pos[1] = y2;

    if constexpr (TInvalidate)
        GX2Invalidate(GX2_INVALIDATE_MODE_CPU_ATTRIBUTE_BUFFER, (void*)vtx, (sizeof(Vertex) * 4));

    return baseVertex;
}

void _RotateVerticesAroundPoint(Vertex* vtx, f32 centerX, f32 centerY, f32 angle)
{
    const f32 PIXEL_WIDTH = 1.0 / 1920.0 * 2.0; // *2.0 since window coordinates are -1.0 to 1.0
    const f32 PIXEL_HEIGHT = 1.0 / 1080.0 * 2.0;
    centerX = centerX * PIXEL_WIDTH - 1.0;
    centerY = 1.0 - centerY * PIXEL_HEIGHT;
    centerX -= sRenderCamOffset.x;
    centerY += sRenderCamOffset.y;

    Vector2f centerVec(centerX, centerY);
    for(int i=0; i<4; i++)
    {
        Vector2f pos(vtx[i].pos[0], vtx[i].pos[1]);
        Vector2f tmp = (pos - centerVec);
        tmp.x /= -(PIXEL_WIDTH / PIXEL_HEIGHT);
        tmp = tmp.Rotate(angle);
        tmp.x *= -(PIXEL_WIDTH / PIXEL_HEIGHT);
        pos = tmp + centerVec;
        vtx[i].pos[0] = pos.x;
        vtx[i].pos[1] = pos.y;
    }
}

void Render::RenderSprite(Sprite* sprite, s32 x, s32 y, s32 pxWidth, s32 pxHeight)
{
    GX2Texture* tex = sprite->GetTexture();
    u32 baseVertex = _SetupSpriteVertexData<true>((f32)x, (f32)y, (f32)pxWidth, (f32)pxHeight);

    RenderState::SetTransparencyMode(sprite->m_hasTransparency ? RenderState::E_TRANSPARENCY_MODE::ADDITIVE : RenderState::E_TRANSPARENCY_MODE::OPAQUE);

    GX2SetPixelTexture(tex, 0);
    GX2SetPixelSampler(sprite->m_sampler, 0);

    GX2DrawIndexedEx(GX2_PRIMITIVE_MODE_TRIANGLES, 6, GX2_INDEX_TYPE_U16, (void*)s_idx_data, baseVertex, 1);
}


void Render::RenderSprite(Sprite* sprite, s32 x, s32 y, s32 pxWidth, s32 pxHeight, f32 angle)
{
    GX2Texture* tex = sprite->GetTexture();
    u32 baseVertex = _SetupSpriteVertexData<true, false>((f32)x - (f32)pxWidth * 0.5f, (f32)y - (f32)pxHeight * 0.5f, (f32)pxWidth, (f32)pxHeight);
    _RotateVerticesAroundPoint(sVtxRingbuffer.base + baseVertex, x, y, angle);

    GX2Invalidate(GX2_INVALIDATE_MODE_CPU_ATTRIBUTE_BUFFER, sVtxRingbuffer.base + baseVertex, (sizeof(Vertex) * 4));

    RenderState::SetTransparencyMode(sprite->m_hasTransparency ? RenderState::E_TRANSPARENCY_MODE::ADDITIVE : RenderState::E_TRANSPARENCY_MODE::OPAQUE);

    GX2SetPixelTexture(tex, 0);
    GX2SetPixelSampler(sprite->m_sampler, 0);

    GX2DrawIndexedEx(GX2_PRIMITIVE_MODE_TRIANGLES, 6, GX2_INDEX_TYPE_U16, (void*)s_idx_data, baseVertex, 1);
}

void Render::RenderSprite(Sprite* sprite, s32 x, s32 y)
{
    GX2Texture* tex = sprite->GetTexture();
    RenderSprite(sprite, x, y, tex->surface.width, tex->surface.height);
}

void Render::RenderSpritePortion(Sprite* sprite, s32 x, s32 y, u32 cx, u32 cy, u32 cw, u32 ch)
{
    GX2Texture* tex = sprite->GetTexture();
    u32 baseVertex = _SetupSpriteVertexData<true>((f32)x, (f32)y, (f32)tex->surface.width, (f32)tex->surface.height, (f32)cx/tex->surface.width, (f32)cy/tex->surface.height, (f32)cw/tex->surface.width, (f32)ch/tex->surface.height);

    RenderState::SetTransparencyMode(sprite->m_hasTransparency ? RenderState::E_TRANSPARENCY_MODE::ADDITIVE : RenderState::E_TRANSPARENCY_MODE::OPAQUE);

    GX2SetPixelTexture(tex, 0);
    GX2SetPixelSampler(sprite->m_sampler, 0);

    GX2DrawIndexedEx(GX2_PRIMITIVE_MODE_TRIANGLES, 6, GX2_INDEX_TYPE_U16, (void*)s_idx_data, baseVertex, 1);
}


// same as RenderSprite but camera position is ignored and it renders a subrect of the sprite
void Render::RenderSpritePortionScreenRelative(Sprite* sprite, s32 x, s32 y, u32 cx, u32 cy, u32 cw, u32 ch)
{
    GX2Texture* tex = sprite->GetTexture();
    u32 baseVertex = _SetupSpriteVertexData<false>((f32)x, (f32)y, (f32)tex->surface.width, (f32)tex->surface.height, (f32)cx/tex->surface.width, (f32)cy/tex->surface.height, (f32)cw/tex->surface.width, (f32)ch/tex->surface.height);

    RenderState::SetTransparencyMode(sprite->m_hasTransparency ? RenderState::E_TRANSPARENCY_MODE::ADDITIVE : RenderState::E_TRANSPARENCY_MODE::OPAQUE);

    GX2SetPixelTexture(tex, 0);
    GX2SetPixelSampler(sprite->m_sampler, 0);

    GX2DrawIndexedEx(GX2_PRIMITIVE_MODE_TRIANGLES, 6, GX2_INDEX_TYPE_U16, (void*)s_idx_data, baseVertex, 1);
}

// same as RenderSprite but camera position is ignored
void Render::RenderSpriteScreenRelative(Sprite* sprite, s32 x, s32 y)
{
    GX2Texture* tex = sprite->GetTexture();
    u32 baseVertex = _SetupSpriteVertexData<false>((f32)x, (f32)y, (f32)tex->surface.width, (f32)tex->surface.height);

    RenderState::SetTransparencyMode(sprite->m_hasTransparency ? RenderState::E_TRANSPARENCY_MODE::ADDITIVE : RenderState::E_TRANSPARENCY_MODE::OPAQUE);

    GX2SetPixelTexture(tex, 0);
    GX2SetPixelSampler(sprite->m_sampler, 0);

    GX2DrawIndexedEx(GX2_PRIMITIVE_MODE_TRIANGLES, 6, GX2_INDEX_TYPE_U16, (void*)s_idx_data, baseVertex, 1);
}

void Render::RenderSpriteScreenRelative(Sprite* sprite, s32 x, s32 y, s32 pxWidth, s32 pxHeight)
{
    GX2Texture* tex = sprite->GetTexture();
    u32 baseVertex = _SetupSpriteVertexData<false>((f32)x, (f32)y, (f32)pxWidth, (f32)pxHeight);

    RenderState::SetTransparencyMode(sprite->m_hasTransparency ? RenderState::E_TRANSPARENCY_MODE::ADDITIVE : RenderState::E_TRANSPARENCY_MODE::OPAQUE);

    GX2SetPixelTexture(tex, 0);
    GX2SetPixelSampler(sprite->m_sampler, 0);

    GX2DrawIndexedEx(GX2_PRIMITIVE_MODE_TRIANGLES, 6, GX2_INDEX_TYPE_U16, (void*)s_idx_data, baseVertex, 1);
}

GX2Texture* LoadFromTGA(u8* data, u32 length);
GX2Texture* _InitSpriteTexture(u32 width, u32 height, E_TEXFORMAT format);

Sprite::Sprite(const char* path, bool hasTransparency) : m_hasTransparency(hasTransparency)
{
    std::ifstream fs((std::string("romfs:/") + path).c_str(), std::ios::in | std::ios::binary);
    if(!fs.is_open())
        CriticalErrorHandler("Failed to open file %s\n", path);
    std::vector<u8> data((std::istreambuf_iterator<char>(fs)), std::istreambuf_iterator<char>());
    m_tex = LoadFromTGA((u8*)data.data(), data.size());
    if (!m_tex) CriticalErrorHandler("Invalid TGA data");
    m_width = (s32)m_tex->surface.width;
    m_height = (s32)m_tex->surface.height;
    m_format = E_TEXFORMAT::RGBA8888_UNORM;
    SetupSampler(true);
}

Sprite::Sprite(u32 width, u32 height, bool hasTransparency, E_TEXFORMAT format) : m_hasTransparency(hasTransparency)
{
    m_format = format;
    m_tex = _InitSpriteTexture(width, height, format);
    if (!m_tex) CriticalErrorHandler("Invalid TGA data");
    m_width = (s32)m_tex->surface.width;
    m_height = (s32)m_tex->surface.height;
    SetupSampler(true);
}

void Sprite::SetupSampler(bool useLinearFiltering)
{
    if(useLinearFiltering)
        m_sampler = &sRenderBaseSampler1_linear;
    else
        m_sampler = &sRenderBaseSampler1_nearest;
}

void Sprite::Clear(u32 color)
{
    if(m_format == E_TEXFORMAT::RGBA8888_UNORM)
    {
        u8* p = (u8*)m_tex->surface.image;
        for(u32 i = 0; i < m_tex->surface.imageSize; i+=4)
        {
            p[i+0] = (u8)(color>>24);
            p[i+1] = (u8)(color>>16);
            p[i+2] = (u8)(color>>8);
            p[i+3] = (u8)(color>>0);
        }
    }
    else if(m_format == E_TEXFORMAT::RG88_UNORM)
    {
        u8* p = (u8*)m_tex->surface.image;
        for(u32 i = 0; i < m_tex->surface.imageSize; i+=2)
        {
            p[i+0] = (u8)(color>>24);
            p[i+1] = (u8)(color>>16);
        }
    }
}

void Sprite::SetPixelRGBA8888(u32 x, u32 y, u32 color)
{
    u8* p = (u8*)m_tex->surface.image + (x + y * m_tex->surface.pitch) * 4;
    p[0] = (u8)(color>>24);
    p[1] = (u8)(color>>16);
    p[2] = (u8)(color>>8);
    p[3] = (u8)(color>>0);
}

void Sprite::SetPixelRG88(u32 x, u32 y, u32 color)
{
    u8* p = (u8*)m_tex->surface.image + (x + y * m_tex->surface.pitch) * 2;
    p[0] = (u8)(color>>24);
    p[1] = (u8)(color>>16);
}

void Sprite::FlushCache()
{
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU_TEXTURE, m_tex->surface.image, m_tex->surface.imageSize);
}

static_assert(sizeof(TGA_HEADER) == 18);

GX2SurfaceFormat _TexFormatToGX2Format(E_TEXFORMAT format)
{
    switch(format)
    {
        case E_TEXFORMAT::RGBA8888_UNORM: return GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8;
        case E_TEXFORMAT::RG88_UNORM: return GX2_SURFACE_FORMAT_UNORM_R8_G8;
        default:
            CriticalErrorHandler("Unsupported sprite format %d", (s32)format);
    }
    return GX2_SURFACE_FORMAT_UNORM_R8_G8_B8_A8;
}

GX2Texture* _InitSpriteTexture(u32 width, u32 height, E_TEXFORMAT format)
{
    GX2Texture* texture = (GX2Texture*)MEMAllocFromDefaultHeap(sizeof(GX2Texture));
    memset(texture, 0, sizeof(GX2Texture));
    GX2SurfaceFormat gx2Format;


    _GX2InitTexture(texture, width, height, 1, 1, _TexFormatToGX2Format(format), GX2_SURFACE_DIM_TEXTURE_2D, GX2_TILE_MODE_LINEAR_ALIGNED);
    if(texture->surface.imageSize == 0)
    {
        CriticalErrorHandler("Failed to init texture");
        return nullptr;
    }
    // OSReport("Tex format: %04x Tilemode: %04x TGARes %d/%d SurfSize: 0x%x\n", (u32)texture->surface.format, (u32)texture->surface.tileMode, width, height, texture->surface.imageSize);
    _GX2AllocateTexture(texture);
    return texture;
}

// quick and dirty TGA loader. Not very safe
GX2Texture* LoadFromTGA(u8* data, u32 length)
{
    TGA_HEADER* tgaHeader = (TGA_HEADER*)data;

    u32 width = _swapU16(tgaHeader->width);
    u32 height = _swapU16(tgaHeader->height);

    GX2Texture* texture = _InitSpriteTexture(width, height, E_TEXFORMAT::RGBA8888_UNORM);
    for(u32 y=0; y<height; y++)
    {
        u32* tga_bgra_data = (u32*)(data+sizeof(TGA_HEADER)) + ((height - y - 1) * width);
        u32* out_data = (u32*)texture->surface.image + (y * texture->surface.pitch);
        for(u32 x=0; x<width; x++)
        {
            u32 c = *tga_bgra_data;
            *out_data = ((c & 0x00FF00FF)) | ((c & 0xFF000000) >> 16) | ((c & 0x0000FF00) << 16);
            tga_bgra_data++;
            out_data++;
        }
    }

    // todo - create texture with optimal tile format and use GX2CopySurface to convert from linear to tiled format
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU | GX2_INVALIDATE_MODE_TEXTURE, texture->surface.image, texture->surface.imageSize);
    return texture;
}

Sprite* Render::sBigFontTextureBlack = nullptr;
Sprite* Render::sBigFontTextureWhite = nullptr;
Sprite* Render::sSmallFontTextureBlack = nullptr;
Sprite* Render::sSmallFontTextureWhite = nullptr;

void Render::RenderText(u32 x, u32 y, u8 textSize, u8 blackLevel, const char* text, ...) {
    if (sBigFontTextureBlack == nullptr) {
        sBigFontTextureBlack = new Sprite("font/source-code-pro-black.tga", true);
        sBigFontTextureWhite = new Sprite("font/source-code-pro-white.tga", true);
        sSmallFontTextureBlack = new Sprite("font/source-code-pro-small-black.tga", true);
        sSmallFontTextureWhite = new Sprite("font/source-code-pro-small-white.tga", true);
    }

    va_list args;
    char maxTextBuffer[1920/14];
    va_start(args, text);
    vsnprintf(maxTextBuffer, sizeof(maxTextBuffer), text, args);
    va_end (args);

    Sprite* spriteUsed = nullptr;
    if (blackLevel != 0x00 && textSize != 0) {
        spriteUsed = sBigFontTextureBlack;
    }
    else if (blackLevel != 0x00 && textSize == 0) {
        spriteUsed = sSmallFontTextureBlack;
    }
    else if (blackLevel == 0x00 && textSize != 0) {
        spriteUsed = sBigFontTextureWhite;
    }
    else if (blackLevel == 0x00 && textSize == 0) {
        spriteUsed = sSmallFontTextureWhite;
    }

    uint32_t effectiveCharacterWidth = (textSize == 0 ? 14 : 26);
    uint32_t characterSize = (textSize == 0 ? 32 : 64);
    uint32_t textureDimensions = (textSize == 0 ? 512 : 1024);
    for (uint32_t i=0; i<strlen(maxTextBuffer); i++) {
        if (maxTextBuffer[i] == '\0') break;
        u32 letter = (u32)maxTextBuffer[i];
        letter -= 32;
        uint32_t bitmapX = (letter) % (textureDimensions/characterSize);
        uint32_t bitmapY = (letter) / (textureDimensions/characterSize);
        RenderSpritePortionScreenRelative(spriteUsed, x+(i*effectiveCharacterWidth), y, bitmapX*characterSize, bitmapY*characterSize, characterSize, characterSize);
    }
}
