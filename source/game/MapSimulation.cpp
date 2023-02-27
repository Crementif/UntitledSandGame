#include "Map.h"
#include "MapPixels.h"
#include "GameSceneIngame.h"

void Map::SpawnMaterialPixel(MAP_PIXEL_TYPE materialType, s32 x, s32 y)
{
    switch(materialType)
    {
        case MAP_PIXEL_TYPE::SAND:
            m_activePixels->sandPixels.emplace_back(new ActivePixelSand(x, y));
            break;
        case MAP_PIXEL_TYPE::LAVA:
            m_activePixels->lavaPixels.emplace_back(new ActivePixelLava(x, y));
            break;
        default:
            break;
    }
}

// technically this should grab the material type from the pixel at x/y,
// but in most cases we already have the material type so it's an extra optimization to not recalculate it
void Map::ReanimateStaticPixel(MAP_PIXEL_TYPE materialType, s32 x, s32 y)
{
    SpawnMaterialPixel(materialType, x, y);
}

void Map::ReanimateStaticPixel(MAP_PIXEL_TYPE materialType, s32 x, s32 y, f32 force)
{
    SpawnMaterialPixel(materialType, x, y);
}

bool _IsReanimatableMaterial(MAP_PIXEL_TYPE materialType)
{
    switch(materialType)
    {
        case MAP_PIXEL_TYPE::LAVA:
        case MAP_PIXEL_TYPE::SAND:
            return true;
        default:
            break;
    }
    return false;
}

template<typename T>
void SimulateMaterial(Map* map, std::vector<T>& pixels)
{
    T* dataArray = pixels.data();
    for(size_t i=0; i<pixels.size(); i++)
    {
        if(!dataArray[i]->SimulateStep(map))
        {
            // removed particle
            dataArray[i]->PixelDeactivated(map);
            delete dataArray[i];
            dataArray[i] = pixels.back();
            pixels.pop_back();
        }
    }

}

#define HOTSPOT_RANGE   16

// check a small rectangle area for pixels that might need reanimation
bool Map::CheckVolatileStaticPixelsHotspot(u32 x, u32 y)
{
    // randomly check a few pixels in each cell to see if we need to simulate them due to surrounding conditions changing
    bool hasActivity = false;
    for(u32 i=0; i<25; i++)
    {
        u32 rdX = this->GetRNGNumber() % (HOTSPOT_RANGE*2);
        u32 rdY = this->GetRNGNumber() % (HOTSPOT_RANGE*2);
        u32 px = x + rdX - HOTSPOT_RANGE;
        u32 py = y + rdY - HOTSPOT_RANGE;
        PixelType& pixelType = GetPixelNoBoundsCheck(px, py);
        if(pixelType.IsDynamic())
            continue;
        auto mat = pixelType._GetPixelTypeStatic();
        if(mat == MAP_PIXEL_TYPE::SAND)
        {
            if(!GetPixelNoBoundsCheck(px, py+1).IsFilled())
            {
                hasActivity = true;
                ReanimateStaticPixel(MAP_PIXEL_TYPE::SAND, px, py);
            }
        }
        else if(mat == MAP_PIXEL_TYPE::LAVA)
        {
            if(!GetPixelNoBoundsCheck(px, py+1).IsFilled())
            {
                hasActivity = true;
                ReanimateStaticPixel(MAP_PIXEL_TYPE::LAVA, px, py);
            }
        }
    }
    return hasActivity;
}

void Map::CheckStaticPixels()
{
    u32 boundX = m_pixelsX - 8;
    u32 boundY = m_pixelsY - 8;

    auto ClampHotspotCoords = [this](u32& x, u32& y)
    {
        x = std::clamp<u32>(x, HOTSPOT_RANGE+1, m_pixelsX-HOTSPOT_RANGE-1);
        y = std::clamp<u32>(y, HOTSPOT_RANGE+1, m_pixelsY-HOTSPOT_RANGE-1);
    };

    for(u32 i=0; i<450; i++)
    {
        u32 x = 4 + (this->GetRNGNumber() % boundX);
        u32 y = 4 + (this->GetRNGNumber() % boundY);
        PixelType& pixelType = GetPixelNoBoundsCheck(x, y);
        if(pixelType._GetPixelTypeStatic() == MAP_PIXEL_TYPE::SAND)
        {
            if(!GetPixelNoBoundsCheck(x, y+1).IsFilled())
            {
                // create hotspot
                u32 hotspotX = x, hotspotY = y;
                ClampHotspotCoords(hotspotX, hotspotY);
                m_volatilityHotspots.emplace_back(x, y);
            }
        }
        else if(pixelType._GetPixelTypeStatic() == MAP_PIXEL_TYPE::LAVA)
        {
            if(!GetPixelNoBoundsCheck(x, y+1).IsFilled())
            {
                u32 hotspotX = x, hotspotY = y;
                ClampHotspotCoords(hotspotX, hotspotY);
                m_volatilityHotspots.emplace_back(x, y);
            }
        }
    }
    // handle hotspots
    auto hotspotIt = m_volatilityHotspots.begin();
    while(hotspotIt != m_volatilityHotspots.end())
    {
        if( CheckVolatileStaticPixelsHotspot(hotspotIt->x, hotspotIt->y) )
        {
            hotspotIt->ttl = 200;
        }
        else
        {
            hotspotIt->ttl--;
            if(hotspotIt->ttl == 0)
            {
                hotspotIt = m_volatilityHotspots.erase(hotspotIt);
                continue;
            }
        }

        hotspotIt++;
    }
}

void Map::SimulateTick()
{
    HandleSynchronizedEvents();

    double startTime = GetMillisecondTimestamp();
    CheckStaticPixels();
    double dur = GetMillisecondTimestamp() - startTime;
    SimulateMaterial(this, m_activePixels->sandPixels);
    SimulateMaterial(this, m_activePixels->lavaPixels);
    g_debugStrings.emplace_back("Particles Sand: " + std::to_string(m_activePixels->sandPixels.size()) + " Lava: " + std::to_string(m_activePixels->lavaPixels.size()));

    char strBuf[64];
    sprintf(strBuf, "%.04lf", dur);
    g_debugStrings.emplace_back("Hotspots: " + std::to_string(m_volatilityHotspots.size()) + " StaticCheck: " + strBuf + "ms");

    m_simulationTick++;
}

void Map::HandleSynchronizedEvents()
{
    GameClient* client = GameScene::sActiveScene->GetClient();
    if(!client)
        return;
    std::vector<GameClient::SynchronizedEvent> syncedEvents;
    if( !client->GetSynchronizedEvents(m_simulationTick, syncedEvents) )
    {
        // next frame not approved by server, need to pause simulation
        return;
    }
    for(const auto& event : syncedEvents)
    {
        switch(event.eventType)
        {
            case GameClient::SynchronizedEvent::EVENT_TYPE::DRILLING:
                HandleSynchronizedEvent_Drilling(event.action_drill.playerId, event.action_drill.pos);
                break;
            case GameClient::SynchronizedEvent::EVENT_TYPE::EXPLOSION:
                HandleSynchronizedEvent_Explosion(event.action_explosion.playerId, event.action_explosion.pos, event.action_explosion.radius);
                break;
        }
    }
}

void Map::HandleSynchronizedEvent_Drilling(u32 playerId, Vector2f pos)
{
    s32 posX = (s32)(pos.x + 0.5f);
    s32 posY = (s32)(pos.y + 0.5f);
    for(s32 y=posY-9; y<=posY+9; y++)
    {
        for(s32 x=posX-9; x<=posX+9; x++)
        {
            s32 dfx = x - posX;
            s32 dfy = y - posY;
            s32 squareDist = dfx * dfx + dfy * dfy;
            if(squareDist >= 9*9-3)
                continue;
            if( IsPixelOOB(x, y) )
                continue;
            PixelType& pt = GetPixel(x, y);
            pt.SetPixel(MAP_PIXEL_TYPE::AIR);
            SetPixelColor(x, y, _GetColorFromPixelType(pt));
        }
    }
}

void Map::HandleSynchronizedEvent_Explosion(u32 playerId, Vector2f pos, f32 radius)
{
    s32 radiusI = (s32)(radius + 0.5f);
    if(radiusI <= 0)
        return;
    s32 posX = (s32)(pos.x + 0.5f);
    s32 posY = (s32)(pos.y + 0.5f);
    for(s32 y=posY-radiusI; y<=posY+radiusI; y++)
    {
        for(s32 x=posX-radiusI; x<=posX+radiusI; x++)
        {
            s32 dfx = x - posX;
            s32 dfy = y - posY;
            s32 squareDist = dfx * dfx + dfy * dfy;
            if(squareDist >= radiusI*radiusI)
                continue;
            if( IsPixelOOB(x, y) )
                continue;
            PixelType& pt = GetPixel(x, y);
            pt.SetPixel(MAP_PIXEL_TYPE::AIR);
            SetPixelColor(x, y, _GetColorFromPixelType(pt));
        }
    }
}