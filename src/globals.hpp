#pragma once

#include <set>
#include <type_traits>
#include <vector>

#include <hyprland/src/desktop/Workspace.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprlang.hpp>

#include "Hy3Layout.hpp"
#include "TabGroup.hpp"
#include "log.hpp"

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

inline Hy3Layout* hy3InstanceForWorkspace(PHLWORKSPACE ws) {
	if (!ws || !ws->m_space || !ws->m_space->algorithm()) return nullptr;
	return dynamic_cast<Hy3Layout*>(ws->m_space->algorithm()->tiledAlgo().get());
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

class HyprlangUnspecifiedCustomType {};

// abandon hope all ye who enter here
template <typename T, typename V = HyprlangUnspecifiedCustomType>
class ConfigValue {
public:
	ConfigValue(const std::string& option) {
		this->static_data_ptr = HyprlandAPI::getConfigValue(PHANDLE, option)->getDataStaticPtr();
	}

	template <typename U = T>
	typename std::enable_if<std::is_same<U, Hyprlang::CUSTOMTYPE>::value, const V&>::type
	operator*() const {
		return *(V*) ((Hyprlang::CUSTOMTYPE*) *this->static_data_ptr)->getData();
	}

	template <typename U = T>
	typename std::enable_if<std::is_same<U, Hyprlang::CUSTOMTYPE>::value, const V*>::type
	operator->() const {
		return &**this;
	}

	// Bullshit microptimization case for strings
	template <typename U = T>
	typename std::enable_if<std::is_same<U, Hyprlang::STRING>::value, const char*>::type
	operator*() const {
		return *(const char**) this->static_data_ptr;
	}

	template <typename U = T>
	typename std::enable_if<
	    !std::is_same<U, Hyprlang::CUSTOMTYPE>::value && !std::is_same<U, Hyprlang::STRING>::value,
	    const T&>::type
	operator*() const {
		return *(T*) *this->static_data_ptr;
	}

private:
	void* const* static_data_ptr;
};
