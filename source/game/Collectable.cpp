#include "Collectable.h"
#include "Player.h"
#include "Map.h"

#include "../framework/audio.h"


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

constexpr s32 min = 1, max = 3;
constexpr s32 range = max - min + 1;

static std::unordered_map<u32, u8> counts;
static s32 lastRepeatedValue = -1;
void Collectable::Pickup(Player *player) {
    m_hidden = true;
    m_respawnTime = OSGetTime() + OSSecondsToTicks(45);

    if (player->IsSelf()) {
        Audio* pickupAudio = new Audio("/sfx/pickup.wav");
        pickupAudio->Play();
        pickupAudio->QueueDestroy();
    }

    auto pickRandomAbility = [&]() {
        s32 value;
        do {
            value = min + std::rand() % range;
        } while (counts[value] == 3 && value == lastRepeatedValue);

        if (value != lastRepeatedValue && lastRepeatedValue != -1) {
            counts[lastRepeatedValue] = 0;
            lastRepeatedValue = -1;
        }

        if (counts[value] == 2) {
            lastRepeatedValue = value;
        }

        counts[value]++;
        return (GameClient::GAME_ABILITY)value;
    };
    player->GiveAbility(pickRandomAbility());
}
