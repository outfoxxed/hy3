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

	HyprlandAPI::addConfigValue(
	    PHANDLE,
	    "plugin:hy3:no_gaps_when_only",
	    SConfigValue {.intValue = 0}
	);
	HyprlandAPI::addConfigValue(PHANDLE, "plugin:hy3:group_inset", SConfigValue {.intValue = 10});
	HyprlandAPI::addConfigValue(PHANDLE, "plugin:hy3:tabs:height", SConfigValue {.intValue = 15});
	HyprlandAPI::addConfigValue(PHANDLE, "plugin:hy3:tabs:padding", SConfigValue {.intValue = 5});
	HyprlandAPI::addConfigValue(PHANDLE, "plugin:hy3:tabs:from_top", SConfigValue {.intValue = 0});
	HyprlandAPI::addConfigValue(PHANDLE, "plugin:hy3:tabs:rounding", SConfigValue {.intValue = 3});
	HyprlandAPI::addConfigValue(PHANDLE, "plugin:hy3:tabs:render_text", SConfigValue {.intValue = 1});
	HyprlandAPI::addConfigValue(
	    PHANDLE,
	    "plugin:hy3:tabs:text_font",
	    SConfigValue {.strValue = "Sans"}
	);
	HyprlandAPI::addConfigValue(PHANDLE, "plugin:hy3:tabs:text_height", SConfigValue {.intValue = 8});
	HyprlandAPI::addConfigValue(
	    PHANDLE,
	    "plugin:hy3:tabs:text_padding",
	    SConfigValue {.intValue = 3}
	);
	HyprlandAPI::addConfigValue(
	    PHANDLE,
	    "plugin:hy3:tabs:col.active",
	    SConfigValue {.intValue = 0xff32b4ff}
	);
	HyprlandAPI::addConfigValue(
	    PHANDLE,
	    "plugin:hy3:tabs:col.urgent",
	    SConfigValue {.intValue = 0xffff4f4f}
	);
	HyprlandAPI::addConfigValue(
	    PHANDLE,
	    "plugin:hy3:tabs:col.inactive",
	    SConfigValue {.intValue = 0x80808080}
	);
	HyprlandAPI::addConfigValue(
	    PHANDLE,
	    "plugin:hy3:tabs:col.text.active",
	    SConfigValue {.intValue = 0xff000000}
	);
	HyprlandAPI::addConfigValue(
	    PHANDLE,
	    "plugin:hy3:tabs:col.text.urgent",
	    SConfigValue {.intValue = 0xff000000}
	);
	HyprlandAPI::addConfigValue(
	    PHANDLE,
	    "plugin:hy3:tabs:col.text.inactive",
	    SConfigValue {.intValue = 0xff000000}
	);

	g_Hy3Layout = std::make_unique<Hy3Layout>();
	HyprlandAPI::addLayout(PHANDLE, "hy3", g_Hy3Layout.get());

	registerDispatchers();

	return {"hy3", "i3 like layout for hyprland", "outfoxxed", "0.1"};
}

APICALL EXPORT void PLUGIN_EXIT() {}
