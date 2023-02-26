#pragma once
#include "../common/common.h"
#include "Object.h"
#include "../framework/render.h"

class TextButton : public Object {
public:
    TextButton(GameScene* parent, AABB box, std::string_view text): Object(parent, box+Vector2f(-box.scale.x/2, -box.scale.y/2), true, DRAW_LAYER_0), m_text(text) {
        this->m_textSize = this->m_aabb.scale.y <= 50.0f ? 0 : 1;
        this->m_textStartX = (u32)(this->m_aabb.GetCenter().x-((((float)text.size()*(this->m_textSize == 0 ? 14.0f : 26.0f))/2)));
        this->m_textStartY = (u32)(this->m_aabb.GetCenter().y-((this->m_textSize == 0 ? 32.0f : 64.0f)/2));
    };

    static Sprite* s_buttonBackdrop;
private:
    void Draw(u32 layerIndex) override {
        Render::RenderSprite(s_buttonBackdrop, (s32)m_aabb.pos.x, (s32)m_aabb.pos.y, (s32)m_aabb.scale.x, (s32)m_aabb.scale.y);
        Render::RenderText(this->m_textStartX, this->m_textStartY, this->m_textSize, 0xFF, m_text.c_str());
    };
    void Update(float timestep) override {
    };
    Vector2f GetPosition() override {
        return m_aabb.GetCenter();
    };

    std::string m_text;
    u32 m_textSize;
    u32 m_textStartX;
    u32 m_textStartY;
};