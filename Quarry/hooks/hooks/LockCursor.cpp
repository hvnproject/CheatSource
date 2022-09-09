#include "..\hooks.hpp"

using LockCursor_t = void(__thiscall*)(void*);

void __stdcall hooks::hkLockCursor()
{
	static auto original_fn = surface_hook->get_func_address <LockCursor_t>(67);

	if (!menu_open)
		return original_fn(m_surface());

	m_surface()->UnlockCursor();
}