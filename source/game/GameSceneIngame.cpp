#include "../common/common.h"
#include "GameSceneIngame.h"
#include "Object.h"
#include "Map.h"
#include "Player.h"

#include "../framework/navigation.h"
#include "../framework/physics/physics.h"
#include "Particle.h"

#include "GameServer.h"
#include "GameClient.h"

GameSceneIngame::GameSceneIngame(class GameClient* client, class GameServer* server) : m_server(server), m_client(client)
{
    m_map = new Map("level0.tga");
    SetCurrentMap(m_map);

    const std::vector<Vector2i> spawnpoints = m_map->GetPlayerSpawnpoints();
    if(spawnpoints.empty())
        CriticalErrorHandler("Level does not have any spawnpoints");

    Vector2i spawnpos = spawnpoints[rand()%spawnpoints.size()]; // non-deterministic for now

    m_selfPlayer = new Player((f32)spawnpos.x, (f32)spawnpos.y);
    m_prevCamPos = Render::GetCameraPosition();

    new Particle(std::make_unique<Sprite>("/tex/ball.tga"), Vector2f(spawnpos), 8, 2.0f, 40, 0.0f);
}

GameSceneIngame::~GameSceneIngame()
{
}

std::vector<std::string> g_debugStrings;

void GameSceneIngame::DrawHUD() {
    static u32 heartsAnimationTicks = 0;
    heartsAnimationTicks++;
    if (heartsAnimationTicks >= 15)
        heartsAnimationTicks = 0;
    for (u32 i=0; i<m_selfPlayer->GetPlayerHealth(); i++) {
        Render::RenderSpritePortionScreenRelative(&m_heartSprite, 20+(i*(m_heartSprite.GetWidth()+20)), 20, 0, 64*(heartsAnimationTicks/5), 64, 64);
    }

    for (u32 i=0; i<g_debugStrings.size(); i++) {
        Render::RenderText(20, 100+(i*16), 0, 0x00, g_debugStrings[i].c_str());
    }
}

void GameSceneIngame::DrawBackground()
{
    Vector2f camPos = Render::GetCameraPosition();
    s32 camX = (s32)camPos.x;
    s32 camY = (s32)camPos.y;

    // todo - we'll have a bounded world with unlockable regions? Show different background tiles based on the area

    /*
    s32 bgTileOffsetX = (camX-255) / 256;
    s32 bgTileOffsetY = (camY-255) / 256;

    for(s32 y=0; y<5; y++)
    {
        for(s32 x=0; x<10; x++)
        {
            Render::RenderSprite(&m_bgSpriteA, (bgTileOffsetX + x) * 256, (bgTileOffsetY + y) * 256);
        }
    }*/
}

void GameSceneIngame::UpdateCamera()
{
    Vector2f newCameraPosition = m_selfPlayer->GetPosition();
    m_prevCamPos = newCameraPosition;
    Render::SetCameraPosition(newCameraPosition);
}

void GameSceneIngame::Draw()
{
    if(m_server)
        m_server->Update();

    m_map->SimulateTick();

    UpdateCamera();

    gPhysicsMgr.Update(1.0f / 60.0f);
    Object::DoUpdates(1.0f / 60.0f);

    DrawBackground();

    m_map->Update();
    m_map->Draw();

    Object::DoDraws();
    DrawHUD();

    m_gameTime += 1.0/60.0;
}

// return bounds of the viewable world in pixel coordinates
AABB GameSceneIngame::GetWorldBounds()
{
    return AABB(0, 0, 256*8, 256*5);
}

void GameSceneIngame::HandleInput()
{
}