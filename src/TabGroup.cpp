#include "globals.hpp"
#include "TabGroup.hpp"
#include "Hy3Layout.hpp"

#include <hyprland/src/Compositor.hpp>
#include <cairo/cairo.h>

void Hy3TabBar::updateWithGroupEntries(Hy3Node& group_node) {
	if (group_node.data.type != Hy3NodeData::Group) return;
	auto& group = group_node.data.as_group;

	auto entries_iter = this->entries.begin();
	auto group_iter = group.children.begin();

	auto* root_node = &group_node;
	while (root_node->parent != nullptr) root_node = root_node->parent;
	Hy3Node* focused_node = root_node->getFocusedNode();

	while (entries_iter != this->entries.end()) {
		if (group_iter == group.children.end()) {
			needs_redraw = true;

			while (entries_iter != this->entries.end()) {
				entries_iter = this->entries.erase(entries_iter);
			}

			return;
		};

		auto& entry = *entries_iter;
		auto& node = **group_iter;

		std::string title = node.getTitle();
		bool urgent = node.isUrgent();
		bool focused = focused_node == &group_node
		|| focused_node == &node
			|| (node.data.type == Hy3NodeData::Group && node.data.as_group.hasChild(focused_node));

		if (entry.urgent != urgent
				|| entry.focused != focused
				|| entry.window_title != title)
			this->needs_redraw = true;

		entry.window_title = std::move(title);
		entry.urgent = urgent;
		entry.focused = focused;

		entries_iter = std::next(entries_iter);
		group_iter = std::next(group_iter);
	}

	while (group_iter != group.children.end()) {
		needs_redraw = true;

		auto& node = **group_iter;

		this->entries.push_back({
			.window_title = node.getTitle(),
			.urgent = node.isUrgent(),
			.focused = focused_node == &group_node
				|| focused_node == &node
				|| (node.data.type == Hy3NodeData::Group && node.data.as_group.hasChild(focused_node)),
		});

		group_iter = std::next(group_iter);
	}
}

void Hy3TabBar::setPos(Vector2D pos) {
	if (pos == this->pos) return;
	needs_redraw = true;
	this->pos = pos;
}

void Hy3TabBar::setSize(Vector2D size) {
	this->size = size;
}

void Hy3TabBar::prepareTexture() {
	auto bar_width = this->size.x;
	auto bar_height = this->size.y;

	if (needs_redraw || this->texture.m_iTexID == 0) {
		static const auto* rounding_setting = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:hy3:tabs:rounding")->intValue;
		auto rounding = *rounding_setting;

		auto cairo_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, this->size.x, this->size.y);
		auto cairo = cairo_create(cairo_surface);

		// clear pixmap
		cairo_save(cairo);
		cairo_set_operator(cairo, CAIRO_OPERATOR_CLEAR);
		cairo_paint(cairo);
		cairo_restore(cairo);

		auto swidth = (double) bar_width / (double) this->entries.size();
		size_t i = 0;

		for (auto& entry: this->entries) {
			auto width = swidth;
			auto x = i * width;

			if (entry.focused) {
				cairo_set_source_rgba(cairo, 0.0, 1.0, 0.7, 0.5);
			} else {
				cairo_set_source_rgba(cairo, 0.2, 0.7, 1.0, 0.5);
			}

			if (i == this->entries.size() - 1) {
				cairo_move_to(cairo, x + width - rounding, rounding);
				cairo_arc(cairo, x + width - rounding, rounding, rounding, -90.0 * (M_PI / 180.0), 0.0);
				cairo_rectangle(cairo, x + width - rounding, rounding, rounding, bar_height - rounding * 2);
				cairo_move_to(cairo, x + width - rounding, bar_height - rounding);
				cairo_arc(cairo, x + width - rounding, bar_height - rounding, rounding, 0.0, 90.0 * (M_PI / 180.0));
				width -= rounding;
			}

			if (i == 0) {
				cairo_move_to(cairo, x + rounding, rounding);
				cairo_arc(cairo, x + rounding, rounding, rounding, -180.0 * (M_PI / 180.0), -90.0 * (M_PI / 180.0));
				cairo_rectangle(cairo, x, rounding, rounding, bar_height - rounding * 2);
				cairo_move_to(cairo, x + rounding, bar_height - rounding);
				cairo_arc(cairo, x + rounding, bar_height - rounding, rounding, -270.0 * (M_PI / 180.0), -180.0 * (M_PI / 180.0));
				x += rounding;
				width -= rounding;
			}

			cairo_rectangle(cairo, x, 0, width, bar_height);
			cairo_fill(cairo);

			i++;
		}

		auto data = cairo_image_surface_get_data(cairo_surface);
		this->texture.allocate();

		glBindTexture(GL_TEXTURE_2D, this->texture.m_iTexID);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	#ifdef GLES32
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_R, GL_BLUE);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_SWIZZLE_B, GL_RED);
	#endif

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, bar_width, bar_height, 0, GL_RGBA, GL_UNSIGNED_BYTE, data);

		cairo_destroy(cairo);
		cairo_surface_destroy(cairo_surface);
	} else {
		glBindTexture(GL_TEXTURE_2D, this->texture.m_iTexID);
	}
}

Hy3TabGroup::Hy3TabGroup(Hy3Node& node) {
	this->pos.create(AVARTYPE_VECTOR, g_pConfigManager->getAnimationPropertyConfig("windowsIn"), nullptr, AVARDAMAGE_NONE);
	this->size.create(AVARTYPE_VECTOR, g_pConfigManager->getAnimationPropertyConfig("windowsIn"), nullptr, AVARDAMAGE_NONE);
	Debug::log(LOG, "registered anims");
	this->pos.registerVar();
	this->size.registerVar();

	this->updateWithGroup(node);
	this->pos.warp(false);
	this->size.warp(false);
}

void Hy3TabGroup::updateWithGroup(Hy3Node& node) {
	static const auto* gaps_in = &HyprlandAPI::getConfigValue(PHANDLE, "general:gaps_in")->intValue;
	static const auto* bar_height = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:hy3:tabs:bar_height")->intValue;

	auto tpos = node.position + Vector2D(*gaps_in, *gaps_in);
	auto tsize = Vector2D(node.size.x - *gaps_in * 2, *bar_height);

	if (this->pos.goalv() != tpos) this->pos = tpos;
	if (this->size.goalv() != tsize) this->size = tsize;

	this->bar.updateWithGroupEntries(node);
}

void Hy3TabGroup::renderTabBar() {

	auto* monitor = g_pHyprOpenGL->m_RenderData.pMonitor;
	auto scale = monitor->scale;

	auto pos = this->pos.vec();
	auto size = this->size.vec();

	auto scaled_pos = Vector2D(std::round(pos.x * scale), std::round(pos.y * scale));
	auto scaled_size = Vector2D(std::round(size.x * scale), std::round(size.y * scale));
	auto box = wlr_box { scaled_pos.x, scaled_pos.y, scaled_size.x, scaled_size.y };

	this->bar.setPos(scaled_pos);
	this->bar.setSize(scaled_size);

	this->bar.prepareTexture();
	g_pHyprOpenGL->renderTexture(this->bar.texture, &box, 1.0);
	g_pHyprRenderer->damageBox(&box);
}
