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
	virtual void onWindowCreated(CWindow*, eDirection = DIRECTION_DEFAULT);
	virtual void onWindowCreatedTiling(CWindow*, eDirection = DIRECTION_DEFAULT);
	virtual void onWindowRemovedTiling(CWindow*);
	virtual void onWindowFocusChange(CWindow*);
	virtual bool isWindowTiled(CWindow*);
	virtual void recalculateMonitor(const int& monitor_id);
	virtual void recalculateWindow(CWindow*);
	virtual void
	resizeActiveWindow(const Vector2D& delta, eRectCorner corner, CWindow* pWindow = nullptr);
	virtual void fullscreenRequestForWindow(CWindow*, eFullscreenMode, bool enable_fullscreen);
	virtual std::any layoutMessage(SLayoutMessageHeader header, std::string content);
	virtual SWindowRenderLayoutHints requestRenderHints(CWindow*);
	virtual void switchWindows(CWindow*, CWindow*);
	virtual void moveWindowTo(CWindow*, const std::string& direction);
	virtual void alterSplitRatio(CWindow*, float, bool);
	virtual std::string getLayoutName();
	virtual CWindow* getNextWindowCandidate(CWindow*);
	virtual void replaceWindowDataWith(CWindow* from, CWindow* to);
	virtual bool isWindowReachable(CWindow*);
	virtual void bringWindowToTop(CWindow*);
	virtual Vector2D predictSizeForNewWindowTiled() { return Vector2D(); }

	virtual void onEnable();
	virtual void onDisable();

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
	void shiftFocus(const PHLWORKSPACE& workspace, ShiftDirection, bool visible);
	void moveNodeToWorkspace(const PHLWORKSPACE& origin, std::string wsname, bool follow);
	void changeFocus(const PHLWORKSPACE& workspace, FocusShift);
	void focusTab(const PHLWORKSPACE& workspace, TabFocus target, TabFocusMousePriority, bool wrap_scroll, int index);
	void setNodeSwallow(const PHLWORKSPACE& workspace, SetSwallowOption);
	void killFocusedNode(const PHLWORKSPACE& workspace);
	void expand(const PHLWORKSPACE& workspace, ExpandOption, ExpandFullscreenOption);

	bool shouldRenderSelected(CWindow*);

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
	Hy3Node* getNodeFromWindow(CWindow*);
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
