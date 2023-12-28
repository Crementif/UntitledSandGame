#pragma once
#include "Map.h"

// align to 16 bytes to allow for pointer tagging in PixelType
class alignas(0x10) ActivePixelBase
{
public:
    ActivePixelBase(s32 x, s32 y, MAP_PIXEL_TYPE material, u8 seed) : x((u16)x), y((u16)y), material(material), seed(seed) {}
    virtual ~ActivePixelBase() {};

    virtual bool SimulateStep(Map* map)
    {
        return true;
    }

    virtual void PixelDeactivated(Map* map)
    {
    }

    void RemoveFromWorld(Map* map)
    {
        PixelType& pt = map->GetPixelNoBoundsCheck(x, y);
        pt.SetStaticPixelToAir();
        map->SetPixelColor(x, y, 0x00000000);
    }

    void IntegrateIntoWorld(Map* map)
    {
        PixelType& pt = map->GetPixelNoBoundsCheck(x, y);
        pt.SetDynamicPixel(this);
        // update pixel color
        map->SetPixelColor(x, y, pt.CalculatePixelColor());
    }

    u16 x;
    u16 y;
    MAP_PIXEL_TYPE material;
    u8 seed;
    u16 idleTime{0};
};

template<MAP_PIXEL_TYPE TMaterial>
class ActivePixel : public ActivePixelBase
{
public:
    ActivePixel(s32 x, s32 y, u8 seed) : ActivePixelBase(x, y, TMaterial, seed) { }

    void ChangeParticleXY(Map* map, s32 x, s32 y)
    {
        RemoveFromWorld(map);
        this->x = x;
        this->y = y;
        IntegrateIntoWorld(map);
        idleTime = 0;
    }

    void SwapPixelPosition(Map* map, s32 otherX, s32 otherY)
    {
        RemoveFromWorld(map);
        PixelType& otherPT = map->GetPixelNoBoundsCheck(otherX, otherY);
        if(otherPT.IsDynamic())
        {
            ActivePixelBase* otherActivePixel = otherPT._GetDynamicPtr();
            otherActivePixel->RemoveFromWorld(map);
            otherActivePixel->x = x;
            otherActivePixel->y = y;
            otherActivePixel->seed = seed;
            otherActivePixel->IntegrateIntoWorld(map);
        }
        else
        {
            PixelType& selfPT = map->GetPixelNoBoundsCheck(x, y);
            selfPT.SetStaticPixel(otherPT.GetPixelType(), otherPT._GetPixelSeedStatic());
            map->SetPixelColor(x, y, selfPT.CalculatePixelColor());
        }
        x = otherX;
        y = otherY;
        seed = otherPT._GetPixelSeedStatic();
        IntegrateIntoWorld(map);
        idleTime = 0;
    }

    void PixelDeactivated(Map* map) override
    {
        PixelType& pt = map->GetPixelNoBoundsCheck(x, y);
        pt.SetStaticPixel(TMaterial, seed);
        map->SetPixelColor(x, y, pt.CalculatePixelColor());
    }
};

class ActivePixelSand : public ActivePixel<MAP_PIXEL_TYPE::SAND>
{
public:
    ActivePixelSand(s32 x, s32 y, u8 seed) : ActivePixel<MAP_PIXEL_TYPE::SAND>(x, y, seed) {};

    bool SimulateStep(Map* map) final override
    {
        idleTime++;
        if(idleTime >= 20)
            return false; // deactivate pixel when EOL

        if (map->IsPixelOOBWithSafetyMargin(x, y, 1))
            return false;

        PixelType& ptBelow = map->GetPixelNoBoundsCheck(x, y+1);
        if(!ptBelow.IsSolid())
        {
            ChangeParticleXY(map, x, y+1);
            return true;
        }
        else
        {
            // if it's a liquid then drop down, but slowly
            if(ptBelow.IsLiquid() && (map->GetRNGNumber()&0xF) < 5)
            {
                SwapPixelPosition(map, x, y+1);
                return true;
            }
        }

        if((map->GetRNGNumber()&0xF) <= 5)
            return true;

        // try to move either left or right
        if(!map->GetPixelNoBoundsCheck(x - 1, y+1).IsSolid())
        {
            ChangeParticleXY(map, x - 1, y+1);
            return true;
        }
        if(!map->GetPixelNoBoundsCheck(x + 1, y+1).IsSolid())
        {
            ChangeParticleXY(map, x + 1, y+1);
            return true;
        }
        return true;
    }
};

class ActivePixelLava : public ActivePixel<MAP_PIXEL_TYPE::LAVA>
{
public:
    ActivePixelLava(s32 x, s32 y, u8 seed) : ActivePixel<MAP_PIXEL_TYPE::LAVA>(x, y, seed) {};

    bool SimulateStep(Map* map) final override
    {
        if (map->IsPixelOOBWithSafetyMargin(x, y, 2)) {
            return false;
        }

        if(!map->GetPixelNoBoundsCheck(x, y+1).IsFilled())
        {
            ChangeParticleXY(map, x, y+1);
            m_xMomentum = -1 + (map->GetRNGNumber()&2);
            return true;
        }
        if(!map->GetPixelNoBoundsCheck(x-1, y+1).IsFilled())
        {
            ChangeParticleXY(map, x-1, y+1);
            m_xMomentum = -1;
            return true;
        }
        if(!map->GetPixelNoBoundsCheck(x+1, y+1).IsFilled())
        {
            ChangeParticleXY(map, x+1, y+1);
            m_xMomentum = 1;
            return true;
        }

        // movement with lower slope is is slower and gets move delayed
        if(m_slopeMoveDelay > 0)
        {
            m_slopeMoveDelay--;
            return true;
        }
        m_slopeMoveDelay = 6;

        if(!map->GetPixelNoBoundsCheck(x-2, y+1).IsFilled())
        {
            ChangeParticleXY(map, x-2, y+1);
            m_xMomentum = -2;
            return true;
        }
        if(!map->GetPixelNoBoundsCheck(x+2, y+1).IsFilled())
        {
            ChangeParticleXY(map, x+2, y+1);
            m_xMomentum = 2;
            return true;
        }

        // keep momentum and move along x axis
        // if above is free then occasionally spawn a smoke pixel
        if(!map->GetPixelNoBoundsCheck(x, y-1).IsFilled() && (map->GetRNGNumber()&127) < 1)
        {
            map->SpawnMaterialPixel(MAP_PIXEL_TYPE::SMOKE, seed, x, y-1);
        }

        idleTime++;
        if(idleTime >= 20)
            return false; // deactivate particle after it got stuck (couldn't change position)

        if(m_xMomentum < 0)
        {
            if(!map->GetPixel(x-1, y).IsFilled())
                ChangeParticleXY(map, x-1, y);
            else
                m_xMomentum = 1;
            return true;
        }
        if(m_xMomentum > 0)
        {
            if(!map->GetPixel(x+1, y).IsFilled())
                ChangeParticleXY(map, x+1, y);
            else
                m_xMomentum = -1;
            return true;
        }
        return true;
    }

private:
    s8 m_xMomentum{0};
    s8 m_slopeMoveDelay{0};
};

class ActivePixelSmoke : public ActivePixel<MAP_PIXEL_TYPE::SMOKE>
{
public:
    ActivePixelSmoke(Map* map, s32 x, s32 y, u8 seed) : ActivePixel<MAP_PIXEL_TYPE::SMOKE>(x, y, seed)
    {
        m_ttl = 40 + (map->GetRNGNumber()%60);
    }

    bool SimulateStep(Map* map) final override
    {
        m_slowdown ^= 1;
        if(m_slowdown == 0)
            return true;

        m_timeAlive++;
        s32 xOffset = -2 + (map->GetRNGNumber()%5);
        PixelType& ptAfter = map->GetPixel(x + xOffset, y-1);
        if(!ptAfter.IsSolid())
        {
            ChangeParticleXY(map, x + xOffset, y-1);
            idleTime = 0;
            return true;
        }
        else
        {
            idleTime++;
            if(idleTime > 5)
                return false;
        }

        if (m_timeAlive > m_ttl)
            return false;
        return true;
    }

    void PixelDeactivated(Map* map) final override
    {
        // smoke just disappears
        map->GetPixel(x, y).SetStaticPixelToAir();
        map->SetPixelColor(x, y, 0x00000000);
    }

private:
    u32 m_ttl;
    u32 m_timeAlive{0};
    u8 m_slowdown{0};
};

class ActivePixelCollection
{
public:
    ~ActivePixelCollection()
    {
        while(!sandPixels.empty())
        {
            delete sandPixels.back();
            sandPixels.pop_back();
        }
        while(!lavaPixels.empty())
        {
            delete lavaPixels.back();
            lavaPixels.pop_back();
        }
        while(!smokePixels.empty())
        {
            delete smokePixels.back();
            smokePixels.pop_back();
        }
    }

    // todo - do we even need per material grouping still?
    std::vector<ActivePixelSand*> sandPixels;
    std::vector<ActivePixelLava*> lavaPixels;
    std::vector<ActivePixelSmoke*> smokePixels;
};

MAP_PIXEL_TYPE _GetPixelTypeFromTGAColor(u32 c);
