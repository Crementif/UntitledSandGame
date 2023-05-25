#include "Map.h"
#include "MapPixels.h"
#include "../framework/render.h"
#include "../framework/window.h"

#include "../framework/fileformat/TGAFile.h"
#include <coreinit/debug.h>
#include <gx2/texture.h>

#include <coreinit/debug.h>

bool s_mapRenderingIsIntialized{false};

constexpr s32 MAP_RENDER_VIEW_BORDER = 8; // size of off-screen pixel border. Needed for postprocessing which reads across pixel boundaries

// quad
constexpr static __attribute__ ((aligned (32))) u16 s_idx_data[] =
{
        0, 1, 2, 2, 1, 3
};

__attribute__ ((aligned (64))) u32 s_mapDrawUF[4*4] =
{
    // 2 channels xy pos, 2 channels uv coordinates
    0
};

inline u32 EndianSwap_F32(f32 v)
{
    return __builtin_bswap32(*(u32*)&v);
}

extern GX2Sampler sRenderBaseSampler1_nearest;

void UpdateMapDrawUniform(s32 pixelMapWidth, s32 pixelMapHeight, f32 xOffset, f32 yOffset)
{
    const f32 pixelWidth = Framebuffer::GetCurrentPixelWidth()*2.0f;
    const f32 pixelHeight = Framebuffer::GetCurrentPixelHeight()*2.0f;

    // note that pixelMapWidth already includes MAP_RENDER_VIEW_BORDER
    // since the border is the same width around the image, the easiest way to render the pixelmap is to center it on the screen
    const f32 x1 = -pixelWidth * (f32)((int)(pixelMapWidth/2)) * (f32)MAP_PIXEL_ZOOM;
    const f32 y1 = -pixelHeight * (f32)((int)(pixelMapHeight/2)) * (f32)MAP_PIXEL_ZOOM;

    const f32 x2 = x1 + pixelWidth * (f32)pixelMapWidth * (f32)MAP_PIXEL_ZOOM;
    const f32 y2 = y1 + pixelHeight * (f32)pixelMapHeight * (f32)MAP_PIXEL_ZOOM;

    const f32 uvX1 = 0.0f;
    const f32 uvX2 = 1.0f;
    const f32 uvY1 = 1.0f;
    const f32 uvY2 = 0.0f;

    // pos
    s_mapDrawUF[0*4 + 0] = EndianSwap_F32(x1 + xOffset);
    s_mapDrawUF[0*4 + 1] = EndianSwap_F32(y1 + yOffset);
    s_mapDrawUF[1*4 + 0] = EndianSwap_F32(x1 + xOffset);
    s_mapDrawUF[1*4 + 1] = EndianSwap_F32(y2 + yOffset);
    s_mapDrawUF[2*4 + 0] = EndianSwap_F32(x2 + xOffset);
    s_mapDrawUF[2*4 + 1] = EndianSwap_F32(y1 + yOffset);
    s_mapDrawUF[3*4 + 0] = EndianSwap_F32(x2 + xOffset);
    s_mapDrawUF[3*4 + 1] = EndianSwap_F32(y2 + yOffset);
    // uv
    s_mapDrawUF[0*4 + 2] = EndianSwap_F32(uvX1);
    s_mapDrawUF[0*4 + 3] = EndianSwap_F32(uvY1);
    s_mapDrawUF[1*4 + 2] = EndianSwap_F32(uvX1);
    s_mapDrawUF[1*4 + 3] = EndianSwap_F32(uvY2);
    s_mapDrawUF[2*4 + 2] = EndianSwap_F32(uvX2);
    s_mapDrawUF[2*4 + 3] = EndianSwap_F32(uvY1);
    s_mapDrawUF[3*4 + 2] = EndianSwap_F32(uvX2);
    s_mapDrawUF[3*4 + 3] = EndianSwap_F32(uvY2);

    GX2Invalidate(GX2_INVALIDATE_MODE_CPU | GX2_INVALIDATE_MODE_UNIFORM_BLOCK, s_mapDrawUF, sizeof(s_mapDrawUF));
}

static ShaderSwitcher s_shaderDrawMap{"draw_map"};

class MapRenderManager
{
public:
    static void Init()
    {
        m_backgroundSprite = new Sprite("/tex/background_tile_a.tga", false);

        // calculate how many pixels will be visible
        s32 visibleWorldPixelsX = (WindowGetWidth() + (MAP_PIXEL_ZOOM-1)) / MAP_PIXEL_ZOOM;
        s32 visibleWorldPixelsY = (WindowGetHeight() + (MAP_PIXEL_ZOOM-1)) / MAP_PIXEL_ZOOM;

        m_pixelMap = new Framebuffer();
        m_pixelMap->SetColorBuffer(0, visibleWorldPixelsX + MAP_RENDER_VIEW_BORDER * 2, visibleWorldPixelsY + MAP_RENDER_VIEW_BORDER * 2, E_TEXFORMAT::RG88_UNORM);

        InitPixelColorLookupMap();
    }

    static void Postprocess()
    {

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
                Render::RenderSprite(m_backgroundSprite, (bgTileOffsetX + x) * 300, (bgTileOffsetY + y) * 300, m_backgroundSprite->GetWidth()*MAP_PIXEL_ZOOM, m_backgroundSprite->GetHeight()*MAP_PIXEL_ZOOM);
            }
        }
    }

    // draw pixel types to a texture for further processing
    // the texture covers a slightly larger area than the screen to allow for postprocessing beyond screen edges
    static void UpdatePixelMap(Map* map, const Rect2D& visibleCells)
    {
        Vector2f cameraPosition = Render::GetUnfilteredCameraPosition();
        Render::SetCameraPosition(cameraPosition / (f32)MAP_PIXEL_ZOOM);

        m_pixelMap->Apply();

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

        Framebuffer::ApplyBackbuffer();
        // restore camera
        Render::SetCameraPosition(cameraPosition);
    }

    // draw map pixels to screen using m_pixelMap
    static void DrawPixelsToScreen2(Map* map)
    {
       // draw fullscreen quad
        s_shaderDrawMap.Activate();

        GX2Texture* pixelMapTexture = m_pixelMap->GetColorBufferTexture(0);
        GX2SetPixelTexture(pixelMapTexture, 0);
        GX2SetPixelSampler(&sRenderBaseSampler1_nearest, 0);
        GX2SetPixelTexture(m_pixelColorLookupMap->GetTexture(), 1);
        GX2SetPixelSampler(&sRenderBaseSampler1_nearest, 1);

        // some math magic to allow for 1-pixel-precise scrolling regardless of zoom factor
        const f32 pixelWidth = Framebuffer::GetCurrentPixelWidth()*2.0f;
        const f32 pixelHeight = Framebuffer::GetCurrentPixelHeight()*2.0f;
        Vector2f camPos = Render::GetCameraPosition();
        s32 pixelX = (s32)camPos.x;
        s32 pixelY = (s32)camPos.y;
        s32 subscrollX = pixelX % MAP_PIXEL_ZOOM;
        s32 subscrollY = pixelY % MAP_PIXEL_ZOOM;
        f32 xOffset = pixelWidth * -(f32)subscrollX * 0.66f; // why 0.66?
        f32 yOffset = pixelHeight * 0.5f * (f32)subscrollY;

        s32 pixelMapWidth = pixelMapTexture->surface.width;
        s32 pixelMapHeight = pixelMapTexture->surface.height;
        f32 pixelMapPixelWidth = 1.0f / pixelMapWidth;
        f32 pixelMapPixelHeight = 1.0f / pixelMapHeight;
        UpdateMapDrawUniform(pixelMapWidth, pixelMapHeight, xOffset, yOffset);

        GX2SetVertexUniformBlock(0, sizeof(s_mapDrawUF), s_mapDrawUF);
        GX2DrawIndexedEx(GX2_PRIMITIVE_MODE_TRIANGLES, 6, GX2_INDEX_TYPE_U16, (void*)s_idx_data, 0, 1);
    }

private:
    static void InitPixelColorLookupMap()
    {
        m_pixelColorLookupMap = new Sprite(32, 32, false, E_TEXFORMAT::RGBA8888_UNORM);
        m_pixelColorLookupMap->Clear(0x00000000);

        s32 selectedMaterialIndex = 0;
        auto SelectMat = [&](MAP_PIXEL_TYPE materialIndex) { selectedMaterialIndex = static_cast<u32>(materialIndex); };

        auto SetMatColor = [&](s32 seedIndex, u32 color)
        {
            m_pixelColorLookupMap->SetPixelRGBA8888(seedIndex , selectedMaterialIndex, color);
        };

        // Setting AIR color
        SelectMat(MAP_PIXEL_TYPE::AIR);
        SetMatColor(0, 0);

        // Setting SAND colors
        SelectMat(MAP_PIXEL_TYPE::SAND);
        SetMatColor(0, 0xE9B356FF);
        SetMatColor(1, 0xEEC785FF);

        // Setting SOIL colors
        SelectMat(MAP_PIXEL_TYPE::SOIL);
        SetMatColor(0, 0x5F3300FF);
        SetMatColor(1, 0x512B00FF);

        // Setting GRASS colors
        SelectMat(MAP_PIXEL_TYPE::GRASS);
        SetMatColor(0, 0x2F861FFF);
        SetMatColor(1, 0x3E932BFF);

        // Setting LAVA colors
        SelectMat(MAP_PIXEL_TYPE::LAVA);
        SetMatColor(0, 0xFF0000FF);
        SetMatColor(1, 0xFF2B00FF);
        SetMatColor(2, 0xFF6A00FF);

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

        // Setting SMOKE colors
        SelectMat(MAP_PIXEL_TYPE::SMOKE);
        SetMatColor(0, 0x30303080);
        SetMatColor(1, 0x50505080);

        m_pixelColorLookupMap->FlushCache();
    }

private:
    static inline Framebuffer* m_pixelMap;
    static inline Sprite* m_backgroundSprite;
    static inline Sprite* m_pixelColorLookupMap;
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
    if(!s_mapRenderingIsIntialized)
    {
        MapRenderManager::Init();
        s_mapRenderingIsIntialized = true;
    }

    MapRenderManager::DrawBackground();

    // get visible area
    Rect2D visibleCells = _GetVisibleCells(this);

    MapRenderManager::UpdatePixelMap(this, visibleCells);
    MapRenderManager::DrawPixelsToScreen2(this);
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
            m_cellSprite->SetPixelRG88(x, y, _GetColorFromPixelType(*pTypeIn));
            pTypeIn++;
            idx++;
        }
    }
    m_cellSprite->FlushCache();
}
