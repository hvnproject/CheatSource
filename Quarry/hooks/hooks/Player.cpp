#include "..\hooks.hpp"
#include "..\..\cheats\lagcompensation\local_animations.h"
#include "..\..\cheats\misc\prediction_system.h"

_declspec(noinline)bool hooks::hkSetupBones_detour(void* ecx, matrix3x4_t* bone_world_out, int max_bones, int bone_mask, float current_time)
{
	auto result = true;

	static auto r_jiggle_bones = m_cvar()->FindVar(crypt_str("r_jiggle_bones"));
	auto r_jiggle_bones_backup = r_jiggle_bones->GetInt();

	r_jiggle_bones->SetValue(0);

	if (!ecx)
		result = ((SetupBonesFn)original_setupbones)(ecx, bone_world_out, max_bones, bone_mask, current_time);
	else if (!g_cfg.ragebot.enable && !g_cfg.legitbot.enabled)
		result = ((SetupBonesFn)original_setupbones)(ecx, bone_world_out, max_bones, bone_mask, current_time);
	else
	{
		auto player = (player_t*)((uintptr_t)ecx - 0x4);

		if (!player->valid(false, false))
			result = ((SetupBonesFn)original_setupbones)(ecx, bone_world_out, max_bones, bone_mask, current_time);
		else
		{
			auto animstate = player->get_animation_state();
			auto previous_weapon = animstate ? animstate->m_pLastBoneSetupWeapon : nullptr;

			if (previous_weapon)
				animstate->m_pLastBoneSetupWeapon = animstate->m_pActiveWeapon;

			if (g_ctx.globals.setuping_bones)
				result = ((SetupBonesFn)original_setupbones)(ecx, bone_world_out, max_bones, bone_mask, current_time);
			else if (g_cfg.legitbot.enabled && player != g_ctx.local())
				result = ((SetupBonesFn)original_setupbones)(ecx, bone_world_out, max_bones, bone_mask, current_time);
			else if (!g_ctx.local()->is_alive())
				result = ((SetupBonesFn)original_setupbones)(ecx, bone_world_out, max_bones, bone_mask, current_time);
			else if (player == g_ctx.local())
				result = ((SetupBonesFn)original_setupbones)(ecx, bone_world_out, max_bones, bone_mask, current_time);
			else if (!player->m_CachedBoneData().Count()) 
				result = ((SetupBonesFn)original_setupbones)(ecx, bone_world_out, max_bones, bone_mask, current_time);
			else if (bone_world_out && max_bones != -1)
				memcpy(bone_world_out, player->m_CachedBoneData().Base(), player->m_CachedBoneData().Count() * sizeof(matrix3x4_t));

			if (previous_weapon)
				animstate->m_pLastBoneSetupWeapon = previous_weapon;
		}
	}

	r_jiggle_bones->SetValue(r_jiggle_bones_backup);
	return result;
}

bool __fastcall hooks::hkSetupBones(void* ecx, void* edx, matrix3x4_t* bone_world_out, int max_bones, int bone_mask, float current_time)
{
	return hkSetupBones_detour(ecx, bone_world_out, max_bones, bone_mask, current_time);
}

_declspec(noinline)void hooks::hkStandardBlendingRules_detour(player_t* player, int i, CStudioHdr* hdr, Vector* pos, Quaternion* q, float curtime, int boneMask)
{
	// return to original if we are not in game or wait till we find the players.
	if (!player->valid(false, false))
		return ((StandardBlendingRulesFn)original_standardblendingrules)(player, hdr, pos, q, curtime, boneMask);

	// backup effects.
	auto backup_effects = player->m_fEffects();

	// enable extrapolation(disabling interpolation).
	if (!(player->m_fEffects() & 8))
		player->m_fEffects() |= 8;

	((StandardBlendingRulesFn)original_standardblendingrules)(player, hdr, pos, q, curtime, boneMask);

	// disable extrapolation after hooks(enabling interpolation).
	if (player->m_fEffects() & 8)
		player->m_fEffects() = backup_effects;
}

void __fastcall hooks::hkStandardBlendingRules(player_t* player, int i, CStudioHdr* hdr, Vector* pos, Quaternion* q, float curtime, int boneMask)
{
	return hkStandardBlendingRules_detour(player, i, hdr, pos, q, curtime, boneMask);
}

_declspec(noinline)void hooks::hkDoExtraBonesProcessing_detour(player_t* player, CStudioHdr* hdr, Vector* pos, Quaternion* q, const matrix3x4_t& matrix, uint8_t* bone_list, void* context)
{
	// get animstate ptr.
	auto animstate = player->get_animation_state();

	// backup pointer.
	player_t* backup = nullptr;

	// zero out animstate player pointer so CCSGOPlayerAnimState::DoProceduralFootPlant will not do anything.
	if (animstate) {
		// backup player ptr.
		backup = animstate->m_pBaseEntity;

		// null player ptr, GUWOP gang.
		animstate->m_pBaseEntity = nullptr;
	}

	((DoExtraBonesProcessingFn)original_doextrabonesprocessing)(player, hdr, pos, q, matrix, bone_list, context);

	// restore ptr.
	if (animstate && backup)
		animstate->m_pBaseEntity = backup;
}

void __fastcall hooks::hkDoExtraBonesProcessing(player_t* player, void* edx, CStudioHdr* hdr, Vector* pos, Quaternion* q, const matrix3x4_t& matrix, uint8_t* bone_list, void* context)
{
	return hkDoExtraBonesProcessing_detour(player, hdr, pos, q, matrix, bone_list, context);
}

_declspec(noinline)void hooks::hkUpdateClientsideAnimation_detour(player_t* player)
{
	if (g_ctx.globals.updating_animation)
		return ((UpdateClientSideAnimationFn)original_updateclientsideanimation)(player);

	if (player == g_ctx.local())
		return ((UpdateClientSideAnimationFn)original_updateclientsideanimation)(player);
	
	if (!g_cfg.ragebot.enable && !g_cfg.legitbot.enabled)
		return ((UpdateClientSideAnimationFn)original_updateclientsideanimation)(player);

	if (!player->valid(false, false))
		return ((UpdateClientSideAnimationFn)original_updateclientsideanimation)(player);
}

void __fastcall hooks::hkUpdateClientsideAnimation(player_t* player, uint32_t i)
{
	return hkUpdateClientsideAnimation_detour(player);
}

_declspec(noinline)void hooks::hkPhysicssimulate_detour(player_t* player)
{
	auto simulation_tick = *(int*)((uintptr_t)player + 0x2AC);

	if (player != g_ctx.local() || !g_ctx.local()->is_alive() || m_globals()->m_tickcount == simulation_tick)
	{
		((PhysicsSimulateFn)original_physicssimulate)(player);
		return;
	}

	engineprediction::get().restore_netvars();
	((PhysicsSimulateFn)original_physicssimulate)(player);
	engineprediction::get().store_netvars();
}

void __fastcall hooks::hkPhysicssimulate(player_t* player)
{
	return hkPhysicssimulate_detour(player);
}

_declspec(noinline)void hooks::hkModifyEyePosition_detour(c_baseplayeranimationstate* state, Vector& position)
{
	if (state && g_ctx.globals.in_createmove)
		return ((ModifyEyePositionFn)original_modifyeyeposition)(state, position);
}

void __fastcall hooks::hkModifyEyePosition(c_baseplayeranimationstate* state, void* edx, Vector& position)
{
	return hkModifyEyePosition_detour(state, position);
}

_declspec(noinline)void hooks::hkCalcviewmodelbob_detour(player_t* player, Vector& position)
{
	if (!g_cfg.esp.removals[REMOVALS_LANDING_BOB] || player != g_ctx.local() || !g_ctx.local()->is_alive())
		return ((CalcViewmodelBobFn)original_calcviewmodelbob)(player, position);
}

void __fastcall hooks::hkCalcviewmodelbob(player_t* player, void* edx, Vector& position)
{
	return hkCalcviewmodelbob_detour(player, position);
}

bool __fastcall hooks::hkShouldSkipAnimFrame()
{
	return false;
}

_declspec(noinline)void hooks::hkCalcview_detour(void* ecx, Vector& eye_origin, Vector& eye_angles, float& z_near, float& z_far, float& fov)
{
	// don't run the codes if we are not in game or there is no player and if the player is not us.
	if (!ecx || !g_ctx.local()->is_alive() || ecx != g_ctx.local())
		return ((CalcViewFn)original_calcview)(ecx, eye_origin, eye_angles, z_near, z_far, fov);

	// backup data for removals.
	Vector aim_punch = g_ctx.local()->m_aimPunchAngle();
	Vector view_punch = g_ctx.local()->m_viewPunchAngle();

	// apply removals if we enable the settings.
	if (g_cfg.esp.removals[REMOVALS_RECOIL] && g_cfg.player.enable)
	{
		g_ctx.local()->m_aimPunchAngle() = ZERO;
		g_ctx.local()->m_viewPunchAngle() = ZERO;
	}

	((CalcViewFn)original_calcview)(ecx, eye_origin, eye_angles, z_near, z_far, fov);

	// restore data.
	if (g_cfg.esp.removals[REMOVALS_RECOIL] && g_cfg.player.enable)
	{
		g_ctx.local()->m_aimPunchAngle() = aim_punch;
		g_ctx.local()->m_viewPunchAngle() = view_punch;
	}
}

void __fastcall hooks::hkCalcview(void* ecx, void* edx, Vector& eye_origin, Vector& eye_angles, float& z_near, float& z_far, float& fov)
{
	return hkCalcview_detour(ecx, eye_origin, eye_angles, z_near, z_far, fov);
}

int hooks::hkProcessInterpolatedList()
{
	// don't run interpolation.
	static auto allow_extrapolation = *(bool**)(util::FindSignature(crypt_str("client.dll"), crypt_str("A2 ? ? ? ? 8B 45 E8")) + 0x1);

	if (allow_extrapolation)
		*allow_extrapolation = false;

	return ((ProcessInterpolatedListFn)original_processinterpolatedlist)();
}