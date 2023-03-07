#pragma once
#include "../common/common.h"
#include "GameScene.h"

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
    friend class GameScene;
    friend class PhysicsObject;
public:
    static inline const int NUM_DRAW_LAYERS = 8;
protected:
    Object(class GameScene* parent, const AABB& boundingBox, bool requiresUpdating, u32 drawLayerMask);

    virtual void Draw(u32 layerIndex) = 0;
    virtual void Update(float timestep) = 0; // only called when requiresUpdating is true

public:
    virtual ~Object();

    AABB& GetBoundingBox() { return m_aabb; }

    virtual Vector2f GetPosition() = 0;
    virtual void UpdatePosition(const Vector2f& newPos) {};

protected:
    GameScene* m_parent;

    AABB m_aabb; // bounding box, for quick collision checks
    bool m_requiresUpdating;
    u32 m_drawLayerMask;
};


class PhysicsObject : public Object {
public:
    PhysicsObject(class GameScene* parent, const AABB& bbox, u32 drawLayerMask);

    void SetVelocity(float x, float y);
    void AddVelocity(float x, float y);
protected:
    template<typename F>
    bool DoesAABBCollide(AABB &aabb, F&& collisionFunc) {
        // check if any of the corners of the bbox collide with the map
        if (collisionFunc(aabb.GetTopLeft()) ||
            collisionFunc(aabb.GetTopRight()) ||
            collisionFunc(aabb.GetBottomLeft()) ||
            collisionFunc(aabb.GetBottomRight())) {
            return true;
        }
        return false;
    }
    void SimulatePhysics();

    Vector2f m_velocity = {0, 0};
};
