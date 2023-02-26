#include "Map.h"
#include "MapPixels.h"
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

MAP_PIXEL_TYPE _GetPixelTypeFromTGAColor(u32 c)
{
    if(c == 0x7F3300)
        return MAP_PIXEL_TYPE::SOIL;
    if(c == 0xFFD800)
        return MAP_PIXEL_TYPE::SAND;
    if(c == 0x007F0E)
        return MAP_PIXEL_TYPE::GRASS;

    return MAP_PIXEL_TYPE::AIR;
}

MapCell::MapCell(Map* map, u32 cellX, u32 cellY) : m_cellX(cellX), m_cellY(cellY), m_posX(cellX * MAP_CELL_WIDTH), m_posY(cellY * MAP_CELL_HEIGHT), m_map(map)
{
    m_cellSprite = new Sprite(MAP_CELL_WIDTH, MAP_CELL_HEIGHT, false);
    m_cellSprite->SetupSampler(false);

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
            u32 c = 0;
            c |= ((u32)tgaPixelIn[0] << 16);
            c |= ((u32)tgaPixelIn[1] << 8);
            c |= ((u32)tgaPixelIn[2] << 0);

            pOut->SetPixel(_GetPixelTypeFromTGAColor(c));

            if(c == 0xFF00DC)
            {
                m_map->m_playerSpawnpoints.emplace_back(m_posX + x, py);
            }

            tgaPixelIn += 3;
            pOut++;
            //px++;
        }
        py++;
    }
    RefreshCellTexture();
}

PixelType& MapCell::GetPixelFromCellCoords(s32 x, s32 y)
{
    return m_pixelArray[x + y*MAP_CELL_WIDTH];
}

void MapCell::UpdateCell()
{

}

void MapCell::DrawCell()
{
    //RefreshCellTexture();
    m_cellSprite->FlushCache();
    // workaround necessary for Cemu
    // Cemu texture invalidation heuristics may not always catch pixel updates
    // so we modify the top left pixel (usually guaranteed to be checked) to coerce an invalidation
    u8* pixData = (u8*)m_cellSprite->GetTexture()->surface.image;
    pixData[3] ^= 1;

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
        case MAP_PIXEL_TYPE::GRASS:
        {
            u8 rd = rand()%2;
            if(rd == 0)
                return 0x2F861FFF;
            return 0x3E932BFF;
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
    m_activePixels = new ActivePixelCollection();
    m_cellsX = (width+MAP_CELL_WIDTH-1) / MAP_CELL_WIDTH;
    m_cellsY = (height+MAP_CELL_HEIGHT-1) / MAP_CELL_HEIGHT;
    m_cells.clear();
    for(u32 y=0; y<m_cellsY; y++)
    {
        for(u32 x=0; x<m_cellsX; x++)
        {
            m_cells.emplace_back(this, x, y);
        }
    }
    //GenerateTerrain();
}

Map::Map(const char* filename, u32 rngSeed)
{
    m_rng.SetSeed(rngSeed);
    double startTime = GetMillisecondTimestamp();
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
    double dur = GetMillisecondTimestamp() - startTime;
    char strBuf[64];
    sprintf(strBuf, "%.04lf", dur);
    OSReport("Level loaded in %sms\n", strBuf);
}

Map::~Map()
{
    delete m_activePixels;
}

void Map::SetPixelColor(s32 x, s32 y, u32 c)
{
    s32 cellX = x >> 6;
    s32 relX = x & 0x3F;
    s32 cellY = y >> 6;
    s32 relY = y & 0x3F;
    m_cells[cellX + cellY * m_cellsX].m_cellSprite->SetPixel(relX, relY, c);
}

static_assert(MAP_CELL_WIDTH == 64); // hardcoded in GetPixel()
static_assert(MAP_CELL_HEIGHT == 64);

PixelType& Map::GetPixel(s32 x, s32 y)
{
    s32 cellX = x >> 6;
    s32 relX = x & 0x3F;
    s32 cellY = y >> 6;
    s32 relY = y & 0x3F;
    if((cellX < 0 || cellX >= (s32)m_cellsX) ||
       (cellY < 0 || cellY >= (s32)m_cellsY))
    {
        CriticalErrorHandler("Map::GetPixel - x/y out of range");
    }
    return m_cells[cellX + cellY * m_cellsX].GetPixelFromCellCoords(relX, relY);
}

// can be optimized, probably more efficient than reading pixels individually
void Map::GetCollisionRect(s32 x, s32 y, s32 width, s32 height, bool* rectOut)
{
    for(s32 iy=0; iy<height; iy++)
    {
        for(s32 ix=0; ix<width; ix++)
        {
            rectOut[ix + iy * width] = GetPixel(x + ix, y + iy).IsSolid();
        }
    }
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

MAP_PIXEL_TYPE PixelType::GetPixelType() const
{
    if(pixelType&1)
    {
        return (MAP_PIXEL_TYPE)((pixelType >> 1)&0x7F);
    }
    ActivePixelBase* pixelBase = (ActivePixelBase*)(pixelType&~1);
    return pixelBase->material;
}

Map* s_currentMap{nullptr};

// objects are tied to whichever map is active
void SetCurrentMap(Map* newMap)
{
    s_currentMap = newMap;
}

Map* GetCurrentMap()
{
    return s_currentMap;
}
