#pragma once

#include <hyprland/src/helpers/AnimatedVariable.hpp>
#include <hyprland/src/helpers/Vector2D.hpp>
#include <list>

class Hy3TabGroup;
class Hy3TabBar;

#include "Hy3Layout.hpp"
#include <hyprland/src/render/Texture.hpp>

struct Hy3TabBarEntry {
	std::string window_title;
	bool urgent = false;
	CAnimatedVariable offset; // offset 0, 0.0-1.0 of total bar
	CAnimatedVariable width; // 0.0-1.0 of total bar
	Hy3TabBar& tab_bar;
	Hy3Node& node; // only used for comparioson. do not deref.

	Hy3TabBarEntry(Hy3TabBar&, Hy3Node&);
	bool operator==(const Hy3Node& node) const;
};

class Hy3TabBar {
public:
	CTexture mask_texture;
	CAnimatedVariable vertical_pos;
	CAnimatedVariable fade_opacity;

	Hy3TabBar();

	void focusNode(Hy3Node*);
	void updateNodeList(std::list<Hy3Node*>& nodes);
	void updateAnimations(bool warp = false);
	void setSize(Vector2D);

	// Redraw the mask texture if necessary, and bind it to GL_TEXTURE_2D
	void prepareMask();

private:
	bool need_mask_redraw = false;
	int last_mask_rounding = 0;

	Hy3Node* focused_node = nullptr;
	CAnimatedVariable focus_opacity;
	CAnimatedVariable focus_start;
	CAnimatedVariable focus_end;

	std::list<Hy3TabBarEntry> entries;
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
