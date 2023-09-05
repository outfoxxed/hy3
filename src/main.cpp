#include <optional>

#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>

#include "dispatchers.hpp"
#include "globals.hpp"
#include "SelectionHook.hpp"

APICALL EXPORT std::string PLUGIN_API_VERSION() { return HYPRLAND_API_VERSION; }

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
	PHANDLE = handle;

	selection_hook::init();

#define CONF(NAME, TYPE, VALUE)                                                                    \
	HyprlandAPI::addConfigValue(PHANDLE, "plugin:hy3:" NAME, SConfigValue {.TYPE##Value = VALUE})

	// general
	CONF("no_gaps_when_only", int, 0);
	CONF("node_collapse_policy", int, 2);
	CONF("group_inset", int, 10);
	CONF("special_scale_factor", float, 0.8);

	// tabs
	CONF("tabs:height", int, 15);
	CONF("tabs:padding", int, 5);
	CONF("tabs:from_top", int, 0);
	CONF("tabs:rounding", int, 3);
	CONF("tabs:render_text", int, 1);
	CONF("tabs:text_font", str, "Sans");
	CONF("tabs:text_height", int, 8);
	CONF("tabs:text_padding", int, 3);
	CONF("tabs:col.active", int, 0xff32b4ff);
	CONF("tabs:col.urgent", int, 0xffff4f4f);
	CONF("tabs:col.inactive", int, 0x80808080);
	CONF("tabs:col.text.active", int, 0xff000000);
	CONF("tabs:col.text.urgent", int, 0xff000000);
	CONF("tabs:col.text.inactive", int, 0xff000000);

	// autotiling
	CONF("autotile:enable", int, 0);
	CONF("autotile:ephemeral_groups", int, 1);
	CONF("autotile:trigger_height", int, 0);
	CONF("autotile:trigger_width", int, 0);

#undef CONF

	g_Hy3Layout = std::make_unique<Hy3Layout>();
	HyprlandAPI::addLayout(PHANDLE, "hy3", g_Hy3Layout.get());

	registerDispatchers();

	return {"hy3", "i3 like layout for hyprland", "outfoxxed", "0.1"};
}

APICALL EXPORT void PLUGIN_EXIT() {}
