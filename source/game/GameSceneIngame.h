#pragma once
#include "GameScene.h"
#include "../framework/render.h"
#include "../framework/physics/physics.h"
#include "../framework/multiplayer/multiplayer.h"
#include "NetCommon.h"

class GameSceneIngame : public GameScene
{
public:
    explicit GameSceneIngame(std::unique_ptr<GameClient> client, std::unique_ptr<GameServer> server);
    ~GameSceneIngame() override;

    void Draw() override;
    void HandleInput() override;

private:
    void RunDeterministicSimulationStep();
    void HandlePlayerCollisions();

    void SpawnCollectibles();
    void SpawnPlayers();

    void DrawBackground();
    void DrawHUD();

    void UpdateCamera();
    void UpdateMultiplayer();

    AABB GetWorldBounds();

    Sprite m_heartSprite{"/tex/heart.tga", true};
    Sprite m_itemBackdropSprite{"/tex/item_background.tga", true};
    Sprite m_itemLandmineSprite{"/tex/landmine.tga", true};
    Sprite m_itemTurboDrillSprite{"/tex/drill_boost.tga", true};
    Sprite m_missileSprite{"/tex/missile0.tga", true};
    Sprite m_blackholeSprite{"/tex/blackhole0.tga", true};

    // player
    class Player* m_selfPlayer{};
    Vector2f m_prevCamPos;
    std::vector<class Collectable*> m_collectibles;

    // multiplayer
    u32 m_lastMovementBroadcast{};

    // misc
    f32 m_gameTime{};
};