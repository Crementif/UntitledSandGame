#include "Object.h"
#include "Map.h"

Object::Object(class GameScene* parent, const AABB& boundingBox, bool requiresUpdating, u32 drawLayerMask): m_parent(parent), m_aabb(boundingBox), m_requiresUpdating(requiresUpdating), m_drawLayerMask(drawLayerMask)
{
    m_parent->RegisterObject(this);
}

Object::~Object()
{
    m_parent->UnregisterObject(this);
}

PhysicsObject::PhysicsObject(struct GameScene *parent, const AABB &bbox, u32 drawLayerMask) : Object(parent, bbox, true, drawLayerMask) {
}

void PhysicsObject::SetVelocity(float x, float y) {
    m_velocity = Vector2f(x, y);
}

void PhysicsObject::AddVelocity(float x, float y) {
    m_velocity.x += x;
    m_velocity.y += y;
}

bool PhysicsObject::DoesCornerCollide(Vector2f cornerPos) {
    Map* map = GetCurrentMap();

    // todo: loss of precision, since the position of a corner could be overlapping multiple pixels
    return map->GetPixel((s32)cornerPos.x, (s32)cornerPos.y).IsCollideWithObjects();
}

bool PhysicsObject::DoesAABBCollide(AABB &aabb) {
    // check if any of the corners of the bbox collide with the map
    if (DoesCornerCollide(aabb.GetTopLeft()) ||
        DoesCornerCollide(aabb.GetTopRight()) ||
        DoesCornerCollide(aabb.GetBottomLeft()) ||
        DoesCornerCollide(aabb.GetBottomRight())) {
        return true;
    }
    return false;
}

void PhysicsObject::SimulatePhysics() {
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
}
