#pragma once

#include "NetCommon.h"
#include "GameClient.h"
#include "GameServer.h"
#include "Object.h"

static inline const int NUM_DRAW_LAYERS = 8;

extern std::vector<std::string> g_debugStrings;

class GameScene
{
    friend class Object;

public:
    explicit GameScene(std::unique_ptr<GameClient> client = nullptr, std::unique_ptr<GameServer> server = nullptr);
    virtual ~GameScene();

    virtual void Draw() = 0;
    virtual void HandleInput() {}

    static GameScene* sActiveScene;
    static void ChangeTo(GameScene* newGameScene) { sActiveScene = newGameScene; }

    void RegisterMap(class Map* newMap) { m_map = newMap; }
    class Map* GetMap() { return m_map; }

    void QueueUnregisterObject(class Object* obj);
    const std::unordered_map<PlayerID, std::unique_ptr<class Player>>& GetPlayers() const;
    GameClient* GetClient() const { return m_gameClient.get(); };
protected:
    class Player* RegisterPlayer(PlayerID id, f32 playerX, f32 playerY);
    void UnregisterAllPlayers();

    void RegisterObject(class Object* newObj);
    void UnregisterObject(class Object* obj);

    void DoUpdates(float timestep);
    void DoDraws();

    std::unique_ptr<GameClient> m_gameClient;
    std::unique_ptr<GameServer> m_gameServer;

    class Map* m_map;

    std::unordered_map<PlayerID, std::unique_ptr<class Player>> sCurrPlayers;

    std::vector<class Object*> m_objects;
    std::vector<class Object*> m_objectsToUpdate;
    std::vector<class Object*> m_drawLayers[NUM_DRAW_LAYERS];

    std::vector<class Object*> m_deletionQueue;
};