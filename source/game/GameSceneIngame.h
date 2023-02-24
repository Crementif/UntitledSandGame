#pragma once
#include "GameScene.h"
#include "../framework/render.h"
#include "../framework/physics/physics.h"
#include "../framework/multiplayer/multiplayer.h"

enum BUILDTYPE
{
    // matching the hotbar order
    BUILDTYPE_BEAM_HORIZONTAL_SHORT = 0,
    BUILDTYPE_BEAM_VERTICAL_LONG = 1,
    BUILDTYPE_SPLITTER = 2,
    BUILDTYPE_SPEED_BOOST = 3
};

class GameSceneIngame : public GameScene
{
public:
    explicit GameSceneIngame(std::unique_ptr<RelayClient> mp_client = nullptr, std::unique_ptr<RelayServer> mp_server = nullptr);
    ~GameSceneIngame() override;

    void Draw() override;
    void HandleInput() override;

private:
    void DrawBackground();
    void DrawHUD();

    void UpdateCamera();

    AABB GetWorldBounds();

    Sprite m_testSprite{"/tex/cross.tga", true};

    Sprite m_bgSpriteA{"/tex/background_tile_a.tga"};

    class Map* m_map;

    // player
    class Player* m_selfPlayer;
    Vector2f m_prevCamPos;

    // multiplayer
    std::unique_ptr<RelayServer> m_server;
    std::unique_ptr<RelayClient> m_client;

    // touch scrolling
    bool m_isScrolling{false};
    Vector2f m_startCameraPosition;

    // misc
    f32 m_gameTime{};

    // money
    uint32_t grayCurrency = 1000;
    uint32_t blueCurrency = 14;
    uint32_t greenCurrency = 0;
    uint32_t redCurrency = 256;
};