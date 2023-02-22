#pragma once

class GameScene
{
public:
    virtual ~GameScene() {};

    virtual void Draw() {}
    virtual void HandleInput() {}
};