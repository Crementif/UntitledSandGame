#include "../common/common.h"
#include "GameSceneMenu.h"
#include "Object.h"
#include "Map.h"

#include "../framework/navigation.h"
#include "GameSceneIngame.h"
#include "GameServer.h"
#include "GameClient.h"
#include "../framework/audio.h"

bool GameScene::s_showCrtFilter = true;

GameSceneMenu::GameSceneMenu(MenuScoreboard scoreboard): GameScene(), m_scoreboard(scoreboard) {
    this->RegisterMap(new Map("menu.tga", 1337));

    m_selectAudio = new Audio("/sfx/select.wav");
    m_startAudio = new Audio("/sfx/start.wav");

    m_sandbox_btn = new TextButton(this, AABB{1920.0f/2, 1080.0f/2+150, 500, 80}, s_buttonSize, s_buttonColor, L"Sandbox");
    m_host_btn = new TextButton(this, AABB{1920.0f/2, 1080.0f/2+250, 500, 80}, s_buttonSize, s_buttonColor, L"Host");
    m_join_btn = new TextButton(this, AABB{1920.0f/2, 1080.0f/2+350, 500, 80}, s_buttonSize, s_buttonColor, L"Join");
    m_crt_btn = new TextButton(this, AABB{1920.0f/2, 1080.0f/2+450, 500, 80}, s_buttonSize, s_buttonColor, s_showCrtFilter ? L"Filter: ON" : L"Filter: OFF");

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
    delete m_crt_btn;

    delete m_selectAudio;
    delete m_startAudio;

    nn::swkbd::Destroy();
    FSDelClient(m_fsClient, FS_ERROR_FLAG_NONE);
    MEMFreeToDefaultHeap(m_fsClient);
}


void GameSceneMenu::HandleInput() {
    bool isTouchValid = false;
    s32 screenX, screenY;
    vpadGetTouchInfo(isTouchValid, screenX, screenY);

    const bool activeInputDebounce = (m_lastInput + (OSTime)OSMillisecondsToTicks(600)) >= OSGetTime();
    if (navigatedUp() && m_selectedButton > 0 && !activeInputDebounce) {
        m_lastInput = OSGetTime();
        m_selectedButton--;
        if (m_selectAudio->GetState() == Audio::StateEnum::PLAYING) m_selectAudio->Reset();
        else m_selectAudio->Play();
    }
    else if (navigatedDown() && m_selectedButton < 3 && !activeInputDebounce) {
        m_lastInput = OSGetTime();
        m_selectedButton++;
        if (m_selectAudio->GetState() == Audio::StateEnum::PLAYING) m_selectAudio->Reset();
        else m_selectAudio->Play();
    }

    vpadUpdateSWKBD();
    if (nn::swkbd::IsNeedCalcSubThreadFont()) {
        nn::swkbd::CalcSubThreadFont();
    }
    if (nn::swkbd::IsNeedCalcSubThreadPredict()) {
        nn::swkbd::CalcSubThreadPredict();
    }

    if (m_gameServer)
        m_gameServer->Update();
    if (m_gameClient) {
        m_gameClient->Update();
        if (m_gameClient->GetGameState() == GameClient::GAME_STATE::STATE_INGAME)
            GameScene::ChangeTo(new GameSceneIngame(std::move(m_gameClient), std::move(m_gameServer)));
    }

    // Client-specific states
    bool pressedOkButton = false;
    if (m_state == MenuState::WAIT_FOR_INPUT && nn::swkbd::IsDecideOkButton(&pressedOkButton)) {
        nn::swkbd::DisappearInputForm();
        m_state = MenuState::WAIT_FOR_CONNECTION;

        auto ipAddress = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>().to_bytes(nn::swkbd::GetInputFormString());
        m_gameClient = std::make_unique<GameClient>(ipAddress);
    }
    bool pressedCancelButton = false;
    if (m_state == MenuState::WAIT_FOR_INPUT && nn::swkbd::IsDecideCancelButton(&pressedCancelButton)) {
        nn::swkbd::DisappearInputForm();
        m_state = MenuState::NORMAL;
    }

    // Server states
    if (m_state == MenuState::WAIT_FOR_GAME && m_gameServer && (pressedStart() || this->m_startSandboxImmediately) && !this->m_startPacketSent) {
        // tell server to start game
        this->m_startPacketSent = true;
        m_gameServer->StartGame();
        //GameScene::ChangeTo(new GameSceneIngame(m_gameClient, m_gameServer)); -> The client controls this
    }

    // Client and server states
    if (m_state == MenuState::WAIT_FOR_CONNECTION && m_gameClient->IsConnected()) {
        m_state = MenuState::WAIT_FOR_GAME;
    }

    if (m_state == MenuState::NORMAL) {
        if (activeInputDebounce)
            return;

        Vector2f touchPos = Vector2f{isTouchValid ? (f32)screenX : 0.0f, isTouchValid ? (f32)screenY : 0.0f};
        if (m_sandbox_btn->GetBoundingBox().Contains(touchPos) || (m_selectedButton == 0 && pressedOk())) {
            m_lastInput = OSGetTime();
            this->m_state = MenuState::WAIT_FOR_CONNECTION;
            if (m_selectAudio->GetState() == Audio::StateEnum::PLAYING) m_selectAudio->Reset();
            else m_selectAudio->Play();

            this->m_gameServer = std::make_unique<GameServer>();
            this->m_gameClient = std::make_unique<GameClient>("127.0.0.1");

            this->m_startSandboxImmediately = true;
        }
        else if (m_host_btn->GetBoundingBox().Contains(touchPos) || (m_selectedButton == 1 && pressedOk())) {
            m_lastInput = OSGetTime();
            this->m_state = MenuState::WAIT_FOR_CONNECTION;
            if (m_selectAudio->GetState() == Audio::StateEnum::PLAYING) m_selectAudio->Reset();
            else m_selectAudio->Play();

            this->m_gameServer = std::make_unique<GameServer>();
            this->m_gameClient = std::make_unique<GameClient>("127.0.0.1");
        }
        else if (m_join_btn->GetBoundingBox().Contains(touchPos) || (m_selectedButton == 2 && pressedOk())) {
            m_lastInput = OSGetTime();
            this->m_state = MenuState::WAIT_FOR_INPUT;
            if (m_selectAudio->GetState() == Audio::StateEnum::PLAYING) m_selectAudio->Reset();
            else m_selectAudio->Play();

            nn::swkbd::AppearArg appearArg = {
                .keyboardArg = {
                    .configArg = nn::swkbd::ConfigArg(),
                    .receiverArg = nn::swkbd::ReceiverArg()
                },
                .inputFormArg = nn::swkbd::InputFormArg()
            };
            appearArg.keyboardArg.configArg.keyboardMode = nn::swkbd::KeyboardMode::Numpad;
            appearArg.keyboardArg.configArg.disableNewLine = true;
            appearArg.keyboardArg.configArg.okString = u"Connect";
            appearArg.keyboardArg.configArg.showWordSuggestions = false;
            appearArg.keyboardArg.configArg.languageType = nn::swkbd::LanguageType::English;
            appearArg.keyboardArg.configArg.numpadCharRight = u'.';
            appearArg.inputFormArg.initialText = u"127.0.0.1";
            appearArg.inputFormArg.hintText = u"Type IP address";
            appearArg.inputFormArg.maxTextLength = 16;
            appearArg.inputFormArg.drawInput0Cursor = true;
            if (!nn::swkbd::AppearInputForm(appearArg))
                OSFatal("nn::swkbd::AppearInputForm failed");
        }
        else if (m_crt_btn->GetBoundingBox().Contains(touchPos) || (m_selectedButton == 3 && pressedOk())) {
            m_lastInput = OSGetTime();
            if (m_selectAudio->GetState() == Audio::StateEnum::PLAYING) m_selectAudio->Reset();
            else m_selectAudio->Play();

            delete m_crt_btn;
            s_showCrtFilter = !s_showCrtFilter;
            m_crt_btn = new TextButton(this, AABB{1920.0f / 2, 1080.0f / 2 + 450, 500, 80}, s_buttonSize, s_buttonColor, s_showCrtFilter ? L"Filter: ON" : L"Filter: OFF");
        }
    }
}

void GameSceneMenu::SimulateAndDrawLevel() {
    if ((this->GetMap()->GetRNGNumber()&0x7) < 1)
        this->GetMap()->SpawnMaterialPixel(MAP_PIXEL_TYPE::SAND, 430, 2);

    if ((this->GetMap()->GetRNGNumber()&0x7) < 1)
        this->GetMap()->SpawnMaterialPixel(MAP_PIXEL_TYPE::SAND, 260, 2);

    if ((this->GetMap()->GetRNGNumber()&0x7) < 2)
        this->GetMap()->SpawnMaterialPixel(MAP_PIXEL_TYPE::LAVA, 2, 178);

    this->GetMap()->SimulateTick();
    this->GetMap()->Draw();
    this->GetMap()->Update(); // map objects are always independent of the world simulation?
}

void GameSceneMenu::DrawButtons() {
    m_sandbox_btn->SetSelected(m_selectedButton == 0);
    m_host_btn->SetSelected(m_selectedButton == 1);
    m_join_btn->SetSelected(m_selectedButton == 2);
    m_crt_btn->SetSelected(m_selectedButton == 3);
    this->DoDraws();
}

void GameSceneMenu::Draw() {
    Render::SetCameraPosition(Vector2f(2, 2) * MAP_PIXEL_ZOOM);
    this->SimulateAndDrawLevel();
    Render::SetStateForSpriteRendering();
    this->DrawButtons();

    if (m_scoreboard == MenuScoreboard::WON) {
        Render::RenderSprite(m_wonScoreboardSprite, 1920/2-(m_wonScoreboardSprite->GetWidth()/2), 1080/2-(m_wonScoreboardSprite->GetHeight()/2)+60);
    }
    if (m_scoreboard == MenuScoreboard::LOST) {
        Render::RenderSprite(m_lostScoreboardSprite, 1920/2-(m_lostScoreboardSprite->GetWidth()/2), 1080/2-(m_lostScoreboardSprite->GetHeight()/2)+60);
    }
    if (m_scoreboard == MenuScoreboard::DIED) {
        Render::RenderSprite(m_diedScoreboardSprite, 1920/2-(m_diedScoreboardSprite->GetWidth()/2), 1080/2-(m_diedScoreboardSprite->GetHeight()/2)+60);
    }

    if (this->m_state == MenuState::WAIT_FOR_GAME && this->m_gameServer) {
        u32 joinedPlayers = this->m_gameServer->GetPlayerCount();
        if (joinedPlayers != this->m_prevPlayerCount) {
            this->m_prevPlayerCount = joinedPlayers;
            std::wstring joinedPlayersText = L"Press START button to start match with "+std::to_wstring(joinedPlayers)+L" players...";
            delete m_startGameWithPlayersSprite;
            m_startGameWithPlayersSprite = Render::RenderTextSprite(32, 0xFFFFFFFF, joinedPlayersText.c_str());
        }
        Render::RenderSprite(m_startGameWithPlayersSprite, 1920/2-(m_startGameWithPlayersSprite->GetWidth()/2), 1080/2-(m_startGameWithPlayersSprite->GetHeight()/2)+60);
    }
    else if (this->m_state == MenuState::WAIT_FOR_GAME) {
        Render::RenderSprite(m_waitingForGameStartSprite, 1920/2-(m_waitingForGameStartSprite->GetWidth()/2), 1080/2-(m_waitingForGameStartSprite->GetHeight()/2)+60);
    }
    else if (this->m_state == MenuState::WAIT_FOR_CONNECTION) {
        Render::RenderSprite(m_connectingToServerSprite, 1920/2-(m_connectingToServerSprite->GetWidth()/2), 1080/2-(m_connectingToServerSprite->GetHeight()/2)+60);
    }
    nn::swkbd::DrawTV();
    nn::swkbd::DrawDRC();
}
