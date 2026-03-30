#include <cstdint>
#include <sstream>
#include <stdexcept>

#include <bits/ranges_util.h>
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/desktop/state/FocusState.hpp>
#include <hyprland/src/config/ConfigManager.hpp>
#include <hyprland/src/defines.hpp>
#include <hyprland/src/plugins/PluginAPI.hpp>
#include <hyprutils/math/Box.hpp>

#include "log.hpp"
#include "Hy3Layout.hpp"
#include "Hy3Node.hpp"
#include "globals.hpp"

using Desktop::View::CWindow;

const float MIN_RATIO = 0.0f;

Hy3GroupNode::Hy3GroupNode(Hy3GroupLayout layout): layout(layout) {
	if (!isTab()) {
		this->previous_nontab_layout = layout;
	}
}

bool Hy3Node::is_root() { return is_group() && as_group().layout == Hy3GroupLayout::Root; }
bool Hy3Node::is_root_group() { return !is_root() && parent->is_root(); }

Hy3RootNode::Hy3RootNode(Hy3Layout* layout)
    : Hy3GroupNode(Hy3GroupLayout::Root), algo(layout) {}

Hy3RootNode* Hy3Node::root() {
	auto* node = this;
	while (!node->is_root() && node->parent.get() != nullptr) {
		node = node->parent.get();
	}
	return dynamic_cast<Hy3RootNode*>(node);
}

Hy3Layout* Hy3Node::layout() {
	auto* r = root();
	return r ? r->algo : nullptr;
}

void Hy3Node::assertNotRoot() {
	if (this->is_root()) {
		hy3_log(ERR, "assertNotRoot failed: node {:x} is root", (uintptr_t) this);
		throw std::runtime_error("operation called on root node");
	}
}

bool Hy3GroupNode::hasChild(Hy3Node& node) {
	for (auto& child: this->children) {
		if (child.get() == &node) return true;

		if (child->is_group()) {
			if (child->as_group().hasChild(node)) return true;
		}
	}

	return false;
}

auto Hy3GroupNode::findChild(Hy3Node& child) -> std::list<UP<Hy3Node>>::iterator {
	for (auto it = children.begin(); it != children.end(); ++it) {
		if (it->get() == &child) return it;
	}
	return children.end();
}

void Hy3GroupNode::insertChild(std::list<UP<Hy3Node>>::iterator pos, UP<Hy3Node> child) {
	child->parent = this->self;
	if (focused_child == nullptr) focused_child = child.get();
	children.insert(pos, std::move(child));
	if (ephemeral == Ephemeral::Staged && children.size() >= 2)
		ephemeral = Ephemeral::Active;
}

void Hy3GroupNode::insertChild(UP<Hy3Node> child) {
	insertChild(children.end(), std::move(child));
}

UP<Hy3Node> Hy3GroupNode::extractChildRaw(std::list<UP<Hy3Node>>::iterator it) {
	auto* child_ptr = it->get();

	// Fix focused_child if we're extracting it
	if (focused_child == child_ptr) {
		if (children.size() <= 1) {
			focused_child = nullptr;
		} else if (it == children.begin()) {
			focused_child = std::next(it)->get();
		} else {
			focused_child = std::prev(it)->get();
		}
	}

	auto up = std::move(*it);
	children.erase(it);
	up->parent.reset();
	return up;
}

UP<Hy3Node> Hy3GroupNode::extractChildRaw(Hy3Node& child) {
	auto it = findChild(child);
	if (it == children.end()) return nullptr;
	return extractChildRaw(it);
}

UP<Hy3Node> Hy3GroupNode::extractChild(Hy3Node& child) {
	if (!child.is_root()) {
		auto& actor = child.getExpandActor();
		if (actor.is_group()) {
			actor.as_group().collapseExpansions();
		}
	}

	auto extracted = extractChildRaw(child);
	if (!extracted) return nullptr;

	group_focused = false;

	if (!children.empty()) {
		auto child_count = children.size();
		auto splitmod = -((1.0 - extracted->size_ratio) / child_count);

		for (auto& c: children) {
			c->size_ratio += splitmod;
		}
	}

	extracted->size_ratio = 1.0;
	return extracted;
}

UP<Hy3Node> Hy3GroupNode::replaceChild(std::list<UP<Hy3Node>>::iterator it, UP<Hy3Node> replacement) {
	replacement->parent = this->self;
	replacement->size_ratio = (*it)->size_ratio;
	if (focused_child == it->get()) focused_child = replacement.get();
	auto old = std::exchange(*it, std::move(replacement));
	old->size_ratio = 1.0;
	old->parent.reset();
	return old;
}

void Hy3GroupNode::collapseExpansions() {
	if (this->expand_focused == ExpandFocusType::NotExpanded) return;
	this->expand_focused = ExpandFocusType::NotExpanded;

	Hy3Node* node = this->focused_child;

	while (node->is_group() && node->as_group().expand_focused == ExpandFocusType::Stack) {
		auto& group = node->as_group();
		group.expand_focused = ExpandFocusType::NotExpanded;
		node = group.focused_child;
	}
}

void Hy3GroupNode::setLayout(Hy3GroupLayout layout) {
	if (layout == Hy3GroupLayout::Root) return; // root layout is immutable
	this->layout = layout;

	if (!isTab()) {
		this->previous_nontab_layout = layout;
	}
}

void Hy3GroupNode::setEphemeral(GroupEphemeralityOption ephemeral) {
	switch (ephemeral) {
	case GroupEphemeralityOption::Standard: this->ephemeral = Ephemeral::Off; break;
	case GroupEphemeralityOption::ForceEphemeral:
		this->ephemeral = this->children.size() == 1 ? Ephemeral::Staged : Ephemeral::Active;
		break;
	case GroupEphemeralityOption::Ephemeral:
		// no change
		break;
	}
}

bool Hy3Node::valid() const {
	if (dynamic_cast<const Hy3GroupNode*>(this)) return true;
	if (auto* t = dynamic_cast<const Hy3TargetNode*>(this)) return !t->target.expired();
	return false;
}

Hy3NodeType Hy3Node::type() const {
	if (dynamic_cast<const Hy3GroupNode*>(this)) return Hy3NodeType::Group;
	if (dynamic_cast<const Hy3TargetNode*>(this)) return Hy3NodeType::Target;
	throw std::runtime_error("Attempted to get Hy3NodeType of uninitialized Hy3Node data");
}

bool Hy3Node::is_group() const { return dynamic_cast<const Hy3GroupNode*>(this) != nullptr; }

bool Hy3Node::is_target() const { return dynamic_cast<const Hy3TargetNode*>(this) != nullptr; }

Hy3GroupNode& Hy3Node::as_group() {
	auto* gn = dynamic_cast<Hy3GroupNode*>(this);
	if (!gn) throw std::runtime_error("Attempted to get group value of a non-group Hy3Node");
	return *gn;
}

SP<Layout::ITarget> Hy3Node::as_target() {
	auto* tn = dynamic_cast<Hy3TargetNode*>(this);
	if (!tn) throw std::runtime_error("Attempted to get target value of a non-target Hy3Node");
	if (tn->target.expired()) throw std::runtime_error("Attempted to upgrade an expired Hy3Node target");
	return tn->target.lock();
}

PHLWINDOW Hy3Node::as_window() {
	return this->as_target()->window();
}

UP<Hy3Node> Hy3Node::create(SP<Layout::ITarget> target) {
	auto up = makeUnique<Hy3TargetNode>();
	up->target = target;
	UP<Hy3Node> result = std::move(up);
	result->self = WP<Hy3Node>(result);
	return result;
}

UP<Hy3Node> Hy3Node::create(Hy3GroupLayout group_layout) {
	auto up = makeUnique<Hy3GroupNode>(group_layout);
	UP<Hy3Node> result = std::move(up);
	result->self = WP<Hy3Node>(result);
	return result;
}

bool Hy3Node::operator==(const Hy3Node& rhs) const { return this == &rhs; }

void Hy3Node::focus(bool warp, Desktop::eFocusReason reason) {
	this->markFocused();

	g_pInputManager->unconstrainMouse();

	switch (this->type()) {
	case Hy3NodeType::Target: {
		auto window = this->as_window();
		window->setHidden(false);
		Desktop::focusState()->fullWindowFocus(window, reason);
		if (warp) Hy3Layout::warpCursorToBox(window->m_position, window->m_size);
		break;
	}
	case Hy3NodeType::Group: {
		Desktop::focusState()->resetWindowFocus();
		for (auto& window: this->windows()) {
			g_pCompositor->changeWindowZOrder(window.m_self.lock(), true);
		}

		if (warp) Hy3Layout::warpCursorToBox(this->visualBox.pos(), this->visualBox.size());
		break;
	}
	}
}

void markGroupFocusedRecursive(Hy3GroupNode& group) {
	group.group_focused = true;
	for (auto& child: group.children) {
		if (child->is_group()) markGroupFocusedRecursive(child->as_group());
	}
}

void Hy3Node::markFocused() {
	auto* root = this->root();

	// update focus
	if (this->is_group()) {
		markGroupFocusedRecursive(this->as_group());
	}

	for (auto& ancestor: this->ancestors()) {
		auto& group = ancestor.parent->as_group();
		group.focused_child = &ancestor;
		group.group_focused = false;
	}

	root->updateDecos();
}

Hy3Node& Hy3Node::getFocusedNode(bool ignore_group_focus, bool stop_at_expanded) {
	switch (this->type()) {
	case Hy3NodeType::Target: return *this;
	case Hy3NodeType::Group: {
		auto& group = this->as_group();

		if (group.focused_child == nullptr || (!ignore_group_focus && group.group_focused)
		    || (stop_at_expanded && group.expand_focused != ExpandFocusType::NotExpanded))
		{
			return *this;
		} else {
			return group.focused_child->getFocusedNode(ignore_group_focus, stop_at_expanded);
		}
	}
	}
	throw std::runtime_error("getFocusedNode: invalid node type");
}

bool Hy3Node::isIndirectlyFocused() {
	for (auto& node: this->ancestors()) {
		auto& group = node.parent->as_group();
		if (!group.group_focused && group.focused_child != &node) return false;
	}

	return true;
}

Hy3Node& Hy3Node::getExpandActor() {
	for (auto& node: this->ancestors()) {
		if (node.parent->as_group().expand_focused == ExpandFocusType::NotExpanded)
			return node;
	}
	hy3_log(ERR, "getExpandActor: no non-expanded ancestor found for node {:x}", (uintptr_t) this);
	return *this;
}

Hy3Node& Hy3Node::getPlacementActor() {
	for (auto& node: this->getExpandActor().ancestors()) {
		if (!node.parent->as_group().locked)
			return node;
	}
	hy3_log(ERR, "getPlacementActor: no non-locked ancestor found for node {:x}", (uintptr_t) this);
	return *this;
}

void Hy3Node::recalcSizePosRecursive(CBox offsets, bool no_animation) {
	// clang-format off
	static const auto p_gaps_in = ConfigValue<Hyprlang::CUSTOMTYPE, CCssGapData>("general:gaps_in");
	static const auto tab_bar_height = ConfigValue<Hyprlang::INT>("plugin:hy3:tabs:height");
	static const auto tab_bar_padding = ConfigValue<Hyprlang::INT>("plugin:hy3:tabs:padding");
	static const auto group_inset = ConfigValue<Hyprlang::INT>("plugin:hy3:group_inset");
	// clang-format on

	this->logicalBox = CBox(
	    this->visualBox.x - offsets.x, this->visualBox.y - offsets.y,
	    this->visualBox.w + offsets.x + offsets.w, this->visualBox.h + offsets.y + offsets.h
	);

	// Keep in sync with WindowTarget::updatePos
	if (this->is_target()) {
		this->as_window()->setHidden(this->hidden);
		this->as_target()->setPositionGlobal({.logicalBox = this->logicalBox, .visualBox = this->visualBox});
		// warp on hidden fixes bounding boxes for the tab click handler
		if (no_animation || this->hidden) this->as_target()->warpPositionSize();
		return;
	}

	auto tpos = this->visualBox.pos();
	auto tsize = this->visualBox.size();

	auto& group = this->as_group();
	auto workspace_rule = g_pConfigManager->getWorkspaceRuleFor(this->layout()->workspace());
	auto gaps_in = workspace_rule.gapsIn.value_or(*p_gaps_in);

	auto expand_focused = group.expand_focused != ExpandFocusType::NotExpanded;
	bool directly_contains_expanded =
	    expand_focused
	    && (group.focused_child->is_target()
	        || group.focused_child->as_group().expand_focused == ExpandFocusType::NotExpanded);

	auto child_count = group.children.size();

	// Latch/expanded: expanded node covers full parent area with parent offsets
	if (group.expand_focused == ExpandFocusType::Latch) {
		auto* expanded_node = group.focused_child;

		while (expanded_node != nullptr && expanded_node->is_group()
		       && expanded_node->as_group().expand_focused != ExpandFocusType::NotExpanded)
		{
			expanded_node = expanded_node->as_group().focused_child;
		}

		if (expanded_node == nullptr) {
			hy3_log(
			    ERR,
			    "recalcSizePosRecursive: unable to find expansion target of latch node {:x}",
			    (uintptr_t) this
			);
			errorNotif();
			return;
		}

		expanded_node->visualBox = CBox(tpos, tsize);
		expanded_node->setHidden(this->hidden);

		expanded_node->recalcSizePosRecursive(offsets, no_animation);
	}

	// Compute constraint for splits: total visible space minus inter-child gaps
	double inter_gap = 0.0;
	double constraint = 0.0;

	switch (group.layout) {
	case Hy3GroupLayout::SplitH:
		inter_gap = gaps_in.m_left + gaps_in.m_right;
		constraint = tsize.x - (child_count > 1 ? (child_count - 1) * inter_gap : 0);
		break;
	case Hy3GroupLayout::SplitV:
		inter_gap = gaps_in.m_top + gaps_in.m_bottom;
		constraint = tsize.y - (child_count > 1 ? (child_count - 1) * inter_gap : 0);
		break;
	case Hy3GroupLayout::Tabbed:
	case Hy3GroupLayout::Root: break;
	}

	double ratio_mul =
	    group.isSplit() ? child_count <= 0 ? 0 : constraint / child_count : 0;

	double offset = 0;

	for (auto& child: group.children) {
		bool is_first = (child.get() == group.children.front().get());
		bool is_last = (child.get() == group.children.back().get());
		int inset = is_first && is_last && !this->is_root_group() ? *group_inset : 0;

		if (directly_contains_expanded && child.get() == group.focused_child) {
			// Advance offset past this child's visible share
			if (group.isSplit()) {
				offset += child->size_ratio * ratio_mul - inset;
				if (!is_last) offset += inter_gap;
			}
			continue;
		}

		CBox child_offsets;

		switch (group.layout) {
		case Hy3GroupLayout::SplitH: {
			double child_w = child->size_ratio * ratio_mul;

			child->visualBox = CBox(tpos.x + offset, tpos.y, child_w - inset, tsize.y);
			child->hidden = this->hidden || expand_focused;

			child_offsets.x = is_first ? offsets.x : gaps_in.m_left;
			child_offsets.w = (is_last ? offsets.w : gaps_in.m_right) + inset;
			child_offsets.y = offsets.y;
			child_offsets.h = offsets.h;

			offset += child_w;
			if (!is_last) offset += inter_gap;

			child->recalcSizePosRecursive(child_offsets, no_animation);
			break;
		}
		case Hy3GroupLayout::SplitV: {
			double child_h = child->size_ratio * ratio_mul;

			child->visualBox = CBox(tpos.x, tpos.y + offset, tsize.x, child_h - inset);
			child->hidden = this->hidden || expand_focused;

			child_offsets.y = (is_first ? offsets.y : gaps_in.m_top) + inset;
			child_offsets.h = is_last ? offsets.h : gaps_in.m_bottom;
			child_offsets.x = offsets.x;
			child_offsets.w = offsets.w;

			offset += child_h;
			if (!is_last) offset += inter_gap;

			child->recalcSizePosRecursive(child_offsets, no_animation);
			break;
		}
		case Hy3GroupLayout::Tabbed: {
			double tab_offset = (double)*tab_bar_height + (double)*tab_bar_padding;

			child->visualBox = CBox(tpos.x, tpos.y + tab_offset, tsize.x, tsize.y - tab_offset);
			child->hidden = this->hidden || expand_focused || group.focused_child != child.get();

			// Tab bar makes child non-edge on top
			child_offsets.x = offsets.x;
			child_offsets.y = offsets.y + tab_offset;
			child_offsets.w = offsets.w;
			child_offsets.h = offsets.h;

			child->recalcSizePosRecursive(child_offsets, no_animation);
			break;
		}
		case Hy3GroupLayout::Root: {
			child->visualBox = CBox(tpos, tsize);
			child->hidden = this->hidden;
			child->recalcSizePosRecursive(offsets, no_animation);
			break;
		}
		}
	}

	this->updateTabBar(no_animation);
}

// Find the visible window with the highest z-order in this subtree.
static CWindow* findTopVisibleWindow(Hy3Node& node) {
	CWindow* result = nullptr;
	auto& compositor_windows = g_pCompositor->m_windows;
	auto it = compositor_windows.begin();
	for (auto& window: node.windows(true)) {
		for (auto search = it; search != compositor_windows.end(); ++search) {
			if (search->get() == &window) {
				result = &window;
				it = search;
				break;
			}
		}
	}
	return result;
}

void Hy3Node::updateTabBar(bool no_animation) {
	if (this->type() == Hy3NodeType::Group) {
		auto& group = this->as_group();

		if (group.isTab()) {
			if (!group.tab_bar) group.tab_bar = Hy3TabGroup::create(*this);
			group.tab_bar->updateWithGroup(*this, no_animation);

			auto top_window = findTopVisibleWindow(*this);
			group.tab_bar->target_window = top_window ? top_window->m_self.lock() : nullptr;
			if (top_window != nullptr) group.tab_bar->workspace = top_window->m_workspace;
		} else if (group.tab_bar) {
			group.tab_bar.release();
		}
	}
}

void Hy3Node::updateTabBarRecursive() {
	for (auto& node: this->ancestors()) {
		node.updateTabBar();
	}
}

void Hy3Node::updateDecos() {
	switch (this->type()) {
	case Hy3NodeType::Target:
		this->as_window()->updateDecorationValues();
		break;
	case Hy3NodeType::Group:
		for (auto& child: this->as_group().children) {
			child->updateDecos();
		}

		this->updateTabBar();
	}
}

std::string Hy3Node::getTitle() {
	switch (this->type()) {
	case Hy3NodeType::Target: return this->as_window()->m_title;
	case Hy3NodeType::Group:
		std::string title;
		auto& group = this->as_group();

		switch (group.layout) {
		case Hy3GroupLayout::Root: title = "[R] "; break;
		case Hy3GroupLayout::SplitH: title = "[H] "; break;
		case Hy3GroupLayout::SplitV: title = "[V] "; break;
		case Hy3GroupLayout::Tabbed: title = "[T] "; break;
		}

		if (group.focused_child == nullptr) {
			title += "Group";
		} else {
			title += group.focused_child->getTitle();
		}

		return title;
	}

	return "";
}

bool Hy3Node::isUrgent() {
	for (auto& window: this->windows()) {
		if (window.m_isUrgent) return true;
	}
	return false;
}

void Hy3Node::setHidden(bool hidden) {
	this->hidden = hidden;

	if (this->is_group()) {
		for (auto& child: this->as_group().children) {
			child->setHidden(hidden);
		}
	}
}

Hy3Node* Hy3Node::findNodeForTabGroup(Hy3TabGroup& tab_group) {
	if (this->is_group()) {
		if (this->hidden) return nullptr;
		auto& group = this->as_group();

		if (group.isTab() && group.tab_bar.get() == &tab_group) {
			return this;
		}

		for (auto& node: group.children) {
			auto* r = node->findNodeForTabGroup(tab_group);
			if (r != nullptr) return r;
		}
	} else return nullptr;

	return nullptr;
}

std::generator<Hy3Node&> Hy3Node::ancestors() {
	auto* node = this;
	while (!node->is_root()) {
		co_yield *node;
		node = node->parent.get();
	}
}

std::generator<CWindow&> Hy3Node::windows(bool visibleOnly) {
	if (this->is_target()) {
		co_yield *this->as_window();
	} else {
		auto& group = this->as_group();
		if (visibleOnly
		    && (group.isTab()
		        || group.expand_focused != ExpandFocusType::NotExpanded))
		{
			if (group.focused_child != nullptr) {
				for (auto& window: group.focused_child->windows(true)) {
					co_yield window;
				}
			}
		} else {
			for (auto& child: group.children) {
				for (auto& window: child->windows(visibleOnly)) {
					co_yield window;
				}
			}
		}
	}
}


std::string Hy3Node::debugNode() {
	std::stringstream buf;
	std::string addr = "0x" + std::to_string((size_t) this);
	switch (this->type()) {
	case Hy3NodeType::Target:
		buf << "window(" << this << " of " << this->parent.get() << ") [hypr " << this->as_window().get() << "] size ratio: " << this->size_ratio;
		break;
	case Hy3NodeType::Group:
		buf << "group(" << this << " of " << this->parent.get() << ") [";

		auto& group = this->as_group();
		switch (group.layout) {
		case Hy3GroupLayout::Root: {
			auto* l = this->layout();
			auto ws = l ? l->workspace() : nullptr;
			buf << "root " << (ws ? ws->m_id : -1);
			break;
		}
		case Hy3GroupLayout::SplitH: buf << "splith"; break;
		case Hy3GroupLayout::SplitV: buf << "splitv"; break;
		case Hy3GroupLayout::Tabbed: buf << "tabs"; break;
		}

		buf << "] size ratio: ";
		buf << this->size_ratio;

		if (group.expand_focused != ExpandFocusType::NotExpanded) {
			buf << ", has-expanded";
		}

		if (group.ephemeral != Ephemeral::Off) {
			buf << ", ephemeral" << (group.ephemeral == Ephemeral::Staged ? "(staged)" : "");
		}

		if (group.containment) {
			buf << ", containment";
		}

		for (auto& child: group.children) {
			buf << "\n|-";
			if (!child) {
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

static bool shouldCollapseNode(Hy3Node* node, CollapsePolicy policy) {
	if (node->is_root()) return false;
	auto& group = node->as_group();
	if (group.children.size() != 1) return false;
	auto* child = group.children.front().get();
	if (node->is_root_group() && !child->is_group()) return false;
	if (policy == CollapsePolicy::SingleNodeGroups || group.ephemeral == Ephemeral::Active) return true;

	if (policy == CollapsePolicy::EmptySplits && group.isSplit()) return true;

	if (child->is_group()) {
		auto& cgroup = child->as_group();
		if (group.isSplit() && cgroup.isSplit()) return true;
		if (cgroup.children.size() == 1 && group.isTab() && cgroup.isTab()) return true;
	}

	return false;
}

static void collapseSingleParentInternal(Hy3Node* into) {
	auto* parent = into->parent.get();
	auto& parentGroup = parent->as_group();
	auto it = parentGroup.findChild(*into);
	auto& intoGroup = into->as_group();

	hy3_log(
	    TRACE,
			"collapsing {:x} in favor of {:x}",
			(uintptr_t) into,
	    (uintptr_t) intoGroup.children.front().get()
	);

	auto childUp = intoGroup.extractChildRaw(intoGroup.children.begin());
	auto* child = childUp.get();
	auto old = parentGroup.replaceChild(it, std::move(childUp));

	// HACK: steal titlebar from parent if we have a new node, prevents visual issues if rewrapped
	if (child->is_group() && old->as_group().isTab() && child->as_group().isTab()) {
		auto& n = child->as_group().tab_bar;
		auto& o = old->as_group().tab_bar;
		if (n->bar.entries.empty() || n->bar.entries.front().vertical_pos->value() == 1) n = std::move(o);
	}
}

Hy3Node* Hy3Node::collapseParents(CollapsePolicy policy) {
	if (this->is_root()) return this;

	if (!this->is_group()) {
		this->parent->collapseParents(CollapsePolicy::InvalidOnly);
		return this;
	}

	auto& group = this->as_group();

	if (group.children.empty()) {
		auto* p = this->parent.get();
		Hy3Node* merged = nullptr;
		p->extractAndMerge(*this, &merged, CollapsePolicy::InvalidOnly);
		return merged;
	}

	hy3_log(LOG, "ShouldCollapse {:x} policy {}: {}", (uintptr_t)this, (int)policy, shouldCollapseNode(this, policy));
	if (shouldCollapseNode(this, policy)) {
		auto* parent_node = this->parent.get();
		collapseSingleParentInternal(this);
		return parent_node->collapseParents(CollapsePolicy::InvalidOnly);
	} else {
		this->parent->collapseParents(CollapsePolicy::InvalidOnly);
	}

	return this;
}

UP<Hy3Node> Hy3Node::extractAndMerge(
    Hy3Node& child,
    Hy3Node** out_parent,
    CollapsePolicy policy
) {
	hy3_log(
	    TRACE,
	    "extractAndMerge: extracting {:x} from {:x}",
	    (uintptr_t) &child,
	    (uintptr_t) this
	);

	auto& group = this->as_group();
	auto extracted = group.extractChild(child);
	if (!extracted) {
		hy3_log(
		    ERR,
		    "unable to extract child node {:x} from parent node {:x}",
		    (uintptr_t) &child,
		    (uintptr_t) this
		);
		errorNotif();
		return nullptr;
	}

	auto* merged = this->collapseParents(policy);
	if (out_parent != nullptr) *out_parent = merged;

	return extracted;
}

void Hy3Node::insertAndMerge(
    std::list<UP<Hy3Node>>::iterator pos,
    UP<Hy3Node> child,
    CollapsePolicy policy
) {
	this->as_group().insertChild(pos, std::move(child));
	this->collapseParents(policy);
}

void Hy3Node::insertAndMerge(UP<Hy3Node> child, CollapsePolicy policy) {
	this->as_group().insertChild(std::move(child));
	this->collapseParents(policy);
}

void Hy3Node::wrap(Hy3GroupLayout layout, GroupEphemeralityOption ephemeral, bool change) {
	auto& parentGroup = this->parent->as_group();
	if (change && !this->parent->is_root() && parentGroup.children.size() == 1) {
		parentGroup.setLayout(layout);
		parentGroup.setEphemeral(ephemeral);
		this->layout()->recalcGeometry();
		this->parent->updateTabBarRecursive();
		return;
	}

	auto it = parentGroup.findChild(*this);

	auto group_up = Hy3Node::create(layout);
	auto& group_node = *group_up;

	auto this_up = parentGroup.replaceChild(it, std::move(group_up));

	auto& group = group_node.as_group();
	group.insertChild(std::move(this_up));
	group.group_focused = false;
	group.focused_child = this;
	if (ephemeral == GroupEphemeralityOption::Ephemeral
	    || ephemeral == GroupEphemeralityOption::ForceEphemeral)
		group.setEphemeral(GroupEphemeralityOption::ForceEphemeral);

	this->layout()->recalcGeometry();
	group_node.updateTabBarRecursive();
}


Hy3Node* getOuterChild(Hy3GroupNode& group, ShiftDirection direction) {
	switch (direction) {
	case ShiftDirection::Left:
	case ShiftDirection::Up: return group.children.front().get(); break;
	case ShiftDirection::Right:
	case ShiftDirection::Down: return group.children.back().get(); break;
	default: throw std::runtime_error("invalid ShiftDirection");
	}
}

Hy3Node* Hy3Node::getImmediateSibling(ShiftDirection direction) {
	auto& group = this->parent->as_group();

	auto iter = group.findChild(*this);
	if (iter == group.children.end()) return nullptr;

	switch (direction) {
	case ShiftDirection::Left:
	case ShiftDirection::Up:
		if (iter == group.children.begin()) return nullptr;
		return std::prev(iter)->get();
	case ShiftDirection::Right:
	case ShiftDirection::Down: {
		auto next = std::next(iter);
		if (next == group.children.end()) return nullptr;
		return next->get();
	}
	default: throw std::runtime_error("invalid ShiftDirection");
	}
}


Axis getAxis(Hy3GroupLayout layout) {
	switch (layout) {
	case Hy3GroupLayout::SplitH: return Axis::Horizontal;
	case Hy3GroupLayout::SplitV: return Axis::Vertical;
	default: return Axis::None;
	}
}

Axis getAxis(ShiftDirection direction) {
	switch (direction) {
	case ShiftDirection::Left:
	case ShiftDirection::Right: return Axis::Horizontal;
	case ShiftDirection::Down:
	case ShiftDirection::Up: return Axis::Vertical;
	default: return Axis::None;
	}
}

Hy3Node* Hy3Node::findNeighbor(ShiftDirection direction) {
	for (auto& node: this->ancestors()) {
		auto& parent_group = node.parent->as_group();

		if (parent_group.isSplit()
		    && getAxis(parent_group.layout) == getAxis(direction)
		    && getOuterChild(parent_group, direction) != &node)
		{
			return node.getImmediateSibling(direction);
		}
	}

	return nullptr;
}

int directionToIteratorIncrement(ShiftDirection direction) {
	switch (direction) {
	case ShiftDirection::Left:
	case ShiftDirection::Up: return -1;
	case ShiftDirection::Right:
	case ShiftDirection::Down: return 1;
	default: throw std::runtime_error("Unknown ShiftDirection");
	}
}

void Hy3Node::resize(ShiftDirection direction, double delta, bool no_animation) {
	auto* parent_node = this->parent.get();
	auto& containing_group = parent_node->as_group();

	if (containing_group.isSplit()
	    && getAxis(direction) == getAxis(containing_group.layout))
	{
		double parent_size =
		    getAxis(direction) == Axis::Horizontal ? parent_node->visualBox.w : parent_node->visualBox.h;
		auto ratio_mod = delta * (float) containing_group.children.size() / parent_size;

		const auto end_of_children = containing_group.children.end();
		auto iter = containing_group.findChild(*this);

		if (iter != end_of_children) {
			const auto outermost_node_in_group = getOuterChild(containing_group, direction);
			if (this != outermost_node_in_group) {
				auto inc = directionToIteratorIncrement(direction);
				iter = std::next(iter, inc);
				ratio_mod *= inc;
			}

			if (iter != end_of_children) {
				auto* neighbor = iter->get();
				auto requested_size_ratio = this->size_ratio + ratio_mod;
				auto requested_neighbor_size_ratio = neighbor->size_ratio - ratio_mod;

				if (requested_size_ratio >= MIN_RATIO && requested_neighbor_size_ratio >= MIN_RATIO) {
					this->size_ratio = requested_size_ratio;
					neighbor->size_ratio = requested_neighbor_size_ratio;

					this->layout()->recalcGeometry(no_animation);
				}
			}
		}
	}
}

