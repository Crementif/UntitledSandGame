#include "../common/common.h"
#include "GameScene.h"
#include "GameSceneMenu.h"
#include "Landmine.h"
#include "TextButton.h"
#include "Collectable.h"
#include "Missile.h"
#include "Blackhole.h"

#include "../framework/navigation.h"
#include "../framework/audio.h"
#include "../framework/debug.h"

GameScene* GameScene::sActiveScene = nullptr;

Sprite* TextButton::s_buttonSelected = nullptr;
Sprite* TextButton::s_buttonBackdrop = nullptr;
Sprite* Landmine::s_landmineSprite = nullptr;
Sprite* Missile::s_missile0Sprite = nullptr;
Sprite* Missile::s_missile1Sprite = nullptr;
Sprite* Missile::s_missile2Sprite = nullptr;
Sprite* Missile::s_missile3Sprite = nullptr;
Sprite* Blackhole::s_blackhole0Sprite = nullptr;
Sprite* Blackhole::s_blackhole1Sprite = nullptr;
Sprite* Blackhole::s_blackhole2Sprite = nullptr;
Sprite* Collectable::s_collectableSprite = nullptr;

#include <chrono>

int main()
{
    WHBLogCafeInit();
    initializeInputs();

    Render::Init();
    GLSL_Init();

    TextButton::s_buttonSelected = new Sprite("/tex/button_selected.tga", true);
    TextButton::s_buttonBackdrop = new Sprite("/tex/button_backdrop.tga", true);
    Landmine::s_landmineSprite = new Sprite("/tex/landmine.tga", true);
    Missile::s_missile0Sprite = new Sprite("/tex/missile0.tga", true);
    Missile::s_missile1Sprite = new Sprite("/tex/missile1.tga", true);
    Missile::s_missile2Sprite = new Sprite("/tex/missile2.tga", true);
    Missile::s_missile3Sprite = new Sprite("/tex/missile3.tga", true);
    Blackhole::s_blackhole0Sprite = new Sprite("/tex/blackhole0.tga", true);
    Blackhole::s_blackhole0Sprite->SetupSampler(false);
    Blackhole::s_blackhole1Sprite = new Sprite("/tex/blackhole1.tga", true);
    Blackhole::s_blackhole1Sprite->SetupSampler(false);
    Blackhole::s_blackhole2Sprite = new Sprite("/tex/blackhole2.tga", true);
    Blackhole::s_blackhole2Sprite->SetupSampler(false);
    Collectable::s_collectableSprite = new Sprite("/tex/ammo.tga", true);

    // set polygon mode
    GX2SetPolygonControl(
            GX2_FRONT_FACE_CCW, // Front-face Mode
            FALSE,              // Disable Culling
            FALSE,              // ^^^^^^^^^^^^^^^
            TRUE,               // Enable Polygon Mode
            GX2_POLYGON_MODE_TRIANGLE, // Front Polygon Mode
            GX2_POLYGON_MODE_TRIANGLE, // Back Polygon Mode
            FALSE, // Disable Polygon Offset
            FALSE, // ^^^^^^^^^^^^^^^^^^^^^^
            FALSE  // ^^^^^^^^^^^^^^^^^^^^^^
    );

    GameScene::ChangeTo(new GameSceneMenu());

    GameScene* currGameScene = nullptr;
    double lastTotalTime = 0.0f;
    double lastFrameWithoutPostProcessingTime = 0.0f;
    double lastFrameTime = 0.0f;
    while (Render::IsRunning())
    {
        DebugLog::Printf("Total Time: %.04lf ms", lastTotalTime);
        DebugLog::Printf("Render Scene Time: %.04lf ms", lastFrameWithoutPostProcessingTime);
        DebugLog::Printf("Post Processing Time: %.04lf ms", lastFrameTime - lastFrameWithoutPostProcessingTime);
        DebugLog::Printf("Swap Buffers Time: %.04lf ms", lastTotalTime - lastFrameTime);

        u64 startTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

        if (currGameScene != GameScene::sActiveScene)
            currGameScene = GameScene::sActiveScene;

        assert(currGameScene != nullptr);

        updateInputs();
        AudioManager::GetInstance().ProcessAudio();

        Render::BeginFrame();
        Render::SetStateForSpriteRendering();

        currGameScene->HandleInput();
        currGameScene->Draw();

        lastFrameWithoutPostProcessingTime = (double)(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - startTime);
        Render::DoPostProcessing();
        lastFrameTime = (double)(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - startTime);
        Render::SwapBuffers();
        lastTotalTime = (double)(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - startTime);

        if (currGameScene != GameScene::sActiveScene)
        {
            delete currGameScene;
            currGameScene = nullptr;
        }

        if (GameScene::sActiveScene == nullptr)
            break;

    }

    GLSL_Shutdown();
    Render::Shutdown();

    WHBLogCafeDeinit();
    return 0;
}

[[noreturn]]
void CriticalErrorHandler(const char* msg, ...)
{
    va_list va;
    va_start(va, msg);
    WHBLogPrintf(msg);
    va_end(va);
    *((unsigned int*)0) = 0xDEAD;
    OSFatal("FATAL ERROR, CHECK LOG!");
    exit(EXIT_FAILURE);
}