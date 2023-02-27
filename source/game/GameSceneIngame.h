#pragma once
#include "GameScene.h"
#include "../framework/render.h"
#include "../framework/physics/physics.h"
#include "../framework/multiplayer/multiplayer.h"
#include "NetCommon.h"

#include <unordered_map>

enum BUILDTYPE
{
    // matching the hotbar order
    BUILDTYPE_BEAM_HORIZONTAL_SHORT = 0,
    BUILDTYPE_BEAM_VERTICAL_LONG = 1,
    BUILDTYPE_SPLITTER = 2,
    BUILDTYPE_SPEED_BOOST = 3
};

extern std::vector<std::string> g_debugStrings;

class GameSceneIngame : public GameScene
{
public:
    explicit GameSceneIngame(std::unique_ptr<GameClient> client, std::unique_ptr<GameServer> server);
    ~GameSceneIngame() override;

    void Draw() override;
    void HandleInput() override;

private:
    void RunDeterministicSimulationStep();

    void SpawnPlayers();

    void DrawBackground();
    void DrawHUD();

    void UpdateCamera();
    void UpdateMultiplayer();

    AABB GetWorldBounds();

    Sprite m_heartSprite{"/tex/heart.tga", true};

    // player
    class Player* m_selfPlayer{};
    Vector2f m_prevCamPos;

    // multiplayer
    u32 m_lastMovementBroadcast{};

    // touch scrolling
    bool m_isScrolling{false};
    Vector2f m_startCameraPosition;

    // misc
    f32 m_gameTime{};
};