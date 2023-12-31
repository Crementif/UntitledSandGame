#pragma once

#include "./../common/types.h"
#include "../framework/noise/noise.h"
#include "../framework/render.h"

#define MAP_PIXEL_ZOOM  (3) // one world pixel will translate to this many screen pixels

#define MAP_CELL_WIDTH  (64)
#define MAP_CELL_HEIGHT  (64)

enum class MAP_PIXEL_TYPE : u8
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

static constexpr u8 _GetMaxVariationsFromPixelType(MAP_PIXEL_TYPE type) {
    switch (type)
    {
        case MAP_PIXEL_TYPE::AIR:
            return 0;
        case MAP_PIXEL_TYPE::SAND:
            return 2;
        case MAP_PIXEL_TYPE::SOIL:
            return 2;
        case MAP_PIXEL_TYPE::GRASS:
            return 2;
        case MAP_PIXEL_TYPE::LAVA:
            return 3;
        case MAP_PIXEL_TYPE::ROCK:
            return 9;
        case MAP_PIXEL_TYPE::SMOKE:
            return 2;
        case MAP_PIXEL_TYPE::_COUNT:
            break;
    }
    return 0;
}

static u8 _GetRandomSeedFromPixelType(MAP_PIXEL_TYPE type) {
    switch(type)
    {
        case MAP_PIXEL_TYPE::AIR:
            return 0;
        case MAP_PIXEL_TYPE::SAND:
            return rand()%_GetMaxVariationsFromPixelType(MAP_PIXEL_TYPE::SAND);
        case MAP_PIXEL_TYPE::SOIL:
            return rand()%_GetMaxVariationsFromPixelType(MAP_PIXEL_TYPE::SOIL);
        case MAP_PIXEL_TYPE::GRASS:
            return rand()%_GetMaxVariationsFromPixelType(MAP_PIXEL_TYPE::GRASS);
        case MAP_PIXEL_TYPE::LAVA:
            return rand()%_GetMaxVariationsFromPixelType(MAP_PIXEL_TYPE::LAVA);
        case MAP_PIXEL_TYPE::ROCK:
            return rand()%_GetMaxVariationsFromPixelType(MAP_PIXEL_TYPE::ROCK);
        case MAP_PIXEL_TYPE::SMOKE:
            return rand()%_GetMaxVariationsFromPixelType(MAP_PIXEL_TYPE::SMOKE);
        case MAP_PIXEL_TYPE::_COUNT:
            break;
    }
    return 0;
}

static u8 _GetDimmedSeedOffsetFromPixelType(MAP_PIXEL_TYPE type, u8 existingSeed) {
    switch (type) {
        case MAP_PIXEL_TYPE::SAND:
            return 2;
        case MAP_PIXEL_TYPE::GRASS:
            return 2;
        case MAP_PIXEL_TYPE::SOIL:
            return 2;
        case MAP_PIXEL_TYPE::ROCK:
            return 9;
        default:
            return existingSeed;
    }
}

union PixelType
{

    inline void SetStaticPixel(MAP_PIXEL_TYPE type, u8 seed)
    {
        pixelType |= 1; // mark as static pixel by setting LSB
        // set type
        pixelType &= ~(0x7F<<1); // 7 bits for type
        pixelType |= ((u32)type << 1);
        // set seed
        pixelType &= ~(0xFF<<8);
        pixelType |= ((u32)seed << 8);
    }

    inline void SetStaticPixelWithRandomSeed(MAP_PIXEL_TYPE type)
    {
        SetStaticPixel(type, _GetRandomSeedFromPixelType(type));
    }

    inline void SetStaticPixelToAir()
    {
        // pre-emptive optimization to remove _GetRandomSeedFromPixelType when air has no seed
        SetStaticPixel(MAP_PIXEL_TYPE::AIR, 0);
    }

    inline void SetDynamicPixel(class ActivePixelBase* pixelPtr)
    {
        pixelType = (u32)(uintptr_t)pixelPtr;
    }

    // if LSB is not set, the bottom 31 bits are a pointer to ActivePixel (pointer tagging, so bottom bit of address isn't stored)
    inline class ActivePixelBase* _GetDynamicPtr() const
    {
        ActivePixelBase* pixelBase = (ActivePixelBase*)(pixelType&~1);
        return pixelBase;
    }

    // if LSB is set, the next 7 bits are used for the MAP_PIXEL_TYPE and the 8 bits after that are used for the seed
    inline u8 _GetPixelSeedStatic() const
    {
        return ((pixelType >> 8)&0xFF);
    }
    inline void _SetPixelSeedStatic(u8 seed) {
        pixelType &= ~(0xFF<<8);
        pixelType |= ((u32)seed << 8);
    }

    MAP_PIXEL_TYPE GetPixelType() const;

    // format: 0xRRGGBBAA
    u32 CalculatePixelColor() const;

    // collision applies
    // deprecated, use IsFilled() instead
    inline bool IsSolid() const
    {
        if(GetPixelType() == MAP_PIXEL_TYPE::AIR)
            return false;
        return true;
    }

    inline bool IsLiquid() const
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

    inline bool CanBeDynamic() const
    {
        MAP_PIXEL_TYPE mat = GetPixelType();
        switch(mat)
        {
            case MAP_PIXEL_TYPE::SAND:
            case MAP_PIXEL_TYPE::LAVA:
                return true;
            default:
                return false;
        }
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

    inline bool IsDimmable() const {
        if (IsDynamic())
            return false;

        switch(_GetPixelTypeStatic())
        {
            case MAP_PIXEL_TYPE::SAND:
            case MAP_PIXEL_TYPE::GRASS:
            case MAP_PIXEL_TYPE::SOIL:
            case MAP_PIXEL_TYPE::ROCK:
                return true;
            default:
                return false;
        }
        return false;
    }

    uint32_t pixelType; // LSB set means static pixel, otherwise dynamic pixel
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
    friend class GravityPixel;
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

    bool IsPixelOOB(s32 x, s32 y) const;
    bool IsPixelOOBWithSafetyMargin(s32 x, s32 y, u32 margin) const;

    void SetPixelColor(s32 x, s32 y, u32 c);

    PixelType& GetPixel(s32 x, s32 y); // crashes with message if out-of-bounds
    PixelType& GetPixelNoBoundsCheck(s32 x, s32 y); // no bounds checking at all, fastest

    bool DoesPixelCollideWithObject(s32 x, s32 y)
    {
        if(IsPixelOOB(x, y))
            return true;
        return GetPixelNoBoundsCheck(x, y).IsCollideWithObjects();
    }

    bool DoesPixelCollideWithSolids(s32 x, s32 y)
    {
        if(IsPixelOOB(x, y))
            return true;
        return GetPixelNoBoundsCheck(x, y).IsCollideWithSolids();
    }

    bool DoesPixelCollideWithType(s32 x, s32 y, MAP_PIXEL_TYPE type)
    {
        if(IsPixelOOB(x, y))
            return true;
        return GetPixelNoBoundsCheck(x, y).GetPixelType() == type;
    }

    void SimulateTick();
    static constexpr u32 HOTSPOT_CHECK_ATTEMPTS_EARLY_EXIT = 6;
    static constexpr u32 HOTSPOT_CHECK_ATTEMPTS = 30;
    static constexpr u32 HOTSPOT_CHECK_RADIUS = 32;
    bool DoVolatilityRadiusCheckForStaticPixels(u32 x, u32 y);
    static constexpr u32 FIND_HOTSPOT_ATTEMPTS = 450 * 1.5;
    static constexpr u32 FIND_HOTSPOT_LIFETIME = 100;
    static constexpr u32 FIND_HOTSPOT_LIFETIME_EXTENSION = 50;
    void FindRandomHotspots();
    void UpdateVolatilityHotspots();
    void SpawnMaterialPixel(MAP_PIXEL_TYPE materialType, u8 materialSeed, s32 x, s32 y);
    void ReanimateStaticPixel(MAP_PIXEL_TYPE materialType, u8 materialSeed, s32 x, s32 y);

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
    void HandleSynchronizedEvent_Explosion(u32 playerId, Vector2f pos, f32 radius, f32 force);
    void HandleSynchronizedEvent_Gravity(u32 playerId, Vector2f pos, f32 strength, f32 lifetimeTicks);

    void SimulateFlungPixels();
    void SimulateGravityPixels();
    Vector2f CalculateGravityInfluences(Vector2f pos, f32 weight) const;

    u32 m_cellsX;
    u32 m_cellsY;
    u32 m_pixelsX;
    u32 m_pixelsY;
    std::vector<MapCell> m_cells;

    std::vector<Vector2i> m_playerSpawnpoints;
    std::vector<Vector2i> m_collectablePoints;

    class ActivePixelCollection* m_activePixels;
    std::vector<class FlungPixel*> m_flungPixels;
    std::vector<class GravityPixel*> m_gravityPixels;

    LCGRng m_rng;
    LCGRng m_nonDeterministicRng;
    u32 m_simulationTick{0};

    // static volatility hotspots
    // these track locations where there have been static pixels found that might need reanimation
    struct StaticVolatilityHotSpot
    {
        StaticVolatilityHotSpot(u16 x, u16 y, u16 ttl) : x(x), y(y), ttl(ttl) {};
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