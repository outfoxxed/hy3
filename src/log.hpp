#pragma once

#include <hyprland/src/debug/Log.hpp>

template <typename... Args>
void hy3_log(LogLevel level, const std::string& fmt, Args&&... args) {
	Debug::log(level, "[hy3] " + fmt, args...);
}
