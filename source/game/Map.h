#include "./../common/types.h"

#include "../framework/noise/noise.h"

#pragma once

#define MAP_PIXEL_ZOOM  (3) // one world pixel will translate to this many screen pixels

#define MAP_CELL_WIDTH  (64)
#define MAP_CELL_HEIGHT  (64)

enum class MAP_PIXEL_TYPE
{
    AIR = 0,
    SAND = 1,
    SOIL = 2,
};

union PixelType
{
    void SetPixel(MAP_PIXEL_TYPE type)
    {
        pixelType = 1;
        pixelType |= ((u32)type << 1);
    }

    /*
    void SetDynamicPixel(ActivePixel* activePixel)
    {

    }*/

    MAP_PIXEL_TYPE GetPixelType() const
    {
        // todo - if LSB not set
        return (MAP_PIXEL_TYPE)(pixelType >> 1);
    }

    // collision applies
    bool IsSolid() const
    {
        return GetPixelType() != MAP_PIXEL_TYPE::AIR;
    }

    uint32_t pixelType; // LSB set
    //ActivePixel* pixelPtr; // LSB not set
};

static_assert(sizeof(PixelType) == 4);

class MapCell
{
public:
    MapCell(class Map* map, u32 cellX, u32 cellY);
    void LoadCellFromTGA(class TGALoader& tgaLoader);

    void UpdateCell();
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
    Map(const char* filename);
    ~Map();

    void Update();
    void Draw();

    const std::vector<Vector2i>& GetPlayerSpawnpoints()
    {
        return m_playerSpawnpoints;
    }

    PixelType& GetPixel(s32 x, s32 y);
    void GetCollisionRect(s32 x, s32 y, s32 width, s32 height, bool* rectOut);

    void SetPixelSpriteColor(s32 x, s32 y);

    void SimulateTick();

    void SpawnMaterialPixel(MAP_PIXEL_TYPE materialType, s32 x, s32 y);

    u32 GetRNGNumber()
    {
        return m_rng.GetNext();
    }

private:
    void Init(uint32_t width, uint32_t height);

    void GenerateTerrain();

    u32 m_cellsX;
    u32 m_cellsY;
    std::vector<MapCell> m_cells;

    std::vector<Vector2i> m_playerSpawnpoints;

    class ActivePixelCollection* m_activePixels{nullptr};

    LCGRng m_rng;
};

void SetCurrentMap(Map* newMap);
Map* GetCurrentMap();