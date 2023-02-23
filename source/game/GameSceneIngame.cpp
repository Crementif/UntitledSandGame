#include "../common/common.h"
#include "GameSceneIngame.h"
#include "Object.h"
#include "Map.h"
#include "Player.h"

#include "../framework/navigation.h"
#include "../framework/physics/physics.h"


GameSceneIngame::GameSceneIngame()
{
    //new PachinkoEmitter(256.0f+128.0f, 0.0f);

    //new BallBucket(256.0f * 3.0f + 128.0f, 256.0f * 3.0f + 128.0f);

    m_map = new Map("level0.tga");
    SetCurrentMap(m_map);

    const std::vector<Vector2i> spawnpoints = m_map->GetPlayerSpawnpoints();
    if(spawnpoints.empty())
        CriticalErrorHandler("Level does not have any spawnpoints");

    Vector2i spawnpos = spawnpoints[rand()%spawnpoints.size()]; // non-deterministic for now

    m_selfPlayer = new Player((f32)spawnpos.x, (f32)spawnpos.y);
    m_prevCamPos = Render::GetCameraPosition();
}

GameSceneIngame::~GameSceneIngame()
{

}

void GameSceneIngame::DrawBackground()
{
    Vector2f camPos = Render::GetCameraPosition();
    s32 camX = (s32)camPos.x;
    s32 camY = (s32)camPos.y;

    // todo - we'll have a bounded world with unlockable regions? Show different background tiles based on the area

    /*
    s32 bgTileOffsetX = (camX-255) / 256;
    s32 bgTileOffsetY = (camY-255) / 256;

    for(s32 y=0; y<5; y++)
    {
        for(s32 x=0; x<10; x++)
        {
            Render::RenderSprite(&m_bgSpriteA, (bgTileOffsetX + x) * 256, (bgTileOffsetY + y) * 256);
        }
    }*/
}

// return screen rect of hotbar area
AABB GameSceneIngame::GetHotbarPosition()
{
    return {(1920-1040) / 2, 1080-152 - 50, 1040, 152};
}

// return screen rect of individual button on hotbar
AABB GameSceneIngame::GetHotbarButtonPosition(u32 buttonIndex)
{
    AABB hotbarAABB = GetHotbarPosition();
    return {hotbarAABB.pos.x + 32 + (96 + 32) * buttonIndex, hotbarAABB.pos.y + 25, 96, 96};
}


void GameSceneIngame::DrawMenu()
{
    // menu backdrop 1040 x 152
    AABB hotbarRect = GetHotbarPosition();
    Render::RenderSpriteScreenRelative(&m_menuBackdrop, hotbarRect.pos.x, hotbarRect.pos.y);

    Sprite* buttonSprite[5] = {
            &m_menuItemBeamHorizontalShort,
            &m_menuItemBeamVerticalLong,
            &m_menuItemSplitter,
            &m_menuSpeedBoost
    };

    // menu item backdrop
    for(u32 hotbarButtonIndex = 0; hotbarButtonIndex < 5; hotbarButtonIndex++)
    {
        AABB hotbarButtonAABB = GetHotbarButtonPosition(hotbarButtonIndex);
        bool isSelected = false;
        isSelected = m_isBuildMode && (u32)m_buildType == hotbarButtonIndex;
        Render::RenderSpriteScreenRelative(isSelected ? &m_menuItemSelected : &m_menuItemDeselected, hotbarButtonAABB.pos.x, hotbarButtonAABB.pos.y);
        // item icon
        Sprite* sprite = buttonSprite[hotbarButtonIndex];
        if(sprite)
            Render::RenderSpriteScreenRelative(sprite, hotbarButtonAABB.pos.x, hotbarButtonAABB.pos.y);
    }

    // resources hud
    /*
    constexpr uint32_t maxPaddingLabel = ((strlen("PaddingBux 4294967295")+1)*14);
    constexpr uint32_t maxPaddingScore = ((strlen("4294967295")+1)*14);
    Render::RenderText(1920-maxPaddingLabel, 20, 0, 0xFF, "GrayBux");
    Render::RenderText(1920-maxPaddingScore, 20, 0, 0xFF, "%u", this->grayCurrency);
    Render::RenderText(1920-maxPaddingLabel, 40, 0, 0xFF, "BlueBux");
    Render::RenderText(1920-maxPaddingScore, 40, 0, 0xFF, "%u", this->blueCurrency);
    Render::RenderText(1920-maxPaddingLabel, 60, 0, 0xFF, "GreenBux");
    Render::RenderText(1920-maxPaddingScore, 60, 0, 0xFF, "%u", this->greenCurrency);
    Render::RenderText(1920-maxPaddingLabel, 80, 0, 0xFF, "RedBux");
    Render::RenderText(1920-maxPaddingScore, 80, 0, 0xFF, "%u", this->redCurrency);
     */
}

void GameSceneIngame::UpdateCamera()
{
    Vector2f newCameraPosition = m_selfPlayer->GetPosition();
    m_prevCamPos = newCameraPosition;
    Render::SetCameraPosition(newCameraPosition);
}

void GameSceneIngame::Draw()
{
    UpdateCamera();

    gPhysicsMgr.Update(1.0f / 60.0f);
    Object::DoUpdates(1.0f / 60.0f);

    DrawBackground();

    m_map->Update();
    m_map->Draw();

    if(m_isDragMode)
    {
       AABB deleteArea = GetDeleteAreaBounds();
       Render::RenderSpriteScreenRelative(m_isHoveringDeleteArea?&m_spriteDeleteAreaHL:&m_spriteDeleteArea, deleteArea.pos.x, deleteArea.pos.y);

       // show glow around dragged object
        bool isTouchValid = false;
        s32 screenX, screenY;
        vpadGetTouchInfo(isTouchValid, screenX, screenY);
        Render::RenderSpriteScreenRelative(&m_spriteGlow128, (f32)screenX - 64, (f32)screenY - 64);
    }

    Object::DoDraws();
    DrawMenu();

    m_gameTime += 1.0/60.0;
}

// return bounds of the viewable world in pixel coordinates
AABB GameSceneIngame::GetWorldBounds()
{
    return AABB(0, 0, 256*8, 256*5);
}

AABB GameSceneIngame::GetDeleteAreaBounds()
{
    return AABB(1920 - 96, 1080 - 96, 96, 96);
}

void GameSceneIngame::StartTouch(Vector2f screenPos)
{
    AABB menuScreenRect = GameSceneIngame::GetHotbarPosition();
    /*
    if(menuScreenRect.Contains(screenPos))
    {
        // check which item
        if(GetHotbarButtonPosition(0).Contains(screenPos))
        {
            m_isBuildMode = true;
            m_buildType = BUILDTYPE_BEAM_HORIZONTAL_SHORT;
        }
        else if(GetHotbarButtonPosition(1).Contains(screenPos))
        {
            m_isBuildMode = true;
            m_buildType = BUILDTYPE_BEAM_VERTICAL_LONG;
        }
        else if(GetHotbarButtonPosition(2).Contains(screenPos))
        {
            m_isBuildMode = true;
            m_buildType = BUILDTYPE_SPLITTER;
        }
        else if(GetHotbarButtonPosition(3).Contains(screenPos))
        {
            m_isBuildMode = true;
            m_buildType = BUILDTYPE_SPEED_BOOST;
        }
        return;
    }
    if(m_isBuildMode)
    {
        // place construction
        Vector2f placeCoord = Render::GetCameraPosition() + screenPos;
        if( m_buildType == BUILDTYPE_BEAM_HORIZONTAL_SHORT)
        {
            new ObstacleCircle(placeCoord.x, placeCoord.y, ObstacleCircle::CIRCLE_TYPE::TYPE_BASIC);
            //new ObstacleBeam(placeCoord.x, placeCoord.y, ObstacleBeam::BEAM_TYPE::HORIZONTAL_SHORT);
        }
        else if( m_buildType == BUILDTYPE_BEAM_VERTICAL_LONG)
            new ObstacleBeam(placeCoord.x, placeCoord.y, ObstacleBeam::BEAM_TYPE::VERTICAL_LONG);
        else if( m_buildType == BUILDTYPE_SPLITTER)
            new Splitter(placeCoord.x, placeCoord.y);
        else if( m_buildType == BUILDTYPE_SPEED_BOOST)
            new SpeedBoost(placeCoord.x, placeCoord.y);
        m_isBuildMode = false;
        return;
    }
    else
    {
        // check if we are pressing on a dragable object
        for(auto& objectIt : Object::GetAllObjects() | std::views::reverse)
        {
            AABB objectScreenAABB = objectIt->GetBoundingBox();
            objectScreenAABB.pos = objectScreenAABB.pos - Render::GetCameraPosition();
            if(objectScreenAABB.Contains(screenPos) && objectIt->CanDrag())
            {
                m_isDragMode = true;
                m_startTouchPosition = screenPos;
                m_dragObject = objectIt;
                m_initialObjectPosition = objectIt->GetPosition();
                m_grabOffset = {0.0f, 0.0f};//screenPos - (objectIt->GetBoundingBox().pos - Render::GetCameraPosition());
                m_isHoveringDeleteArea = false;
                return;
            }
        }
    }*/

    m_startTouchPosition = screenPos;
    m_startCameraPosition = Render::GetCameraPosition();
    m_isScrolling = true;
}

void GameSceneIngame::UpdateTouch(Vector2f screenPos)
{
    if(m_isScrolling)
    {
        /*
        AABB cameraBounds = GetWorldBounds();
        cameraBounds.scale.x -= 1920.0;
        if(cameraBounds.scale.x < 0)
            cameraBounds.scale.x = 0.0;
        cameraBounds.scale.y -= 1080.0;
        if(cameraBounds.scale.y < 0)
            cameraBounds.scale.y = 0.0;
        Vector2f newCameraPosition = m_startCameraPosition - (screenPos - m_startTouchPosition);
        newCameraPosition.x = std::clamp(newCameraPosition.x, cameraBounds.pos.x, cameraBounds.pos.x + cameraBounds.scale.x);
        newCameraPosition.y = std::clamp(newCameraPosition.y, cameraBounds.pos.y, cameraBounds.pos.y + cameraBounds.scale.y);

        newCameraPosition = newCameraPosition * 0.2f + Render::GetUnfilteredCameraPosition() * 0.8f;

        Render::SetCameraPosition(newCameraPosition);
         */
    }
    else if(m_isDragMode)
    {
        Vector2f newObjectPosition = m_initialObjectPosition + (screenPos - m_startTouchPosition) + m_grabOffset;
        m_dragObject->UpdatePosition(newObjectPosition);
        m_isHoveringDeleteArea = GetDeleteAreaBounds().Contains(screenPos);
    }
}

void GameSceneIngame::EndTouch()
{
    m_isScrolling = false;
    if(m_isDragMode && m_isHoveringDeleteArea)
    {
        delete m_dragObject;
        m_dragObject = nullptr;
    }
    m_isDragMode = false;
}

void GameSceneIngame::HandleInput()
{
    bool isTouchValid = false;
    s32 screenX, screenY;
    vpadGetTouchInfo(isTouchValid, screenX, screenY);
    if(isTouchValid)
    {
        Vector2f touchScreenPos{(f32)screenX, (f32)screenY};
        if(m_isTouching)
        {
            UpdateTouch(touchScreenPos);
        }
        else
        {
            StartTouch(touchScreenPos);
            m_isTouching = true;
        }
    }
    else if(m_isTouching)
    {
        EndTouch();
        m_isTouching = false;
    }
}