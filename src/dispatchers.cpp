#include <optional>

#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/desktop/DesktopTypes.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprutils/string/String.hpp>

#include "dispatchers.hpp"
#include "globals.hpp"
#include "src/SharedDefs.hpp"

static Hy3Layout* hy3InstanceForAction(bool allow_fullscreen = false) {
	auto workspace = workspace_for_action(allow_fullscreen);
	if (!valid(workspace)) return nullptr;
	return hy3InstanceForWorkspace(workspace);
}

static SDispatchResult dispatch_makegroup(std::string value) {
	auto* hy3 = hy3InstanceForAction();
	if (!hy3) return SDispatchResult {};
	auto ws = hy3->workspace();

	auto args = CVarList(value);

	auto toggle = args[1] == "toggle";
	auto i = toggle ? 2 : 1;

	GroupEphemeralityOption ephemeral = GroupEphemeralityOption::Standard;
	if (args[i] == "ephemeral") {
		ephemeral = GroupEphemeralityOption::Ephemeral;
	} else if (args[i] == "force_ephemeral") {
		ephemeral = GroupEphemeralityOption::ForceEphemeral;
	}

	if (args[0] == "h") {
		hy3->makeGroupOnWorkspace(ws.get(), Hy3GroupLayout::SplitH, ephemeral, toggle);
	} else if (args[0] == "v") {
		hy3->makeGroupOnWorkspace(ws.get(), Hy3GroupLayout::SplitV, ephemeral, toggle);
	} else if (args[0] == "tab") {
		hy3->makeGroupOnWorkspace(ws.get(), Hy3GroupLayout::Tabbed, ephemeral, toggle);
	} else if (args[0] == "opposite") {
		hy3->makeOppositeGroupOnWorkspace(ws.get(), ephemeral);
	}
	return SDispatchResult {};
}

static SDispatchResult dispatch_changegroup(std::string value) {
	auto* hy3 = hy3InstanceForAction();
	if (!hy3) return SDispatchResult {};
	auto ws = hy3->workspace();

	auto args = CVarList(value);

	if (args[0] == "h") {
		hy3->changeGroupOnWorkspace(ws.get(), Hy3GroupLayout::SplitH);
	} else if (args[0] == "v") {
		hy3->changeGroupOnWorkspace(ws.get(), Hy3GroupLayout::SplitV);
	} else if (args[0] == "tab") {
		hy3->changeGroupOnWorkspace(ws.get(), Hy3GroupLayout::Tabbed);
	} else if (args[0] == "untab") {
		hy3->untabGroupOnWorkspace(ws.get());
	} else if (args[0] == "toggletab") {
		hy3->toggleTabGroupOnWorkspace(ws.get());
	} else if (args[0] == "opposite") {
		hy3->changeGroupToOppositeOnWorkspace(ws.get());
	}
	return SDispatchResult {};
}

static SDispatchResult dispatch_setephemeral(std::string value) {
	auto* hy3 = hy3InstanceForAction();
	if (!hy3) return SDispatchResult {};

	auto args = CVarList(value);

	bool ephemeral = args[0] == "true";

	hy3->changeGroupEphemeralityOnWorkspace(hy3->workspace().get(), ephemeral);
	return SDispatchResult {};
}

std::optional<ShiftDirection> parseShiftArg(std::string arg) {
	if (arg == "l" || arg == "left") return ShiftDirection::Left;
	else if (arg == "r" || arg == "right") return ShiftDirection::Right;
	else if (arg == "u" || arg == "up") return ShiftDirection::Up;
	else if (arg == "d" || arg == "down") return ShiftDirection::Down;
	else return {};
}

static SDispatchResult dispatch_movewindow(std::string value) {
	auto* hy3 = hy3InstanceForAction();
	if (!hy3) return SDispatchResult {};

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

		hy3->shiftWindow(hy3->workspace().get(), shift.value(), once, visible);
	}
	return SDispatchResult {};
}

static SDispatchResult dispatch_movefocus(std::string value) {
	auto* hy3 = hy3InstanceForAction(true);
	if (!hy3) return SDispatchResult {};
	auto ws = hy3->workspace();

	auto args = CVarList(value);

	static const auto no_cursor_warps = ConfigValue<Hyprlang::INT>("cursor:no_warps");
	auto warp_cursor = !*no_cursor_warps;

	int argi = 0;
	auto shift = parseShiftArg(args[argi++]);
	if (!shift) return SDispatchResult {};
	if (ws->m_hasFullscreenWindow) {
		hy3->focusMonitor(shift.value());
		return SDispatchResult {};
	}

	auto visible = args[argi] == "visible";
	if (visible) argi++;

	if (args[argi] == "nowarp") warp_cursor = false;
	else if (args[argi] == "warp") warp_cursor = true;

	hy3->shiftFocus(ws.get(), shift.value(), visible, warp_cursor);
	return SDispatchResult {};
}

static SDispatchResult dispatch_togglefocuslayer(std::string value) {
	auto* hy3 = hy3InstanceForAction();
	if (!hy3) return SDispatchResult {};

	hy3->toggleFocusLayer(hy3->workspace().get(), value != "nowarp");
	return SDispatchResult {};
}

static SDispatchResult dispatch_warpcursor(std::string value) {
	auto* hy3 = hy3InstanceForAction(true);
	if (!hy3) return SDispatchResult {};

	hy3->warpCursor();
	return SDispatchResult {};
}

static SDispatchResult dispatch_move_to_workspace(std::string value) {
	auto* hy3 = hy3InstanceForAction(true);
	if (!hy3) return SDispatchResult {};

	auto args = CVarList(value);

	static const auto no_cursor_warps = ConfigValue<Hyprlang::INT>("cursor:no_warps");

	auto workspace = args[0];
	if (workspace == "") return SDispatchResult {};

	auto follow = args[1] == "follow";

	auto warp_cursor =
	    follow
	    && ((!*no_cursor_warps && args[2] != "nowarp") || (*no_cursor_warps && args[2] == "warp"));

	hy3->moveNodeToWorkspace(hy3->workspace().get(), workspace, follow, warp_cursor);
	return SDispatchResult {};
}

static SDispatchResult dispatch_changefocus(std::string arg) {
	auto* hy3 = hy3InstanceForAction();
	if (!hy3) return SDispatchResult {};
	auto ws = hy3->workspace();

	if (arg == "top") hy3->changeFocus(ws.get(), FocusShift::Top);
	else if (arg == "bottom") hy3->changeFocus(ws.get(), FocusShift::Bottom);
	else if (arg == "raise") hy3->changeFocus(ws.get(), FocusShift::Raise);
	else if (arg == "lower") hy3->changeFocus(ws.get(), FocusShift::Lower);
	else if (arg == "tab") hy3->changeFocus(ws.get(), FocusShift::Tab);
	else if (arg == "tabnode") hy3->changeFocus(ws.get(), FocusShift::TabNode);
	return SDispatchResult {};
}

static SDispatchResult dispatch_focustab(std::string value) {
	auto* hy3 = hy3InstanceForAction();
	if (!hy3) return SDispatchResult {};
	auto ws = hy3->workspace();

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
		if (!isNumber(args[i])) return SDispatchResult {};
		index = std::stoi(args[i]);
		hy3_log(LOG, "Focus index '%s' -> %d, errno: %d", args[i].c_str(), index, errno);
	} else return SDispatchResult {};

	i++;

	if (args[i] == "prioritize_hovered") {
		mouse = TabFocusMousePriority::Prioritize;
		i++;
	} else if (args[i] == "require_hovered") {
		mouse = TabFocusMousePriority::Require;
		i++;
	}

	if (args[i++] == "wrap") wrap_scroll = true;

	hy3->focusTab(ws.get(), focus, mouse, wrap_scroll, index);
	return SDispatchResult {};
}

static SDispatchResult dispatch_setswallow(std::string arg) {
	auto* hy3 = hy3InstanceForAction();
	if (!hy3) return SDispatchResult {};

	SetSwallowOption option;
	if (arg == "true") {
		option = SetSwallowOption::Swallow;
	} else if (arg == "false") {
		option = SetSwallowOption::NoSwallow;
	} else if (arg == "toggle") {
		option = SetSwallowOption::Toggle;
	} else return SDispatchResult {};

	hy3->setNodeSwallow(hy3->workspace().get(), option);
	return SDispatchResult {};
}

static SDispatchResult dispatch_killactive(std::string value) {
	auto* hy3 = hy3InstanceForAction(true);
	if (!hy3) return SDispatchResult {};

	hy3->killFocusedNode(hy3->workspace().get());
	return SDispatchResult {};
}

static SDispatchResult dispatch_expand(std::string value) {
	auto* hy3 = hy3InstanceForAction();
	if (!hy3) return SDispatchResult {};

	auto args = CVarList(value);

	ExpandOption expand;
	ExpandFullscreenOption fs_expand = ExpandFullscreenOption::MaximizeIntermediate;

	if (args[0] == "expand") expand = ExpandOption::Expand;
	else if (args[0] == "shrink") expand = ExpandOption::Shrink;
	else if (args[0] == "base") expand = ExpandOption::Base;
	else if (args[0] == "maximize") expand = ExpandOption::Maximize;
	else if (args[0] == "fullscreen") expand = ExpandOption::Fullscreen;
	else return SDispatchResult {};

	if (args[1] == "intermediate_maximize") fs_expand = ExpandFullscreenOption::MaximizeIntermediate;
	else if (args[1] == "fullscreen_maximize")
		fs_expand = ExpandFullscreenOption::MaximizeAsFullscreen;
	else if (args[1] == "maximize_only") fs_expand = ExpandFullscreenOption::MaximizeOnly;
	else if (args[1] != "") return SDispatchResult {};

	hy3->expand(hy3->workspace().get(), expand, fs_expand);
	return SDispatchResult {};
}

static SDispatchResult dispatch_locktab(std::string arg) {
	auto* hy3 = hy3InstanceForAction();
	if (!hy3) return SDispatchResult {};

	auto mode = TabLockMode::Toggle;
	if (arg == "lock") mode = TabLockMode::Lock;
	else if (arg == "unlock") mode = TabLockMode::Unlock;

	hy3->setTabLock(hy3->workspace().get(), mode);
	return SDispatchResult {};
}

static SDispatchResult dispatch_equalize(std::string arg) {
	auto* hy3 = hy3InstanceForAction();
	if (!hy3) return SDispatchResult {};

	bool recursive = (arg == "workspace");
	hy3->equalize(hy3->workspace().get(), recursive);
	return SDispatchResult {};
}

static SDispatchResult dispatch_debug(std::string arg) {
	auto output = Hy3Layout::debugNodes();

	if (output.empty()) {
		hy3_log(LOG, "DEBUG NODES: no nodes");
		return { .success = false, .error = "no nodes" };
	}

	hy3_log(LOG, "DEBUG NODES\n{}", output);
	return { .success = false, .error = output };
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
	HyprlandAPI::addDispatcherV2(PHANDLE, "hy3:locktab", dispatch_locktab);
	HyprlandAPI::addDispatcherV2(PHANDLE, "hy3:equalize", dispatch_equalize);
	HyprlandAPI::addDispatcherV2(PHANDLE, "hy3:debugnodes", dispatch_debug);
}
