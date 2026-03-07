#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/config/ConfigDataValues.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <hyprland/src/render/OpenGL.hpp>
#include <hyprland/src/version.h>
#include <hyprlang.hpp>

#include "dispatchers.hpp"
#include "globals.hpp"
#include "TabGroup.hpp"

APICALL EXPORT std::string PLUGIN_API_VERSION() { return HYPRLAND_API_VERSION; }

APICALL EXPORT PLUGIN_DESCRIPTION_INFO PLUGIN_INIT(HANDLE handle) {
	PHANDLE = handle;

#ifndef HY3_NO_VERSION_CHECK
	const std::string COMPOSITOR_HASH = __hyprland_api_get_hash();
	const std::string CLIENT_HASH = __hyprland_api_get_client_hash();

	if (COMPOSITOR_HASH != CLIENT_HASH) {
		HyprlandAPI::addNotification(
		    PHANDLE,
		    "[hy3] hy3 was compiled for a different version of hyprland; refusing to load.",
		    CHyprColor {1.0, 0.2, 0.2, 1.0},
		    10000
		);

		throw std::runtime_error("[hy3] target hyprland version mismatch");
	}
#endif

#define CONF(NAME, TYPE, VALUE)                                                                    \
	HyprlandAPI::addConfigValue(PHANDLE, "plugin:hy3:" NAME, Hyprlang::CConfigValue((TYPE) VALUE))

	using Hyprlang::FLOAT;
	using Hyprlang::INT;
	using Hyprlang::STRING;

	// general
	CONF("no_gaps_when_only", INT, 0);
	CONF("node_collapse_policy", INT, 2);
	CONF("group_inset", INT, 10);
	CONF("tab_first_window", INT, 0);

	// tabs
	CONF("tabs:height", INT, 22);
	CONF("tabs:padding", INT, 5);
	CONF("tabs:from_top", INT, 0);
	CONF("tabs:radius", INT, 6);
	CONF("tabs:border_width", INT, 2);
	CONF("tabs:render_text", INT, 1);
	CONF("tabs:text_center", INT, 1);
	CONF("tabs:text_font", STRING, "Sans");
	CONF("tabs:text_height", INT, 8);
	CONF("tabs:text_padding", INT, 3);
	CONF("tabs:opacity", FLOAT, 1.0);
	CONF("tabs:blur", INT, 1);
	CONF("tabs:col.active", INT, 0x4033ccff);
	CONF("tabs:col.active.border", INT, 0xee33ccff);
	CONF("tabs:col.active.text", INT, 0xffffffff);
	CONF("tabs:col.active_alt_monitor", INT, 0x40606060);
	CONF("tabs:col.active_alt_monitor.border", INT, 0xee808080);
	CONF("tabs:col.active_alt_monitor.text", INT, 0xffffffff);
	CONF("tabs:col.focused", INT, 0x40606060);
	CONF("tabs:col.focused.border", INT, 0xee808080);
	CONF("tabs:col.focused.text", INT, 0xffffffff);
	CONF("tabs:col.inactive", INT, 0x20303030);
	CONF("tabs:col.inactive.border", INT, 0xaa606060);
	CONF("tabs:col.inactive.text", INT, 0xffffffff);
	CONF("tabs:col.urgent", INT, 0x40ff2233);
	CONF("tabs:col.urgent.border", INT, 0xeeff2233);
	CONF("tabs:col.urgent.text", INT, 0xffffffff);
	CONF("tabs:col.locked", INT, 0x40909033);
	CONF("tabs:col.locked.border", INT, 0xee909033);
	CONF("tabs:col.locked.text", INT, 0xffffffff);

	// autotiling
	CONF("autotile:enable", INT, 0);
	CONF("autotile:ephemeral_groups", INT, 1);
	CONF("autotile:trigger_height", INT, 0);
	CONF("autotile:trigger_width", INT, 0);
	CONF("autotile:workspaces", STRING, "all");

#undef CONF

	HyprlandAPI::addTiledAlgo(PHANDLE, "hy3", &typeid(Hy3Layout), []() -> UP<Layout::ITiledAlgorithm> {
		return makeUnique<Hy3Layout>();
	});

	g_renderListener = Event::bus()->m_events.render.stage.listen([](eRenderStage stage) {
		static bool rendering_normally = false;
		static std::vector<Hy3TabGroup*> rendered_groups;

		switch (stage) {
		case RENDER_PRE_WINDOWS:
			rendering_normally = true;
			rendered_groups.clear();
			break;
		case RENDER_POST_WINDOW:
			if (!rendering_normally) break;

			for (auto& wp: g_tabGroups) {
				auto* entry = wp.get();
				if (!entry) continue;
				if (!entry->hidden
				    && entry->target_window == g_pHyprOpenGL->m_renderData.currentWindow.lock()
				    && std::find(rendered_groups.begin(), rendered_groups.end(), entry)
				           == rendered_groups.end())
				{
					g_pHyprRenderer->m_renderPass.add(makeUnique<Hy3TabPassElement>(entry));
					rendered_groups.push_back(entry);
				}
			}
			break;
		case RENDER_POST_WINDOWS: rendering_normally = false; break;
		default: break;
		}
	});

	g_tickListener = Event::bus()->m_events.tick.listen([]() {
		for (auto& wp: g_tabGroups) {
			if (auto* tg = wp.get()) tg->tick();
		}
		std::erase_if(g_destroyingTabGroups, [](auto& up) { return up->bar.destroy; });
		std::erase_if(g_tabGroups, [](auto& wp) { return !wp; });
	});

	g_windowTitleListener = Event::bus()->m_events.window.title.listen([](PHLWINDOW window) {
		if (!window) return;
		auto* hy3 = hy3InstanceForWorkspace(window->m_workspace);
		if (!hy3) return;
		auto* node = hy3->getNodeFromWindow(window.get());
		if (!node) return;
		node->updateTabBarRecursive();
	});

	g_urgentListener = Event::bus()->m_events.window.urgent.listen([](PHLWINDOW window) {
		if (!window) return;
		window->m_isUrgent = true;
		auto* hy3 = hy3InstanceForWorkspace(window->m_workspace);
		if (!hy3) return;
		auto* node = hy3->getNodeFromWindow(window.get());
		if (!node) return;
		node->updateTabBarRecursive();
	});

	registerDispatchers();

	HyprlandAPI::reloadConfig();

	return {"hy3", "i3 like layout for hyprland", "outfoxxed", "0.1"};
}

APICALL EXPORT void PLUGIN_EXIT() {
	g_renderListener.reset();
	g_tickListener.reset();
	g_windowTitleListener.reset();
	g_urgentListener.reset();

	g_tabGroups.clear();
	g_destroyingTabGroups.clear();
}
