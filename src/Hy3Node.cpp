#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>

#include "globals.hpp"
#include "Hy3Node.hpp"

// Hy3GroupData //

Hy3GroupData::Hy3GroupData(Hy3GroupLayout layout): layout(layout) {}

Hy3GroupData::Hy3GroupData(Hy3GroupData&& from) {
	this->layout = from.layout;
	this->children = std::move(from.children);
	this->group_focused = from.group_focused;
	this->focused_child = from.focused_child;
	from.focused_child = nullptr;
	this->tab_bar = from.tab_bar;
	from.tab_bar = nullptr;
}

Hy3GroupData::~Hy3GroupData() {
	if (this->tab_bar != nullptr) this->tab_bar->bar.beginDestroy();
}

bool Hy3GroupData::hasChild(Hy3Node* node) {
	Debug::log(LOG, "Searching for child %p of %p", this, node);
	for (auto child: this->children) {
		if (child == node) return true;

		if (child->data.type == Hy3NodeType::Group) {
			if (child->data.as_group.hasChild(node)) return true;
		}
	}

	return false;
}

// Hy3NodeData //

Hy3NodeData::Hy3NodeData(): Hy3NodeData((CWindow*) nullptr) {}

Hy3NodeData::Hy3NodeData(CWindow* window): type(Hy3NodeType::Window) { this->as_window = window; }

Hy3NodeData::Hy3NodeData(Hy3GroupLayout layout): Hy3NodeData(Hy3GroupData(layout)) {}

Hy3NodeData::Hy3NodeData(Hy3GroupData group): type(Hy3NodeType::Group) {
	new (&this->as_group) Hy3GroupData(std::move(group));
}

Hy3NodeData::Hy3NodeData(Hy3NodeData&& from): type(from.type) {
	Debug::log(
	    LOG,
	    "Move CTor type matches? %d is group? %d",
	    this->type == from.type,
	    this->type == Hy3NodeType::Group
	);

	switch (from.type) {
	case Hy3NodeType::Window: this->as_window = from.as_window; break;
	case Hy3NodeType::Group: new (&this->as_group) Hy3GroupData(std::move(from.as_group)); break;
	}
}

Hy3NodeData::~Hy3NodeData() {
	switch (this->type) {
	case Hy3NodeType::Window: break;
	case Hy3NodeType::Group:
		this->as_group.~Hy3GroupData();

		// who ever thought calling the dtor after a move was a good idea?
		this->type = Hy3NodeType::Window;
		break;
	}
}

Hy3NodeData& Hy3NodeData::operator=(CWindow* window) {
	*this = Hy3NodeData(window);

	return *this;
}

Hy3NodeData& Hy3NodeData::operator=(Hy3GroupLayout layout) {
	*this = Hy3NodeData(layout);

	return *this;
}

Hy3NodeData& Hy3NodeData::operator=(Hy3NodeData&& from) {
	Debug::log(
	    LOG,
	    "operator= type matches? %d is group? %d",
	    this->type == from.type,
	    this->type == Hy3NodeType::Group
	);

	if (this->type == Hy3NodeType::Group) {
		this->as_group.~Hy3GroupData();
	}

	this->type = from.type;

	switch (this->type) {
	case Hy3NodeType::Window: this->as_window = from.as_window; break;
	case Hy3NodeType::Group: new (&this->as_group) Hy3GroupData(std::move(from.as_group)); break;
	}

	return *this;
}

bool Hy3NodeData::operator==(const Hy3NodeData& rhs) const { return this == &rhs; }

// Hy3Node //

bool Hy3Node::operator==(const Hy3Node& rhs) const { return this->data == rhs.data; }

void Hy3Node::focus() {
	this->markFocused();

	switch (this->data.type) {
	case Hy3NodeType::Window:
		this->data.as_window->setHidden(false);
		g_pCompositor->focusWindow(this->data.as_window);
		break;
	case Hy3NodeType::Group:
		g_pCompositor->focusWindow(nullptr);
		this->raiseToTop();
		break;
	}
}

bool Hy3Node::focusWindow() {
	switch (this->data.type) {
	case Hy3NodeType::Window:
		this->markFocused();
		g_pCompositor->focusWindow(this->data.as_window);

		return true;
	case Hy3NodeType::Group:
		if (this->data.as_group.layout == Hy3GroupLayout::Tabbed) {
			if (this->data.as_group.focused_child != nullptr) {
				return this->data.as_group.focused_child->focusWindow();
			}
		} else {
			for (auto* node: this->data.as_group.children) {
				if (node->focusWindow()) break;
			}
		}

		return false;
	}
}

void markGroupFocusedRecursive(Hy3GroupData& group) {
	group.group_focused = true;
	for (auto& child: group.children) {
		if (child->data.type == Hy3NodeType::Group) markGroupFocusedRecursive(child->data.as_group);
	}
}

void Hy3Node::markFocused() {
	Hy3Node* node = this;

	// undo decos for root focus
	auto* root = node;
	while (root->parent != nullptr) root = root->parent;

	// update focus
	if (this->data.type == Hy3NodeType::Group) {
		markGroupFocusedRecursive(this->data.as_group);
	}

	auto* node2 = node;
	while (node2->parent != nullptr) {
		node2->parent->data.as_group.focused_child = node2;
		node2->parent->data.as_group.group_focused = false;
		node2 = node2->parent;
	}

	root->updateDecos();
}

void Hy3Node::raiseToTop() {
	switch (this->data.type) {
	case Hy3NodeType::Window: g_pCompositor->moveWindowToTop(this->data.as_window); break;
	case Hy3NodeType::Group:
		for (auto* child: this->data.as_group.children) {
			child->raiseToTop();
		}
		break;
	}
}

Hy3Node* Hy3Node::getFocusedNode() {
	switch (this->data.type) {
	case Hy3NodeType::Window: return this;
	case Hy3NodeType::Group:
		if (this->data.as_group.focused_child == nullptr || this->data.as_group.group_focused) {
			return this;
		} else {
			return this->data.as_group.focused_child->getFocusedNode();
		}
	}
}

bool Hy3Node::isIndirectlyFocused() {
	Hy3Node* node = this;

	while (node->parent != nullptr) {
		if (!node->parent->data.as_group.group_focused
		    && node->parent->data.as_group.focused_child != node)
			return false;
		node = node->parent;
	}

	return true;
}

void Hy3Node::recalcSizePosRecursive(bool no_animation) {
	// clang-format off
	static const auto* gaps_in = &HyprlandAPI::getConfigValue(PHANDLE, "general:gaps_in")->intValue;
	static const auto* gaps_out = &HyprlandAPI::getConfigValue(PHANDLE, "general:gaps_out")->intValue;
	static const auto* group_inset = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:hy3:group_inset")->intValue;
	static const auto* tab_bar_height = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:hy3:tabs:height")->intValue;
	static const auto* tab_bar_padding = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:hy3:tabs:padding")->intValue;
	// clang-format on

	int outer_gaps = 0;
	Vector2D gap_pos_offset;
	Vector2D gap_size_offset;
	if (this->parent == nullptr) {
		outer_gaps = *gaps_out - *gaps_in;

		gap_pos_offset = Vector2D(outer_gaps, outer_gaps);
		gap_size_offset = Vector2D(outer_gaps * 2, outer_gaps * 2);
	} else {
		gap_pos_offset = this->gap_pos_offset;
		gap_size_offset = this->gap_size_offset;
	}

	auto tpos = this->position;
	auto tsize = this->size;

	double tab_height_offset = *tab_bar_height + *tab_bar_padding;

	if (this->data.type != Hy3NodeType::Group) {
		this->data.as_window->setHidden(this->hidden);
		this->layout->applyNodeDataToWindow(this, no_animation);
		return;
	}

	auto* group = &this->data.as_group;

	if (group->children.size() == 1 && this->parent != nullptr) {
		auto child = group->children.front();

		if (child == this) {
			Debug::log(ERR, "a group (%p) has become its own child", this);
			errorNotif();
		}

		switch (group->layout) {
		case Hy3GroupLayout::SplitH:
			child->position.x = tpos.x;
			child->size.x = tsize.x - *group_inset;
			child->position.y = tpos.y;
			child->size.y = tsize.y;
			break;
		case Hy3GroupLayout::SplitV:
			child->position.y = tpos.y;
			child->size.y = tsize.y - *group_inset;
			child->position.x = tpos.x;
			child->size.x = tsize.x;
			break;
		case Hy3GroupLayout::Tabbed:
			child->position.y = tpos.y + tab_height_offset;
			child->size.y = tsize.y - tab_height_offset;
			child->position.x = tpos.x;
			child->size.x = tsize.x;
			break;
		}

		child->gap_pos_offset = gap_pos_offset;
		child->gap_size_offset = gap_size_offset;

		child->setHidden(this->hidden);

		child->recalcSizePosRecursive(no_animation);
		this->updateTabBar(no_animation);
		return;
	}

	int constraint;
	switch (group->layout) {
	case Hy3GroupLayout::SplitH: constraint = tsize.x; break;
	case Hy3GroupLayout::SplitV: constraint = tsize.y; break;
	case Hy3GroupLayout::Tabbed: break;
	}

	double ratio_mul = group->layout != Hy3GroupLayout::Tabbed
	                     ? group->children.empty() ? 0 : constraint / group->children.size()
	                     : 0;

	double offset = 0;

	if (group->layout == Hy3GroupLayout::Tabbed && group->focused_child != nullptr
	    && !group->focused_child->hidden)
	{
		group->focused_child->setHidden(false);

		auto box = wlr_box {tpos.x, tpos.y, tsize.x, tsize.y};
		g_pHyprRenderer->damageBox(&box);
	}

	for (auto* child: group->children) {
		switch (group->layout) {
		case Hy3GroupLayout::SplitH:
			child->position.x = tpos.x + offset;
			child->size.x = child->size_ratio * ratio_mul;
			offset += child->size.x;
			child->position.y = tpos.y;
			child->size.y = tsize.y;
			child->setHidden(this->hidden);
			child->recalcSizePosRecursive(no_animation);
			break;
		case Hy3GroupLayout::SplitV:
			child->position.y = tpos.y + offset;
			child->size.y = child->size_ratio * ratio_mul;
			offset += child->size.y;
			child->position.x = tpos.x;
			child->size.x = tsize.x;
			child->setHidden(this->hidden);
			child->recalcSizePosRecursive(no_animation);
			break;
		case Hy3GroupLayout::Tabbed:
			child->position.y = tpos.y + tab_height_offset;
			child->size.y = tsize.y - tab_height_offset;
			child->position.x = tpos.x;
			child->size.x = tsize.x;
			bool hidden = this->hidden || group->focused_child != child;
			child->setHidden(hidden);
			child->recalcSizePosRecursive(no_animation);
			break;
		}

		child->gap_pos_offset = gap_pos_offset;
		child->gap_size_offset = gap_pos_offset;
	}

	this->updateTabBar(no_animation);
}

struct FindTopWindowInNodeResult {
	CWindow* window = nullptr;
	size_t index = 0;
};

void findTopWindowInNode(Hy3Node& node, FindTopWindowInNodeResult& result) {
	switch (node.data.type) {
	case Hy3NodeType::Window: {
		auto* window = node.data.as_window;
		auto& windows = g_pCompositor->m_vWindows;

		for (; result.index < windows.size(); result.index++) {
			if (&*windows[result.index] == window) {
				result.window = window;
				break;
			}
		}

	} break;
	case Hy3NodeType::Group: {
		auto& group = node.data.as_group;

		if (group.layout == Hy3GroupLayout::Tabbed) {
			if (group.focused_child != nullptr) findTopWindowInNode(*group.focused_child, result);
		} else {
			for (auto* child: group.children) {
				findTopWindowInNode(*child, result);
			}
		}
	} break;
	}
}

void Hy3Node::updateTabBar(bool no_animation) {
	if (this->data.type == Hy3NodeType::Group) {
		auto& group = this->data.as_group;

		if (group.layout == Hy3GroupLayout::Tabbed) {
			if (group.tab_bar == nullptr) group.tab_bar = &this->layout->tab_groups.emplace_back(*this);
			group.tab_bar->updateWithGroup(*this, no_animation);

			FindTopWindowInNodeResult result;
			findTopWindowInNode(*this, result);
			group.tab_bar->target_window = result.window;
			if (result.window != nullptr) group.tab_bar->workspace_id = result.window->m_iWorkspaceID;
		} else if (group.tab_bar != nullptr) {
			group.tab_bar->bar.beginDestroy();
			group.tab_bar = nullptr;
		}
	}
}

void Hy3Node::updateTabBarRecursive() {
	auto* node = this;

	do {
		node->updateTabBar();
		node = node->parent;
	} while (node != nullptr);
}

void Hy3Node::updateDecos() {
	switch (this->data.type) {
	case Hy3NodeType::Window:
		if (this->data.as_window->m_bIsMapped)
			g_pCompositor->updateWindowAnimatedDecorationValues(this->data.as_window);
		break;
	case Hy3NodeType::Group:
		for (auto* child: this->data.as_group.children) {
			child->updateDecos();
		}

		this->updateTabBar();
	}
}

std::string Hy3Node::getTitle() {
	switch (this->data.type) {
	case Hy3NodeType::Window: return this->data.as_window->m_szTitle;
	case Hy3NodeType::Group:
		std::string title;

		switch (this->data.as_group.layout) {
		case Hy3GroupLayout::SplitH: title = "[H] "; break;
		case Hy3GroupLayout::SplitV: title = "[V] "; break;
		case Hy3GroupLayout::Tabbed: title = "[T] "; break;
		}

		if (this->data.as_group.focused_child == nullptr) {
			title += "Group";
		} else {
			title += this->data.as_group.focused_child->getTitle();
		}

		return title;
	}

	return "";
}

bool Hy3Node::isUrgent() {
	switch (this->data.type) {
	case Hy3NodeType::Window: return this->data.as_window->m_bIsUrgent;
	case Hy3NodeType::Group:
		for (auto* child: this->data.as_group.children) {
			if (child->isUrgent()) return true;
		}

		return false;
	}
}

void Hy3Node::setHidden(bool hidden) {
	this->hidden = hidden;

	if (this->data.type == Hy3NodeType::Group) {
		for (auto* child: this->data.as_group.children) {
			child->setHidden(hidden);
		}
	}
}

Hy3Node* Hy3Node::findNodeForTabGroup(Hy3TabGroup& tab_group) {
	if (this->data.type == Hy3NodeType::Group) {
		if (this->hidden) return nullptr;

		auto& group = this->data.as_group;

		if (group.layout == Hy3GroupLayout::Tabbed && group.tab_bar == &tab_group) {
			return this;
		}

		for (auto& node: group.children) {
			auto* r = node->findNodeForTabGroup(tab_group);
			if (r != nullptr) return r;
		}
	} else return nullptr;

	return nullptr;
}

void Hy3Node::appendAllWindows(std::vector<CWindow*>& list) {
	switch (this->data.type) {
	case Hy3NodeType::Window: list.push_back(this->data.as_window); break;
	case Hy3NodeType::Group:
		for (auto* child: this->data.as_group.children) {
			child->appendAllWindows(list);
		}
		break;
	}
}

std::string Hy3Node::debugNode() {
	std::stringstream buf;
	std::string addr = "0x" + std::to_string((size_t) this);
	switch (this->data.type) {
	case Hy3NodeType::Window:
		buf << "window(";
		buf << std::hex << this;
		buf << ") [hypr ";
		buf << this->data.as_window;
		buf << "] size ratio: ";
		buf << this->size_ratio;
		break;
	case Hy3NodeType::Group:
		buf << "group(";
		buf << std::hex << this;
		buf << ") [";

		switch (this->data.as_group.layout) {
		case Hy3GroupLayout::SplitH: buf << "splith"; break;
		case Hy3GroupLayout::SplitV: buf << "splitv"; break;
		case Hy3GroupLayout::Tabbed: buf << "tabs"; break;
		}

		buf << "] size ratio: ";
		buf << this->size_ratio;
		for (auto* child: this->data.as_group.children) {
			buf << "\n|-";
			if (child == nullptr) {
				buf << "nullptr";
			} else {
				// this is terrible
				for (char c: child->debugNode()) {
					buf << c;
					if (c == '\n') buf << "  ";
				}
			}
		}

		break;
	}

	return buf.str();
}

Hy3Node* Hy3Node::removeFromParentRecursive() {
	Hy3Node* parent = this;

	Debug::log(LOG, "Recursively removing parent nodes of %p", parent);

	while (parent != nullptr) {
		if (parent->parent == nullptr) {
			Debug::log(ERR, "* UAF DEBUGGING - %p's parent is null, its the root group", parent);

			if (parent == this) {
				Debug::log(ERR, "* UAF DEBUGGING - returning nullptr as this == root group");
			} else {
				Debug::log(ERR, "* UAF DEBUGGING - deallocing %p and returning nullptr", parent);
				parent->layout->nodes.remove(*parent);
			}
			return nullptr;
		}

		auto* child = parent;
		parent = parent->parent;
		auto& group = parent->data.as_group;

		if (group.children.size() > 2) {
			auto iter = std::find(group.children.begin(), group.children.end(), child);

			group.group_focused = false;
			if (iter == group.children.begin()) {
				group.focused_child = *std::next(iter);
			} else {
				group.focused_child = *std::prev(iter);
			}
		}

		if (!group.children.remove(child)) {
			Debug::log(
			    ERR,
			    "Was unable to remove child node %p from parent %p. Child likely has "
			    "a false parent pointer.",
			    child,
			    parent
			);

			errorNotif();
			return nullptr;
		}

		group.group_focused = false;
		if (group.children.size() == 1) {
			group.focused_child = group.children.front();
		}

		auto child_size_ratio = child->size_ratio;
		if (child != this) {
			parent->layout->nodes.remove(*child);
		} else {
			child->parent = nullptr;
		}

		if (!group.children.empty()) {
			auto child_count = group.children.size();
			if (std::find(group.children.begin(), group.children.end(), this) != group.children.end()) {
				child_count -= 1;
			}

			auto splitmod = -((1.0 - child_size_ratio) / child_count);

			for (auto* child: group.children) {
				child->size_ratio += splitmod;
			}

			break;
		}
	}

	return parent;
}

Hy3Node* Hy3Node::intoGroup(Hy3GroupLayout layout) {
	this->layout->nodes.push_back({
	    .parent = this,
	    .data = layout,
	    .workspace_id = this->workspace_id,
	    .layout = this->layout,
	});

	auto* node = &this->layout->nodes.back();
	swapData(*this, *node);

	this->data = layout;
	this->data.as_group.children.push_back(node);
	this->data.as_group.group_focused = false;
	this->data.as_group.focused_child = node;
	this->recalcSizePosRecursive();
	this->updateTabBarRecursive();

	return node;
}

bool Hy3Node::swallowGroups(Hy3Node* into) {
	if (into == nullptr || into->data.type != Hy3NodeType::Group
	    || into->data.as_group.children.size() != 1)
		return false;

	auto* child = into->data.as_group.children.front();

	// a lot of segfaulting happens once the assumption that the root node is a
	// group is wrong.
	if (into->parent == nullptr && child->data.type != Hy3NodeType::Group) return false;

	Debug::log(LOG, "Swallowing %p into %p", child, into);
	Hy3Node::swapData(*into, *child);
	into->layout->nodes.remove(*child);

	return true;
}

void Hy3Node::swapData(Hy3Node& a, Hy3Node& b) {
	Hy3NodeData aData = std::move(a.data);
	a.data = std::move(b.data);
	b.data = std::move(aData);

	if (a.data.type == Hy3NodeType::Group) {
		for (auto child: a.data.as_group.children) {
			child->parent = &a;
		}
	}

	if (b.data.type == Hy3NodeType::Group) {
		for (auto child: b.data.as_group.children) {
			child->parent = &b;
		}
	}
}
