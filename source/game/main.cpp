#include "../common/common.h"
//#include <nsysnet/_socket.h>
#include "GameScene.h"
#include "GameSceneIngame.h"
#include "../framework/render.h"
#include "../framework/navigation.h"
#include "../framework/audio.h"

GameScene* sActiveGameScene{nullptr};

void SwitchToGameScene(GameScene* newGameScene)
{
    if (sActiveGameScene)
        delete sActiveGameScene;
    sActiveGameScene = newGameScene;
}

int main()
{
    WHBLogCafeInit();
    WHBLogUdpInit();
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

    SwitchToGameScene(new GameSceneIngame());

    while (Render::IsRunning())
    {
        updateInputs();
        AudioManager::GetInstance().ProcessAudio();
        if (sActiveGameScene)
        {
            sActiveGameScene->HandleInput();
            sActiveGameScene->Draw();
        }
        Render::SwapBuffers();
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