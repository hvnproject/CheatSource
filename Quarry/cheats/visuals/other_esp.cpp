#include "other_esp.h"
#include "..\autowall\autowall.h"
#include "..\ragebot\antiaim.h"
#include "..\misc\logs.h"
#include "..\misc\misc.h"
#include "..\lagcompensation\local_animations.h"

bool can_penetrate(weapon_t* weapon, float& damage)
{
	auto weapon_info = weapon->get_csweapon_info();

	if (!weapon_info)
		return false;

	Vector view_angles;
	m_engine()->GetViewAngles(view_angles);

	Vector direction;
	math::angle_vectors(view_angles, direction);

	CTraceFilter filter;
	filter.pSkip = g_ctx.local();

	trace_t trace;
	util::trace_line(g_ctx.globals.eye_pos, g_ctx.globals.eye_pos + direction * weapon_info->flRange, MASK_SHOT_HULL | CONTENTS_HITBOX, &filter, &trace);

	if (trace.fraction == 1.0f)
		return false;

	auto eye_pos = g_ctx.globals.eye_pos;
	auto hits = 1;
	damage = (float)weapon_info->iDamage;
	auto penetration_power = weapon_info->flPenetration;

	static auto damageReductionBullets = m_cvar()->FindVar(crypt_str("ff_damage_reduction_bullets"));
	static auto damageBulletPenetration = m_cvar()->FindVar(crypt_str("ff_damage_bullet_penetration"));

	return autowall::get().handle_bullet_penetration(weapon_info, trace, eye_pos, direction, hits, damage, penetration_power, damageReductionBullets->GetFloat(), damageBulletPenetration->GetFloat());
}

void otheresp::penetration_reticle(LPDIRECT3DDEVICE9 device)
{
	if (!g_cfg.player.enable)
		return;

	if (!g_cfg.esp.penetration_reticle)
		return;

	if (!g_ctx.local()->is_alive())
		return;

	auto weapon = g_ctx.local()->m_hActiveWeapon().Get();

	if (!weapon)
		return;

	static auto alpha = 255;
	auto factor = 550 * m_globals()->m_frametime;

	if (weapon->is_non_aim() || weapon->m_iItemDefinitionIndex() == WEAPON_TASER)
	{
		alpha = math::clamp(alpha - factor, 0, 255);
		goto draw;
	}

	auto weapon_info = weapon->get_csweapon_info();

	if (!weapon_info)
		return;

	float damage;
	bool penetrate = can_penetrate(weapon, damage);
	damage = math::clamp(damage, 0, 100);

	static Color color = Color(127, 0, 127, 0);
	static float dist = 1.f;
	static float radius = 10.f;

	if (!penetrate || !damage)
	{
		alpha = math::clamp(alpha - factor, 0, 255);
		goto draw;
	}

	alpha = math::clamp(alpha + int(factor * 1.5f), 0, 255);
	color = Color((int)((100 - damage) / 100.f * 255), (int)(damage / 100.f * 255), 0, alpha);

	CGameTrace enterTrace;
	CTraceFilter filter;
	Ray_t ray;

	Vector viewangles;
	m_engine()->GetViewAngles(viewangles);

	Vector direction;
	math::angle_vectors(viewangles, direction);

	Vector start = g_ctx.globals.eye_pos;
	auto m_flMaxRange = weapon_info->flRange;
	Vector end = start + (direction * m_flMaxRange * 2);

	filter.pSkip = g_ctx.local();
	ray.Init(start, end);
	m_trace()->TraceRay(ray, MASK_SHOT | CONTENTS_GRATE, &filter, &enterTrace);

	dist = g_ctx.globals.eye_pos.DistTo(enterTrace.endpos) / 12.f;
	radius = math::clamp((100.f - dist) / 100.f * 40.f, 5.f, 25.f);

draw:
	if (alpha > 0 && radius)
		g_render::get().dual_circle(g_render::get().m_screen_size.x / 2, g_render::get().m_screen_size.y / 2, radius, 50, D3DCOLOR_RGBA(0, 0, 0, 0), D3DCOLOR_RGBA(color.r(), color.g(), color.b(), alpha), device);
}

void otheresp::indicators()
{
	if (!g_ctx.local()->is_alive())
		return;

	auto weapon = g_ctx.local()->m_hActiveWeapon().Get();

	if (!weapon)
		return;

	if (g_cfg.esp.indicators[INDICATOR_FAKE] && (g_cfg.antiaim.type[antiaim::get().type].desync))
	{
		auto color = Color(130, 20, 20);
		auto animstate = g_ctx.local()->get_animation_state();

		if (animstate && local_animations::get().local_data.animstate)
		{
			auto delta = fabs(math::normalize_yaw(animstate->m_flGoalFeetYaw - local_animations::get().local_data.animstate->m_flGoalFeetYaw));
			auto desync_delta = max(g_ctx.local()->get_max_desync_delta(), 58.0f);

			color = Color(130, 20 + (int)(min(delta / desync_delta, 1.0f) * 150.0f), 20);
		}

		m_indicators.push_back(m_indicator("FAKE", color));
	}

	if (g_cfg.esp.indicators[INDICATOR_DESYNC_SIDE] && (g_cfg.antiaim.desync == 1 || g_cfg.antiaim.type[antiaim::get().type].desync == 1) && !antiaim::get().condition(g_ctx.get_command()))
	{
		auto side = antiaim::get().desync_angle > 0.0f ? "RIGHT" : "LEFT";
		m_indicators.push_back(m_indicator(side, Color(130, 170, 20)));
	}

	auto choke_indicator = false;

	if (g_cfg.esp.indicators[INDICATOR_CHOKE] && !fakelag::get().condition && !misc::get().double_tap_enabled && !misc::get().hide_shots_enabled)
	{
		m_indicators.push_back(m_indicator(("CHOKE: " + std::to_string(fakelag::get().max_choke)), Color(130, 170, 20)));
		choke_indicator = true;
	}

	if (g_cfg.esp.indicators[INDICATOR_DAMAGE] && g_ctx.globals.current_weapon != -1 && key_binds::get().get_key_bind_state(4 + g_ctx.globals.current_weapon) && !weapon->is_non_aim())
	{
		if (g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].minimum_override_damage > 100)
			m_indicators.push_back(m_indicator(("DMG: HP + " + std::to_string(g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].minimum_override_damage - 100)), Color(130, 170, 20)));
		else
			m_indicators.push_back(m_indicator(("DMG: " + std::to_string(g_cfg.ragebot.weapon[g_ctx.globals.current_weapon].minimum_override_damage)), Color(130, 170, 20)));
	}

	if (g_cfg.esp.indicators[INDICATOR_SAFE_POINTS] && key_binds::get().get_key_bind_state(3) && !weapon->is_non_aim())
		m_indicators.push_back(m_indicator("SAFE POINTS", Color(130, 170, 20)));

	if (g_cfg.esp.indicators[INDICATOR_BODY_AIM] && key_binds::get().get_key_bind_state(22) && !weapon->is_non_aim())
		m_indicators.push_back(m_indicator("BODY AIM", Color(130, 170, 20)));

	if (choke_indicator)
		return;

	if (g_cfg.esp.indicators[INDICATOR_DT] && g_cfg.ragebot.double_tap && g_cfg.ragebot.double_tap_key.key > KEY_NONE && g_cfg.ragebot.double_tap_key.key < KEY_MAX && misc::get().double_tap_key)
		m_indicators.push_back(m_indicator("DT", !g_ctx.local()->m_bGunGameImmunity() && !(g_ctx.local()->m_fFlags() & FL_FROZEN) && !antiaim::get().freeze_check && misc::get().double_tap_enabled && g_ctx.globals.tocharge >= g_ctx.globals.weapon->get_max_tickbase_shift() && !weapon->is_grenade() && weapon->m_iItemDefinitionIndex() != WEAPON_TASER && weapon->m_iItemDefinitionIndex() != WEAPON_REVOLVER && weapon->can_fire(false) ? Color(255, 255, 255) : Color(130, 20, 20)));

	if (g_cfg.esp.indicators[INDICATOR_HS] && g_cfg.antiaim.hide_shots && g_cfg.antiaim.hide_shots_key.key > KEY_NONE && g_cfg.antiaim.hide_shots_key.key < KEY_MAX && misc::get().hide_shots_key)
		m_indicators.push_back(m_indicator("HS", !g_ctx.local()->m_bGunGameImmunity() && !(g_ctx.local()->m_fFlags() & FL_FROZEN) && !antiaim::get().freeze_check && misc::get().hide_shots_enabled ? Color(130, 170, 20) : Color(130, 20, 20)));
}

void otheresp::draw_indicators()
{
	if (!g_ctx.local()->is_alive())
		return;

	static int width, height;
	m_engine()->GetScreenSize(width, height);

	auto h = height - 325;

	for (auto& indicator : m_indicators)
	{
		g_render::get().text(g_render::get().indicators, 27, h, indicator.m_color, false, true, true, true, indicator.m_text.c_str());
		h -= 25;
	}

	m_indicators.clear();
}

void otheresp::hitmarker_paint()
{
	if (!g_cfg.esp.hitmarker[0] && !g_cfg.esp.hitmarker[1])
	{
		hitmarker.hurt_time = FLT_MIN;
		hitmarker.point = ZERO;
		return;
	}

	if (!g_ctx.local()->is_alive())
	{
		hitmarker.hurt_time = FLT_MIN;
		hitmarker.point = ZERO;
		return;
	}

	int w, h = 0;
	m_engine()->GetScreenSize(w, h);
	w = w / 2;
	h = h / 2;

	if (hitmarker.hurt_time + 2.0f > m_globals()->m_curtime)
	{
		// Screen hitmarker
		if (g_cfg.esp.hitmarker[0])
		{
			auto alpha = (int)((hitmarker.hurt_time + 2.0f - m_globals()->m_curtime) * 127.5f);
			hitmarker.hurt_color.SetAlpha(alpha);

			g_render::get().line(w + 10, h + 10, w + 3, h + 3, hitmarker.hurt_color);
			g_render::get().line(w - 10, h - 10, w - 3, h - 3, hitmarker.hurt_color);
			g_render::get().line(w + 10, h - 10, w + 3, h - 3, hitmarker.hurt_color);
			g_render::get().line(w - 10, h + 10, w - 3, h + 3, hitmarker.hurt_color);
		}

		// World hitmarker
		if (g_cfg.esp.hitmarker[1])
		{
			Vector origin;

			if (!math::world_to_screen(hitmarker.point, origin))
				return;

			auto alpha = (int)((hitmarker.hurt_time + 2.0f - m_globals()->m_curtime) * 127.5f);
			hitmarker.hurt_color.SetAlpha(alpha);

			g_render::get().line(origin.x + 3, origin.y + 3, origin.x + 6, origin.y + 6, hitmarker.hurt_color);
			g_render::get().line(origin.x - 3, origin.y - 3, origin.x - 6, origin.y - 6, hitmarker.hurt_color);
			g_render::get().line(origin.x + 3, origin.y - 3, origin.x + 6, origin.y - 6, hitmarker.hurt_color);
			g_render::get().line(origin.x - 3, origin.y + 3, origin.x - 6, origin.y + 6, hitmarker.hurt_color);
		}
	}
}

void otheresp::damage_marker_paint()
{
	for (auto i = 1; i < m_globals()->m_maxclients; i++)
	{
		if (damage_marker[i].hurt_time + 2.0f > m_globals()->m_curtime)
		{
			Vector screen;

			if (!math::world_to_screen(damage_marker[i].position, screen))
				continue;

			auto alpha = (int)((damage_marker[i].hurt_time + 2.0f - m_globals()->m_curtime) * 127.5f);

			damage_marker[i].hurt_color.SetAlpha(alpha);
			damage_marker[i].animation += 0.3f;

			g_render::get().text(g_render::get().damage, screen.x, screen.y - damage_marker[i].animation, damage_marker[i].hurt_color, true, true, true, false, "-%i", damage_marker[i].damage);
		}
	}
}

void otheresp::spread_crosshair(LPDIRECT3DDEVICE9 device)
{
	if (!g_cfg.player.enable)
		return;

	if (!g_cfg.esp.show_spread)
		return;

	if (!g_ctx.local()->is_alive())
		return;

	auto weapon = g_ctx.local()->m_hActiveWeapon().Get();

	if (weapon->is_non_aim())
		return;

	int w, h;
	m_engine()->GetScreenSize(w, h);

	g_render::get().dual_circle((float)w * 0.5f, (float)h * 0.5f, g_ctx.globals.inaccuracy * 500.0f, 50, D3DCOLOR_RGBA(g_cfg.esp.show_spread_color.r(), g_cfg.esp.show_spread_color.g(), g_cfg.esp.show_spread_color.b(), g_cfg.esp.show_spread_color.a()), D3DCOLOR_RGBA(0, 0, 0, 0), device);
}

void otheresp::automatic_peek_indicator()
{
	auto weapon = g_ctx.local()->m_hActiveWeapon().Get();

	if (!weapon)
		return;

	static auto position = ZERO;

	if (!g_ctx.globals.start_position.IsZero())
		position = g_ctx.globals.start_position;

	if (position.IsZero())
		return;

	static auto alpha = 4.0f;

	if (!weapon->is_non_aim() && key_binds::get().get_key_bind_state(18) || alpha)
	{
		if (!weapon->is_non_aim() && key_binds::get().get_key_bind_state(18))
			alpha += 9.0f * m_globals()->m_frametime;
		else
			alpha -= 9.0f * m_globals()->m_frametime;

		alpha = math::clamp(alpha, 0.0f, 1.0f);

		g_render::get().Draw3DFilledCircle(position, alpha * 15.f, g_ctx.globals.fired_shot ? Color(200, 200, 200, (int)(alpha * 135.0f)) : g_cfg.misc.automatic_peek_color);

		Vector screen;

		if (math::world_to_screen(position, screen))
		{
			static auto offset = 15.0f;

			if (!g_ctx.globals.fired_shot)
			{
				static auto switch_offset = false;

				if (offset <= 15.0f || offset >= 35.0f)
					switch_offset = !switch_offset;

				offset += switch_offset ? 22.0f * m_globals()->m_frametime : -22.0f * m_globals()->m_frametime;
				offset = math::clamp(offset, 15.0f, 35.0f);
			}
		}
	}
}

void otheresp::desync_arrows()
{
	if (!g_ctx.local()->is_alive())
		return;

	if (!g_cfg.ragebot.enable)
		return;

	if (!g_cfg.antiaim.enable)
		return;

	if ((g_cfg.antiaim.manual_back.key <= KEY_NONE || g_cfg.antiaim.manual_back.key >= KEY_MAX) && (g_cfg.antiaim.manual_left.key <= KEY_NONE || g_cfg.antiaim.manual_left.key >= KEY_MAX) && (g_cfg.antiaim.manual_right.key <= KEY_NONE || g_cfg.antiaim.manual_right.key >= KEY_MAX))
		antiaim::get().manual_side = SIDE_NONE;

	if (!g_cfg.antiaim.flip_indicator)
		return;

	static int width, height;
	m_engine()->GetScreenSize(width, height);

	static auto alpha = 1.0f;
	static auto switch_alpha = false;

	if (alpha <= 0.0f || alpha >= 1.0f)
		switch_alpha = !switch_alpha;

	alpha += switch_alpha ? 2.0f * m_globals()->m_frametime : -2.0f * m_globals()->m_frametime;
	alpha = math::clamp(alpha, 0.0f, 1.0f);

	auto color = g_cfg.antiaim.flip_indicator_color;
	color.SetAlpha((int)(min(255.0f * alpha, color.a())));

	if (antiaim::get().manual_side == SIDE_BACK)
		g_render::get().filled_triangle(Vector2D(width / 2, height / 2 + 80), Vector2D(width / 2 - 10, height / 2 + 60), Vector2D(width / 2 + 10, height / 2 + 60), color);
	else if (antiaim::get().manual_side == SIDE_LEFT)
		g_render::get().filled_triangle(Vector2D(width / 2 - 55, height / 2 + 10), Vector2D(width / 2 - 75, height / 2), Vector2D(width / 2 - 55, height / 2 - 10), color);
	else if (antiaim::get().manual_side == SIDE_RIGHT)
		g_render::get().filled_triangle(Vector2D(width / 2 + 55, height / 2 - 10), Vector2D(width / 2 + 75, height / 2), Vector2D(width / 2 + 55, height / 2 + 10), color);
}