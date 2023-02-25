#include "Map.h"
#include "MapPixels.h"
#include "GameSceneIngame.h"

void Map::SpawnMaterialPixel(MAP_PIXEL_TYPE materialType, s32 x, s32 y)
{
    m_activePixels->sandPixels.emplace_back(x, y).IntegrateIntoWorld(this);
    //m_map->GetPixel(150, 160);
}

template<typename T>
void SimulateMaterial(Map* map, std::vector<T>& pixels)
{
    T* dataArray = pixels.data();
    for(size_t i=0; i<pixels.size(); i++)
    {
        if(!dataArray[i].SimulateStep(map))
        {
            // removed particle
            dataArray[i] = pixels.back();
            pixels.pop_back();
        }
    }

}

void Map::SimulateTick()
{
    SimulateMaterial(GetCurrentMap(), m_activePixels->sandPixels);
    g_debugStrings.emplace_back("Sand Particles: " + std::to_string(m_activePixels->sandPixels.size()));
}