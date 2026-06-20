#include <optional>
#include <string>
#include <string_view>

#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/desktop/DesktopTypes.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprutils/string/String.hpp>
#include <hyprutils/string/VarList.hpp>
#include <hyprland/src/config/lua/LuaBindings.hpp>
#include <hyprland/src/config/lua/bindings/LuaBindingsInternal.hpp>

#include "dispatchers.hpp"
#include "log.hpp"
#include "globals.hpp"
#include "src/SharedDefs.hpp"

using Hyprutils::String::CVarList;
namespace LuaInternal = Config::Lua::Bindings::Internal;

static Hy3Layout* hy3InstanceForAction(bool allow_fullscreen = false) {
	auto workspace = workspace_for_action(allow_fullscreen);
	if (!valid(workspace)) return nullptr;
	return hy3InstanceForWorkspace(workspace);
}

static void luaCheckArgCount(lua_State* L, const char* fn, int min, int max) {
	const int count = lua_gettop(L);
	if (count < min || count > max) {
		if (min == max)
			luaL_error(L, "%s: expected %d argument(s), got %d", fn, min, count);
		else
			luaL_error(L, "%s: expected %d-%d argument(s), got %d", fn, min, max, count);
	}
}

static bool luaHasOptionsTable(lua_State* L, int idx, const char* fn) {
	if (lua_isnoneornil(L, idx)) return false;
	if (!lua_istable(L, idx))
		luaL_error(L, "%s: options must be a table", fn);
	return true;
}

static std::string luaStringArg(lua_State* L, int idx, const char* fn, const char* name) {
	if (lua_isnoneornil(L, idx)) {
		luaL_error(L, "%s: missing required argument '%s'", fn, name);
		return {};
	}

	if (!lua_isstring(L, idx)) {
		luaL_error(L, "%s: argument '%s' must be a string", fn, name);
		return {};
	}

	return lua_tostring(L, idx);
}

static std::optional<bool> luaBoolLikeArg(lua_State* L, int idx, const char* fn, const char* name) {
	if (lua_isboolean(L, idx)) return lua_toboolean(L, idx);

	if (lua_isstring(L, idx)) {
		std::string value = lua_tostring(L, idx);
		if (value == "true") return true;
		if (value == "false") return false;
	}

	luaL_error(L, "%s: argument '%s' must be a boolean", fn, name);
	return {};
}

static int luaOptionalBoolMode(std::optional<bool> value) {
	if (!value) return -1;
	return *value ? 1 : 0;
}

static std::optional<bool> luaOptionalBoolFromUpvalue(lua_State* L, int idx) {
	auto mode = static_cast<int>(lua_tointeger(L, lua_upvalueindex(idx)));
	if (mode < 0) return {};
	return mode != 0;
}

struct SMakeGroupAction {
	Hy3GroupLayout layout = Hy3GroupLayout::SplitH;
	bool opposite = false;
};

static std::optional<SMakeGroupAction> parseMakeGroupArg(std::string_view arg) {
	if (arg == "h") return SMakeGroupAction { .layout = Hy3GroupLayout::SplitH };
	if (arg == "v") return SMakeGroupAction { .layout = Hy3GroupLayout::SplitV };
	if (arg == "tab") return SMakeGroupAction { .layout = Hy3GroupLayout::Tabbed };
	if (arg == "stack") return SMakeGroupAction { .layout = Hy3GroupLayout::Stack };
	if (arg == "opposite") return SMakeGroupAction { .opposite = true };
	return {};
}

static SMakeGroupAction luaMakeGroupActionArg(lua_State* L, int idx, const char* fn) {
	auto value = luaStringArg(L, idx, fn, "layout");
	auto action = parseMakeGroupArg(value);
	if (!action)
		luaL_error(L, "%s: invalid layout '%s' (expected h/v/tab/stack/opposite)", fn, value.c_str());
	return *action;
}

static GroupEphemeralityOption luaTableEphemerality(lua_State* L, int idx, const char* fn) {
	idx = lua_absindex(L, idx);

	lua_getfield(L, idx, "ephemeral");
	if (lua_isnil(L, -1)) {
		lua_pop(L, 1);
		return GroupEphemeralityOption::Standard;
	}

	if (lua_isboolean(L, -1)) {
		const bool ephemeral = lua_toboolean(L, -1);
		lua_pop(L, 1);
		return ephemeral ? GroupEphemeralityOption::Ephemeral : GroupEphemeralityOption::Standard;
	}

	if (lua_isstring(L, -1)) {
		std::string value = lua_tostring(L, -1);
		lua_pop(L, 1);
		if (value == "force") return GroupEphemeralityOption::ForceEphemeral;
		luaL_error(L, "%s: invalid ephemeral option '%s' (expected true/false/\"force\")", fn, value.c_str());
		return GroupEphemeralityOption::Standard;
	}

	lua_pop(L, 1);
	luaL_error(L, "%s: ephemeral must be true, false, or \"force\"", fn);
	return GroupEphemeralityOption::Standard;
}

static SDispatchResult makeGroup(SMakeGroupAction action, GroupEphemeralityOption ephemeral, bool toggle) {
	auto* hy3 = hy3InstanceForAction();
	if (!hy3) return SDispatchResult {};
	auto ws = hy3->workspace();

	if (action.opposite) {
		hy3->makeOppositeGroupOnWorkspace(ws.get(), ephemeral);
	} else {
		hy3->makeGroupOnWorkspace(ws.get(), action.layout, ephemeral, toggle);
	}

	return SDispatchResult {};
}

static int luaMakeGroup(lua_State* L) {
	static constexpr const char* FN = "hl.plugin.hy3.make_group";
	luaCheckArgCount(L, FN, 1, 2);

	auto action = luaMakeGroupActionArg(L, 1, FN);
	bool toggle = false;
	auto ephemeral = GroupEphemeralityOption::Standard;

	if (luaHasOptionsTable(L, 2, FN)) {
		toggle = LuaInternal::tableOptBool(L, 2, "toggle").value_or(false);
		ephemeral = luaTableEphemerality(L, 2, FN);
	}

	auto dspMakeGroup = [](lua_State* L) -> int {
		SMakeGroupAction action {
			.layout = static_cast<Hy3GroupLayout>(lua_tointeger(L, lua_upvalueindex(1))),
			.opposite = static_cast<bool>(lua_toboolean(L, lua_upvalueindex(2))),
		};
		auto ephemeral = static_cast<GroupEphemeralityOption>(lua_tointeger(L, lua_upvalueindex(3)));
		bool toggle = lua_toboolean(L, lua_upvalueindex(4));
		makeGroup(action, ephemeral, toggle);
		return 0;
	};

	lua_pushinteger(L, static_cast<lua_Integer>(action.layout));
	lua_pushboolean(L, action.opposite);
	lua_pushinteger(L, static_cast<lua_Integer>(ephemeral));
	lua_pushboolean(L, toggle);
	lua_pushcclosure(L, dspMakeGroup, 4);
	return 1;
}

static SDispatchResult dispatch_makegroup(std::string value) {
	auto args = CVarList(value);

	auto action = parseMakeGroupArg(args[0]);
	if (!action) return SDispatchResult {};

	auto toggle = args[1] == "toggle";
	auto i = toggle ? 2 : 1;

	GroupEphemeralityOption ephemeral = GroupEphemeralityOption::Standard;
	if (args[i] == "ephemeral") {
		ephemeral = GroupEphemeralityOption::Ephemeral;
	} else if (args[i] == "force_ephemeral") {
		ephemeral = GroupEphemeralityOption::ForceEphemeral;
	}

	return makeGroup(*action, ephemeral, toggle);
}

enum class ChangeGroupAction {
	SplitH,
	SplitV,
	Tabbed,
	Stack,
	Untab,
	ToggleTab,
	Opposite,
};

static std::optional<ChangeGroupAction> parseChangeGroupArg(std::string_view arg) {
	if (arg == "h") return ChangeGroupAction::SplitH;
	if (arg == "v") return ChangeGroupAction::SplitV;
	if (arg == "tab") return ChangeGroupAction::Tabbed;
	if (arg == "stack") return ChangeGroupAction::Stack;
	if (arg == "untab") return ChangeGroupAction::Untab;
	if (arg == "toggletab") return ChangeGroupAction::ToggleTab;
	if (arg == "opposite") return ChangeGroupAction::Opposite;
	return {};
}

static ChangeGroupAction luaChangeGroupActionArg(lua_State* L, int idx, const char* fn) {
	auto value = luaStringArg(L, idx, fn, "layout");
	auto action = parseChangeGroupArg(value);
	if (!action)
		luaL_error(L, "%s: invalid layout '%s' (expected h/v/tab/stack/untab/toggletab/opposite)", fn, value.c_str());
	return *action;
}

static SDispatchResult changeGroup(ChangeGroupAction action) {
	auto* hy3 = hy3InstanceForAction();
	if (!hy3) return SDispatchResult {};
	auto ws = hy3->workspace();

	switch (action) {
	case ChangeGroupAction::SplitH:
		hy3->changeGroupOnWorkspace(ws.get(), Hy3GroupLayout::SplitH);
		break;
	case ChangeGroupAction::SplitV:
		hy3->changeGroupOnWorkspace(ws.get(), Hy3GroupLayout::SplitV);
		break;
	case ChangeGroupAction::Tabbed:
		hy3->changeGroupOnWorkspace(ws.get(), Hy3GroupLayout::Tabbed);
		break;
	case ChangeGroupAction::Stack:
		hy3->changeGroupOnWorkspace(ws.get(), Hy3GroupLayout::Stack);
		break;
	case ChangeGroupAction::Untab:
		hy3->untabGroupOnWorkspace(ws.get());
		break;
	case ChangeGroupAction::ToggleTab:
		hy3->toggleTabGroupOnWorkspace(ws.get());
		break;
	case ChangeGroupAction::Opposite:
		hy3->changeGroupToOppositeOnWorkspace(ws.get());
		break;
	}

	return SDispatchResult {};
}

static int luaChangeGroup(lua_State* L) {
	static constexpr const char* FN = "hl.plugin.hy3.change_group";
	luaCheckArgCount(L, FN, 1, 1);

	auto dspChangeGroup = [](lua_State* L) -> int {
		auto action = static_cast<ChangeGroupAction>(lua_tointeger(L, lua_upvalueindex(1)));
		changeGroup(action);
		return 0;
	};

	lua_pushinteger(L, static_cast<lua_Integer>(luaChangeGroupActionArg(L, 1, FN)));
	lua_pushcclosure(L, dspChangeGroup, 1);
	return 1;
}

static SDispatchResult dispatch_changegroup(std::string value) {
	auto args = CVarList(value);
	auto action = parseChangeGroupArg(args[0]);
	if (!action) return SDispatchResult {};
	return changeGroup(*action);
}

static SDispatchResult setEphemeral(bool ephemeral) {
	auto* hy3 = hy3InstanceForAction();
	if (!hy3) return SDispatchResult {};

	hy3->changeGroupEphemeralityOnWorkspace(hy3->workspace().get(), ephemeral);
	return SDispatchResult {};
}

static int luaSetEphemeral(lua_State* L) {
	static constexpr const char* FN = "hl.plugin.hy3.set_ephemeral";
	luaCheckArgCount(L, FN, 1, 1);

	auto ephemeral = luaBoolLikeArg(L, 1, FN, "value");

	auto dspSetEphemeral = [](lua_State* L) -> int {
		setEphemeral(lua_toboolean(L, lua_upvalueindex(1)));
		return 0;
	};

	lua_pushboolean(L, ephemeral.value_or(false));
	lua_pushcclosure(L, dspSetEphemeral, 1);
	return 1;
}

static SDispatchResult dispatch_setephemeral(std::string value) {
	auto args = CVarList(value);
	return setEphemeral(args[0] == "true");
}

std::optional<ShiftDirection> parseShiftArg(std::string arg) {
	if (arg == "l" || arg == "left") return ShiftDirection::Left;
	else if (arg == "r" || arg == "right") return ShiftDirection::Right;
	else if (arg == "u" || arg == "up") return ShiftDirection::Up;
	else if (arg == "d" || arg == "down") return ShiftDirection::Down;
	else return {};
}

static ShiftDirection luaShiftArg(lua_State* L, int idx, const char* fn) {
	auto value = luaStringArg(L, idx, fn, "direction");
	auto shift = parseShiftArg(value);
	if (!shift)
		luaL_error(L, "%s: invalid direction '%s' (expected left/right/up/down)", fn, value.c_str());
	return *shift;
}

static SDispatchResult moveFocus(ShiftDirection shift, bool visible, std::optional<bool> warp_override) {
	auto* hy3 = hy3InstanceForAction(true);
	if (!hy3) return SDispatchResult {};
	auto ws = hy3->workspace();

	static const auto no_cursor_warps = CConfigValue<Config::INTEGER>("cursor:no_warps");
	auto warp_cursor = warp_override.value_or(!*no_cursor_warps);
	if (ws->m_hasFullscreenWindow) {
		hy3->focusMonitor(shift);
		return SDispatchResult {};
	}

	hy3->shiftFocus(ws.get(), shift, visible, warp_cursor);
	return SDispatchResult {};
}

static int luaMoveFocus(lua_State* L) {
	static constexpr const char* FN = "hl.plugin.hy3.move_focus";
	luaCheckArgCount(L, FN, 1, 2);

	auto shift = luaShiftArg(L, 1, FN);
	bool visible = false;
	std::optional<bool> warp;

	if (luaHasOptionsTable(L, 2, FN)) {
		visible = LuaInternal::tableOptBool(L, 2, "visible").value_or(false);
		warp = LuaInternal::tableOptBool(L, 2, "warp");
	}

	auto dspMoveFocus = [](lua_State* L) -> int {
		auto shift = static_cast<ShiftDirection>(lua_tointeger(L, lua_upvalueindex(1)));
		bool visible = lua_toboolean(L, lua_upvalueindex(2));
		moveFocus(shift, visible, luaOptionalBoolFromUpvalue(L, 3));
		return 0;
	};

	lua_pushinteger(L, static_cast<lua_Integer>(shift));
	lua_pushboolean(L, visible);
	lua_pushinteger(L, luaOptionalBoolMode(warp));
	lua_pushcclosure(L, dspMoveFocus, 3);
	return 1;
}

static SDispatchResult dispatch_movefocus(std::string value) {
	auto args = CVarList(value);

	int argi = 0;
	auto shift = parseShiftArg(args[argi++]);
	if (!shift) return SDispatchResult {};

	auto visible = args[argi] == "visible";
	if (visible) argi++;

	std::optional<bool> warp_override;
	if (args[argi] == "nowarp") warp_override = false;
	else if (args[argi] == "warp") warp_override = true;

	return moveFocus(*shift, visible, warp_override);
}

static SDispatchResult toggleFocusLayer(bool warp) {
	auto* hy3 = hy3InstanceForAction();
	if (!hy3) return SDispatchResult {};

	hy3->toggleFocusLayer(hy3->workspace().get(), warp);
	return SDispatchResult {};
}

static int luaToggleFocusLayer(lua_State* L) {
	static constexpr const char* FN = "hl.plugin.hy3.toggle_focus_layer";
	luaCheckArgCount(L, FN, 0, 1);

	bool warp = true;
	if (luaHasOptionsTable(L, 1, FN))
		warp = LuaInternal::tableOptBool(L, 1, "warp").value_or(true);

	auto dspToggleFocusLayer = [](lua_State* L) -> int {
		toggleFocusLayer(lua_toboolean(L, lua_upvalueindex(1)));
		return 0;
	};

	lua_pushboolean(L, warp);
	lua_pushcclosure(L, dspToggleFocusLayer, 1);
	return 1;
}

static SDispatchResult dispatch_togglefocuslayer(std::string value) {
	return toggleFocusLayer(value != "nowarp");
}

static SDispatchResult warpCursor() {
	auto* hy3 = hy3InstanceForAction(true);
	if (!hy3) return SDispatchResult {};

	hy3->warpCursor();
	return SDispatchResult {};
}

static int luaWarpCursor(lua_State* L) {
	static constexpr const char* FN = "hl.plugin.hy3.warp_cursor";
	luaCheckArgCount(L, FN, 0, 0);

	auto dspWarpCursor = [](lua_State* L) -> int {
		warpCursor();
		return 0;
	};

	lua_pushcclosure(L, dspWarpCursor, 0);
	return 1;
}

static SDispatchResult dispatch_warpcursor(std::string value) {
	return warpCursor();
}

static SDispatchResult moveWindow(ShiftDirection shift, bool once, bool visible) {
	auto* hy3 = hy3InstanceForAction();
	if (!hy3) return SDispatchResult {};

	hy3->shiftWindow(hy3->workspace().get(), shift, once, visible);
	return SDispatchResult {};
}

static int luaMoveWindow(lua_State* L) {
	static constexpr const char* FN = "hl.plugin.hy3.move_window";
	luaCheckArgCount(L, FN, 1, 2);

	auto shift = luaShiftArg(L, 1, FN);
	bool once = false;
	bool visible = false;

	if (luaHasOptionsTable(L, 2, FN)) {
		once = LuaInternal::tableOptBool(L, 2, "once").value_or(false);
		visible = LuaInternal::tableOptBool(L, 2, "visible").value_or(false);
	}

	auto dspMoveWindow = [](lua_State* L) -> int {
		auto shift = static_cast<ShiftDirection>(lua_tointeger(L, lua_upvalueindex(1)));
		bool once = lua_toboolean(L, lua_upvalueindex(2));
		bool visible = lua_toboolean(L, lua_upvalueindex(3));
		moveWindow(shift, once, visible);
		return 0;
	};

	lua_pushinteger(L, static_cast<lua_Integer>(shift));
	lua_pushboolean(L, once);
	lua_pushboolean(L, visible);
	lua_pushcclosure(L, dspMoveWindow, 3);
	return 1;
}

static SDispatchResult dispatch_movewindow(std::string value) {
	auto args = CVarList(value);
	auto shift = parseShiftArg(args[0]);
	if (!shift) return SDispatchResult {};

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

	return moveWindow(*shift, once, visible);
}

static SDispatchResult moveToWorkspace(std::string workspace, bool follow, std::optional<bool> warp_override) {
	auto* hy3 = hy3InstanceForAction(true);
	if (!hy3) return SDispatchResult {};

	static const auto no_cursor_warps = CConfigValue<Config::INTEGER>("cursor:no_warps");

	auto warp_cursor = follow && warp_override.value_or(!*no_cursor_warps);

	hy3->moveNodeToWorkspace(hy3->workspace().get(), workspace, follow, warp_cursor);
	return SDispatchResult {};
}

static int luaMoveToWorkspace(lua_State* L) {
	static constexpr const char* FN = "hl.plugin.hy3.move_to_workspace";
	luaCheckArgCount(L, FN, 1, 2);

	auto workspace = luaStringArg(L, 1, FN, "workspace");
	if (workspace == "")
		return luaL_error(L, "%s: workspace must not be empty", FN);

	bool follow = false;
	std::optional<bool> warp;

	if (luaHasOptionsTable(L, 2, FN)) {
		follow = LuaInternal::tableOptBool(L, 2, "follow").value_or(false);
		warp = LuaInternal::tableOptBool(L, 2, "warp");
	}

	auto dspMoveToWorkspace = [](lua_State* L) -> int {
		std::string workspace = lua_tostring(L, lua_upvalueindex(1));
		bool follow = lua_toboolean(L, lua_upvalueindex(2));
		moveToWorkspace(workspace, follow, luaOptionalBoolFromUpvalue(L, 3));
		return 0;
	};

	lua_pushstring(L, workspace.c_str());
	lua_pushboolean(L, follow);
	lua_pushinteger(L, luaOptionalBoolMode(warp));
	lua_pushcclosure(L, dspMoveToWorkspace, 3);
	return 1;
}

static SDispatchResult dispatch_move_to_workspace(std::string value) {
	auto args = CVarList(value);

	auto workspace = args[0];
	if (workspace == "") return SDispatchResult {};

	auto follow = args[1] == "follow";

	std::optional<bool> warp_override;
	if (args[2] == "nowarp") warp_override = false;
	else if (args[2] == "warp") warp_override = true;

	return moveToWorkspace(workspace, follow, warp_override);
}

static std::optional<FocusShift> parseFocusShiftArg(std::string_view arg) {
	if (arg == "top") return FocusShift::Top;
	if (arg == "bottom") return FocusShift::Bottom;
	if (arg == "raise") return FocusShift::Raise;
	if (arg == "lower") return FocusShift::Lower;
	if (arg == "tab") return FocusShift::Tab;
	if (arg == "tabnode") return FocusShift::TabNode;
	return {};
}

static FocusShift luaFocusShiftArg(lua_State* L, int idx, const char* fn) {
	auto value = luaStringArg(L, idx, fn, "mode");
	auto shift = parseFocusShiftArg(value);
	if (!shift)
		luaL_error(L, "%s: invalid mode '%s'", fn, value.c_str());
	return *shift;
}

static SDispatchResult changeFocus(FocusShift shift) {
	auto* hy3 = hy3InstanceForAction();
	if (!hy3) return SDispatchResult {};
	auto ws = hy3->workspace();

	hy3->changeFocus(ws.get(), shift);
	return SDispatchResult {};
}

static int luaChangeFocus(lua_State* L) {
	static constexpr const char* FN = "hl.plugin.hy3.change_focus";
	luaCheckArgCount(L, FN, 1, 1);

	auto dspChangeFocus = [](lua_State* L) -> int {
		auto shift = static_cast<FocusShift>(lua_tointeger(L, lua_upvalueindex(1)));
		changeFocus(shift);
		return 0;
	};

	lua_pushinteger(L, static_cast<lua_Integer>(luaFocusShiftArg(L, 1, FN)));
	lua_pushcclosure(L, dspChangeFocus, 1);
	return 1;
}

static SDispatchResult dispatch_changefocus(std::string arg) {
	auto shift = parseFocusShiftArg(arg);
	if (!shift) return SDispatchResult {};
	return changeFocus(*shift);
}

static std::optional<TabFocusMousePriority> parseTabMouseArg(std::string_view arg) {
	if (arg == "" || arg == "ignore") return TabFocusMousePriority::Ignore;
	if (arg == "prioritize_hovered") return TabFocusMousePriority::Prioritize;
	if (arg == "require_hovered") return TabFocusMousePriority::Require;
	return {};
}

static SDispatchResult focusTab(TabFocus focus, TabFocusMousePriority mouse, bool wrap_scroll, int index) {
	auto* hy3 = hy3InstanceForAction();
	if (!hy3) return SDispatchResult {};
	auto ws = hy3->workspace();

	hy3->focusTab(ws.get(), focus, mouse, wrap_scroll, index);
	return SDispatchResult {};
}

static int luaFocusTab(lua_State* L) {
	static constexpr const char* FN = "hl.plugin.hy3.focus_tab";
	luaCheckArgCount(L, FN, 1, 1);
	if (!lua_istable(L, 1))
		return luaL_error(L, "%s: expected a table", FN);

	auto index = LuaInternal::tableOptNum(L, 1, "index");
	auto direction = LuaInternal::tableOptStr(L, 1, "direction");
	if (index && direction)
		return luaL_error(L, "%s: options 'index' and 'direction' are mutually exclusive", FN);

	TabFocus focus;
	int focus_index = 0;

	if (index) {
		focus = TabFocus::Index;
		focus_index = static_cast<int>(*index);
	} else {
		if (!direction)
			return luaL_error(L, "%s: expected either 'direction' or 'index'", FN);
		if (*direction == "l" || *direction == "left") focus = TabFocus::Left;
		else if (*direction == "r" || *direction == "right") focus = TabFocus::Right;
		else if (*direction == "u" || *direction == "up") focus = TabFocus::Up;
		else if (*direction == "d" || *direction == "down") focus = TabFocus::Down;
		else
			return luaL_error(L, "%s: invalid direction '%s' (expected left/right/up/down)", FN, direction->c_str());
	}

	auto mouse = TabFocusMousePriority::Ignore;
	if (auto mouse_arg = LuaInternal::tableOptStr(L, 1, "mouse")) {
		auto parsed = parseTabMouseArg(*mouse_arg);
		if (!parsed)
			return luaL_error(L, "%s: invalid mouse option '%s'", FN, mouse_arg->c_str());
		mouse = *parsed;
	}

	bool wrap = LuaInternal::tableOptBool(L, 1, "wrap").value_or(false);

	auto dspFocusTab = [](lua_State* L) -> int {
		auto focus = static_cast<TabFocus>(lua_tointeger(L, lua_upvalueindex(1)));
		auto mouse = static_cast<TabFocusMousePriority>(lua_tointeger(L, lua_upvalueindex(2)));
		bool wrap = lua_toboolean(L, lua_upvalueindex(3));
		int index = static_cast<int>(lua_tointeger(L, lua_upvalueindex(4)));
		focusTab(focus, mouse, wrap, index);
		return 0;
	};

	lua_pushinteger(L, static_cast<lua_Integer>(focus));
	lua_pushinteger(L, static_cast<lua_Integer>(mouse));
	lua_pushboolean(L, wrap);
	lua_pushinteger(L, focus_index);
	lua_pushcclosure(L, dspFocusTab, 4);
	return 1;
}

static SDispatchResult dispatch_focustab(std::string value) {
	auto i = 0;
	auto args = CVarList(value);

	TabFocus focus;
	auto mouse = TabFocusMousePriority::Ignore;
	bool wrap_scroll = false;
	int index = 0;

	if (args[i] == "l" || args[i] == "left") focus = TabFocus::Left;
	else if (args[i] == "r" || args[i] == "right") focus = TabFocus::Right;
	else if (args[i] == "u" || args[i] == "up") focus = TabFocus::Up;
	else if (args[i] == "d" || args[i] == "down") focus = TabFocus::Down;
	else if (args[i] == "index") {
		i++;
		focus = TabFocus::Index;
		if (!Hyprutils::String::isNumber(args[i])) return SDispatchResult {};
		index = std::stoi(args[i]);
		hy3_log(LOG, "Focus index '%s' -> %d, errno: %d", args[i].c_str(), index, errno);
	} else return SDispatchResult {};

	i++;

	if (auto parsed_mouse = parseTabMouseArg(args[i]); parsed_mouse && *parsed_mouse != TabFocusMousePriority::Ignore) {
		mouse = *parsed_mouse;
		i++;
	}

	if (args[i++] == "wrap") wrap_scroll = true;

	return focusTab(focus, mouse, wrap_scroll, index);
}

static std::optional<SetSwallowOption> parseSetSwallowArg(std::string_view arg) {
	if (arg == "true") return SetSwallowOption::Swallow;
	if (arg == "false") return SetSwallowOption::NoSwallow;
	if (arg == "toggle") return SetSwallowOption::Toggle;
	return {};
}

static SetSwallowOption luaSetSwallowArg(lua_State* L, int idx, const char* fn) {
	if (lua_isboolean(L, idx))
		return lua_toboolean(L, idx) ? SetSwallowOption::Swallow : SetSwallowOption::NoSwallow;

	auto value = luaStringArg(L, idx, fn, "value");
	auto option = parseSetSwallowArg(value);
	if (!option)
		luaL_error(L, "%s: invalid value '%s' (expected true/false/toggle)", fn, value.c_str());
	return *option;
}

static SDispatchResult setSwallow(SetSwallowOption option) {
	auto* hy3 = hy3InstanceForAction();
	if (!hy3) return SDispatchResult {};

	hy3->setNodeSwallow(hy3->workspace().get(), option);
	return SDispatchResult {};
}

static int luaSetSwallow(lua_State* L) {
	static constexpr const char* FN = "hl.plugin.hy3.set_swallow";
	luaCheckArgCount(L, FN, 1, 1);

	auto dspSetSwallow = [](lua_State* L) -> int {
		auto option = static_cast<SetSwallowOption>(lua_tointeger(L, lua_upvalueindex(1)));
		setSwallow(option);
		return 0;
	};

	lua_pushinteger(L, static_cast<lua_Integer>(luaSetSwallowArg(L, 1, FN)));
	lua_pushcclosure(L, dspSetSwallow, 1);
	return 1;
}

static SDispatchResult dispatch_setswallow(std::string arg) {
	auto option = parseSetSwallowArg(arg);
	if (!option) return SDispatchResult {};
	return setSwallow(*option);
}

static SDispatchResult killActive() {
	auto* hy3 = hy3InstanceForAction(true);
	if (!hy3) return SDispatchResult {};

	hy3->killFocusedNode(hy3->workspace().get());
	return SDispatchResult {};
}

static int luaKillActive(lua_State* L) {
	static constexpr const char* FN = "hl.plugin.hy3.kill_active";
	luaCheckArgCount(L, FN, 0, 0);

	auto dspKillActive = [](lua_State* L) -> int {
		killActive();
		return 0;
	};

	lua_pushcclosure(L, dspKillActive, 0);
	return 1;
}

static SDispatchResult dispatch_killactive(std::string value) {
	return killActive();
}

static std::optional<ExpandOption> parseExpandArg(std::string_view arg) {
	if (arg == "expand") return ExpandOption::Expand;
	if (arg == "shrink") return ExpandOption::Shrink;
	if (arg == "base") return ExpandOption::Base;
	if (arg == "maximize") return ExpandOption::Maximize;
	if (arg == "fullscreen") return ExpandOption::Fullscreen;
	return {};
}

static std::optional<ExpandFullscreenOption> parseExpandFullscreenArg(std::string_view arg) {
	if (arg == "" || arg == "intermediate_maximize") return ExpandFullscreenOption::MaximizeIntermediate;
	if (arg == "fullscreen_maximize") return ExpandFullscreenOption::MaximizeAsFullscreen;
	if (arg == "maximize_only") return ExpandFullscreenOption::MaximizeOnly;
	return {};
}

static SDispatchResult expand(ExpandOption expand, ExpandFullscreenOption fs_expand) {
	auto* hy3 = hy3InstanceForAction();
	if (!hy3) return SDispatchResult {};

	hy3->expand(hy3->workspace().get(), expand, fs_expand);
	return SDispatchResult {};
}

static int luaExpand(lua_State* L) {
	static constexpr const char* FN = "hl.plugin.hy3.expand";
	luaCheckArgCount(L, FN, 1, 2);

	auto value = luaStringArg(L, 1, FN, "mode");
	auto option = parseExpandArg(value);
	if (!option)
		return luaL_error(L, "%s: invalid mode '%s'", FN, value.c_str());

	auto fs_option = ExpandFullscreenOption::MaximizeIntermediate;
	if (luaHasOptionsTable(L, 2, FN)) {
		auto fs_value = LuaInternal::tableOptStr(L, 2, "fullscreen").value_or("intermediate_maximize");
		auto parsed = parseExpandFullscreenArg(fs_value);
		if (!parsed)
			return luaL_error(L, "%s: invalid fullscreen option '%s'", FN, fs_value.c_str());
		fs_option = *parsed;
	}

	auto dspExpand = [](lua_State* L) -> int {
		auto option = static_cast<ExpandOption>(lua_tointeger(L, lua_upvalueindex(1)));
		auto fs_option = static_cast<ExpandFullscreenOption>(lua_tointeger(L, lua_upvalueindex(2)));
		expand(option, fs_option);
		return 0;
	};

	lua_pushinteger(L, static_cast<lua_Integer>(*option));
	lua_pushinteger(L, static_cast<lua_Integer>(fs_option));
	lua_pushcclosure(L, dspExpand, 2);
	return 1;
}

static SDispatchResult dispatch_expand(std::string value) {
	auto args = CVarList(value);

	auto expand_option = parseExpandArg(args[0]);
	if (!expand_option) return SDispatchResult {};

	auto fs_expand = parseExpandFullscreenArg(args[1]);
	if (!fs_expand) return SDispatchResult {};

	return expand(*expand_option, *fs_expand);
}

static std::optional<TabLockMode> parseTabLockModeArg(std::string_view arg) {
	if (arg == "" || arg == "toggle") return TabLockMode::Toggle;
	if (arg == "lock") return TabLockMode::Lock;
	if (arg == "unlock") return TabLockMode::Unlock;
	return {};
}

static SDispatchResult lockTab(TabLockMode mode) {
	auto* hy3 = hy3InstanceForAction();
	if (!hy3) return SDispatchResult {};

	hy3->setTabLock(hy3->workspace().get(), mode);
	return SDispatchResult {};
}

static int luaLockTab(lua_State* L) {
	static constexpr const char* FN = "hl.plugin.hy3.lock_tab";
	luaCheckArgCount(L, FN, 0, 1);

	std::string mode_arg;
	if (!lua_isnoneornil(L, 1))
		mode_arg = luaStringArg(L, 1, FN, "mode");

	auto mode = parseTabLockModeArg(mode_arg);
	if (!mode)
		return luaL_error(L, "%s: invalid mode '%s' (expected lock/unlock/toggle)", FN, mode_arg.c_str());

	auto dspLockTab = [](lua_State* L) -> int {
		auto mode = static_cast<TabLockMode>(lua_tointeger(L, lua_upvalueindex(1)));
		lockTab(mode);
		return 0;
	};

	lua_pushinteger(L, static_cast<lua_Integer>(*mode));
	lua_pushcclosure(L, dspLockTab, 1);
	return 1;
}

static SDispatchResult dispatch_locktab(std::string arg) {
	auto mode = parseTabLockModeArg(arg);
	if (!mode) return SDispatchResult {};
	return lockTab(*mode);
}

static SDispatchResult equalize(bool recursive) {
	auto* hy3 = hy3InstanceForAction();
	if (!hy3) return SDispatchResult {};

	hy3->equalize(hy3->workspace().get(), recursive);
	return SDispatchResult {};
}

static int luaEqualize(lua_State* L) {
	static constexpr const char* FN = "hl.plugin.hy3.equalize";
	luaCheckArgCount(L, FN, 0, 1);

	bool recursive = false;
	if (luaHasOptionsTable(L, 1, FN)) {
		if (auto scope = LuaInternal::tableOptStr(L, 1, "scope")) {
			if (*scope == "workspace") recursive = true;
			else if (*scope == "" || *scope == "group") recursive = false;
			else
				return luaL_error(L, "%s: invalid scope '%s' (expected group/workspace)", FN, scope->c_str());
		}

		if (auto workspace = LuaInternal::tableOptBool(L, 1, "workspace"); workspace)
			recursive = *workspace;
		if (auto recursive_opt = LuaInternal::tableOptBool(L, 1, "recursive"); recursive_opt)
			recursive = *recursive_opt;
	}

	auto dspEqualize = [](lua_State* L) -> int {
		equalize(lua_toboolean(L, lua_upvalueindex(1)));
		return 0;
	};

	lua_pushboolean(L, recursive);
	lua_pushcclosure(L, dspEqualize, 1);
	return 1;
}

static SDispatchResult dispatch_equalize(std::string arg) {
	return equalize(arg == "workspace");
}

static SDispatchResult debugNodes() {
	auto output = Hy3Layout::debugNodes();

	if (output.empty()) {
		hy3_log(LOG, "DEBUG NODES: no nodes");
		return { .success = false, .error = "no nodes" };
	}

	hy3_log(LOG, "DEBUG NODES\n{}", output);
	return { .success = false, .error = output };
}

static int luaDebugNodes(lua_State* L) {
	static constexpr const char* FN = "hl.plugin.hy3.debug_nodes";
	luaCheckArgCount(L, FN, 0, 0);

	auto dspDebugNodes = [](lua_State* L) -> int {
		debugNodes();
		return 0;
	};

	lua_pushcclosure(L, dspDebugNodes, 0);
	return 1;
}

static SDispatchResult dispatch_debug(std::string arg) {
	return debugNodes();
}

static void registerLuaDispatchers() {
	HyprlandAPI::addLuaFunction(PHANDLE, "hy3", "make_group", luaMakeGroup);
	HyprlandAPI::addLuaFunction(PHANDLE, "hy3", "change_group", luaChangeGroup);
	HyprlandAPI::addLuaFunction(PHANDLE, "hy3", "set_ephemeral", luaSetEphemeral);
	HyprlandAPI::addLuaFunction(PHANDLE, "hy3", "move_focus", luaMoveFocus);
	HyprlandAPI::addLuaFunction(PHANDLE, "hy3", "toggle_focus_layer", luaToggleFocusLayer);
	HyprlandAPI::addLuaFunction(PHANDLE, "hy3", "warp_cursor", luaWarpCursor);
	HyprlandAPI::addLuaFunction(PHANDLE, "hy3", "move_window", luaMoveWindow);
	HyprlandAPI::addLuaFunction(PHANDLE, "hy3", "move_to_workspace", luaMoveToWorkspace);
	HyprlandAPI::addLuaFunction(PHANDLE, "hy3", "change_focus", luaChangeFocus);
	HyprlandAPI::addLuaFunction(PHANDLE, "hy3", "focus_tab", luaFocusTab);
	HyprlandAPI::addLuaFunction(PHANDLE, "hy3", "set_swallow", luaSetSwallow);
	HyprlandAPI::addLuaFunction(PHANDLE, "hy3", "kill_active", luaKillActive);
	HyprlandAPI::addLuaFunction(PHANDLE, "hy3", "expand", luaExpand);
	HyprlandAPI::addLuaFunction(PHANDLE, "hy3", "lock_tab", luaLockTab);
	HyprlandAPI::addLuaFunction(PHANDLE, "hy3", "equalize", luaEqualize);
	HyprlandAPI::addLuaFunction(PHANDLE, "hy3", "debug_nodes", luaDebugNodes);
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
	registerLuaDispatchers();
}
