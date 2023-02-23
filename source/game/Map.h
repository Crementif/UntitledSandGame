#include "./../common/types.h"

#include "../framework/noise/noise.h"

#pragma once

#define MAP_PIXEL_ZOOM  (4) // one world pixel will translate to this many screen pixels

#define MAP_CELL_WIDTH  (64)
#define MAP_CELL_HEIGHT  (64)

struct ActivePixel
{
    uint16_t x;
    uint16_t y;
};

enum class MAP_PIXEL_TYPE
{
    AIR = 0,
    SAND = 1,
};

union PixelType
{
    void SetPixel(MAP_PIXEL_TYPE type)
    {
        pixelType = 1;
        pixelType |= ((u32)type << 1);
    }

    void SetDynamicPixel(ActivePixel* activePixel)
    {

    }

    MAP_PIXEL_TYPE GetPixelType()
    {
        // todo - if LSB not set
        return (MAP_PIXEL_TYPE)(pixelType >> 1);
    }

    uint32_t pixelType; // LSB set
    ActivePixel* pixelPtr; // LSB not set
};

static_assert(sizeof(PixelType) == 4);


class MapCell
{
public:
    MapCell(u32 cellX, u32 cellY);

    void UpdateCell();
    void DrawCell();

    void RefreshCellTexture();

private:
    PixelType m_pixelArray[MAP_CELL_WIDTH * MAP_CELL_HEIGHT];
    u32 m_cellX;
    u32 m_cellY;
    u32 m_posX;
    u32 m_posY;
    class Sprite* m_cellSprite{nullptr};
};

class Map
{
public:
    Map();
    ~Map();

    void Update();
    void Draw();

private:
    void GenerateTerrain();

    u32 m_cellsX;
    u32 m_cellsY;
    std::vector<MapCell> m_cells;
};

