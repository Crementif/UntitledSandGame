#include "../common/common.h"
#include "GameScene.h"
#include "GameSceneMenu.h"
#include "Landmine.h"
#include "TextButton.h"
#include "Collectable.h"
#include "Missile.h"
#include "Blackhole.h"

#include "../framework/navigation.h"
#include "../framework/audio/audio.h"
#include "../framework/debug.h"
#include "../framework/window.h"

GameScene* GameScene::s_activeScene = nullptr;

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

Settings GameScene::s_settings = {
};


int main()
{
    WHBLogCafeInit();
    initializeInputs();

    Render::Init();

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
        FALSE,              // Disable cullFront
        FALSE,              // Disable cullBack
        TRUE,               // Enable Polygon Mode
        GX2_POLYGON_MODE_TRIANGLE, // Front Polygon Mode
        GX2_POLYGON_MODE_TRIANGLE, // Back Polygon Mode
        FALSE, // Disable Polygon Offset
        FALSE, // ^^^^^^^^^^^^^^^^^^^^^^
        FALSE  // ^^^^^^^^^^^^^^^^^^^^^^
    );

    GameScene::ChangeTo(new GameSceneMenu());

    GameScene* currGameScene = nullptr;
    while (Render::IsRunning())
    {
        DebugProfile::Start("Total");

        if (currGameScene != GameScene::s_activeScene)
            currGameScene = GameScene::s_activeScene;

        assert(currGameScene != nullptr);

        // update inputs
        updateInputs();
        currGameScene->HandleInput();

        // update audio
        AudioManager::GetInstance().ProcessAudio();

        // update scene and objects
        currGameScene->Update();

        // wait for buffers from the previous frame to be finished before drawing
        DebugProfile::Start("WaitForPreviousFrame");
        WindowWaitForPreviousFrame(1);
        DebugProfile::End("WaitForPreviousFrame");

        // draw the frame
        Render::BeginFrame();
        Render::SetStateForSpriteRendering();

        DebugProfile::Start("Scene");
        currGameScene->Draw();
        DebugProfile::End("Scene");

        DebugProfile::Start("Post Processing");
        Render::DoPostProcessing();
        DebugProfile::End("Post Processing");
        DebugProfile::Start("Swap Buffers");
        Render::SwapBuffers();
        DebugProfile::End("Swap Buffers");

        if (currGameScene != GameScene::s_activeScene)
        {
            delete currGameScene;
            currGameScene = nullptr;
        }

        if (GameScene::s_activeScene == nullptr)
            break;
        DebugProfile::End("Total");

        DebugProfile::Print();
    }

    Render::Shutdown();

    WHBLogCafeDeinit();
    return 0;
}

[[noreturn]]
void CriticalErrorHandler(const char* msg, ...)
{
    char buffer[1024];
    va_list va;
    va_start(va, msg);
    vsnprintf(buffer, sizeof(buffer), msg, va);
    va_end(va);
    WHBLogPrint(buffer);
    OSFatal(buffer);
    *((unsigned int*)0) = 0xDEAD;
    exit(EXIT_FAILURE);
}