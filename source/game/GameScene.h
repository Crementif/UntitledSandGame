#pragma once

#include "NetCommon.h"
#include "GameClient.h"
#include "GameServer.h"
#include "Object.h"

static inline const int NUM_DRAW_LAYERS = 8;

struct Settings {
    bool showCrtFilter = true;
};

class GameScene
{
    friend class Object;

public:
    explicit GameScene(std::unique_ptr<GameClient> client = nullptr, std::unique_ptr<GameServer> server = nullptr);
    virtual ~GameScene();

    virtual void HandleInput() = 0;
    virtual void Update() = 0;
    virtual void Draw() = 0;

    static GameScene* s_activeScene;
    static void ChangeTo(GameScene* newGameScene) { s_activeScene = newGameScene; }

    void RegisterMap(class Map* newMap) { m_map = newMap; }
    class Map* GetMap() { return m_map; }

    void QueueUnregisterObject(class Object* obj);
    const std::unordered_map<PlayerID, class Player*>& GetPlayers() const;
    Player *GetPlayer() const;
    Player *GetPlayerById(PlayerID id) const;
    bool IsSingleplayer() const;
    GameClient* GetClient() const { return m_gameClient.get(); };

    static Settings s_settings;

protected:
    class Player* RegisterPlayer(PlayerID id, f32 playerX, f32 playerY);
    void UnregisterAllPlayers();

    void RegisterObject(class Object* newObj);
    void UnregisterObject(class Object* obj);

    void DoObjectUpdates(float timestep);
    void DoObjectDraws();

    std::unique_ptr<GameClient> m_gameClient;
    std::unique_ptr<GameServer> m_gameServer;

    class Map* m_map;

    std::unordered_map<PlayerID, class Player*> sCurrPlayers;

    std::vector<class Object*> m_objects;
    std::vector<class Object*> m_objectsToUpdate;
    std::vector<class Object*> m_drawLayers[NUM_DRAW_LAYERS];

    std::vector<class Object*> m_deletionQueue;
};