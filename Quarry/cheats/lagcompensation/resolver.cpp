#include "animation_system.h"
#include "..\ragebot\aim.h"

void resolver::initialize(player_t* e, adjust_data* record, const float& goal_feet_yaw, const float& pitch)
{
    player = e;
    player_record = record;

    original_goal_feet_yaw = math::normalize_yaw(goal_feet_yaw);
    original_pitch = math::normalize_pitch(pitch);
}

float flAngleMod(float flAngle)
{
    return((360.0f / 65536.0f) * ((int32_t)(flAngle * (65536.0f / 360.0f)) & 65530));
}

float GetBackwardYaw(player_t* ent) {
    return math::calculate_angle(g_ctx.local()->GetAbsOrigin(), ent->GetAbsOrigin()).y;
}

float GetForwardYaw(player_t* ent) {
    return math::normalize_yaw(GetBackwardYaw(ent) - 180.f);
}

float ApproachAngle(float target, float value, float speed)
{
    target = flAngleMod(target);
    value = flAngleMod(value);

    float delta = target - value;

    if (speed < 0)
        speed = -speed;

    if (delta < -180)
        delta += 360;
    else if (delta > 180)
        delta -= 360;

    if (delta > speed)
        value += speed;
    else if (delta < -speed)
        value -= speed;
    else
        value = target;

    return value;
}

float AngleDiff(float destAngle, float srcAngle)
{
    float delta;

    delta = fmodf(destAngle - srcAngle, 360.0f);
    if (destAngle > srcAngle)
    {
        if (delta >= 180)
            delta -= 360;
    }
    else
    {
        if (delta <= -180)
            delta += 360;
    }
    return delta;
}

float GetChokedPackets(player_t* e)
{
    float simtime = e->m_flSimulationTime();
    float oldsimtime = e->m_flOldSimulationTime();
    float simdiff = simtime - oldsimtime;

    auto chokedpackets = TIME_TO_TICKS(max(0, simdiff));

    if (chokedpackets >= 1)
        return chokedpackets;

    return 0;
}

void resolver::reset()
{
    player = nullptr;
    player_record = nullptr;

    side = false;
    fake = false;

    original_goal_feet_yaw = 0.0f;
    original_pitch = 0.0f;
}

bool resolver::Side()
{
    AnimationLayer layers[13];
    AnimationLayer moveLayers[3][13];
    AnimationLayer preserver_anim_layers[13];

    auto speed = player->m_vecVelocity().Length2D();

    if (speed > 1.1f)
    {
        if (!layers[11].m_flWeight * 1000.f)
        {
            if ((layers[4].m_flWeight * 1000.f) == (layers[4].m_flWeight * 1000.f))
            {
                float EyeYaw = fabs(layers[6].m_flPlaybackRate - moveLayers[0][6].m_flPlaybackRate);
                float Negative = fabs(layers[6].m_flPlaybackRate - moveLayers[1][6].m_flPlaybackRate);
                float Positive = fabs(layers[6].m_flPlaybackRate - moveLayers[2][6].m_flPlaybackRate);

                if (Positive >= EyeYaw || Positive >= Negative || (Positive * 1000.f))
                {
                    last_anims_update_time = m_globals()->m_realtime;
                    return true;
                }
            }
            else
            {
                last_anims_update_time = m_globals()->m_realtime;
                return false;
            }
        }
    }
    else if (layers[11].m_flWeight == 0.0f && layers[11].m_flCycle == 0.0f)
    {
        auto delta = std::remainder(math::normalize_yaw(player->m_angEyeAngles().y - player->m_flLowerBodyYawTarget()), 360.f) <= 0.f;
        if (2 * delta)
        {
            if (2 * delta == 2)
            {
                return false;
            }
        }
        else
        {
            return true;
        }
    }
}

bool resolver::DesyncDetect()
{
    if (!player->is_alive())
        return false;
    if (!player->m_fFlags() & FL_ONGROUND)
        return false;
    if (player->get_max_desync_delta() < 10)
        return false;
    if (!player->m_iTeamNum() == g_ctx.local()->m_iTeamNum())
        return false;
    if (GetChokedPackets(player) == 0)
        return false;
    if (!player->m_hActiveWeapon().Get()->can_fire(true))
        return false;

    return true;
}

void resolver::shitresolver()
{
    auto animstate = player->get_animation_state();

    static auto GetSmoothedVelocity = [](float min_delta, Vector a, Vector b) {
        Vector delta = a - b;
        float delta_length = delta.Length();

        if (delta_length <= min_delta) {
            Vector result;
            if (-min_delta <= delta_length) {
                return a;
            }
            else {
                float iradius = 1.0f / (delta_length + FLT_EPSILON);
                return b - ((delta * iradius) * min_delta);
            }
        }
        else {
            float iradius = 1.0f / (delta_length + FLT_EPSILON);
            return b + ((delta * iradius) * min_delta);
        }
    };

    float maxMovementSpeed = 260.0f;

    auto weapon = player->m_hActiveWeapon().Get();

    Vector velocity = player->m_vecVelocity();
    Vector animVelocity = GetSmoothedVelocity(m_clientstate()->iChokedCommands * 2000.0f, velocity, player->m_vecVelocity());

    float speed = std::fminf(animVelocity.Length(), 260.0f);
    float runningSpeed = speed / (maxMovementSpeed * 0.520f);
    float duckSpeed = speed / (maxMovementSpeed * 0.340);
    float yawModifer = (((animstate->m_bOnGround * -0.3f) - 0.2f) * runningSpeed) + 1.0f;
    float maxBodyYaw = *(float*)(uintptr_t(animstate) + 0x334) * yawModifer;
    float minBodyYaw = *(float*)(uintptr_t(animstate) + 0x330) * yawModifer;
    float eyeYaw = player->m_angEyeAngles().y;
    float left = eyeYaw - minBodyYaw;
    float right = eyeYaw + maxBodyYaw;
    float eyeDiff = std::remainderf(eyeYaw - original_goal_feet_yaw, 360.f);

    if (eyeDiff <= minBodyYaw)
    {
        if (minBodyYaw > eyeDiff)
            original_goal_feet_yaw = fabs(minBodyYaw) + eyeYaw;
    }
    else
    {
        original_goal_feet_yaw = eyeYaw - fabs(maxBodyYaw);
    }

    original_goal_feet_yaw = std::remainderf(original_goal_feet_yaw, 360.f);

    if (speed > 0.1f || fabs(velocity.z) > 100.0f)
    {
        original_goal_feet_yaw = ApproachAngle(
            eyeYaw,
            original_goal_feet_yaw,
            ((animstate->m_bOnGround * 15.0f) + 30.0f) * m_clientstate()->iChokedCommands
        );
    }
    else
    {
        original_goal_feet_yaw = ApproachAngle(
            player->m_flLowerBodyYawTarget(), original_goal_feet_yaw, m_clientstate()->iChokedCommands * 90.0f);
    }

    if (resolver::Side())
    {
        switch (g_ctx.globals.missed_shots[player->EntIndex()] % 3)
        {
        case 0:
            animstate->m_flGoalFeetYaw = math::normalize_yaw(right);
            break;
        case 1:
            animstate->m_flGoalFeetYaw = math::normalize_yaw(original_goal_feet_yaw);
            break;
        case 2:
            animstate->m_flGoalFeetYaw = math::normalize_yaw(left);
            break;
        }
    }
    else
    {
        switch (g_ctx.globals.missed_shots[player->EntIndex()] % 3)
        {
        case 0:
            animstate->m_flGoalFeetYaw = math::normalize_yaw(left);
            break;
        case 1:
            animstate->m_flGoalFeetYaw = math::normalize_yaw(original_goal_feet_yaw);
            break;
        case 2:
            animstate->m_flGoalFeetYaw = math::normalize_yaw(right);
            break;
        }
    }
}

void resolver::resolve_yaw()
{
    player_info_t player_info;

    if (!player || !player->get_animation_state() || !m_engine()->GetPlayerInfo(player->EntIndex(), &player_info) || player_info.fakeplayer || !resolver::DesyncDetect())
        return;

    if (g_ctx.globals.missed_shots[player->EntIndex()] <= 2)
    {
        resolver::shitresolver();
        return;
    }

    if (!DesyncDetect())
    {
        player_record->side = RESOLVER_ORIGINAL;
        player_record->curMode = NO_MODE;
        player_record->curSide = NO_SIDE;
        return;
    }

    if (player_record->curMode == AIR)
    {
        player_record->side = RESOLVER_ORIGINAL;
        player_record->curMode = AIR;
        player_record->curSide = NO_SIDE;
        return;
    }

    auto animstate = player->get_animation_state();
    float resolveValue;
    int resolveSide = resolver::Side() ? 1 : -1;

    if (player->m_vecVelocity().Length2D() > 1.1f && player->m_vecVelocity().Length2D() < 110.0f)
    {
        switch (g_ctx.globals.missed_shots[player->EntIndex()] % 3)
        {
        case 0:
            resolveValue = 30.0f;
            break;
        case 1:
            resolveValue = 58.0f;
            break;
        case 2:
            resolveValue = 19.0f;
            break;
        }
    }
    else
    {
        if (g_cfg.player_list.low_delta[player->EntIndex()])
        {
            switch (g_ctx.globals.missed_shots[player->EntIndex()] % 3)
            {
            case 0:
                resolveValue = 30.0f;
                break;
            case 1:
                resolveValue = 19.0f;
                break;
            case 2:
                resolveValue = 69.0f;
                break;
            }
        }
        else
        {
            switch (g_ctx.globals.missed_shots[player->EntIndex()] % 3)
            {
            case 0:
                resolveValue = 69.0f;
                break;
            case 1:
                resolveValue = 30.0f;
                break;
            case 2:
                resolveValue = 19.0f;
                break;
            }
        }
    }

    static int side[63];
    auto speed = g_ctx.local()->m_vecVelocity().Length2D();
    int m_Side;
    if (speed <= 0.1f)
    {
        if (player_record->layers[4].m_flWeight == 0.0 && player_record->layers[4].m_flCycle == 0.0)
        {
            side[player->EntIndex()] = 2 * (math::normalize_diff(player->m_angEyeAngles().y, player_record->abs_angles.y) <= 0.0) - 1;
        }
    }
    else
    {
        const float f_delta = abs(player_record->layers[6].m_flPlaybackRate - player_record->left_layers[6].m_flPlaybackRate);
        const float s_delta = abs(player_record->layers[6].m_flPlaybackRate - player_record->center_layers[6].m_flPlaybackRate);
        const float t_delta = abs(player_record->layers[6].m_flPlaybackRate - player_record->right_layers[6].m_flPlaybackRate);

        if (f_delta < s_delta || t_delta <= s_delta || (s_delta * 1800.0))
        {
            if (f_delta >= t_delta && s_delta > t_delta && !(t_delta * 1800.0))
            {
                side[player->EntIndex()] = 1;
            }
        }
        else
        {
            side[player->EntIndex()] = -1;
        }
    }

    if (m_Side == 1)
    {
        animstate->m_flGoalFeetYaw = math::normalize_yaw(player->m_angEyeAngles().y + 58.f);
    }
    else if (m_Side == -1)
    {
        animstate->m_flGoalFeetYaw = math::normalize_yaw(player->m_angEyeAngles().y - 58.f);
    }

    if (player->GetAbsOrigin().y == GetForwardYaw(player))
        side[player->EntIndex()] *= -1;

    if (resolveValue > player->get_max_desync_delta())
        resolveValue = player->get_max_desync_delta();

    player->get_animation_state()->m_flGoalFeetYaw = math::normalize_yaw(player->m_angEyeAngles().y + resolveValue * resolveSide);

    float flRawYawIdeal = (math::angle_diff(-player->m_vecAbsVelocity().y, -player->m_vecAbsVelocity().x) * 180 / M_PI);
    if (flRawYawIdeal < 0)
        flRawYawIdeal += 360;
}

float resolver::resolve_pitch()
{
    return original_pitch;
}