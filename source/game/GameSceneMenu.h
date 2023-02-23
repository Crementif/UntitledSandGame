#pragma once
#include "GameScene.h"
#include "../framework/render.h"
#include "../framework/physics/physics.h"
#include "TextButton.h"

enum class MenuState {
    NORMAL,
    WAIT_FOR_CLIENTS,
    WAIT_FOR_SERVER,
    WAIT_FOR_SERVER_CONNECTING
};

class GameSceneMenu : public GameScene {
public:
    GameSceneMenu();
    ~GameSceneMenu() override;

    void Draw() override;
    void HandleInput() override;

private:
    void DrawBackground();
    void DrawButtons();

    TextButton* m_sandbox_btn;
    TextButton* m_host_btn;
    TextButton* m_join_btn;
    TextButton* m_exit_btn;

    FSClient* m_fsClient;

    MenuState m_state = MenuState::NORMAL;
};