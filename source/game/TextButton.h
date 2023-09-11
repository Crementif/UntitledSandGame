#pragma once
#include "../common/common.h"
#include "Object.h"
#include "../framework/render.h"

class TextButton : public Object {
    friend class GameSceneMenu;
public:
    TextButton(GameScene* parent, AABB box, std::wstring_view text): Object(parent, box+Vector2f(-box.scale.x/2, -box.scale.y/2), true, DRAW_LAYER_0), m_selected(false) {
        this->m_textSize = this->m_aabb.scale.y <= 50.0f ? 32 : 64;
        // todo: implement centering of text with new libschrift renderer (requires calculating text box width)
        this->m_textStartX = (u32)(this->m_aabb.GetCenter().x-((((float)text.size()*(this->m_textSize == 0 ? 14.0f : 26.0f))/2)));
        this->m_textStartY = (u32)(this->m_aabb.GetCenter().y-((this->m_textSize == 0 ? 32.0f : 64.0f)/2));
        this->m_textSprite = Render::RenderTextSprite(m_textSize, 0xFFFF00FF, std::wstring(text).c_str());
    };

    ~TextButton() override {
        delete m_textSprite;
    }

    static Sprite* s_buttonBackdrop;
    static Sprite* s_buttonSelected;
    void SetSelected(bool selected) { m_selected = selected; };
protected:
    void Draw(u32 layerIndex) override {
        Render::RenderSprite(m_selected ? s_buttonSelected : s_buttonBackdrop, (s32)m_aabb.pos.x, (s32)m_aabb.pos.y, (s32)m_aabb.scale.x, (s32)m_aabb.scale.y);
        Render::RenderSprite(m_textSprite, this->m_textStartX, this->m_textStartY, (u32)m_textSprite->GetWidth(), (u32)m_textSprite->GetHeight());
    };
    void Update(float timestep) override {
    };
    Vector2f GetPosition() override {
        return m_aabb.GetCenter();
    };

    Sprite* m_textSprite;
    u32 m_textSize;
    u32 m_textStartX;
    u32 m_textStartY;
    bool m_selected;
};