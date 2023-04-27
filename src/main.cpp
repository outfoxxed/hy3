#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/Compositor.hpp>

#include "globals.hpp"


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

int workspace_for_action() {
	if (g_pLayoutManager->getCurrentLayout() != g_Hy3Layout.get()) return -1;

	int workspace_id = g_pCompositor->m_pLastMonitor->activeWorkspace;

	if (workspace_id < 0) return -1;
	auto* workspace = g_pCompositor->getWorkspaceByID(workspace_id);
	if (workspace == nullptr) return -1;
	if (workspace->m_bHasFullscreenWindow) return -1;

	return workspace_id;
}

void dispatch_makegroup(std::string arg) {
	int workspace = workspace_for_action();
	if (workspace < 0) return;

	if (arg == "h") {
		g_Hy3Layout->makeGroupOn(workspace, Hy3GroupLayout::SplitH);
	} else if (arg == "v") {
		g_Hy3Layout->makeGroupOn(workspace, Hy3GroupLayout::SplitV);
	}
}

void dispatch_movewindow(std::string arg) {
	int workspace = workspace_for_action();
	if (workspace < 0) return;

	if (arg == "l") {
		g_Hy3Layout->shiftWindow(workspace, ShiftDirection::Left);
	} else if (arg == "u") {
		g_Hy3Layout->shiftWindow(workspace, ShiftDirection::Up);
	} else if (arg == "d") {
		g_Hy3Layout->shiftWindow(workspace, ShiftDirection::Down);
	} else if (arg == "r") {
		g_Hy3Layout->shiftWindow(workspace, ShiftDirection::Right);
	}
}

void dispatch_movefocus(std::string arg) {
	int workspace = workspace_for_action();
	if (workspace < 0) return;

	if (arg == "l") {
		g_Hy3Layout->shiftFocus(workspace, ShiftDirection::Left);
	} else if (arg == "u") {
		g_Hy3Layout->shiftFocus(workspace, ShiftDirection::Up);
	} else if (arg == "d") {
		g_Hy3Layout->shiftFocus(workspace, ShiftDirection::Down);
	} else if (arg == "r") {
		g_Hy3Layout->shiftFocus(workspace, ShiftDirection::Right);
	}
}

void dispatch_raisefocus(std::string arg) {
	int workspace = workspace_for_action();
	if (workspace < 0) return;

	g_Hy3Layout->raiseFocus(workspace);
}

void dispatch_debug(std::string arg) {
	int workspace = workspace_for_action();
	if (workspace < 0) return;

	auto* root = g_Hy3Layout->getWorkspaceRootGroup(workspace);
	if (workspace < 0) {
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

	HyprlandAPI::addDispatcher(PHANDLE, "hy3:makegroup", dispatch_makegroup);
	HyprlandAPI::addDispatcher(PHANDLE, "hy3:movefocus", dispatch_movefocus);
	HyprlandAPI::addDispatcher(PHANDLE, "hy3:movewindow", dispatch_movewindow);
	HyprlandAPI::addDispatcher(PHANDLE, "hy3:raisefocus", dispatch_raisefocus);
	HyprlandAPI::addDispatcher(PHANDLE, "hy3:debugnodes", dispatch_debug);

	return {"hy3", "i3 like layout for hyprland", "outfoxxed", "0.1"};
}

APICALL EXPORT void PLUGIN_EXIT() {}
