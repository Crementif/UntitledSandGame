#include "PachinkoBall.h"
#include "../framework/render.h"
#include "../framework/physics/physics.h"


Sprite* sSpritePachinkoBall_red = nullptr;
Sprite* sSpritePachinkoBall_blue = nullptr;

std::vector<PachinkoBall*> sPachinkoBalls;

constexpr f32 PACHINKO_BALL_RADIUS = 32.0;

PachinkoBall::PachinkoBall(f32 posX, f32 posY, BALL_TYPE type) : Object(CalcAABB(), true, DRAW_LAYER_1), m_pos(posX, posY), m_type(type)
{
    if (!sSpritePachinkoBall_red)
    {
        sSpritePachinkoBall_red = new Sprite("tex/ball.tga", true);
        sSpritePachinkoBall_blue = new Sprite("tex/ball_blue.tga", true);
    }
    this->ballHitSound = new Audio("sfx/metal-ball-hit.wav");

    if(type == BALL_TYPE::BLUE)
        m_radius = 24.0f;
    else
        m_radius = 32.0f;

    m_physicsObject = new PhysicsMovingBall(&gPhysicsMgr, {posX, posY}, m_radius);
    sPachinkoBalls.emplace_back(this);
}

PachinkoBall::~PachinkoBall()
{
    sPachinkoBalls.erase(std::find(sPachinkoBalls.begin(), sPachinkoBalls.end(), this));
    delete m_physicsObject;
}

AABB PachinkoBall::CalcAABB()
{
    return AABB(m_pos.x - m_radius, m_pos.y - m_radius, m_radius*2.0, m_radius*2.0);
}

void PachinkoBall::Draw(u32 layerIndex)
{
    Sprite* sprite = sSpritePachinkoBall_red;
    if(m_type == BALL_TYPE::BLUE)
        sprite = sSpritePachinkoBall_blue;
    // todo - motion blur trail maybe?
    Render::RenderSprite(sprite, m_pos.x - m_radius, m_pos.y - m_radius);
}

void PachinkoBall::Update(float timestep)
{
    m_pos = m_physicsObject->GetPosition();
    if(m_physicsObject->HadCollisionQueued())
    {
        if(this->ballHitSound->GetState() != Audio::StateEnum::PLAYING)
            this->ballHitSound->Play();
    }
}

Vector2f PachinkoBall::GetPosition()
{
    return {m_pos.x - m_radius, m_pos.y - m_radius};
}

void PachinkoBall::ApplyForce(const Vector2f& force)
{
    m_physicsObject->ApplyForce(force);
}

std::span<PachinkoBall*> PachinkoBall::GetAllBalls()
{
    return {sPachinkoBalls.data(), sPachinkoBalls.size()};
};