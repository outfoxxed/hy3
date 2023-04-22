#include "globals.hpp"
#include "Hy3Layout.hpp"

#include <src/Compositor.hpp>

Hy3GroupData::Hy3GroupData(Hy3GroupLayout layout): layout(layout) {}

Hy3NodeData::Hy3NodeData(): Hy3NodeData((CWindow*)nullptr) {}

Hy3NodeData::Hy3NodeData(CWindow *window): type(Hy3NodeData::Window) {
	this->as_window = window;
}

Hy3NodeData::Hy3NodeData(Hy3GroupData group): type(Hy3NodeData::Group) {
	new(&this->as_group) Hy3GroupData(std::move(group));
}

Hy3NodeData::Hy3NodeData(Hy3GroupLayout layout): Hy3NodeData(Hy3GroupData(layout)) {}

Hy3NodeData::~Hy3NodeData() {
	switch (this->type) {
	case Hy3NodeData::Window:
		break;
	case Hy3NodeData::Group:
		this->as_group.~Hy3GroupData();

		// who ever thought calling the dtor after a move was a good idea?
		this->type = Hy3NodeData::Window;
		break;
	}
}

Hy3NodeData::Hy3NodeData(const Hy3NodeData& from): type(from.type) {
	Debug::log(LOG, "Copy CTor type matches? %d is group? %d", this->type == from.type, this->type == Hy3NodeData::Group);
	switch (from.type) {
	case Hy3NodeData::Window:
		this->as_window = from.as_window;
		break;
	case Hy3NodeData::Group:
		new(&this->as_group) Hy3GroupData(from.as_group);
		break;
	}
}

Hy3NodeData::Hy3NodeData(Hy3NodeData&& from): type(from.type) {
	Debug::log(LOG, "Move CTor type matches? %d is group? %d", this->type == from.type, this->type == Hy3NodeData::Group);
	switch (from.type) {
	case Hy3NodeData::Window:
		this->as_window = from.as_window;
		break;
	case Hy3NodeData::Group:
		new(&this->as_group) Hy3GroupData(std::move(from.as_group));
		break;
	}
}

Hy3NodeData& Hy3NodeData::operator=(const Hy3NodeData& from) {
	Debug::log(LOG, "operator= type matches? %d is group? %d", this->type == from.type, this->type == Hy3NodeData::Group);
	if (this->type == Hy3NodeData::Group) {
		this->as_group.~Hy3GroupData();
	}

	this->type = from.type;

	switch (this->type) {
	case Hy3NodeData::Window:
		this->as_window = from.as_window;
		break;
	case Hy3NodeData::Group:
		new(&this->as_group) Hy3GroupData(from.as_group);
		break;
	}

	return *this;
}

Hy3NodeData& Hy3NodeData::operator=(CWindow* window) {
	*this = Hy3NodeData(window);

	return *this;
}

Hy3NodeData& Hy3NodeData::operator=(Hy3GroupLayout layout) {
	*this = Hy3NodeData(layout);

	return *this;
}

bool Hy3NodeData::operator==(const Hy3NodeData& rhs) const {
	if (this->type != rhs.type) return false;
	switch (this->type) {
	case Hy3NodeData::Window:
		return this->as_window == rhs.as_window;
	case Hy3NodeData::Group:
		return this->as_group.children == rhs.as_group.children;
	}

	return false;
}

bool Hy3Node::operator==(const Hy3Node& rhs) const {
	return this->data == rhs.data;
}

void Hy3Node::recalcSizePosRecursive(bool force) {
	if (this->data.type != Hy3NodeData::Group) {
		this->layout->applyNodeDataToWindow(this, force);
		return;
	}

	auto* group = &this->data.as_group;

	if (group->children.size() == 1 && this->parent != nullptr) {
		auto child = group->children.front();

		if (child == this) {
			Debug::log(ERR, "a group (%p) has become its own child", this);
		}

		double distortOut;
		double distortIn;

		const auto* gaps_in     = &g_pConfigManager->getConfigValuePtr("general:gaps_in")->intValue;
		const auto* gaps_out    = &g_pConfigManager->getConfigValuePtr("general:gaps_out")->intValue;

		if (gaps_in > gaps_out) {
			distortOut = *gaps_out - 1.0;
		} else {
			distortOut = *gaps_in - 1.0;
		}

		if (distortOut < 0) distortOut = 0.0;

		distortIn = *gaps_in * 2;

		switch (group->layout) {
		case Hy3GroupLayout::SplitH:
			child->position.x = this->position.x - distortOut;
			child->size.x = this->size.x - distortIn;
			child->position.y = this->position.y;
			child->size.y = this->size.y;
			break;
		case Hy3GroupLayout::SplitV:
			child->position.y = this->position.y - distortOut;
			child->size.y = this->size.y - distortIn;
			child->position.x = this->position.x;
			child->size.x = this->size.x;
		case Hy3GroupLayout::Tabbed:
			// TODO
			break;
		}

		child->recalcSizePosRecursive(force);
		return;
	}

	int constraint;
	switch (group->layout) {
	case Hy3GroupLayout::SplitH:
		constraint = this->size.x;
		break;
	case Hy3GroupLayout::SplitV:
		constraint = this->size.y;
		break;
	case Hy3GroupLayout::Tabbed:
		break;
	}

	double ratio_mul = group->layout != Hy3GroupLayout::Tabbed ? group->children.empty() ? 0 : constraint / group->children.size() : 0;

	double offset = 0;

	for(auto child: group->children) {
		switch (group->layout) {
		case Hy3GroupLayout::SplitH:
			child->position.x = this->position.x + offset;
			child->size.x = child->size_ratio * ratio_mul;
			offset += child->size.x;
			child->position.y = this->position.y;
			child->size.y = this->size.y;
			break;
		case Hy3GroupLayout::SplitV:
			child->position.y = this->position.y + offset;
			child->size.y = child->size_ratio * ratio_mul;
			offset += child->size.y;
			child->position.x = this->position.x;
			child->size.x = this->size.x;
			break;
		case Hy3GroupLayout::Tabbed:
			// TODO: tab bars
			child->position = this->position;
			child->size = this->size;
			break;
		}

		child->recalcSizePosRecursive(force);
	}
}

bool swallowGroup(Hy3Node* into) {
	if (into == nullptr
			|| into->parent == nullptr
			|| into->data.type != Hy3NodeData::Group
			|| into->data.as_group.children.size() != 1)
		return false;

	auto* child = into->data.as_group.children.front();

	Debug::log(LOG, "Swallowing %p into %p", child, into);
	swapNodeData(*into, *child);
	into->layout->nodes.remove(*child);

	return true;
}

bool Hy3GroupData::hasChild(Hy3Node* node) {
	Debug::log(LOG, "Searching for child %p of %p", this, node);
	for (auto child: this->children) {
		if (child == node) return true;

		if (child->data.type == Hy3NodeData::Group) {
			if (child->data.as_group.hasChild(node)) return true;
		}
	}

	return false;
}

void swapNodeData(Hy3Node& a, Hy3Node& b) {
	Hy3NodeData aData = std::move(a.data);
	a.data = b.data;
	b.data = aData;

	if (a.data.type == Hy3NodeData::Group) {
		for (auto child: a.data.as_group.children) {
			child->parent = &a;
		}
	}

	if (b.data.type == Hy3NodeData::Group) {
		for (auto child: b.data.as_group.children) {
			child->parent = &b;
		}
	}
}

int Hy3Layout::getWorkspaceNodeCount(const int& id) {
	int count = 0;

	for (auto& node: this->nodes) {
		if (node.workspace_id == id && node.valid) count++;
	}

	return count;
}

Hy3Node* Hy3Layout::getNodeFromWindow(CWindow* window) {
	for (auto& node: this->nodes) {
		if (node.data.type == Hy3NodeData::Window && node.data.as_window == window) {
			return &node;
		}
	}

	return nullptr;
}

Hy3Node* Hy3Layout::getWorkspaceRootGroup(const int& id) {
	for (auto& node: this->nodes) {
		if (node.workspace_id == id && node.parent == nullptr && node.data.type == Hy3NodeData::Group) {
			return &node;
		}
	}

	return nullptr;
}

void Hy3Layout::applyNodeDataToWindow(Hy3Node* node, bool force) {
	if (node->data.type != Hy3NodeData::Window) return;
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
		monitor = g_pCompositor->getMonitorFromID(g_pCompositor->getWorkspaceByID(node->workspace_id)->m_iMonitorID);
	}

	if (monitor == nullptr) {
		Debug::log(ERR, "Orphaned Node %x (workspace ID: %i)!!", node, node->workspace_id);
		return;
	}

	// for gaps outer
	const bool display_left   = STICKS(node->position.x, monitor->vecPosition.x + monitor->vecReservedTopLeft.x);
	const bool display_right  = STICKS(node->position.x + node->size.x, monitor->vecPosition.x + monitor->vecSize.x - monitor->vecReservedBottomRight.x);
	const bool display_top    = STICKS(node->position.y, monitor->vecPosition.y + monitor->vecReservedTopLeft.y);
	const bool display_bottom = STICKS(node->position.y + node->size.y, monitor->vecPosition.y + monitor->vecSize.y - monitor->vecReservedBottomRight.y);

	const auto* border_size = &g_pConfigManager->getConfigValuePtr("general:border_size")->intValue;
	const auto* gaps_in     = &g_pConfigManager->getConfigValuePtr("general:gaps_in")->intValue;
	const auto* gaps_out    = &g_pConfigManager->getConfigValuePtr("general:gaps_out")->intValue;
	static auto* const single_window_no_gaps = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:hy3:no_gaps_when_only")->intValue;

	if (!g_pCompositor->windowExists(window) || !window->m_bIsMapped) {
		Debug::log(ERR, "Node %p holding invalid window %p!!", node, window);
		this->onWindowRemovedTiling(window);
		return;
	}

	window->m_vSize = node->size;
	window->m_vPosition = node->position;

	auto calcPos = window->m_vPosition + Vector2D(*border_size, *border_size);
	auto calcSize = window->m_vSize - Vector2D(2 * *border_size, 2 * *border_size);

	const auto workspace_node_count = this->getWorkspaceNodeCount(window->m_iWorkspaceID);

	if (*single_window_no_gaps
			&& !g_pCompositor->isWorkspaceSpecial(window->m_iWorkspaceID)
			&& (workspace_node_count == 1
					|| (window->m_bIsFullscreen
							&& g_pCompositor->getWorkspaceByID(window->m_iWorkspaceID)->m_efFullscreenMode == FULLSCREEN_MAXIMIZED)))
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

		Vector2D offset_topleft(
			display_left ? *gaps_out : *gaps_in,
			display_top ? *gaps_out : *gaps_in
		);

		Vector2D offset_bottomright(
			display_right ? *gaps_out : *gaps_in,
			display_bottom ? *gaps_out : *gaps_in
		);

		calcPos = calcPos + offset_topleft;
		calcSize = calcSize - offset_topleft - offset_bottomright;

		const auto reserved_area = window->getFullWindowReservedArea();
		calcPos = calcPos + reserved_area.topLeft;
		calcSize = calcSize - (reserved_area.topLeft - reserved_area.bottomRight);

		window->m_vRealPosition = calcPos;
		window->m_vRealSize = calcSize;
		Debug::log(LOG, "Set size (%f %f)", calcSize.x, calcSize.y);

		g_pXWaylandManager->setWindowSize(window, calcSize);

		if (force) {
			g_pHyprRenderer->damageWindow(window);

			window->m_vRealPosition.warp();
			window->m_vRealSize.warp();

			g_pHyprRenderer->damageWindow(window);
		}

		window->updateWindowDecos();
	}
}

void Hy3Layout::onWindowCreatedTiling(CWindow* window) {
	if (window->m_bIsFloating) return;

	auto* existing = this->getNodeFromWindow(window);
	if (existing != nullptr) {
		Debug::log(WARN, "Attempted to add a window(%p) that is already tiled(as %p) to the layout", window, existing);
		return;
	}

	auto* monitor = g_pCompositor->getMonitorFromID(window->m_iMonitorID);

	Hy3Node* opening_into;
	Hy3Node* opening_after;

	if (g_pCompositor->m_pLastWindow != nullptr
			&& !g_pCompositor->m_pLastWindow->m_bIsFloating
			&& g_pCompositor->m_pLastWindow != window
			&& g_pCompositor->m_pLastWindow->m_iWorkspaceID == window->m_iWorkspaceID
			&& g_pCompositor->m_pLastWindow->m_bIsMapped)
	{
		opening_after = this->getNodeFromWindow(g_pCompositor->m_pLastWindow);
	} else {
		opening_after = this->getNodeFromWindow(g_pCompositor->vectorToWindowTiled(g_pInputManager->getMouseCoordsInternal()));
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

	if (opening_into->data.type != Hy3NodeData::Group) {
		Debug::log(ERR, "opening_into node %p was not of type Group", opening_into);
		return;
	}

	if (opening_into->workspace_id != window->m_iWorkspaceID) {
		Debug::log(WARN, "opening_into node %p has workspace %d which does not match the opening window (workspace %d)", opening_into, opening_into->workspace_id, window->m_iWorkspaceID);
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
	Debug::log(LOG, "opened new window %p(node: %p) on window %p in %p", window, &node, opening_after, opening_into);

	opening_into->data.as_group.lastFocusedChild = &node;
	opening_into->recalcSizePosRecursive();
	Debug::log(LOG, "opening_into (%p) contains new child (%p)? %d", opening_into, &node, opening_into->data.as_group.hasChild(&node));
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

	auto* parent = node->parent;
	auto* group = &parent->data.as_group;

	if (group->children.size() > 2) {
		auto iter = std::find(group->children.begin(), group->children.end(), node);
		if (iter == group->children.begin()) {
			group->lastFocusedChild = *std::next(iter);
		} else {
			group->lastFocusedChild = *std::prev(iter);
		}
	}

	group->children.remove(node);

	auto splitmod = group->children.empty() ? 0.0 : (1.0 - node->size_ratio) / group->children.size();
	for (auto child: group->children) {
		child->size_ratio -= splitmod;
	}

	this->nodes.remove(*node);

	if (group->children.size() == 1) {
		group->lastFocusedChild = group->children.front();
	}

	while (parent->parent != nullptr && group->children.empty()) {
		auto* child = parent;
		parent = parent->parent;
		group = &parent->data.as_group;

		if (group->children.size() > 2) {
			auto iter = std::find(group->children.begin(), group->children.end(), child);
			if (iter == group->children.begin()) {
				group->lastFocusedChild = *std::next(iter);
			} else {
				group->lastFocusedChild = *std::prev(iter);
			}
		}

		group->children.remove(child);

		auto splitmod = group->children.empty() ? 0.0 : (1.0 - child->size_ratio) / group->children.size();
		for (auto child: group->children) {
				child->size_ratio -= splitmod;
		}

		this->nodes.remove(*child);

		if (group->children.size() == 1) {
			group->lastFocusedChild = group->children.front();
		}
	}

	if (parent != nullptr) {
		parent->recalcSizePosRecursive();

		if (parent->data.as_group.children.size() == 1
				&& parent->data.as_group.children.front()->data.type == Hy3NodeData::Group)
		{
			auto* target_parent = parent;
			while (target_parent != nullptr && swallowGroup(target_parent)) {
				target_parent = target_parent->parent;
			}

			if (target_parent != parent && target_parent != nullptr)
				target_parent->recalcSizePosRecursive();
		}
	}

}

CWindow* Hy3Layout::getNextWindowCandidate(CWindow* window) {
	auto* node = this->getWorkspaceRootGroup(window->m_iWorkspaceID);
	if (node == nullptr) return nullptr;
	while (node->data.type == Hy3NodeData::Group) {
		node = node->data.as_group.lastFocusedChild;
		if (node == nullptr) {
			Debug::log(ERR, "A group's last focused child was null when getting the next selection candidate");
			return nullptr;
		}
	}
	return node->data.as_window;
}

void Hy3Layout::onWindowFocusChange(CWindow* window) {
	Debug::log(LOG, "Switched windows from %p to %p", this->lastActiveWindow, window);
	this->lastActiveWindow = window;
	auto* node = this->getNodeFromWindow(this->lastActiveWindow);
	if (node == nullptr) return;
	Debug::log(LOG, "Switched focused node to %p (parent: %p)", node, node->parent);

	while (node->parent != nullptr) {
		node->parent->data.as_group.lastFocusedChild = node;
		node = node->parent;
	}
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
			top_node->size = monitor->vecSize - monitor->vecReservedTopLeft - monitor->vecReservedBottomRight;
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
			top_node->size = monitor->vecSize - monitor->vecReservedTopLeft - monitor->vecReservedBottomRight;
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
		auto mouse = g_pInputManager->getMouseCoordsInternal();
		auto mouseOffset = mouse - window->m_vPosition;

		this->drag_flags = {
			.started = true,
			.xExtent = mouseOffset.x > window->m_vSize.x / 2,
			.yExtent = mouseOffset.y > window->m_vSize.y / 2,
		};

		Debug::log(LOG, "Positive offsets - x: %d, y: %d", this->drag_flags.xExtent, this->drag_flags.yExtent);
	}

	const auto animate = &g_pConfigManager->getConfigValuePtr("misc:animate_manual_resizes")->intValue;

	auto monitor = g_pCompositor->getMonitorFromID(window->m_iMonitorID);

	const bool display_left   = STICKS(node->position.x, monitor->vecPosition.x + monitor->vecReservedTopLeft.x);
	const bool display_right  = STICKS(node->position.x + node->size.x, monitor->vecPosition.x + monitor->vecSize.x - monitor->vecReservedBottomRight.x);
	const bool display_top    = STICKS(node->position.y, monitor->vecPosition.y + monitor->vecReservedTopLeft.y);
	const bool display_bottom = STICKS(node->position.y + node->size.y, monitor->vecPosition.y + monitor->vecSize.y - monitor->vecReservedBottomRight.y);

	Vector2D allowed_movement = delta;
	if (display_left && display_right) allowed_movement.x = 0;
	if (display_top && display_bottom) allowed_movement.y = 0;

	auto* inner_node = node;

	// break into parent groups when encountering a corner we're dragging in or a tab group
	while (inner_node->parent != nullptr) {
		auto& group = inner_node->parent->data.as_group;

		switch (group.layout) {
		case Hy3GroupLayout::Tabbed:
			// treat tabbed layouts as if they dont exist during resizing
			goto cont;
		case Hy3GroupLayout::SplitH:
			if ((this->drag_flags.xExtent && group.children.back() == inner_node)
					|| (!this->drag_flags.xExtent && group.children.front() == inner_node)) {
				goto cont;
			}
			break;
		case Hy3GroupLayout::SplitV:
			if ((this->drag_flags.yExtent && group.children.back() == inner_node)
					|| (!this->drag_flags.yExtent && group.children.front() == inner_node)) {
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

	// break into parent groups when encountering a corner we're dragging in, a tab group,
	// or a layout matching the inner_parent.
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
					|| (!this->drag_flags.xExtent && group.children.front() == outer_node)) {
				goto cont2;
			}
			break;
		case Hy3GroupLayout::SplitV:
			if ((this->drag_flags.yExtent && group.children.back() == outer_node)
					|| (!this->drag_flags.yExtent && group.children.front() == outer_node)) {
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
		auto ratio_mod = allowed_movement.x * (float) inner_group.children.size() / inner_parent->size.x;

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
		auto ratio_mod = allowed_movement.y * (float) inner_parent->data.as_group.children.size() / inner_parent->size.y;

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
	}

	inner_parent->recalcSizePosRecursive(*animate == 0);

	if (outer_node != nullptr && outer_node->parent != nullptr) {
		auto* outer_parent = outer_node->parent;
		auto& outer_group = outer_parent->data.as_group;
		// adjust the outer node
		switch (outer_group.layout) {
		case Hy3GroupLayout::SplitH: {
			auto ratio_mod = allowed_movement.x * (float) outer_group.children.size() / outer_parent->size.x;

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
			auto ratio_mod = allowed_movement.y * (float) outer_parent->data.as_group.children.size() / outer_parent->size.y;

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
		}

		outer_parent->recalcSizePosRecursive(*animate == 0);
	}
}

void Hy3Layout::fullscreenRequestForWindow(CWindow* window, eFullscreenMode fullscreen_mode, bool on) {
	if (!g_pCompositor->windowValidMapped(window)) return;
	if (on == window->m_bIsFullscreen || g_pCompositor->isWorkspaceSpecial(window->m_iWorkspaceID)) return;

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
	return "";
}

SWindowRenderLayoutHints Hy3Layout::requestRenderHints(CWindow* window) {
	return {};
}

void Hy3Layout::switchWindows(CWindow* pWindowA, CWindow* pWindowB) {
	// todo
}

void Hy3Layout::alterSplitRatio(CWindow* pWindow, float delta, bool exact) {
	// todo
}

std::string Hy3Layout::getLayoutName() {
    return "hy3";
}

void Hy3Layout::replaceWindowDataWith(CWindow* from, CWindow* to) {
	auto* node = this->getNodeFromWindow(from);
	if (node == nullptr) return;

	node->data.as_window = to;
	this->applyNodeDataToWindow(node);
}

void Hy3Layout::onEnable() {
	for (auto &window : g_pCompositor->m_vWindows) {
		if (window->isHidden()
				|| !window->m_bIsMapped
				|| window->m_bFadingOut
				|| window->m_bIsFloating)
			continue;

		this->onWindowCreatedTiling(window.get());
	}
}

void Hy3Layout::onDisable() {
	this->nodes.clear();
}

void Hy3Layout::makeGroupOn(CWindow* window, Hy3GroupLayout layout) {
	auto* node = this->getNodeFromWindow(window);
	if (node == nullptr) return;

	if (node->parent->data.as_group.children.size() == 1
			&& (node->parent->data.as_group.layout == Hy3GroupLayout::SplitH
			|| node->parent->data.as_group.layout == Hy3GroupLayout::SplitV))
	{
			node->parent->data.as_group.layout = layout;
			node->parent->recalcSizePosRecursive();
			return;
	}

	this->nodes.push_back({
			.parent = node,
			.data = node->data.as_window,
			.workspace_id = node->workspace_id,
			.layout = this,
	});

	node->data = layout;
	node->data.as_group.children.push_back(&this->nodes.back());
	node->data.as_group.lastFocusedChild = &this->nodes.back();
	node->recalcSizePosRecursive();

	return;
}

Hy3Node* shiftOrGetFocus(Hy3Node& node, ShiftDirection direction, bool shift);

void Hy3Layout::shiftFocus(CWindow* window, ShiftDirection direction) {
	Debug::log(LOG, "ShiftFocus %p %d", window, direction);
	auto* node = this->getNodeFromWindow(window);
	if (node == nullptr) return;

	Hy3Node* target;
	if ((target = shiftOrGetFocus(*node, direction, false))) {
		g_pCompositor->focusWindow(target->data.as_window);
	}
}

void Hy3Layout::shiftWindow(CWindow* window, ShiftDirection direction) {
	Debug::log(LOG, "ShiftWindow %p %d", window, direction);
	auto* node = this->getNodeFromWindow(window);
	if (node == nullptr) return;


	shiftOrGetFocus(*node, direction, true);
}

Hy3Node* findCommonParentNode(Hy3Node& a, Hy3Node& b) {
	Hy3Node* last_node = nullptr;
	Hy3Node* searcher = &a;

	while (searcher != nullptr) {
		if (searcher->data.type == Hy3NodeData::Group) {
			for (auto child: searcher->data.as_group.children) {
				if (last_node == child) continue; // dont rescan already scanned tree
				if (child == &b) return searcher;
				if (child->data.type == Hy3NodeData::Group && child->data.as_group.hasChild(&b)) {
					return searcher;
				}
			}
		}

		last_node = searcher;
		searcher = searcher->parent;
	}

	return nullptr;
}

bool shiftIsForward(ShiftDirection direction) {
	return direction == ShiftDirection::Right || direction == ShiftDirection::Down;
}

// if shift is true, shift the window in the given direction, returning nullptr,
// if shift is false, return the window in the given direction or nullptr.
Hy3Node* shiftOrGetFocus(Hy3Node& node, ShiftDirection direction, bool shift) {

	auto* break_origin = &node;
	auto* break_parent = break_origin->parent;

	// break parents until we hit a container oriented the same way as the shift direction
	while (true) {
		if (break_parent == nullptr) return nullptr;

		auto& group = break_parent->data.as_group; // must be a group in order to be a parent

		if (((group.layout == Hy3GroupLayout::SplitH || group.layout == Hy3GroupLayout::Tabbed)
				 && (direction == ShiftDirection::Left || direction == ShiftDirection::Right))
				|| (group.layout == Hy3GroupLayout::SplitV
						&& (direction == ShiftDirection::Up || direction == ShiftDirection::Down)))
		{
			// group has the correct orientation

			// if this movement would break out of the group, continue the break loop (do not enter this if)
			// otherwise break.
			if (!((!shiftIsForward(direction) && group.children.front() == break_origin)
						|| (shiftIsForward(direction) && group.children.back() == break_origin)))
				break;
		}

		// always break at the outermost group
		if (break_parent->parent == nullptr) {
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

		if ((*iter)->data.type == Hy3NodeData::Window) {
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

				if (((group_data.layout == Hy3GroupLayout::SplitH || group_data.layout == Hy3GroupLayout::Tabbed)
						 && (direction == ShiftDirection::Left || direction == ShiftDirection::Right))
						|| (group_data.layout == Hy3GroupLayout::SplitV
								&& (direction == ShiftDirection::Up || direction == ShiftDirection::Down)))
				{
					// if the group has the same orientation as movement pick the last/first child based
					// on movement direction
					if (shiftIsForward(direction)) iter = group_data.children.begin();
					else {
						iter = std::prev(group_data.children.end());
						shift_after = true;
					}
				} else {
					if (group_data.lastFocusedChild != nullptr) {
						iter = std::find(group_data.children.begin(), group_data.children.end(), group_data.lastFocusedChild);
						shift_after = true;
					} else {
						iter = group_data.children.begin();
					}
				}

				if ((*iter)->data.type == Hy3NodeData::Window) {
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
		// nullptr is used as a signal value instead of removing it first to avoid iterator invalidation.
		auto iter = std::find(group_data.children.begin(), group_data.children.end(), &node);
		*iter = nullptr;
		target_group->data.as_group.children.insert(insert, &node);
		target_group->data.as_group.children.remove(nullptr);
		target_group->recalcSizePosRecursive();
	} else {
		auto* old_parent = node.parent;
		auto* old_group = &old_parent->data.as_group;

		if (old_group->children.size() > 2) {
			auto iter = std::find(old_group->children.begin(), old_group->children.end(), &node);
			if (iter == old_group->children.begin()) {
				old_group->lastFocusedChild = *std::next(iter);
			} else {
				old_group->lastFocusedChild = *std::prev(iter);
			}
		}

		node.parent = target_group;
		target_group->data.as_group.children.insert(insert, &node);

		// must happen AFTER `insert` is used
		old_group->children.remove(&node);

		auto splitmod = old_group->children.empty() ? 0.0 : (1.0 - node.size_ratio) / old_group->children.size();
		for (auto child: old_group->children) {
			child->size_ratio -= splitmod;
		}

		node.size_ratio = 1.0;

		if (old_group->children.empty()) {
			while (old_parent->parent != nullptr && old_parent->data.as_group.children.empty()) {
				auto* child = old_parent;
				old_parent = old_parent->parent;
				old_group = &old_parent->data.as_group;

				if (old_group->children.size() > 2) {
					auto iter = std::find(old_group->children.begin(), old_group->children.end(), child);
					if (iter == old_group->children.begin()) {
						old_group->lastFocusedChild = *std::next(iter);
					} else {
						old_group->lastFocusedChild = *std::prev(iter);
					}
				}

				old_parent->data.as_group.children.remove(child);

				if (old_group->children.size() == 1) {
					old_group->lastFocusedChild = old_group->children.front();
				}

				old_parent->layout->nodes.remove(*child);

				auto splitmod = old_group->children.empty() ? 0.0 : (1.0 - child->size_ratio) / old_group->children.size();
				for (auto child: old_group->children) {
					child->size_ratio -= splitmod;
				}
			}
		} else if (old_group->children.size() == 1) {
			old_group->lastFocusedChild = old_group->children.front();
		}

		old_parent->recalcSizePosRecursive();
		target_group->recalcSizePosRecursive();

		auto* target_parent = target_group->parent;
		while (target_parent != nullptr && swallowGroup(target_parent)) {
			target_parent = target_parent->parent;
		}

		if (target_parent != target_group && target_parent != nullptr)
			target_parent->recalcSizePosRecursive();
	}

	return nullptr;
}

std::string Hy3Node::debugNode() {
	std::stringstream buf;
	std::string addr = "0x" + std::to_string((size_t)this);
	switch (this->data.type) {
	case Hy3NodeData::Window:
		buf << "window(";
		buf << std::hex << this;
		buf << ") [hypr ";
		buf << this->data.as_window;
		buf << "]";
		break;
	case Hy3NodeData::Group:
		buf << "group(";
		buf << std::hex << this;
		buf << ") [";

		switch (this->data.as_group.layout) {
		case Hy3GroupLayout::SplitH:
			buf << "splith";
			break;
		case Hy3GroupLayout::SplitV:
			buf << "splitv";
			break;
		case Hy3GroupLayout::Tabbed:
			buf << "tabs";
			break;
		}

		buf << "]";
		for (auto* child: this->data.as_group.children) {
			buf << "\n|-";
			// this is terrible
			for (char c: child->debugNode()) {
				buf << c;
				if (c == '\n') buf << "  ";
			}
		}

		break;
	}

	return buf.str();
}
