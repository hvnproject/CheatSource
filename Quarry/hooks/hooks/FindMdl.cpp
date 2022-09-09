#include "..\hooks.hpp"

using FindMDL_t = void(__thiscall*)(void*, char*);

void __fastcall hooks::hkFindMdl(void* ecx, void* edx, char* FilePath)
{
	static auto original_fn = modelcache_hook->get_func_address <FindMDL_t> (10);
	return original_fn(ecx, FilePath);
}