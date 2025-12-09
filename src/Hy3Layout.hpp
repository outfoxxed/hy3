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

enum class TabLockMode {
	Lock,
	Unlock,
	Toggle,
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

PHLWORKSPACE workspace_for_action(bool allow_fullscreen = false);

class Hy3Layout: public IHyprLayout {
public:
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
	void makeGroupOnWorkspace(
	    const CWorkspace* workspace,
	    Hy3GroupLayout,
	    GroupEphemeralityOption,
	    bool toggle
	);
	void makeOppositeGroupOnWorkspace(const CWorkspace* workspace, GroupEphemeralityOption);
	void changeGroupOnWorkspace(const CWorkspace* workspace, Hy3GroupLayout);
	void untabGroupOnWorkspace(const CWorkspace* workspace);
	void toggleTabGroupOnWorkspace(const CWorkspace* workspace);
	void changeGroupToOppositeOnWorkspace(const CWorkspace* workspace);
	void changeGroupEphemeralityOnWorkspace(const CWorkspace* workspace, bool ephemeral);
	void makeGroupOn(Hy3Node*, Hy3GroupLayout, GroupEphemeralityOption);
	void makeOppositeGroupOn(Hy3Node*, GroupEphemeralityOption);
	void changeGroupOn(Hy3Node&, Hy3GroupLayout);
	void untabGroupOn(Hy3Node&);
	void toggleTabGroupOn(Hy3Node&);
	void changeGroupToOppositeOn(Hy3Node&);
	void changeGroupEphemeralityOn(Hy3Node&, bool ephemeral);
	void shiftNode(Hy3Node&, ShiftDirection, bool once, bool visible);
	void shiftWindow(const CWorkspace* workspace, ShiftDirection, bool once, bool visible);
	void shiftFocus(const CWorkspace* workspace, ShiftDirection, bool visible, bool warp);
	void toggleFocusLayer(const CWorkspace* workspace, bool warp);
	bool shiftMonitor(Hy3Node&, ShiftDirection, bool follow);
	Hy3Node* focusMonitor(ShiftDirection);

	void warpCursor();
	void moveNodeToWorkspace(CWorkspace* origin, std::string wsname, bool follow, bool warp);
	void changeFocus(const CWorkspace* workspace, FocusShift);
	void focusTab(
	    const CWorkspace* workspace,
	    TabFocus target,
	    TabFocusMousePriority,
	    bool wrap_scroll,
	    int index
	);
	void setNodeSwallow(const CWorkspace* workspace, SetSwallowOption);
	void killFocusedNode(const CWorkspace* workspace);
	void expand(const CWorkspace* workspace, ExpandOption, ExpandFullscreenOption);
	void setTabLock(const CWorkspace* workspace, TabLockMode);
	static void warpCursorToBox(const Vector2D& pos, const Vector2D& size);
	static void warpCursorWithFocus(const Vector2D& pos, bool force = false);

	bool shouldRenderSelected(const Desktop::View::CWindow*);
	PHLWINDOW findTiledWindowCandidate(const Desktop::View::CWindow* from);
	PHLWINDOW findFloatingWindowCandidate(const Desktop::View::CWindow* from);

	Hy3Node* getWorkspaceRootGroup(const CWorkspace* workspace);
	Hy3Node* getWorkspaceFocusedNode(
	    const CWorkspace* workspace,
	    bool ignore_group_focus = false,
	    bool stop_at_expanded = false
	);

	static void renderHook(void*, SCallbackInfo&, std::any);
	static void windowGroupUrgentHook(void*, SCallbackInfo&, std::any);
	static void windowGroupUpdateRecursiveHook(void*, SCallbackInfo&, std::any);
	static void tickHook(void*, SCallbackInfo&, std::any);
	static void mouseButtonHook(void*, SCallbackInfo&, std::any);

	std::list<Hy3Node> nodes;
	std::list<Hy3TabGroup> tab_groups;

private:
	Hy3Node* getNodeFromWindow(const Desktop::View::CWindow*);
	void applyNodeDataToWindow(Hy3Node*, bool no_animation = false);

	// if shift is true, shift the window in the given direction, returning
	// nullptr, if shift is false, return the window in the given direction or
	// nullptr. if once is true, only one group will be broken out of / into
	Hy3Node* shiftOrGetFocus(Hy3Node*, ShiftDirection, bool shift, bool once, bool visible);

	void updateAutotileWorkspaces();
	bool shouldAutotileWorkspace(const CWorkspace* workspace);
	void resizeNode(Hy3Node*, Vector2D, ShiftDirection resize_edge_x, ShiftDirection resize_edge_y);

	struct {
		std::string raw_workspaces;
		bool workspace_blacklist;
		std::set<int> workspaces;
	} autotile;

	friend struct Hy3Node;
};
