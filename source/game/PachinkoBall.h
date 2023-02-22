#pragma once
#include "Object.h"
#include "../framework/audio.h"

class PachinkoBall : public Object
{
public:
    enum class BALL_TYPE
    {
        RED, // basic red ball
        BLUE, // blue ball, created by splitting red balls
    };

    PachinkoBall(f32 posX, f32 posY, BALL_TYPE type);
    ~PachinkoBall();

    BALL_TYPE GetType() { return m_type; }

    void Draw(u32 layerIndex) override;
    void Update(float timestep) override;

    Vector2f GetPosition() override;

    void ApplyForce(const Vector2f& force);

    static std::span<PachinkoBall*> GetAllBalls();
private:
    Vector2f m_pos;
    f32 m_radius;
    class PhysicsMovingBall* m_physicsObject;
    AABB CalcAABB();
    BALL_TYPE m_type;

    Audio* ballHitSound;
};