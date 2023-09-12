#pragma once
#include "../common/common.h"
#include "Object.h"
#include "../framework/render.h"

class TextButton : public Object {
    friend class GameSceneMenu;
public:
    TextButton(GameScene* parent, AABB bgBox, u8 size, u32 color, std::wstring_view text): Object(parent, bgBox + Vector2f(-bgBox.scale.x / 2, -bgBox.scale.y / 2), true, DRAW_LAYER_0), m_selected(false) {
        // todo: implement centering of text with new libschrift renderer (requires calculating text box width)
        this->m_textSprite = Render::RenderTextSprite(size, color, std::wstring(text).c_str());

        this->m_textStartX = (s32)this->m_aabb.GetCenter().x - (m_textSprite->GetWidth()/2);
        this->m_textStartY = (s32)this->m_aabb.GetCenter().y - (m_textSprite->GetHeight()/2);
        this->m_textWidth = m_textSprite->GetWidth();
        this->m_textHeight = m_textSprite->GetHeight();
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
        Render::RenderSprite(m_textSprite, m_textStartX, m_textStartY-5/*hardcoded Y offset...*/, m_textWidth, m_textHeight);
    };
    void Update(float timestep) override {
    };
    Vector2f GetPosition() override {
        return m_aabb.GetCenter();
    };

    Sprite* m_textSprite;
    s32 m_textStartX;
    s32 m_textStartY;
    s32 m_textWidth;
    s32 m_textHeight;
    bool m_selected;
};