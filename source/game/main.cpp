#include "../common/common.h"
#include "GameScene.h"
#include "GameSceneMenu.h"
#include "GameSceneIngame.h"
#include "Object.h"
#include "Landmine.h"
#include "TextButton.h"
#include "Collectable.h"
#include "Missile.h"

#include "../framework/render.h"
#include "../framework/navigation.h"
#include "../framework/audio.h"
#include "../framework/compiler.h"

GameScene* GameScene::sActiveScene = nullptr;

Sprite* TextButton::s_buttonSelected = nullptr;
Sprite* TextButton::s_buttonBackdrop = nullptr;
Sprite* Landmine::s_landmineSprite = nullptr;
Sprite* Missile::s_missile0Sprite = nullptr;
Sprite* Missile::s_missile1Sprite = nullptr;
Sprite* Missile::s_missile2Sprite = nullptr;
Sprite* Missile::s_missile3Sprite = nullptr;
Sprite* Collectable::s_collectableSprite = nullptr;

#include <chrono>

int main()
{
    WHBLogCafeInit();
    romfsInit();
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
    double lastFrameTime = 0.0f;
    while (Render::IsRunning())
    {
        g_debugStrings.clear();
        g_debugStrings.emplace_back("Total Time: " + std::to_string(lastTotalTime).substr(0, std::to_string(lastTotalTime).find("."))+" ms");
        g_debugStrings.emplace_back("Render Time: " + std::to_string(lastFrameTime).substr(0, std::to_string(lastFrameTime).find("."))+" ms");

        u64 startTime = std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();

        if (currGameScene != GameScene::sActiveScene)
            currGameScene = GameScene::sActiveScene;

        updateInputs();
        AudioManager::GetInstance().ProcessAudio();

        Render::BeginFrame();
        Render::SetStateForSpriteRendering();

        currGameScene->HandleInput();
        currGameScene->Draw();

        lastFrameTime = (double)(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - startTime);
        Render::SwapBuffers();

        if (currGameScene != GameScene::sActiveScene)
        {
            delete currGameScene;
            currGameScene = nullptr;
        }

        if (GameScene::sActiveScene == nullptr)
            break;

        lastTotalTime = (double)(std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count() - startTime);
    }

    GLSL_Shutdown();
    Render::Shutdown();

    WHBLogCafeDeinit();
    WHBLogUdpDeinit();
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