#pragma once
#include "../../common/types.h"

enum class COLLISION_SHAPE
{
    RECTANGLE = 0,
    CIRCLE = 1,
};

// moving object
class PhysicsMovingBall
{
    friend class PhysicsManager;
public:
    PhysicsMovingBall(class PhysicsManager* mgr, Vector2f pos, f32 radius);
    ~PhysicsMovingBall();

    static bool GetCollisionAngle(PhysicsMovingBall* obj, PhysicsMovingBall* otherBall, const Vector2f& moveVec, f32& timeOfCollision, Vector2f& collisionNormal);

    Vector2f GetPosition() const { return m_pos; }
    float GetRadius() const { return m_radius; }

    void ApplyForce(const Vector2f& force)
    {
        m_velocity = m_velocity + force;
    }

    bool HadCollisionQueued()
    {
        bool r = m_hasCollisionQueued;
        m_hasCollisionQueued = false;
        return r;
    }

private:
    Vector2f m_pos;
    Vector2f m_velocity{0.0f, 0.0f};
    f32 m_radius;
    class PhysicsManager* m_mgr;
    bool m_hasCollisionQueued = false;
};

// static collision shape
class PhysicsCollisionShape
{
public:
    PhysicsCollisionShape(class PhysicsManager* mgr);
    virtual ~PhysicsCollisionShape() = 0;
    virtual bool GetCollisionAngle(PhysicsMovingBall* obj, const Vector2f& moveVec, f32& timeOfCollision, Vector2f& collisionNormal) = 0;

private:
    class PhysicsManager* m_mgr;
};

class PhysicsCollisionShapeRect : public PhysicsCollisionShape
{
public:
    PhysicsCollisionShapeRect(class PhysicsManager* mgr, AABB rect) : PhysicsCollisionShape(mgr), m_rect(rect)
    {

    }

    ~PhysicsCollisionShapeRect() override
    {

    }

    void UpdateAABB(const AABB& rect)
    {
        m_rect = rect;
    }

    bool GetCollisionAngle(PhysicsMovingBall* obj, const Vector2f& moveVec, f32& timeOfCollision, Vector2f& collisionNormal) override;

private:
    AABB m_rect;
};

class PhysicsCollisionShapeCircle : public PhysicsCollisionShape
{
public:
    PhysicsCollisionShapeCircle(class PhysicsManager* mgr, Vector2f pos, f32 radius) : PhysicsCollisionShape(mgr), m_pos(pos), m_radius(radius)
    {

    }

    ~PhysicsCollisionShapeCircle() override
    {

    }

    void UpdatePosition(const Vector2f& pos)
    {
        m_pos = pos;
    }

    bool GetCollisionAngle(PhysicsMovingBall* obj, const Vector2f& moveVec, f32& timeOfCollision, Vector2f& collisionNormal) override;

private:
    Vector2f m_pos;
    f32 m_radius;
};

class PhysicsManager
{
    friend class PhysicsMovingBall;
    friend class PhysicsCollisionShape;
public:
    void Update(f32 timestep);

private:
    void ProcessBall(PhysicsMovingBall* obj, f32 timestep);

    std::vector<PhysicsMovingBall*> m_movingBalls;
    std::vector<PhysicsCollisionShape*> m_collisionShapes;
};

extern PhysicsManager gPhysicsMgr;