#pragma once

#include "./../common/types.h"
#include "../framework/noise/noise.h"

#define MAP_PIXEL_ZOOM  (3) // one world pixel will translate to this many screen pixels

#define MAP_CELL_WIDTH  (64)
#define MAP_CELL_HEIGHT  (64)

enum class MAP_PIXEL_TYPE
{
    AIR = 0,
    SAND = 1,
    SOIL = 2,
    GRASS = 3,
    LAVA = 4,
    ROCK = 5,
};

union PixelType
{
    void SetPixel(MAP_PIXEL_TYPE type)
    {
        pixelType = 1;
        pixelType |= ((u32)type << 1);
    }

    void SetDynamicPixel(class ActivePixelBase* pixelPtr)
    {
        pixelType = (u32)(uintptr_t)pixelPtr;
    }

    MAP_PIXEL_TYPE GetPixelType() const;

    // collision applies
    bool IsSolid() const
    {
        if(GetPixelType() == MAP_PIXEL_TYPE::AIR)
            return false;
        return true;
    }

    bool IsDestructible() const
    {
        return true;
    }

    inline bool IsDynamic() const
    {
        return (pixelType&1) == 0;
    }

    // if we know IsDynamic is false, then calling this directly is faster
    inline MAP_PIXEL_TYPE _GetPixelTypeStatic() const
    {
        return (MAP_PIXEL_TYPE)((pixelType >> 1)&0x7F);
    }

    bool IsCollideWithObjects() const
    {
        if((pixelType&1) == 0)
            return false; // active pixels are exempt from object collision
        if(GetPixelType() == MAP_PIXEL_TYPE::AIR)
            return false;
        return true;
        //return (pixelType&BIT_DYNAMIC) == 0;
    }

    // has any pixel other than air
    const bool IsFilled() const
    {
        if((pixelType&1) == 0)
            return true;
        return _GetPixelTypeStatic() != MAP_PIXEL_TYPE::AIR;
    }

    uint32_t pixelType; // LSB set
    //ActivePixel* pixelPtr; // LSB not set
};

static_assert(sizeof(PixelType) == 4);

class MapCell
{
    friend class Map;
public:
    MapCell(class Map* map, u32 cellX, u32 cellY);
    void LoadCellFromTGA(class TGALoader& tgaLoader);

    void DrawCell();

    void RefreshCellTexture();

    PixelType& GetPixelFromCellCoords(s32 x, s32 y);

private:
    PixelType m_pixelArray[MAP_CELL_WIDTH * MAP_CELL_HEIGHT];
    u32 m_cellX;
    u32 m_cellY;
    u32 m_posX;
    u32 m_posY;
    class Map* m_map;
    class Sprite* m_cellSprite{nullptr};
};

class Map
{
    friend class MapCell;
public:
    Map(const char* filename, u32 rngSeed);
    ~Map();

    void Update();
    void Draw();

    const std::vector<Vector2i>& GetPlayerSpawnpoints() {
        return m_playerSpawnpoints;
    }

    bool IsPixelOOB(s32 x, s32 y);
    void SetPixelColor(s32 x, s32 y, u32 c);

    PixelType& GetPixel(s32 x, s32 y);
    PixelType& GetPixelNoBoundsCheck(s32 x, s32 y);
    void GetCollisionRect(s32 x, s32 y, s32 width, s32 height, bool* rectOut);

    void SimulateTick();
    bool CheckVolatileStaticPixelsHotspot(u32 x, u32 y);
    void CheckStaticPixels();
    void SpawnMaterialPixel(MAP_PIXEL_TYPE materialType, s32 x, s32 y);
    void ReanimateStaticPixel(MAP_PIXEL_TYPE materialType, s32 x, s32 y);
    void ReanimateStaticPixel(MAP_PIXEL_TYPE materialType, s32 x, s32 y, f32 force);

    u32 GetRNGNumber()
    {
        return m_rng.GetNext();
    }

private:
    void Init(uint32_t width, uint32_t height);

    void HandleSynchronizedEvents();
    void HandleSynchronizedEvent_Drilling(u32 playerId, Vector2f pos);

    u32 m_cellsX;
    u32 m_cellsY;
    u32 m_pixelsX;
    u32 m_pixelsY;
    std::vector<MapCell> m_cells;

    std::vector<Vector2i> m_playerSpawnpoints;

    class ActivePixelCollection* m_activePixels{nullptr};

    LCGRng m_rng;
    u32 m_simulationTick{0};

    // static volatility hotspots
    // these track locations where there have been static pixels found that might need reanimation
    struct StaticVolatilityHotSpot
    {
        StaticVolatilityHotSpot(u16 x, u16 y) : x(x), y(y), ttl(500) {};
        u16 x;
        u16 y;
        u16 ttl; // ticks to live
    };

    std::vector<StaticVolatilityHotSpot> m_volatilityHotspots;
};

void SetCurrentMap(Map* newMap);
Map* GetCurrentMap();