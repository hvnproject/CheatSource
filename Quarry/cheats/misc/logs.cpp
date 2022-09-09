#include "logs.h"

float absolute_time()
{
	return (float)(clock() / (float)1000.f);
}

std::map<std::string, std::string> event_to_normal =
{
	// Other
	{ "weapon_taser", "Zeus" },
	{ "item_kevlar", "Kevlar" },
	{ "item_defuser", "Defuse kit" },
	{ "item_assaultsuit", "Kevlar + Helmet" },

	// Pistols
	{ "weapon_p250", "P250" },
	{ "weapon_tec9", "TEC-9" },
	{ "weapon_cz75a", "CZ75A" },
	{ "weapon_glock", "Glock" },
	{ "weapon_elite", "Double-Berretas" },
	{ "weapon_deagle", "Desert-Eagle" },
	{ "weapon_hkp2000", "P2000" },
	{ "weapon_usp_silencer", "USP-S" },
	{ "weapon_revolver", "R8 Revolver" },
	{ "weapon_fiveseven", "Five-Seven" },

	// PP
	{ "weapon_mp9", "MP-9" },
	{ "weapon_mac10", "MAC-10" },
	{ "weapon_mp7", "MP-7" },
	{ "weapon_mp5sd", "MP5-SD" },
	{ "weapon_ump45", "UMP-45" },
	{ "weapon_p90", "P90" },
	{ "weapon_bizon", "PP-Bizon" },

	//rifles
	{ "weapon_famas", "FAMAS" },
	{ "weapon_m4a1_silencer", "M4A1-s" },
	{ "weapon_m4a1", "M4A1" },
	{ "weapon_ssg08", "SSG08" },
	{ "weapon_aug", "AUG" },
	{ "weapon_awp", "AWP" },
	{ "weapon_scar20", "SCAR20" },
	{ "weapon_galilar", "AR-Galil" },
	{ "weapon_ak47", "AK-47" },
	{ "weapon_sg556", "SG553" },
	{ "weapon_g3sg1", "G3SG1" },

	// Heavy
	{ "weapon_nova", "Nova" },
	{ "weapon_xm1014", "XM1014" },
	{ "weapon_sawedoff", "Sawed-Off" },
	{ "weapon_m249", "M249" },
	{ "weapon_negev", "Negev" },
	{ "weapon_mag7", "MAG-7" },

	// Grenades
	{ "weapon_flashbang", "Flash grenade" },
	{ "weapon_smokegrenade", "Smoke grenade" },
	{ "weapon_molotov", "Molotov" },
	{ "weapon_incgrenade", "Incereative grenade" },
	{ "weapon_decoy", "Decoy grenade" },
	{ "weapon_hegrenade", "HE Grenade" },
};

void eventlogs::paint_traverse()
{
	constexpr float showtime = 5.f;
	constexpr float animation_time = 0.2f;

	if (logs.empty())
		return;

	while (logs.size() > 25)
		logs.erase(logs.begin());

	static const auto easeOutQuad = [](float x) {
		return 1 - (1 - x) * (1 - x);
	};

	static const auto easeInQuad = [](float x) {
		return x * x;
	};

	for (int i = logs.size() - 1; i >= 0; i--)
	{
		float in_anim = math::clamp((absolute_time() - logs[i].log_time) / animation_time, 0.01f, 1.f);
		float out_anim = math::clamp(((absolute_time() - logs[i].log_time) - showtime) / animation_time, 0.f, FLT_MAX);

		logs[i].color.SetAlpha(in_anim * (1.f - out_anim) * 255.f);

		if (out_anim > 1.f)
			logs[i].color.SetAlpha(0.f);
		if (in_anim > 1.f)
			logs[i].color.SetAlpha(255.f);

		in_anim = easeInQuad(in_anim);
		out_anim = easeOutQuad(out_anim);

		if (logs[i].color.a() > 0.f)
		{
			const float x_position = in_anim * 5.f - out_anim * 5.f;
			const float y_position = 14.f * i;

			g_render::get().text(g_render::get().logs, x_position, y_position + 6.5f, Color(255, 255, 255, logs[i].color.a()), false, false, true, true, logs[i].message.c_str());
		}
	}

	for (int i = logs.size() - 1; i >= 0; i--)
	{
		if (logs[i].color.a() <= 0.f)
		{
			logs.erase(logs.begin() + i);
			break;
		}
	}
}

void eventlogs::events(IGameEvent* event)
{
	static auto get_hitgroup_name = [](int hitgroup) -> std::string
	{
		switch (hitgroup)
		{
		case HITGROUP_HEAD:
			return crypt_str("head");
		case HITGROUP_CHEST:
			return crypt_str("chest");
		case HITGROUP_STOMACH:
			return crypt_str("stomach");
		case HITGROUP_LEFTARM:
			return crypt_str("left arm");
		case HITGROUP_RIGHTARM:
			return crypt_str("right arm");
		case HITGROUP_LEFTLEG:
			return crypt_str("left leg");
		case HITGROUP_RIGHTLEG:
			return crypt_str("right leg");
		default:
			return crypt_str("generic");
		}
	};

	if (g_cfg.misc.events_to_log[EVENTLOG_HIT] && !strcmp(event->GetName(), crypt_str("player_hurt")))
	{
		auto userid = event->GetInt(crypt_str("userid")), attacker = event->GetInt(crypt_str("attacker"));

		if (!userid || !attacker)
			return;

		auto userid_id = m_engine()->GetPlayerForUserID(userid), attacker_id = m_engine()->GetPlayerForUserID(attacker);

		player_info_t userid_info, attacker_info;

		if (!m_engine()->GetPlayerInfo(userid_id, &userid_info))
			return;

		if (!m_engine()->GetPlayerInfo(attacker_id, &attacker_info))
			return;

		auto m_victim = static_cast<player_t*>(m_entitylist()->GetClientEntity(userid_id));

		std::stringstream ss;

		if (attacker_id == m_engine()->GetLocalPlayer() && userid_id != m_engine()->GetLocalPlayer())
		{
			ss << crypt_str("hurt ") << userid_info.szName << "'s" << crypt_str(" in the ") << get_hitgroup_name(event->GetInt(crypt_str("hitgroup"))) << crypt_str(" for ") << event->GetInt(crypt_str("dmg_health"));
			ss << crypt_str(" damage [ ") << event->GetInt(crypt_str("health")) << crypt_str(" health remaining ]");

			add(ss.str());
		}
		else if (userid_id == m_engine()->GetLocalPlayer() && attacker_id != m_engine()->GetLocalPlayer())
		{
			ss << crypt_str("harmed for ") << event->GetInt(crypt_str("dmg_health")) << crypt_str(" damage from ") << attacker_info.szName << crypt_str(" in the ") << get_hitgroup_name(event->GetInt(crypt_str("hitgroup")));

			add(ss.str());
		}
	}

	if (g_cfg.misc.events_to_log[EVENTLOG_ITEM_PURCHASES] && !strcmp(event->GetName(), crypt_str("item_purchase")))
	{
		auto userid = event->GetInt(crypt_str("userid"));

		if (!userid)
			return;

		auto userid_id = m_engine()->GetPlayerForUserID(userid);

		player_info_t userid_info;

		if (!m_engine()->GetPlayerInfo(userid_id, &userid_info))
			return;

		auto m_player = static_cast<player_t*>(m_entitylist()->GetClientEntity(userid_id));

		if (!g_ctx.local() || !m_player)
			return;

		if (g_ctx.local() == m_player)
			g_ctx.globals.should_buy = 0;

		if (m_player->m_iTeamNum() == g_ctx.local()->m_iTeamNum())
			return;

		std::string weapon = event->GetString(crypt_str("weapon"));

		std::stringstream ss;
		ss << userid_info.szName << crypt_str(" purchase ") << event_to_normal[weapon.c_str()];

		add(ss.str());
	}

	if (g_cfg.misc.events_to_log[EVENTLOG_BOMB] && !strcmp(event->GetName(), crypt_str("bomb_beginplant")))
	{
		auto userid = event->GetInt(crypt_str("userid"));

		if (!userid)
			return;

		auto userid_id = m_engine()->GetPlayerForUserID(userid);

		player_info_t userid_info;

		if (!m_engine()->GetPlayerInfo(userid_id, &userid_info))
			return;

		auto m_player = static_cast<player_t*>(m_entitylist()->GetClientEntity(userid_id));

		if (!m_player)
			return;

		std::stringstream ss;
		ss << userid_info.szName << crypt_str(" has began planting the bomb");

		add(ss.str());
	}

	if (g_cfg.misc.events_to_log[EVENTLOG_BOMB] && !strcmp(event->GetName(), crypt_str("bomb_begindefuse")))
	{
		auto userid = event->GetInt(crypt_str("userid"));

		if (!userid)
			return;

		auto userid_id = m_engine()->GetPlayerForUserID(userid);

		player_info_t userid_info;

		if (!m_engine()->GetPlayerInfo(userid_id, &userid_info))
			return;

		auto m_player = static_cast<player_t*>(m_entitylist()->GetClientEntity(userid_id));

		if (!m_player)
			return;

		std::stringstream ss;
		ss << userid_info.szName << crypt_str(" has began defusing the bomb ") << (event->GetBool(crypt_str("haskit")) ? crypt_str("with defuse kit") : crypt_str("without defuse kit"));

		add(ss.str());
	}
}

void eventlogs::add(std::string text, bool full_display)
{
	logs.emplace_front(loginfo_t(absolute_time(), text, g_cfg.misc.log_color));

	if (!full_display)
		return;

	if (g_cfg.misc.log_output[EVENTLOG_OUTPUT_CONSOLE])
	{
		last_log = true;

		m_cvar()->ConsoleColorPrintf(g_cfg.misc.log_color, crypt_str("[ heaven ] "));

		m_cvar()->ConsoleColorPrintf(Color::White, text.c_str());
		m_cvar()->ConsolePrintf(crypt_str("\n"));
	}

	if (g_cfg.misc.log_output[EVENTLOG_OUTPUT_CHAT])
	{
		static CHudChat* chat = nullptr;

		if (!chat)
			chat = util::FindHudElement <CHudChat>(crypt_str("CHudChat"));

		auto log = crypt_str("[ \x0Cheaven \x01] ") + text;
		chat->chat_print(log.c_str());
	}
}