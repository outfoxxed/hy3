#pragma once

class Hy3TabGroup;
class Hy3TabBar;

#include <list>
#include <memory>
#include <vector>

#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/render/Texture.hpp>

#include "Hy3Node.hpp"

struct Hy3TabBarEntry {
	std::string window_title;
	bool destroying = false;
	CTexture texture;
	CAnimatedVariable focused;
	CAnimatedVariable urgent;
	CAnimatedVariable offset;       // 0.0-1.0 of total bar
	CAnimatedVariable width;        // 0.0-1.0 of total bar
	CAnimatedVariable vertical_pos; // 0.0-1.0, user specified direction
	CAnimatedVariable fade_opacity; // 0.0-1.0
	Hy3TabBar& tab_bar;
	Hy3Node& node; // only used for comparioson. do not deref.

	struct {
		int x, y;
		float rounding = 0.0;
		float scale = 0.0;
		float focused = 0.0;
		float urgent = 0.0;
		std::string window_title;

		std::string text_font;
		int text_height = 0;
		int text_padding = 0;
		int col_active = 0;
		int col_urgent = 0;
		int col_inactive = 0;
		int col_text_active = 0;
		int col_text_urgent = 0;
		int col_text_inactive = 0;
	} last_render;

	Hy3TabBarEntry(Hy3TabBar&, Hy3Node&);
	bool operator==(const Hy3Node&) const;
	bool operator==(const Hy3TabBarEntry&) const;

	void setFocused(bool);
	void setUrgent(bool);
	void setWindowTitle(std::string);
	void beginDestroy();
	void unDestroy();
	bool shouldRemove();
	void prepareTexture(float, CBox&);
};

class Hy3TabBar {
public:
	bool destroy = false;
	bool dirty = true;
	bool damaged = true;
	CAnimatedVariable fade_opacity;

	Hy3TabBar();
	void beginDestroy();

	void tick();
	void updateNodeList(std::list<Hy3Node*>& nodes);
	void updateAnimations(bool warp = false);
	void setSize(Vector2D);

	std::list<Hy3TabBarEntry> entries;

private:
	Hy3Node* focused_node = nullptr;
	Vector2D size;

	// Tab bar entries take a reference to `this`.
	Hy3TabBar(Hy3TabBar&&) = delete;
	Hy3TabBar(const Hy3TabBar&) = delete;
};

class Hy3TabGroup {
public:
	CWindow* target_window = nullptr;
	int workspace_id = -1;
	bool hidden = false;
	Hy3TabBar bar;
	CAnimatedVariable pos;
	CAnimatedVariable size;

	// initialize a group with the given node. UB if node is not a group.
	Hy3TabGroup(Hy3Node&);

	// update tab bar with node position and data. UB if node is not a group.
	void updateWithGroup(Hy3Node&, bool warp);
	void tick();
	// render the scaled tab bar on the current monitor.
	void renderTabBar();

private:
	std::vector<CWindow*> stencil_windows;
	Vector2D last_pos;
	Vector2D last_size;

	Hy3TabGroup();

	// moving a Hy3TabGroup will unregister any active animations
	Hy3TabGroup(Hy3TabGroup&&) = delete;

	// UB if node is not a group.
	void updateStencilWindows(Hy3Node&);
};
