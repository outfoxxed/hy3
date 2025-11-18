#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>

#include "globals.hpp"

namespace selection_hook {
inline CFunctionHook* g_LastSelectionHook = nullptr;

void hook_updateDecos(void* thisptr) {
	auto* window = static_cast<CWindow*>(thisptr);
	bool explicitly_selected = g_Hy3Layout->shouldRenderSelected(window);

	auto lastWindow = g_pCompositor->m_lastWindow;
	if (explicitly_selected) {
		g_pCompositor->m_lastWindow = window->m_self;
	}

	((void (*)(void*)) g_LastSelectionHook->m_original)(thisptr);

	if (explicitly_selected) {
		g_pCompositor->m_lastWindow = lastWindow;
	}
}

void init() {
	static const auto decoUpdateCandidates =
	    HyprlandAPI::findFunctionsByName(PHANDLE, "updateDecorationValues");

	if (decoUpdateCandidates.size() != 1) {
		g_LastSelectionHook = nullptr;

		hy3_log(
		    ERR,
		    "expected one matching function to hook for"
		    "\"updateDecorationValues\", found {}",
		    decoUpdateCandidates.size()
		);

		HyprlandAPI::addNotificationV2(
		    PHANDLE,
		    {
		        {"text",
		         "Failed to load function hooks: "
		         "\"updateDecorationValues\""},
		        {"time", (uint64_t) 10000},
		        {"color", CHyprColor(1.0, 0.0, 0.0, 1.0)},
		        {"icon", ICON_ERROR},
		    }
		);
		return;
	}

	g_LastSelectionHook = HyprlandAPI::createFunctionHook(
	    PHANDLE,
	    decoUpdateCandidates[0].address,
	    (void*) &hook_updateDecos
	);
}

void enable() {
	if (g_LastSelectionHook != nullptr) {
		g_LastSelectionHook->hook();
	}
}

void disable() {
	if (g_LastSelectionHook != nullptr) {
		g_LastSelectionHook->unhook();
	}
}
} // namespace selection_hook
