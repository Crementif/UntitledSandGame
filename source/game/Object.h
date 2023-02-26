#pragma once
#include "../common/common.h"
#include "Map.h"
#include <unordered_set>

#define DRAW_LAYER_0    (1<<0)
#define DRAW_LAYER_1    (1<<1)
#define DRAW_LAYER_2    (1<<2)
#define DRAW_LAYER_3    (1<<3)
#define DRAW_LAYER_4    (1<<4)
#define DRAW_LAYER_5    (1<<5)
#define DRAW_LAYER_6    (1<<6)
#define DRAW_LAYER_7    (1<<7)

// draw layer helper aliases
#define DRAW_LAYER_PLAYERS  (4)

// the common base class for all 2D objects
class Object
{
    friend class PhysicsObject;
public:
    static inline const int NUM_DRAW_LAYERS = 8;
protected:
    Object(const AABB& boundingBox, bool requiresUpdating, u32 drawLayerMask);

    virtual void Draw(u32 layerIndex) = 0;
    virtual void Update(float timestep) = 0; // only called when requiresUpdating is true

public:
    virtual ~Object();

    AABB& GetBoundingBox() { return m_aabb; }

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


class PhysicsObject : public Object {
public:
    PhysicsObject(const AABB& bbox, u32 drawLayerMask): Object(bbox, true, drawLayerMask) {
    }

    void SetVelocity(float x, float y) {
        m_velocity = Vector2f(x, y);
    }
    void AddVelocity(float x, float y) {
        m_velocity.x += x;
        m_velocity.y += y;
    }
protected:
    static bool DoesCornerCollide(Vector2f cornerPos) {
        Map* map = GetCurrentMap();

        // todo: loss of precision, since the position of a corner could be overlapping multiple pixels
        return map->GetPixel((s32)cornerPos.x, (s32)cornerPos.y).IsCollideWithObjects();
    }

    static bool DoesAABBCollide(AABB& aabb) {
        // check if any of the corners of the bbox collide with the map
        if (DoesCornerCollide(aabb.GetTopLeft()) ||
            DoesCornerCollide(aabb.GetTopRight()) ||
            DoesCornerCollide(aabb.GetBottomLeft()) ||
            DoesCornerCollide(aabb.GetBottomRight())) {
            return true;
        }
        return false;
    }

    void SimulatePhysics() {
        AABB newPos = m_aabb + m_velocity;
        // Try moving the entire distance first
        if (!DoesAABBCollide(newPos)) {
            m_aabb = newPos;
            return;
        }

        // use binary search to find the distance we can actually move
        Vector2f moveVec = newPos.pos - m_aabb.pos;
        Vector2f tryMoveVec = moveVec * 0.5f;
        bool hasClosestTarget = false;
        AABB closestTarget = {0, 0, 0,0};
        for (s32 t=0; t<5; t++) {
            AABB tmpTarget = newPos + tryMoveVec;
            if (!DoesAABBCollide(tmpTarget)) {
                hasClosestTarget = true;
                closestTarget = tmpTarget;
                // try moving further
                tryMoveVec = tryMoveVec + tryMoveVec * 0.5f;
                continue;
            }
            // try a shorter distance
            tryMoveVec = tryMoveVec * 0.5f;
        }
        if (hasClosestTarget)
            m_aabb = closestTarget;
        else {
            SetVelocity(0, 0);
        }
    };

    Vector2f m_velocity = {0, 0};
};