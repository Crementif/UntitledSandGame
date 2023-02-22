#include "Splitter.h"
#include "PachinkoBall.h"
#include "../framework/render.h"
#include "../framework/physics/physics.h"

Sprite* sSpriteSplitter_bg = nullptr;

static const std::array<AABB, 3> sSplitterCollisionShapes
        {AABB{0.0f, 0.0f, 13.0f, 128.0f},
         AABB{128.0f - 13.0, 0.0f, 27.0f, 128.0f},
        AABB{0.0f, 128.0f, 128.0f, 10.0f}};

Splitter::Splitter(f32 posX, f32 posY) : Object(CalcAABB({posX, posY}), true, DRAW_LAYER_0 | DRAW_LAYER_2)
{
    if (!sSpriteSplitter_bg)
    {
        sSpriteSplitter_bg = new Sprite("tex/splitter_0.tga", true);
    }
    Vector2f pos = m_aabb.pos;
    for(size_t i=0; i<sSplitterCollisionShapes.size(); i++)
        m_collisionShapes.emplace_back(new PhysicsCollisionShapeRect(&gPhysicsMgr, sSplitterCollisionShapes[i] + pos));
}

Splitter::~Splitter()
{
    for(auto& it : m_collisionShapes)
        delete it;
    m_collisionShapes.clear();
}

AABB Splitter::CalcAABB(Vector2f pos)
{
    return AABB(pos.x - 64.0, pos.y - 64.0, 128.0, 128.0);
}

void Splitter::Draw(u32 layerIndex)
{
    if(layerIndex == 0)
        ;
    else if(layerIndex == 2)
        Render::RenderSprite(sSpriteSplitter_bg, m_aabb.pos.x, m_aabb.pos.y);
}

Vector2f Splitter::GetPosition()
{
    return m_aabb.GetCenter();
}

void Splitter::UpdatePosition(const Vector2f& newPos)
{
    m_aabb = CalcAABB(newPos);
    for(size_t i=0; i<sSplitterCollisionShapes.size(); i++)
        m_collisionShapes[i]->UpdateAABB(sSplitterCollisionShapes[i] + m_aabb.pos);
}

void Splitter::Update(float timestep)
{
    auto balls = PachinkoBall::GetAllBalls();
    AABB area = m_aabb;
    area.pos.y += 15.0f;
    area.scale.x -= 20.0f;
    for(auto& it : balls)
    {
        if(area.Contains(it->GetPosition()))
        {
            Object::QueueForDeletion(it);

            Vector2f pos = m_aabb.GetCenter();
            PachinkoBall* bLeft = new PachinkoBall(pos.x - 90.0f, pos.y, PachinkoBall::BALL_TYPE::BLUE);
            PachinkoBall* bRight = new PachinkoBall(pos.x + 90.0f, pos.y, PachinkoBall::BALL_TYPE::BLUE);

            bLeft->ApplyForce({-50.0f, 0.0f});
            bRight->ApplyForce({50.0f, 0.0f});
        }
    }
}

