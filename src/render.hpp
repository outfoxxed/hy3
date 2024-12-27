#pragma once
#include <hyprland/src/helpers/Color.hpp>
#include <hyprutils/math/Box.hpp>

class Hy3Render {
public:
	static void renderBorderRect(
	    const CBox& box,
	    const CHyprColor& fillColor,
	    const CHyprColor& borderColor,
	    int borderWidth,
	    int radius
	);
};
