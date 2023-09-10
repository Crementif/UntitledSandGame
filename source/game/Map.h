#pragma once

#include "./../common/types.h"
#include "../framework/noise/noise.h"
#include "../framework/render.h"

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
    SMOKE = 6,
    _COUNT = 7,
};

union PixelType
{
    inline void SetPixel(MAP_PIXEL_TYPE type)
    {
        pixelType = 1;
        pixelType |= ((u32)type << 1);
    }

    inline void SetDynamicPixel(class ActivePixelBase* pixelPtr)
    {
        pixelType = (u32)(uintptr_t)pixelPtr;
    }

    inline class ActivePixelBase* _GetDynamicPtr() const
    {
        ActivePixelBase* pixelBase = (ActivePixelBase*)(pixelType&~1);
        return pixelBase;
    }

    MAP_PIXEL_TYPE GetPixelType() const;

    // collision applies
    // deprecated, use IsFilled() instead
    bool IsSolid() const
    {
        if(GetPixelType() == MAP_PIXEL_TYPE::AIR)
            return false;
        return true;
    }

    bool IsLiquid() const
    {
        MAP_PIXEL_TYPE mat = GetPixelType();
        switch(mat)
        {
            case MAP_PIXEL_TYPE::LAVA:
                return true;
            default:
                break;
        }
        return false;
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
        MAP_PIXEL_TYPE mat = GetPixelType();
        if(mat == MAP_PIXEL_TYPE::AIR)
            return false;
        return true;
    }

    bool IsCollideWithSolids() const
    {
        if((pixelType&1) == 0)
            return false; // active pixels are exempt from object collision
        MAP_PIXEL_TYPE mat = GetPixelType();
        if(mat == MAP_PIXEL_TYPE::AIR)
            return false;
        if(mat == MAP_PIXEL_TYPE::LAVA)
            return false;
        return true;
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

    void FlushDrawCache();
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
    friend class FlungPixel;
public:
    Map(const char* filename, u32 rngSeed);
    ~Map();

    void Update();
    void Draw();

    const std::vector<Vector2i>& GetPlayerSpawnpoints() {
        return m_playerSpawnpoints;
    }

    const std::vector<Vector2i>& GetCollectablePoints() {
        return m_collectablePoints;
    }

    bool IsPixelOOB(s32 x, s32 y);
    void SetPixelColor(s32 x, s32 y, u32 c);

    PixelType& GetPixel(s32 x, s32 y); // crashes with message if out-of-bounds
    PixelType& GetPixelNoBoundsCheck(s32 x, s32 y); // no bounds checking at all, fastest

    bool DoesPixelCollideWithObject(s32 x, s32 y)
    {
        if(IsPixelOOB(x, y))
            return true;
        return GetPixel(x, y).IsCollideWithObjects();
    }

    bool DoesPixelCollideWithSolids(s32 x, s32 y)
    {
        if(IsPixelOOB(x, y))
            return true;
        return GetPixel(x, y).IsCollideWithSolids();
    }

    bool DoesPixelCollideWithType(s32 x, s32 y, MAP_PIXEL_TYPE type)
    {
        if(IsPixelOOB(x, y))
            return true;
        return GetPixel(x, y).GetPixelType() == type;
    }

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

    f32 GetRNGFloat01()
    {
        u32 v = GetRNGNumber() & 0xFFFF;
        return (float)v / 65535.0f;
    }

    u32 GetPixelWidth() const { return m_pixelsX; }
    u32 GetPixelHeight() const { return m_pixelsY; }
    u32 GetCellsX() const { return m_cellsX; }
    u32 GetCellsY() const { return m_cellsY; }
    MapCell* GetCellPtrArray() { return m_cells.data(); }

private:
    void Init(uint32_t width, uint32_t height);

    void HandleSynchronizedEvents();
    void HandleSynchronizedEvent_Drilling(u32 playerId, Vector2f pos);
    void HandleSynchronizedEvent_Explosion(u32 playerId, Vector2f pos, f32 radius);

    void SimulateFlungPixels();

    u32 m_cellsX;
    u32 m_cellsY;
    u32 m_pixelsX;
    u32 m_pixelsY;
    std::vector<MapCell> m_cells;

    std::vector<Vector2i> m_playerSpawnpoints;
    std::vector<Vector2i> m_collectablePoints;

    class ActivePixelCollection* m_activePixels{nullptr};
    std::vector<class FlungPixel*> m_flungPixels;

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

    uint32_t m_currLavaHiss = 0;
    OSTime m_lastLavaHiss = 0;
    class Audio* m_lavaHiss0Audio = nullptr;
    class Audio* m_lavaHiss1Audio = nullptr;
    class Audio* m_lavaHiss2Audio = nullptr;
    class Audio* m_lavaHiss3Audio = nullptr;
};