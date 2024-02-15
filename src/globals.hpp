#pragma once

#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprlang.hpp>

#include "Hy3Layout.hpp"
#include "log.hpp"

inline HANDLE PHANDLE = nullptr;
inline std::unique_ptr<Hy3Layout> g_Hy3Layout;

inline void errorNotif() {
	HyprlandAPI::addNotificationV2(
	    PHANDLE,
	    {
	        {"text", "Something has gone very wrong. Check the log for details."},
	        {"time", (uint64_t) 10000},
	        {"color", CColor(1.0, 0.0, 0.0, 1.0)},
	        {"icon", ICON_ERROR},
	    }
	);
}

template <typename T>
class ConfigValue {
public:
	ConfigValue(const std::string& option) {
		this->static_data_ptr = HyprlandAPI::getConfigValue(PHANDLE, option)->getDataStaticPtr();
	}

	const T& get() const { return *(T*) *this->static_data_ptr; }

	const T& operator*() const { return this->get(); }

private:
	void* const* static_data_ptr;
};

// Bullshit undocumented microptimization case for strings
template <>
inline const Hyprlang::STRING& ConfigValue<Hyprlang::STRING>::get() const {
	return *(char* const*) this->static_data_ptr;
}
