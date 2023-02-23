#include "../common/common.h"
//#include <nsysnet/_socket.h>
#include "GameScene.h"
#include "GameSceneMenu.h"
#include "GameSceneIngame.h"
#include "Object.h"
#include "../framework/render.h"
#include "../framework/navigation.h"
#include "../framework/audio.h"

GameScene* GameScene::sActiveScene = nullptr;

int main()
{
    WHBLogCafeInit();
    romfsInit();
    initializeInputs();

    Render::Init();

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
    while (Render::IsRunning())
    {
        if (currGameScene != GameScene::sActiveScene)
            currGameScene = GameScene::sActiveScene;

        updateInputs();
        AudioManager::GetInstance().ProcessAudio();

        Render::BeginFrame();
        Render::BeginSpriteRendering();

        currGameScene->HandleInput();
        currGameScene->Draw();

        Render::EndSpriteRendering();
        Render::SwapBuffers();

        if (currGameScene != GameScene::sActiveScene)
        {
            delete currGameScene;
            currGameScene = nullptr;
        }
    }

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