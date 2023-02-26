#pragma once

#include "NetCommon.h"
#include "Player.h"
#include "GameClient.h"
#include "GameServer.h"

class GameScene
{
public:
    GameScene(std::unique_ptr<GameClient> client = nullptr, std::unique_ptr<GameServer> server = nullptr): m_gameClient(std::move(client)), m_gameServer(std::move(server)) {};
    virtual ~GameScene() {};

    virtual void Draw() = 0;
    virtual void HandleInput() {}

    static GameScene* sActiveScene;

    static void ChangeTo(GameScene* newGameScene)
    {
        sActiveScene = newGameScene;
    }
    static GameScene* GetCurrent() {
        return sActiveScene;
    }

    std::unordered_map<PlayerID, std::unique_ptr<Player>> sCurrPlayers;
    Player* RegisterPlayer(PlayerID id, f32 playerX, f32 playerY) {
        return sCurrPlayers.emplace(id, std::make_unique<Player>(id, playerX, playerY)).first->second.get();
    }
    const std::unordered_map<PlayerID, std::unique_ptr<Player>>& GetPlayers() const {
        return sCurrPlayers;
    };
    void UnregisterAllPlayers() {
        sCurrPlayers.clear();
    }

protected:
    std::unique_ptr<GameClient> m_gameClient;
    std::unique_ptr<GameServer> m_gameServer;
};