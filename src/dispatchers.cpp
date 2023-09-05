#include <optional>

#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>

#include "dispatchers.hpp"
#include "globals.hpp"

int workspace_for_action() {
	if (g_pLayoutManager->getCurrentLayout() != g_Hy3Layout.get()) return -1;
	int specialWorkspaceID = g_pCompositor->m_pLastMonitor->specialWorkspaceID;
	int workspace_id
	    = specialWorkspaceID ? specialWorkspaceID : g_pCompositor->m_pLastMonitor->activeWorkspace;

	if (workspace_id == -1) return -1;
	auto* workspace = g_pCompositor->getWorkspaceByID(workspace_id);
	if (workspace == nullptr) return -1;
	if (workspace->m_bHasFullscreenWindow) return -1;

	return workspace_id;
}

void dispatch_makegroup(std::string value) {
	int workspace = workspace_for_action();
	if (workspace == -1) return;

	auto args = CVarList(value);

	GroupEphemeralityOption ephemeral = GroupEphemeralityOption::Standard;
	if (args[1] == "ephemeral") {
		ephemeral = GroupEphemeralityOption::Ephemeral;
	} else if (args[1] == "force_ephemeral") {
		ephemeral = GroupEphemeralityOption::ForceEphemeral;
	}

	if (args[0] == "h") {
		g_Hy3Layout->makeGroupOnWorkspace(workspace, Hy3GroupLayout::SplitH, ephemeral);
	} else if (args[0] == "v") {
		g_Hy3Layout->makeGroupOnWorkspace(workspace, Hy3GroupLayout::SplitV, ephemeral);
	} else if (args[0] == "tab") {
		g_Hy3Layout->makeGroupOnWorkspace(workspace, Hy3GroupLayout::Tabbed, ephemeral);
	} else if (args[0] == "opposite") {
		g_Hy3Layout->makeOppositeGroupOnWorkspace(workspace, ephemeral);
	}
}

void dispatch_changegroup(std::string value) {
	int workspace = workspace_for_action();
	if (workspace == -1) return;

	auto args = CVarList(value);

	if (args[0] == "h") {
		g_Hy3Layout->changeGroupOnWorkspace(workspace, Hy3GroupLayout::SplitH);
	} else if (args[0] == "v") {
		g_Hy3Layout->changeGroupOnWorkspace(workspace, Hy3GroupLayout::SplitV);
	} else if (args[0] == "tab") {
		g_Hy3Layout->changeGroupOnWorkspace(workspace, Hy3GroupLayout::Tabbed);
	} else if (args[0] == "untab") {
		g_Hy3Layout->untabGroupOnWorkspace(workspace);
	} else if (args[0] == "toggletab") {
		g_Hy3Layout->toggleTabGroupOnWorkspace(workspace);
	} else if (args[0] == "opposite") {
		g_Hy3Layout->changeGroupToOppositeOnWorkspace(workspace);
	}
}

void dispatch_setephemeral(std::string value) {
	int workspace = workspace_for_action();
	if (workspace == -1) return;

	auto args = CVarList(value);

	bool ephemeral = args[0] == "true";

	g_Hy3Layout->changeGroupEphemeralityOnWorkspace(workspace, ephemeral);
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
		int i = 1;
		bool once = false;
		bool visible = false;

		if (args[i] == "once") {
			once = true;
			i++;
		}

		if (args[i] == "visible") {
			visible = true;
			i++;
		}

		g_Hy3Layout->shiftWindow(workspace, shift.value(), once, visible);
	}
}

void dispatch_movefocus(std::string value) {
	int workspace = workspace_for_action();
	if (workspace == -1) return;

	auto args = CVarList(value);

	if (auto shift = parseShiftArg(args[0])) {
		g_Hy3Layout->shiftFocus(workspace, shift.value(), args[1] == "visible");
	}
}

void dispatch_changefocus(std::string arg) {
	int workspace = workspace_for_action();
	if (workspace == -1) return;

	if (arg == "top") g_Hy3Layout->changeFocus(workspace, FocusShift::Top);
	else if (arg == "bottom") g_Hy3Layout->changeFocus(workspace, FocusShift::Bottom);
	else if (arg == "raise") g_Hy3Layout->changeFocus(workspace, FocusShift::Raise);
	else if (arg == "lower") g_Hy3Layout->changeFocus(workspace, FocusShift::Lower);
	else if (arg == "tab") g_Hy3Layout->changeFocus(workspace, FocusShift::Tab);
	else if (arg == "tabnode") g_Hy3Layout->changeFocus(workspace, FocusShift::TabNode);
}

void dispatch_focustab(std::string value) {
	int workspace = workspace_for_action();
	if (workspace == -1) return;

	auto i = 0;
	auto args = CVarList(value);

	TabFocus focus;
	auto mouse = TabFocusMousePriority::Ignore;
	bool wrap_scroll = false;
	int index = 0;

	if (args[i] == "l" || args[i] == "left") focus = TabFocus::Left;
	else if (args[i] == "r" || args[i] == "right") focus = TabFocus::Right;
	else if (args[i] == "index") {
		i++;
		focus = TabFocus::Index;
		if (!isNumber(args[i])) return;
		index = std::stoi(args[i]);
		Debug::log(LOG, "Focus index '%s' -> %d, errno: %d", args[i].c_str(), index, errno);
	} else if (args[i] == "mouse") {
		g_Hy3Layout->focusTab(workspace, TabFocus::MouseLocation, mouse, false, 0);
		return;
	} else return;

	i++;

	if (args[i] == "prioritize_hovered") {
		mouse = TabFocusMousePriority::Prioritize;
		i++;
	} else if (args[i] == "require_hovered") {
		mouse = TabFocusMousePriority::Require;
		i++;
	}

	if (args[i++] == "wrap") wrap_scroll = true;

	g_Hy3Layout->focusTab(workspace, focus, mouse, wrap_scroll, index);
}

void dispatch_setswallow(std::string arg) {
	int workspace = workspace_for_action();
	if (workspace == -1) return;

	SetSwallowOption option;
	if (arg == "true") {
		option = SetSwallowOption::Swallow;
	} else if (arg == "false") {
		option = SetSwallowOption::NoSwallow;
	} else if (arg == "toggle") {
		option = SetSwallowOption::Toggle;
	} else return;

	g_Hy3Layout->setNodeSwallow(workspace, option);
}

void dispatch_killactive(std::string value) {
	int workspace = workspace_for_action();
	if (workspace == -1) return;

	g_Hy3Layout->killFocusedNode(workspace);
}

void dispatch_expand(std::string value) {
	int workspace = workspace_for_action();
	if (workspace == -1) return;

	auto args = CVarList(value);

	ExpandOption expand;
	ExpandFullscreenOption fs_expand = ExpandFullscreenOption::MaximizeIntermediate;

	if (args[0] == "expand") expand = ExpandOption::Expand;
	else if (args[0] == "shrink") expand = ExpandOption::Shrink;
	else if (args[0] == "base") expand = ExpandOption::Base;
	else if (args[0] == "maximize") expand = ExpandOption::Maximize;
	else if (args[0] == "fullscreen") expand = ExpandOption::Fullscreen;
	else return;

	if (args[1] == "intermediate_maximize") fs_expand = ExpandFullscreenOption::MaximizeIntermediate;
	else if (args[1] == "fullscreen_maximize")
		fs_expand = ExpandFullscreenOption::MaximizeAsFullscreen;
	else if (args[1] == "maximize_only") fs_expand = ExpandFullscreenOption::MaximizeOnly;
	else if (args[1] != "") return;

	g_Hy3Layout->expand(workspace, expand, fs_expand);
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

void registerDispatchers() {
	HyprlandAPI::addDispatcher(PHANDLE, "hy3:makegroup", dispatch_makegroup);
	HyprlandAPI::addDispatcher(PHANDLE, "hy3:changegroup", dispatch_changegroup);
	HyprlandAPI::addDispatcher(PHANDLE, "hy3:setephemeral", dispatch_setephemeral);
	HyprlandAPI::addDispatcher(PHANDLE, "hy3:movefocus", dispatch_movefocus);
	HyprlandAPI::addDispatcher(PHANDLE, "hy3:movewindow", dispatch_movewindow);
	HyprlandAPI::addDispatcher(PHANDLE, "hy3:changefocus", dispatch_changefocus);
	HyprlandAPI::addDispatcher(PHANDLE, "hy3:focustab", dispatch_focustab);
	HyprlandAPI::addDispatcher(PHANDLE, "hy3:setswallow", dispatch_setswallow);
	HyprlandAPI::addDispatcher(PHANDLE, "hy3:killactive", dispatch_killactive);
	HyprlandAPI::addDispatcher(PHANDLE, "hy3:expand", dispatch_expand);
	HyprlandAPI::addDispatcher(PHANDLE, "hy3:debugnodes", dispatch_debug);
}
