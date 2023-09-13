#pragma once

class Hy3Layout;

enum class GroupEphemeralityOption {
	Ephemeral,
	Standard,
	ForceEphemeral,
};

#include <list>

#include <hyprland/src/layout/IHyprLayout.hpp>

#include "Hy3Node.hpp"
#include "TabGroup.hpp"

enum class ShiftDirection {
	Left,
	Up,
	Down,
	Right,
};

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
	virtual void onWindowCreated(CWindow*);
  virtual void onWindowCreatedTiling(CWindow*, eDirection direction = DIRECTION_DEFAULT);
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

	virtual void onEnable();
	virtual void onDisable();

	void makeGroupOnWorkspace(int workspace, Hy3GroupLayout, GroupEphemeralityOption);
	void makeOppositeGroupOnWorkspace(int workspace, GroupEphemeralityOption);
	void changeGroupOnWorkspace(int workspace, Hy3GroupLayout);
	void untabGroupOnWorkspace(int workspace);
	void toggleTabGroupOnWorkspace(int workspace);
	void changeGroupToOppositeOnWorkspace(int workspace);
	void changeGroupEphemeralityOnWorkspace(int workspace, bool ephemeral);
	void makeGroupOn(Hy3Node*, Hy3GroupLayout, GroupEphemeralityOption);
	void makeOppositeGroupOn(Hy3Node*, GroupEphemeralityOption);
	void changeGroupOn(Hy3Node&, Hy3GroupLayout);
	void untabGroupOn(Hy3Node&);
	void toggleTabGroupOn(Hy3Node&);
	void changeGroupToOppositeOn(Hy3Node&);
	void changeGroupEphemeralityOn(Hy3Node&, bool ephemeral);
	void shiftNode(Hy3Node&, ShiftDirection, bool once, bool visible);
	void shiftWindow(int workspace, ShiftDirection, bool once, bool visible);
	void shiftFocus(int workspace, ShiftDirection, bool visible);
	void changeFocus(int workspace, FocusShift);
	void focusTab(int workspace, TabFocus target, TabFocusMousePriority, bool wrap_scroll, int index);
	void setNodeSwallow(int workspace, SetSwallowOption);
	void killFocusedNode(int workspace);
	void expand(int workspace, ExpandOption, ExpandFullscreenOption);

	bool shouldRenderSelected(CWindow*);

	Hy3Node* getWorkspaceRootGroup(const int& workspace);
	Hy3Node* getWorkspaceFocusedNode(
	    const int& workspace,
	    bool ignore_group_focus = false,
	    bool stop_at_expanded = false
	);

	static void renderHook(void*, std::any);
	static void windowGroupUrgentHook(void*, std::any);
	static void windowGroupUpdateRecursiveHook(void*, std::any);
	static void tickHook(void*, std::any);

	std::list<Hy3Node> nodes;
	std::list<Hy3TabGroup> tab_groups;

private:
	Hy3Node* getNodeFromWindow(CWindow*);
	void applyNodeDataToWindow(Hy3Node*, bool no_animation = false);

	// if shift is true, shift the window in the given direction, returning
	// nullptr, if shift is false, return the window in the given direction or
	// nullptr. if once is true, only one group will be broken out of / into
	Hy3Node* shiftOrGetFocus(Hy3Node&, ShiftDirection, bool shift, bool once, bool visible);

	friend struct Hy3Node;
};
