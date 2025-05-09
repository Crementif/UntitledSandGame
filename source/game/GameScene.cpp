#include "GameScene.h"
#include "Player.h"
#include "Map.h"
#include "../framework/debug.h"

GameScene::GameScene(std::unique_ptr<GameClient> client, std::unique_ptr<GameServer> server): m_gameClient(std::move(client)), m_gameServer(std::move(server)) {
};

GameScene::~GameScene() {
    while (!m_objects.empty()) {
        Object* obj = m_objects.back();
        delete obj;
    }

    delete this->m_map;
}

Player* GameScene::RegisterPlayer(PlayerID id, f32 playerX, f32 playerY) {
    return sCurrPlayers.emplace(id, new Player(this, id, playerX, playerY)).first->second;
}

const std::unordered_map<PlayerID, Player*>& GameScene::GetPlayers() const {
    return sCurrPlayers;
}

bool GameScene::IsSingleplayer() const {
    return sCurrPlayers.size() == 1;
}

Player* GameScene::GetPlayerById(PlayerID id) const {
    for (auto& player : sCurrPlayers) {
        if (player.first == id)
            return player.second;
    }
    return nullptr;
}

Player* GameScene::GetPlayer() const {
    for (auto& player : sCurrPlayers) {
        if (player.second->IsSelf())
            return player.second;
    }
    return nullptr;
}

void GameScene::UnregisterAllPlayers() {
    sCurrPlayers.clear();
}

void GameScene::QueueUnregisterObject(struct Object *obj) {
    m_deletionQueue.emplace_back(obj);
}

void GameScene::RegisterObject(struct Object *newObj) {
    m_objects.emplace_back(newObj);
    if (newObj->m_requiresUpdating)
        m_objectsToUpdate.emplace_back(newObj);
    for (u32 i=0; i<Object::NUM_DRAW_LAYERS; i++) {
        if ((newObj->m_drawLayerMask&(1<<i)) == 0)
            continue;
        m_drawLayers[i].emplace_back(newObj);
    }
}

void GameScene::UnregisterObject(struct Object *obj) {
    auto it = std::find(m_objects.begin(), m_objects.end(), obj);
    if (it != m_objects.end())
        m_objects.erase(it);

    auto it2 = std::find(m_objectsToUpdate.begin(), m_objectsToUpdate.end(), obj);
    if (it2 != m_objectsToUpdate.end())
        m_objectsToUpdate.erase(it2);

    for (u32 i=0; i<Object::NUM_DRAW_LAYERS; i++) {
        if ((obj->m_drawLayerMask&(1<<i)) == 0)
            continue;
        auto it3 = std::find(m_drawLayers[i].begin(), m_drawLayers[i].end(), obj);
        if (it3 != m_drawLayers[i].end())
            m_drawLayers[i].erase(it3);
    }
}

void GameScene::DoObjectUpdates(float timestep) {
    DebugProfile::Start("[CPU] Scene -> Update Objects");
    for (auto& it : m_objectsToUpdate) {
        it->Update(timestep);
    }

    while (!m_deletionQueue.empty()) {
        UnregisterObject(m_deletionQueue.back());
        m_deletionQueue.pop_back();
    }
    DebugProfile::End("[CPU] Scene -> Update Objects");
}

void GameScene::DoObjectDraws() {
    for (u32 i=0; i<Object::NUM_DRAW_LAYERS; i++) {
        for (auto& obj : m_drawLayers[i]) {
            obj->Draw(i);
        }
    }
}