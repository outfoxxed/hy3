#pragma once

struct Hy3Node;
struct Hy3TargetNode;
struct Hy3GroupNode;
struct Hy3RootNode;
enum class Hy3GroupLayout;

#include <generator>

#include <hyprland/src/defines.hpp>
#include <hyprland/src/desktop/view/Window.hpp>
#include <hyprland/src/layout/target/Target.hpp>

#include "Hy3Layout.hpp"
#include "TabGroup.hpp"

enum class Hy3GroupLayout {
	Root,
	SplitH,
	SplitV,
	Tabbed,
};

enum class Hy3NodeType {
	Target,
	Group,
};

enum class ExpandFocusType {
	NotExpanded,
	Latch,
	Stack,
};

enum class Ephemeral {
	Off,
	Staged,
	Active,
};

enum class CollapsePolicy {
	InvalidOnly,
	EmptySplits,
	SingleNodeGroups,
};

struct Hy3Node {
	WP<Hy3Node> parent;
	WP<Hy3Node> self; // set from owning UP at creation time
	Vector2D position;
	Vector2D size;
	float size_ratio = 1.0;
	bool hidden = false;

	virtual ~Hy3Node() = default;
	Hy3Node(const Hy3Node&) = delete;
	Hy3Node& operator=(const Hy3Node&) = delete;

	template<typename T> bool is() const { return dynamic_cast<const T*>(this) != nullptr; }
	template<typename T> T& as() { return dynamic_cast<T&>(*this); }
	template<typename T> const T& as() const { return dynamic_cast<const T&>(*this); }

	bool valid() const;
	Hy3NodeType type() const;
	bool is_target() const;
	bool is_group() const;
	Hy3GroupNode& as_group();
	SP<Layout::ITarget> as_target();
	PHLWINDOW as_window();

	bool operator==(const Hy3Node&) const;
	bool is_root();
	bool is_root_group();
	void assertNotRoot();
	Hy3RootNode* root();
	Hy3Layout* layout();

	static UP<Hy3Node> create(SP<Layout::ITarget> target);
	static UP<Hy3Node> create(Hy3GroupLayout group_layout);

	void focus(bool warp, Desktop::eFocusReason reason);
	void markFocused();
	Hy3Node& getFocusedNode(bool ignore_group_focus = false, bool stop_at_expanded = false);
	Hy3Node* findNeighbor(ShiftDirection);
	Hy3Node* getImmediateSibling(ShiftDirection);
	void resize(ShiftDirection, double, bool no_animation = false);
	bool isIndirectlyFocused();
	Hy3Node& getExpandActor();
	Hy3Node& getPlacementActor();

	void recalcSizePosRecursive(CBox offsets, bool no_animation = false);
	void updateTabBar(bool no_animation = false);
	void updateTabBarRecursive();
	void updateDecos();

	std::string getTitle();
	bool isUrgent();
	void setHidden(bool);

	Hy3Node* findNodeForTabGroup(Hy3TabGroup&);
	std::generator<Hy3Node&> ancestors();
	std::generator<Desktop::View::CWindow&> windows(bool visibleOnly = false);
	std::string debugNode();

	[[nodiscard]] Hy3Node* collapseParents(CollapsePolicy policy);
	UP<Hy3Node> extractAndMerge(
	    Hy3Node& child,
	    Hy3Node** out_parent = nullptr,
	    CollapsePolicy policy = CollapsePolicy::EmptySplits
	);

	void insertAndMerge(
	    std::list<UP<Hy3Node>>::iterator pos,
	    UP<Hy3Node> child,
	    CollapsePolicy policy = CollapsePolicy::EmptySplits
	);
	void insertAndMerge(UP<Hy3Node> child, CollapsePolicy policy = CollapsePolicy::EmptySplits);

	void wrap(Hy3GroupLayout, GroupEphemeralityOption, bool change = true);

protected:
	Hy3Node() = default;
};

struct Hy3TargetNode : Hy3Node {
	WP<Layout::ITarget> target;
};

struct Hy3GroupNode : Hy3Node {
	Hy3GroupLayout layout = Hy3GroupLayout::SplitH;
	Hy3GroupLayout previous_nontab_layout = Hy3GroupLayout::SplitH;
	std::list<UP<Hy3Node>> children;
	bool group_focused = true;
	Hy3Node* focused_child = nullptr; // non-owning observer, always valid while parent group lives
	ExpandFocusType expand_focused = ExpandFocusType::NotExpanded;
	Ephemeral ephemeral = Ephemeral::Off;
	bool locked = false;
	bool containment = false;
	Hy3TabGroupWrapper tab_bar;

	Hy3GroupNode(Hy3GroupLayout layout);
	~Hy3GroupNode() override = default;

	bool isSplit() const { return layout == Hy3GroupLayout::SplitH || layout == Hy3GroupLayout::SplitV; }
	bool isTab() const { return layout == Hy3GroupLayout::Tabbed; }

	bool hasChild(Hy3Node& child);
	void collapseExpansions();
	void setLayout(Hy3GroupLayout layout);
	void setEphemeral(GroupEphemeralityOption ephemeral);

	auto findChild(Hy3Node& child) -> std::list<UP<Hy3Node>>::iterator;
	void insertChild(std::list<UP<Hy3Node>>::iterator pos, UP<Hy3Node> child);
	void insertChild(UP<Hy3Node> child);
	UP<Hy3Node> extractChildRaw(std::list<UP<Hy3Node>>::iterator it);
	UP<Hy3Node> extractChildRaw(Hy3Node& child);
	UP<Hy3Node> replaceChild(std::list<UP<Hy3Node>>::iterator it, UP<Hy3Node> replacement);
	UP<Hy3Node> extractChild(Hy3Node& child);

	friend struct Hy3Node;
};

struct Hy3RootNode : Hy3GroupNode {
	Hy3Layout* algo = nullptr;
	Hy3RootNode(Hy3Layout* layout);
};
