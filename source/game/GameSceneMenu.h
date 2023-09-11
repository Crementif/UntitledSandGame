#pragma once
#include "GameScene.h"
#include "TextButton.h"
#include "../framework/render.h"
#include "../framework/physics/physics.h"
#include "../framework/multiplayer/multiplayer.h"

enum class MenuState {
    NORMAL,
    WAIT_FOR_INPUT,
    WAIT_FOR_CONNECTION,
    WAIT_FOR_GAME
};

enum class MenuScoreboard {
    NORMAL,
    WON,
    LOST,
    DIED
};

class GameSceneMenu : public GameScene {
public:
    GameSceneMenu(MenuScoreboard scoreboard = MenuScoreboard::NORMAL);
    ~GameSceneMenu() override;

    void HandleInput() override;
    void Draw() override;

private:
    void SimulateAndDrawLevel();
    void DrawButtons();

    TextButton* m_sandbox_btn;
    TextButton* m_host_btn;
    TextButton* m_join_btn;
    TextButton* m_crt_btn;

    class Audio* m_selectAudio;
    class Audio* m_startAudio;

    FSClient* m_fsClient;

    MenuState m_state = MenuState::NORMAL;
    MenuScoreboard m_scoreboard = MenuScoreboard::NORMAL;
    uint32_t m_selectedButton = 0;
    OSTime m_lastInput = 0;

    bool m_startSandboxImmediately{false};
    bool m_startPacketSent{false};
};