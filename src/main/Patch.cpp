#include "Patch.h"

bool Patch::WriteDiscoveryMusicPatch(AssignMusicCallback* a_callback)
{
	REL::Relocation<std::uintptr_t> hook{ Offset::HUDNotifications::ProcessMessage, 0x3B3 };

	struct Patch : Xbyak::CodeGenerator
	{
		Patch(std::uintptr_t a_hookAddr, std::uintptr_t a_funcAddr)
		{
			Xbyak::Label funcLbl;
			Xbyak::Label retnLbl;

			mov(edx, ptr[rdi + 0x44]);
			lea(rcx, ptr[rbp - 0x29]);
			call(ptr[rip + funcLbl]);
			jmp(ptr[rip + retnLbl]);

			L(funcLbl);
			dq(a_funcAddr);

			L(retnLbl);
			dq(a_hookAddr + 0x16A);
		}
	};

	std::uintptr_t funcAddr = reinterpret_cast<std::uintptr_t>(a_callback);
	Patch patch{ hook.address(), funcAddr };
	patch.ready();

	if (patch.getSize() > 0x6B) {
		logger::critical("Patch was too large, failed to install"sv);
		return false;
	}

	REL::safe_write(hook.address(), patch.getCode(), patch.getSize());
	return true;
}

bool Patch::WriteLoadHUDPatch(LoadMovieFunc* a_newCall, REL::Relocation<LoadMovieFunc>& a_origCall)
{
	auto& trampoline = SKSE::GetTrampoline();

	auto hud_hook = REL::Relocation<std::uintptr_t>{ Offset::HUDMenu::Ctor, 0xFF };

	constexpr auto relcall = REL::make_pattern<"E8">();

	if (relcall.match(hud_hook.address())) {
		logger::trace("Using normal address for HUD hook"sv);
		a_origCall = trampoline.write_call<5>(hud_hook.address(), a_newCall);
		return true;
	}
	else {
		logger::warn("Failed to install HUD hook"sv);
		return false;
	}
}

bool Patch::WriteLoadMapPatch(LoadMovieFunc* a_newCall, REL::Relocation<LoadMovieFunc>& a_origCall)
{
	auto& trampoline = SKSE::GetTrampoline();

	auto map_hook = REL::Relocation<std::uintptr_t>{ Offset::MapMenu::Ctor, 0x1CF };

	constexpr auto relcall = REL::make_pattern<"E8">();

	if (relcall.match(map_hook.address())) {
		a_origCall = trampoline.write_call<5>(map_hook.address(), a_newCall);
		return true;
	}
	else {
		logger::warn("Failed to install Map hook"sv);
		return false;
	}
}

bool Patch::WriteLocalMapPatch(
	GetSpecialMarkerFunc* a_newCall,
	REL::Relocation<GetSpecialMarkerFunc>& a_origCall)
{
	auto& trampoline = SKSE::GetTrampoline();

	auto door_hook = REL::Relocation<std::uintptr_t>{ Offset::LocalMapMenu::PopulateData, 0x7B5 };

	constexpr auto relcall = REL::make_pattern<"E8">();

	if (relcall.match(door_hook.address())) {
		a_origCall = trampoline.write_call<5>(door_hook.address(), a_newCall);
		return true;
	}
	else {
		logger::warn("Failed to install Local Map hook"sv);
		return false;
	}
}
