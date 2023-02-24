#include "../common/common.h"
#include "GameSceneMenu.h"
#include "Object.h"

#include "../framework/navigation.h"
#include "GameSceneIngame.h"

GameSceneMenu::GameSceneMenu() {
    m_sandbox_btn = new TextButton(AABB{1920.0f/2, 1080.0f/2+000, 500, 80}, "Sandbox");
    m_host_btn = new TextButton(AABB{1920.0f/2, 1080.0f/2+100, 500, 80}, "Host");
    m_join_btn = new TextButton(AABB{1920.0f/2, 1080.0f/2+200, 500, 80}, "Join");
    m_exit_btn = new TextButton(AABB{1920.0f/2, 1080.0f/2+300, 500, 80}, "Exit");

    m_fsClient = (FSClient*)MEMAllocFromDefaultHeap(sizeof(FSClient));
    FSAddClient(m_fsClient, FS_ERROR_FLAG_NONE);

    nn::swkbd::CreateArg createArg;
    createArg.regionType = nn::swkbd::RegionType::Europe;
    createArg.workMemory = MEMAllocFromDefaultHeap(nn::swkbd::GetWorkMemorySize(0));
    createArg.fsClient = m_fsClient;

    if (!nn::swkbd::Create(createArg))
        OSFatal("Error: couldn't create swkbd!");

    nn::swkbd::MuteAllSound(false);
}

GameSceneMenu::~GameSceneMenu() {
    delete m_sandbox_btn;
    delete m_host_btn;
    delete m_join_btn;
    delete m_exit_btn;

    delete m_fsClient;
}

void GameSceneMenu::DrawBackground() {
    // Render something
}

void GameSceneMenu::HandleInput() {
    bool isTouchValid = false;
    s32 screenX, screenY;
    vpadGetTouchInfo(isTouchValid, screenX, screenY);
    vpadUpdateSWKBD();

    if (nn::swkbd::IsNeedCalcSubThreadFont()) {
        nn::swkbd::CalcSubThreadFont();
    }

    // Client-specific states
    bool pressedOkButton = false;
    if (m_state == MenuState::WAIT_FOR_INPUT && nn::swkbd::IsDecideOkButton(&pressedOkButton)) {
        nn::swkbd::DisappearInputForm();
        m_state = MenuState::WAIT_FOR_CONNECTION;

        auto ipAddress = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>().to_bytes(nn::swkbd::GetInputFormString());
        mp_client = std::make_unique<RelayClient>();
        mp_client->ConnectTo(ipAddress);
    }
    bool pressedCancelButton = false;
    if (m_state == MenuState::WAIT_FOR_INPUT && nn::swkbd::IsDecideCancelButton(&pressedCancelButton)) {
        nn::swkbd::DisappearInputForm();
        m_state = MenuState::NORMAL;
    }

    // Server states
    if (m_state == MenuState::WAIT_FOR_CONNECTION && this->mp_server) {
        this->mp_server->GetConnectedPlayers();
    }
    if (m_state == MenuState::WAIT_FOR_GAME && this->mp_server && pressedStart()) {
        GameScene::ChangeTo(new GameSceneIngame(std::move(mp_client), std::move(mp_server)));
    }

    // Client and server states
    if (m_state == MenuState::WAIT_FOR_CONNECTION && this->mp_client->IsConnected()) {
        m_state = MenuState::WAIT_FOR_GAME;
    }


    if (isTouchValid && m_state == MenuState::NORMAL)
    {
        if (m_sandbox_btn->GetBoundingBox().Contains(Vector2f{(f32)screenX, (f32)screenY}))
            GameScene::ChangeTo(new GameSceneIngame());
        else if (m_host_btn->GetBoundingBox().Contains(Vector2f{(f32)screenX, (f32)screenY})) {
            this->m_state = MenuState::WAIT_FOR_CONNECTION;

            this->mp_server = std::make_unique<RelayServer>();
            this->mp_server->AcceptConnections();
            this->mp_client = std::make_unique<RelayClient>();
            this->mp_client->ConnectTo("127.0.0.1");
        }
        else if (m_join_btn->GetBoundingBox().Contains(Vector2f{(f32)screenX, (f32)screenY})) {
            this->m_state = MenuState::WAIT_FOR_INPUT;

            nn::swkbd::AppearArg appearArg = {};
            appearArg.keyboardArg.configArg.keyboardMode = nn::swkbd::KeyboardMode::Numpad;
            appearArg.keyboardArg.configArg.disableNewLine = true;
            appearArg.keyboardArg.configArg.okString = u"Connect";
            appearArg.keyboardArg.configArg.showWordSuggestions = false;
            appearArg.keyboardArg.configArg.languageType = nn::swkbd::LanguageType::English;
            appearArg.inputFormArg.hintText = u"Type IP address";
            if (!nn::swkbd::AppearInputForm(appearArg))
                OSFatal("nn::swkbd::AppearInputForm failed");
        }
        else if (m_exit_btn->GetBoundingBox().Contains(Vector2f{(f32)screenX, (f32)screenY}))
            GameScene::ChangeTo(nullptr);
    }
}

void GameSceneMenu::DrawButtons() {
    Object::DoDraws();
}

void GameSceneMenu::Draw() {
    this->DrawBackground();
    this->DrawButtons();
    if (this->m_state == MenuState::WAIT_FOR_GAME && this->mp_server) {
        u32 joinedPlayers = this->mp_server->GetConnectedPlayers();
        std::string joinedPlayersText = "Press START to start match with "+std::to_string(joinedPlayers)+" players...";
        const u32 stringWidth = joinedPlayersText.size()*16;
        Render::RenderText(1920-stringWidth-20, 1080-80, 0, 0x00, joinedPlayersText.c_str());
    }
    else if (this->m_state == MenuState::WAIT_FOR_GAME) {
        const u32 stringWidth = strlen("Waiting for game to start...")*16;
        Render::RenderText(1920-stringWidth-20, 1080-80, 0, 0x00, "Waiting for game to start...");
    }
    else if (this->m_state == MenuState::WAIT_FOR_CONNECTION) {
        const u32 stringWidth = strlen("Connecting to server...")*16;
        Render::RenderText(1920-stringWidth-20, 1080-80, 0, 0x00, "Connecting to server...");
    }
    nn::swkbd::DrawTV();
    nn::swkbd::DrawDRC();
}
