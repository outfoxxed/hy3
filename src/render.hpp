#pragma once
#include <hyprland/src/helpers/Color.hpp>
#include <hyprutils/math/Box.hpp>

namespace Render {
class CRenderContext;
}

class Hy3Render {
public:
	static void renderTab(
	    Render::CRenderContext& ctx,
	    const Hyprutils::Math::CBox& box,
	    float opacity,
	    bool blur,
	    const CHyprColor& fillColor,
	    const CHyprColor& borderColor,
	    int borderWidth,
	    int radius
	);
};
