#pragma once

struct Hy3Node;
#include "TabGroup.hpp"

#include <list>
#include <hyprland/src/layout/IHyprLayout.hpp>

class Hy3Layout;
struct Hy3Node;

enum class Hy3GroupLayout {
	SplitH,
	SplitV,
	Tabbed,
};

enum class ShiftDirection {
	Left,
	Up,
	Down,
	Right,
};

struct Hy3GroupData {
	Hy3GroupLayout layout = Hy3GroupLayout::SplitH;
	std::list<Hy3Node*> children;
	bool group_focused = true;
	Hy3Node* focused_child = nullptr;
	Hy3TabGroup* tab_bar = nullptr;

	bool hasChild(Hy3Node* child);

	Hy3GroupData(Hy3GroupLayout layout);
	~Hy3GroupData();

private:
	Hy3GroupData(Hy3GroupData&&);
	Hy3GroupData(const Hy3GroupData&) = delete;

	friend class Hy3NodeData;
};

class Hy3NodeData {
public:
	enum { Group, Window } type;
	union {
		Hy3GroupData as_group;
		CWindow* as_window;
	};

	bool operator==(const Hy3NodeData&) const;

	Hy3NodeData();
	~Hy3NodeData();
	Hy3NodeData(CWindow*);
	Hy3NodeData(Hy3GroupLayout);
	Hy3NodeData& operator=(CWindow*);
	Hy3NodeData& operator=(Hy3GroupLayout);

	//private: - I give up, C++ wins
	Hy3NodeData(Hy3GroupData);
	Hy3NodeData(Hy3NodeData&&);
	Hy3NodeData& operator=(Hy3NodeData&&);
};

struct Hy3Node {
	Hy3Node* parent = nullptr;
	Hy3NodeData data;
	Vector2D position;
	Vector2D size;
	float size_ratio = 1.0;
	int workspace_id = -1;
	bool hidden = false;
	bool valid = true;
	Hy3Layout* layout = nullptr;

	void recalcSizePosRecursive(bool force = false);
	std::string debugNode();
	void markFocused();
	void focus();
	bool focusWindow();
	void raiseToTop();
	Hy3Node* getFocusedNode();
	void updateDecos();
	void setHidden(bool hidden);
	void updateTabBar();
	void updateTabBarRecursive();
	bool isUrgent();
	bool isIndirectlyFocused();
	std::string getTitle();

	bool operator==(const Hy3Node&) const;

	// Attempt to swallow a group. returns true if swallowed
	static bool swallowGroups(Hy3Node*);
	// Remove this node from its parent, deleting the parent if it was
	// the only child and recursing if the parent was the only child of it's parent.
	Hy3Node* removeFromParentRecursive();

	// Replace this node with a group, returning this node's new address.
	Hy3Node* intoGroup(Hy3GroupLayout);

	static void swapData(Hy3Node&, Hy3Node&);
};

class Hy3Layout: public IHyprLayout {
public:
	virtual void onWindowCreatedTiling(CWindow*);
	virtual void onWindowRemovedTiling(CWindow*);
	virtual void onWindowFocusChange(CWindow*);
	virtual bool isWindowTiled(CWindow*);
	virtual void recalculateMonitor(const int&);
	virtual void recalculateWindow(CWindow*);
	virtual void onBeginDragWindow();
	virtual void resizeActiveWindow(const Vector2D&, CWindow* pWindow = nullptr);
	virtual void fullscreenRequestForWindow(CWindow*, eFullscreenMode, bool);
	virtual std::any layoutMessage(SLayoutMessageHeader, std::string);
	virtual SWindowRenderLayoutHints requestRenderHints(CWindow*);
	virtual void switchWindows(CWindow*, CWindow*);
	virtual void alterSplitRatio(CWindow*, float, bool);
	virtual std::string getLayoutName();
	virtual CWindow* getNextWindowCandidate(CWindow*);
	virtual void replaceWindowDataWith(CWindow*, CWindow*);

	virtual void onEnable();
	virtual void onDisable();

	void makeGroupOnWorkspace(int, Hy3GroupLayout);
	void makeOppositeGroupOnWorkspace(int);
	void makeGroupOn(Hy3Node*, Hy3GroupLayout);
	void makeOppositeGroupOn(Hy3Node*);
	void shiftWindow(int, ShiftDirection, bool);
	void shiftFocus(int, ShiftDirection);
	void raiseFocus(int);

	bool shouldRenderSelected(CWindow*);

	Hy3Node* getWorkspaceRootGroup(const int&);
	Hy3Node* getWorkspaceFocusedNode(const int&);

	static void renderHook(void*, std::any);
	static void windowTitleHook(void*, std::any);
	static void tickHook(void*, std::any);

	std::list<Hy3Node> nodes;
	std::list<Hy3TabGroup> tab_groups;
private:
	struct {
		bool started = false;
		bool xExtent = false;
		bool yExtent = false;
	} drag_flags;

	int getWorkspaceNodeCount(const int&);
	Hy3Node* getNodeFromWindow(CWindow*);
	void applyNodeDataToWindow(Hy3Node*, bool force = false);

	// if shift is true, shift the window in the given direction, returning nullptr,
	// if shift is false, return the window in the given direction or nullptr.
	// if once is true, only one group will be broken out of / into
	Hy3Node* shiftOrGetFocus(Hy3Node&, ShiftDirection, bool, bool);

	friend struct Hy3Node;
};
