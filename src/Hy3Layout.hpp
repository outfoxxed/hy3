#pragma once

#include <hyprland/src/desktop/DesktopTypes.hpp>
class Hy3Layout;

enum class GroupEphemeralityOption {
	Ephemeral,
	Standard,
	ForceEphemeral,
};

#include <list>
#include <set>

#include <hyprland/src/layout/IHyprLayout.hpp>

enum class ShiftDirection {
	Left,
	Up,
	Down,
	Right,
};
inline static constexpr char getShiftDirectionChar(ShiftDirection direction) {
	return direction == ShiftDirection::Left ? 'l'
	     : direction == ShiftDirection::Up   ? 'u'
	     : direction == ShiftDirection::Down ? 'd'
	                                         : 'r';
}

enum class Axis { None, Horizontal, Vertical };

#include "Hy3Node.hpp"
#include "TabGroup.hpp"

enum class FocusShift {
	Top,
	Bottom,
	Raise,
	Lower,
	Tab,
	TabNode,
};

enum class TabFocus {
	MouseLocation,
	Left,
	Right,
	Index,
};

enum class TabFocusMousePriority {
	Ignore,
	Prioritize,
	Require,
};

enum class SetSwallowOption {
	NoSwallow,
	Swallow,
	Toggle,
};

enum class ExpandOption {
	Expand,
	Shrink,
	Base,
	Maximize,
	Fullscreen,
};

enum class ExpandFullscreenOption {
	MaximizeOnly,
	MaximizeIntermediate,
	MaximizeAsFullscreen,
};

class Hy3Layout: public IHyprLayout {
public:
	void onWindowCreated(PHLWINDOW, eDirection = DIRECTION_DEFAULT) override;
	void onWindowCreatedTiling(PHLWINDOW, eDirection = DIRECTION_DEFAULT) override;
	void onWindowRemovedTiling(PHLWINDOW) override;
	void onWindowFocusChange(PHLWINDOW) override;
	bool isWindowTiled(PHLWINDOW) override;
	void recalculateMonitor(const MONITORID& monitor_id) override;
	void recalculateWindow(PHLWINDOW) override;
	void resizeActiveWindow(const Vector2D& delta, eRectCorner corner, PHLWINDOW pWindow = nullptr)
	    override;
	void
	fullscreenRequestForWindow(PHLWINDOW, eFullscreenMode current_mode, eFullscreenMode target_mode)
	    override;
	std::any layoutMessage(SLayoutMessageHeader header, std::string content) override;
	SWindowRenderLayoutHints requestRenderHints(PHLWINDOW) override;
	void switchWindows(PHLWINDOW, PHLWINDOW) override;
	void moveWindowTo(PHLWINDOW, const std::string& direction, bool silent) override;
	void alterSplitRatio(PHLWINDOW, float, bool) override;
	std::string getLayoutName() override;
	PHLWINDOW getNextWindowCandidate(PHLWINDOW) override;
	void replaceWindowDataWith(PHLWINDOW from, PHLWINDOW to) override;
	bool isWindowReachable(PHLWINDOW) override;
	void bringWindowToTop(PHLWINDOW) override;
	Vector2D predictSizeForNewWindowTiled() override { return Vector2D(); }

	void onEnable() override;
	void onDisable() override;

	void insertNode(Hy3Node& node);
	void makeGroupOnWorkspace(const PHLWORKSPACE& workspace, Hy3GroupLayout, GroupEphemeralityOption);
	void makeOppositeGroupOnWorkspace(const PHLWORKSPACE& workspace, GroupEphemeralityOption);
	void changeGroupOnWorkspace(const PHLWORKSPACE& workspace, Hy3GroupLayout);
	void untabGroupOnWorkspace(const PHLWORKSPACE& workspace);
	void toggleTabGroupOnWorkspace(const PHLWORKSPACE& workspace);
	void changeGroupToOppositeOnWorkspace(const PHLWORKSPACE& workspace);
	void changeGroupEphemeralityOnWorkspace(const PHLWORKSPACE& workspace, bool ephemeral);
	void makeGroupOn(Hy3Node*, Hy3GroupLayout, GroupEphemeralityOption);
	void makeOppositeGroupOn(Hy3Node*, GroupEphemeralityOption);
	void changeGroupOn(Hy3Node&, Hy3GroupLayout);
	void untabGroupOn(Hy3Node&);
	void toggleTabGroupOn(Hy3Node&);
	void changeGroupToOppositeOn(Hy3Node&);
	void changeGroupEphemeralityOn(Hy3Node&, bool ephemeral);
	void shiftNode(Hy3Node&, ShiftDirection, bool once, bool visible);
	void shiftWindow(const PHLWORKSPACE& workspace, ShiftDirection, bool once, bool visible);
	void shiftFocus(const PHLWORKSPACE& workspace, ShiftDirection, bool visible, bool warp);
	void toggleFocusLayer(const PHLWORKSPACE& workspace, bool warp);
	bool shiftMonitor(Hy3Node&, ShiftDirection, bool follow);
	Hy3Node* focusMonitor(ShiftDirection);

	void warpCursor();
	void moveNodeToWorkspace(const PHLWORKSPACE& origin, std::string wsname, bool follow);
	void changeFocus(const PHLWORKSPACE& workspace, FocusShift);
	void focusTab(
	    const PHLWORKSPACE& workspace,
	    TabFocus target,
	    TabFocusMousePriority,
	    bool wrap_scroll,
	    int index
	);
	void setNodeSwallow(const PHLWORKSPACE& workspace, SetSwallowOption);
	void killFocusedNode(const PHLWORKSPACE& workspace);
	void expand(const PHLWORKSPACE& workspace, ExpandOption, ExpandFullscreenOption);
	static void warpCursorToBox(const Vector2D& pos, const Vector2D& size);

	bool shouldRenderSelected(const PHLWINDOW&);
	PHLWINDOW findTiledWindowCandidate(const PHLWINDOW& from);
	PHLWINDOW findFloatingWindowCandidate(const PHLWINDOW& from);

	Hy3Node* getWorkspaceRootGroup(const PHLWORKSPACE& workspace);
	Hy3Node* getWorkspaceFocusedNode(
	    const PHLWORKSPACE& workspace,
	    bool ignore_group_focus = false,
	    bool stop_at_expanded = false
	);

	static void renderHook(void*, SCallbackInfo&, std::any);
	static void windowGroupUrgentHook(void*, SCallbackInfo&, std::any);
	static void windowGroupUpdateRecursiveHook(void*, SCallbackInfo&, std::any);
	static void tickHook(void*, SCallbackInfo&, std::any);

	std::list<Hy3Node> nodes;
	std::list<Hy3TabGroup> tab_groups;

private:
	Hy3Node* getNodeFromWindow(const PHLWINDOW&);
	void applyNodeDataToWindow(Hy3Node*, bool no_animation = false);

	// if shift is true, shift the window in the given direction, returning
	// nullptr, if shift is false, return the window in the given direction or
	// nullptr. if once is true, only one group will be broken out of / into
	Hy3Node* shiftOrGetFocus(Hy3Node&, ShiftDirection, bool shift, bool once, bool visible);

	void updateAutotileWorkspaces();
	bool shouldAutotileWorkspace(const PHLWORKSPACE& workspace);
	void resizeNode(Hy3Node*, Vector2D, ShiftDirection resize_edge_x, ShiftDirection resize_edge_y);

	struct {
		std::string raw_workspaces;
		bool workspace_blacklist;
		std::set<int> workspaces;
	} autotile;

	friend struct Hy3Node;
};
