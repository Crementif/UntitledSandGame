#pragma once
#include "../common/common.h"
#include <unordered_set>

#define DRAW_LAYER_0    (1<<0)
#define DRAW_LAYER_1    (1<<1)
#define DRAW_LAYER_2    (1<<2)
#define DRAW_LAYER_3    (1<<3)
#define DRAW_LAYER_4    (1<<4)
#define DRAW_LAYER_5    (1<<5)
#define DRAW_LAYER_6    (1<<6)
#define DRAW_LAYER_7    (1<<7)

// the common base class for all 2D objects
class Object
{
public:
    static inline const int NUM_DRAW_LAYERS = 8;
protected:
    Object(const AABB& boundingBox, bool requiresUpdating, u32 drawLayerMask);

    virtual void Draw(u32 layerIndex) = 0;
    virtual void Update(float timestep) = 0; // only called when requiresUpdating is true

public:
    virtual ~Object();

    AABB& GetBoundingBox() { return m_aabb; }

    virtual bool CanDrag() { return false; }
    virtual Vector2f GetPosition() = 0;
    virtual void UpdatePosition(const Vector2f& newPos) {};

    static std::span<Object*> GetAllObjects();
    static void DoUpdates(float timestep);
    static void DoDraws();

    static void QueueForDeletion(Object* obj);
protected:
    AABB m_aabb; // bounding box, for quick collision checks
    u32 m_drawLayerMask;
};