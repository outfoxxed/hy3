#pragma once

#include <list>
#include <src/layout/IHyprLayout.hpp>

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
	Hy3Node* lastFocusedChild = nullptr;

	bool hasChild(Hy3Node* child);

	Hy3GroupData(Hy3GroupLayout layout);

private:
	Hy3GroupData(Hy3GroupData&&) = default;
	Hy3GroupData(const Hy3GroupData&) = default;

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
	Hy3NodeData(const Hy3NodeData&);
	Hy3NodeData(Hy3NodeData&&);
	Hy3NodeData& operator=(const Hy3NodeData&);
};

struct Hy3Node {
	Hy3Node* parent = nullptr;
	Hy3NodeData data;
	Vector2D position;
	Vector2D size;
	float size_ratio = 1.0;
	int workspace_id = -1;
	bool valid = true;
	Hy3Layout* layout = nullptr;

	void recalcSizePosRecursive(bool force = false);
	std::string debugNode();
	void markFocused();
	void focus();
	void raiseToTop();
	Hy3Node* getFocusedNode();
	void updateDecos();

	bool operator==(const Hy3Node&) const;

	// Attempt to swallow a group. returns true if swallowed
	static bool swallowGroups(Hy3Node*);
	// Remove this node from its parent, deleting the parent if it was
	// the only child and recursing if the parent was the only child of it's parent.
	Hy3Node* removeFromParentRecursive();

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

	void makeGroupOn(int, Hy3GroupLayout);
	void shiftWindow(int, ShiftDirection);
	void shiftFocus(int, ShiftDirection);
	void raiseFocus(int);

	bool shouldRenderSelected(CWindow*);

	Hy3Node* getWorkspaceRootGroup(const int&);

	std::list<Hy3Node> nodes;
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
	Hy3Node* shiftOrGetFocus(Hy3Node&, ShiftDirection, bool);

	friend struct Hy3Node;
};
