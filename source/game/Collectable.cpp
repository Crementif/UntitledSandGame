#include "Collectable.h"
#include "Player.h"
#include "Map.h"


void Collectable::Draw(u32 layerIndex) {
    if (!m_hidden) {
        Render::RenderSprite(s_collectableSprite, (s32)m_aabb.pos.x*MAP_PIXEL_ZOOM, (s32)m_aabb.pos.y*MAP_PIXEL_ZOOM-((f32)m_hoverAnim*0.05f), (s32)m_aabb.scale.x*MAP_PIXEL_ZOOM, (s32)m_aabb.scale.y*MAP_PIXEL_ZOOM);
    }
}

void Collectable::Update(float timestep) {
    if (m_respawnTime < OSGetTime()) {
        m_hidden = false;
    }

    if (m_hoverAnimDirUp)
        m_hoverAnim--;
    else
        m_hoverAnim++;
    if (m_hoverAnim >= 60)
        m_hoverAnimDirUp = true;
    else if (m_hoverAnim <= -60)
        m_hoverAnimDirUp = false;
}

void Collectable::Pickup(Player *player) {
    m_hidden = true;
    m_respawnTime = OSGetTime() + OSSecondsToTicks(45);
    player->GiveAbility(GameClient::GAME_ABILITY::LANDMINE);
}