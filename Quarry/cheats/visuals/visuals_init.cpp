#pragma once

#include "visuals_init.h"
#include "nightmode.h"

void visuals_init::init()
{
	if (g_ctx.available())
	{
		g_ctx.globals.bomb_carrier = -1;

		if (g_cfg.player.enable)
		{
			worldesp::get().paint_traverse();
			playeresp::get().paint_traverse();
		}

		otheresp::get().desync_arrows();

		auto weapon = g_ctx.local()->m_hActiveWeapon().Get();

		if (weapon->is_grenade() && g_cfg.esp.grenade_prediction && g_cfg.player.enable)
			GrenadePrediction::get().Paint();

		if (g_cfg.player.enable && g_cfg.esp.removals[REMOVALS_SCOPE] && g_ctx.globals.scoped && weapon->is_sniper())
		{
			static int w, h;
			m_engine()->GetScreenSize(w, h);

			g_render::get().line(w / 2, 0, w / 2, h, Color::Black);
			g_render::get().line(0, h / 2, w, h / 2, Color::Black);
		}

		if (g_cfg.player.enable)
			otheresp::get().hitmarker_paint();

		if (g_cfg.player.enable && g_cfg.esp.damage_marker)
			otheresp::get().damage_marker_paint();

		otheresp::get().automatic_peek_indicator();
		misc::get().spectators_list();
	}

	static auto framerate = 0.0f;
	framerate = 0.9f * framerate + 0.1f * m_globals()->m_absoluteframetime;

	if (framerate <= 0.0f)
		framerate = 1.0f;

	g_ctx.globals.framerate = (int)(1.0f / framerate);
	auto nci = m_engine()->GetNetChannelInfo();

	if (nci)
	{
		auto latency = m_engine()->IsPlayingDemo() ? 0.0f : nci->GetAvgLatency(FLOW_OUTGOING);

		if (latency)
		{
			static auto cl_updaterate = m_cvar()->FindVar(crypt_str("cl_updaterate"));
			latency -= 0.5f / cl_updaterate->GetFloat();
		}

		g_ctx.globals.ping = (int)(max(0.0f, latency) * 1000.0f);
	}

	time_t lt;
	struct tm* t_m;

	lt = time(nullptr);
	t_m = localtime(&lt);

	auto time_h = t_m->tm_hour;
	auto time_m = t_m->tm_min;
	auto time_s = t_m->tm_sec;

	std::string time;

	if (time_h < 10)
		time += "0";

	time += std::to_string(time_h) + ":";

	if (time_m < 10)
		time += "0";

	time += std::to_string(time_m) + ":";

	if (time_s < 10)
		time += "0";

	time += std::to_string(time_s);
	g_ctx.globals.time = std::move(time);

	misc::get().watermark();
	eventlogs::get().paint_traverse();
	nightmode::get().nightmode_fix();
	otheresp::get().indicators();

	if (g_ctx.globals.loaded_script)
		for (auto current : c_lua::get().hooks.getHooks(crypt_str("on_paint")))
			current.func();

	otheresp::get().draw_indicators();
}