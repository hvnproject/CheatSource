#include "..\hooks.hpp"

using BeginFrame_t = void(__thiscall*)(void*, float);

void __fastcall hooks::hkBeginFrame(void* ecx, void* edx, float ft)
{
	static auto original_fn = materialsys_hook->get_func_address <BeginFrame_t>(42);
	return original_fn(ecx, ft);
}

_declspec(noinline)const char* hooks::hkGetForignFallbackfontname_detour(void* ecx, uint32_t i)
{
	if (g_ctx.last_font_name.empty())
		return ((GetForeignFallbackFontNameFn)original_getforeignfallbackfontname)(ecx);

	return g_ctx.last_font_name.c_str();
}

const char* __fastcall hooks::hkGetForignFallbackfontname(void* ecx, uint32_t i)
{
	return hkGetForignFallbackfontname_detour(ecx, i);
}