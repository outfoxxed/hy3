#pragma once

#include <hyprland/src/helpers/AnimatedVariable.hpp>
#include <hyprland/src/helpers/Vector2D.hpp>
#include <list>
#include <vector>

class Hy3TabGroup;
class Hy3TabBar;

#include "Hy3Layout.hpp"
#include <hyprland/src/render/Texture.hpp>

struct Hy3TabBarEntry {
	std::string window_title;
	bool focused = false;
	bool urgent = false;
	bool needs_redraw = false;
	CTexture texture;
	CAnimatedVariable offset; // offset 0, 0.0-1.0 of total bar
	CAnimatedVariable width; // 0.0-1.0 of total bar
	Hy3TabBar& tab_bar;
	Hy3Node& node; // only used for comparioson. do not deref.
	Vector2D last_render_size;
	float last_render_scale = 0.0;
	float last_render_rounding = 0.0;

	Hy3TabBarEntry(Hy3TabBar&, Hy3Node&);
	bool operator==(const Hy3Node& node) const;

	void prepareTexture(float scale, Vector2D size);
};

class Hy3TabBar {
public:
	CAnimatedVariable vertical_pos;
	CAnimatedVariable fade_opacity;

	Hy3TabBar();

	void focusNode(Hy3Node*);
	void updateNodeList(std::list<Hy3Node*>& nodes);
	void updateAnimations(bool warp = false);
	void setSize(Vector2D);

	std::list<Hy3TabBarEntry> entries;
private:
	Hy3Node* focused_node = nullptr;
	CAnimatedVariable focus_opacity;
	CAnimatedVariable focus_start;
	CAnimatedVariable focus_end;

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
	std::vector<CWindow*> stencil_windows;

	// moving a Hy3TabGroup will unregister any active animations
	Hy3TabGroup(Hy3TabGroup&&) = delete;

	// UB if node is not a group.
	void updateStencilWindows(Hy3Node&);
};
