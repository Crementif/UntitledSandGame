#include "Player.h"
#include "../framework/render.h"
#include "Map.h"

#include "../framework/navigation.h"

#include <coreinit/debug.h>

Sprite* s_sprite_default{nullptr};
Sprite* s_sprite_debug_touch{nullptr};

Player::Player(f32 posX, f32 posY) : Object(CalcAABB(posX, posY), true, DRAW_LAYER_PLAYERS)
{
    Vector2f playerPos = Vector2f(posX, posY - GetPlayerHeight());
    UpdatePosition(playerPos);
    // player_default.tga
    if(!s_sprite_default)
    {
        s_sprite_default = new Sprite("tex/player_default.tga", true);
        s_sprite_debug_touch = new Sprite("tex/debug_touch.tga", false);
    }
}

Player::~Player()
{

}

// width/height in world pixels
s32 Player::GetPlayerWidth() const
{
    return 12;
}

s32 Player::GetPlayerHeight() const
{
    return 20;
}


// player AABB is grounded
AABB Player::CalcAABB(f32 posX, f32 posY)
{
    return AABB(posX - (float)GetPlayerWidth()*0.5f, posY - GetPlayerHeight(), GetPlayerWidth(), GetPlayerHeight());
}

void Player::UpdatePosition(const Vector2f& newPos)
{
    m_pos = newPos;
    // update AABB
    m_aabb = CalcAABB(m_pos.x, m_pos.y);
}

void Player::Draw(u32 layerIndex)
{
    //if(layerIndex != DRAW_LAYER_PLAYERS)
    //    return;
    Render::RenderSprite(s_sprite_default, m_aabb.pos.x * MAP_PIXEL_ZOOM, m_aabb.pos.y * MAP_PIXEL_ZOOM, m_aabb.scale.x * MAP_PIXEL_ZOOM, m_aabb.scale.y * MAP_PIXEL_ZOOM);
    // debug touch icon
    if(m_isTouchingGround)
        Render::RenderSprite(s_sprite_debug_touch, m_aabb.pos.x * MAP_PIXEL_ZOOM, m_aabb.pos.y * MAP_PIXEL_ZOOM, 8, 8);
    // s_sprite_debug_touch
}

Vector2f Player::GetPosition()
{
    return m_pos;
}

void Player::HandleLocalPlayerControl()
{
    Vector2f leftStick = getLeftStick();
    ButtonState& buttonState = GetButtonState();

    // jumping
    if(m_isTouchingGround && buttonState.buttonA.changedState && buttonState.buttonA.isDown)
    {
        Vector2f slightlyAbove = m_pos;
        slightlyAbove.y -= 1.5f;
        if(DoesPlayerCollideAtPos(m_pos.x, m_pos.y) && !DoesPlayerCollideAtPos(slightlyAbove.x, slightlyAbove.y))
            m_pos = slightlyAbove;
        m_speed.y = -1.5f;
        m_isTouchingGround = false;
        // player might be slightly stuck in the ground so move him up by a tiny bit?
    }
    if(!m_isTouchingGround)
    {
        // air movement
        if(leftStick.x < -0.1f)
        {
            if(m_speed.x > -1.0)
                m_speed.x -= 0.2f;
        }
        else if(leftStick.x > 0.1f)
        {
            if(m_speed.x < 1.0)
                m_speed.x += 0.2f;
        }
    }
    else
    {
        // ground movement
        // todo
    }

}

// "down" is +y
// "up" is -y
// top left corner of the level is 0/0
void Player::Update(float timestep)
{
    HandleLocalPlayerControl();
    Map* map = GetCurrentMap();

    // get pos as pixel integer coordinates
    s32 pix = (s32)(m_pos.x + 0.5f);
    s32 piy = (s32)(m_pos.y + 0.5f);

    // apply gravity
    if(!m_isTouchingGround)
        m_speed.y += 0.16f;

    Vector2f newPos = m_pos + m_speed;
    if( !SlidePlayerPos(newPos) )
    {
        m_speed = m_speed * 0.5f; // collision happened, so slow down movement
        // hacky way to detect partial down movement as touching the ground?
    }

    // if we are moving down then check if ground touching flag needs to be set
    if(!m_isTouchingGround && m_speed.y >= 0.0001f)
    {
        if(DoesPlayerCollideAtPos(m_pos.x, m_pos.y + 0.6f))
            m_isTouchingGround = true;
    }
    else if(m_isTouchingGround)
    {
        // if m_isTouchingGround is set then make sure the player "sticks" to the ground
        m_speed.x *= 0.5f;
    }

    //if(!map->GetPixel(pix, piy).IsSolid())
    //{
    //    UpdatePosition(Vector2f(m_pos.x, m_pos.y + 0.1f));
    //}

    // check collision!
}

// move player to new position, stop at collisions
bool Player::SlidePlayerPos(const Vector2f& newPos)
{
    // try moving full range first
    if(!DoesPlayerCollideAtPos(newPos.x, newPos.y))
    {
        UpdatePosition(Vector2f(newPos.x, newPos.y));
        return true;
    }
    // use binary search to find the distance we can actually move
    /*
    Vector2f moveVec = newPos - m_pos;
    Vector2f tryMoveVec = moveVec * 0.5f;
    bool hasClosestTarget = false;
    Vector2f closestTarget;
    for(s32 t=0; t<5; t++)
    {
        Vector2f tmpTarget = m_pos + tryMoveVec;
        if(!DoesPlayerCollideAtPos(tmpTarget.x, tmpTarget.y))
        {
            hasClosestTarget = true;
            closestTarget = tmpTarget;
            // try moving further
            tryMoveVec = tryMoveVec + tryMoveVec * 0.5f;
            continue;
        }
        // try a shorter distance
        tryMoveVec = tryMoveVec * 0.5f;
    }
    if(hasClosestTarget)
    {
        UpdatePosition(closestTarget);
    }
     */

    return false; // only partial move possible
}

bool Player::DoesPlayerCollideAtPos(f32 posX, f32 posY)
{
    Map* map = GetCurrentMap();

    AABB playerAABB = Player::CalcAABB(posX, posY);
    // we need to do geometry-perfect collisions with the pixels
    // so instead of treating pixels as infinitely small points, we treat them as actual solid rectangles
    s32 px1 = (s32)m_pos.x - 1.0f;
    s32 px2 = (s32)m_pos.x + 1.0f + 0.5f;
    s32 py1 = (s32)m_pos.y - 1.0f;
    s32 py2 = (s32)m_pos.y + 1.0f + 0.5f;
    for(s32 py=py1; py<=py2; py++)
    {
        for(s32 px=px1; px<=px2; px++)
        {
            if(!map->GetPixel(px, py).IsSolid())
                continue;
            AABB pixelAABB(px, py, px+1, py + 1);
            if(pixelAABB.Intersects(playerAABB))
                return true;
        }
    }

    /*
    s32 pix = (s32)(m_pos.x + 0.5f);
    s32 piy = (s32)(m_pos.y + 0.5f - 0.1f); // move collision a bit up so we can avoid hovering character sprites
    if(map->GetPixel(pix, piy).IsSolid())
        return true;
        */

    return false;
}
