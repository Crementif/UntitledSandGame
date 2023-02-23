#pragma once

class GameScene
{
public:
    virtual ~GameScene() {};

    virtual void Draw() = 0;
    virtual void HandleInput() {}

    static GameScene* sActiveScene;

    static void ChangeTo(GameScene* newGameScene)
    {
        sActiveScene = newGameScene;
    }
};