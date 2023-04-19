#include <src/plugins/PluginAPI.hpp>

#include "globals.hpp"
#include "Hy3Layout.hpp"
#include "src/Compositor.hpp"
#include "src/config/ConfigManager.hpp"

inline std::unique_ptr<Hy3Layout> g_Hy3Layout;

APICALL EXPORT std::string PLUGIN_API_VERSION() {
	return HYPRLAND_API_VERSION;
}

// return a window if a window action makes sense now
CWindow* window_for_action() {
	if (g_pLayoutManager->getCurrentLayout() != g_Hy3Layout.get()) return nullptr;

	auto* window = g_pCompositor->m_pLastWindow;

	if (!window) return nullptr;

	const auto workspace = g_pCompositor->getWorkspaceByID(window->m_iWorkspaceID);
	if (workspace->m_bHasFullscreenWindow) return nullptr;

	return window;
}

void dispatch_makegroup(std::string arg) {
	CWindow* window = window_for_action();
	if (window == nullptr) return;

	if (arg == "h") {
		g_Hy3Layout->makeGroupOn(window, Hy3GroupLayout::SplitH);
	} else if (arg == "v") {
		g_Hy3Layout->makeGroupOn(window, Hy3GroupLayout::SplitV);
	}
}

void dispatch_movewindow(std::string arg) {
	CWindow* window = window_for_action();
	if (window == nullptr) return;

	if (arg == "l") {
		g_Hy3Layout->shiftWindow(window, ShiftDirection::Left);
	} else if (arg == "u") {
		g_Hy3Layout->shiftWindow(window, ShiftDirection::Up);
	} else if (arg == "d") {
		g_Hy3Layout->shiftWindow(window, ShiftDirection::Down);
	} else if (arg == "r") {
		g_Hy3Layout->shiftWindow(window, ShiftDirection::Right);
	}
}

void dispatch_movefocus(std::string arg) {
	CWindow* window = window_for_action();
	if (window == nullptr) return;

	if (arg == "l") {
		g_Hy3Layout->shiftFocus(window, ShiftDirection::Left);
	} else if (arg == "u") {
		g_Hy3Layout->shiftFocus(window, ShiftDirection::Up);
	} else if (arg == "d") {
		g_Hy3Layout->shiftFocus(window, ShiftDirection::Down);
	} else if (arg == "r") {
		g_Hy3Layout->shiftFocus(window, ShiftDirection::Right);
	}
}

void dispatch_debug(std::string arg) {
	CWindow* window = window_for_action();
	if (window == nullptr) return;

	auto* root = g_Hy3Layout->getWorkspaceRootGroup(window->m_iWorkspaceID);
	if (window == nullptr) {
		Debug::log(LOG, "DEBUG NODES: no nodes on workspace");
	} else {
		Debug::log(LOG, "DEBUG NODES\n%s", root->debugNode().c_str());
	}
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
	PHANDLE = handle;

	HyprlandAPI::addConfigValue(PHANDLE, "plugin:hy3:no_gaps_when_only", SConfigValue{.intValue = 0});

	g_Hy3Layout = std::make_unique<Hy3Layout>();
	HyprlandAPI::addLayout(PHANDLE, "hy3", g_Hy3Layout.get());

	HyprlandAPI::addDispatcher(PHANDLE, "hy3_makegroup", dispatch_makegroup);
	HyprlandAPI::addDispatcher(PHANDLE, "hy3_movefocus", dispatch_movefocus);
	HyprlandAPI::addDispatcher(PHANDLE, "hy3_movewindow", dispatch_movewindow);
	HyprlandAPI::addDispatcher(PHANDLE, "hy3_debugnodes", dispatch_debug);

	return {"hy3", "i3 like layout for hyprland", "outfoxxed", "0.1"};
}

APICALL EXPORT void PLUGIN_EXIT() {}
