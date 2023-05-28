#include "globals.hpp"
#include "TabGroup.hpp"
#include "Hy3Layout.hpp"

#include <hyprland/src/Compositor.hpp>
#include <cairo/cairo.h>

Hy3TabBarEntry::Hy3TabBarEntry(Hy3TabBar& tab_bar, Hy3Node& node): tab_bar(tab_bar), node(node) {
	this->offset.create(AVARTYPE_FLOAT, -1.0f, g_pConfigManager->getAnimationPropertyConfig("windowsMove"), nullptr, AVARDAMAGE_NONE);
	this->width.create(AVARTYPE_FLOAT, -1.0f, g_pConfigManager->getAnimationPropertyConfig("windowsMove"), nullptr, AVARDAMAGE_NONE);

	this->offset.registerVar();
	this->width.registerVar();

	this->window_title = node.getTitle();
	this->urgent = node.isUrgent();
}

bool Hy3TabBarEntry::operator==(const Hy3Node& node) const {
	return this->node == node;
}

void Hy3TabBarEntry::prepareTexture(float scale, Vector2D size) {
	static const auto* rounding_setting = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:hy3:tabs:rounding")->intValue;
	auto rounding = *rounding_setting;

	auto width = size.x * scale;
	auto height = size.y * scale;

	if (this->needs_redraw
			|| this->texture.m_iTexID == 0
			|| this->last_render_rounding == rounding
			|| this->last_render_scale == scale
			|| this->last_render_size == size
			|| true // todo
	) {
		this->needs_redraw = false;
		this->last_render_rounding = rounding;
		this->last_render_scale = scale;
		this->last_render_size = size;

		auto cairo_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
		auto cairo = cairo_create(cairo_surface);

		// clear pixmap
		cairo_save(cairo);
		cairo_set_operator(cairo, CAIRO_OPERATOR_CLEAR);
		cairo_paint(cairo);
		cairo_restore(cairo);

		// set brush
		if (this->focused) {
			cairo_set_source_rgba(cairo, 0.2, 0.7, 1.0, 1.0);
		} else {
			cairo_set_source_rgba(cairo, 0.5, 0.5, 0.5, 1.0);
		}

		// outline bar shape
		cairo_move_to(cairo, 0, rounding);
		cairo_arc(cairo, rounding, rounding, rounding, -180.0 * (M_PI / 180.0), -90.0 * (M_PI / 180.0));
		cairo_line_to(cairo, width - rounding, 0);
		cairo_arc(cairo, width - rounding, rounding, rounding, -90.0 * (M_PI / 180.0), 0.0);
		cairo_line_to(cairo, width, height - rounding);
		cairo_arc(cairo, width - rounding, height - rounding, rounding, 0.0, 90.0 * (M_PI / 180.0));
		cairo_line_to(cairo, rounding, height);
		cairo_arc(cairo, rounding, height - rounding, rounding, -270.0 * (M_PI / 180.0), -180.0 * (M_PI / 180.0));
		cairo_close_path(cairo);

		// draw
		cairo_fill(cairo);

		auto data = cairo_image_surface_get_data(cairo_surface);
		this->texture.allocate();

		glBindTexture(GL_TEXTURE_2D, this->texture.m_iTexID);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	#ifdef GLES32
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
	#endif

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

		cairo_destroy(cairo);
		cairo_surface_destroy(cairo_surface);
	} else {
		glBindTexture(GL_TEXTURE_2D, this->texture.m_iTexID);
	}
}


Hy3TabBar::Hy3TabBar() {
	this->vertical_pos.create(AVARTYPE_FLOAT, 1.0f, g_pConfigManager->getAnimationPropertyConfig("windowsMove"), nullptr, AVARDAMAGE_NONE);
	this->fade_opacity.create(AVARTYPE_FLOAT, 1.0f, g_pConfigManager->getAnimationPropertyConfig("windowsMove"), nullptr, AVARDAMAGE_NONE);
	this->focus_start.create(AVARTYPE_FLOAT, 0.0f, g_pConfigManager->getAnimationPropertyConfig("windowsIn"), nullptr, AVARDAMAGE_NONE);
	this->focus_end.create(AVARTYPE_FLOAT, 1.0f, g_pConfigManager->getAnimationPropertyConfig("windowsIn"), nullptr, AVARDAMAGE_NONE);
	this->vertical_pos.registerVar();
	this->fade_opacity.registerVar();
	this->focus_start.registerVar();
	this->focus_end.registerVar();

	this->vertical_pos = 0.0;
	this->fade_opacity = 1.0;
}

void Hy3TabBar::focusNode(Hy3Node* node) {
	this->focused_node = node;

	if (this->focused_node == nullptr) {
		this->focus_start = 0.0;
		this->focus_end = 1.0;
	} else {
		auto entry = std::find(this->entries.begin(), this->entries.end(), *node);

		if (entry != this->entries.end()) {
			this->focus_start = entry->offset.goalf();
			this->focus_end = entry->offset.goalf() + entry->width.goalf();
		}
	}
}

void Hy3TabBar::updateNodeList(std::list<Hy3Node*>& nodes) {
	std::list<Hy3TabBarEntry> removed_entries;

	auto entry = this->entries.begin();
	auto node = nodes.begin();

	// move any out of order entries to removed_entries
	while (node != nodes.end()) {
		while (true) {
			if (entry == this->entries.end()) goto exitloop;
			if (*entry == **node) break;
			removed_entries.splice(removed_entries.end(), this->entries, entry++);
		}

		node = std::next(node);
		entry = std::next(entry);
	}

 exitloop:

	// move any extra entries to removed_entries
	removed_entries.splice(removed_entries.end(), this->entries, entry, this->entries.end());

	entry = this->entries.begin();
	node = nodes.begin();

	// add missing entries, taking first from removed_entries
	while (node != nodes.end()) {
		if (entry == this->entries.end() || *entry != **node) {
			auto moved = std::find(removed_entries.begin(), removed_entries.end(), **node);
			if (moved != removed_entries.end()) {
				this->entries.splice(entry, removed_entries, moved);
				entry = moved;
			} else {
				entry = this->entries.emplace(entry, *this, **node);
			}
		}

		// set stats from node data
		auto* root = *node;
		while (root->parent != nullptr) root = root->parent;
		auto* focused = root->getFocusedNode();
		entry->focused = focused == *node;
		entry->urgent = (*node)->isUrgent();

		node = std::next(node);
		if (entry != this->entries.end()) entry = std::next(entry);
	}

	// initiate remove animations for any removed entries
	for (auto& entry: removed_entries) {
		// TODO: working entry remove anim
		entry.width = 0.0;
	}
}

void Hy3TabBar::updateAnimations(bool warp) {
	int active_entries = 0;
	for (auto& entry: this->entries) {
		if (entry.width.goalf() != 0.0) active_entries++;
	}

	float entry_width = active_entries == 0 ? 0.0 : 1.0 / active_entries;
	float offset = 0.0;

	auto entry = this->entries.begin();
	while (entry != this->entries.end()) {
		if (warp) {
			if (entry->width.goalf() == 0.0) {
				this->entries.erase(entry++);
				continue;
			}

			entry->offset.setValueAndWarp(offset);
			entry->width.setValueAndWarp(entry_width);
		} else {
			auto warp_init = entry->offset.goalf() == -1.0;

			if (warp_init) {
				float start_offset = offset;
				if (entry == std::prev(this->entries.end())) start_offset += entry_width;
				else if (entry != this->entries.begin()) start_offset += (entry_width * 0.5);

				entry->offset.setValueAndWarp(start_offset);
				entry->width.setValueAndWarp(0.0);
			}

			if (entry->offset.goalf() != offset) entry->offset = offset;
			if (entry->width.goalf() != entry_width) entry->width = entry_width;
		}

		offset += entry_width;
		entry = std::next(entry);
	}
}

void Hy3TabBar::setSize(Vector2D size) {
	if (size == this->size) return;
	this->size = size;
}

Hy3TabGroup::Hy3TabGroup(Hy3Node& node) {
	this->pos.create(AVARTYPE_VECTOR, g_pConfigManager->getAnimationPropertyConfig("windowsIn"), nullptr, AVARDAMAGE_NONE);
	this->size.create(AVARTYPE_VECTOR, g_pConfigManager->getAnimationPropertyConfig("windowsIn"), nullptr, AVARDAMAGE_NONE);
	Debug::log(LOG, "registered anims");
	this->pos.registerVar();
	this->size.registerVar();

	this->updateWithGroup(node);
	this->bar.updateAnimations(true);
	this->pos.warp();
	this->size.warp();
}

void Hy3TabGroup::updateWithGroup(Hy3Node& node) {
	static const auto* gaps_in = &HyprlandAPI::getConfigValue(PHANDLE, "general:gaps_in")->intValue;
	static const auto* bar_height = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:hy3:tabs:bar_height")->intValue;

	auto tpos = node.position + Vector2D(*gaps_in, *gaps_in);
	auto tsize = Vector2D(node.size.x - *gaps_in * 2, *bar_height);

	if (this->pos.goalv() != tpos) this->pos = tpos;
	if (this->size.goalv() != tsize) this->size = tsize;

	this->bar.updateNodeList(node.data.as_group.children);
	this->bar.updateAnimations();

	if (node.data.as_group.focused_child != nullptr) {
		this->updateStencilWindows(*node.data.as_group.focused_child);
	}
}

void Hy3TabGroup::renderTabBar() {
	static const auto* window_rounding = &HyprlandAPI::getConfigValue(PHANDLE, "decoration:rounding")->intValue;
	static const auto* enter_from_top = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:hy3:tabs:from_top")->intValue;
	static const auto* padding = &HyprlandAPI::getConfigValue(PHANDLE, "general:gaps_in")->intValue;

	auto* monitor = g_pHyprOpenGL->m_RenderData.pMonitor;
	auto scale = monitor->scale;

	auto pos = this->pos.vec();
	auto size = this->size.vec();
	pos.y += (this->bar.vertical_pos.fl() * size.y) * (*enter_from_top ? -1 : 1);

	auto scaled_pos = Vector2D(std::round(pos.x * scale), std::round(pos.y * scale));
	auto scaled_size = Vector2D(std::round(size.x * scale), std::round(size.y * scale));
	auto box = wlr_box { scaled_pos.x, scaled_pos.y, scaled_size.x, scaled_size.y };

	this->bar.setSize(scaled_size);

	{
		glEnable(GL_STENCIL_TEST);
		glClearStencil(0);
		glClear(GL_STENCIL_BUFFER_BIT);
		glStencilMask(0xff);
		glStencilFunc(GL_ALWAYS, 1, 0xff);
		glStencilOp(GL_KEEP, GL_KEEP, GL_REPLACE);

		glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);

		for (auto* window: this->stencil_windows) {
			if (!g_pCompositor->windowExists(window)) continue;

			auto wpos = window->m_vRealPosition.vec();
			auto wsize = window->m_vRealPosition.vec();

			wlr_box window_box = { wpos.x, wpos.y, wsize.x, wsize.y };
			scaleBox(&window_box, scale);

			g_pHyprOpenGL->renderRect(&window_box, CColor(0, 0, 0, 0), *window_rounding);
		}

		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		glStencilMask(0x00);
		glStencilFunc(GL_EQUAL, 0, 0xff);
		glStencilOp(GL_KEEP, GL_KEEP, GL_INCR);
	}

	auto render_entry = [&](Hy3TabBarEntry& entry) {
		Vector2D entry_size = { (entry.width.fl() * size.x) - *padding, size.y };
		entry.prepareTexture(scale, entry_size);

		wlr_box box = {
			(pos.x + (entry.offset.fl() * size.x) + (*padding * 0.5)) * scale,
			scaled_pos.y,
			entry_size.x * scale,
			scaled_size.y,
		};

		g_pHyprOpenGL->renderTexture(entry.texture, &box, this->bar.fade_opacity.fl());
	};

	for (auto& entry: this->bar.entries) {
		if (entry.focused) continue;
		render_entry(entry);
	}

	for (auto& entry: this->bar.entries) {
		if (!entry.focused) continue;
		render_entry(entry);
	}

	{
		glClearStencil(0);
		glClear(GL_STENCIL_BUFFER_BIT);
		glDisable(GL_STENCIL_TEST);
		glStencilMask(0xff);
		glStencilFunc(GL_ALWAYS, 1, 0xff);
	}

	g_pHyprRenderer->damageBox(&box);
}

void findOverlappingWindows(Hy3Node& node, float height, std::vector<CWindow*>& windows) {
	switch (node.data.type) {
	case Hy3NodeData::Window:
		windows.push_back(node.data.as_window);
		break;
	case Hy3NodeData::Group:
		auto& group = node.data.as_group;

		switch (group.layout) {
		case Hy3GroupLayout::SplitH:
			for (auto* node: group.children) {
				findOverlappingWindows(*node, height, windows);
			}
			break;
		case Hy3GroupLayout::SplitV:
			for (auto* node: group.children) {
				findOverlappingWindows(*node, height, windows);
				height -= node->size.y;
				if (height <= 0) break;
			}
			break;
		case Hy3GroupLayout::Tabbed:
			// assume the height of that node's tab bar already pushes it out of range
			break;
		}
	}
}

void Hy3TabGroup::updateStencilWindows(Hy3Node& group) {
	this->stencil_windows.clear();
	findOverlappingWindows(group, this->size.goalv().y, this->stencil_windows);
}
