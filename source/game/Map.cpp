#include "Map.h"
#include "../framework/render.h"

#include "../framework/fileformat/TGAFile.h"
#include <coreinit/debug.h>

MAP_PIXEL_TYPE _GeneratePixelAtWorldPos(s32 x, s32 y)
{
    f32 f = perlinNoise_2d((float)x * 0.017, (float)y * 0.017);

    f += abs(perlinNoise_2d((float)x * 0.00111 + 13.333f, (float)y * 0.00111));

    if( f <= 0.15 )
        return MAP_PIXEL_TYPE::SAND;

    return MAP_PIXEL_TYPE::AIR;
}

MAP_PIXEL_TYPE _GetPixelTypeFromTGAColor(u8* tgaPixel)
{
    u32 c = 0;
    c |= ((u32)tgaPixel[0] << 16);
    c |= ((u32)tgaPixel[1] << 8);
    c |= ((u32)tgaPixel[2] << 0);

    if(c == 0x7F3300)
        return MAP_PIXEL_TYPE::SOIL;
    if(c == 0xFFD800)
        return MAP_PIXEL_TYPE::SAND;

    return MAP_PIXEL_TYPE::AIR;
}

MapCell::MapCell(u32 cellX, u32 cellY) : m_cellX(cellX), m_cellY(cellY), m_posX(cellX * MAP_CELL_WIDTH), m_posY(cellY * MAP_CELL_HEIGHT)
{
    /*
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
    }*/
}

void MapCell::LoadCellFromTGA(class TGALoader& tgaLoader)
{
    if(tgaLoader.GetBPP() != 24)
        CriticalErrorHandler("LoadCellFromTGA - TGA not 24bit");

    s32 py = m_posY;
    PixelType* pOut = m_pixelArray;
    for (s32 y = 0; y < MAP_CELL_HEIGHT; y++)
    {
        //s32 px = m_posX;
        u8* tgaPixelIn = tgaLoader.GetData() + (py * tgaLoader.GetWidth() + m_posX) * 3;
        for (s32 x = 0; x < MAP_CELL_WIDTH; x++)
        {
            pOut->SetPixel(_GetPixelTypeFromTGAColor(tgaPixelIn));


            tgaPixelIn += 3;
            pOut++;
            //px++;
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
            // hacky multicolor test
            u8 rd = rand()%2;
            if(rd == 0)
                return 0xE9B356FF;
            //if(rd == 1)
            //    return 0xE8B666FF;
            return 0xEEC785FF;
        }
        case MAP_PIXEL_TYPE::SOIL:
        {
            u8 rd = rand()%2;
            if(rd == 0)
                return 0x5F3300FF;
            return 0x512B00FF;
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

void Map::Init(uint32_t width, uint32_t height)
{
    perlinNoise_init();
    m_cellsX = (width+MAP_CELL_WIDTH-1) / MAP_CELL_WIDTH;
    m_cellsY = (height+MAP_CELL_HEIGHT-1) / MAP_CELL_HEIGHT;
    m_cells.clear();
    for(u32 y=0; y<m_cellsY; y++)
    {
        for(u32 x=0; x<m_cellsX; x++)
        {
            m_cells.emplace_back(x, y);
        }
    }
    //GenerateTerrain();
}

Map::Map(const char* filename)
{
    OSReport("Level load - Phase 1\n");
    auto mapData = LoadFileToMem(std::string("level/").append(filename));
    if(mapData.empty())
        OSReport("Unable to load map file");
    OSReport("Level load - Phase 2\n");
    TGALoader tgaLoader;
    if(!tgaLoader.LoadFromTGA(mapData))
        CriticalErrorHandler("Unable to load level");
    OSReport("Level load - Phase 3 (Map size: %ux%u)\n", tgaLoader.GetWidth(), tgaLoader.GetHeight());
    Init(tgaLoader.GetWidth(), tgaLoader.GetHeight());
    OSReport("Level load - Phase 4\n");
    for(auto& it : m_cells)
        it.LoadCellFromTGA(tgaLoader);
    OSReport("Level load - Finished\n");
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
