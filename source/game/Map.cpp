#include "Map.h"
#include "MapPixels.h"
#include "../framework/render.h"

#include "../framework/fileformat/TGAFile.h"
#include "../framework/audio/audio.h"
#include "MapFlungPixels.h"
#include "MapGravityPixels.h"
#include "../framework/debug.h"

MAP_PIXEL_TYPE _GeneratePixelAtWorldPos(s32 x, s32 y)
{
    f32 f = perlinNoise_2d((float)x * 0.017, (float)y * 0.017);

    f += abs(perlinNoise_2d((float)x * 0.00111 + 13.333f, (float)y * 0.00111));

    if( f <= 0.15 )
        return MAP_PIXEL_TYPE::SAND;

    return MAP_PIXEL_TYPE::AIR;
}

MapCell::MapCell(Map* map, u32 cellX, u32 cellY) : m_cellX(cellX), m_cellY(cellY), m_posX(cellX * MAP_CELL_WIDTH), m_posY(cellY * MAP_CELL_HEIGHT), m_map(map)
{
    m_cellSprite = new Sprite(MAP_CELL_WIDTH, MAP_CELL_HEIGHT, false, E_TEXFORMAT::RG88_UNORM);
    m_cellSprite->SetupSampler(false);
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

            pOut->SetStaticPixelWithRandomSeed(_GetPixelTypeFromTGAColor(c));

            if(c == 0xFF00DC)
            {
                m_map->m_playerSpawnpoints.emplace_back(m_posX + x, py);
            }

            if (c == 0x4800FF)
            {
                m_map->m_collectablePoints.emplace_back(m_posX + x, py);
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

void Map::Init(uint32_t width, uint32_t height)
{
    perlinNoise_init();
    m_activePixels = new ActivePixelCollection();
    OSReport("Create map with size %dx%d\n", width, height);
    m_cellsX = (width+MAP_CELL_WIDTH-1) / MAP_CELL_WIDTH;
    m_cellsY = (height+MAP_CELL_HEIGHT-1) / MAP_CELL_HEIGHT;
    m_pixelsX = m_cellsX * MAP_CELL_WIDTH;
    m_pixelsY = m_cellsY * MAP_CELL_HEIGHT;
    if(m_pixelsX != width || m_pixelsY != height)
        CriticalErrorHandler("Map size is not aligned to cell size. Level TGA width and height must be a multiple of 64");
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
    auto mapData = LoadFileToMem(std::string("levels/").append(filename));
    if(mapData.empty())
        OSReport("Unable to load map file");
    OSReport("Level load - Phase 2\n");
    TGALoader tgaLoader;
    if(!tgaLoader.LoadFromTGA(mapData))
        CriticalErrorHandler("Unable to load level");
    OSReport("Level load - Phase 3 (Map size: %ux%u)\n", tgaLoader.GetWidth(), tgaLoader.GetHeight());
    this->Init(tgaLoader.GetWidth(), tgaLoader.GetHeight());
    OSReport("Level load - Phase 4\n");
    for(auto& it : m_cells)
        it.LoadCellFromTGA(tgaLoader);
    OSReport("Level load - Finished\n");
    double dur = GetMillisecondTimestamp() - startTime;
    char strBuf[64];
    sprintf(strBuf, "%.04lf", dur);
    OSReport("Level loaded in %sms\n", strBuf);

    m_lavaHiss0Audio = new Audio("/sfx/lava_hiss0.ogg");
    m_lavaHiss1Audio = new Audio("/sfx/lava_hiss1.ogg");
    m_lavaHiss2Audio = new Audio("/sfx/lava_hiss2.ogg");
    m_lavaHiss3Audio = new Audio("/sfx/lava_hiss3.ogg");
}

Map::~Map()
{
    delete m_activePixels;
    while (!m_flungPixels.empty())
    {
        delete m_flungPixels.back();
        m_flungPixels.pop_back();
    }
    while (!m_gravityPixels.empty())
    {
        delete m_gravityPixels.back();
        m_gravityPixels.pop_back();
    }
}

void Map::SetPixelColor(s32 x, s32 y, u32 c)
{
    s32 cellX = x >> 6;
    s32 relX = x & 0x3F;
    s32 cellY = y >> 6;
    s32 relY = y & 0x3F;
    m_cells[cellX + cellY * m_cellsX].m_cellSprite->SetPixelRG88(relX, relY, c);
}

static_assert(MAP_CELL_WIDTH == 64); // hardcoded in GetPixel()
static_assert(MAP_CELL_HEIGHT == 64);

bool Map::IsPixelOOB(const s32 x, const s32 y) const
{
    return (static_cast<u32>(x) >= m_pixelsX) || (static_cast<u32>(y) >= m_pixelsY);
}

bool Map::IsPixelOOBWithSafetyMargin(const s32 x, const s32 y, const u32 margin) const
{
    return (static_cast<u32>(x + margin) >= (m_pixelsX - margin)) || (static_cast<u32>(y + margin) >= (m_pixelsY - margin));
}

PixelType& Map::GetPixel(s32 x, s32 y)
{
    s32 cellX = x >> 6;
    s32 relX = x & 0x3F;
    s32 cellY = y >> 6;
    s32 relY = y & 0x3F;
    /*
    if((cellX < 0 || cellX >= (s32)m_cellsX) ||
       (cellY < 0 || cellY >= (s32)m_cellsY))
    {
        CriticalErrorHandler("Map::GetPixel - x/y out of range");
    }*/
    // optimized:
    if(static_cast<u32>(cellX) >= m_cellsX || static_cast<u32>(cellY) >= m_cellsY)
    {
        CriticalErrorHandler("Map::GetPixel - x/y out of range");
    }
    return m_cells[cellX + cellY * m_cellsX].GetPixelFromCellCoords(relX, relY);
}

PixelType& Map::GetPixelNoBoundsCheck(s32 x, s32 y)
{
    s32 cellX = x >> 6;
    s32 relX = x & 0x3F;
    s32 cellY = y >> 6;
    s32 relY = y & 0x3F;
    return m_cells[cellX + cellY * m_cellsX].GetPixelFromCellCoords(relX, relY);
}

constexpr u32 LAVA_HISS_ATTEMPTS = 2;
void Map::Update()
{
    DebugProfile::Start("Scene -> Simulation -> Lava Hiss");
    if ((m_lastLavaHiss + (OSTime)OSSecondsToTicks(5)) >= OSGetTime())
        return;

    // for the entire screen, check if there is lava and play a hiss sound
    for (u32 i = 0; i < LAVA_HISS_ATTEMPTS; i++)
    {
        u32 ss_x = m_nonDeterministicRng.GetNext() % (u32)(Render::GetScreenSize().x / MAP_PIXEL_ZOOM);
        u32 ws_x = std::clamp<u32>(ss_x + (u32)(Render::GetCameraPosition().x / MAP_PIXEL_ZOOM), 0, GetPixelWidth()-1);
        u32 ss_y = m_nonDeterministicRng.GetNext() % (u32)(Render::GetScreenSize().y / MAP_PIXEL_ZOOM);
        u32 ws_y = std::clamp<u32>(ss_y + (u32)(Render::GetCameraPosition().y / MAP_PIXEL_ZOOM), 0, GetPixelHeight()-1);

        if (GetPixelNoBoundsCheck((s32)ws_x, (s32)ws_y).GetPixelType() == MAP_PIXEL_TYPE::LAVA) {
            if (m_currLavaHiss == 0) m_lavaHiss0Audio->Play();
            else if (m_currLavaHiss == 1) m_lavaHiss1Audio->Play();
            else if (m_currLavaHiss == 2) m_lavaHiss2Audio->Play();
            else if (m_currLavaHiss == 3) m_lavaHiss3Audio->Play();

            m_currLavaHiss++;
            m_lastLavaHiss = OSGetTime();
        }
    }

    if (m_currLavaHiss >= 4)
        m_currLavaHiss = 0;
    DebugProfile::End("Scene -> Simulation -> Lava Hiss");
}

MAP_PIXEL_TYPE PixelType::GetPixelType() const
{
    if(pixelType&1)
    {
        return (MAP_PIXEL_TYPE)((pixelType >> 1)&0x7F);
    }
    ActivePixelBase* pixelBase = _GetDynamicPtr();
    return pixelBase->material;
}

// format: 0xRRGGBBAA
u32 PixelType::CalculatePixelColor() const
{
    u8 type = 0;
    u8 seed = 0;
    if (!IsDynamic())
    {
        type = (u8)(MAP_PIXEL_TYPE)((pixelType >> 1)&0x7F);
        seed = (u8)((pixelType >> 8)&0xFF);
    }
    else {
        ActivePixelBase* pixelBase = _GetDynamicPtr();
        type = (u8)pixelBase->material;
        seed = (u8)pixelBase->seed;
    }
    return (type << 24) | (seed << 16);
}
