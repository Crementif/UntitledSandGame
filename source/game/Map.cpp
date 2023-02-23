#include "Map.h"
#include "../framework/render.h"

#include <coreinit/debug.h>

MAP_PIXEL_TYPE _GeneratePixelAtWorldPos(s32 x, s32 y)
{
    f32 f = perlinNoise_2d((float)x * 0.017, (float)y * 0.017);

    f += abs(perlinNoise_2d((float)x * 0.00111 + 13.333f, (float)y * 0.00111));

    if( f <= 0.15 )
        return MAP_PIXEL_TYPE::SAND;

    return MAP_PIXEL_TYPE::AIR;
}

MapCell::MapCell(u32 cellX, u32 cellY) : m_cellX(cellX), m_cellY(cellY), m_posX(cellX * MAP_CELL_WIDTH), m_posY(cellY * MAP_CELL_HEIGHT)
{
    s32 py = m_posY;
    PixelType* pOut = m_pixelArray;
    for (s32 y = 0; y < MAP_CELL_HEIGHT; y++)
    {
        s32 px = m_posX;
        for (s32 x = 0; x < MAP_CELL_WIDTH; x++)
        {
            pOut->SetPixel(_GeneratePixelAtWorldPos(px, py));

            pOut++;
            px++;
        }
        py++;
    }
}

void MapCell::UpdateCell()
{

}

void MapCell::DrawCell()
{
    if(!m_cellSprite)
    {
        m_cellSprite = new Sprite(MAP_CELL_WIDTH, MAP_CELL_HEIGHT, false);
        RefreshCellTexture();
    }
    Render::RenderSprite(m_cellSprite, m_posX * MAP_PIXEL_ZOOM, m_posY * MAP_PIXEL_ZOOM, MAP_CELL_WIDTH * MAP_PIXEL_ZOOM, MAP_CELL_HEIGHT * MAP_PIXEL_ZOOM);
}

// format: RRGGBBAA
u32 _GetColorFromPixelType(PixelType& pixelType)
{
    auto matType = pixelType.GetPixelType();
    switch(matType)
    {
        case MAP_PIXEL_TYPE::AIR:
            return 0;
        case MAP_PIXEL_TYPE::SAND:
        {
            // hacky multi-color test
            u8 rd = rand()%2;
            if(rd == 0)
                return 0xE9B356FF;
            //if(rd == 1)
            //    return 0xE8B666FF;
            return 0xEEC785FF;
        }
    }
    return 0x12345678;
}

void MapCell::RefreshCellTexture()
{
    s32 idx;
    PixelType* pTypeIn = m_pixelArray;
    for(u32 y=0; y<MAP_CELL_HEIGHT; y++)
    {
        for(u32 x=0; x<MAP_CELL_WIDTH; x++)
        {
            m_cellSprite->SetPixel(x, y, _GetColorFromPixelType(*pTypeIn));
            pTypeIn++;
            idx++;
        }
    }
    m_cellSprite->FlushCache();
}

Map::Map()
{
    perlinNoise_init();
    m_cellsX = 16;
    m_cellsY = 8;
    m_cells.clear();
    for(u32 y=0; y<m_cellsY; y++)
    {
        for(u32 x=0; x<m_cellsX; x++)
        {
            m_cells.emplace_back(x, y);
        }
    }
    GenerateTerrain();
}

Map::~Map()
{

}

void Map::GenerateTerrain()
{
    // done in MapCell constructor atm

    //for(auto& it : m_pixelArray)
    //{
    //    it.SetPixel(MAP_PIXEL_TYPE::AIR);
    //}
}

void Map::Update()
{
    // limit to active cells?
    for(s32 y=0; y<(s32)m_cellsY; y++)
    {
        for(s32 x=0; x<(s32)m_cellsX; x++)
        {
            m_cells[x + y * m_cellsX].UpdateCell();
        }
    }
}

void Map::Draw()
{
    // todo - limit to visible cells
    for(s32 y=0; y<(s32)m_cellsY; y++)
    {
        for(s32 x=0; x<(s32)m_cellsX; x++)
        {
            m_cells[x + y * m_cellsX].DrawCell();
        }
    }
}
