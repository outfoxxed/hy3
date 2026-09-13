#pragma once

#include <hyprland/src/desktop/DesktopTypes.hpp>
class Hy3Layout;

enum class GroupEphemeralityOption {
	Ephemeral,
	Standard,
	ForceEphemeral,
};

#include <set>

#include <hyprland/src/layout/algorithm/TiledAlgorithm.hpp>
#include <hyprland/src/layout/algorithm/Algorithm.hpp>
#include <hyprland/src/layout/space/Space.hpp>
#include <hyprland/src/output/Monitor.hpp>
#include <hyprland/src/layout/target/Target.hpp>
#include <hyprland/src/layout/LayoutManager.hpp>
#include <hyprland/src/helpers/signal/Signal.hpp>
#include <hyprland/src/event/EventBus.hpp>

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

inline static Math::eDirection shiftToMathDirection(ShiftDirection direction) {
	switch (direction) {
	case ShiftDirection::Left: return Math::DIRECTION_LEFT;
	case ShiftDirection::Right: return Math::DIRECTION_RIGHT;
	case ShiftDirection::Up: return Math::DIRECTION_UP;
	case ShiftDirection::Down: return Math::DIRECTION_DOWN;
	}
	return Math::DIRECTION_DEFAULT;
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

class Hy3Layout: public Layout::ITiledAlgorithm {
public:
	Hy3Layout();
	~Hy3Layout() override;

	// ITiledAlgorithm / IModeAlgorithm overrides
	void newTarget(SP<Layout::ITarget> target) override;
	void movedTarget(SP<Layout::ITarget> target, std::optional<Vector2D> focalPoint = std::nullopt) override;
	void removeTarget(SP<Layout::ITarget> target) override;
	void resizeTarget(const Vector2D& delta, SP<Layout::ITarget> target, Layout::eRectCorner corner = Layout::CORNER_NONE) override;
	void recalculate(Layout::eRecalculateReason reason) override;
	void recalcGeometry(bool no_animation = false);
	void swapTargets(SP<Layout::ITarget> a, SP<Layout::ITarget> b) override;
	void moveTargetInDirection(SP<Layout::ITarget> t, Math::eDirection dir, bool silent) override;
	Config::ErrorResult layoutMsg(const std::string_view& sv) override;
	std::optional<Vector2D> predictSizeForNewTarget() override;
	SP<Layout::ITarget> getNextCandidate(SP<Layout::ITarget> old) override;

	// Hy3-specific public methods
	void insertNode(UP<Hy3Node> node, std::optional<Vector2D> focalPoint = std::nullopt);
	void onWindowFocusChange(PHLWINDOW window);
	void updateGroupBorderColors();

	void makeGroupOnWorkspace(
	    const Workspace::CHLWorkspace* workspace,
	    Hy3GroupLayout,
	    GroupEphemeralityOption,
	    bool toggle
	);
	void makeOppositeGroupOnWorkspace(const Workspace::CHLWorkspace* workspace, GroupEphemeralityOption);
	void changeGroupOnWorkspace(const Workspace::CHLWorkspace* workspace, Hy3GroupLayout);
	void untabGroupOnWorkspace(const Workspace::CHLWorkspace* workspace);
	void toggleTabGroupOnWorkspace(const Workspace::CHLWorkspace* workspace);
	void changeGroupToOppositeOnWorkspace(const Workspace::CHLWorkspace* workspace);
	void changeGroupEphemeralityOnWorkspace(const Workspace::CHLWorkspace* workspace, bool ephemeral);
	void makeGroupOn(Hy3Node&, Hy3GroupLayout, GroupEphemeralityOption);
	void makeOppositeGroupOn(Hy3Node&, GroupEphemeralityOption);
	void changeGroupOn(Hy3Node&, Hy3GroupLayout);
	void untabGroupOn(Hy3Node&);
	void toggleTabGroupOn(Hy3Node&);
	void changeGroupToOppositeOn(Hy3Node&);
	void changeGroupEphemeralityOn(Hy3Node&, bool ephemeral);
	void shiftNode(Hy3Node&, ShiftDirection, bool once, bool visible);
	void shiftWindow(const Workspace::CHLWorkspace* workspace, ShiftDirection, bool once, bool visible);
	void shiftFocus(const Workspace::CHLWorkspace* workspace, ShiftDirection, bool visible, bool warp);
	void toggleFocusLayer(const Workspace::CHLWorkspace* workspace, bool warp);
	bool shiftMonitor(Hy3Node&, ShiftDirection, bool follow);
	Hy3Node* focusMonitor(ShiftDirection);

	void warpCursor();
	void moveNodeToWorkspace(Workspace::CHLWorkspace* origin, std::string wsname, bool follow, bool warp);
	void changeFocus(const Workspace::CHLWorkspace* workspace, FocusShift);
	void focusTab(
	    const Workspace::CHLWorkspace* workspace,
	    TabFocus target,
	    TabFocusMousePriority,
	    bool wrap_scroll,
	    int index
	);
	void setNodeSwallow(const Workspace::CHLWorkspace* workspace, SetSwallowOption);
	void killFocusedNode(const Workspace::CHLWorkspace* workspace);
	void expand(const Workspace::CHLWorkspace* workspace, ExpandOption, ExpandFullscreenOption);
	void setTabLock(const Workspace::CHLWorkspace* workspace, TabLockMode);
	void equalize(const Workspace::CHLWorkspace* workspace, bool recursive = false);
	static void warpCursorToBox(const Vector2D& pos, const Vector2D& size);
	static void warpCursorWithFocus(const Vector2D& pos, bool force = false);
	static std::string debugNodes();

	bool shouldRenderSelected(const Desktop::View::CWindow*);
	PHLWINDOW findTiledWindowCandidate(const Desktop::View::CWindow* from);
	PHLWINDOW findFloatingWindowCandidate(const Desktop::View::CWindow* from);

	Hy3Node* getWorkspaceRootGroup(const Workspace::CHLWorkspace* workspace);
	Hy3Node* getWorkspaceFocusedNode(
	    const Workspace::CHLWorkspace* workspace,
	    bool ignore_group_focus = false,
	    bool stop_at_expanded = false
	);

	Hy3Node* getNodeFromWindow(const Desktop::View::CWindow*);
	Hy3Node* getNodeFromTarget(SP<Layout::ITarget> target);

	PHLWORKSPACE workspace();
	PHLMONITORREF monitor();

	UP<Hy3RootNode> root;

private:
	// if shift is true, shift the window in the given direction, returning
	// nullptr, if shift is false, return the window in the given direction or
	// nullptr. if once is true, only one group will be broken out of / into
	Hy3Node* shiftOrGetFocus(Hy3Node&, ShiftDirection, bool shift, bool once, bool visible);

	void updateAutotileWorkspaces();
	bool shouldAutotileWorkspace(const Workspace::CHLWorkspace* workspace);

	// Per-instance event listeners
	CHyprSignalListener m_windowActiveListener;
	CHyprSignalListener m_mouseButtonListener;

	struct {
		std::string raw_workspaces;
		bool workspace_blacklist;
		std::set<int> workspaces;
	} autotile;

	friend struct Hy3Node;
};
