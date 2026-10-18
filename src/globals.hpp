#pragma once

#include <map>
#include <set>
#include <vector>

#include <hyprland/src/workspace/HLWorkspace.hpp>
#include <hyprland/src/config/ConfigValue.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprlang.hpp>

#include "Hy3Layout.hpp"
#include "TabGroup.hpp"
#include "config/shared/complex/ComplexDataType.hpp"

inline HANDLE PHANDLE = nullptr;

// Suppress algorithm callbacks (newTarget/movedTarget/removeTarget) during
// cross-workspace moves where we manually manage the tree.
inline bool g_suppressInsert = false;

inline std::set<Hy3Layout*> g_hy3Instances;

inline std::vector<WP<Hy3TabGroup>> g_tabGroups;
inline std::vector<UP<Hy3TabGroup>> g_destroyingTabGroups;

inline CHyprSignalListener g_renderListener;
inline CHyprSignalListener g_tickListener;
inline CHyprSignalListener g_windowTitleListener;
inline CHyprSignalListener g_urgentListener;
inline CHyprSignalListener g_touchDownListener;
inline CHyprSignalListener g_touchMotionListener;
inline CHyprSignalListener g_touchUpListener;

// Touches that started on a tab, by ID. Plugin-wide so a layout destroyed
// mid-touch can't leak the rest of the touch to a client that never saw the down.
inline std::map<int32_t, WP<ITouch>> g_tabBarTouches;

inline Hy3Layout* hy3InstanceForWorkspace(PHLWORKSPACE ws) {
	if (!ws || !ws->space() || !ws->space()->algorithm()) return nullptr;
	return dynamic_cast<Hy3Layout*>(ws->space()->algorithm()->tiledAlgo().get());
}

inline void errorNotif() {
	HyprlandAPI::addNotificationV2(
	    PHANDLE,
	    {
	        {"text", "Something has gone very wrong. Check the log for details."},
	        {"time", (uint64_t) 10000},
	        {"color", CHyprColor(1.0, 0.0, 0.0, 1.0)},
	        {"icon", ICON_ERROR},
	    }
	);
}
