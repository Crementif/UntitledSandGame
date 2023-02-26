#pragma once
#include "GameScene.h"
#include "TextButton.h"
#include "Map.h"
#include "../framework/render.h"
#include "../framework/physics/physics.h"
#include "../framework/multiplayer/multiplayer.h"

enum class MenuState {
    NORMAL,
    WAIT_FOR_INPUT,
    WAIT_FOR_CONNECTION,
    WAIT_FOR_GAME
};

class GameSceneMenu : public GameScene {
public:
    GameSceneMenu();
    ~GameSceneMenu() override;

    void HandleInput() override;
    void Draw() override;

private:
    void DrawBackground();
    void DrawButtons();

    Map* m_map;
    TextButton* m_sandbox_btn;
    TextButton* m_host_btn;
    TextButton* m_join_btn;

    FSClient* m_fsClient;

    MenuState m_state = MenuState::NORMAL;

    bool m_startSandboxImmediately{false};
    bool m_startPacketSent{false};
};