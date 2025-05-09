#include "Map.h"
#include "MapPixels.h"
#include "../framework/render.h"
#include "../framework/window.h"
#include "../framework/debug.h"

#include "../framework/fileformat/TGAFile.h"
#include <coreinit/debug.h>
#include <gx2/texture.h>

#include <coreinit/debug.h>


// shaders
static ShaderSwitcher s_shaderDrawMap{"draw_map"};
static ShaderSwitcher s_shaderEnvironmentPass{"environment_pass"};
static ShaderSwitcher s_shaderEnvironmentPrepass{"environment_prepass"};

// framebuffers
static Framebuffer* s_pixelMap;
static Framebuffer* s_environmentPrepass;
static Framebuffer* s_environmentTempMap;
static Framebuffer* s_environmentMap;

// sprites
static Sprite* s_backgroundSprite;
static Sprite* s_dirtDetailSprite;
static Sprite* s_pixelColorLookupMap;


bool s_mapRenderingIsInitialized{false};

constexpr s32 MAP_RENDER_VIEW_BORDER = 8; // size of off-screen pixel border. Needed for postprocessing which reads across pixel boundaries

// quad
alignas(256) u16 s_idx_data[] =
{
    0, 1, 2, 2, 1, 3
};

alignas(256) u32 s_mapDrawUFVertex[4 * 4 + 4] =
{
    // [0]: 2 channels xy pos, 2 channels uv coordinates
    0, 0, 0, 0,
    // [1]: 2 channels xy pos, 2 channels uv coordinates
    0, 0, 0, 0,
    // [2]: 2 channels xy pos, 2 channels uv coordinates
    0, 0, 0, 0,
    // [3]: 2 channels xy pos, 2 channels uv coordinates
    0, 0, 0, 0,

    // [4]: render bounds top left xy, render bounds size xy
    0, 0, 0, 0,
};

alignas(256) u32 s_mapDrawUFPixel[4] =
{
    // [0]: time, 0, 0, 0
};

alignas(256) u32 s_envBlurUFPixelHorizontal[4] =
{
    // [0]: pixelSizeX, pixelSizeY, isVertical, 0
};

alignas(256) u32 s_envBlurUFPixelVertical[4] =
{
    // [0]: pixelSizeX, pixelSizeY, isVertical, 0
};

inline u32 EndianSwap_F32(f32 v)
{
    return std::byteswap(*(u32*)&v);
};

extern GX2Sampler sRenderBaseSampler1_nearest;
extern GX2Sampler sRenderBaseSampler1_linear;
extern GX2Sampler sRenderBaseSampler1_linear_repeat;

static uint64_t launchTime = OSGetTime();

void UpdateMapDrawUniform(f32 xOffset, f32 yOffset, AABB ws_cameraBounds, f32 ws_worldWidth, f32 ws_worldHeight)
{
    const f32 pixelWidth = Framebuffer::GetCurrentPixelWidth()*2.0f;
    const f32 pixelHeight = Framebuffer::GetCurrentPixelHeight()*2.0f;

    // render bounds = camera bounds but includes overshoot (MAP_RENDER_VIEW_BORDER) to allow for postprocessing (lava glow) beyond screen edges
    AABB ws_renderBounds = AABB(ws_cameraBounds.pos.x - (f32)(MAP_RENDER_VIEW_BORDER * MAP_PIXEL_ZOOM), ws_cameraBounds.pos.y - (f32)(MAP_RENDER_VIEW_BORDER * MAP_PIXEL_ZOOM), ws_cameraBounds.scale.x + (f32)(MAP_RENDER_VIEW_BORDER * MAP_PIXEL_ZOOM) * 2.0f, ws_cameraBounds.scale.y + (f32)(MAP_RENDER_VIEW_BORDER * MAP_PIXEL_ZOOM) * 2.0f);
    AABB px_renderBounds = ws_renderBounds / (f32)MAP_PIXEL_ZOOM;

    // since the border is the same width around the image, the easiest way to render the pixel map is to center it on the screen
    const f32 x1 = -pixelWidth * (f32)((int)(px_renderBounds.scale.x/2)) * (f32)MAP_PIXEL_ZOOM;
    const f32 y1 = -pixelHeight * (f32)((int)(px_renderBounds.scale.y/2)) * (f32)MAP_PIXEL_ZOOM;

    const f32 x2 = x1 + pixelWidth * (f32)px_renderBounds.scale.x * (f32)MAP_PIXEL_ZOOM;
    const f32 y2 = y1 + pixelHeight * (f32)px_renderBounds.scale.y * (f32)MAP_PIXEL_ZOOM;

    const f32 uvX1 = 0.0f;
    const f32 uvX2 = 1.0f;
    const f32 uvY1 = 1.0f;
    const f32 uvY2 = 0.0f;

    // pos
    s_mapDrawUFVertex[0 * 4 + 0] = EndianSwap_F32(x1 + xOffset);
    s_mapDrawUFVertex[0 * 4 + 1] = EndianSwap_F32(y1 + yOffset);
    s_mapDrawUFVertex[1 * 4 + 0] = EndianSwap_F32(x1 + xOffset);
    s_mapDrawUFVertex[1 * 4 + 1] = EndianSwap_F32(y2 + yOffset);
    s_mapDrawUFVertex[2 * 4 + 0] = EndianSwap_F32(x2 + xOffset);
    s_mapDrawUFVertex[2 * 4 + 1] = EndianSwap_F32(y1 + yOffset);
    s_mapDrawUFVertex[3 * 4 + 0] = EndianSwap_F32(x2 + xOffset);
    s_mapDrawUFVertex[3 * 4 + 1] = EndianSwap_F32(y2 + yOffset);
    // uv
    s_mapDrawUFVertex[0 * 4 + 2] = EndianSwap_F32(uvX1);
    s_mapDrawUFVertex[0 * 4 + 3] = EndianSwap_F32(uvY1);
    s_mapDrawUFVertex[1 * 4 + 2] = EndianSwap_F32(uvX1);
    s_mapDrawUFVertex[1 * 4 + 3] = EndianSwap_F32(uvY2);
    s_mapDrawUFVertex[2 * 4 + 2] = EndianSwap_F32(uvX2);
    s_mapDrawUFVertex[2 * 4 + 3] = EndianSwap_F32(uvY1);
    s_mapDrawUFVertex[3 * 4 + 2] = EndianSwap_F32(uvX2);
    s_mapDrawUFVertex[3 * 4 + 3] = EndianSwap_F32(uvY2);
    // normalized render bounds (divided by map/world size)
    s_mapDrawUFVertex[4 * 4 + 0] = EndianSwap_F32(ws_renderBounds.GetTopLeft().x / ws_worldWidth);
    s_mapDrawUFVertex[4 * 4 + 1] = EndianSwap_F32(ws_renderBounds.GetTopLeft().y / ws_worldHeight);
    s_mapDrawUFVertex[4 * 4 + 2] = EndianSwap_F32(ws_renderBounds.scale.x / ws_worldWidth);
    s_mapDrawUFVertex[4 * 4 + 3] = EndianSwap_F32(ws_renderBounds.scale.y / ws_worldHeight);
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU | GX2_INVALIDATE_MODE_UNIFORM_BLOCK, s_mapDrawUFVertex, sizeof(s_mapDrawUFVertex));

    // update pixel uniform
    uint64_t time64 = OSTicksToMilliseconds((OSGetTime() - launchTime));
    float time = (float)time64;
    time *= 0.001f;
    s_mapDrawUFPixel[0] = EndianSwap_F32(time);
    GX2Invalidate(GX2_INVALIDATE_MODE_CPU | GX2_INVALIDATE_MODE_UNIFORM_BLOCK, s_mapDrawUFPixel, sizeof(s_mapDrawUFPixel));
}

class MapRenderManager
{
public:
    static void Init()
    {
        s_backgroundSprite = new Sprite("/tex/background_tile_a.tga", false);
        s_dirtDetailSprite = new Sprite("/tex/dirt_detail.tga", false);

        // calculate how many pixels will be visible
        s32 visibleWorldPixelsX = (WindowGetWidth() + (MAP_PIXEL_ZOOM-1)) / MAP_PIXEL_ZOOM;
        s32 visibleWorldPixelsY = (WindowGetHeight() + (MAP_PIXEL_ZOOM-1)) / MAP_PIXEL_ZOOM;

        s32 pixelMapWidth = visibleWorldPixelsX + MAP_RENDER_VIEW_BORDER * 2;
        s32 pixelMapHeight = visibleWorldPixelsY + MAP_RENDER_VIEW_BORDER * 2;

        s_pixelMap = new Framebuffer();
        s_pixelMap->SetColorBuffer(0, pixelMapWidth, pixelMapHeight, E_TEXFORMAT::RGBA8888_UNORM, 1, true, true);

        s_environmentPrepass = new Framebuffer();
        s_environmentPrepass->SetColorBuffer(0, pixelMapWidth, pixelMapHeight, E_TEXFORMAT::RG88_UNORM, 2, true, true);

        s_environmentTempMap = new Framebuffer();
        s_environmentTempMap->SetColorBuffer(0, pixelMapWidth, pixelMapHeight, E_TEXFORMAT::RG88_UNORM, 3, true, true);

        s_environmentMap = new Framebuffer();
        s_environmentMap->SetColorBuffer(0, pixelMapWidth, pixelMapHeight, E_TEXFORMAT::RG88_UNORM, 4, true, true);

        InitPixelColorLookupMap();
    }

    static void DrawBackground()
    {
        Vector2f camPos = Render::GetCameraPosition();
        s32 bgTileOffsetX = ((s32)camPos.x-299) / 300;
        s32 bgTileOffsetY = ((s32)camPos.y-299) / 300;

        for(s32 y=0; y<6; y++)
        {
            for(s32 x=0; x<12; x++)
            {
                Render::RenderSprite(s_backgroundSprite, (bgTileOffsetX + x) * 300, (bgTileOffsetY + y) * 300, s_backgroundSprite->GetWidth()*MAP_PIXEL_ZOOM, s_backgroundSprite->GetHeight()*MAP_PIXEL_ZOOM);
            }
        }
    }

    // draw pixel types to a texture for further processing
    // the texture covers a slightly larger area than the screen to allow for postprocessing beyond screen edges
    static void UpdatePixelMap(Map* map, const Rect2D& visibleCells)
    {
        Vector2f cameraPosition = Render::GetUnfilteredCameraPosition();
        Render::SetCameraPosition(cameraPosition / (f32)MAP_PIXEL_ZOOM);

        s_pixelMap->Apply();
        DebugWaitAndMeasureGPUDone("[GPU] Map::UpdatePixelMap::ApplyPixelMap");

        MapCell* cellArray = map->GetCellPtrArray();
        s32 cellsX = (s32)map->GetCellsX();
        s32 cellsY = (s32)map->GetCellsY();

        for(s32 y=visibleCells.y; y<(visibleCells.y + visibleCells.height); y++)
        {
            for(s32 x=visibleCells.x; x<(visibleCells.x + visibleCells.width); x++)
            {
                cellArray[x + y * cellsX].FlushDrawCache();
                cellArray[x + y * cellsX].DrawCell();
            }
        }
        DebugWaitAndMeasureGPUDone("[GPU] Map::UpdatePixelMap::DrawCells");
        // restore camera
        Render::SetCameraPosition(cameraPosition);
    }

    // handle lava glow and light obstruction by preprocessing the pixel map
    static void DoEnvironmentPass()
    {
        // prepass to turn map pixels into two channels:
        // - lava pixels in the red channel
        // - solid objects (light obstructions) in the green channel (for ambient occlusion)
        s_environmentPrepass->Apply();
        DebugWaitAndMeasureGPUDone("[GPU] Map::DoEnvironmentPass::Prepass::Apply");

        s_shaderEnvironmentPrepass.Activate();

        GX2SetPixelTexture(s_pixelMap->GetColorBufferTexture(0), 0);
        GX2SetPixelSampler(&sRenderBaseSampler1_linear, 0);

        GX2DrawIndexedEx(GX2_PRIMITIVE_MODE_TRIANGLES, 6, GX2_INDEX_TYPE_U16, s_idx_data, 0, 1); // environment_prepass.ps
        DebugWaitAndMeasureGPUDone("[GPU] Map::DoEnvironmentPass::Prepass::DrawQuad");

        // do environment blur/bloom-like pass to create a glow effect around lava pixels and blur solids (light obstructions) to create ambient occlusion
        s_shaderEnvironmentPass.Activate();

        // horizontal pass
        s_environmentTempMap->Apply();
        DebugWaitAndMeasureGPUDone("[GPU] Map::DoEnvironmentPass::HorizontalPass::Apply");

        GX2SetPixelTexture(s_environmentPrepass->GetColorBufferTexture(0), 0);
        GX2SetPixelSampler(&sRenderBaseSampler1_linear, 0);

        s_envBlurUFPixelHorizontal[0] = EndianSwap_F32(1.0f / s_environmentPrepass->GetColorBufferTexture(0)->surface.width);
        s_envBlurUFPixelHorizontal[1] = EndianSwap_F32(1.0f / s_environmentPrepass->GetColorBufferTexture(0)->surface.height);
        s_envBlurUFPixelHorizontal[2] = 0;
        GX2SetPixelUniformBlock(0, sizeof(s_envBlurUFPixelHorizontal), s_envBlurUFPixelHorizontal);
        GX2DrawIndexedEx(GX2_PRIMITIVE_MODE_TRIANGLES, 6, GX2_INDEX_TYPE_U16, s_idx_data, 0, 1); // environment_pass.ps
        DebugWaitAndMeasureGPUDone("[GPU] Map::DoEnvironmentPass::HorizontalPass::DrawQuad");

        // vertical pass
        s_environmentMap->Apply();
        DebugWaitAndMeasureGPUDone("[GPU] Map::DoEnvironmentPass::VerticalPass::Apply");

        GX2SetPixelTexture(s_environmentTempMap->GetColorBufferTexture(0), 0);
        GX2SetPixelSampler(&sRenderBaseSampler1_linear, 0);

        s_envBlurUFPixelVertical[0] = EndianSwap_F32(1.0f / s_environmentTempMap->GetColorBufferTexture(0)->surface.width);
        s_envBlurUFPixelVertical[1] = EndianSwap_F32(1.0f / s_environmentTempMap->GetColorBufferTexture(0)->surface.height);
        s_envBlurUFPixelVertical[2] = 1;
        GX2SetPixelUniformBlock(0, sizeof(s_envBlurUFPixelVertical), s_envBlurUFPixelVertical);
        GX2DrawIndexedEx(GX2_PRIMITIVE_MODE_TRIANGLES, 6, GX2_INDEX_TYPE_U16, s_idx_data, 0, 1); // environment_pass.ps
        DebugWaitAndMeasureGPUDone("[GPU] Map::DoEnvironmentPass::VerticalPass::DrawQuad");
    }

    // draw map pixels to screen using m_pixelMap
    static void DrawPixelsToScreen(Map *map)
    {
        Framebuffer::ApplyBackbuffer();
        // draw fullscreen quad
        s_shaderDrawMap.Activate();

        GX2Texture* pixelMapTexture = s_pixelMap->GetColorBufferTexture(0);
        GX2SetPixelTexture(pixelMapTexture, 0);
        GX2SetPixelSampler(&sRenderBaseSampler1_nearest, 0);
        GX2SetPixelTexture(s_environmentMap->GetColorBufferTexture(0), 1);
        GX2SetPixelSampler(&sRenderBaseSampler1_linear, 1);
        GX2SetPixelTexture(s_pixelColorLookupMap->GetTexture(), 2);
        GX2SetPixelSampler(&sRenderBaseSampler1_nearest, 2);
        GX2SetPixelTexture(s_dirtDetailSprite->GetTexture(), 3);
        GX2SetPixelSampler(&sRenderBaseSampler1_linear_repeat, 3);

        // some math magic to allow for 1-pixel-precise scrolling regardless of zoom factor
        const f32 pixelWidth = Framebuffer::GetCurrentPixelWidth()*2.0f;
        const f32 pixelHeight = Framebuffer::GetCurrentPixelHeight()*2.0f;
        Vector2f camPos = Render::GetCameraPosition();
        s32 pixelX = (s32)camPos.x;
        s32 pixelY = (s32)camPos.y;
        s32 subscrollX = pixelX % MAP_PIXEL_ZOOM;
        s32 subscrollY = pixelY % MAP_PIXEL_ZOOM;
        s32 gridOffsetX = (s32)(pixelWidth * (f32)MAP_PIXEL_ZOOM * (f32)subscrollX);
        s32 gridOffsetY = (s32)(pixelHeight * (f32)MAP_PIXEL_ZOOM * (f32)subscrollY);

        s32 pixelMapWidth = pixelMapTexture->surface.width;
        s32 pixelMapHeight = pixelMapTexture->surface.height;
        f32 pixelMapPixelWidth = 1.0f / pixelMapWidth;
        f32 pixelMapPixelHeight = 1.0f / pixelMapHeight;

        AABB cameraBounds = AABB(Render::GetCameraPosition().x, Render::GetCameraPosition().y, (f32)Render::GetScreenSize().x, (f32)Render::GetScreenSize().y);

        UpdateMapDrawUniform(gridOffsetX, gridOffsetY, cameraBounds, (f32)map->GetPixelWidth() * MAP_PIXEL_ZOOM, (f32)map->GetPixelHeight() * MAP_PIXEL_ZOOM);

        RenderState::SetShaderMode(GX2_SHADER_MODE_UNIFORM_BLOCK);
        RenderState::SetTransparencyMode(RenderState::E_TRANSPARENCY_MODE::ADDITIVE);

        GX2SetVertexUniformBlock(0, sizeof(s_mapDrawUFVertex), s_mapDrawUFVertex);
        GX2SetPixelUniformBlock(0, sizeof(s_mapDrawUFPixel), s_mapDrawUFPixel);

        GX2DrawIndexedEx(GX2_PRIMITIVE_MODE_TRIANGLES, 6, GX2_INDEX_TYPE_U16, (void*)s_idx_data, 0, 1);

        DebugWaitAndMeasureGPUDone("[GPU] Map::DrawPixelsToScreen");
    }

private:
    static void InitPixelColorLookupMap()
    {
        s_pixelColorLookupMap = new Sprite(32, 32, false, E_TEXFORMAT::RGB565_UNORM);
        s_pixelColorLookupMap->Clear(0x00000000);

        s32 selectedMaterialIndex = 0;
        auto SelectMat = [&](MAP_PIXEL_TYPE materialIndex) { selectedMaterialIndex = static_cast<u32>(materialIndex); };

        auto SetMatColor = [&](s32 seedIndex, u32 color)
        {
            s_pixelColorLookupMap->SetPixelRGB565(seedIndex, selectedMaterialIndex, color);
        };
        auto GenerateDimmerMatColor = [&](s32 seedIndex, u32 color, float seedDimFactor)
        {
            // dim the color
            const u8 r = (u8)((color & 0xFF000000) >> 24) * seedDimFactor;
            const u8 g = (u8)((color & 0x00FF0000) >> 16) * seedDimFactor;
            const u8 b = (u8)((color & 0x0000FF00) >> 8) * seedDimFactor;
            const u32 dimmedColor = (r << 24) | (g << 16) | (b << 8) | 0xFF;

            s_pixelColorLookupMap->SetPixelRGB565(seedIndex, selectedMaterialIndex, dimmedColor);
        };
        constexpr float dimFactor = 0.85f;

        // Setting AIR color
        SelectMat(MAP_PIXEL_TYPE::AIR);
        SetMatColor(0, 0);

        // Setting SAND colors
        SelectMat(MAP_PIXEL_TYPE::SAND);
        SetMatColor(0, 0xE9B356FF);
        SetMatColor(1, 0xEEC785FF);
        SetMatColor(2, 0xDEB56FFF);
        GenerateDimmerMatColor(3, 0xE9B356FF, dimFactor);
        GenerateDimmerMatColor(4, 0xEEC785FF, dimFactor);
        GenerateDimmerMatColor(5, 0xDEB56FFF, dimFactor);

        // Setting SOIL colors
        SelectMat(MAP_PIXEL_TYPE::SOIL);
        SetMatColor(0, 0x5F3300FF);
        SetMatColor(1, 0x512B00FF);
        SetMatColor(2, 0x472600FF);
        GenerateDimmerMatColor(3, 0x5F3300FF, dimFactor);
        GenerateDimmerMatColor(4, 0x512B00FF, dimFactor);
        GenerateDimmerMatColor(5, 0x472600FF, dimFactor);

        // Setting GRASS colors
        SelectMat(MAP_PIXEL_TYPE::GRASS);
        SetMatColor(0, 0x2F861FFF);
        SetMatColor(1, 0x3E932BFF);
        SetMatColor(2, 0x3D9629FF);
        GenerateDimmerMatColor(3, 0x2F861FFF, dimFactor);
        GenerateDimmerMatColor(4, 0x3E932BFF, dimFactor);
        GenerateDimmerMatColor(5, 0x3D9629FF, dimFactor);

        // Setting LAVA colors
        SelectMat(MAP_PIXEL_TYPE::LAVA);
        SetMatColor(0, 0xFF0000FF);
        SetMatColor(1, 0xFF2B00FF);
        SetMatColor(2, 0xFF6A00FF);
        SetMatColor(3, 0xFA2F19FF);

        // Setting ROCK colors
        SelectMat(MAP_PIXEL_TYPE::ROCK);
        SetMatColor(0, 0x515152FF);
        SetMatColor(1, 0x484849FF);
        SetMatColor(2, 0x484849FF);
        SetMatColor(3, 0x484849FF);
        SetMatColor(4, 0x414142FF);
        SetMatColor(5, 0x414142FF);
        SetMatColor(6, 0x414142FF);
        SetMatColor(7, 0x414142FF);
        SetMatColor(8, 0x414142FF);
        GenerateDimmerMatColor(9, 0x515152FF, dimFactor);
        GenerateDimmerMatColor(10, 0x484849FF, dimFactor);
        GenerateDimmerMatColor(11, 0x484849FF, dimFactor);
        GenerateDimmerMatColor(12, 0x484849FF, dimFactor);
        GenerateDimmerMatColor(13, 0x414142FF, dimFactor);
        GenerateDimmerMatColor(14, 0x414142FF, dimFactor);
        GenerateDimmerMatColor(15, 0x414142FF, dimFactor);
        GenerateDimmerMatColor(16, 0x414142FF, dimFactor);
        GenerateDimmerMatColor(17, 0x414142FF, dimFactor);

        // Setting SMOKE colors
        SelectMat(MAP_PIXEL_TYPE::SMOKE);
        SetMatColor(0, 0x30303080);
        SetMatColor(1, 0x50505080);
        SetMatColor(2, 0x54545480);

        s_pixelColorLookupMap->FlushCache();
    }
};

Rect2D _GetVisibleCells(Map* map)
{
    // get visible area
    Vector2f camPos = Render::GetCameraPosition();
    Vector2f camTopLeft = camPos - Vector2f(1.0f, 1.0f);
    camTopLeft = camTopLeft / MAP_PIXEL_ZOOM;
    Vector2i screenSize = Render::GetScreenSize();
    Vector2f camBottomRight = camPos + Vector2f((f32)screenSize.x + 1.0f, (f32)screenSize.y + 1.0f);
    camBottomRight = camBottomRight / MAP_PIXEL_ZOOM;

    s32 cx1 = (s32)camTopLeft.x / MAP_CELL_WIDTH;
    s32 cy1 = (s32)camTopLeft.y / MAP_CELL_HEIGHT;
    s32 cx2 = ((s32)camBottomRight.x + screenSize.x +  MAP_CELL_WIDTH) / MAP_CELL_WIDTH;
    s32 cy2 = ((s32)camBottomRight.y + screenSize.y + MAP_CELL_HEIGHT) / MAP_CELL_HEIGHT;

    s32 cellsX = (s32)map->GetCellsX();
    s32 cellsY = (s32)map->GetCellsY();

    cx1 = std::clamp<s32>(cx1, 0, cellsX-1);
    cy1 = std::clamp<s32>(cy1, 0, cellsY-1);
    cx2 = std::clamp<s32>(cx2, 0, cellsX-1);
    cy2 = std::clamp<s32>(cy2, 0, cellsY-1);

    return {cx1, cy1, (cx2-cx1 + 1), (cy2-cy1 + 1)};
}

void Map::Draw()
{
    if(!s_mapRenderingIsInitialized)
    {
        MapRenderManager::Init();
        s_mapRenderingIsInitialized = true;
    }

    DebugWaitAndMeasureGPUDone("[GPU] Map::DrawStart");
    MapRenderManager::DrawBackground();
    DebugWaitAndMeasureGPUDone("[GPU] Map::DrawBackground");

    // get visible area
    Rect2D visibleCells = _GetVisibleCells(this);
    MapRenderManager::UpdatePixelMap(this, visibleCells);
    MapRenderManager::DoEnvironmentPass();
    MapRenderManager::DrawPixelsToScreen(this);
}

void MapCell::FlushDrawCache()
{
    m_cellSprite->FlushCache();
    // workaround necessary for Cemu
    // Cemu texture invalidation heuristics may not always catch pixel updates
    // so we modify the top left pixel (usually guaranteed to be checked) to coerce an invalidation
    u8* pixData = (u8*)m_cellSprite->GetTexture()->surface.image;
    pixData[3] ^= 1;
}

void MapCell::DrawCell()
{
    // manually shift cell render position to offset pixelmap border
    // The screen's top left 0/0 is at MAP_RENDER_VIEW_BORDER/MAP_RENDER_VIEW_BORDER
    s32 x = m_posX + MAP_RENDER_VIEW_BORDER;
    s32 y = m_posY + MAP_RENDER_VIEW_BORDER;
    Render::RenderSprite(m_cellSprite, x, y, MAP_CELL_WIDTH, MAP_CELL_HEIGHT);
}

void MapCell::RefreshCellTexture()
{
    s32 idx;
    PixelType* pTypeIn = m_pixelArray;
    for(u32 y=0; y<MAP_CELL_HEIGHT; y++)
    {
        for(u32 x=0; x<MAP_CELL_WIDTH; x++)
        {
            m_cellSprite->SetPixelRG88(x, y, pTypeIn->CalculatePixelColor());
            pTypeIn++;
            idx++;
        }
    }
    m_cellSprite->FlushCache();
}
