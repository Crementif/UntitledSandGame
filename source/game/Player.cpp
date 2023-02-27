#include "Player.h"
#include "../framework/render.h"
#include "Map.h"
#include "GameClient.h"

#include "../framework/navigation.h"
#include "Landmine.h"

Sprite* s_tankBodySprite{nullptr};
Sprite* s_tankDrill0Sprite{nullptr};
Sprite* s_tankDrill1Sprite{nullptr};
Sprite* s_tankDrill2Sprite{nullptr};
Sprite* s_tankWheelSprite{nullptr};

Player::Player(GameScene* parent, u32 playerId, f32 posX, f32 posY) : Object(parent, CalcAABB(posX, posY), true, DRAW_LAYER_PLAYERS), m_playerId(playerId)
{
    Vector2f playerPos = Vector2f(posX, posY - GetPlayerHeight());
    UpdatePosition(playerPos);
    // player_default.tga
    if(!s_tankBodySprite)
    {
        s_tankBodySprite = new Sprite("tex/tank.tga", true);
        s_tankDrill0Sprite = new Sprite("tex/tank_drill0.tga", true);
        s_tankDrill1Sprite = new Sprite("tex/tank_drill1.tga", true);
        s_tankDrill2Sprite = new Sprite("tex/tank_drill2.tga", true);
        s_tankWheelSprite = new Sprite("tex/tank_wheel.tga", true);
    }
}

Player::~Player()
{
}

// width/height in world pixels
s32 Player::GetPlayerWidth() const
{
    return 28;
}

s32 Player::GetPlayerHeight() const
{
    return 28;
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

void Player::SyncMovement(Vector2f pos, Vector2f speed, bool isDrilling, f32 drillAngle)
{
    m_pos = pos;
    m_speed = speed;
    m_isTouchingGround = false; // calculating this properly requires more sophisticated logic
    m_isDrilling = isDrilling;
    m_drillAngle = drillAngle;
}

void Player::Draw(u32 layerIndex)
{
    // draw body
    Render::RenderSprite(s_tankBodySprite, m_aabb.pos.x * MAP_PIXEL_ZOOM, m_aabb.pos.y * MAP_PIXEL_ZOOM, s_tankBodySprite->GetWidth(), s_tankBodySprite->GetHeight());

    // draw drill
    Vector2f playerCenter(m_aabb.pos.x + m_aabb.scale.x * 0.5f, m_aabb.pos.y + m_aabb.scale.y * 0.58f);
    m_visualDrillAngle = _InterpolateAngle(m_visualDrillAngle, m_drillAngle, 0.2f);
    Vector2f drillPos = playerCenter + Vector2f(12.0f, 0.0).Rotate(m_visualDrillAngle);

    if (m_drillAnimIdx > 30)
        Render::RenderSprite(s_tankDrill2Sprite, drillPos.x * MAP_PIXEL_ZOOM, drillPos.y * MAP_PIXEL_ZOOM, s_tankDrill2Sprite->GetWidth(), s_tankDrill2Sprite->GetHeight(), m_visualDrillAngle);
    else if (m_drillAnimIdx > 15)
        Render::RenderSprite(s_tankDrill1Sprite, drillPos.x * MAP_PIXEL_ZOOM, drillPos.y * MAP_PIXEL_ZOOM, s_tankDrill1Sprite->GetWidth(), s_tankDrill1Sprite->GetHeight(), m_visualDrillAngle);
    else
        Render::RenderSprite(s_tankDrill0Sprite, drillPos.x * MAP_PIXEL_ZOOM, drillPos.y * MAP_PIXEL_ZOOM, s_tankDrill0Sprite->GetWidth(), s_tankDrill0Sprite->GetHeight(), m_visualDrillAngle);

    // draw wheels
    Render::RenderSprite(s_tankWheelSprite, (m_aabb.pos.x+3.5f) * MAP_PIXEL_ZOOM, (m_aabb.pos.y * MAP_PIXEL_ZOOM) + ((m_aabb.scale.y * MAP_PIXEL_ZOOM)-s_tankWheelSprite->GetHeight()), s_tankWheelSprite->GetWidth(), s_tankWheelSprite->GetHeight(), m_moveAnimRot);
    Render::RenderSprite(s_tankWheelSprite, (m_aabb.pos.x+19.2f) * MAP_PIXEL_ZOOM, (m_aabb.pos.y * MAP_PIXEL_ZOOM) + ((m_aabb.scale.y * MAP_PIXEL_ZOOM)-s_tankWheelSprite->GetHeight()), s_tankWheelSprite->GetWidth(), s_tankWheelSprite->GetHeight(), m_moveAnimRot);
}

Vector2f Player::GetPosition()
{
    return m_pos;
}

void Player::HandleLocalPlayerControl_WalkMode(ButtonState& buttonState, Vector2f leftStick)
{
    // bomb placing (temporary)
    if (buttonState.buttonB.changedState && buttonState.buttonB.isDown) {
        Vector2f slightlyAbove = m_pos;
        slightlyAbove.y -= 60.0f;

        m_parent->GetClient()->SendAbility(GameClient::GAME_ABILITY::LANDMINE, {slightlyAbove.x, slightlyAbove.y}, {0.0f, 0.0f});
        //newLandmine->AddVelocity(0.5f, -1.0f);
    }

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
        if(leftStick.x < -0.1f)
        {
            m_moveAnimRot -= M_PI_4/10.0f;
            if(m_speed.x > -0.4)
                m_speed.x -= 0.15f;

            if (m_moveAnimRot <= 0.01)
                m_moveAnimRot = M_TWOPI;
        }
        else if(leftStick.x > 0.1f)
        {
            m_moveAnimRot += M_PI_4/10.0f;
            if(m_speed.x < 0.4)
                m_speed.x += 0.15f;
            if (m_moveAnimRot >= M_TWOPI)
                m_moveAnimRot = 0.0;
        }
    }

    // drill arm can be controlled freely when not drilling
    if( leftStick.Length() > 0.1f)
        m_drillAngle = atan2(leftStick.x, leftStick.y) - M_PI_2;
}

void Player::HandleLocalPlayerControl_DrillMode(struct ButtonState& buttonState, Vector2f leftStick)
{
    m_drillingDur += 1000.0f / 60.0f;
    //m_speed = m_speed + Vector2f(1.0f, 0.0f).Rotate(m_drillAngle) * 0.01f;

    // drill arm can be controlled freely when not drilling
    if( leftStick.Length() > 0.1f)
    {
        f32 targetAngle = atan2(leftStick.x, leftStick.y) - M_PI_2;
        m_drillAngle = _MoveAngleTowardsTarget(m_drillAngle, targetAngle, 0.017f);
    }

    m_drillAnimIdx = m_drillAnimIdx > 45 ? 0 : m_drillAnimIdx + 1;
}

void Player::HandleLocalPlayerControl()
{
    ButtonState& buttonState = GetButtonState();
    Vector2f leftStick = getLeftStick();

    if(buttonState.buttonA.isDown)
        m_isDrilling = true;
    else
    {
        m_isDrilling = false; // probably should have a tiny cooldown?
        m_drillingDur = 0.0f;
    }

    if(!m_isDrilling)
        HandleLocalPlayerControl_WalkMode(buttonState, leftStick);
    else
        HandleLocalPlayerControl_DrillMode(buttonState, leftStick);
}

// "down" is +y
// "up" is -y
// top left corner of the level is 0/0
void Player::Update(float timestep)
{
    Map* map = GetCurrentMap();

    if(m_isDrilling)
    {
        Update_DrillMode(timestep);
        return;
    }

    // get pos as pixel integer coordinates
    //s32 pix = (s32)(m_pos.x + 0.5f);
    //s32 piy = (s32)(m_pos.y + 0.5f);

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
        m_speed.x *= 0.90f;
        m_speed.y = 0.0f;

        bool isTouchingGround = false;
        f32 groundHeight;
        bool isStuckInGround, isFloatingInAir;
        if( FindAdjustedGroundHeight(m_pos.x, m_pos.y, groundHeight, isStuckInGround, isFloatingInAir) )
        {
            m_pos.y = groundHeight;
            isTouchingGround = true;
        }
        else if(isFloatingInAir)
        {
            // back to falling mode
            m_isTouchingGround = false;
        }

        if(m_isTouchingGround && isTouchingGround)
        {
            // left/ride sliding and walking
            f32 walkedPosX = m_pos.x + m_speed.x;
            if(FindAdjustedGroundHeight(walkedPosX, m_pos.y, groundHeight, isStuckInGround, isFloatingInAir))
            {
                m_pos.x = walkedPosX;
                m_pos.y = groundHeight;
            }

        }
    }
}

void Player::Update_DrillMode(float timestep)
{
    m_speed = Vector2f(0.4f, 0.0f).Rotate(m_drillAngle);
    Vector2f newPos = m_pos + m_speed;
    UpdatePosition(Vector2f(newPos.x, newPos.y));

    if(!DoesPlayerCollideAtPos(newPos.x, newPos.y + 2.0f))
    {
        // falling!
        m_speed.x = m_speed.x * 0.9f;
        m_speed.y += 0.16f;
    }
    else
        m_speed = m_speed * 0.9f;
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

    return false; // only partial move possible
}

bool Player::DoesPlayerCollideAtPos(f32 posX, f32 posY)
{
    Map* map = GetCurrentMap();

    AABB playerAABB = Player::CalcAABB(posX, posY);
    // we need to do geometry-perfect collisions with the pixels
    // so instead of treating pixels as infinitely small points, we treat them as actual solid rectangles
    s32 px1 = (s32)m_pos.x - 4.0f;
    s32 px2 = (s32)m_pos.x + 4.0f + 0.5f;
    s32 py1 = (s32)m_pos.y - 4.0f;
    s32 py2 = (s32)m_pos.y + 4.0f + 0.5f;
    for(s32 py=py1; py<=py2; py++)
    {
        for(s32 px=px1; px<=px2; px++)
        {
            if(!map->GetPixel(px, py).IsCollideWithObjects())
                continue;
            AABB pixelAABB(px, py, px+1, py + 1);
            if(pixelAABB.Intersects(playerAABB))
                return true;
        }
    }
    return false;
}

// find the actual player y position based on the current surrounding terrain
// only seeks within a very small y variance, otherwise we assume player is stuck or floating in the air
bool Player::FindAdjustedGroundHeight(f32 posX, f32 posY, f32& groundHeight, bool& isStuckInGround, bool& isFloatingInAir)
{
    isStuckInGround = false;
    isFloatingInAir = false;
    posY = floor(posY);
    f32 testedPosY;
    // free fall check
    if(!DoesPlayerCollideAtPos(posX, posY + 2.1f))
    {
        isFloatingInAir = true;
        return false;
    }
    // check for heights that we can clip to
    bool collidesAtAllPoints = false;
    for(f32 bias=2.0f; bias >= -2.1f; bias -= 1.0f)
    {
        testedPosY = posY + bias;
        if(!DoesPlayerCollideAtPos(posX, testedPosY))
        {
            groundHeight = testedPosY;
            return true;
        }
    }
    // all collision checks are true so we are stuck inside the ground
    isStuckInGround = true;
    return false;
}