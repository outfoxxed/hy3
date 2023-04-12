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

struct Hy3GroupData {
	Hy3GroupLayout layout = Hy3GroupLayout::SplitH;
	std::list<Hy3Node*> children;

	Hy3GroupData(Hy3GroupLayout layout);
};

struct Hy3NodeData {
	enum { Group, Window } type;
	union {
		Hy3GroupData as_group;
		CWindow* as_window;
	};

	bool operator==(const Hy3NodeData&) const;

	Hy3NodeData();
	Hy3NodeData(CWindow* window);
	Hy3NodeData(Hy3GroupData group);
	Hy3NodeData(const Hy3NodeData&);
	Hy3NodeData(Hy3NodeData&&);
	Hy3NodeData& operator=(const Hy3NodeData&);
	~Hy3NodeData();
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

	bool operator==(const Hy3Node&) const;
};

class Hy3Layout: public IHyprLayout {
public:
	virtual void onWindowCreatedTiling(CWindow*);
	virtual void onWindowRemovedTiling(CWindow*);
	virtual void onWindowFocusChange(CWindow*);
	virtual bool isWindowTiled(CWindow*);
	virtual void recalculateMonitor(const int&);
	virtual void recalculateWindow(CWindow*);
	virtual void resizeActiveWindow(const Vector2D&, CWindow* pWindow = nullptr);
	virtual void fullscreenRequestForWindow(CWindow*, eFullscreenMode, bool);
	virtual std::any layoutMessage(SLayoutMessageHeader, std::string);
	virtual SWindowRenderLayoutHints requestRenderHints(CWindow*);
	virtual void switchWindows(CWindow*, CWindow*);
	virtual void alterSplitRatio(CWindow*, float, bool);
	virtual std::string getLayoutName();
	virtual void replaceWindowDataWith(CWindow* from, CWindow* to);

	virtual void onEnable();
	virtual void onDisable();

private:
	// std::list is used over std::vector because it does not invalidate references
	// when mutated.
	std::list<Hy3Node> nodes;
	CWindow* lastActiveWindow = nullptr;

	int getWorkspaceNodeCount(const int&);
	Hy3Node* getNodeFromWindow(CWindow*);
	Hy3Node* getWorkspaceRootGroup(const int&);
	void applyNodeDataToWindow(Hy3Node*, bool force = false);

	friend struct Hy3Node;
};
