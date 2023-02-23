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

    bool pressedOkButton = false;
    if (m_state == MenuState::WAIT_FOR_SERVER && nn::swkbd::IsDecideOkButton(&pressedOkButton)) {
        nn::swkbd::DisappearInputForm();
        m_state = MenuState::WAIT_FOR_SERVER_CONNECTING;

        // todo: do something with this ip address
        const char16_t *str = nn::swkbd::GetInputFormString();
    }
    bool pressedCancelButton = false;
    if (m_state == MenuState::WAIT_FOR_SERVER && nn::swkbd::IsDecideCancelButton(&pressedCancelButton)) {
        nn::swkbd::DisappearInputForm();
        m_state = MenuState::NORMAL;
    }

    if (isTouchValid && m_state == MenuState::NORMAL)
    {
        if (m_sandbox_btn->GetBoundingBox().Contains(Vector2f{(f32)screenX, (f32)screenY}))
            GameScene::ChangeTo(new GameSceneIngame());
        else if (m_host_btn->GetBoundingBox().Contains(Vector2f{(f32)screenX, (f32)screenY}))
            this->m_state = MenuState::WAIT_FOR_CLIENTS;
        else if (m_join_btn->GetBoundingBox().Contains(Vector2f{(f32)screenX, (f32)screenY})) {
            this->m_state = MenuState::WAIT_FOR_SERVER;

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
    if (this->m_state == MenuState::WAIT_FOR_CLIENTS) {
        const u32 stringWidth = strlen("Waiting for other players...")*16;
        Render::RenderText(1920-stringWidth-20, 1080-80, 0, 0x00, "Waiting for other players...");
    }
    else if (this->m_state == MenuState::WAIT_FOR_SERVER_CONNECTING) {
        const u32 stringWidth = strlen("Connecting to server...")*16;
        Render::RenderText(1920-stringWidth-20, 1080-80, 0, 0x00, "Connecting to server...");
    }
    nn::swkbd::DrawTV();
    nn::swkbd::DrawDRC();
}
