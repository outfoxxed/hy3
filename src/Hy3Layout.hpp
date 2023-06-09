#pragma once

class Hy3Layout;

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

class Hy3Layout: public IHyprLayout {
public:
	virtual void onWindowCreatedTiling(CWindow*);
	virtual void onWindowRemovedTiling(CWindow*);
	virtual void onWindowFocusChange(CWindow*);
	virtual bool isWindowTiled(CWindow*);
	virtual void recalculateMonitor(const int& monitor_id);
	virtual void recalculateWindow(CWindow*);
	virtual void onBeginDragWindow();
	virtual void resizeActiveWindow(const Vector2D& delta, CWindow* pWindow = nullptr);
	virtual void fullscreenRequestForWindow(CWindow*, eFullscreenMode, bool enable_fullscreen);
	virtual std::any layoutMessage(SLayoutMessageHeader header, std::string content);
	virtual SWindowRenderLayoutHints requestRenderHints(CWindow*);
	virtual void switchWindows(CWindow*, CWindow*);
	virtual void alterSplitRatio(CWindow*, float, bool);
	virtual std::string getLayoutName();
	virtual CWindow* getNextWindowCandidate(CWindow*);
	virtual void replaceWindowDataWith(CWindow* from, CWindow* to);

	virtual void onEnable();
	virtual void onDisable();

	void makeGroupOnWorkspace(int workspace, Hy3GroupLayout);
	void makeOppositeGroupOnWorkspace(int workspace);
	void makeGroupOn(Hy3Node*, Hy3GroupLayout);
	void makeOppositeGroupOn(Hy3Node*);
	void shiftWindow(int workspace, ShiftDirection, bool once);
	void shiftFocus(int workspace, ShiftDirection, bool visible);
	void changeFocus(int workspace, FocusShift);
	void focusTab(int workspace, TabFocus target, TabFocusMousePriority, bool wrap_scroll, int index);
	void killFocusedNode(int workspace);

	bool shouldRenderSelected(CWindow*);

	Hy3Node* getWorkspaceRootGroup(const int& workspace);
	Hy3Node* getWorkspaceFocusedNode(const int& workspace);

	static void renderHook(void*, std::any);
	static void windowGroupUrgentHook(void*, std::any);
	static void windowGroupUpdateRecursiveHook(void*, std::any);
	static void tickHook(void*, std::any);

	std::list<Hy3Node> nodes;
	std::list<Hy3TabGroup> tab_groups;

private:
	struct {
		bool started = false;
		bool xExtent = false;
		bool yExtent = false;
	} drag_flags;

	Hy3Node* getNodeFromWindow(CWindow*);
	void applyNodeDataToWindow(Hy3Node*, bool no_animation = false);

	// if shift is true, shift the window in the given direction, returning
	// nullptr, if shift is false, return the window in the given direction or
	// nullptr. if once is true, only one group will be broken out of / into
	Hy3Node* shiftOrGetFocus(Hy3Node&, ShiftDirection, bool shift, bool once, bool visible);

	friend struct Hy3Node;
};
