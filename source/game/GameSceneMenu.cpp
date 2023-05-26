#include "../common/common.h"
#include "GameSceneMenu.h"
#include "Object.h"
#include "Map.h"

#include "../framework/navigation.h"
#include "GameSceneIngame.h"
#include "GameServer.h"
#include "GameClient.h"
#include "../framework/audio.h"

GameSceneMenu::GameSceneMenu(MenuScoreboard scoreboard): GameScene(), m_scoreboard(scoreboard), m_lastInput(OSGetTick()) {
    this->RegisterMap(new Map("menu.tga", 1337));

    m_selectAudio = new Audio("/sfx/select.wav");
    m_startAudio = new Audio("/sfx/start.wav");

    m_sandbox_btn = new TextButton(this, AABB{1920.0f/2, 1080.0f/2+150, 500, 80}, "Sandbox");
    m_host_btn = new TextButton(this, AABB{1920.0f/2, 1080.0f/2+250, 500, 80}, "Host");
    m_join_btn = new TextButton(this, AABB{1920.0f/2, 1080.0f/2+350, 500, 80}, "Join");

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

    if (navigatedUp() && m_selectedButton > 0 && m_lastInput < OSGetTick()) {
        m_lastInput = OSGetTick() + OSMillisecondsToTicks(400);
        m_selectedButton--;
        if (m_selectAudio->GetState() == Audio::StateEnum::PLAYING) m_selectAudio->Reset();
        else m_selectAudio->Play();
    }
    else if (navigatedDown() && m_selectedButton < 2 && m_lastInput < OSGetTick()) {
        m_lastInput = OSGetTick() + OSMillisecondsToTicks(400);
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
    if (m_gameClient)
    {
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
        Vector2f touchPos = Vector2f{isTouchValid ? (f32)screenX : 0.0f, isTouchValid ? (f32)screenY : 0.0f};
        if (m_sandbox_btn->GetBoundingBox().Contains(touchPos) || (m_selectedButton == 0 && pressedOk())) {
            this->m_state = MenuState::WAIT_FOR_CONNECTION;

            this->m_gameServer = std::make_unique<GameServer>();
            this->m_gameClient = std::make_unique<GameClient>("127.0.0.1");

            this->m_startSandboxImmediately = true;
        }
        else if (m_host_btn->GetBoundingBox().Contains(touchPos) || (m_selectedButton == 1 && pressedOk())) {
            this->m_state = MenuState::WAIT_FOR_CONNECTION;

            this->m_gameServer = std::make_unique<GameServer>();
            this->m_gameClient = std::make_unique<GameClient>("127.0.0.1");
        }
        else if (m_join_btn->GetBoundingBox().Contains(touchPos) || (m_selectedButton == 2 && pressedOk())) {
            this->m_state = MenuState::WAIT_FOR_INPUT;

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
    this->DoDraws();
}

void GameSceneMenu::Draw() {
    Render::SetCameraPosition(Vector2f(2, 2) * MAP_PIXEL_ZOOM);
    this->SimulateAndDrawLevel();
    Render::SetStateForSpriteRendering();
    this->DrawButtons();

    if (m_scoreboard == MenuScoreboard::WON) {
        Render::RenderText(1920/2-((strlen("You won!")*26)/2), 1080/2+14, 1, 0x00, "You won!");
    }
    if (m_scoreboard == MenuScoreboard::LOST) {
        Render::RenderText(1920/2-((strlen("You lost!")*26)/2), 1080/2+14, 1, 0x00, "You lost!");
    }
    if (m_scoreboard == MenuScoreboard::DIED) {
        Render::RenderText(1920/2-((strlen("You died!")*26)/2), 1080/2+14, 1, 0x00, "You died!");
    }

    if (this->m_state == MenuState::WAIT_FOR_GAME && this->m_gameServer) {
        u32 joinedPlayers = this->m_gameServer->GetPlayerCount();
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
