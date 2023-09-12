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
    constexpr static u32 s_buttonSize = 42;
    constexpr static u32 s_buttonColor = 0x3b3833FF;

    class Audio* m_selectAudio;
    class Audio* m_startAudio;

    FSClient* m_fsClient;

    MenuState m_state = MenuState::NORMAL;
    uint32_t m_prevPlayerCount = 1;
    Sprite* m_startGameWithPlayersSprite = Render::RenderTextSprite(32, 0xFFFFFFFF, L"Press START button to start match with 1 player...");
    Sprite* m_waitingForGameStartSprite = Render::RenderTextSprite(32, 0xFFFFFFFF, L"Waiting for game to start...");
    Sprite* m_connectingToServerSprite = Render::RenderTextSprite(32, 0xFFFFFFFF, L"Connecting to server...");
    MenuScoreboard m_scoreboard = MenuScoreboard::NORMAL;
    Sprite* m_wonScoreboardSprite = Render::RenderTextSprite(32, 0x00FF00FF, L"YOU WON!");
    Sprite* m_lostScoreboardSprite = Render::RenderTextSprite(32, 0xFF0000FF, L"YOU LOST!");
    Sprite* m_diedScoreboardSprite = Render::RenderTextSprite(32, 0xFF0000FF, L"YOU DIED!");
    uint32_t m_selectedButton = 0;
    OSTime m_lastInput = 0;

    bool m_startSandboxImmediately{false};
    bool m_startPacketSent{false};
};