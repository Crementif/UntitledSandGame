#include "../common/common.h"
#include "GameSceneIngame.h"
#include "Object.h"
#include "Map.h"
#include "Player.h"

#include "../framework/navigation.h"
#include "../framework/physics/physics.h"

#include "GameServer.h"
#include "GameClient.h"

GameSceneIngame::GameSceneIngame(std::unique_ptr<GameClient> client, std::unique_ptr<GameServer> server): GameScene(std::move(client), std::move(server))
{
    u32 levelId = m_gameClient->GetGameSessionInfo().levelId;
    u32 rngSeed = m_gameClient->GetGameSessionInfo().rngSeed;

    char levelFilename[32];
    sprintf(levelFilename, "level%u.tga", levelId);
    m_map = new Map(levelFilename, rngSeed);
    SetCurrentMap(m_map);

    SpawnPlayers();
    m_prevCamPos = Render::GetCameraPosition();
}

GameSceneIngame::~GameSceneIngame()
{
}

void GameSceneIngame::SpawnPlayers()
{
    UnregisterAllPlayers();
    m_selfPlayer = nullptr;

    const std::vector<Vector2i> spawnpoints = m_map->GetPlayerSpawnpoints();
    if(spawnpoints.empty())
        CriticalErrorHandler("Level does not have any spawnpoints");

    OSReport("[GameSceneIngame::SpawnPlayers] Spawning %d players...\n", (int)this->m_gameClient->GetGameSessionInfo().playerIds.size());
    PlayerID ourPlayerId = this->m_gameClient->GetGameSessionInfo().ourPlayerId;
    // spawn players
    std::vector<Vector2i> availSpawnpoints = spawnpoints;
    m_selfPlayer = nullptr;
    OSReport("Our player id: %08x\n", ourPlayerId);

    for (PlayerID playerId : this->m_gameClient->GetGameSessionInfo().playerIds)
    {
        if (availSpawnpoints.empty())
        {
            OSReport("Ran out of spawnpoints. Reusing already assigned locations");
            availSpawnpoints = spawnpoints;
        }
        // get random spawn position according to player id
        size_t spawnIdx = m_map->GetRNGNumber() % availSpawnpoints.size();
        Vector2i playerSpawnPos = availSpawnpoints[spawnIdx];
        availSpawnpoints.erase(availSpawnpoints.begin() + spawnIdx);
        // spawn player
        OSReport("Spawning player %08x at %d/%d\n", playerId, playerSpawnPos.x, playerSpawnPos.y);
        Player* player = this->RegisterPlayer(playerId, (f32)playerSpawnPos.x, (f32)playerSpawnPos.y);
        // is it us?
        if (playerId == ourPlayerId)
            m_selfPlayer = player;
    }
    if (!m_selfPlayer)
        CriticalErrorHandler("Game started without self-player");
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
    Vector2f newCameraPosition = m_selfPlayer->GetPosition() * MAP_PIXEL_ZOOM;
    // center on player
    newCameraPosition = newCameraPosition - Vector2f(1920.0f, 1080.0f) * 0.5f;
    newCameraPosition = newCameraPosition * 0.75f + m_prevCamPos * 0.25f;
    m_prevCamPos = newCameraPosition;
    Render::SetCameraPosition(newCameraPosition);
}

void GameSceneIngame::UpdateMultiplayer()
{
    if (m_gameServer)
        m_gameServer->Update();
    if (!m_gameClient)
        return;
    m_gameClient->Update();
    // process received events
    std::vector<GameClient::EventMovement> eventMovement = m_gameClient->GetAndClearMovementEvents();
    const auto& players = this->GetPlayers();
    for(auto& event : eventMovement)
    {
        auto playerIt = players.find(event.playerId);
        playerIt->second->SyncMovement(event.pos, event.speed);
    }
    // player started jumping

    // send movement state
    u32 elapsedTicks = OSGetTick() - m_lastMovementBroadcast;
    if(elapsedTicks > OSMillisecondsToTicks(180))
    {
        // also sent this immediately when the player is starting a jump?
        Vector2f pos = m_selfPlayer->GetPosition();
        Vector2f speed = m_selfPlayer->GetSpeed();
        m_gameClient->SendMovement(pos, speed);
        m_lastMovementBroadcast = OSGetTick();
    }

}

void GameSceneIngame::Draw()
{
    UpdateMultiplayer();

    m_selfPlayer->HandleLocalPlayerControl();

    RunDeterministicSimulationStep();

    UpdateCamera();

    gPhysicsMgr.Update(1.0f / 60.0f);
    DoUpdates(1.0f / 60.0f);

    DrawBackground();

    m_map->Draw();

    DoDraws();
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

void GameSceneIngame::RunDeterministicSimulationStep()
{
    double startTime = GetMillisecondTimestamp();

    if ((m_map->GetRNGNumber()&0x7) < 3)
        m_map->SpawnMaterialPixel(MAP_PIXEL_TYPE::SAND, 200, 155);

    if ((m_map->GetRNGNumber()&0x7) < 3)
        m_map->SpawnMaterialPixel(MAP_PIXEL_TYPE::SAND, 700, 210);

    m_map->SimulateTick();
    m_map->Update(); // map objects are always independent of the world simulation?

    double dur = GetMillisecondTimestamp() - startTime;
    char buf[128];
    sprintf(buf, "Simulation time: %.4lfms", dur);
    g_debugStrings.push_back(buf);
}