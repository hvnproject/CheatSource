#include "world_esp.h"
#include "GrenadeWarning.h"

void worldesp::paint_traverse()
{
	skybox_changer();

	for (int i = 1; i <= m_entitylist()->GetHighestEntityIndex(); i++)
	{
		auto e = static_cast<entity_t*>(m_entitylist()->GetClientEntity(i));

		if (!e)
			continue;

		if (e->is_player())
			continue;

		if (e->IsDormant())
			continue;

		auto client_class = e->GetClientClass();

		if (!client_class)
			continue;

		switch (client_class->m_ClassID)
		{
		case CEnvTonemapController:
			world_modulation(e);
			break;
		case CInferno:
			molotov_timer(e);
			break;
		case CSmokeGrenadeProjectile:
			smoke_timer(e);
			break;
		case CPlantedC4:
			bomb_timer(e);
			break;
		case CC4:
			if (g_cfg.player.type[ENEMY].flags[FLAGS_C4] || g_cfg.player.type[TEAM].flags[FLAGS_C4] || g_cfg.player.type[LOCAL].flags[FLAGS_C4] || g_cfg.esp.bomb_timer)
			{
				auto owner = (player_t*)m_entitylist()->GetClientEntityFromHandle(e->m_hOwnerEntity());

				if ((g_cfg.player.type[ENEMY].flags[FLAGS_C4] || g_cfg.player.type[TEAM].flags[FLAGS_C4] || g_cfg.player.type[LOCAL].flags[FLAGS_C4]) && owner->valid(false, false))
					g_ctx.globals.bomb_carrier = owner->EntIndex();
				else if (g_cfg.esp.bomb_timer && !owner->is_player())
				{
					auto screen = ZERO;

					if (math::world_to_screen(e->GetAbsOrigin(), screen))
						g_render::get().text(g_render::get().world, screen.x, screen.y, Color(215, 20, 20), true, true, true, false, "C4");
				}
			}

			break;
		default:
			grenade_projectiles(e);
			c_grenade_warning::get().grenade_warning(e);

			if (client_class->m_ClassID == CAK47 || client_class->m_ClassID == CDEagle || client_class->m_ClassID >= CWeaponAug && client_class->m_ClassID <= CWeaponZoneRepulsor)
				dropped_weapons(e);

			break;
		}
	}
}

void worldesp::skybox_changer()
{
	static auto load_skybox = reinterpret_cast<void(__fastcall*)(const char*)>(util::FindSignature(crypt_str("engine.dll"), crypt_str("55 8B EC 81 EC ? ? ? ? 56 57 8B F9 C7 45")));
	auto skybox_name = backup_skybox;

	switch (g_cfg.esp.skybox)
	{
	case 1:
		skybox_name = "cs_tibet";
		break;
	case 2:
		skybox_name = "cs_baggage_skybox_";
		break;
	case 3:
		skybox_name = "italy";
		break;
	case 4:
		skybox_name = "jungle";
		break;
	case 5:
		skybox_name = "office";
		break;
	case 6:
		skybox_name = "sky_cs15_daylight01_hdr";
		break;
	case 7:
		skybox_name = "sky_cs15_daylight02_hdr";
		break;
	case 8:
		skybox_name = "vertigoblue_hdr";
		break;
	case 9:
		skybox_name = "vertigo";
		break;
	case 10:
		skybox_name = "sky_day02_05_hdr";
		break;
	case 11:
		skybox_name = "nukeblank";
		break;
	case 12:
		skybox_name = "sky_venice";
		break;
	case 13:
		skybox_name = "sky_cs15_daylight03_hdr";
		break;
	case 14:
		skybox_name = "sky_cs15_daylight04_hdr";
		break;
	case 15:
		skybox_name = "sky_csgo_cloudy01";
		break;
	case 16:
		skybox_name = "sky_csgo_night02";
		break;
	case 17:
		skybox_name = "sky_csgo_night02b";
		break;
	case 18:
		skybox_name = "sky_csgo_night_flat";
		break;
	case 19:
		skybox_name = "sky_dust";
		break;
	case 20:
		skybox_name = "vietnam";
		break;
	case 21:
		skybox_name = g_cfg.esp.custom_skybox;
		break;
	}

	static auto skybox_number = 0;
	static auto old_skybox_name = skybox_name;

	static auto color_r = (unsigned char)255;
	static auto color_g = (unsigned char)255;
	static auto color_b = (unsigned char)255;

	if (skybox_number != g_cfg.esp.skybox)
	{
		changed = true;
		skybox_number = g_cfg.esp.skybox;
	}
	else if (old_skybox_name != skybox_name)
	{
		changed = true;
		old_skybox_name = skybox_name;
	}
	else if (color_r != g_cfg.esp.skybox_color[0])
	{
		changed = true;
		color_r = g_cfg.esp.skybox_color[0];
	}
	else if (color_g != g_cfg.esp.skybox_color[1])
	{
		changed = true;
		color_g = g_cfg.esp.skybox_color[1];
	}
	else if (color_b != g_cfg.esp.skybox_color[2])
	{
		changed = true;
		color_b = g_cfg.esp.skybox_color[2];
	}

	if (changed)
	{
		changed = false;
		load_skybox(skybox_name.c_str());

		auto materialsystem = m_materialsystem();

		for (auto i = materialsystem->FirstMaterial(); i != materialsystem->InvalidMaterial(); i = materialsystem->NextMaterial(i))
		{
			auto material = materialsystem->GetMaterial(i);

			if (!material)
				continue;

			if (strstr(material->GetTextureGroupName(), crypt_str("SkyBox")))
				material->ColorModulate(g_cfg.esp.skybox_color[0] / 255.0f, g_cfg.esp.skybox_color[1] / 255.0f, g_cfg.esp.skybox_color[2] / 255.0f);
		}
	}
}

void worldesp::fog_changer()
{
	static auto fog_override = m_cvar()->FindVar(crypt_str("fog_override")); 

	if (!g_cfg.esp.fog)
	{
		if (fog_override->GetBool())
			fog_override->SetValue(FALSE);

		return;
	}

	if (!fog_override->GetBool())
		fog_override->SetValue(TRUE);

	static auto fog_start = m_cvar()->FindVar(crypt_str("fog_start"));

	if (fog_start->GetInt())
		fog_start->SetValue(0);

	static auto fog_end = m_cvar()->FindVar(crypt_str("fog_end"));

	if (fog_end->GetInt() != g_cfg.esp.fog_distance)
		fog_end->SetValue(g_cfg.esp.fog_distance);

	static auto fog_maxdensity = m_cvar()->FindVar(crypt_str("fog_maxdensity"));

	if (fog_maxdensity->GetFloat() != (float)g_cfg.esp.fog_density * 0.01f) 
		fog_maxdensity->SetValue((float)g_cfg.esp.fog_density * 0.01f);

	char buffer_color[12];
	sprintf_s(buffer_color, 12, "%i %i %i", g_cfg.esp.fog_color.r(), g_cfg.esp.fog_color.g(), g_cfg.esp.fog_color.b());

	static auto fog_color = m_cvar()->FindVar(crypt_str("fog_color"));

	if (strcmp(fog_color->GetString(), buffer_color)) 
		fog_color->SetValue(buffer_color);
}

void worldesp::world_modulation(entity_t* entity)
{
	if (!g_cfg.esp.world_modulation)
		return;

	entity->set_m_bUseCustomBloomScale(TRUE);
	entity->set_m_flCustomBloomScale(g_cfg.esp.bloom * 0.01f);

	entity->set_m_bUseCustomAutoExposureMin(TRUE);
	entity->set_m_flCustomAutoExposureMin(g_cfg.esp.exposure * 0.001f);

	entity->set_m_bUseCustomAutoExposureMax(TRUE);
	entity->set_m_flCustomAutoExposureMax(g_cfg.esp.exposure * 0.001f);
}

void worldesp::molotov_timer(entity_t* entity)
{
	if (!g_cfg.esp.molotov_timer)
		return;

	auto inferno = reinterpret_cast<inferno_t*>(entity);

	auto origin = inferno->GetAbsOrigin();

	Vector screen_origin;

	if (!math::world_to_screen(origin, screen_origin))
		return;

	auto spawn_time = inferno->get_spawn_time();

	auto factor = (spawn_time + inferno_t::get_expiry_time() - m_globals()->m_curtime) / inferno_t::get_expiry_time();

	static const auto global_size = Vector2D(35.0f, 5.0f);

	auto color = g_cfg.esp.molotov_timer_color;

	auto distance = g_ctx.local()->m_vecOrigin().DistTo(origin) / 12;

	auto alpha_damage = 0.f;

	if (distance <= 20)
		alpha_damage = 255 - 255 * (distance / 20);

	g_render::get().ring3d(origin.x, origin.y, origin.z, 144, 256, Color(color.r(), color.g(), color.b(), 255), Color(color.r(), color.g(), color.b(), 35), 1, factor);

	g_render::get().text(g_render::get().grenades, screen_origin.x, screen_origin.y - global_size.y * 0.5f - 12, Color(color.r(), color.g(), color.b()), true, true, true, false, "l");
}

void worldesp::smoke_timer(entity_t* entity)
{
	if (!g_cfg.esp.smoke_timer)
		return;

	auto smoke = reinterpret_cast<smoke_t*>(entity);

	auto origin = smoke->GetAbsOrigin();

	if (!smoke->m_nSmokeEffectTickBegin() || !smoke->m_bDidSmokeEffect())
		return;

	Vector screen_origin;

	if (!math::world_to_screen(origin, screen_origin))
		return;

	auto spawn_time = TICKS_TO_TIME(smoke->m_nSmokeEffectTickBegin());

	auto factor = (spawn_time + smoke_t::get_expiry_time() - m_globals()->m_curtime) / smoke_t::get_expiry_time();

	static const auto global_size = Vector2D(35.0f, 5.0f);

	auto color = g_cfg.esp.smoke_timer_color;

	g_render::get().ring3d(origin.x, origin.y, origin.z, 144, 256, Color(color.r(), color.g(), color.b(), 255), Color(color.r(), color.g(), color.b(), 35), 1, factor);

	g_render::get().text(g_render::get().grenades, screen_origin.x, screen_origin.y - global_size.y * 0.5f - 12, Color(color.r(), color.g(), color.b()), true, true, true, false, "k");
}

void worldesp::grenade_projectiles(entity_t* entity)
{
	if (!g_cfg.esp.projectiles)
		return;

	auto client_class = entity->GetClientClass();

	if (!client_class)
		return;

	auto model = entity->GetModel();

	if (!model)
		return;

	auto studio_model = m_modelinfo()->GetStudioModel(model);

	if (!studio_model)
		return;

	auto name = studio_model->szName;

	if (strstr(name, "thrown") != NULL ||
		client_class->m_ClassID == CBaseCSGrenadeProjectile || client_class->m_ClassID == CDecoyProjectile || client_class->m_ClassID == CMolotovProjectile || client_class->m_ClassID == CSmokeGrenadeProjectile)
	{
		auto grenade_origin = entity->GetAbsOrigin();
		auto grenade_position = ZERO;

		if (!math::world_to_screen(grenade_origin, grenade_position))
			return;

		std::string grenade_name, grenade_icon;

		if (strstr(name, "flashbang") != NULL)
		{
			grenade_name = "Flashbang";
			grenade_icon = "i";
		}
		else if (strstr(name, "smokegrenade") != NULL)
		{
			grenade_name = "Smoke";
			grenade_icon = "k";
		}
		else if (strstr(name, "incendiarygrenade") != NULL)
		{
			grenade_name = "Incendiary";
			grenade_icon = "n";
		}
		else if (strstr(name, "molotov") != NULL)
		{
			grenade_name = "Molotov";
			grenade_icon = "l";
		}
		else if (strstr(name, "fraggrenade") != NULL)
		{
			grenade_name = "He grenade";
			grenade_icon = "j";
		}
		else if (strstr(name, "decoy") != NULL)
		{
			grenade_name = "Decoy";
			grenade_icon = "m";
		}
		else
			return;

		Box box;

		if (util::get_bbox(entity, box, false))
		{
			if (g_cfg.esp.grenade_esp[GRENADE_BOX])
			{
				g_render::get().rect(box.x, box.y, box.w, box.h, g_cfg.esp.grenade_box_color);

				if (g_cfg.esp.grenade_esp[GRENADE_ICON])
					g_render::get().text(g_render::get().grenades, box.x + box.w / 2, box.y - 21, g_cfg.esp.projectiles_color, true, false, true, true, grenade_icon.c_str());

				if (g_cfg.esp.grenade_esp[GRENADE_TEXT])
					g_render::get().text(g_render::get().world, box.x + box.w / 2, box.y + box.h + 2, g_cfg.esp.projectiles_color, true, false, true, true, grenade_name.c_str());
			}
			else
			{
				if (g_cfg.esp.grenade_esp[GRENADE_ICON] && g_cfg.esp.grenade_esp[GRENADE_TEXT])
				{
					g_render::get().text(g_render::get().grenades, box.x + box.w / 2, box.y + box.h / 2 - 10, g_cfg.esp.projectiles_color, true, false, true, true, grenade_icon.c_str());
					g_render::get().text(g_render::get().world, box.x + box.w / 2, box.y + box.h / 2 + 7, g_cfg.esp.projectiles_color, true, false, true, true, grenade_name.c_str());
				}
				else
				{
					if (g_cfg.esp.grenade_esp[GRENADE_ICON])
						g_render::get().text(g_render::get().grenades, box.x + box.w / 2, box.y + box.h / 2, g_cfg.esp.projectiles_color, true, true, true, true, grenade_icon.c_str());

					if (g_cfg.esp.grenade_esp[GRENADE_TEXT])
						g_render::get().text(g_render::get().world, box.x + box.w / 2, box.y + box.h / 2, g_cfg.esp.projectiles_color, true, true, true, true, grenade_name.c_str());
				}
			}
		}
	}
	else if (strstr(name, "dropped") != NULL)
	{

		if (strstr(name, "flashbang") != NULL ||
			strstr(name, "smokegrenade") != NULL ||
			strstr(name, "incendiarygrenade") != NULL ||
			strstr(name, "molotov") != NULL ||
			strstr(name, "fraggrenade") != NULL ||
			strstr(name, "decoy") != NULL)
		{

		}
		else
			return;

		auto weapon = (weapon_t*)entity; 
		Box box;

		if (util::get_bbox(weapon, box, false))
		{
			auto offset = 0;

			if (g_cfg.esp.weapon[WEAPON_BOX])
			{
				g_render::get().rect(box.x, box.y, box.w, box.h, g_cfg.esp.box_color);

				if (g_cfg.esp.weapon[WEAPON_ICON])
				{
					g_render::get().text(g_render::get().weapon_icons, box.x + box.w / 2, box.y - 14, g_cfg.esp.weapon_color, true, false, true, true, weapon->get_icon());
					offset = 14;
				}

				if (g_cfg.esp.weapon[WEAPON_TEXT])
					g_render::get().text(g_render::get().world, box.x + box.w / 2, box.y + box.h + 2, g_cfg.esp.weapon_color, true, false, true, false, weapon->get_name().c_str());

				if (g_cfg.esp.weapon[WEAPON_DISTANCE])
				{
					auto distance = g_ctx.local()->GetAbsOrigin().DistTo(weapon->GetAbsOrigin()) / 12.0f;
					g_render::get().text(g_render::get().world, box.x + box.w / 2, box.y - 13 - offset, g_cfg.esp.weapon_color, true, false, true, false, "%i ft", (int)distance);
				}
			}
			else
			{
				if (g_cfg.esp.weapon[WEAPON_ICON])
					g_render::get().text(g_render::get().weapon_icons, box.x + box.w / 2, box.y + box.h / 2 - 7, g_cfg.esp.weapon_color, true, false, true, true, weapon->get_icon());

				if (g_cfg.esp.weapon[WEAPON_TEXT])
					g_render::get().text(g_render::get().world, box.x + box.w / 2, box.y + box.h / 2 + 6, g_cfg.esp.weapon_color, true, false, true, false, weapon->get_name().c_str());

				if (g_cfg.esp.weapon[WEAPON_DISTANCE])
				{
					auto distance = g_ctx.local()->GetAbsOrigin().DistTo(weapon->GetAbsOrigin()) / 12.0f;

					if (g_cfg.esp.weapon[WEAPON_ICON] && g_cfg.esp.weapon[WEAPON_TEXT])
						offset = 21;
					else if (g_cfg.esp.weapon[WEAPON_ICON])
						offset = 21;
					else if (g_cfg.esp.weapon[WEAPON_TEXT])
						offset = 8;

					g_render::get().text(g_render::get().world, box.x + box.w / 2, box.y + box.h / 2 - offset, g_cfg.esp.weapon_color, true, false, true, false, "%i ft", (int)distance);
				}
			}
		}

	}
}

void worldesp::bomb_timer(entity_t* entity)
{
	if (!g_cfg.esp.bomb_timer)
		return;

	if (!g_ctx.globals.bomb_timer_enable)
		return;

	static auto mp_c4timer = m_cvar()->FindVar(crypt_str("mp_c4timer"));
	auto bomb = (CCSBomb*)entity;

	auto c4timer = mp_c4timer->GetFloat();
	auto bomb_timer = bomb->m_flC4Blow() - m_globals()->m_curtime;

	if (bomb_timer < 0.0f)
		return;

	static int width, height;
	m_engine()->GetScreenSize(width, height);

	auto factor = bomb_timer / c4timer * height;

	auto red_factor = (int)(255.0f - bomb_timer / c4timer * 255.0f);
	auto green_factor = (int)(bomb_timer / c4timer * 255.0f);

	g_render::get().rect_filled(0, height - factor, 26, factor, Color(red_factor, green_factor, 0, 100));

	auto text_position = height - factor + 11;

	if (text_position > height - 9)
		text_position = height - 9;

	g_render::get().text(g_render::get().indicators, 13, text_position, Color::White, true, true, true, true, "%0.1f", bomb_timer);

	Vector screen;

	if (math::world_to_screen(entity->GetAbsOrigin(), screen))
		g_render::get().text(g_render::get().indicators, screen.x, screen.y, Color(red_factor, green_factor, 0), true, true, true, true, "BOMB");
}

void worldesp::dropped_weapons(entity_t* entity)
{
	auto weapon = (weapon_t*)entity;
	auto owner = (player_t*)m_entitylist()->GetClientEntityFromHandle(weapon->m_hOwnerEntity());

	if (owner->is_player())
		return;

	Box box;

	if (util::get_bbox(weapon, box, false))
	{
		auto offset = 0;

		if (g_cfg.esp.weapon[WEAPON_BOX])
		{
			g_render::get().rect(box.x, box.y, box.w, box.h, g_cfg.esp.box_color);

			if (g_cfg.esp.weapon[WEAPON_ICON])
			{
				g_render::get().text(g_render::get().weapon_icons, box.x + box.w / 2, box.y - 14, g_cfg.esp.weapon_color, true, false, true, true, weapon->get_icon());
				offset = 14;
			}

			if (g_cfg.esp.weapon[WEAPON_TEXT])
				g_render::get().text(g_render::get().world, box.x + box.w / 2, box.y + box.h + 2, g_cfg.esp.weapon_color, true, false, true, false, weapon->get_name().c_str());

			if (g_cfg.esp.weapon[WEAPON_AMMO] && entity->GetClientClass()->m_ClassID != CBaseCSGrenadeProjectile && entity->GetClientClass()->m_ClassID != CSmokeGrenadeProjectile && entity->GetClientClass()->m_ClassID != CSensorGrenadeProjectile && entity->GetClientClass()->m_ClassID != CMolotovProjectile && entity->GetClientClass()->m_ClassID != CDecoyProjectile)
			{
				auto inner_back_color = Color::Black;
				inner_back_color.SetAlpha(153);

				g_render::get().rect_filled(box.x - 1, box.y + box.h + 14, box.w + 2, 4, inner_back_color);
				g_render::get().rect_filled(box.x, box.y + box.h + 15, weapon->m_iClip1() * box.w / weapon->get_csweapon_info()->iMaxClip1, 2, g_cfg.esp.weapon_ammo_color);
			}

			if (g_cfg.esp.weapon[WEAPON_DISTANCE])
			{
				auto distance = g_ctx.local()->GetAbsOrigin().DistTo(weapon->GetAbsOrigin()) / 12.0f;
				g_render::get().text(g_render::get().world, box.x + box.w / 2, box.y - 13 - offset, g_cfg.esp.weapon_color, true, false, true, false, "%i ft", (int)distance);
			}
		}
		else
		{
			if (g_cfg.esp.weapon[WEAPON_ICON])
				g_render::get().text(g_render::get().weapon_icons, box.x + box.w / 2, box.y + box.h / 2 - 7, g_cfg.esp.weapon_color, true, false, true, true, weapon->get_icon());

			if (g_cfg.esp.weapon[WEAPON_TEXT])
				g_render::get().text(g_render::get().world, box.x + box.w / 2, box.y + box.h / 2 + 6, g_cfg.esp.weapon_color, true, false, true, false, weapon->get_name().c_str());

			if (g_cfg.esp.weapon[WEAPON_AMMO] && entity->GetClientClass()->m_ClassID != CBaseCSGrenadeProjectile && entity->GetClientClass()->m_ClassID != CSmokeGrenadeProjectile && entity->GetClientClass()->m_ClassID != CSensorGrenadeProjectile && entity->GetClientClass()->m_ClassID != CMolotovProjectile && entity->GetClientClass()->m_ClassID != CDecoyProjectile)
			{
				static auto pos = 0;

				if (g_cfg.esp.weapon[WEAPON_ICON] && g_cfg.esp.weapon[WEAPON_TEXT])
					pos = 19;
				else if (g_cfg.esp.weapon[WEAPON_ICON])
					pos = 8;
				else if (g_cfg.esp.weapon[WEAPON_TEXT])
					pos = 19;

				auto inner_back_color = Color::Black;
				inner_back_color.SetAlpha(153);

				g_render::get().rect_filled(box.x - 1, box.y + box.h / 2 + pos - 1, box.w + 2, 4, inner_back_color);
				g_render::get().rect_filled(box.x, box.y + box.h / 2 + pos, weapon->m_iClip1() * box.w / weapon->get_csweapon_info()->iMaxClip1, 2, g_cfg.esp.weapon_ammo_color);
			}

			if (g_cfg.esp.weapon[WEAPON_DISTANCE])
			{
				auto distance = g_ctx.local()->GetAbsOrigin().DistTo(weapon->GetAbsOrigin()) / 12.0f;

				if (g_cfg.esp.weapon[WEAPON_ICON] && g_cfg.esp.weapon[WEAPON_TEXT])
					offset = 21;
				else if (g_cfg.esp.weapon[WEAPON_ICON])
					offset = 21;
				else if (g_cfg.esp.weapon[WEAPON_TEXT])
					offset = 8;

				g_render::get().text(g_render::get().world, box.x + box.w / 2, box.y + box.h / 2 - offset, g_cfg.esp.weapon_color, true, false, true, false, "%i FT", (int)distance);
			}
		}
	}
}

void worldesp::viewmodel_changer()
{
	if (g_cfg.esp.viewmodel_fov)
	{
		auto viewFOV = (float)g_cfg.esp.viewmodel_fov + 68.0f;
		static auto viewFOVcvar = m_cvar()->FindVar(crypt_str("viewmodel_fov"));

		if (viewFOVcvar->GetFloat() != viewFOV)
		{
			*(float*)((DWORD)&viewFOVcvar->m_fnChangeCallbacks + 0xC) = 0.0f;
			viewFOVcvar->SetValue(viewFOV);
		}
	}

	if (g_cfg.esp.viewmodel_x)
	{
		auto viewX = (float)g_cfg.esp.viewmodel_x / 2.0f;
		static auto viewXcvar = m_cvar()->FindVar(crypt_str("viewmodel_offset_x"));

		if (viewXcvar->GetFloat() != viewX)
		{
			*(float*)((DWORD)&viewXcvar->m_fnChangeCallbacks + 0xC) = 0.0f;
			viewXcvar->SetValue(viewX);
		}
	}

	if (g_cfg.esp.viewmodel_y)
	{
		auto viewY = (float)g_cfg.esp.viewmodel_y / 2.0f;
		static auto viewYcvar = m_cvar()->FindVar(crypt_str("viewmodel_offset_y"));

		if (viewYcvar->GetFloat() != viewY)
		{
			*(float*)((DWORD)&viewYcvar->m_fnChangeCallbacks + 0xC) = 0.0f;
			viewYcvar->SetValue(viewY);
		}
	}

	if (g_cfg.esp.viewmodel_z)
	{
		auto viewZ = (float)g_cfg.esp.viewmodel_z / 2.0f;
		static auto viewZcvar = m_cvar()->FindVar(crypt_str("viewmodel_offset_z"));

		if (viewZcvar->GetFloat() != viewZ)
		{
			*(float*)((DWORD)&viewZcvar->m_fnChangeCallbacks + 0xC) = 0.0f;
			viewZcvar->SetValue(viewZ);
		}
	}
}