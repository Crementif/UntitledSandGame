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

extern std::vector<std::string> g_debugStrings;

class GameSceneIngame : public GameScene
{
public:
    explicit GameSceneIngame(class GameClient* client = nullptr, class GameServer* server = nullptr);
    ~GameSceneIngame() override;

    void Draw() override;
    void HandleInput() override;

private:
    void DrawBackground();
    void DrawHUD();

    void UpdateCamera();

    AABB GetWorldBounds();

    Sprite m_heartSprite{"/tex/heart.tga", true};

    Sprite m_bgSpriteA{"/tex/background_tile_a.tga"};

    class Map* m_map;

    // player
    class Player* m_selfPlayer;
    Vector2f m_prevCamPos;

    // multiplayer
    class GameServer* m_server{nullptr};
    class GameClient* m_client{nullptr};

    // touch scrolling
    bool m_isScrolling{false};
    Vector2f m_startCameraPosition;

    // misc
    f32 m_gameTime{};
};