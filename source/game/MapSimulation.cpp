#include "Map.h"
#include "MapPixels.h"
#include "GameSceneIngame.h"
#include "MapFlungPixels.h"
#include "MapGravityPixels.h"
#include "../framework/audio/audio.h"
#include "../framework/debug.h"

void Map::SpawnMaterialPixel(MAP_PIXEL_TYPE materialType, u8 materialSeed, s32 x, s32 y)
{
    switch(materialType)
    {
        case MAP_PIXEL_TYPE::SAND:
            m_activePixels->sandPixels.emplace_back(new ActivePixelSand(x, y, materialSeed));
            break;
        case MAP_PIXEL_TYPE::LAVA:
            m_activePixels->lavaPixels.emplace_back(new ActivePixelLava(x, y, materialSeed));
            break;
        case MAP_PIXEL_TYPE::SMOKE:
            m_activePixels->smokePixels.emplace_back(new ActivePixelSmoke(this, x, y, materialSeed));
            break;
        default:
            // for non-dynamic pixels we just place them as-is
            //PixelType& pt = GetPixel(x, y);
            //if(!pt.IsFilled())
            //    pt.SetPixelRGBA8888(materialType);
            // -> bad because static pixels tend to get stuck in the air on stuff like smoke or other temporary/moving pixels
            break;
    }
}

// technically this should grab the material type from the pixel at x/y,
// but in most cases we already have the material type so it's an extra optimization to not recalculate it
void Map::ReanimateStaticPixel(MAP_PIXEL_TYPE materialType, u8 materialSeed, s32 x, s32 y)
{
    SpawnMaterialPixel(materialType, materialSeed, x, y);
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

// check a small rectangle area for pixels that might need reanimation
// all volatility hotspots NEED to be created within the map bounds (with a safety margin of HOTSPOT_CHECK_RADIUS)
bool Map::DoVolatilityRadiusCheckForStaticPixels(u32 x, u32 y)
{
    // randomly check a few pixels in each cell to see if we need to simulate them due to surrounding conditions changing
    bool hasActivity = false;
    const u32 radiusStartX = x - (HOTSPOT_CHECK_RADIUS/2);
    const u32 radiusStartY = y - (HOTSPOT_CHECK_RADIUS/2);

    u32 earlyExitAttempts = HOTSPOT_CHECK_ATTEMPTS_EARLY_EXIT; // early exit if we keep finding pixels that can't be reanimated
    for(u32 i=0; i<HOTSPOT_CHECK_ATTEMPTS && earlyExitAttempts != 0; i++)
    {
        const u32 rdX = this->GetRNGNumber() % HOTSPOT_CHECK_RADIUS;
        const u32 rdY = this->GetRNGNumber() % HOTSPOT_CHECK_RADIUS;
        const u32 px = radiusStartX + rdX;
        const u32 py = radiusStartY + rdY;

        PixelType& pixelType = GetPixelNoBoundsCheck(px, py);
        if (pixelType.IsDynamic())
            continue;
        auto mat = pixelType._GetPixelTypeStatic();
        if(mat == MAP_PIXEL_TYPE::SAND)
        {
            earlyExitAttempts = std::numeric_limits<u32>::max();
            if(!GetPixelNoBoundsCheck(px, py+1).IsFilled())
            {
                hasActivity = true;
                ReanimateStaticPixel(MAP_PIXEL_TYPE::SAND, pixelType._GetPixelSeedStatic(), px, py);
            }
        }
        else if(mat == MAP_PIXEL_TYPE::LAVA)
        {
            earlyExitAttempts = std::numeric_limits<u32>::max();
            // check for any (diagonal) downwards lava flow
            if(!GetPixelNoBoundsCheck(px-1, py+1).IsFilled() || !GetPixelNoBoundsCheck(px, py+1).IsFilled() || !GetPixelNoBoundsCheck(px+1, py+1).IsFilled())
            {
                hasActivity = true;
                ReanimateStaticPixel(MAP_PIXEL_TYPE::LAVA, pixelType._GetPixelSeedStatic(), px, py);
            }
            // note: for performance reasons we don't check for sideways lava flow
            // else if (!GetPixelNoBoundsCheck(px-1, py).IsFilled() || !GetPixelNoBoundsCheck(px+1, py).IsFilled()) {
            //     hasActivity = true;
            //     ReanimateStaticPixel(MAP_PIXEL_TYPE::LAVA, pixelType._GetPixelSeedStatic(), px, py);
            // }
        }
        else {
            earlyExitAttempts--;
        }
    }
    return hasActivity;
}

void Map::FindRandomHotspots()
{
    u32 boundStartX = HOTSPOT_CHECK_RADIUS + 1;
    u32 boundStartY = HOTSPOT_CHECK_RADIUS + 1;
    u32 boundLengthX = m_pixelsX - HOTSPOT_CHECK_RADIUS - HOTSPOT_CHECK_RADIUS - 1;
    u32 boundLengthY = m_pixelsY - HOTSPOT_CHECK_RADIUS - HOTSPOT_CHECK_RADIUS - 1;

    for(u32 i=0; i<FIND_HOTSPOT_ATTEMPTS; i++)
    {
        u32 x = boundStartX + (this->GetRNGNumber() % boundLengthX);
        u32 y = boundStartY + (this->GetRNGNumber() % boundLengthY);
        PixelType& pixelType = GetPixelNoBoundsCheck(x, y);
        if (pixelType._GetPixelTypeStatic() == MAP_PIXEL_TYPE::SAND || pixelType._GetPixelTypeStatic() == MAP_PIXEL_TYPE::LAVA)
        {
            if (!GetPixelNoBoundsCheck(x, y+1).IsFilled())
            {
                // create a new hotspot
                m_volatilityHotspots.emplace_back(x, y, FIND_HOTSPOT_LIFETIME);
            }
        }
    }
}

void Map::UpdateVolatilityHotspots()
{
    // handle hotspots checking spots
    auto hotspotIt = m_volatilityHotspots.begin();
    while(hotspotIt != m_volatilityHotspots.end())
    {
        if(DoVolatilityRadiusCheckForStaticPixels(hotspotIt->x, hotspotIt->y))
        {
            hotspotIt->ttl = FIND_HOTSPOT_LIFETIME_EXTENSION;
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

        ++hotspotIt;
    }
}

void Map::SimulateTick()
{
    HandleSynchronizedEvents();

    DebugProfile::Start("Scene -> Simulation -> Find Volatile Static Pixels");
    FindRandomHotspots();
    UpdateVolatilityHotspots();
    DebugProfile::End("Scene -> Simulation -> Find Volatile Static Pixels");
    DebugProfile::Start("Scene -> Simulation -> Simulate Active Pixels");
    SimulateMaterial(this, m_activePixels->sandPixels);
    SimulateMaterial(this, m_activePixels->lavaPixels);
    SimulateMaterial(this, m_activePixels->smokePixels);
    DebugProfile::End("Scene -> Simulation -> Simulate Active Pixels");
    DebugLog::Printf("Currently Active Pixels: Sand: %u Lava: %u Smoke: %u", m_activePixels->sandPixels.size(), m_activePixels->lavaPixels.size(), m_activePixels->smokePixels.size());
    DebugLog::Printf("Currently Active Hotspot Locations: %u", m_volatilityHotspots.size());

    SimulateFlungPixels();
    SimulateGravityPixels();

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
                // todo: find a way to properly clean up a sound... we want to have multiple explosions at the same time probably?
                // todo: also make audio volume depend on distance to explosion. Same for all other audio bytes probably!
                HandleSynchronizedEvent_Explosion(event.action_explosion.playerId, event.action_explosion.pos, event.action_explosion.radius, event.action_explosion.force);
                break;
            case GameClient::SynchronizedEvent::EVENT_TYPE::GRAVITY:
                HandleSynchronizedEvent_Gravity(event.action_gravity.playerId, event.action_gravity.pos, event.action_gravity.strength, event.action_gravity.lifetimeTicks);
                break;
        }
    }
}

#define DIGGING_RADIUS      11
#define TRANSITION_RADIUS   14
#define SIMULATE_RADIUS     15
void Map::HandleSynchronizedEvent_Drilling(u32 playerId, Vector2f pos)
{
    const s32 posX = (s32)(pos.x + 0.5f);
    const s32 posY = (s32)(pos.y + 0.5f);
    for (s32 y=posY-SIMULATE_RADIUS; y<=posY+SIMULATE_RADIUS; y++)
    {
        const s32 dfy = y - posY;
        const s32 dfySq = dfy * dfy;
        for (s32 x=posX-SIMULATE_RADIUS; x<=posX+SIMULATE_RADIUS; x++)
        {
            if (IsPixelOOBWithSafetyMargin(x, y, HOTSPOT_CHECK_RADIUS/2))
                continue;

            const s32 dfx = x - posX;
            const s32 dfxSq = dfx * dfx;

            const s32 squareDist = dfxSq + dfySq;
            bool shouldDig = squareDist < (DIGGING_RADIUS*DIGGING_RADIUS-MAP_PIXEL_ZOOM);
            bool shouldTransition = squareDist < (TRANSITION_RADIUS*TRANSITION_RADIUS-MAP_PIXEL_ZOOM);
            bool shouldSimulate = squareDist < (SIMULATE_RADIUS*SIMULATE_RADIUS-MAP_PIXEL_ZOOM);

            if (shouldDig) {
                PixelType& pt = GetPixelNoBoundsCheck(x, y);
                pt.SetStaticPixelToAir();
                SetPixelColor(x, y, pt.CalculatePixelColor());
            }
            else if (shouldTransition) {
                PixelType& pt = GetPixelNoBoundsCheck(x, y);
                if (pt.IsDimmable()) {
                    u8 seed = pt._GetPixelSeedStatic();
                    u8 maxSeed = _GetMaxVariationsFromPixelType(pt._GetPixelTypeStatic());
                    if (seed >= maxSeed) {
                        continue;
                    }

                    pt._SetPixelSeedStatic(seed + maxSeed);
                    SetPixelColor(x, y, pt.CalculatePixelColor());
                }
            }
            else if (shouldSimulate) {
                DoVolatilityRadiusCheckForStaticPixels(x, y);
            }
        }
    }
}

bool _CanMaterialBeFlung(MAP_PIXEL_TYPE mat)
{
    switch(mat)
    {
        case MAP_PIXEL_TYPE::SOIL:
        case MAP_PIXEL_TYPE::SAND:
        case MAP_PIXEL_TYPE::GRASS:
        case MAP_PIXEL_TYPE::LAVA:
            return true;
        default:
            return false;
    }
}

constexpr u32 EXPLOSION_HOTSPOT_ATTEMPTS = 16;
void Map::HandleSynchronizedEvent_Explosion(u32 playerId, Vector2f pos, f32 radius, f32 force)
{
    s32 radiusI = (s32)(radius + 0.5f);
    if (radiusI <= 0)
        return;
    s32 posX = (s32)(pos.x + 0.5f);
    s32 posY = (s32)(pos.y + 0.5f);
    u32 flungCountTracker[(size_t)MAP_PIXEL_TYPE::_COUNT]{}; // keeps track of how many pixels per material need to be flung

    const s32 radiusEdgeInner = radiusI * radiusI;
    const s32 radiusEdgeOuter = (radiusI+1) * (radiusI+1);
    for (s32 y=posY-radiusI-1; y<=posY+radiusI+1; y++)
    {
        const s32 radY = y - posY;
        const s32 radYSq = radY * radY;
        for (s32 x=posX-radiusI-1; x<=posX+radiusI-1; x++)
        {
            if (IsPixelOOBWithSafetyMargin(x, y, HOTSPOT_CHECK_RADIUS/2))
                continue;

            const s32 radX = x - posX;
            const s32 radXSq = radX * radX;

            // check if inside destruction radius
            const s32 distSq = radXSq + radYSq;
            const bool shouldCheckHotspot = distSq <= radiusEdgeOuter;
            const bool shouldDig = distSq <= radiusEdgeInner;
            const u8 pixelRNG = GetRNGNumber()&0x7;

            if (!shouldDig) {
                if (shouldCheckHotspot && pixelRNG < 0x2) {
                    for (u32 i=0; i<EXPLOSION_HOTSPOT_ATTEMPTS; i++) {
                        DoVolatilityRadiusCheckForStaticPixels(x, y);
                    }
                }
                // outside radius (even though it might be just outside the edge), skip
                continue;
            }

            PixelType& pt = GetPixel(x, y);

            // decide if the particle should be flung
            if (MAP_PIXEL_TYPE material = pt.GetPixelType(); _CanMaterialBeFlung(material) && pixelRNG < 0x3)
                flungCountTracker[(size_t)material]++;

            // change pixels to air
            pt.SetStaticPixelToAir();
            SetPixelColor(x, y, pt.CalculatePixelColor());

            // randomly spawn smoke
            if (pixelRNG < 0x1)
            {
                SpawnMaterialPixel(MAP_PIXEL_TYPE::SMOKE, _GetRandomSeedFromPixelType(MAP_PIXEL_TYPE::SMOKE), x, y);
                PixelType& pt2 = GetPixelNoBoundsCheck(x, y);
                SetPixelColor(x, y, pt2.CalculatePixelColor());
            }
        }
    }

    // second pass to create all the flung pixels
    if (radius >= 2.0f)
    {
        const float radPxStartX = pos.x - radius;
        const float radPxStartY = pos.y - radius;
        const float radiusDistance = 2.0f * radius;
        const float radSq = radius * radius;

        for(u32 matIndex = 0; matIndex < (size_t)MAP_PIXEL_TYPE::_COUNT; matIndex++)
        {
            MAP_PIXEL_TYPE mat = (MAP_PIXEL_TYPE)matIndex;
            while(flungCountTracker[matIndex] > 0)
            {
                flungCountTracker[matIndex]--;
                // find a random location within the circle
                while( true )
                {
                    f32 relX = GetRNGFloat01();
                    f32 relY = GetRNGFloat01();
                    Vector2f pixelPos(radPxStartX + (relX * radiusDistance), radPxStartY + (relY * radiusDistance));
                    // spawn
                    Vector2f flungDir = pixelPos - pos;
                    if (flungDir.LengthSquare() >= radSq)
                        continue;
                    new FlungPixel(this, pixelPos, flungDir * force, mat, _GetRandomSeedFromPixelType(mat));
                    break;
                }
            }
        }
    }
}

void Map::HandleSynchronizedEvent_Gravity(u32 playerId, Vector2f pos, f32 strength, f32 lifetimeTicks)
{
    new GravityPixel(this, pos, strength, (u32)lifetimeTicks);
}