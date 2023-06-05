#include <optional>

#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/Compositor.hpp>

#include "globals.hpp"
#include "SelectionHook.hpp"

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

	if (workspace_id == -1) return -1;
	auto* workspace = g_pCompositor->getWorkspaceByID(workspace_id);
	if (workspace == nullptr) return -1;
	if (workspace->m_bHasFullscreenWindow) return -1;

	return workspace_id;
}

void dispatch_makegroup(std::string arg) {
	int workspace = workspace_for_action();
	if (workspace == -1) return;

	if (arg == "h") {
		g_Hy3Layout->makeGroupOnWorkspace(workspace, Hy3GroupLayout::SplitH);
	} else if (arg == "v") {
		g_Hy3Layout->makeGroupOnWorkspace(workspace, Hy3GroupLayout::SplitV);
	} else if (arg == "tab") {
		g_Hy3Layout->makeGroupOnWorkspace(workspace, Hy3GroupLayout::Tabbed);
	} else if (arg == "opposite") {
		g_Hy3Layout->makeOppositeGroupOnWorkspace(workspace);
	}
}

std::optional<ShiftDirection> parseShiftArg(std::string arg) {
	if (arg == "l" || arg == "left") return ShiftDirection::Left;
	else if (arg == "r" || arg == "right") return ShiftDirection::Right;
	else if (arg == "u" || arg == "up") return ShiftDirection::Up;
	else if (arg == "d" || arg == "down") return ShiftDirection::Down;
	else return {};
}

void dispatch_movewindow(std::string value) {
	int workspace = workspace_for_action();
	if (workspace == -1) return;

	auto args = CVarList(value);

	if (auto shift = parseShiftArg(args[0])) {
		auto once = args[1] == "once";
		g_Hy3Layout->shiftWindow(workspace, shift.value(), once);
	}
}

void dispatch_movefocus(std::string arg) {
	int workspace = workspace_for_action();
	if (workspace == -1) return;

	if (auto shift = parseShiftArg(arg)) {
		g_Hy3Layout->shiftFocus(workspace, shift.value());
	}
}

void dispatch_raisefocus(std::string arg) {
	int workspace = workspace_for_action();
	if (workspace == -1) return;

	g_Hy3Layout->raiseFocus(workspace);
}

void dispatch_focustab(std::string arg) {
	int workspace = workspace_for_action();
	if (workspace == -1) return;

	g_Hy3Layout->focusTab(workspace);
}

void dispatch_debug(std::string arg) {
	int workspace = workspace_for_action();
	if (workspace == -1) return;

	auto* root = g_Hy3Layout->getWorkspaceRootGroup(workspace);
	if (workspace == -1) {
		Debug::log(LOG, "DEBUG NODES: no nodes on workspace");
	} else {
		Debug::log(LOG, "DEBUG NODES\n%s", root->debugNode().c_str());
	}
}

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
	PHANDLE = handle;

	selection_hook::init();

	HyprlandAPI::addConfigValue(PHANDLE, "plugin:hy3:no_gaps_when_only", SConfigValue{.intValue = 0});
	HyprlandAPI::addConfigValue(PHANDLE, "plugin:hy3:group_inset", SConfigValue{.intValue = 10});
	HyprlandAPI::addConfigValue(PHANDLE, "plugin:hy3:tabs:height", SConfigValue{.intValue = 15});
	HyprlandAPI::addConfigValue(PHANDLE, "plugin:hy3:tabs:padding", SConfigValue{.intValue = 5});
	HyprlandAPI::addConfigValue(PHANDLE, "plugin:hy3:tabs:from_top", SConfigValue{.intValue = 0});
	HyprlandAPI::addConfigValue(PHANDLE, "plugin:hy3:tabs:rounding", SConfigValue{.intValue = 3});
	HyprlandAPI::addConfigValue(PHANDLE, "plugin:hy3:tabs:render_text", SConfigValue{.intValue = 1});
	HyprlandAPI::addConfigValue(PHANDLE, "plugin:hy3:tabs:text_font", SConfigValue{.strValue = "Sans"});
	HyprlandAPI::addConfigValue(PHANDLE, "plugin:hy3:tabs:text_height", SConfigValue{.intValue = 8});
	HyprlandAPI::addConfigValue(PHANDLE, "plugin:hy3:tabs:text_padding", SConfigValue{.intValue = 3});
	HyprlandAPI::addConfigValue(PHANDLE, "plugin:hy3:tabs:col.active", SConfigValue{.intValue = 0xff32b4ff});
	HyprlandAPI::addConfigValue(PHANDLE, "plugin:hy3:tabs:col.urgent", SConfigValue{.intValue = 0xffff4f4f});
	HyprlandAPI::addConfigValue(PHANDLE, "plugin:hy3:tabs:col.inactive", SConfigValue{.intValue = 0x80808080});
	HyprlandAPI::addConfigValue(PHANDLE, "plugin:hy3:tabs:col.text.active", SConfigValue{.intValue = 0xff000000});
	HyprlandAPI::addConfigValue(PHANDLE, "plugin:hy3:tabs:col.text.urgent", SConfigValue{.intValue = 0xff000000});
	HyprlandAPI::addConfigValue(PHANDLE, "plugin:hy3:tabs:col.text.inactive", SConfigValue{.intValue = 0xff000000});

	g_Hy3Layout = std::make_unique<Hy3Layout>();
	HyprlandAPI::addLayout(PHANDLE, "hy3", g_Hy3Layout.get());

	HyprlandAPI::addDispatcher(PHANDLE, "hy3:makegroup", dispatch_makegroup);
	HyprlandAPI::addDispatcher(PHANDLE, "hy3:movefocus", dispatch_movefocus);
	HyprlandAPI::addDispatcher(PHANDLE, "hy3:movewindow", dispatch_movewindow);
	HyprlandAPI::addDispatcher(PHANDLE, "hy3:raisefocus", dispatch_raisefocus);
	HyprlandAPI::addDispatcher(PHANDLE, "hy3:focustab", dispatch_focustab);
	HyprlandAPI::addDispatcher(PHANDLE, "hy3:debugnodes", dispatch_debug);

	return {"hy3", "i3 like layout for hyprland", "outfoxxed", "0.1"};
}

APICALL EXPORT void PLUGIN_EXIT() {}
