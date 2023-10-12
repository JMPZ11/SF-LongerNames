
#include "PCH.h"

namespace
{
	struct InputBoxCave : Xbyak::CodeGenerator
	{
		InputBoxCave(int maxNameLength)
		{
			mov(r8d, maxNameLength);
		}
	};

	struct ItemNameLengthCheckCave : Xbyak::CodeGenerator
	{
		ItemNameLengthCheckCave(int maxNameLength)
		{
			cmp(edi, maxNameLength);
		}
	};

	void Hook_Placeholder()
	{
	}

	void PatchInputBox()
	{
		auto inputBoxPatchAddress = reinterpret_cast<uintptr_t>(search_pattern<"48 8B 88 ?? ?? ?? ?? 44 89 81 ?? ?? ?? ?? C3">());
		if (inputBoxPatchAddress) {
			inputBoxPatchAddress += 7;
			INFO("Found the input box patch address");
			DEBUG("Input box patch address: {:x}", inputBoxPatchAddress);

			InputBoxCave inputBoxCave{ Settings::GetSingleton()->GetMaxNameLength() };
			inputBoxCave.ready();
			
			const auto caveHookHandle = AddCaveHook(
				inputBoxPatchAddress,
				{ 0, 7 },
				FUNC_INFO(Hook_Placeholder),
				&inputBoxCave,
				nullptr,
				HookFlag::kRestoreAfterProlog);
			caveHookHandle->Enable();
			INFO("Input Box patched")
		} else {
			ERROR("Couldn't find the input box patch address");
		}
	}

	void PatchItemNameLengthCheck()
	{
		auto itemNameLengthAddress = reinterpret_cast<uintptr_t>(search_pattern<"3B 3D ?? ?? ?? ?? 0F 87 ?? ?? ?? ?? F6 43 14 02">());
		if (itemNameLengthAddress) {
			INFO("Found the item name length check address");
			DEBUG("Item name length check address: {:x}", itemNameLengthAddress);

			ItemNameLengthCheckCave itemNameLengthCheckCave{ Settings::GetSingleton()->GetMaxNameLength() };
			itemNameLengthCheckCave.ready();

			const auto inputBoxPatch = AddASMPatch(itemNameLengthAddress, { 0, 6 }, &itemNameLengthCheckCave);
			inputBoxPatch->Enable();
			INFO("Item name length check patched")
		}
		else {
			ERROR("Couldn't find the item name length check address");
		}
	}
}

DLLEXPORT constinit auto SFSEPlugin_Version = []() noexcept {
	SFSE::PluginVersionData data{};

	data.PluginVersion(Plugin::Version);
	data.PluginName(Plugin::NAME);
	data.AuthorName(Plugin::AUTHOR);
	data.UsesSigScanning(true);
	//data.UsesAddressLibrary(true);
	data.HasNoStructUse(true);
	//data.IsLayoutDependent(true);
	data.CompatibleVersions({
	    SFSE::RUNTIME_SF_1_7_23,
		SFSE::RUNTIME_SF_1_7_29,
		SFSE::RUNTIME_SF_1_7_33,
		SFSE::RUNTIME_SF_1_7_36,
		SFSE::RUNTIME_LATEST
	});

	return data;
}();

namespace
{
	void MessageCallback(SFSE::MessagingInterface::Message* a_msg) noexcept
	{
		switch (a_msg->type) {
		case SFSE::MessagingInterface::kPostLoad:
			{
				// Load the settings file
				Settings::GetSingleton()->Load();
				// Apply patches
				PatchInputBox();
				PatchItemNameLengthCheck();
				break;
			}
		default:
			break;
		}
	}
}

DLLEXPORT bool SFSEAPI SFSEPlugin_Load(const SFSE::LoadInterface* a_sfse)
{
#ifndef NDEBUG
	MessageBoxA(NULL, "Loaded. You can attach the debugger now or continue", Plugin::NAME.data(), NULL);
#endif

	SFSE::Init(a_sfse, false);

	DKUtil::Logger::Init(Plugin::NAME, std::to_string(Plugin::Version));

	INFO("{} v{} loaded", Plugin::NAME, Plugin::Version);

	// Insert plugin to messaging interface
	SFSE::AllocTrampoline(1 << 8);
	SFSE::GetMessagingInterface()->RegisterListener(MessageCallback);

	return true;
}
