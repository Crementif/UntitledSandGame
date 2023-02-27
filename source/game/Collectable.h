#pragma once
#include "../common/types.h"
#include "../common/common.h"
#include "GameClient.h"
#include "Object.h"

#include "../framework/render.h"

class Collectable : public Object {
public:
    Collectable(GameScene* parent, Vector2f pos): Object(parent, AABB(pos.x-5.0f, pos.y-5.0f, 10.0f, 10.0f), true, DRAW_LAYER_1) {}
    ~Collectable() override = default;

    bool m_hidden = false;
    void Draw(u32 layerIndex) override;
    void Update(float timestep) override;
    Vector2f GetPosition() override { return m_aabb.GetCenter(); };

    OSTime m_respawnTime = 0;
    void Pickup(Player* player);

    static Sprite* s_collectableSprite;
private:
};
