#include "Player.h"
#include "../framework/render.h"
#include "Map.h"
#include "GameClient.h"

#include "GameSceneMenu.h"
#include "../framework/audio.h"
#include "MapPixels.h"

Sprite* s_tankBodySprite{nullptr};
Sprite* s_tankDrill0Sprite{nullptr};
Sprite* s_tankDrill1Sprite{nullptr};
Sprite* s_tankDrill2Sprite{nullptr};
Sprite* s_tankWheelSprite{nullptr};

Player::Player(GameScene* parent, u32 playerId, f32 posX, f32 posY) : Object(parent, CalcAABB(posX, posY), true, DRAW_LAYER_PLAYERS), m_playerId(playerId)
{
    m_moveFlags.isDrilling = false;
    m_moveFlags.walkingLeft = false;
    m_moveFlags.walkingRight = false;
    Vector2f playerPos = Vector2f(posX, posY - GetPlayerHeight());
    UpdatePosition(playerPos);
    if(!s_tankBodySprite)
    {
        s_tankBodySprite = new Sprite("/tex/tank.tga", true);
        s_tankDrill0Sprite = new Sprite("/tex/tank_drill0.tga", true);
        s_tankDrill1Sprite = new Sprite("/tex/tank_drill1.tga", true);
        s_tankDrill2Sprite = new Sprite("/tex/tank_drill2.tga", true);
        s_tankWheelSprite = new Sprite("/tex/tank_wheel.tga", true);
    }

    m_teleportAudio = new Audio("/sfx/teleport.wav");
    m_deathAudio = new Audio("/sfx/death_explosion.wav");
    m_hitAudio = new Audio("/sfx/hit.wav");
    m_drillStartAudio = new Audio("/sfx/drill_start.wav");
    m_drillMoveAudio = new Audio("/sfx/drill_loop.wav");
    m_drillStopAudio = new Audio("/sfx/drill_stop.wav");
}

Player::~Player()
{
    m_teleportAudio->QueueDestroy();
    m_deathAudio->QueueDestroy();
    m_hitAudio->QueueDestroy();
    m_drillMoveAudio->SetLooping(false);
    m_drillStartAudio->QueueDestroy();
    m_drillMoveAudio->QueueDestroy();
    m_drillStopAudio->QueueDestroy();
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

void Player::SyncMovement(Vector2f pos, Vector2f speed, u8 moveFlags, f32 drillAngle)
{
    m_pos = pos;
    m_speed = speed;
    m_isTouchingGround = false; // calculating this properly requires more sophisticated logic
    m_moveFlags.rawBits = moveFlags;
    m_drillAngle = drillAngle;
}

void Player::Draw(u32 layerIndex) {
    if (this->IsSpectating()) {
        return;
    }

    if (!this->IsInvincible() || (OSTicksToSeconds(OSGetTime()-this->m_invincibility)%2 == 0)) {
        // draw body
        Render::RenderSprite(s_tankBodySprite, m_aabb.pos.x * MAP_PIXEL_ZOOM, m_aabb.pos.y * MAP_PIXEL_ZOOM, s_tankBodySprite->GetWidth(), s_tankBodySprite->GetHeight());

        // draw wheels
        Render::RenderSprite(s_tankWheelSprite, (m_aabb.pos.x + 5.9f) * MAP_PIXEL_ZOOM, (m_aabb.pos.y * MAP_PIXEL_ZOOM) + ((m_aabb.scale.y * MAP_PIXEL_ZOOM) - s_tankWheelSprite->GetHeight()) + 11.0f, s_tankWheelSprite->GetWidth(), s_tankWheelSprite->GetHeight(), m_moveAnimRot);
        Render::RenderSprite(s_tankWheelSprite, (m_aabb.pos.x + 21.6f) * MAP_PIXEL_ZOOM, (m_aabb.pos.y * MAP_PIXEL_ZOOM) + ((m_aabb.scale.y * MAP_PIXEL_ZOOM) - s_tankWheelSprite->GetHeight()) + 11.0f, s_tankWheelSprite->GetWidth(), s_tankWheelSprite->GetHeight(), m_moveAnimRot);

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
    }
}

Vector2f Player::GetPosition()
{
    return m_pos;
}

void Player::HandleLocalPlayerControl_WalkMode(ButtonState& buttonState, Vector2f leftStick)
{
    // bomb placing (temporary)
    if (buttonState.buttonB.changedState && buttonState.buttonB.isDown) {
        if (m_ability == GameClient::GAME_ABILITY::LANDMINE) {
            Vector2f slightlyAbove = m_pos;
            slightlyAbove.y -= 20.0f;

            m_parent->GetClient()->SendAbility(GameClient::GAME_ABILITY::LANDMINE, {slightlyAbove.x, slightlyAbove.y}, {0.0f, 0.0f});
            GiveAbility(m_parent->IsDebugEnabled() && m_parent->IsSingleplayer() ? GameClient::GAME_ABILITY::LANDMINE : GameClient::GAME_ABILITY::NONE);
        }
        else if (m_ability == GameClient::GAME_ABILITY::MISSILE) {
            Vector2f shootPosition = m_parent->GetPlayer()->GetPosition() + Vector2f(0.0f, -15.0f) + (Vector2f(1.0f, 0.0f).Rotate(m_visualDrillAngle).GetNormalized()*20.0f);
            Vector2f shootDirection = Vector2f(1.0f, 0.0f).Rotate(m_visualDrillAngle).GetNormalized()*6.0f;
            m_parent->GetClient()->SendAbility(GameClient::GAME_ABILITY::MISSILE, shootPosition, shootDirection);
            GiveAbility(m_parent->IsDebugEnabled() && m_parent->IsSingleplayer() ? GameClient::GAME_ABILITY::MISSILE : GameClient::GAME_ABILITY::NONE);
        }
        else if (m_ability == GameClient::GAME_ABILITY::TURBO_DRILL) {
            m_turboBoost = OSGetTime() + OSSecondsToTicks(25);
            GiveAbility(
                    m_parent->IsDebugEnabled() && m_parent->IsSingleplayer() ? GameClient::GAME_ABILITY::TURBO_DRILL : GameClient::GAME_ABILITY::NONE);
        }
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
    m_moveFlags.walkingLeft = false;
    m_moveFlags.walkingRight = false;
    if(!m_isTouchingGround)
    {
        // air movement
        if(leftStick.x < -0.1f)
        {
            m_moveFlags.walkingLeft = true;
        }
        else if(leftStick.x > 0.1f)
        {
            m_moveFlags.walkingRight = true;
        }
    }
    else
    {
        // ground movement
        if(leftStick.x < -0.1f)
        {
            m_moveFlags.walkingLeft = true;
        }
        else if(leftStick.x > 0.1f)
        {
            m_moveFlags.walkingRight = true;
        }
    }

    // drill arm can be controlled freely when not drilling
    if( leftStick.Length() > 0.1f)
        m_drillAngle = atan2(leftStick.x, leftStick.y) - M_PI_2;
}

void Player::HandleLocalPlayerControl_DrillMode(float timestep, struct ButtonState &buttonState, Vector2f leftStick)
{
    m_drillingDur += timestep;
    // drill arm can be controlled freely when not drilling
    if( leftStick.Length() > 0.1f)
    {
        f32 targetAngle = atan2(leftStick.x, leftStick.y) - M_PI_2;
        m_drillAngle = _MoveAngleTowardsTarget(m_drillAngle, targetAngle, 0.017f);
    }
}

void Player::HandleLocalPlayerControl_SpectatingMode(struct ButtonState& buttonState, Vector2f leftStick) {
    std::vector<PlayerID> alivePlayers = {};
    for (auto& player : this->m_parent->GetPlayers()) {
        if (!player.second->IsSpectating())
            alivePlayers.emplace_back(player.first);
    }

    if (buttonState.buttonA.changedState && buttonState.buttonA.isDown && !alivePlayers.empty()) {
        if (m_spectatingPlayerIdx < alivePlayers.size()-1)
            m_spectatingPlayerIdx++;
        else
            m_spectatingPlayerIdx = 0;
    }
    if (!alivePlayers.empty())
        m_spectatingPlayer = m_parent->GetPlayerById(alivePlayers[m_spectatingPlayerIdx]);
    else {
        m_spectatingPlayer = nullptr;
        m_spectatingPlayerIdx = 0;
    }
}

void Player::HandleLocalPlayerControl()
{
    g_debugStrings.emplace_back("Current Item: "+std::to_string((int)m_ability));

    ButtonState& buttonState = GetButtonState();
    Vector2f leftStick = getLeftStick();

    if (!this->IsSpectating()) {
        if (buttonState.buttonA.isDown && CheckCanDrill())
            m_moveFlags.isDrilling = true;
        else {
            m_moveFlags.isDrilling = false; // probably should have a tiny cooldown?
            m_drillingDur = 0.0f;
        }

        if (!m_moveFlags.isDrilling)
            HandleLocalPlayerControl_WalkMode(buttonState, leftStick);
        else
            HandleLocalPlayerControl_DrillMode(1000.0f/60.0f, buttonState, leftStick);
    }
    else {
        m_moveFlags.walkingLeft = false;
        m_moveFlags.walkingRight = false;
        m_moveFlags.isDrilling = false;
        m_drillingDur = 0.0f;
        HandleLocalPlayerControl_SpectatingMode(buttonState, leftStick);
    }
}

// "down" is +y
// "up" is -y
// top left corner of the level is 0/0
void Player::Update(float timestep)
{
    Map* map = m_parent->GetMap();

    if (IsSpectating()) {
        Update_SpectatingMode(timestep);
        return;
    }

    Update_MovementSFX(timestep);

    if(m_moveFlags.isDrilling)
    {
        Update_DrillMode(timestep);
        return;
    }

    // left/right move
    if(!m_isTouchingGround)
    {
        // air movement
        if(m_moveFlags.walkingLeft)
        {
            if(m_speed.x > -1.0)
                m_speed.x -= 0.2f;
        }
        else if(m_moveFlags.walkingRight)
        {
            if(m_speed.x < 1.0)
                m_speed.x += 0.2f;
        }
    }
    else
    {
        // ground movement
        if(m_moveFlags.walkingLeft)
        {
            m_moveAnimRot -= M_PI_4/10.0f;
            if(m_speed.x > -0.4)
                m_speed.x -= (IsTurboBoosting() ? 0.4f : 0.15f);

            if (m_moveAnimRot <= 0.01)
                m_moveAnimRot = M_TWOPI;
        }
        else if(m_moveFlags.walkingRight)
        {
            m_moveAnimRot += M_PI_4/10.0f;
            if(m_speed.x < 0.4)
                m_speed.x += (IsTurboBoosting() ? 0.4f : 0.15f);
            if (m_moveAnimRot >= M_TWOPI)
                m_moveAnimRot = 0.0;
        }
    }


    // apply gravity
    if(!m_isTouchingGround)
        m_speed.y += 0.16f;

    Vector2f newPos = m_pos + m_speed;
    if( !SlidePlayerPos(map, newPos) )
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

void Player::Update_MovementSFX(float timestep) {
    if (m_drillAudioState == DrillAudioState::STOPPING && m_drillStopAudio->GetState() == Audio::StateEnum::FINISHED) {
        m_drillAudioState = DrillAudioState::STOPPED;
        // OSReport("Changing DrillAudioState to STOPPED\n");
    }
    if (m_drillAudioState == DrillAudioState::STARTING && m_drillStartAudio->GetState() == Audio::StateEnum::FINISHED) {
        m_drillAudioState = DrillAudioState::STARTED;
        // OSReport("Changing DrillAudioState to STARTED\n");
    }

    if (m_moveFlags.isDrilling) {
        m_drillMoveAudio->SetLooping(true);
        // start drill audio
        if (m_drillAudioState == DrillAudioState::STOPPED) {
            m_drillAudioState = DrillAudioState::STARTING;
            m_drillStartAudio->Play();
            // OSReport("Drill start audio\n");
        }
            // start moving loop when drill has started
        else if (m_drillAudioState == DrillAudioState::STARTED) {
            m_drillAudioState = DrillAudioState::MOVING;
            m_drillMoveAudio->Play();
            // OSReport("Drill move audio\n");
        }
            // continue moving loop if drill is already moving
        else if (m_drillAudioState == DrillAudioState::MOVING && m_drillMoveAudio->GetState() != Audio::StateEnum::PLAYING) {
            m_drillMoveAudio->Play();
            // OSReport("Continuing drill move audio\n");
        }
            // continue moving loop again if drill temporarily stopped
        else if (m_drillAudioState == DrillAudioState::STOPPING && m_drillStopAudio->GetState() != Audio::StateEnum::FINISHED) {
            m_drillAudioState = DrillAudioState::MOVING;
            m_drillMoveAudio->Play();
            // Reset drill stopping audio to beginning
            m_drillStopAudio->Reset();
            m_drillStopAudio->Pause();
            //OSReport("Doing dirty restart of move loop after stopping audio got interrupted\n");
        }
    }
    else {
        m_drillMoveAudio->SetLooping(false);
        // stop drill audio if player was moving
        if (m_drillAudioState == DrillAudioState::MOVING && m_drillMoveAudio->GetState() != Audio::StateEnum::PLAYING) {
            m_drillAudioState = DrillAudioState::STOPPING;
            m_drillStopAudio->Play();
            // OSReport("Drill stop audio from moving state\n");
        }
        // stop drill audio if player was starting
        if (m_drillAudioState == DrillAudioState::STARTING && m_drillStopAudio->GetState() != Audio::StateEnum::PLAYING) {
            m_drillAudioState = DrillAudioState::STOPPING;
            m_drillStopAudio->Play();
            // OSReport("Drill stop audio from starting state\n");
        }
    }

    // change volume depending on distance
    float distance = (m_parent->GetPlayer()->m_pos.Distance(this->m_pos)+0.00000001f)/10.0f;
    float volume = 30.0f - (distance/100.0f*7.0f);

    m_drillStartAudio->SetVolume((uint32_t)volume);
    m_drillMoveAudio->SetVolume((uint32_t)volume);
    m_drillStopAudio->SetVolume((uint32_t)volume);
}

void Player::Update_DrillMode(float timestep)
{
    m_speed = Vector2f((IsTurboBoosting() ? 1.0f : 0.4f), 0.0f).Rotate(m_drillAngle);
    Vector2f newPos = m_pos + m_speed;

    Map* map = m_parent->GetMap();
    newPos.x = std::clamp(newPos.x, 8.0f, (f32)map->GetPixelWidth() - 8.0f);
    newPos.y = std::clamp(newPos.y, 8.0f, (f32)map->GetPixelHeight() - 8.0f);

    UpdatePosition(Vector2f(newPos.x, newPos.y));

    if(!DoesPlayerCollideAtPos(newPos.x, newPos.y + 2.0f))
    {
        // falling!
        m_speed.x = m_speed.x * 0.9f;
        m_speed.y += 0.16f;
    }
    else
        m_speed = m_speed * 0.9f;

    m_drillAnimIdx = m_drillAnimIdx > 45 ? 0 : m_drillAnimIdx + (IsTurboBoosting() ? 3 : 1);
}

void Player::Update_SpectatingMode(float timestep)
{
    if (m_spectatingPlayer != nullptr)
        UpdatePosition(m_spectatingPlayer->m_pos);
}

// move player to new position, stop at collisions
bool Player::SlidePlayerPos(Map* map, Vector2f newPos)
{
    newPos.x = std::clamp(newPos.x, 8.0f, (f32)map->GetPixelWidth() - 8.0f);
    newPos.y = std::clamp(newPos.y, 8.0f, (f32)map->GetPixelHeight() - 8.0f);

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

bool Player::CheckCanDrill() {
    // calculate drill head size
    Vector2f playerCenter(m_aabb.pos.x + m_aabb.scale.x * 0.5f, m_aabb.pos.y + m_aabb.scale.y * 0.58f);
    float toBeDrilledAngle = _InterpolateAngle(m_visualDrillAngle, m_drillAngle, 0.2f);
    Vector2f drillPosLeft = playerCenter + (Vector2f(1.0f, 1.5f).Rotate(toBeDrilledAngle).GetNormalized()*14.0f);
    Vector2f drillPosCenter = playerCenter + (Vector2f(1.0f, 0.0f).Rotate(toBeDrilledAngle).GetNormalized()*14.0f);
    Vector2f drillPosRight = playerCenter + (Vector2f(1.0f, -1.5f).Rotate(toBeDrilledAngle).GetNormalized()*14.0f);

    Map* map = this->m_parent->GetMap();
    if (map->IsPixelOOB(drillPosLeft.x, drillPosLeft.y) || map->IsPixelOOB(drillPosCenter.x, drillPosCenter.y) || map->IsPixelOOB(drillPosRight.x, drillPosRight.y)) {
        return false;
    }
    if (!map->GetPixelNoBoundsCheck(drillPosLeft.x, drillPosLeft.y).IsSolid() && !map->GetPixelNoBoundsCheck(drillPosCenter.x, drillPosCenter.y).IsSolid() && !map->GetPixelNoBoundsCheck(drillPosRight.x, drillPosRight.y).IsSolid()) {
        return false;
    }
    return true;
}

bool Player::DoesPlayerCollideAtPos(f32 posX, f32 posY)
{
    Map* map = this->m_parent->GetMap();

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
            if(!map->DoesPixelCollideWithObject(px, py))
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

void Player::ChangeToSpectator() {
    m_health = 0;
    m_spectating = true;
}

u32 Player::TakeDamage(u8 damage) {
    if (!this->IsInvincible()) {
        if (m_health > damage) {
            m_health -= damage;
            if (m_health != 0) {
                m_hitAudio->Play();
            }
        }
        else
            m_health = 0;

        if (m_health == 0) {
            m_deathAudio->Play();
            m_parent->GetClient()->SendAbility(GameClient::GAME_ABILITY::DEATH, GetPosition(), Vector2f(0.0f, 0.0f));
            m_parent->GetClient()->SendSyncedEvent(GameClient::SynchronizedEvent::EVENT_TYPE::EXPLOSION, GetPosition(), 40.0f, 0.0f);
        }
        m_invincibility = OSGetTime() + OSSecondsToTicks(8);
    }
    return m_health;
}