#pragma once

struct Hy3Node;
enum class Hy3GroupLayout;

#include <list>

#include <hyprland/src/Window.hpp>

#include "Hy3Layout.hpp"
#include "TabGroup.hpp"

enum class Hy3GroupLayout {
	SplitH,
	SplitV,
	Tabbed,
};

enum class Hy3NodeType {
	Window,
	Group,
};

struct Hy3GroupData {
	Hy3GroupLayout layout = Hy3GroupLayout::SplitH;
	std::list<Hy3Node*> children;
	bool group_focused = true;
	Hy3Node* focused_child = nullptr;
	bool ephemeral = false;
	bool containment = false;
	Hy3TabGroup* tab_bar = nullptr;

	Hy3GroupData(Hy3GroupLayout layout);
	~Hy3GroupData();

	bool hasChild(Hy3Node* child);

private:
	Hy3GroupData(Hy3GroupData&&);
	Hy3GroupData(const Hy3GroupData&) = delete;

	friend class Hy3NodeData;
};

class Hy3NodeData {
public:
	Hy3NodeType type;
	union {
		Hy3GroupData as_group;
		CWindow* as_window;
	};

	Hy3NodeData();
	Hy3NodeData(CWindow* window);
	Hy3NodeData(Hy3GroupLayout layout);
	~Hy3NodeData();

	Hy3NodeData& operator=(CWindow*);
	Hy3NodeData& operator=(Hy3GroupLayout);

	bool operator==(const Hy3NodeData&) const;

	// private: - I give up, C++ wins
	Hy3NodeData(Hy3GroupData);
	Hy3NodeData(Hy3NodeData&&);
	Hy3NodeData& operator=(Hy3NodeData&&);
};

struct Hy3Node {
	Hy3Node* parent = nullptr;
	Hy3NodeData data;
	Vector2D position;
	Vector2D size;
	Vector2D gap_pos_offset;
	Vector2D gap_size_offset;
	float size_ratio = 1.0;
	int workspace_id = -1;
	bool hidden = false;
	Hy3Layout* layout = nullptr;

	bool operator==(const Hy3Node&) const;

	void focus();
	bool focusWindow();
	void markFocused();
	void raiseToTop();
	Hy3Node* getFocusedNode(bool ignore_group_focus = false);
	bool isIndirectlyFocused();

	void recalcSizePosRecursive(bool no_animation = false);
	void updateTabBar(bool no_animation = false);
	void updateTabBarRecursive();
	void updateDecos();

	std::string getTitle();
	bool isUrgent();
	void setHidden(bool);

	Hy3Node* findNodeForTabGroup(Hy3TabGroup&);
	void appendAllWindows(std::vector<CWindow*>&);
	std::string debugNode();

	// Remove this node from its parent, deleting the parent if it was
	// the only child and recursing if the parent was the only child of it's
	// parent.
	Hy3Node* removeFromParentRecursive();

	// Replace this node with a group, returning this node's new address.
	Hy3Node* intoGroup(Hy3GroupLayout, GroupEphemeralityOption);

	// Attempt to swallow a group. returns true if swallowed
	static bool swallowGroups(Hy3Node* into);
	static void swapData(Hy3Node&, Hy3Node&);
};
