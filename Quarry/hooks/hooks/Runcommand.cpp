#include "..\hooks.hpp"
#include "..\..\cheats\misc\prediction_system.h"
#include "..\..\cheats\lagcompensation\local_animations.h"
#include "..\..\cheats\misc\misc.h"
#include "..\..\cheats\misc\logs.h"

using RunCommand_t = void(__thiscall*)(void*, player_t*, CUserCmd*, IMoveHelper*);
using WriteUsercmdDeltaToBuffer_t = bool(__thiscall*)(void*, int, void*, int, int, bool);

void FixRevolver(CUserCmd* cmd, player_t* player)
{
	if (!g_ctx.local()->is_alive())
		return;

	if (g_cfg.ragebot.enable)
	{
		auto weapon = player->m_hActiveWeapon().Get();

		if (weapon->m_iItemDefinitionIndex() == WEAPON_REVOLVER)
		{
			static float m_nTickBase[MULTIPLAYER_BACKUP];
			static bool m_nShotFired[MULTIPLAYER_BACKUP];
			static bool m_nCanFire[MULTIPLAYER_BACKUP];

			m_nTickBase[cmd->m_command_number % MULTIPLAYER_BACKUP] = player->m_nTickBase();
			m_nShotFired[cmd->m_command_number % MULTIPLAYER_BACKUP] = cmd->m_buttons & IN_ATTACK || cmd->m_buttons & IN_ATTACK2;
			m_nCanFire[cmd->m_command_number % MULTIPLAYER_BACKUP] = weapon->can_fire(false);

			int nLowestCommand = cmd->m_command_number - (int(1.0f / m_globals()->m_intervalpertick) >> 1);
			int nCheckCommand = cmd->m_command_number - 1;
			int nFireCommand = 0;

			while (nCheckCommand > nLowestCommand)
			{
				nFireCommand = nCheckCommand;

				if (!m_nShotFired[nCheckCommand % MULTIPLAYER_BACKUP])
					break;

				if (!m_nCanFire[nCheckCommand % MULTIPLAYER_BACKUP])
					break;

				nCheckCommand--;
			}

			if (nFireCommand)
			{
				int nOffset = 1 - (-0.03348f / m_globals()->m_intervalpertick);

				if (weapon->m_iItemDefinitionIndex() == WEAPON_REVOLVER)
				{
					if (cmd->m_command_number - nFireCommand >= nOffset)
						weapon->m_flPostponeFireReadyTime() = TICKS_TO_TIME(m_nTickBase[(nFireCommand + nOffset) % MULTIPLAYER_BACKUP]) + 0.2f;
				}
			}
		}
	}
}

void FixAttackPacket(CUserCmd* m_pCmd, bool m_bPredict)
{
	static bool m_bLastAttack = false;
	static bool m_bInvalidCycle = false;
	static float m_flLastCycle = 0.f;

	if (!g_ctx.local())
		return;

	if (m_bPredict)
	{
		m_bLastAttack = m_pCmd->m_weaponselect || (m_pCmd->m_buttons & IN_ATTACK);
		m_flLastCycle = g_ctx.local()->m_flCycle();
	}
	else if (m_bLastAttack && !m_bInvalidCycle)
		m_bInvalidCycle = g_ctx.local()->m_flCycle() == 0.f && m_flLastCycle > 0.f;

	if (m_bInvalidCycle)
		g_ctx.local()->m_flCycle() = m_flLastCycle;
}

void __fastcall hooks::hkRunCommand(void* ecx, void* edx, player_t* player, CUserCmd* m_pcmd, IMoveHelper* move_helper)
{
	static auto original_fn = prediction_hook->get_func_address <RunCommand_t>(19);
	g_ctx.local((player_t*)m_entitylist()->GetClientEntity(m_engine()->GetLocalPlayer()), true);

	if (!player || player != g_ctx.local())
		return original_fn(ecx, player, m_pcmd, move_helper);

	if (!m_pcmd)
		return original_fn(ecx, player, m_pcmd, move_helper);

	if (m_pcmd->m_tickcount > m_globals()->m_tickcount + 72)
	{
		m_pcmd->m_predicted = true;
		player->set_abs_origin(player->m_vecOrigin());

		if (m_globals()->m_frametime > 0.0f && !m_prediction()->EnginePaused)
			++player->m_nTickBase();

		return;
	}

	if (g_cfg.ragebot.enable && player->is_alive())
	{
		auto backup_velocity_modifier = player->m_flVelocityModifier();

		FixAttackPacket(m_pcmd, true);

		if (g_ctx.globals.in_createmove && m_pcmd->m_command_number == m_clientstate()->nLastCommandAck + 1)
			player->m_flVelocityModifier() = g_ctx.globals.last_velocity_modifier;

		if (player->m_hActiveWeapon().Get())
			FixRevolver(m_pcmd, player);

		original_fn(ecx, player, m_pcmd, move_helper);

		if (player->m_hActiveWeapon().Get())
			FixRevolver(m_pcmd, player);

		if (!g_ctx.globals.in_createmove)
			player->m_flVelocityModifier() = backup_velocity_modifier;

		FixAttackPacket(m_pcmd, false);
	}
	else
		return original_fn(ecx, player, m_pcmd, move_helper);
}

using InPrediction_t = bool(__thiscall*)(void*);

bool __stdcall hooks::hkInPrediction()
{
	static auto original_fn = prediction_hook->get_func_address <InPrediction_t>(14);
	return original_fn(m_prediction());
}

using CLMove_t = void(__vectorcall*)(float, bool);
void __vectorcall hooks::hkClMove(float accumulated_extra_samples, bool bFinalTick)
{
	if (g_ctx.globals.should_recharge)
	{
		g_ctx.get_command()->m_tickcount = INT_MAX;

		if (++g_ctx.globals.ticks_allowed >= 14)
		{
			g_ctx.globals.should_recharge = false;
			g_ctx.send_packet = true;
		}
		else
			g_ctx.send_packet = false;

		return;
	}

	((CLMove_t)hooks::original_clmove)(accumulated_extra_samples, bFinalTick);
}

void WriteUserÑmd(void* buf, CUserCmd* incmd, CUserCmd* outcmd)
{
	using WriteUserCmd_t = void(__fastcall*)(void*, CUserCmd*, CUserCmd*);
	static auto Fn = (WriteUserCmd_t)util::FindSignature(crypt_str("client.dll"), crypt_str("55 8B EC 83 E4 F8 51 53 56 8B D9 8B 0D"));

	__asm
	{
		mov     ecx, buf
		mov     edx, incmd
		push    outcmd
		call    Fn
		add     esp, 4
	}
}

bool __fastcall hooks::hkWriteUserCmdDeltaToBuffer(void* ecx, void* edx, int slot, bf_write* buf, int from, int to, bool is_new_command)
{
	static auto original_fn = client_hook->get_func_address <WriteUsercmdDeltaToBuffer_t>(24);

	if (!g_ctx.globals.tickbase_shift)
		return original_fn(ecx, slot, buf, from, to, is_new_command);

	if (from != -1)
		return true;

	from = -1;

	int m_nTickbase = g_ctx.globals.tickbase_shift;
	g_ctx.globals.tickbase_shift = 0;

	int* m_pnNewCmds = (int*)((uintptr_t)buf - 0x2C);
	int* m_pnBackupCmds = (int*)((uintptr_t)buf - 0x30);

	*m_pnBackupCmds = 0;

	int m_nNewCmds = *m_pnNewCmds;
	int m_nNextCmd = m_clientstate()->iChokedCommands + m_clientstate()->nLastOutgoingCommand + 1;
	int m_nTotalNewCmds = min(m_nNewCmds + abs(m_nTickbase), 62);

	*m_pnNewCmds = m_nTotalNewCmds;

	for (to = m_nNextCmd - m_nNewCmds + 1; to <= m_nNextCmd; to++)
	{
		if (!original_fn(ecx, slot, buf, from, to, true))
			return false;

		from = to;
	}

	CUserCmd* m_pCmd = m_input()->GetUserCmd(from);

	if (!m_pCmd)
		return true;

	CUserCmd m_ToCmd = *m_pCmd, m_FromCmd = *m_pCmd;

	m_ToCmd.m_command_number++;
	m_ToCmd.m_tickcount += ((int)std::round(1.f / m_globals()->m_intervalpertick) * 3);

	for (int i = m_nNewCmds; i <= m_nTotalNewCmds; i++) 
	{
		WriteUserÑmd(buf, &m_ToCmd, &m_FromCmd);
		m_FromCmd = m_ToCmd;

		m_ToCmd.m_command_number++;
		m_ToCmd.m_tickcount++;
	}

	g_ctx.globals.tickbase_shift = m_nTickbase;

	return true;
}