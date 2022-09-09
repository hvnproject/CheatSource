#include "..\hooks.hpp"

bool __fastcall hooks::hkDrawFog(void* ecx, void* edx)
{
	return !g_cfg.esp.removals[REMOVALS_FOGS] || g_cfg.esp.fog;
}