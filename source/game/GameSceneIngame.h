#pragma once
#include "GameScene.h"
#include "../framework/render.h"
#include "../framework/physics/physics.h"

enum BUILDTYPE
{
    // matching the hotbar order
    BUILDTYPE_BEAM_HORIZONTAL_SHORT = 0,
    BUILDTYPE_BEAM_VERTICAL_LONG = 1,
    BUILDTYPE_SPLITTER = 2,
    BUILDTYPE_SPEED_BOOST = 3
};

class GameSceneIngame : public GameScene
{
public:
    GameSceneIngame();
    ~GameSceneIngame() override;

    void Draw() override;
    void HandleInput() override;

    Vector2f GetCameraPosition();

private:
    void DrawBackground();
    AABB GetHotbarPosition();
    AABB GetHotbarButtonPosition(u32 buttonIndex);
    void DrawMenu();

    AABB GetWorldBounds();
    AABB GetDeleteAreaBounds();

    void StartTouch(Vector2f screenPos);
    void UpdateTouch(Vector2f screenPos);
    void EndTouch();

    Sprite m_testSprite{"/tex/cross.tga", true};

    Sprite m_bgSpriteA{"/tex/background_tile_a.tga"};

    // hotbar / menu graphics
    Sprite m_menuBackdrop{"/tex/menu_backdrop.tga", true};
    Sprite m_menuItemBeamHorizontalShort{"/tex/menu_beam.tga", true};
    Sprite m_menuItemBeamVerticalLong{"/tex/menu_beam_2.tga", true};
    Sprite m_menuItemSplitter{"/tex/menu_splitter.tga", true};
    Sprite m_menuSpeedBoost{"/tex/speed_boost_0.tga", true};
    Sprite m_menuItemSelected{"/tex/menu_item_selected.tga", true};
    Sprite m_menuItemDeselected{"/tex/menu_item_deselected.tga", true};

    Sprite m_spriteDeleteArea{"/tex/trash_symbol.tga"};
    Sprite m_spriteDeleteAreaHL{"/tex/trash_symbol_hl.tga"};

    Sprite m_spriteGlow128{"/tex/glow128.tga", true};

    // touch
    bool m_isTouching{false};
    Vector2f m_startTouchPosition;

    // touch scrolling
    bool m_isScrolling{false};
    Vector2f m_startCameraPosition;

    // touch building
    bool m_isBuildMode{false};
    BUILDTYPE m_buildType{};

    // touch dragging / delete
    bool m_isDragMode{false};
    class Object* m_dragObject{nullptr};
    Vector2f m_initialObjectPosition;
    Vector2f m_grabOffset; // grab offset, relative to top-left corner of currently dragged object
    bool m_isHoveringDeleteArea{false};

    // misc
    f32 m_gameTime{};

    // money
    uint32_t grayCurrency = 1000;
    uint32_t blueCurrency = 14;
    uint32_t greenCurrency = 0;
    uint32_t redCurrency = 256;
};