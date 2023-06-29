#include <sstream>

#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>

#include "globals.hpp"
#include "Hy3Layout.hpp"
#include "SelectionHook.hpp"

std::unique_ptr<HOOK_CALLBACK_FN> renderHookPtr
    = std::make_unique<HOOK_CALLBACK_FN>(Hy3Layout::renderHook);
std::unique_ptr<HOOK_CALLBACK_FN> windowTitleHookPtr
    = std::make_unique<HOOK_CALLBACK_FN>(Hy3Layout::windowGroupUpdateRecursiveHook);
std::unique_ptr<HOOK_CALLBACK_FN> urgentHookPtr
    = std::make_unique<HOOK_CALLBACK_FN>(Hy3Layout::windowGroupUrgentHook);
std::unique_ptr<HOOK_CALLBACK_FN> tickHookPtr
    = std::make_unique<HOOK_CALLBACK_FN>(Hy3Layout::tickHook);

void Hy3Layout::onWindowCreatedTiling(CWindow* window) {
	if (window->m_bIsFloating) return;

	auto* existing = this->getNodeFromWindow(window);
	if (existing != nullptr) {
		Debug::log(
		    WARN,
		    "Attempted to add a window(%p) that is already tiled(as %p) to the "
		    "layout",
		    window,
		    existing
		);
		return;
	}

	auto* monitor = g_pCompositor->getMonitorFromID(window->m_iMonitorID);

	Hy3Node* opening_into;
	Hy3Node* opening_after = nullptr;

	if (monitor->activeWorkspace != -1) {
		auto* root = this->getWorkspaceRootGroup(monitor->activeWorkspace);

		if (root != nullptr) {
			opening_after = root->getFocusedNode();

			// opening_after->parent cannot be nullptr
			if (opening_after == root) {
				opening_after = root->data.as_group.focused_child;
			}
		}
	}

	if (opening_after == nullptr) {
		if (g_pCompositor->m_pLastWindow != nullptr && !g_pCompositor->m_pLastWindow->m_bIsFloating
		    && g_pCompositor->m_pLastWindow != window
		    && g_pCompositor->m_pLastWindow->m_iWorkspaceID == window->m_iWorkspaceID
		    && g_pCompositor->m_pLastWindow->m_bIsMapped)
		{
			opening_after = this->getNodeFromWindow(g_pCompositor->m_pLastWindow);
		} else {
			opening_after = this->getNodeFromWindow(
			    g_pCompositor->vectorToWindowTiled(g_pInputManager->getMouseCoordsInternal())
			);
		}
	}

	if (opening_after != nullptr && opening_after->workspace_id != window->m_iWorkspaceID) {
		opening_after = nullptr;
	}

	if (opening_after != nullptr) {
		opening_into = opening_after->parent;
	} else {
		if ((opening_into = this->getWorkspaceRootGroup(window->m_iWorkspaceID)) == nullptr) {
			this->nodes.push_back({
			    .data = Hy3GroupLayout::SplitH,
			    .position = monitor->vecPosition + monitor->vecReservedTopLeft,
			    .size = monitor->vecSize - monitor->vecReservedTopLeft - monitor->vecReservedBottomRight,
			    .workspace_id = window->m_iWorkspaceID,
			    .layout = this,
			});

			opening_into = &this->nodes.back();
		}
	}

	if (opening_into->data.type != Hy3NodeType::Group) {
		Debug::log(ERR, "opening_into node %p was not of type Group", opening_into);
		errorNotif();
		return;
	}

	if (opening_into->workspace_id != window->m_iWorkspaceID) {
		Debug::log(
		    WARN,
		    "opening_into node %p has workspace %d which does not match the "
		    "opening window (workspace %d)",
		    opening_into,
		    opening_into->workspace_id,
		    window->m_iWorkspaceID
		);
	}

	this->nodes.push_back({
	    .parent = opening_into,
	    .data = window,
	    .workspace_id = window->m_iWorkspaceID,
	    .layout = this,
	});

	auto& node = this->nodes.back();

	if (opening_after == nullptr) {
		opening_into->data.as_group.children.push_back(&node);
	} else {
		auto& children = opening_into->data.as_group.children;
		auto iter = std::find(children.begin(), children.end(), opening_after);
		auto iter2 = std::next(iter);
		children.insert(iter2, &node);
	}

	Debug::log(
	    LOG,
	    "opened new window %p(node: %p) on window %p in %p",
	    window,
	    &node,
	    opening_after,
	    opening_into
	);

	node.markFocused();
	opening_into->recalcSizePosRecursive();
	Debug::log(
	    LOG,
	    "opening_into (%p) contains new child (%p)? %d",
	    opening_into,
	    &node,
	    opening_into->data.as_group.hasChild(&node)
	);
}

void Hy3Layout::onWindowRemovedTiling(CWindow* window) {
	auto* node = this->getNodeFromWindow(window);
	Debug::log(LOG, "remove tiling %p (window %p)", node, window);

	if (node == nullptr) {
		Debug::log(ERR, "onWindowRemovedTiling node null?");
		return;
	}

	window->m_sSpecialRenderData.rounding = true;
	window->m_sSpecialRenderData.border = true;
	window->m_sSpecialRenderData.decorate = true;

	if (window->m_bIsFullscreen) {
		g_pCompositor->setWindowFullscreen(window, false, FULLSCREEN_FULL);
	}

	auto* parent = node->removeFromParentRecursive();
	this->nodes.remove(*node);

	if (parent != nullptr) {
		parent->recalcSizePosRecursive();

		if (parent->data.as_group.children.size() == 1
		    && parent->data.as_group.children.front()->data.type == Hy3NodeType::Group)
		{
			auto* target_parent = parent;
			while (target_parent != nullptr && Hy3Node::swallowGroups(target_parent)) {
				target_parent = target_parent->parent;
			}

			if (target_parent != parent && target_parent != nullptr)
				target_parent->recalcSizePosRecursive();
		}
	}
}

void Hy3Layout::onWindowFocusChange(CWindow* window) {
	Debug::log(LOG, "Switched windows to %p", window);
	auto* node = this->getNodeFromWindow(window);
	if (node == nullptr) return;

	node->markFocused();
	while (node->parent != nullptr) node = node->parent;
	node->recalcSizePosRecursive();
}

bool Hy3Layout::isWindowTiled(CWindow* window) {
	return this->getNodeFromWindow(window) != nullptr;
}

void Hy3Layout::recalculateMonitor(const int& monitor_id) {
	Debug::log(LOG, "Recalculate monitor %d", monitor_id);
	const auto monitor = g_pCompositor->getMonitorFromID(monitor_id);
	if (monitor == nullptr) return;

	g_pHyprRenderer->damageMonitor(monitor);

	const auto workspace = g_pCompositor->getWorkspaceByID(monitor->activeWorkspace);
	if (workspace == nullptr) return;

	if (monitor->specialWorkspaceID) {
		const auto top_node = this->getWorkspaceRootGroup(monitor->specialWorkspaceID);

		if (top_node != nullptr) {
			top_node->position = monitor->vecPosition + monitor->vecReservedTopLeft;
			top_node->size
			    = monitor->vecSize - monitor->vecReservedTopLeft - monitor->vecReservedBottomRight;
			top_node->recalcSizePosRecursive();
		}
	}

	if (workspace->m_bHasFullscreenWindow) {
		const auto window = g_pCompositor->getFullscreenWindowOnWorkspace(workspace->m_iID);

		if (workspace->m_efFullscreenMode == FULLSCREEN_FULL) {
			window->m_vRealPosition = monitor->vecPosition;
			window->m_vRealSize = monitor->vecSize;
		} else {
			// Vaxry's hack from below, but again

			Hy3Node fakeNode = {
			    .data = window,
			    .position = monitor->vecPosition + monitor->vecReservedTopLeft,
			    .size = monitor->vecSize - monitor->vecReservedTopLeft - monitor->vecReservedBottomRight,
			    .workspace_id = window->m_iWorkspaceID,
			};

			this->applyNodeDataToWindow(&fakeNode);
		}
	} else {
		const auto top_node = this->getWorkspaceRootGroup(monitor->activeWorkspace);

		if (top_node != nullptr) {
			top_node->position = monitor->vecPosition + monitor->vecReservedTopLeft;
			top_node->size
			    = monitor->vecSize - monitor->vecReservedTopLeft - monitor->vecReservedBottomRight;
			top_node->recalcSizePosRecursive();
		}
	}
}

void Hy3Layout::recalculateWindow(CWindow* window) {
	auto* node = this->getNodeFromWindow(window);
	if (node == nullptr) return;
	node->recalcSizePosRecursive();
}

void Hy3Layout::onBeginDragWindow() {
	this->drag_flags.started = false;
	IHyprLayout::onBeginDragWindow();
}

void Hy3Layout::resizeActiveWindow(const Vector2D& delta, CWindow* pWindow) {
	auto window = pWindow ? pWindow : g_pCompositor->m_pLastWindow;
	if (!g_pCompositor->windowValidMapped(window)) return;

	auto* node = this->getNodeFromWindow(window);
	if (node == nullptr) return;

	if (!this->drag_flags.started) {
		if (g_pInputManager->currentlyDraggedWindow == window) {
			auto mouse = g_pInputManager->getMouseCoordsInternal();
			auto mouse_offset = mouse - window->m_vPosition;

			this->drag_flags = {
			    .started = true,
			    .xExtent = mouse_offset.x > window->m_vSize.x / 2,
			    .yExtent = mouse_offset.y > window->m_vSize.y / 2,
			};

			Debug::log(
			    LOG,
			    "Positive offsets - x: %d, y: %d",
			    this->drag_flags.xExtent,
			    this->drag_flags.yExtent
			);
		} else {
			this->drag_flags = {
			    .started = false,
			    .xExtent = delta.x > 0,
			    .yExtent = delta.y > 0,
			};
		}
	}

	const auto animate
	    = &g_pConfigManager->getConfigValuePtr("misc:animate_manual_resizes")->intValue;

	auto monitor = g_pCompositor->getMonitorFromID(window->m_iMonitorID);

	const bool display_left
	    = STICKS(node->position.x, monitor->vecPosition.x + monitor->vecReservedTopLeft.x);
	const bool display_right = STICKS(
	    node->position.x + node->size.x,
	    monitor->vecPosition.x + monitor->vecSize.x - monitor->vecReservedBottomRight.x
	);
	const bool display_top
	    = STICKS(node->position.y, monitor->vecPosition.y + monitor->vecReservedTopLeft.y);
	const bool display_bottom = STICKS(
	    node->position.y + node->size.y,
	    monitor->vecPosition.y + monitor->vecSize.y - monitor->vecReservedBottomRight.y
	);

	Vector2D allowed_movement = delta;
	if (display_left && display_right) allowed_movement.x = 0;
	if (display_top && display_bottom) allowed_movement.y = 0;

	auto* inner_node = node;

	// break into parent groups when encountering a corner we're dragging in or a
	// tab group
	while (inner_node->parent != nullptr) {
		auto& group = inner_node->parent->data.as_group;

		switch (group.layout) {
		case Hy3GroupLayout::Tabbed:
			// treat tabbed layouts as if they dont exist during resizing
			goto cont;
		case Hy3GroupLayout::SplitH:
			if ((this->drag_flags.xExtent && group.children.back() == inner_node)
			    || (!this->drag_flags.xExtent && group.children.front() == inner_node))
			{
				goto cont;
			}
			break;
		case Hy3GroupLayout::SplitV:
			if ((this->drag_flags.yExtent && group.children.back() == inner_node)
			    || (!this->drag_flags.yExtent && group.children.front() == inner_node))
			{
				goto cont;
			}
			break;
		}

		break;
	cont:
		inner_node = inner_node->parent;
	}

	auto* inner_parent = inner_node->parent;
	if (inner_parent == nullptr) return;

	auto* outer_node = inner_node;

	// break into parent groups when encountering a corner we're dragging in, a
	// tab group, or a layout matching the inner_parent.
	while (outer_node->parent != nullptr) {
		auto& group = outer_node->parent->data.as_group;

		// break out of all layouts that match the orientation of the inner_parent
		if (group.layout == inner_parent->data.as_group.layout) goto cont2;

		switch (group.layout) {
		case Hy3GroupLayout::Tabbed:
			// treat tabbed layouts as if they dont exist during resizing
			goto cont2;
		case Hy3GroupLayout::SplitH:
			if ((this->drag_flags.xExtent && group.children.back() == outer_node)
			    || (!this->drag_flags.xExtent && group.children.front() == outer_node))
			{
				goto cont2;
			}
			break;
		case Hy3GroupLayout::SplitV:
			if ((this->drag_flags.yExtent && group.children.back() == outer_node)
			    || (!this->drag_flags.yExtent && group.children.front() == outer_node))
			{
				goto cont2;
			}
			break;
		}

		break;
	cont2:
		outer_node = outer_node->parent;
	}

	Debug::log(LOG, "resizeActive - inner_node: %p, outer_node: %p", inner_node, outer_node);

	auto& inner_group = inner_parent->data.as_group;
	// adjust the inner node
	switch (inner_group.layout) {
	case Hy3GroupLayout::SplitH: {
		auto ratio_mod
		    = allowed_movement.x * (float) inner_group.children.size() / inner_parent->size.x;

		auto iter = std::find(inner_group.children.begin(), inner_group.children.end(), inner_node);

		if (this->drag_flags.xExtent) {
			if (inner_node == inner_group.children.back()) break;
			iter = std::next(iter);
		} else {
			if (inner_node == inner_group.children.front()) break;
			iter = std::prev(iter);
			ratio_mod = -ratio_mod;
		}

		auto* neighbor = *iter;

		inner_node->size_ratio += ratio_mod;
		neighbor->size_ratio -= ratio_mod;
	} break;
	case Hy3GroupLayout::SplitV: {
		auto ratio_mod = allowed_movement.y * (float) inner_parent->data.as_group.children.size()
		               / inner_parent->size.y;

		auto iter = std::find(inner_group.children.begin(), inner_group.children.end(), inner_node);

		if (this->drag_flags.yExtent) {
			if (inner_node == inner_group.children.back()) break;
			iter = std::next(iter);
		} else {
			if (inner_node == inner_group.children.front()) break;
			iter = std::prev(iter);
			ratio_mod = -ratio_mod;
		}

		auto* neighbor = *iter;

		inner_node->size_ratio += ratio_mod;
		neighbor->size_ratio -= ratio_mod;
	} break;
	case Hy3GroupLayout::Tabbed: break;
	}

	inner_parent->recalcSizePosRecursive(*animate == 0);

	if (outer_node != nullptr && outer_node->parent != nullptr) {
		auto* outer_parent = outer_node->parent;
		auto& outer_group = outer_parent->data.as_group;
		// adjust the outer node
		switch (outer_group.layout) {
		case Hy3GroupLayout::SplitH: {
			auto ratio_mod
			    = allowed_movement.x * (float) outer_group.children.size() / outer_parent->size.x;

			auto iter = std::find(outer_group.children.begin(), outer_group.children.end(), outer_node);

			if (this->drag_flags.xExtent) {
				if (outer_node == inner_group.children.back()) break;
				iter = std::next(iter);
			} else {
				if (outer_node == inner_group.children.front()) break;
				iter = std::prev(iter);
				ratio_mod = -ratio_mod;
			}

			auto* neighbor = *iter;

			outer_node->size_ratio += ratio_mod;
			neighbor->size_ratio -= ratio_mod;
		} break;
		case Hy3GroupLayout::SplitV: {
			auto ratio_mod = allowed_movement.y * (float) outer_parent->data.as_group.children.size()
			               / outer_parent->size.y;

			auto iter = std::find(outer_group.children.begin(), outer_group.children.end(), outer_node);

			if (this->drag_flags.yExtent) {
				if (outer_node == outer_group.children.back()) break;
				iter = std::next(iter);
			} else {
				if (outer_node == outer_group.children.front()) break;
				iter = std::prev(iter);
				ratio_mod = -ratio_mod;
			}

			auto* neighbor = *iter;

			outer_node->size_ratio += ratio_mod;
			neighbor->size_ratio -= ratio_mod;
		} break;
		case Hy3GroupLayout::Tabbed: break;
		}

		outer_parent->recalcSizePosRecursive(*animate == 0);
	}
}

void Hy3Layout::fullscreenRequestForWindow(
    CWindow* window,
    eFullscreenMode fullscreen_mode,
    bool on
) {
	if (!g_pCompositor->windowValidMapped(window)) return;
	if (on == window->m_bIsFullscreen || g_pCompositor->isWorkspaceSpecial(window->m_iWorkspaceID))
		return;

	const auto monitor = g_pCompositor->getMonitorFromID(window->m_iMonitorID);
	const auto workspace = g_pCompositor->getWorkspaceByID(window->m_iWorkspaceID);
	if (workspace->m_bHasFullscreenWindow && on) return;

	window->m_bIsFullscreen = on;
	workspace->m_bHasFullscreenWindow = !workspace->m_bHasFullscreenWindow;

	if (!window->m_bIsFullscreen) {
		auto* node = this->getNodeFromWindow(window);

		if (node) {
			// restore node positioning if tiled
			this->applyNodeDataToWindow(node);
		} else {
			// restore floating position if not
			window->m_vRealPosition = window->m_vLastFloatingPosition;
			window->m_vRealSize = window->m_vLastFloatingSize;

			window->m_sSpecialRenderData.rounding = true;
			window->m_sSpecialRenderData.border = true;
			window->m_sSpecialRenderData.decorate = true;
		}
	} else {
		workspace->m_efFullscreenMode = fullscreen_mode;

		// save position and size if floating
		if (window->m_bIsFloating) {
			window->m_vLastFloatingPosition = window->m_vRealPosition.goalv();
			window->m_vPosition = window->m_vRealPosition.goalv();
			window->m_vLastFloatingSize = window->m_vRealSize.goalv();
			window->m_vSize = window->m_vRealSize.goalv();
		}

		if (fullscreen_mode == FULLSCREEN_FULL) {
			Debug::log(LOG, "fullscreen");
			window->m_vRealPosition = monitor->vecPosition;
			window->m_vRealSize = monitor->vecSize;
		} else {
			Debug::log(LOG, "vaxry hack");
			// Copy of vaxry's massive hack

			Hy3Node fakeNode = {
			    .data = window,
			    .position = monitor->vecPosition + monitor->vecReservedTopLeft,
			    .size = monitor->vecSize - monitor->vecReservedTopLeft - monitor->vecReservedBottomRight,
			    .workspace_id = window->m_iWorkspaceID,
			};

			this->applyNodeDataToWindow(&fakeNode);
		}
	}

	g_pCompositor->updateWindowAnimatedDecorationValues(window);
	g_pXWaylandManager->setWindowSize(window, window->m_vRealSize.goalv());
	g_pCompositor->moveWindowToTop(window);
	this->recalculateMonitor(monitor->ID);
}

std::any Hy3Layout::layoutMessage(SLayoutMessageHeader header, std::string content) {
	if (content == "togglesplit") {
		auto* node = this->getNodeFromWindow(header.pWindow);
		if (node != nullptr && node->parent != nullptr) {
			auto& layout = node->parent->data.as_group.layout;

			switch (layout) {
			case Hy3GroupLayout::SplitH:
				layout = Hy3GroupLayout::SplitV;
				node->parent->recalcSizePosRecursive();
				break;
			case Hy3GroupLayout::SplitV:
				layout = Hy3GroupLayout::SplitH;
				node->parent->recalcSizePosRecursive();
				break;
			case Hy3GroupLayout::Tabbed: break;
			}
		}
	}

	return "";
}

SWindowRenderLayoutHints Hy3Layout::requestRenderHints(CWindow* window) { return {}; }

void Hy3Layout::switchWindows(CWindow* pWindowA, CWindow* pWindowB) {
	// todo
}

void Hy3Layout::alterSplitRatio(CWindow* pWindow, float delta, bool exact) {
	// todo
}

std::string Hy3Layout::getLayoutName() { return "hy3"; }

CWindow* Hy3Layout::getNextWindowCandidate(CWindow* window) {
	auto* node = this->getWorkspaceFocusedNode(window->m_iWorkspaceID);
	if (node == nullptr) return nullptr;

	switch (node->data.type) {
	case Hy3NodeType::Window: return node->data.as_window;
	case Hy3NodeType::Group: return nullptr;
	}
}

void Hy3Layout::replaceWindowDataWith(CWindow* from, CWindow* to) {
	auto* node = this->getNodeFromWindow(from);
	if (node == nullptr) return;

	node->data.as_window = to;
	this->applyNodeDataToWindow(node);
}

void Hy3Layout::onEnable() {
	for (auto& window: g_pCompositor->m_vWindows) {
		if (window->isHidden() || !window->m_bIsMapped || window->m_bFadingOut || window->m_bIsFloating)
			continue;

		this->onWindowCreatedTiling(window.get());
	}

	HyprlandAPI::registerCallbackStatic(PHANDLE, "render", renderHookPtr.get());
	HyprlandAPI::registerCallbackStatic(PHANDLE, "windowTitle", windowTitleHookPtr.get());
	HyprlandAPI::registerCallbackStatic(PHANDLE, "urgent", urgentHookPtr.get());
	HyprlandAPI::registerCallbackStatic(PHANDLE, "tick", tickHookPtr.get());
	selection_hook::enable();
}

void Hy3Layout::onDisable() {
	HyprlandAPI::unregisterCallback(PHANDLE, renderHookPtr.get());
	HyprlandAPI::unregisterCallback(PHANDLE, windowTitleHookPtr.get());
	HyprlandAPI::unregisterCallback(PHANDLE, urgentHookPtr.get());
	HyprlandAPI::unregisterCallback(PHANDLE, tickHookPtr.get());
	selection_hook::disable();

	for (auto& node: this->nodes) {
		if (node.data.type == Hy3NodeType::Window) {
			node.data.as_window->setHidden(false);
		}
	}

	this->nodes.clear();
}

void Hy3Layout::makeGroupOnWorkspace(int workspace, Hy3GroupLayout layout) {
	auto* node = this->getWorkspaceFocusedNode(workspace);
	this->makeGroupOn(node, layout);
}

void Hy3Layout::makeOppositeGroupOnWorkspace(int workspace) {
	auto* node = this->getWorkspaceFocusedNode(workspace);
	this->makeOppositeGroupOn(node);
}

void Hy3Layout::makeGroupOn(Hy3Node* node, Hy3GroupLayout layout) {
	if (node == nullptr) return;

	if (node->parent != nullptr) {
		auto& group = node->parent->data.as_group;
		if (group.children.size() == 1) {
			group.layout = layout;
			node->parent->recalcSizePosRecursive();
			return;
		}
	}

	node->intoGroup(layout);
}

void Hy3Layout::makeOppositeGroupOn(Hy3Node* node) {
	if (node == nullptr) return;

	if (node->parent == nullptr) {
		node->intoGroup(Hy3GroupLayout::SplitH);
	} else {
		auto& group = node->parent->data.as_group;
		auto layout
		    = group.layout == Hy3GroupLayout::SplitH ? Hy3GroupLayout::SplitV : Hy3GroupLayout::SplitH;

		if (group.children.size() == 1) {
			group.layout = layout;
			node->parent->recalcSizePosRecursive();
		} else {
			node->intoGroup(layout);
		}
	}
}

void Hy3Layout::shiftWindow(int workspace, ShiftDirection direction, bool once) {
	auto* node = this->getWorkspaceFocusedNode(workspace);
	Debug::log(LOG, "ShiftWindow %p %d", node, direction);
	if (node == nullptr) return;

	if (once && node->parent != nullptr && node->parent->data.as_group.children.size() == 1) {
		if (node->parent->parent == nullptr) {
			node->parent->data.as_group.layout = Hy3GroupLayout::SplitH;
			node->parent->recalcSizePosRecursive();
		} else {
			auto* node2 = node->parent;
			Hy3Node::swapData(*node, *node2);
			node2->layout->nodes.remove(*node);
			node2->recalcSizePosRecursive();
		}
	} else {
		this->shiftOrGetFocus(*node, direction, true, once, false);
	}
}

void Hy3Layout::shiftFocus(int workspace, ShiftDirection direction, bool visible) {
	auto* node = this->getWorkspaceFocusedNode(workspace);
	Debug::log(LOG, "ShiftFocus %p %d", node, direction);
	if (node == nullptr) return;

	Hy3Node* target;
	if ((target = this->shiftOrGetFocus(*node, direction, false, false, visible))) {
		target->focus();
		while (target->parent != nullptr) target = target->parent;
		target->recalcSizePosRecursive();
	}
}

void Hy3Layout::changeFocus(int workspace, FocusShift shift) {
	auto* node = this->getWorkspaceFocusedNode(workspace);
	if (node == nullptr) return;

	switch (shift) {
	case FocusShift::Bottom: goto bottom;
	case FocusShift::Top:
		while (node->parent != nullptr) {
			node = node->parent;
		}

		node->focus();
		return;
	case FocusShift::Raise:
		if (node->parent == nullptr) goto bottom;
		else {
			node->parent->focus();
		}
		return;
	case FocusShift::Lower:
		if (node->data.type == Hy3NodeType::Group && node->data.as_group.focused_child != nullptr)
			node->data.as_group.focused_child->focus();
		return;
	case FocusShift::Tab:
		// make sure we go up at least one level
		if (node->parent != nullptr) node = node->parent;
		while (node->parent != nullptr) {
			if (node->data.as_group.layout == Hy3GroupLayout::Tabbed) {
				node->focus();
				return;
			}

			node = node->parent;
		}
		return;
	case FocusShift::TabNode:
		// make sure we go up at least one level
		if (node->parent != nullptr) node = node->parent;
		while (node->parent != nullptr) {
			if (node->parent->data.as_group.layout == Hy3GroupLayout::Tabbed) {
				node->focus();
				return;
			}

			node = node->parent;
		}
		return;
	}

bottom:
	while (node->data.type == Hy3NodeType::Group && node->data.as_group.focused_child != nullptr) {
		node = node->data.as_group.focused_child;
	}

	node->focus();
	return;
}

Hy3Node* findTabBarAt(Hy3Node& node, Vector2D pos, Hy3Node** focused_node) {
	// clang-format off
	static const auto* gaps_in = &HyprlandAPI::getConfigValue(PHANDLE, "general:gaps_in")->intValue;
	static const auto* gaps_out = &HyprlandAPI::getConfigValue(PHANDLE, "general:gaps_out")->intValue;
	static const auto* tab_bar_height = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:hy3:tabs:height")->intValue;
	static const auto* tab_bar_padding = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:hy3:tabs:padding")->intValue;
	// clang-format on

	auto inset = *tab_bar_height + *tab_bar_padding;

	if (node.parent == nullptr) {
		inset += *gaps_out;
	} else {
		inset += *gaps_in;
	}

	if (node.data.type == Hy3NodeType::Group) {
		if (node.hidden) return nullptr;
		// note: tab bar clicks ignore animations
		if (node.position.x > pos.x || node.position.y > pos.y || node.position.x + node.size.x < pos.x
		    || node.position.y + node.size.y < pos.y)
			return nullptr;

		if (node.data.as_group.layout == Hy3GroupLayout::Tabbed
		    && node.data.as_group.tab_bar != nullptr)
		{
			if (pos.y < node.position.y + inset) {
				auto& children = node.data.as_group.children;
				auto& tab_bar = *node.data.as_group.tab_bar;

				auto size = tab_bar.size.vec();
				auto x = pos.x - tab_bar.pos.vec().x;
				auto child_iter = children.begin();

				for (auto& tab: tab_bar.bar.entries) {
					if (child_iter == children.end()) break;

					if (x > tab.offset.fl() * size.x && x < (tab.offset.fl() + tab.width.fl()) * size.x) {
						*focused_node = *child_iter;
						return &node;
					}

					child_iter = std::next(child_iter);
				}
			}

			if (node.data.as_group.focused_child != nullptr) {
				return findTabBarAt(*node.data.as_group.focused_child, pos, focused_node);
			}
		} else {
			for (auto child: node.data.as_group.children) {
				if (findTabBarAt(*child, pos, focused_node)) return child;
			}
		}
	}

	return nullptr;
}

void Hy3Layout::focusTab(
    int workspace,
    TabFocus target,
    TabFocusMousePriority mouse,
    bool wrap_scroll,
    int index
) {
	auto* node = this->getWorkspaceRootGroup(workspace);
	if (node == nullptr) return;

	Hy3Node* tab_node = nullptr;
	Hy3Node* tab_focused_node;

	if (target == TabFocus::MouseLocation || mouse != TabFocusMousePriority::Ignore) {
		if (g_pCompositor->windowFloatingFromCursor() == nullptr) {
			auto mouse_pos = g_pInputManager->getMouseCoordsInternal();
			tab_node = findTabBarAt(*node, mouse_pos, &tab_focused_node);
			if (tab_node != nullptr) goto hastab;
		}

		if (target == TabFocus::MouseLocation || mouse == TabFocusMousePriority::Require) return;
	}

	if (tab_node == nullptr) {
		tab_node = this->getWorkspaceFocusedNode(workspace);
		if (tab_node == nullptr) return;

		while (tab_node != nullptr && tab_node->data.as_group.layout != Hy3GroupLayout::Tabbed
		       && tab_node->parent != nullptr)
			tab_node = tab_node->parent;

		if (tab_node == nullptr || tab_node->data.type != Hy3NodeType::Group
		    || tab_node->data.as_group.layout != Hy3GroupLayout::Tabbed)
			return;
	}

hastab:
	if (target != TabFocus::MouseLocation) {
		if (tab_node->data.as_group.focused_child == nullptr
		    || tab_node->data.as_group.children.size() < 2)
			return;

		auto& children = tab_node->data.as_group.children;
		if (target == TabFocus::Index) {
			int i = 1;

			for (auto* node: children) {
				if (i == index) {
					tab_focused_node = node;
					goto cont;
				}

				i++;
			}

			return;
		cont:;
		} else {
			auto node_iter
			    = std::find(children.begin(), children.end(), tab_node->data.as_group.focused_child);
			if (node_iter == children.end()) return;
			if (target == TabFocus::Left) {
				if (node_iter == children.begin()) {
					if (wrap_scroll) node_iter = std::prev(children.end());
					else return;
				} else node_iter = std::prev(node_iter);

				tab_focused_node = *node_iter;
			} else {
				if (node_iter == std::prev(children.end())) {
					if (wrap_scroll) node_iter = children.begin();
					else return;
				} else node_iter = std::next(node_iter);

				tab_focused_node = *node_iter;
			}
		}
	}

	auto* focus = tab_focused_node;
	while (focus->data.type == Hy3NodeType::Group && !focus->data.as_group.group_focused
	       && focus->data.as_group.focused_child != nullptr)
		focus = focus->data.as_group.focused_child;

	focus->focus();
	tab_node->recalcSizePosRecursive();
}

void Hy3Layout::killFocusedNode(int workspace) {
	if (g_pCompositor->m_pLastWindow != nullptr && g_pCompositor->m_pLastWindow->m_bIsFloating) {
		g_pCompositor->closeWindow(g_pCompositor->m_pLastWindow);
	} else {
		auto* node = this->getWorkspaceFocusedNode(workspace);
		if (node == nullptr) return;

		std::vector<CWindow*> windows;
		node->appendAllWindows(windows);

		for (auto* window: windows) {
			window->setHidden(false);
			g_pCompositor->closeWindow(window);
		}
	}
}

bool Hy3Layout::shouldRenderSelected(CWindow* window) {
	if (window == nullptr) return false;
	auto* root = this->getWorkspaceRootGroup(window->m_iWorkspaceID);
	if (root == nullptr || root->data.as_group.focused_child == nullptr) return false;
	auto* focused = root->getFocusedNode();
	if (focused == nullptr
	    || (focused->data.type == Hy3NodeType::Window
	        && focused->data.as_window != g_pCompositor->m_pLastWindow))
		return false;

	switch (focused->data.type) {
	case Hy3NodeType::Window: return focused->data.as_window == window;
	case Hy3NodeType::Group:
		auto* node = this->getNodeFromWindow(window);
		if (node == nullptr) return false;
		return focused->data.as_group.hasChild(node);
	}
}

Hy3Node* Hy3Layout::getWorkspaceRootGroup(const int& workspace) {
	for (auto& node: this->nodes) {
		if (node.workspace_id == workspace && node.parent == nullptr
		    && node.data.type == Hy3NodeType::Group)
		{
			return &node;
		}
	}

	return nullptr;
}

Hy3Node* Hy3Layout::getWorkspaceFocusedNode(const int& workspace) {
	auto* rootNode = this->getWorkspaceRootGroup(workspace);
	if (rootNode == nullptr) return nullptr;
	return rootNode->getFocusedNode();
}

void Hy3Layout::renderHook(void*, std::any data) {
	static bool rendering_normally = false;
	static std::vector<Hy3TabGroup*> rendered_groups;

	auto render_stage = std::any_cast<eRenderStage>(data);

	switch (render_stage) {
	case RENDER_PRE_WINDOWS:
		rendering_normally = true;
		rendered_groups.clear();
		break;
	case RENDER_POST_WINDOW:
		if (!rendering_normally) break;

		for (auto& entry: g_Hy3Layout->tab_groups) {
			if (!entry.hidden && entry.target_window == g_pHyprOpenGL->m_pCurrentWindow
			    && std::find(rendered_groups.begin(), rendered_groups.end(), &entry)
			           == rendered_groups.end())
			{
				entry.renderTabBar();
				rendered_groups.push_back(&entry);
			}
		}

		break;
	case RENDER_POST_WINDOWS:
		rendering_normally = false;

		for (auto& entry: g_Hy3Layout->tab_groups) {
			if (!entry.hidden
			    && entry.target_window->m_iMonitorID == g_pHyprOpenGL->m_RenderData.pMonitor->ID
			    && std::find(rendered_groups.begin(), rendered_groups.end(), &entry)
			           == rendered_groups.end())
			{
				entry.renderTabBar();
			}
		}

		break;
	default: break;
	}
}

void Hy3Layout::windowGroupUrgentHook(void* p, std::any data) {
	CWindow* window = std::any_cast<CWindow*>(data);
	if (window == nullptr) return;
	window->m_bIsUrgent = true;
	Hy3Layout::windowGroupUpdateRecursiveHook(p, data);
}

void Hy3Layout::windowGroupUpdateRecursiveHook(void*, std::any data) {
	CWindow* window = std::any_cast<CWindow*>(data);
	if (window == nullptr) return;
	auto* node = g_Hy3Layout->getNodeFromWindow(window);

	// it is UB for `this` to be null
	if (node == nullptr) return;
	node->updateTabBarRecursive();
}

void Hy3Layout::tickHook(void*, std::any) {
	auto& tab_groups = g_Hy3Layout->tab_groups;
	auto entry = tab_groups.begin();
	while (entry != tab_groups.end()) {
		entry->tick();
		if (entry->bar.destroy) tab_groups.erase(entry++);
		else entry = std::next(entry);
	}
}

Hy3Node* Hy3Layout::getNodeFromWindow(CWindow* window) {
	for (auto& node: this->nodes) {
		if (node.data.type == Hy3NodeType::Window && node.data.as_window == window) {
			return &node;
		}
	}

	return nullptr;
}

void Hy3Layout::applyNodeDataToWindow(Hy3Node* node, bool no_animation) {
	if (node->data.type != Hy3NodeType::Window) return;
	CWindow* window = node->data.as_window;

	CMonitor* monitor = nullptr;

	if (g_pCompositor->isWorkspaceSpecial(node->workspace_id)) {
		for (auto& m: g_pCompositor->m_vMonitors) {
			if (m->specialWorkspaceID == node->workspace_id) {
				monitor = m.get();
				break;
			}
		}
	} else {
		monitor = g_pCompositor->getMonitorFromID(
		    g_pCompositor->getWorkspaceByID(node->workspace_id)->m_iMonitorID
		);
	}

	if (monitor == nullptr) {
		Debug::log(ERR, "Orphaned Node %x (workspace ID: %i)!!", node, node->workspace_id);
		errorNotif();
		return;
	}

	// clang-format off
	static const auto* border_size = &HyprlandAPI::getConfigValue(PHANDLE, "general:border_size")->intValue;
	static const auto* gaps_in = &HyprlandAPI::getConfigValue(PHANDLE, "general:gaps_in")->intValue;
	static const auto* single_window_no_gaps = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:hy3:no_gaps_when_only")->intValue;
	// clang-format on

	if (!g_pCompositor->windowExists(window) || !window->m_bIsMapped) {
		Debug::log(ERR, "Node %p holding invalid window %p!!", node, window);
		errorNotif();
		this->onWindowRemovedTiling(window);
		return;
	}

	window->m_vSize = node->size;
	window->m_vPosition = node->position;

	auto calcPos = window->m_vPosition + Vector2D(*border_size, *border_size);
	auto calcSize = window->m_vSize - Vector2D(2 * *border_size, 2 * *border_size);

	auto root_node = this->getWorkspaceRootGroup(window->m_iWorkspaceID);
	auto only_node = root_node->data.as_group.children.size() == 1
	              && root_node->data.as_group.children.front()->data.type == Hy3NodeType::Window;

	if (!g_pCompositor->isWorkspaceSpecial(window->m_iWorkspaceID)
	    && ((*single_window_no_gaps && only_node)
	        || (window->m_bIsFullscreen
	            && g_pCompositor->getWorkspaceByID(window->m_iWorkspaceID)->m_efFullscreenMode
	                   == FULLSCREEN_FULL)))
	{
		window->m_vRealPosition = window->m_vPosition;
		window->m_vRealSize = window->m_vSize;

		window->updateWindowDecos();

		window->m_sSpecialRenderData.rounding = false;
		window->m_sSpecialRenderData.border = false;
		window->m_sSpecialRenderData.decorate = false;
	} else {
		window->m_sSpecialRenderData.rounding = true;
		window->m_sSpecialRenderData.border = true;
		window->m_sSpecialRenderData.decorate = true;

		auto gaps_offset_topleft = Vector2D(*gaps_in, *gaps_in) + node->gap_pos_offset;
		auto gaps_offset_bottomright = Vector2D(*gaps_in * 2, *gaps_in * 2) + node->gap_size_offset;

		calcPos = calcPos + gaps_offset_topleft;
		calcSize = calcSize - gaps_offset_bottomright;

		const auto reserved_area = window->getFullWindowReservedArea();
		calcPos = calcPos + reserved_area.topLeft;
		calcSize = calcSize - (reserved_area.topLeft - reserved_area.bottomRight);

		window->m_vRealPosition = calcPos;
		window->m_vRealSize = calcSize;
		Debug::log(LOG, "Set size (%f %f)", calcSize.x, calcSize.y);

		g_pXWaylandManager->setWindowSize(window, calcSize);

		if (no_animation) {
			g_pHyprRenderer->damageWindow(window);

			window->m_vRealPosition.warp();
			window->m_vRealSize.warp();

			g_pHyprRenderer->damageWindow(window);
		}

		window->updateWindowDecos();
	}
}

bool shiftIsForward(ShiftDirection direction) {
	return direction == ShiftDirection::Right || direction == ShiftDirection::Down;
}

bool shiftIsVertical(ShiftDirection direction) {
	return direction == ShiftDirection::Up || direction == ShiftDirection::Down;
}

bool shiftMatchesLayout(Hy3GroupLayout layout, ShiftDirection direction) {
	return (layout == Hy3GroupLayout::SplitV && shiftIsVertical(direction))
	    || (layout != Hy3GroupLayout::SplitV && !shiftIsVertical(direction));
}

Hy3Node* Hy3Layout::shiftOrGetFocus(
    Hy3Node& node,
    ShiftDirection direction,
    bool shift,
    bool once,
    bool visible
) {
	auto* break_origin = &node;
	auto* break_parent = break_origin->parent;

	auto has_broken_once = false;

	// break parents until we hit a container oriented the same way as the shift
	// direction
	while (true) {
		if (break_parent == nullptr) return nullptr;

		auto& group = break_parent->data.as_group; // must be a group in order to be a parent

		if (shiftMatchesLayout(group.layout, direction)
		    && (!visible || group.layout != Hy3GroupLayout::Tabbed))
		{
			// group has the correct orientation

			if (once && shift && has_broken_once) break;
			if (break_origin != &node) has_broken_once = true;

			// if this movement would break out of the group, continue the break loop
			// (do not enter this if) otherwise break.
			if ((has_broken_once && once && shift)
			    || !(
			        (!shiftIsForward(direction) && group.children.front() == break_origin)
			        || (shiftIsForward(direction) && group.children.back() == break_origin)
			    ))
				break;
		}

		if (break_parent->parent == nullptr) {
			if (!shift) return nullptr;

			// if we haven't gone up any levels and the group is in the same direction
			// there's no reason to wrap the root group.
			if (group.layout != Hy3GroupLayout::Tabbed && shiftMatchesLayout(group.layout, direction))
				break;

			if (group.layout != Hy3GroupLayout::Tabbed && group.children.size() == 2
			    && std::find(group.children.begin(), group.children.end(), &node) != group.children.end())
			{
				group.layout = shiftIsVertical(direction) ? Hy3GroupLayout::SplitV : Hy3GroupLayout::SplitH;
			} else {
				// wrap the root group in another group
				this->nodes.push_back({
				    .parent = break_parent,
				    .data = shiftIsVertical(direction) ? Hy3GroupLayout::SplitV : Hy3GroupLayout::SplitH,
				    .position = break_parent->position,
				    .size = break_parent->size,
				    .workspace_id = break_parent->workspace_id,
				    .layout = this,
				});

				auto* newChild = &this->nodes.back();
				Hy3Node::swapData(*break_parent, *newChild);
				break_parent->data.as_group.children.push_back(newChild);
				break_parent->data.as_group.group_focused = false;
				break_parent->data.as_group.focused_child = newChild;
				break_origin = newChild;
			}

			break;
		} else {
			break_origin = break_parent;
			break_parent = break_origin->parent;
		}
	}

	auto& parent_group = break_parent->data.as_group;
	Hy3Node* target_group = break_parent;
	std::list<Hy3Node*>::iterator insert;

	if (break_origin == parent_group.children.front() && !shiftIsForward(direction)) {
		if (!shift) return nullptr;
		insert = parent_group.children.begin();
	} else if (break_origin == parent_group.children.back() && shiftIsForward(direction)) {
		if (!shift) return nullptr;
		insert = parent_group.children.end();
	} else {
		auto& group_data = target_group->data.as_group;

		auto iter = std::find(group_data.children.begin(), group_data.children.end(), break_origin);
		if (shiftIsForward(direction)) iter = std::next(iter);
		else iter = std::prev(iter);

		if ((*iter)->data.type == Hy3NodeType::Window || (shift && once && has_broken_once)) {
			if (shift) {
				if (target_group == node.parent) {
					if (shiftIsForward(direction)) insert = std::next(iter);
					else insert = iter;
				} else {
					if (shiftIsForward(direction)) insert = iter;
					else insert = std::next(iter);
				}
			} else return *iter;
		} else {
			// break into neighboring groups until we hit a window
			while (true) {
				target_group = *iter;
				auto& group_data = target_group->data.as_group;

				if (group_data.children.empty()) return nullptr; // in theory this would never happen

				bool shift_after = false;

				if (!shift && group_data.layout == Hy3GroupLayout::Tabbed
				    && group_data.focused_child != nullptr)
				{
					iter = std::find(
					    group_data.children.begin(),
					    group_data.children.end(),
					    group_data.focused_child
					);
				} else if (shiftMatchesLayout(group_data.layout, direction)) {
					// if the group has the same orientation as movement pick the
					// last/first child based on movement direction
					if (shiftIsForward(direction)) iter = group_data.children.begin();
					else {
						iter = std::prev(group_data.children.end());
						shift_after = true;
					}
				} else {
					if (group_data.focused_child != nullptr) {
						iter = std::find(
						    group_data.children.begin(),
						    group_data.children.end(),
						    group_data.focused_child
						);
						shift_after = true;
					} else {
						iter = group_data.children.begin();
					}
				}

				if (shift && once) {
					if (shift_after) insert = std::next(iter);
					else insert = iter;
					break;
				}

				if ((*iter)->data.type == Hy3NodeType::Window) {
					if (shift) {
						if (shift_after) insert = std::next(iter);
						else insert = iter;
						break;
					} else {
						return *iter;
					}
				}
			}
		}
	}

	auto& group_data = target_group->data.as_group;

	if (target_group == node.parent) {
		// nullptr is used as a signal value instead of removing it first to avoid
		// iterator invalidation.
		auto iter = std::find(group_data.children.begin(), group_data.children.end(), &node);
		*iter = nullptr;
		target_group->data.as_group.children.insert(insert, &node);
		target_group->data.as_group.children.remove(nullptr);
		target_group->recalcSizePosRecursive();
	} else {
		target_group->data.as_group.children.insert(insert, &node);

		// must happen AFTER `insert` is used
		auto* old_parent = node.removeFromParentRecursive();
		node.parent = target_group;
		node.size_ratio = 1.0;

		if (old_parent != nullptr) old_parent->recalcSizePosRecursive();
		target_group->recalcSizePosRecursive();

		auto* target_parent = target_group->parent;
		while (target_parent != nullptr && Hy3Node::swallowGroups(target_parent)) {
			target_parent = target_parent->parent;
		}

		node.focus();

		if (target_parent != target_group && target_parent != nullptr)
			target_parent->recalcSizePosRecursive();
	}

	return nullptr;
}
