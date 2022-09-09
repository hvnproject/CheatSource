#include "local_animations.h"

void local_animations::run(ClientFrameStage_t stage)
{
	if (!fakelag::get().condition && key_binds::get().get_key_bind_state(20))
	{
		if (stage == FRAME_NET_UPDATE_END)
		{
			fake_server_update = false;

			if (g_ctx.local()->m_flSimulationTime() != fake_simulation_time) 
			{
				fake_server_update = true;
				fake_simulation_time = g_ctx.local()->m_flSimulationTime();
			}

			g_ctx.local()->get_animlayers()[3].m_flWeight = 0.0f;
			g_ctx.local()->get_animlayers()[3].m_flCycle = 0.0f;
			g_ctx.local()->get_animlayers()[12].m_flWeight = 0.0f;

			update_fake_animations();
		}
		else if (stage == FRAME_RENDER_START)
		{
			auto animstate = g_ctx.local()->get_animation_state();

			if (!animstate)
				return;

			real_server_update = false;

			if (g_ctx.local()->m_flSimulationTime() != real_simulation_time) 
			{
				real_server_update = true;
				real_simulation_time = g_ctx.local()->m_flSimulationTime();
			}

			g_ctx.local()->get_animlayers()[3].m_flWeight = 0.0f;
			g_ctx.local()->get_animlayers()[3].m_flCycle = 0.0f;
			g_ctx.local()->get_animlayers()[12].m_flWeight = 0.0f;

			update_local_animations(animstate);
		}
	}
	else if (stage == FRAME_RENDER_START)
	{
		auto animstate = g_ctx.local()->get_animation_state(); 

		if (!animstate)
			return;

		real_server_update = false;
		fake_server_update = false;

		if (g_ctx.local()->m_flSimulationTime() != real_simulation_time || g_ctx.local()->m_flSimulationTime() != fake_simulation_time)
		{
			real_server_update = fake_server_update = true;
			real_simulation_time = fake_simulation_time = g_ctx.local()->m_flSimulationTime();
		}

		g_ctx.local()->get_animlayers()[3].m_flWeight = 0.0f;
		g_ctx.local()->get_animlayers()[3].m_flCycle = 0.0f;
		g_ctx.local()->get_animlayers()[12].m_flWeight = 0.0f;

		update_fake_animations();
		update_local_animations(animstate);
	}
}

void local_animations::update_prediction_animations()
{
	auto alloc = !local_data.prediction_animstate;
	auto change = !alloc && handle != &g_ctx.local()->GetRefEHandle();
	auto reset = !alloc && !change && g_ctx.local()->m_flSpawnTime() != spawntime;

	if (change)
		m_memalloc()->Free(local_data.prediction_animstate);

	if (reset)
	{
		util::reset_state(local_data.prediction_animstate);
		spawntime = g_ctx.local()->m_flSpawnTime();
	}

	if (alloc || change)
	{
		local_data.prediction_animstate = (c_baseplayeranimationstate*)m_memalloc()->Alloc(sizeof(c_baseplayeranimationstate));

		if (local_data.prediction_animstate)
			util::create_state(local_data.prediction_animstate, g_ctx.local());

		handle = (CBaseHandle*)&g_ctx.local()->GetRefEHandle();
		spawntime = g_ctx.local()->m_flSpawnTime();
	}

	if (!alloc && !change && !reset)
	{
		// copy pose parameters.
		memcpy(pose_parameters, &g_ctx.local()->m_flPoseParameter(), 24 * sizeof(float));

		// copy animation layers.
		memcpy(animation_layers, g_ctx.local()->get_animlayers(), 13 * sizeof(AnimationLayer));

		local_data.prediction_animstate->m_pBaseEntity = g_ctx.local();
		util::update_state(local_data.prediction_animstate, ZERO);

		g_ctx.local()->setup_bones_fixed(g_ctx.globals.prediction_matrix, BONE_USED_BY_HITBOX);

		// copy pose parameters.
		memcpy(&g_ctx.local()->m_flPoseParameter(), pose_parameters, 24 * sizeof(float));

		// copy animation layers.
		memcpy(g_ctx.local()->get_animlayers(), animation_layers, 13 * sizeof(AnimationLayer));
	}
}

void local_animations::update_fake_animations()
{
	auto alloc = !local_data.animstate;
	auto change = !alloc && handle != &g_ctx.local()->GetRefEHandle();
	auto reset = !alloc && !change && g_ctx.local()->m_flSpawnTime() != spawntime;

	if (change)
		m_memalloc()->Free(local_data.animstate);

	if (reset)
	{
		util::reset_state(local_data.animstate);
		spawntime = g_ctx.local()->m_flSpawnTime();
	}

	if (alloc || change)
	{
		local_data.animstate = (c_baseplayeranimationstate*)m_memalloc()->Alloc(sizeof(c_baseplayeranimationstate));

		if (local_data.animstate)
			util::create_state(local_data.animstate, g_ctx.local());

		handle = (CBaseHandle*)&g_ctx.local()->GetRefEHandle();
		spawntime = g_ctx.local()->m_flSpawnTime();
	}

	if (!alloc && !change && !reset && fake_server_update)
	{
		// copy pose parameters.
		memcpy(pose_parameters, &g_ctx.local()->m_flPoseParameter(), 24 * sizeof(float));

		// copy animation layers.
		memcpy(animation_layers, g_ctx.local()->get_animlayers(), 13 * sizeof(AnimationLayer));

		auto backup_frametime = m_globals()->m_frametime;
		auto backup_curtime = m_globals()->m_curtime;

		m_globals()->m_frametime = m_globals()->m_intervalpertick;
		m_globals()->m_curtime = g_ctx.local()->m_flSimulationTime();

		local_data.animstate->m_pBaseEntity = g_ctx.local();
		util::update_state(local_data.animstate, local_animations::get().local_data.fake_angles);

		local_data.animstate->m_bInHitGroundAnimation = false;
		local_data.animstate->m_fLandingDuckAdditiveSomething = 0.0f;
		local_data.animstate->m_flHeadHeightOrOffsetFromHittingGroundAnimation = 1.0f;

		// resets the entity's bone cache values in order to prepare for a model change.
		g_ctx.local()->invalidate_bone_cache();

		// rebuild fake matrix.
		g_ctx.local()->setup_bones_fixed(g_ctx.globals.fake_matrix, BONE_USED_BY_ANYTHING);
		local_data.visualize_lag = g_cfg.player.visualize_lag;

		// update fake matrix.
		if (!local_data.visualize_lag)
		{
			for (auto& i : g_ctx.globals.fake_matrix)
			{
				i[0][3] -= g_ctx.local()->GetRenderOrigin().x;
				i[1][3] -= g_ctx.local()->GetRenderOrigin().y;
				i[2][3] -= g_ctx.local()->GetRenderOrigin().z;
			}
		}

		m_globals()->m_frametime = backup_frametime;
		m_globals()->m_curtime = backup_curtime;

		memcpy(&g_ctx.local()->m_flPoseParameter(), pose_parameters, 24 * sizeof(float));
		memcpy(g_ctx.local()->get_animlayers(), animation_layers, 13 * sizeof(AnimationLayer));
	}
}

void local_animations::update_local_animations(c_baseplayeranimationstate* animstate)
{
	if (tickcount != m_globals()->m_tickcount)
	{
		tickcount = m_globals()->m_tickcount;

		// copy animation layers.
		memcpy(animation_layers, g_ctx.local()->get_animlayers(), 13 * sizeof(AnimationLayer));

		if (local_data.animstate)
			animstate->m_fDuckAmount = local_data.animstate->m_fDuckAmount;

		animstate->m_iLastClientSideAnimationUpdateFramecount = 0;
		util::update_state(animstate, local_animations::get().local_data.fake_angles);

		if (real_server_update)
		{
			abs_angles = animstate->m_flGoalFeetYaw;

			// copy pose parameters.
			memcpy(pose_parameters, &g_ctx.local()->m_flPoseParameter(), 24 * sizeof(float));
		}
	}
	else
		animstate->m_iLastClientSideAnimationUpdateFramecount = m_globals()->m_framecount;

	// set template of goal feet yaw.
	animstate->m_flGoalFeetYaw = antiaim::get().condition(g_ctx.get_command()) ? abs_angles : local_animations::get().local_data.real_angles.y;

	// set abs angles to goal feet yaw.
	g_ctx.local()->set_abs_angles(Vector(0.0f, abs_angles, 0.0f));

	// set uninterpolated origin.
	g_ctx.local()->set_abs_origin(g_ctx.local()->m_vecOrigin());

	// set uninterpolated velocity.
	g_ctx.local()->set_abs_velocity(g_ctx.local()->m_vecAbsVelocity());

	// copy animation layers.
	memcpy(g_ctx.local()->get_animlayers(), animation_layers, 13 * sizeof(AnimationLayer));

	// copy pose parameters.
	memcpy(&g_ctx.local()->m_flPoseParameter(), pose_parameters, 24 * sizeof(float));
}