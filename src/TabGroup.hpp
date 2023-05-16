#pragma once

#include <hyprland/src/helpers/AnimatedVariable.hpp>
#include <hyprland/src/helpers/Vector2D.hpp>
#include <vector>

class Hy3TabGroup;

#include "Hy3Layout.hpp"
#include <hyprland/src/render/Texture.hpp>

struct Hy3TabBarEntry {
	std::string window_title;
	bool urgent = false;
	bool focused = false;
};

class Hy3TabBar {
public:
	CTexture texture;

	void updateWithGroupEntries(Hy3Node&);
	void setPos(Vector2D);
	void setSize(Vector2D);

	// Redraw the texture if necessary, and bind it to GL_TEXTURE_2D
	void prepareTexture();
private:
	bool needs_redraw = true;

	std::vector<Hy3TabBarEntry> entries;
	// scaled pos/size
	Vector2D pos;
	Vector2D size;
};

class Hy3TabGroup {
public:
	Hy3TabBar bar;
	CAnimatedVariable pos;
	CAnimatedVariable size;

	// initialize a group with the given node. UB if node is not a group.
	Hy3TabGroup(Hy3Node&);

	// update tab bar with node position and data. UB if node is not a group.
	void updateWithGroup(Hy3Node&);
	// render the scaled tab bar on the current monitor.
	void renderTabBar();

private:
	// moving a Hy3TabGroup will unregister any active animations
	Hy3TabGroup(Hy3TabGroup&&) = delete;
};
