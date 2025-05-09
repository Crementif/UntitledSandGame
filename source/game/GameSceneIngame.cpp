#include "../common/common.h"
#include "GameSceneIngame.h"
#include "Object.h"
#include "Map.h"
#include "Player.h"
#include "../framework/audio/audio.h"

#include "GameServer.h"
#include "GameClient.h"
#include "Landmine.h"
#include "Missile.h"
#include "Collectable.h"
#include "MapPixels.h"
#include "GameSceneMenu.h"
#include "Blackhole.h"
#include "../framework/debug.h"


GameSceneIngame::GameSceneIngame(std::unique_ptr<GameClient> client, std::unique_ptr<GameServer> server): GameScene(std::move(client), std::move(server))
{
    u32 levelId = m_gameClient->GetGameSessionInfo().levelId;
    u32 rngSeed = m_gameClient->GetGameSessionInfo().rngSeed;

    char levelFilename[32];
    sprintf(levelFilename, "level%u.tga", levelId);
    this->RegisterMap(new Map(levelFilename, rngSeed));

    SpawnPlayers();
    SpawnCollectibles();
    m_prevCamPos = Render::GetCameraPosition();
    m_ingameBgm = new Audio("/bgm/ingame.ogg");
    m_ingameBgm->Play();
    m_ingameBgm->SetLooping(true);
    m_ingameBgm->SetVolume(100);

    m_ingameMuffledBgm = new Audio("/bgm/ingame_muffled.ogg");
    m_ingameMuffledBgm->Play();
    m_ingameMuffledBgm->SetLooping(true);
    m_ingameMuffledBgm->SetVolume(100);
    m_ingameBgm->SetOffset(m_ingameMuffledBgm->GetOffset());
}

GameSceneIngame::~GameSceneIngame()
{
    m_ingameBgm->Stop();
    m_ingameBgm->QueueDestroy();
    m_ingameMuffledBgm->Stop();
    m_ingameMuffledBgm->QueueDestroy();
}

void GameSceneIngame::SpawnPlayers()
{
    UnregisterAllPlayers();
    m_selfPlayer = nullptr;

    const std::vector<Vector2i> spawnpoints = this->GetMap()->GetPlayerSpawnpoints();
    if (spawnpoints.empty())
        CriticalErrorHandler("Level does not have any spawnpoints");

    OSReport("[GameSceneIngame] GameSceneIngame::SpawnPlayers: Spawning %d players...\n", (int)this->m_gameClient->GetGameSessionInfo().playerIds.size());
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
        size_t spawnIdx = this->GetMap()->GetRNGNumber() % availSpawnpoints.size();
        Vector2i playerSpawnPos = availSpawnpoints[spawnIdx];
        availSpawnpoints.erase(availSpawnpoints.begin() + spawnIdx);
        // spawn player
        OSReport("Spawning player %08x at %d/%d\n", playerId, playerSpawnPos.x, playerSpawnPos.y);
        Player* player = this->RegisterPlayer(playerId, (f32)playerSpawnPos.x, (f32)playerSpawnPos.y);
        if (playerId == ourPlayerId)
        {
            player->SetSelfFlag(true);
            m_selfPlayer = player;
        }
    }
    if (!m_selfPlayer)
        CriticalErrorHandler("Game started without self-player");
}

void GameSceneIngame::SpawnCollectibles() {
    const std::vector<Vector2i> collectiblesPoints = this->GetMap()->GetCollectablePoints();
    // spawn collectibles
    for (Vector2i spawnpoint : collectiblesPoints)
    {
        Collectable* collectable = new Collectable(this, {(f32)spawnpoint.x, (f32)spawnpoint.y});
        m_collectibles.emplace_back(collectable);
    }
}

void GameSceneIngame::DrawHUD() {
    static u32 heartsAnimationTicks = 0;
    heartsAnimationTicks++;
    if (heartsAnimationTicks >= 15)
        heartsAnimationTicks = 0;
    for (u32 i=0; i<m_selfPlayer->GetPlayerHealth(); i++) {
        Render::RenderSpritePortionScreenRelative(&m_heartSprite, 20+(i*(m_heartSprite.GetWidth()+20)), 20, 0, 64*(heartsAnimationTicks/5), 64, 64);
    }

    Render::RenderSpriteScreenRelative(&m_itemBackdropSprite, 20, 20+64+20, 64, 64);
    if (m_selfPlayer->GetAbility() == GameClient::GAME_ABILITY::LANDMINE) {
        Render::RenderSpritePortionScreenRelative(&m_itemLandmineSprite, 20+14, 20+64+20+(64/2)-10, 36, 0, 36, 28);
    }
    else if (m_selfPlayer->GetAbility() == GameClient::GAME_ABILITY::TURBO_DRILL) {
        Render::RenderSpriteScreenRelative(&m_itemTurboDrillSprite, 20+16, 20+64+20+(64/2)-9, 32, 32);
    }
    else if (m_selfPlayer->GetAbility() == GameClient::GAME_ABILITY::MISSILE) {
        Render::RenderSpriteScreenRelative(&m_missileSprite, 20+16, 20+64+20+(64/2)-9, 32, 32);
    }
    else if (m_selfPlayer->GetAbility() == GameClient::GAME_ABILITY::BLACKHOLE) {
        Render::RenderSpriteScreenRelative(&m_blackholeSprite, 20+16, 20+64+20+(64/2)-9, 32, 32);
    }

    if (m_selfPlayer->IsSpectating()) {
        std::string spectatingText = "Press A to spectate next player";
        const u32 stringWidth = spectatingText.size() * 16;
        Render::RenderText(1920 - stringWidth - 20, 1080 - (80 * 2), 0x00, spectatingText.c_str());
    }
}

void GameSceneIngame::UpdateCamera()
{
    const f32 SCREEN_WIDTH = 1920.0f;
    const f32 SCREEN_HEIGHT = 1080.0f;

    Vector2f newCameraPosition = m_selfPlayer->GetPosition() * MAP_PIXEL_ZOOM;
    // center on player
    newCameraPosition = newCameraPosition - Vector2f(SCREEN_WIDTH, SCREEN_HEIGHT) * 0.5f;
    newCameraPosition = newCameraPosition * 0.75f + m_prevCamPos * 0.25f;
    newCameraPosition.y -= 130.0f;
    // clamp to map
    newCameraPosition.x = std::max(newCameraPosition.x, 0.0f);
    newCameraPosition.y = std::max(newCameraPosition.y, 0.0f);
    newCameraPosition.x = std::min(newCameraPosition.x, (f32)GetMap()->GetPixelWidth() * MAP_PIXEL_ZOOM - SCREEN_WIDTH);
    newCameraPosition.y = std::min(newCameraPosition.y, (f32)GetMap()->GetPixelHeight() * MAP_PIXEL_ZOOM - SCREEN_HEIGHT);

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
        playerIt->second->SyncMovement(event.pos, event.speed, event.moveFlags, event.drillAngle);
    }
    auto eventAbility = m_gameClient->GetAndClearAbilityEvents();
    for (auto& event : eventAbility)
    {
        if (event.ability == GameClient::GAME_ABILITY::LANDMINE)
            new Landmine(this, event.playerId, event.pos.x, event.pos.y);
        else if (event.ability == GameClient::GAME_ABILITY::MISSILE)
            new Missile(this, event.playerId, event.pos.x, event.pos.y, event.velocity.x, event.velocity.y);
        else if (event.ability == GameClient::GAME_ABILITY::BLACKHOLE)
            new Blackhole(this, event.playerId, event.pos.x, event.pos.y, event.velocity.x, event.velocity.y);
        else if (event.ability == GameClient::GAME_ABILITY::DEATH) {
            this->GetPlayerById(event.playerId)->ChangeToSpectator();

            u32 alivePlayers = 0;
            Player* winner = nullptr;
            for (auto& player : players) {
                if (player.second->GetPlayerHealth() > 0) {
                    winner = player.second;
                    alivePlayers++;
                }
            }

            if (alivePlayers > 1) {
                float distance = (GetPlayer()->GetPosition().Distance(event.pos)+0.00000001f)/20.0f;
                float volume = 20.0f - (distance/100.0f*20.0f);

                Audio* explosionAudio = new Audio("/sfx/explosion.ogg");
                explosionAudio->Play();
                explosionAudio->SetVolume((u32)volume);
                explosionAudio->QueueDestroy();

                new ExplosiveParticle(this, Vector2f(event.pos.x-11.0f, event.pos.y-11.0f), 8, 1.6f, 2, 20.0f, 20.0f);
            }
            else if (winner == nullptr) {
                GameScene::ChangeTo(new GameSceneMenu(MenuScoreboard::DIED));
                return;
            }
            else {
                OSReport("Game ended, winner: %08x\n", winner->GetPlayerId());
                GameScene::ChangeTo(new GameSceneMenu(winner->GetPlayerId() == m_selfPlayer->GetPlayerId() ? MenuScoreboard::WON : MenuScoreboard::LOST));
                return;
            }
        }
    }
    auto eventPicking = m_gameClient->GetAndClearPickingEvents();
    for (auto& event : eventPicking)
    {
        Player* player = this->GetPlayerById(event.playerId);
        if (player == nullptr) {
            OSReport("Received picking event for unknown player %08x\n", event.playerId);
            continue;
        }

        float closestDistance = std::numeric_limits<float>::max();
        Collectable* closestCollectible = nullptr;
        for (auto& collectible : this->m_collectibles)
        {
            float distanceToCollectible = collectible->GetPosition().DistanceSquare(player->GetPosition());
            if (distanceToCollectible < closestDistance) {
                closestDistance = collectible->GetPosition().DistanceSquare(player->GetPosition());
                closestCollectible = collectible;
            }
        }
        if (closestCollectible != nullptr) {
            closestCollectible->Pickup(player);
        }
    }

    // send movement state
    u32 elapsedTicks = OSGetTick() - m_lastMovementBroadcast;
    if(elapsedTicks > OSMillisecondsToTicks(170))
    {
        // also sent this immediately when the player is starting a jump?
        Vector2f pos = m_selfPlayer->GetPosition();
        Vector2f speed = m_selfPlayer->GetSpeed();
        m_gameClient->SendMovement(pos, speed, m_selfPlayer->GetMovementFlags(), m_selfPlayer->GetDrillAngle());
        if(m_selfPlayer->IsDrilling())
            m_gameClient->SendDrillingAction(pos + Vector2f(0.0f, -13.0f));
        m_lastMovementBroadcast = OSGetTick();
    }
}

constexpr s32 COLLISION_RADIUS = 14;
constexpr s32 BGM_COLLISION_RADIUS = 18;
void GameSceneIngame::HandlePlayerCollisions() {
    // check for collisions with other objects
    for (auto& collectible : m_collectibles) {
        if (collectible->m_hidden)
            continue;
        if (m_selfPlayer->GetBoundingBox().Intersects(collectible->GetBoundingBox())) {
            this->m_gameClient->SendPickAction(m_selfPlayer->GetPosition());
        }
    }

    // check for collisions with lava
    Vector2f pos = m_selfPlayer->GetPosition();
    s32 posX = (s32)(pos.x + 0.5f);
    s32 posY = (s32)(pos.y + 0.5f) - m_selfPlayer->GetBoundingBox().scale.y/2;

    s32 musicHits = 0;
    s32 musicMisses = 0;
    for (s32 y=posY-BGM_COLLISION_RADIUS; y<=posY+BGM_COLLISION_RADIUS; y++) {
        const s32 dfy = y - posY;
        const s32 dfySq = dfy * dfy;
        for (s32 x=posX-BGM_COLLISION_RADIUS; x<=posX+BGM_COLLISION_RADIUS; x++) {
            s32 dfx = x - posX;
            s32 squareDist = dfx * dfx + dfySq;

            if (squareDist <= COLLISION_RADIUS * COLLISION_RADIUS) {
                // for dynamic BGM we only check the border pixels
                if (this->GetMap()->IsPixelOOB(x, y) )
                    continue;
                PixelType& pt = this->GetMap()->GetPixel(x, y);
                if (pt.GetPixelType() == MAP_PIXEL_TYPE::LAVA) {
                    m_selfPlayer->TakeDamage();
                    break;
                }
            }
            else if (squareDist >= BGM_COLLISION_RADIUS * BGM_COLLISION_RADIUS && squareDist <= (BGM_COLLISION_RADIUS+1) * (BGM_COLLISION_RADIUS+1)) {
                // for dynamic BGM we only check the border pixels
                if (this->GetMap()->IsPixelOOB(x, y))
                    continue;
                PixelType& pt = this->GetMap()->GetPixel(x, y);
                if (pt.IsFilled()) {
                    musicHits++;
                }
                else {
                    musicMisses++;
                }
            }
        }
    }


    // if the ratio of solid pixels to empty pixels is above a certain threshold, fade the dampened version in
    const float ratio = musicHits / (float)(musicHits + musicMisses);
    DebugLog::Printf("Music hits: %d, misses: %d, ratio: %f", musicHits, musicMisses, ratio);
    constexpr float DAMP_FADE_START = 0.45f;
    constexpr float DAMP_FADE_FINISH = 0.65f;
    if (ratio > DAMP_FADE_START) {
        if (ratio > DAMP_FADE_FINISH) {
            m_ingameBgm->SetVolume(0);
            m_ingameMuffledBgm->SetVolume(100);
        }
        else {
            m_ingameBgm->SetVolume((u32)(100.0f - (ratio-DAMP_FADE_START) / (DAMP_FADE_FINISH-DAMP_FADE_START) * 100.0f));
            m_ingameMuffledBgm->SetVolume((u32)((ratio-DAMP_FADE_START) / (DAMP_FADE_FINISH-DAMP_FADE_START) * 100.0f));
        }
    }
    else {
        m_ingameBgm->SetVolume(100);
        m_ingameMuffledBgm->SetVolume(0);
    }
}

void GameSceneIngame::HandleInput()
{
    UpdateCamera();

    m_selfPlayer->HandleLocalPlayerControl();
}

void GameSceneIngame::Update()
{
    UpdateMultiplayer();

    if (!m_selfPlayer->IsSpectating())
        HandlePlayerCollisions();

    DebugProfile::Start("[CPU] Scene -> Simulation");
    RunDeterministicSimulationStep();
    DebugProfile::End("[CPU] Scene -> Simulation");

    DoObjectUpdates(1.0f / 60.0f);
}

void GameSceneIngame::Draw()
{
    this->GetMap()->Draw();
    Render::SetStateForSpriteRendering(); // restore sprite drawing state

    DoObjectDraws();
    DebugWaitAndMeasureGPUDone("[GPU] Scene::DoDraws");
    DrawHUD();
    DebugWaitAndMeasureGPUDone("[GPU] Scene::DrawHUD");
    DebugLog::Draw();
    DebugWaitAndMeasureGPUDone("[GPU] DebugLog::Draw");
    DebugOverlay::Draw();
    DebugWaitAndMeasureGPUDone("[GPU] DebugOverlay::Draw");

    m_gameTime += 1.0/60.0;
}

// return bounds of the viewable world in pixel coordinates
AABB GameSceneIngame::GetWorldBounds()
{
    return AABB(0, 0, 256*8, 256*5);
}

void GameSceneIngame::RunDeterministicSimulationStep()
{
    if ((this->GetMap()->GetRNGNumber()&0x7) < 3)
        this->GetMap()->SpawnMaterialPixel(MAP_PIXEL_TYPE::SAND, _GetRandomSeedFromPixelType(MAP_PIXEL_TYPE::SAND), 200, 155);

    if ((this->GetMap()->GetRNGNumber()&0x7) < 3)
        this->GetMap()->SpawnMaterialPixel(MAP_PIXEL_TYPE::SAND, _GetRandomSeedFromPixelType(MAP_PIXEL_TYPE::SAND), 700, 210);

    if ((this->GetMap()->GetRNGNumber()&0x7) < 5)
        this->GetMap()->SpawnMaterialPixel(MAP_PIXEL_TYPE::LAVA, _GetRandomSeedFromPixelType(MAP_PIXEL_TYPE::LAVA), 480, 160);

    this->GetMap()->SimulateTick();
    this->GetMap()->Update(); // map objects are always independent of the world simulation?
}