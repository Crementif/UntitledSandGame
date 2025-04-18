#include "../../common/types.h"
#include "physics.h"
#include <coreinit/debug.h>
#include <whb/log.h>

PhysicsMovingBall::PhysicsMovingBall(class PhysicsManager* mgr, Vector2f pos, f32 radius) : m_pos(pos), m_radius(radius), m_mgr(mgr)
{
    mgr->m_movingBalls.emplace_back(this);
}

PhysicsMovingBall::~PhysicsMovingBall()
{
    m_mgr->m_movingBalls.erase(std::find(m_mgr->m_movingBalls.begin(), m_mgr->m_movingBalls.end(), this));
}

PhysicsCollisionShape::PhysicsCollisionShape(class PhysicsManager* mgr) : m_mgr(mgr)
{
    mgr->m_collisionShapes.emplace_back(this);
}

PhysicsCollisionShape::~PhysicsCollisionShape()
{
    m_mgr->m_collisionShapes.erase(std::find(m_mgr->m_collisionShapes.begin(), m_mgr->m_collisionShapes.end(), this));
}

// returns true on collision
// collidingPointOut is set to the point within the rect that has the closest distance to the circle center
bool _CheckCircleRectIntersection(const Vector2f circlePos, float radiusSquared, const AABB& rect, Vector2f& collidingPointOut)
{
    Vector2f collidingPoint = circlePos.Clamp(rect.pos, rect.pos + rect.scale);
    f32 dist = collidingPoint.DistanceSquare(circlePos);
    collidingPointOut = collidingPoint;
    if (dist > radiusSquared)
    {
        // no collision
        return false;
    }
    return true;
}

bool PhysicsCollisionShapeRect::GetCollisionAngle(PhysicsMovingBall* obj, const Vector2f& moveVec, f32& timeOfCollision, Vector2f& collisionNormal)
{
    f32 squareRadius = obj->GetRadius();
    squareRadius *= squareRadius;
    // check if the destination is inside the collidable shape

    Vector2f rectPos2 = m_rect.pos + m_rect.scale;

    f32 radiusSquared = obj->GetRadius() * obj->GetRadius();

    // first we do a quick check to see if the ball collides after finishing the current movement
    Vector2f targetPos = obj->GetPosition() + moveVec;
    Vector2f collidingPoint = targetPos.Clamp(m_rect.pos, rectPos2);
    f32 dist = collidingPoint.DistanceSquare(targetPos);
    if( dist > radiusSquared)
        return false; // no collision

    // use binary search to approximate time of collision
    f32 currentScaler = 0.5f;
    f32 currentStep = 0.25f;
    f32 nearestNonCollisionScaler = 0.0f;
    Vector2f collisionAngle{0.0, 1.0f};
    bool foundTOC = false;
    for(u32 i=0; i<6; i++)
    {
        targetPos = obj->GetPosition() + moveVec * currentScaler;
        Vector2f collidingPoint;
        if(!_CheckCircleRectIntersection(targetPos, radiusSquared, m_rect, collidingPoint))
        {
            nearestNonCollisionScaler = currentScaler;
            currentScaler += currentStep;
            collisionAngle = targetPos - collidingPoint;
            foundTOC = true;
        }
        else
        {
            currentScaler -= currentStep;
        }
        currentStep *= 0.5f;
    }
    if(!foundTOC)
    {
        if(_CheckCircleRectIntersection(obj->GetPosition(), radiusSquared, m_rect, collidingPoint))
        {
            // object already inside of rect, ignore collisions
            return false;
        }
        collisionAngle = obj->GetPosition() - collidingPoint;
        collisionNormal = collisionAngle.GetNormalized();
        timeOfCollision = 0.0f;
        return true;
    }
    collisionNormal = collisionAngle.GetNormalized();
    timeOfCollision = nearestNonCollisionScaler;
    return true;
}

bool _GetCollisionAngleCircleCircle(const Vector2f& posA, float radiusA, const Vector2f& moveVec, const Vector2f& posB, float radiusB, f32& timeOfCollision, Vector2f& collisionNormal)
{
    Vector2f targetPos = posA + moveVec;
    //Vector2f otherPos = otherBall->GetPosition();
    Vector2f dist = targetPos - posB;
    f32 touchDist = radiusA + radiusB;
    f32 distAtTarget = dist.LengthSquare();
    if(distAtTarget > touchDist * touchDist)
        return false;
    // to avoid balls getting stuck inside other circles, we ignore collisions if they are moving apart
    f32 distAtOrigin = (posB - posA).LengthSquare();
    if(distAtOrigin < distAtTarget)
        return false;
    // math is hard, use brute force approach for now to determine time of collision
    f32 currentScaler = 0.5f;
    f32 currentStep = 0.25f;
    f32 nearestNonCollisionScaler = 0.0f;
    Vector2f collisionAngle{0.0, 1.0f};
    for(u32 i=0; i<6; i++)
    {
        targetPos = posA + moveVec * currentScaler;
        dist = targetPos - posB;
        if( dist.LengthSquare() > touchDist * touchDist)
        {
            // no collision, try longer scaler
            nearestNonCollisionScaler = currentScaler;
            currentScaler += currentStep;
            collisionAngle = dist;//targetPos - posB;
        }
        else
        {
            // collision, try shorter scaler
            currentScaler -= currentStep;
        }
        currentStep *= 0.5f;
    }
    timeOfCollision = nearestNonCollisionScaler;
    collisionNormal = collisionAngle.GetNormalized();
    return true;
}

bool PhysicsCollisionShapeCircle::GetCollisionAngle(PhysicsMovingBall* obj, const Vector2f& moveVec, f32& timeOfCollision, Vector2f& collisionNormal)
{
    return _GetCollisionAngleCircleCircle(obj->GetPosition(), obj->GetRadius(), moveVec, this->m_pos, this->m_radius, timeOfCollision, collisionNormal);
}

bool PhysicsMovingBall::GetCollisionAngle(PhysicsMovingBall* obj, PhysicsMovingBall* otherBall, const Vector2f& moveVec, f32& timeOfCollision, Vector2f& collisionNormal)
{
    return _GetCollisionAngleCircleCircle(obj->GetPosition(), obj->GetRadius(), moveVec, otherBall->GetPosition(), otherBall->GetRadius(), timeOfCollision, collisionNormal);
}

void PhysicsManager::Update(f32 timestep)
{
    // process all moving objects
    for(auto& it : m_movingBalls)
        ProcessBall(it, timestep);
}

void PhysicsManager::ProcessBall(PhysicsMovingBall* obj, f32 timestep)
{
    // handle movement with collision
    PhysicsMovingBall* collisionObject = nullptr;
    Vector2f moveVec = obj->m_velocity * timestep;
    bool hasCollision = false;
    for(u32 moveSteps = 0; moveSteps < 10; moveSteps++)
    {
        if(moveVec.LengthSquare() < 0.0000001)
            break;
        // find nearest collision
        f32 nearestTOC = 9999.0f;
        Vector2f nearestCollisionNormal;
        // check collision shapes
        for(auto& collisionShape : m_collisionShapes)
        {
            f32 dist;
            Vector2f collisionNormal;
            bool r = collisionShape->GetCollisionAngle(obj, moveVec, dist, collisionNormal);
            if (r)
            {
                if (dist < nearestTOC)
                {
                    nearestTOC = dist;
                    nearestCollisionNormal = collisionNormal;
                    collisionObject = nullptr;
                }
            }
        }
        // check other balls
        for(auto& ballIt : m_movingBalls)
        {
            if(ballIt == obj)
                continue;
            f32 dist;
            Vector2f collisionNormal;
            bool r = PhysicsMovingBall::GetCollisionAngle(obj, ballIt, moveVec, dist, collisionNormal);
            if (r)
            {
                if (dist < nearestTOC)
                {
                    nearestTOC = dist;
                    nearestCollisionNormal = collisionNormal;
                    collisionObject = ballIt;
                }
            }
        }
        // handle collision result
        if(nearestTOC <= 1.0001f)
        {
            // has collision
            hasCollision = true;
            // move ball up to point of collision
            obj->m_pos = obj->m_pos + moveVec * nearestTOC;
            // then calculate angle of deflection
            f32 velocityLength = obj->m_velocity.Length();
            Vector2f vel = obj->m_velocity.GetNeg().GetNormalized();
            Vector2f reflectionVec = nearestCollisionNormal * ((vel.Dot(nearestCollisionNormal)) * 2.0f) - vel;
            // when colliding with other balls, propagate the lost force to the other ball
            float forceDampFactor = 0.8f;
            // if the angle of deflection is really shallow we assume this is mostly a rolling motion
            // and we dont dampen the force
            // favor sliding motions
            //OSReport("DebugCollision | moveVec %f/%f | CollisionNormal %f/%f reflectionVec %f/%f\n", moveVec.x, moveVec.y, nearestCollisionNormal.x, nearestCollisionNormal.y, reflectionVec.x, reflectionVec.y);

            //OSReport("CollisionNormal %f/%f reflectionVec %f/%f\n", nearestCollisionNormal.x, nearestCollisionNormal.y, reflectionVec.x, reflectionVec.y);

            if(collisionObject)
            {
                // calculate angle of deflection from the perspective of the hit ball
                // Vector2f nInv = nearestCollisionNormal.GetNeg();
                // Vector2f velInv = vel.GetNormalized();
                // Vector2f hitReflectionVelocity = nInv * ((velInv.Dot(nInv)) * 2.0f) - velInv;
                // hitReflectionVelocity = hitReflectionVelocity * velocityLength;

                // we ran into issues with this so as a workaround, we just push away from the collider for now
                Vector2f pushAwayForce = nearestCollisionNormal.GetNormalized() * -velocityLength;
                forceDampFactor = 0.5f;
                //collisionObject->ApplyForce(hitReflectionVelocity * (1.0f - forceDampFactor));
                collisionObject->ApplyForce(pushAwayForce * (1.0f - forceDampFactor));
            }

            Vector2f reflectionVelocity = reflectionVec * velocityLength;
            obj->m_velocity = reflectionVelocity * forceDampFactor;
            // and set move vec to the remaining distance to be traveled for this update cycle
            moveVec = obj->m_velocity * (1.0 - nearestTOC);
            if(velocityLength > 50.0) // only consider it a reported collision if the force was strong enough
                obj->m_hasCollisionQueued = true;
            break;
        }
        else
        {
            // no collision, we can move the object freely
            obj->m_pos = obj->m_pos + moveVec;
        }
    }
    // apply gravity
    if(hasCollision)
        obj->m_velocity.y += timestep * 9.807f * 2.0;
    else
        obj->m_velocity.y += timestep * 9.807f * 10.0;
}

PhysicsManager gPhysicsMgr;