#pragma once
#include "Map.h"
#include <vector>

#include <coreinit/debug.h>

u32 _GetColorFromPixelType(PixelType& pixelType);

class ActivePixelBase
{
public:
    ActivePixelBase(s32 x, s32 y, MAP_PIXEL_TYPE material) : x((u16)x), y((u16)y), material(material) {}
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
        PixelType& pt = map->GetPixel(x, y);
        pt.SetPixel(MAP_PIXEL_TYPE::AIR);
        map->SetPixelColor(x, y, 0x00000000);
    }

    void IntegrateIntoWorld(Map* map)
    {
        PixelType& pt = map->GetPixel(x, y);
        pt.SetDynamicPixel(this);
        // update pixel color
        map->SetPixelColor(x, y, _GetColorFromPixelType(pt));
    }

    u16 x;
    u16 y;
    MAP_PIXEL_TYPE material;
    u16 idleTime{0};
};

template<MAP_PIXEL_TYPE TMaterial>
class ActivePixel : public ActivePixelBase
{
public:
    ActivePixel(s32 x, s32 y) : ActivePixelBase(x, y, TMaterial) { }

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
            otherActivePixel->IntegrateIntoWorld(map);
        }
        else
        {
            PixelType& selfPT = map->GetPixelNoBoundsCheck(x, y);
            selfPT.SetPixel(otherPT.GetPixelType());
            map->SetPixelColor(x, y, _GetColorFromPixelType(selfPT));
        }
        x = otherX;
        y = otherY;
        IntegrateIntoWorld(map);
        idleTime = 0;
    }

    void PixelDeactivated(Map* map) override
    {
        PixelType& pt = map->GetPixel(x, y);
        pt.SetPixel(TMaterial);
        map->SetPixelColor(x, y, _GetColorFromPixelType(pt));
    }

};

class ActivePixelSand : public ActivePixel<MAP_PIXEL_TYPE::SAND>
{
public:
    ActivePixelSand(s32 x, s32 y) : ActivePixel<MAP_PIXEL_TYPE::SAND>(x, y) {};

    bool SimulateStep(Map* map) final override
    {
        idleTime++;
        PixelType& ptBelow = map->GetPixel(x, y+1);
        if(!ptBelow.IsSolid())
        {
            ChangeParticleXY(map, x, y+1);
            return true;
        }
        else
        {
            // if it's a liquid then drop down, but slowly
            if( ptBelow.IsLiquid() && (map->GetRNGNumber()&0xF) < 5)
            {
                SwapPixelPosition(map, x, y+1);
                return true;
            }
        }

        if((map->GetRNGNumber()&0xF) <= 5)
            return true;

        // try to move either left or right
        if(!map->IsPixelOOB(x-1, y+1) && !map->GetPixelNoBoundsCheck(x - 1, y+1).IsSolid())
        {
            ChangeParticleXY(map, x - 1, y+1);
            return true;
        }
        if(!map->IsPixelOOB(x+1, y+1) && !map->GetPixelNoBoundsCheck(x + 1, y+1).IsSolid())
        {
            ChangeParticleXY(map, x + 1, y+1);
            return true;
        }
        if(idleTime >= 20)
        {
            return false; // inactivate particle
        }
        return true;
    }
};

class ActivePixelLava : public ActivePixel<MAP_PIXEL_TYPE::LAVA>
{
public:
    ActivePixelLava(s32 x, s32 y) : ActivePixel<MAP_PIXEL_TYPE::LAVA>(x, y) {};

    bool SimulateStep(Map* map) final override
    {
        // try moving down if possible
        if (!map->IsPixelOOB(x, y+1) && !map->GetPixelNoBoundsCheck(x, y+1).IsFilled())
        {
            ChangeParticleXY(map, x, y+1);
            m_xMomentum = -1 + (map->GetRNGNumber()&2);
            m_lavaNoEventTime = 0;
            return true;
        }
        if(!map->IsPixelOOB(x-1, y+1) && !map->GetPixelNoBoundsCheck(x-1, y+1).IsFilled())
        {
            ChangeParticleXY(map, x-1, y+1);
            m_xMomentum = -1;
            m_lavaNoEventTime = 0;
            return true;
        }
        if(!map->IsPixelOOB(x+1, y+1) && !map->GetPixelNoBoundsCheck(x+1, y+1).IsFilled())
        {
            ChangeParticleXY(map, x+1, y+1);
            m_xMomentum = 1;
            m_lavaNoEventTime = 0;
            return true;
        }

        // movement with lower slope is slowish
        if(m_moveDelay > 0)
        {
            m_moveDelay--;
            return true;
        }
        m_moveDelay = 6;

        idleTime++;


        if(!map->IsPixelOOB(x-2, y+1) && !map->GetPixelNoBoundsCheck(x-2, y+1).IsFilled())
        {
            ChangeParticleXY(map, x-2, y+1);
            m_xMomentum = -2;
            m_lavaNoEventTime = 0;
            return true;
        }
        if(!map->IsPixelOOB(x+2, y+1) && !map->GetPixelNoBoundsCheck(x+2, y+1).IsFilled())
        {
            ChangeParticleXY(map, x+2, y+1);
            m_xMomentum = 2;
            m_lavaNoEventTime = 0;
            return true;
        }
        // keep momentum and move along x axis
        // if above is free then occasionally spawn a smoke pixel
        if(!map->IsPixelOOB(x, y-1) && !map->GetPixelNoBoundsCheck(x, y-1).IsFilled() && (map->GetRNGNumber()&127) < 1)
        {
            map->SpawnMaterialPixel(MAP_PIXEL_TYPE::SMOKE, x, y-1);
        }

        if(m_lavaNoEventTime >= 200)
            return false; // inactivate particle
        m_lavaNoEventTime++;

        if(m_xMomentum < 0)
        {
            if(!map->IsPixelOOB(x-1, y) && !map->GetPixelNoBoundsCheck(x-1, y).IsFilled())
                ChangeParticleXY(map, x-1, y);
            else
                m_xMomentum = 1;
            return true;
        }
        if(m_xMomentum > 0)
        {
            if(!map->IsPixelOOB(x+1, y) && !map->GetPixelNoBoundsCheck(x+1, y).IsFilled())
                ChangeParticleXY(map, x+1, y);
            else
                m_xMomentum = -1;
            return true;
        }
        return true;
    }

private:
    s8 m_xMomentum{0};
    s8 m_moveDelay{0};
    u8 m_lavaNoEventTime{0};
};

class ActivePixelSmoke : public ActivePixel<MAP_PIXEL_TYPE::SMOKE>
{
public:
    ActivePixelSmoke(Map* map, s32 x, s32 y) : ActivePixel<MAP_PIXEL_TYPE::SMOKE>(x, y)
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

        if(m_timeAlive > m_ttl)
            return false;
        return true;
    }

    void PixelDeactivated(Map* map) final override
    {
        // smoke just disappears
        map->GetPixel(x, y).SetPixel(MAP_PIXEL_TYPE::AIR);
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
// format: RRGGBBAA
u32 _GetColorFromPixelType(PixelType& pixelType);
u32 _CalculateDimColor(u32 color, float dimFactor);
