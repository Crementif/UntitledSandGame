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

    void ChangeParticleXY(Map* map, s32 x, s32 y)
    {
        RemoveFromWorld(map);
        this->x = x;
        this->y = y;
        IntegrateIntoWorld(map);
        idleTime = 0;
    }

    void PixelDeactivated(Map* map) override
    {
        PixelType& pt = map->GetPixel(x, y);
        pt.SetPixel(TMaterial);
    }

};

class ActivePixelSand : public ActivePixel<MAP_PIXEL_TYPE::SAND>
{
public:
    ActivePixelSand(s32 x, s32 y) : ActivePixel<MAP_PIXEL_TYPE::SAND>(x, y) {};

    bool SimulateStep(Map* map) override
    {
        idleTime++;
        if(!map->GetPixel(x, y+1).IsSolid())
        {
            ChangeParticleXY(map, x, y+1);
            return true;
        }

        if((map->GetRNGNumber()&0xF) <= 5)
            return true;

        // try to move either left or right
        if(!map->GetPixel(x - 1, y+1).IsSolid())
        {
            ChangeParticleXY(map, x - 1, y+1);
            return true;
        }
        if(!map->GetPixel(x + 1, y+1).IsSolid())
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

    bool SimulateStep(Map* map) override
    {
        // try moving down if possible
        if(!map->GetPixel(x, y+1).IsFilled())
        {
            ChangeParticleXY(map, x, y+1);
            m_xMomentum = -1 + (map->GetRNGNumber()&2);
            return true;
        }
        if(!map->GetPixel(x-1, y+1).IsFilled())
        {
            ChangeParticleXY(map, x-1, y+1);
            m_xMomentum = -1;
            return true;
        }
        if(!map->GetPixel(x+1, y+1).IsFilled())
        {
            ChangeParticleXY(map, x+1, y+1);
            m_xMomentum = 1;
            return true;
        }

        // movement with lower slope is slowish
        if(m_moveDelay > 0)
        {
            m_moveDelay--;
            return true;
        }
        m_moveDelay = 4;

        idleTime++;


        if(!map->GetPixel(x-2, y+1).IsFilled())
        {
            ChangeParticleXY(map, x-2, y+1);
            m_xMomentum = -2;
            return true;
        }
        if(!map->GetPixel(x+2, y+1).IsFilled())
        {
            ChangeParticleXY(map, x+2, y+1);
            m_xMomentum = 2;
            return true;
        }
        // keep momentum and move along x axis
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

        if(idleTime >= 20)
        {
            return false; // inactivate particle
        }
        return true;
    }

private:
    s8 m_xMomentum{0};
    s8 m_moveDelay{0};
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
    }

    // todo - do we even need per material grouping still?
    std::vector<ActivePixelSand*> sandPixels;
    std::vector<ActivePixelLava*> lavaPixels;
};