#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/config/values/ConfigValues.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <hyprland/src/render/OpenGL.hpp>
#include <hyprland/src/version.h>

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
	HyprlandAPI::addConfigValueV2(PHANDLE, makeShared<Config::Values::TYPE>("plugin:hy3:" NAME, "", VALUE))

	// general
	CONF("no_gaps_when_only", Int, 0);
	CONF("node_collapse_policy", Int, 2);
	CONF("group_inset", Int, 10);
	CONF("tab_first_window", Int, 0);

	// tabs
	CONF("tabs:height", Int, 22);
	CONF("tabs:padding", Int, 5);
	CONF("tabs:from_top", Int, 0);
	CONF("tabs:radius", Int, 6);
	CONF("tabs:border_width", Int, 2);
	CONF("tabs:render_text", Int, 1);
	CONF("tabs:text_center", Int, 1);
	CONF("tabs:text_font", String, "Sans");
	CONF("tabs:text_height", Int, 8);
	CONF("tabs:text_padding", Int, 3);
	CONF("tabs:opacity", Float, 1.0);
	CONF("tabs:blur", Int, 1);
	CONF("tabs:col.active", Int, 0x4033ccff);
	CONF("tabs:col.active.border", Int, 0xee33ccff);
	CONF("tabs:col.active.text", Int, 0xffffffff);
	CONF("tabs:col.active_alt_monitor", Int, 0x40606060);
	CONF("tabs:col.active_alt_monitor.border", Int, 0xee808080);
	CONF("tabs:col.active_alt_monitor.text", Int, 0xffffffff);
	CONF("tabs:col.focused", Int, 0x40606060);
	CONF("tabs:col.focused.border", Int, 0xee808080);
	CONF("tabs:col.focused.text", Int, 0xffffffff);
	CONF("tabs:col.inactive", Int, 0x20303030);
	CONF("tabs:col.inactive.border", Int, 0xaa606060);
	CONF("tabs:col.inactive.text", Int, 0xffffffff);
	CONF("tabs:col.urgent", Int, 0x40ff2233);
	CONF("tabs:col.urgent.border", Int, 0xeeff2233);
	CONF("tabs:col.urgent.text", Int, 0xffffffff);
	CONF("tabs:col.locked", Int, 0x40909033);
	CONF("tabs:col.locked.border", Int, 0xee909033);
	CONF("tabs:col.locked.text", Int, 0xffffffff);

	// autotiling
	CONF("autotile:enable", Int, 0);
	CONF("autotile:ephemeral_groups", Int, 1);
	CONF("autotile:trigger_height", Int, 0);
	CONF("autotile:trigger_width", Int, 0);
	CONF("autotile:workspaces", String, "all");

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
				    && entry->target_window == g_pHyprRenderer->m_renderData.currentWindow.lock()
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
