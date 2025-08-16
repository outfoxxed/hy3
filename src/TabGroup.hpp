#pragma once

#include <optional>
#include <utility>
class Hy3TabGroup;
class Hy3TabBar;

#include <list>
#include <vector>

#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprland/src/render/Renderer.hpp>
#include <hyprland/src/render/Texture.hpp>

#include "Hy3Node.hpp"

struct Hy3TabBarEntry {
	std::string window_title;
	bool destroying = false;
	SP<CTexture> texture;
	PHLANIMVAR<float> active;
	PHLANIMVAR<float> focused;
	PHLANIMVAR<float> urgent;
	PHLANIMVAR<float> active_monitor;
	PHLANIMVAR<float> offset;       // 0.0-1.0 of total bar
	PHLANIMVAR<float> width;        // 0.0-1.0 of total bar
	PHLANIMVAR<float> vertical_pos; // 0.0-1.0, user specified direction
	PHLANIMVAR<float> fade_opacity; // 0.0-1.0
	Hy3TabBar& tab_bar;
	Hy3Node& node; // only used for comparison. do not deref.

	struct {
		float scale = 0.0;
		std::string window_title;
		int full_logical_width = 0;
		float render_width = 0;

		std::string text_font;
		int font_height = 0;

		int texture_x_offset = 0;
		int texture_y_offset = 0;
		int texture_width = 0;
		int texture_height = 0;

		int logical_width = 0;
		int logical_height = 0;
	} last_render;

	Hy3TabBarEntry(Hy3TabBar&, Hy3Node&);
	bool operator==(const Hy3Node&) const;
	bool operator==(const Hy3TabBarEntry&) const;

	void setActive(bool);
	void setFocused(bool);
	void setUrgent(bool);
	void setWindowTitle(std::string);
	void setMonitorActive(bool);
	void beginDestroy();
	void unDestroy();
	bool shouldRemove();
	void render(float scale, CBox& box, float opacity_mul);

private:
	void renderText(float scale, CBox& box, float opacity);
	CHyprColor mergeColors(
	    const CHyprColor& active,
	    const CHyprColor& focused,
	    const CHyprColor& urgent,
	    const CHyprColor& locked,
	    const CHyprColor& inactiveMonitor,
	    const CHyprColor& inactive
	);
};

class Hy3TabBar {
public:
	bool destroy = false;
	bool dirty = true;
	bool damaged = true;
	PHLANIMVAR<float> fade_opacity;
	PHLANIMVAR<float> locked;
	// The monitor this bar resides on
	MONITORID monitor_id = MONITOR_INVALID;

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

class Hy3TabPassElement: public IPassElement {
public:
	Hy3TabPassElement(Hy3TabGroup* group): group(group) {}

	const char* passName() override { return "Hy3TabPassElement"; }
	void draw(const CRegion& damage) override;
	bool needsLiveBlur() override { return false; }
	bool needsPrecomputeBlur() override;
	std::optional<CBox> boundingBox() override;

private:
	Hy3TabGroup* group;
};

class Hy3TabGroup {
public:
	PHLWINDOW target_window = nullptr;
	PHLWORKSPACE workspace = nullptr;
	bool hidden = false;
	Hy3TabBar bar;
	PHLANIMVAR<Vector2D> pos;
	PHLANIMVAR<Vector2D> size;

	// initialize a group with the given node. UB if node is not a group.
	Hy3TabGroup(Hy3Node&);

	// update tab bar with node position and data. UB if node is not a group.
	void updateWithGroup(Hy3Node&, bool warp);
	void tick();
	std::pair<CBox, CBox> getRenderBB() const;
	// render the scaled tab bar on the current monitor.
	void renderTabBar();

private:
	std::vector<PHLWINDOWREF> stencil_windows;
	Vector2D last_workspace_offset;
	Vector2D last_pos;
	Vector2D last_size;

	Hy3TabGroup();

	// moving a Hy3TabGroup will unregister any active animations
	Hy3TabGroup(Hy3TabGroup&&) = delete;

	// UB if node is not a group.
	void updateStencilWindows(Hy3Node&);
};
