#include "../common/common.h"
#include "GameSceneMenu.h"
#include "Object.h"
#include "Map.h"

#include "../framework/navigation.h"
#include "GameSceneIngame.h"
#include "GameServer.h"
#include "GameClient.h"
#include "../framework/audio/audio.h"
#include "../framework/debug.h"

GameSceneMenu::GameSceneMenu(MenuScoreboard scoreboard): GameScene(), m_scoreboard(scoreboard) {
    this->RegisterMap(new Map("menu.tga", 1337));

    m_lastInput = OSGetTime() + (OSTime)OSSecondsToTicks(1);
    m_selectAudio = new Audio("/sfx/select.ogg");
    m_startAudio = new Audio("/sfx/start.ogg");

    m_sandbox_btn = new TextButton(this, AABB{1920.0f/2, 1080.0f/2+175, 500, 75}, s_buttonSize, s_buttonColor, L"Sandbox");
    m_host_btn = new TextButton(this, AABB{1920.0f/2, 1080.0f/2+175+90, 500, 75}, s_buttonSize, s_buttonColor, L"Host");
    m_join_btn = new TextButton(this, AABB{1920.0f/2, 1080.0f/2+175+90+90, 500, 75}, s_buttonSize, s_buttonColor, L"Join");
    m_crt_btn = new TextButton(this, AABB{1920.0f/2, 1080.0f/2+175+90+90+90, 500, 75}, s_buttonSize, s_buttonColor, s_settings.showCrtFilter ? L"Filter: ON" : L"Filter: OFF");

    std::wstring ipAddress = std::wstring_convert<std::codecvt_utf8<wchar_t>>().from_bytes(GameServer::GetLocalIPAddress());
    m_localIpAddress = Render::RenderTextSprite(24, 0xFFFFFFFF, (L"Your IP address is "+ipAddress).c_str());

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

    delete m_startGameWithPlayersSprite;
    delete m_startingGameInSandboxSprite;
    delete m_localIpAddress;
    delete m_waitingForGameStartSprite;
    delete m_connectingToServerSprite;
    delete m_wonScoreboardSprite;
    delete m_lostScoreboardSprite;
    delete m_diedScoreboardSprite;

    delete m_selectAudio;
    delete m_startAudio;

    nn::swkbd::Destroy();
    FSDelClient(m_fsClient, FS_ERROR_FLAG_NONE);
    MEMFreeToDefaultHeap(m_fsClient);
}


void GameSceneMenu::HandleInput() {
    // reset last input
    m_pressSelectedButton = false;

    Vector2i touchPos = {};
    bool pressedTouchScreen = vpadGetTouchInfo(&touchPos.x, &touchPos.y);

    // update touchscreen navigation
    u32 newSelectedButton = std::numeric_limits<u32>::max();
    bool pressedTouchScreenButton = false;
    if (pressedTouchScreen && !m_touchSpawning) {
        pressedTouchScreenButton = true;
        if (m_sandbox_btn->GetBoundingBox().Contains(touchPos))
            newSelectedButton = 0;
        else if (m_host_btn->GetBoundingBox().Contains(touchPos))
            newSelectedButton = 1;
        else if (m_join_btn->GetBoundingBox().Contains(touchPos))
            newSelectedButton = 2;
        else if (m_crt_btn->GetBoundingBox().Contains(touchPos))
            newSelectedButton = 3;
        else
            pressedTouchScreenButton = false;
    }

    // handle spawning visuals
    if (pressedTouchScreen && !pressedTouchScreenButton) {
        s32 ws_screen_x = (s32)(Render::GetCameraPosition().x / MAP_PIXEL_ZOOM);
        s32 ws_screen_y = (s32)(Render::GetCameraPosition().y / MAP_PIXEL_ZOOM);
        s32 ws_touch_x = ws_screen_x + (touchPos.x / MAP_PIXEL_ZOOM);
        s32 ws_touch_y = ws_screen_y + (touchPos.y / MAP_PIXEL_ZOOM);

        // choose next material type each time we touch the screen
        if (!m_touchSpawning) {
            if (m_touchSpawningType == MAP_PIXEL_TYPE::SMOKE)
                m_touchSpawningType = MAP_PIXEL_TYPE::SAND;
            else
                m_touchSpawningType = (MAP_PIXEL_TYPE)((u8)m_touchSpawningType + 1);
            m_touchSpawning = true;
        }

        // spawn material pixels
        constexpr s32 SPAWN_RADIUS = 2;
        for (s32 y = ws_touch_y - SPAWN_RADIUS; y <= ws_touch_y + SPAWN_RADIUS; y++) {
            for (s32 x = ws_touch_x - SPAWN_RADIUS; x <= ws_touch_x + SPAWN_RADIUS; x++) {
                if (!this->GetMap()->IsPixelOOB(x, y)) {
                    PixelType& pt = this->GetMap()->GetPixelNoBoundsCheck(x, y);
                    if (pt.IsDynamic()) {
                        pt.SetStaticPixelToAir();
                        this->GetMap()->SpawnMaterialPixel(m_touchSpawningType, _GetRandomSeedFromPixelType(m_touchSpawningType), x, y);
                    }
                    else {
                        if (!pt.IsFilled())
                            pt.SetStaticPixel(m_touchSpawningType, _GetRandomSeedFromPixelType(m_touchSpawningType));
                        this->GetMap()->SetPixelColor(x, y, pt.CalculatePixelColor());
                    }
                }
            }
        }
    }
    else {
        m_touchSpawning = false;
    }

    // update software keyboard
    vpadUpdateSWKBD();
    if (nn::swkbd::IsNeedCalcSubThreadFont()) {
        nn::swkbd::CalcSubThreadFont();
    }
    if (nn::swkbd::IsNeedCalcSubThreadPredict()) {
        nn::swkbd::CalcSubThreadPredict();
    }

    // handle keyboard actions
    if (nn::swkbd::GetStateInputForm() == nn::swkbd::State::Visible) {
        if (nn::swkbd::IsDecideOkButton(nullptr)) {
            auto ipAddress = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>().to_bytes(nn::swkbd::GetInputFormString());
            m_gameClient = std::make_unique<GameClient>(ipAddress);

            nn::swkbd::DisappearInputForm();
        }
        if (nn::swkbd::IsDecideCancelButton(nullptr)) {
            nn::swkbd::DisappearInputForm();
        }
    }

    // update button navigation
    if (navigatedUp() && m_selectedButton > 0)
        newSelectedButton = m_selectedButton - 1;
    else if (navigatedDown() && m_selectedButton < 3)
        newSelectedButton = m_selectedButton + 1;

    // do navigation
    bool activeConnection = m_gameClient != nullptr;
    bool activeInputDebounce = (m_lastInput + (OSTime)OSMillisecondsToTicks(600)) >= OSGetTime();
    bool activeKeyboardInput = nn::swkbd::GetStateInputForm() != nn::swkbd::State::Hidden;
    if ((newSelectedButton != std::numeric_limits<u32>::max() && newSelectedButton != m_selectedButton) && !(activeInputDebounce || activeKeyboardInput || activeConnection)) {
        m_lastInput = OSGetTime();
        m_selectedButton = newSelectedButton;
        m_selectAudio->ResetAndPlay();
        pressedTouchScreenButton = false;
    }

    // do selection
    if ((pressedOk() || pressedTouchScreenButton) && !(activeInputDebounce || activeKeyboardInput || activeConnection)) {
        m_lastInput = OSGetTime();
        m_pressSelectedButton = true;
        m_selectAudio->ResetAndPlay();
    }

    // react to input states
    if (!m_pressSelectedButton)
        return;

    switch (m_selectedButton) {
        case 0: { // sandbox button
            this->m_gameServer = std::make_unique<GameServer>();
            this->m_gameClient = std::make_unique<GameClient>("127.0.0.1");
            this->m_state = MenuState::WAIT_FOR_CONNECTION;
            break;
        }
        case 1: { // host button
            this->m_gameServer = std::make_unique<GameServer>();
            this->m_gameClient = std::make_unique<GameClient>("127.0.0.1");
            this->m_state = MenuState::WAIT_FOR_CONNECTION;
            break;
        }
        case 2: { // join button
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
            break;
        }
        case 3: { // crt button
            s_settings.showCrtFilter = !s_settings.showCrtFilter;
            delete m_crt_btn;
            m_crt_btn = new TextButton(this, AABB{1920.0f / 2, 1080.0f / 2 + 450, 500, 80}, s_buttonSize, s_buttonColor, s_settings.showCrtFilter ? L"Filter: ON" : L"Filter: OFF");
            break;
        }
        default:
            CriticalErrorHandler("Tried to select %d button but the current button that is selected isn't handled properly.", m_selectedButton);
    }
}

void GameSceneMenu::SimulateLevel() {
    if ((this->GetMap()->GetRNGNumber()&0x7) < 1)
        this->GetMap()->SpawnMaterialPixel(MAP_PIXEL_TYPE::SAND, _GetRandomSeedFromPixelType(MAP_PIXEL_TYPE::SAND), 430, 2);

    if ((this->GetMap()->GetRNGNumber()&0x7) < 1)
        this->GetMap()->SpawnMaterialPixel(MAP_PIXEL_TYPE::SAND, _GetRandomSeedFromPixelType(MAP_PIXEL_TYPE::SAND), 260, 2);

    if ((this->GetMap()->GetRNGNumber()&0x7) < 2)
        this->GetMap()->SpawnMaterialPixel(MAP_PIXEL_TYPE::LAVA, _GetRandomSeedFromPixelType(MAP_PIXEL_TYPE::LAVA), 2, 178);

    this->GetMap()->SimulateTick();
    this->GetMap()->Update(); // map objects are always independent of the world simulation?
}

void GameSceneMenu::Update() {
    // update camera
    // todo: technically we could be spawning something in the air on the first frame, if the touch screen is used?
    Render::SetCameraPosition(Vector2f(2, 2) * MAP_PIXEL_ZOOM);

    // update map
    this->SimulateLevel();

    // update buttons
    m_sandbox_btn->SetSelected(m_selectedButton == 0);
    m_host_btn->SetSelected(m_selectedButton == 1);
    m_join_btn->SetSelected(m_selectedButton == 2);
    m_crt_btn->SetSelected(m_selectedButton == 3);

    // handle updates
    if (m_gameServer) {
        m_gameServer->Update();
        if (m_gameServer->GetPlayerCount() == 1 && m_selectedButton == 0) {
            m_gameServer->StartGame();
        }
        if (m_selectedButton == 1 && pressedStart() && m_gameServer->GetPlayerCount() >= 1) {
            m_gameServer->StartGame();
        }
    }
    if (m_gameClient) {
        m_gameClient->Update();
        if (m_gameClient->GetGameState() == GameClient::GAME_STATE::STATE_INGAME)
            GameScene::ChangeTo(new GameSceneIngame(std::move(m_gameClient), std::move(m_gameServer)));
    }
}

void GameSceneMenu::Draw() {
    // draw map
    this->GetMap()->Draw();

    // setup sprite drawing
    Render::SetStateForSpriteRendering();

    // draw objects (buttons)
    this->DoObjectDraws();

    // draw matchmaking status
    const bool isHosting = this->m_gameServer != nullptr;
    const bool isSinglePlayer = isHosting && m_selectedButton == 0;
    const bool isJoining = this->m_gameServer == nullptr && this->m_gameClient != nullptr && !this->m_gameClient->IsConnected();
    const bool isJoined = this->m_gameServer == nullptr && this->m_gameClient != nullptr && this->m_gameClient->IsConnected();
    if (isHosting && !isSinglePlayer) {
        u32 joinedPlayers = this->m_gameServer->GetPlayerCount();
        if (joinedPlayers != this->m_prevPlayerCount) {
            this->m_prevPlayerCount = joinedPlayers;
            std::wstring joinedPlayersText = L"Press START button to start match with "+std::to_wstring(joinedPlayers)+L" players...";
            delete m_startGameWithPlayersSprite;
            m_startGameWithPlayersSprite = Render::RenderTextSprite(32, 0xFFFFFFFF, joinedPlayersText.c_str());
        }
        Render::RenderSprite(m_startGameWithPlayersSprite, 1920/2-(m_startGameWithPlayersSprite->GetWidth()/2), 1080/2-(m_startGameWithPlayersSprite->GetHeight()/2)+48);
        Render::RenderSprite(m_localIpAddress, 1920/2-(m_localIpAddress->GetWidth()/2), 1080/2-(m_localIpAddress->GetHeight()/2)+48+32+14);
    }
    else if (isHosting && isSinglePlayer) {
        Render::RenderSprite(m_startingGameInSandboxSprite, 1920/2-(m_startingGameInSandboxSprite->GetWidth()/2), 1080/2-(m_startingGameInSandboxSprite->GetHeight()/2)+60);
    }
    else if (isJoining) {
        Render::RenderSprite(m_waitingForGameStartSprite, 1920/2-(m_waitingForGameStartSprite->GetWidth()/2), 1080/2-(m_waitingForGameStartSprite->GetHeight()/2)+60);
    }
    else if (isJoined) {
        Render::RenderSprite(m_connectingToServerSprite, 1920/2-(m_connectingToServerSprite->GetWidth()/2), 1080/2-(m_connectingToServerSprite->GetHeight()/2)+60);
    }
    else {
        // draw scoreboard
        if (m_scoreboard == MenuScoreboard::WON) {
            Render::RenderSprite(m_wonScoreboardSprite, 1920/2-(m_wonScoreboardSprite->GetWidth()/2), 1080/2-(m_wonScoreboardSprite->GetHeight()/2)+64);
        }
        if (m_scoreboard == MenuScoreboard::LOST) {
            Render::RenderSprite(m_lostScoreboardSprite, 1920/2-(m_lostScoreboardSprite->GetWidth()/2), 1080/2-(m_lostScoreboardSprite->GetHeight()/2)+64);
        }
        if (m_scoreboard == MenuScoreboard::DIED) {
            Render::RenderSprite(m_diedScoreboardSprite, 1920/2-(m_diedScoreboardSprite->GetWidth()/2), 1080/2-(m_diedScoreboardSprite->GetHeight()/2)+64);
        }
    }

    // draw debug log
    DebugLog::Draw();

    // draw software keyboard
    nn::swkbd::DrawTV();
    nn::swkbd::DrawDRC();
}
