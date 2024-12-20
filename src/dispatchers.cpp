#include <optional>

#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/desktop/DesktopTypes.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprutils/string/String.hpp>

#include "dispatchers.hpp"
#include "globals.hpp"

PHLWORKSPACE workspace_for_action(bool allow_fullscreen = false) {
	if (g_pLayoutManager->getCurrentLayout() != g_Hy3Layout.get()) return nullptr;

	auto workspace = g_pCompositor->m_pLastMonitor->activeSpecialWorkspace;
	if (!valid(workspace)) workspace = g_pCompositor->m_pLastMonitor->activeWorkspace;

	if (!valid(workspace)) return nullptr;
	if (!allow_fullscreen && workspace->m_bHasFullscreenWindow) return nullptr;

	return workspace;
}

static SDispatchResult dispatch_makegroup(std::string value) {
	auto workspace = workspace_for_action();
	if (!valid(workspace)) return SDispatchResult{};

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
	return SDispatchResult{};
}

static SDispatchResult dispatch_changegroup(std::string value) {
	auto workspace = workspace_for_action();
	if (!valid(workspace)) return SDispatchResult{};

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
	return SDispatchResult{};
}

static SDispatchResult dispatch_setephemeral(std::string value) {
	auto workspace = workspace_for_action();
	if (!valid(workspace)) return SDispatchResult{};

	auto args = CVarList(value);

	bool ephemeral = args[0] == "true";

	g_Hy3Layout->changeGroupEphemeralityOnWorkspace(workspace, ephemeral);
	return SDispatchResult{};
}

std::optional<ShiftDirection> parseShiftArg(std::string arg) {
	if (arg == "l" || arg == "left") return ShiftDirection::Left;
	else if (arg == "r" || arg == "right") return ShiftDirection::Right;
	else if (arg == "u" || arg == "up") return ShiftDirection::Up;
	else if (arg == "d" || arg == "down") return ShiftDirection::Down;
	else return {};
}

static SDispatchResult dispatch_movewindow(std::string value) {
	auto workspace = workspace_for_action();
	if (!valid(workspace)) return SDispatchResult{};

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
	return SDispatchResult{};
}

static SDispatchResult dispatch_movefocus(std::string value) {
	auto workspace = workspace_for_action(true);
	if (!valid(workspace)) return SDispatchResult{};

	auto args = CVarList(value);

	static const auto no_cursor_warps = ConfigValue<Hyprlang::INT>("cursor:no_warps");
	auto warp_cursor = !*no_cursor_warps;

	int argi = 0;
	auto shift = parseShiftArg(args[argi++]);
	if (!shift) return SDispatchResult{};
	if (workspace->m_bHasFullscreenWindow) {
		g_Hy3Layout->focusMonitor(shift.value());
		return SDispatchResult{};
	}

	auto visible = args[argi] == "visible";
	if (visible) argi++;

	if (args[argi] == "nowarp") warp_cursor = false;
	else if (args[argi] == "warp") warp_cursor = true;

	g_Hy3Layout->shiftFocus(workspace, shift.value(), visible, warp_cursor);
	return SDispatchResult{};
}

static SDispatchResult dispatch_togglefocuslayer(std::string value) {
	auto workspace = workspace_for_action();
	if (!valid(workspace)) return SDispatchResult{};

	g_Hy3Layout->toggleFocusLayer(workspace, value != "nowarp");
	return SDispatchResult{};
}

static SDispatchResult dispatch_warpcursor(std::string value) { g_Hy3Layout->warpCursor(); 
	return SDispatchResult{};}

static SDispatchResult dispatch_move_to_workspace(std::string value) {
	auto origin_workspace = workspace_for_action(true);
	if (!valid(origin_workspace)) return SDispatchResult{};

	auto args = CVarList(value);

	auto workspace = args[0];
	if (workspace == "") return SDispatchResult{};

	bool follow = args[1] == "follow";

	g_Hy3Layout->moveNodeToWorkspace(origin_workspace, workspace, follow);
	return SDispatchResult{};
}

static SDispatchResult dispatch_changefocus(std::string arg) {
	auto workspace = workspace_for_action();
	if (!valid(workspace)) return SDispatchResult{};

	if (arg == "top") g_Hy3Layout->changeFocus(workspace, FocusShift::Top);
	else if (arg == "bottom") g_Hy3Layout->changeFocus(workspace, FocusShift::Bottom);
	else if (arg == "raise") g_Hy3Layout->changeFocus(workspace, FocusShift::Raise);
	else if (arg == "lower") g_Hy3Layout->changeFocus(workspace, FocusShift::Lower);
	else if (arg == "tab") g_Hy3Layout->changeFocus(workspace, FocusShift::Tab);
	else if (arg == "tabnode") g_Hy3Layout->changeFocus(workspace, FocusShift::TabNode);
	return SDispatchResult{};
}

static SDispatchResult dispatch_focustab(std::string value) {
	auto workspace = workspace_for_action();
	if (!valid(workspace)) return SDispatchResult{};

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
		if (!isNumber(args[i])) return SDispatchResult{};
		index = std::stoi(args[i]);
		Debug::log(LOG, "Focus index '%s' -> %d, errno: %d", args[i].c_str(), index, errno);
	} else if (args[i] == "mouse") {
		g_Hy3Layout->focusTab(workspace, TabFocus::MouseLocation, mouse, false, 0);
		return SDispatchResult{};
	} else return SDispatchResult{};

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
	return SDispatchResult{};
}

static SDispatchResult dispatch_setswallow(std::string arg) {
	auto workspace = workspace_for_action();
	if (!valid(workspace)) return SDispatchResult{};

	SetSwallowOption option;
	if (arg == "true") {
		option = SetSwallowOption::Swallow;
	} else if (arg == "false") {
		option = SetSwallowOption::NoSwallow;
	} else if (arg == "toggle") {
		option = SetSwallowOption::Toggle;
	} else return SDispatchResult{};

	g_Hy3Layout->setNodeSwallow(workspace, option);
	return SDispatchResult{};
}

static SDispatchResult dispatch_killactive(std::string value) {
	auto workspace = workspace_for_action(true);
	if (!valid(workspace)) return SDispatchResult{};

	g_Hy3Layout->killFocusedNode(workspace);
	return SDispatchResult{};
}

static SDispatchResult dispatch_expand(std::string value) {
	auto workspace = workspace_for_action();
	if (!valid(workspace)) return SDispatchResult{};

	auto args = CVarList(value);

	ExpandOption expand;
	ExpandFullscreenOption fs_expand = ExpandFullscreenOption::MaximizeIntermediate;

	if (args[0] == "expand") expand = ExpandOption::Expand;
	else if (args[0] == "shrink") expand = ExpandOption::Shrink;
	else if (args[0] == "base") expand = ExpandOption::Base;
	else if (args[0] == "maximize") expand = ExpandOption::Maximize;
	else if (args[0] == "fullscreen") expand = ExpandOption::Fullscreen;
	else return SDispatchResult{};

	if (args[1] == "intermediate_maximize") fs_expand = ExpandFullscreenOption::MaximizeIntermediate;
	else if (args[1] == "fullscreen_maximize")
		fs_expand = ExpandFullscreenOption::MaximizeAsFullscreen;
	else if (args[1] == "maximize_only") fs_expand = ExpandFullscreenOption::MaximizeOnly;
	else if (args[1] != "") return SDispatchResult{};

	g_Hy3Layout->expand(workspace, expand, fs_expand);
	return SDispatchResult{};
}

static SDispatchResult dispatch_debug(std::string arg) {
	auto workspace = workspace_for_action();

	auto* root = g_Hy3Layout->getWorkspaceRootGroup(workspace);
	if (!valid(workspace)) {
		hy3_log(LOG, "DEBUG NODES: no nodes on workspace");
	} else {
		hy3_log(LOG, "DEBUG NODES\n{}", root->debugNode().c_str());
	}
	return SDispatchResult{};
}

void registerDispatchers() {
	HyprlandAPI::addDispatcherV2(PHANDLE, "hy3:makegroup", dispatch_makegroup);
	HyprlandAPI::addDispatcherV2(PHANDLE, "hy3:changegroup", dispatch_changegroup);
	HyprlandAPI::addDispatcherV2(PHANDLE, "hy3:setephemeral", dispatch_setephemeral);
	HyprlandAPI::addDispatcherV2(PHANDLE, "hy3:movefocus", dispatch_movefocus);
	HyprlandAPI::addDispatcherV2(PHANDLE, "hy3:togglefocuslayer", dispatch_togglefocuslayer);
	HyprlandAPI::addDispatcherV2(PHANDLE, "hy3:warpcursor", dispatch_warpcursor);
	HyprlandAPI::addDispatcherV2(PHANDLE, "hy3:movewindow", dispatch_movewindow);
	HyprlandAPI::addDispatcherV2(PHANDLE, "hy3:movetoworkspace", dispatch_move_to_workspace);
	HyprlandAPI::addDispatcherV2(PHANDLE, "hy3:changefocus", dispatch_changefocus);
	HyprlandAPI::addDispatcherV2(PHANDLE, "hy3:focustab", dispatch_focustab);
	HyprlandAPI::addDispatcherV2(PHANDLE, "hy3:setswallow", dispatch_setswallow);
	HyprlandAPI::addDispatcherV2(PHANDLE, "hy3:killactive", dispatch_killactive);
	HyprlandAPI::addDispatcherV2(PHANDLE, "hy3:expand", dispatch_expand);
	HyprlandAPI::addDispatcherV2(PHANDLE, "hy3:debugnodes", dispatch_debug);
}
