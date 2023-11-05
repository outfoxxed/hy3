#include "TabGroup.hpp"

#include <cairo/cairo.h>
#include <hyprland/src/Compositor.hpp>
#include <hyprland/src/helpers/Box.hpp>
#include <hyprland/src/helpers/Color.hpp>
#include <hyprland/src/render/OpenGL.hpp>
#include <pango/pangocairo.h>
#include <pixman.h>

#include "globals.hpp"

Hy3TabBarEntry::Hy3TabBarEntry(Hy3TabBar& tab_bar, Hy3Node& node): tab_bar(tab_bar), node(node) {
	this->focused.create(
	    AVARTYPE_FLOAT,
	    0.0f,
	    g_pConfigManager->getAnimationPropertyConfig("fadeSwitch"),
	    nullptr,
	    AVARDAMAGE_NONE
	);

	this->urgent.create(
	    AVARTYPE_FLOAT,
	    0.0f,
	    g_pConfigManager->getAnimationPropertyConfig("fadeSwitch"),
	    nullptr,
	    AVARDAMAGE_NONE
	);

	this->offset.create(
	    AVARTYPE_FLOAT,
	    -1.0f,
	    g_pConfigManager->getAnimationPropertyConfig("windowsMove"),
	    nullptr,
	    AVARDAMAGE_NONE
	);

	this->width.create(
	    AVARTYPE_FLOAT,
	    -1.0f,
	    g_pConfigManager->getAnimationPropertyConfig("windowsMove"),
	    nullptr,
	    AVARDAMAGE_NONE
	);

	this->vertical_pos.create(
	    AVARTYPE_FLOAT,
	    1.0f,
	    g_pConfigManager->getAnimationPropertyConfig("windowsIn"),
	    nullptr,
	    AVARDAMAGE_NONE
	);

	this->fade_opacity.create(
	    AVARTYPE_FLOAT,
	    0.0f,
	    g_pConfigManager->getAnimationPropertyConfig("windowsIn"),
	    nullptr,
	    AVARDAMAGE_NONE
	);

	this->focused.registerVar();
	this->urgent.registerVar();
	this->offset.registerVar();
	this->width.registerVar();
	this->vertical_pos.registerVar();
	this->fade_opacity.registerVar();

	auto update_callback = [this](void*) { this->tab_bar.dirty = true; };

	this->focused.setUpdateCallback(update_callback);
	this->urgent.setUpdateCallback(update_callback);
	this->offset.setUpdateCallback(update_callback);
	this->width.setUpdateCallback(update_callback);
	this->vertical_pos.setUpdateCallback(update_callback);
	this->fade_opacity.setUpdateCallback(update_callback);

	this->window_title = node.getTitle();
	this->urgent = node.isUrgent();

	this->vertical_pos = 0.0;
	this->fade_opacity = 1.0;
}

bool Hy3TabBarEntry::operator==(const Hy3Node& node) const { return this->node == node; }

bool Hy3TabBarEntry::operator==(const Hy3TabBarEntry& entry) const {
	return this->node == entry.node;
}

void Hy3TabBarEntry::setFocused(bool focused) {
	if (this->focused.goalf() != focused) {
		this->focused = focused;
	}
}

void Hy3TabBarEntry::setUrgent(bool urgent) {
	if (urgent && this->focused.goalf() == 1.0) urgent = false;
	if (this->urgent.goalf() != urgent) {
		this->urgent = urgent;
	}
}

void Hy3TabBarEntry::setWindowTitle(std::string title) {
	if (this->window_title != title) {
		this->window_title = title;
		this->tab_bar.dirty = true;
	}
}

void Hy3TabBarEntry::beginDestroy() {
	this->destroying = true;
	this->vertical_pos = 1.0;
	this->fade_opacity = 0.0;
}

void Hy3TabBarEntry::unDestroy() {
	this->destroying = false;
	this->vertical_pos = 0.0;
	this->fade_opacity = 1.0;
}

bool Hy3TabBarEntry::shouldRemove() {
	return this->destroying && (this->vertical_pos.fl() == 1.0 || this->width.fl() == 0.0);
}

void Hy3TabBarEntry::prepareTexture(float scale, CBox& box) {
	// clang-format off
	static const auto* s_rounding = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:hy3:tabs:rounding")->intValue;
	static const auto* render_text = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:hy3:tabs:render_text")->intValue;
	static const auto* text_font = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:hy3:tabs:text_font")->strValue;
	static const auto* text_height = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:hy3:tabs:text_height")->intValue;
	static const auto* text_padding = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:hy3:tabs:text_padding")->intValue;
	static const auto* col_active = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:hy3:tabs:col.active")->intValue;
	static const auto* col_urgent = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:hy3:tabs:col.urgent")->intValue;
	static const auto* col_inactive = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:hy3:tabs:col.inactive")->intValue;
	static const auto* col_text_active = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:hy3:tabs:col.text.active")->intValue;
	static const auto* col_text_urgent = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:hy3:tabs:col.text.urgent")->intValue;
	static const auto* col_text_inactive = &HyprlandAPI::getConfigValue(PHANDLE, "plugin:hy3:tabs:col.text.inactive")->intValue;
	// clang-format on

	auto width = box.width;
	auto height = box.height;

	auto rounding = std::min((double) *s_rounding, std::min(width * 0.5, height * 0.5));

	if (this->texture.m_iTexID == 0
	    // clang-format off
			|| this->last_render.x != box.x
			|| this->last_render.y != box.y
	    || this->last_render.focused != this->focused.fl()
			|| this->last_render.urgent != this->urgent.fl()
	    || this->last_render.window_title != this->window_title
	    || this->last_render.rounding != rounding
			|| this->last_render.text_font != *text_font
	    || this->last_render.text_height != *text_height
	    || this->last_render.text_padding != *text_padding
	    || this->last_render.col_active != *col_active
			|| this->last_render.col_urgent != *col_urgent
	    || this->last_render.col_inactive != *col_inactive
	    || this->last_render.col_text_active != *col_text_active
	    || this->last_render.col_text_urgent != *col_text_urgent
	    || this->last_render.col_text_inactive != *col_text_inactive
	    // clang-format on
	)
	{
		this->last_render.x = box.x;
		this->last_render.y = box.y;
		this->last_render.focused = this->focused.fl();
		this->last_render.urgent = this->urgent.fl();
		this->last_render.window_title = this->window_title;
		this->last_render.rounding = rounding;
		this->last_render.text_font = *text_font;
		this->last_render.text_height = *text_height;
		this->last_render.text_padding = *text_padding;
		this->last_render.col_active = *col_active;
		this->last_render.col_urgent = *col_urgent;
		this->last_render.col_inactive = *col_inactive;
		this->last_render.col_text_active = *col_text_active;
		this->last_render.col_text_urgent = *col_text_urgent;
		this->last_render.col_text_inactive = *col_text_inactive;

		auto cairo_surface = cairo_image_surface_create(CAIRO_FORMAT_ARGB32, width, height);
		auto cairo = cairo_create(cairo_surface);

		// clear pixmap
		cairo_save(cairo);
		cairo_set_operator(cairo, CAIRO_OPERATOR_CLEAR);
		cairo_paint(cairo);
		cairo_restore(cairo);

		// set brush
		auto focused = this->focused.fl();
		auto urgent = this->urgent.fl();
		auto inactive = 1.0 - (focused + urgent);
		auto c = (CColor(*col_active) * focused) + (CColor(*col_urgent) * urgent)
		       + (CColor(*col_inactive) * inactive);

		cairo_set_source_rgba(cairo, c.r, c.g, c.b, c.a);

		// outline bar shape
		cairo_move_to(cairo, 0, rounding);
		cairo_arc(cairo, rounding, rounding, rounding, -180.0 * (M_PI / 180.0), -90.0 * (M_PI / 180.0));
		cairo_line_to(cairo, width - rounding, 0);
		cairo_arc(cairo, width - rounding, rounding, rounding, -90.0 * (M_PI / 180.0), 0.0);
		cairo_line_to(cairo, width, height - rounding);
		cairo_arc(cairo, width - rounding, height - rounding, rounding, 0.0, 90.0 * (M_PI / 180.0));
		cairo_line_to(cairo, rounding, height);
		cairo_arc(
		    cairo,
		    rounding,
		    height - rounding,
		    rounding,
		    -270.0 * (M_PI / 180.0),
		    -180.0 * (M_PI / 180.0)
		);
		cairo_close_path(cairo);

		// draw
		cairo_fill(cairo);

		// render window title
		if (*render_text) {
			PangoLayout* layout = pango_cairo_create_layout(cairo);
			pango_layout_set_text(layout, this->window_title.c_str(), -1);

			PangoFontDescription* font_desc = pango_font_description_from_string(text_font->c_str());
			pango_font_description_set_size(font_desc, *text_height * scale * PANGO_SCALE);
			pango_layout_set_font_description(layout, font_desc);
			pango_font_description_free(font_desc);

			int padding = *text_padding * scale;
			int width = box.width - padding * 2;

			pango_layout_set_width(layout, width * PANGO_SCALE);
			pango_layout_set_ellipsize(layout, PANGO_ELLIPSIZE_END);

			auto c = (CColor(*col_text_active) * focused) + (CColor(*col_text_urgent) * urgent)
			       + (CColor(*col_text_inactive) * inactive);

			cairo_set_source_rgba(cairo, c.r, c.g, c.b, c.a);

			int layout_width, layout_height;
			pango_layout_get_size(layout, &layout_width, &layout_height);

			auto y_offset = (height / 2.0) - (((double) layout_height / PANGO_SCALE) / 2.0);
			cairo_move_to(cairo, padding, y_offset);
			pango_cairo_show_layout(cairo, layout);
			g_object_unref(layout);
		}

		// flush cairo
		cairo_surface_flush(cairo_surface);

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
	this->fade_opacity.create(
	    AVARTYPE_FLOAT,
	    1.0f,
	    g_pConfigManager->getAnimationPropertyConfig("windowsMove"),
	    nullptr,
	    AVARDAMAGE_NONE
	);

	this->fade_opacity.registerVar();
	this->fade_opacity.setUpdateCallback([this](void*) { this->dirty = true; });
}

void Hy3TabBar::beginDestroy() {
	for (auto& entry: this->entries) {
		entry.beginDestroy();
	}
}

void Hy3TabBar::tick() {
	auto iter = this->entries.begin();

	while (iter != this->entries.end()) {
		if (iter->shouldRemove()) iter = this->entries.erase(iter);
		else iter = std::next(iter);
	}

	if (this->entries.empty()) this->destroy = true;
}

void Hy3TabBar::updateNodeList(std::list<Hy3Node*>& nodes) {
	std::list<std::list<Hy3TabBarEntry>::iterator> removed_entries;

	auto entry = this->entries.begin();
	auto node = nodes.begin();

	// move any out of order entries to removed_entries
	while (node != nodes.end()) {
		while (true) {
			if (entry == this->entries.end()) goto exitloop;
			if (*entry == **node) break;
			removed_entries.push_back(entry);
			entry = std::next(entry);
		}

		node = std::next(node);
		entry = std::next(entry);
	}

exitloop:

	// move any extra entries to removed_entries
	while (entry != this->entries.end()) {
		removed_entries.push_back(entry);
		entry = std::next(entry);
	}

	entry = this->entries.begin();
	node = nodes.begin();

	// add missing entries, taking first from removed_entries
	while (node != nodes.end()) {
		if (entry == this->entries.end() || *entry != **node) {
			if (std::find(removed_entries.begin(), removed_entries.end(), entry) != removed_entries.end())
			{
				entry = std::next(entry);
				continue;
			}

			auto moved =
			    std::find_if(removed_entries.begin(), removed_entries.end(), [&node](auto entry) {
				    return **node == *entry;
			    });

			if (moved != removed_entries.end()) {
				this->entries.splice(entry, this->entries, *moved);
				entry = *moved;
				removed_entries.erase(moved);
			} else {
				entry = this->entries.emplace(entry, *this, **node);
			}
		}

		entry->unDestroy();

		// set stats from node data
		auto* parent = (*node)->parent;
		auto& parent_group = parent->data.as_group;

		entry->setFocused(
		    parent_group.focused_child == *node
		    || (parent_group.group_focused && parent->isIndirectlyFocused())
		);

		entry->setUrgent((*node)->isUrgent());
		entry->setWindowTitle((*node)->getTitle());

		node = std::next(node);
		if (entry != this->entries.end()) entry = std::next(entry);
	}

	// initiate remove animations for any removed entries
	for (auto& entry: removed_entries) {
		if (!entry->destroying) entry->beginDestroy();
		if (entry->shouldRemove()) this->entries.erase(entry);
	}
}

void Hy3TabBar::updateAnimations(bool warp) {
	int active_entries = 0;
	for (auto& entry: this->entries) {
		if (!entry.destroying) active_entries++;
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
				entry->offset.setValueAndWarp(offset);
				entry->width.setValueAndWarp(entry->vertical_pos.fl() == 0.0 ? 0.0 : entry_width);
			}

			if (!entry->destroying) {
				if (entry->offset.goalf() != offset) entry->offset = offset;
				if ((warp_init || entry->width.goalf() != 0.0) && entry->width.goalf() != entry_width)
					entry->width = entry_width;
			}
		}

		if (!entry->destroying) offset += entry->width.goalf();
		entry = std::next(entry);
	}
}

void Hy3TabBar::setSize(Vector2D size) {
	if (size == this->size) return;
	this->size = size;
}

Hy3TabGroup::Hy3TabGroup(Hy3Node& node) {
	this->pos.create(
	    AVARTYPE_VECTOR,
	    g_pConfigManager->getAnimationPropertyConfig("windowsMove"),
	    nullptr,
	    AVARDAMAGE_NONE
	);

	this->size.create(
	    AVARTYPE_VECTOR,
	    g_pConfigManager->getAnimationPropertyConfig("windowsMove"),
	    nullptr,
	    AVARDAMAGE_NONE
	);

	this->pos.registerVar();
	this->size.registerVar();

	this->updateWithGroup(node, true);
	this->pos.warp();
	this->size.warp();
}

void Hy3TabGroup::updateWithGroup(Hy3Node& node, bool warp) {
	static const auto* gaps_in = &HyprlandAPI::getConfigValue(PHANDLE, "general:gaps_in")->intValue;
	static const auto* gaps_out = &HyprlandAPI::getConfigValue(PHANDLE, "general:gaps_out")->intValue;
	static const auto* bar_height =
	    &HyprlandAPI::getConfigValue(PHANDLE, "plugin:hy3:tabs:height")->intValue;

	auto gaps = node.parent == nullptr ? *gaps_out : *gaps_in;
	auto tpos = node.position + Vector2D(gaps, gaps) + node.gap_topleft_offset;
	auto tsize = Vector2D(
	    node.size.x - node.gap_bottomright_offset.x - node.gap_topleft_offset.x - gaps * 2,
	    *bar_height
	);

	this->hidden = node.hidden;
	if (this->pos.goalv() != tpos) {
		this->pos = tpos;
		if (warp) this->pos.warp();
	}

	if (this->size.goalv() != tsize) {
		this->size = tsize;
		if (warp) this->size.warp();
	}

	this->bar.updateNodeList(node.data.as_group.children);
	this->bar.updateAnimations(warp);

	if (node.data.as_group.focused_child != nullptr) {
		this->updateStencilWindows(*node.data.as_group.focused_child);
	}
}

void Hy3TabGroup::tick() {
	static const auto* enter_from_top =
	    &HyprlandAPI::getConfigValue(PHANDLE, "plugin:hy3:tabs:from_top")->intValue;
	static const auto* padding =
	    &HyprlandAPI::getConfigValue(PHANDLE, "plugin:hy3:tabs:padding")->intValue;
	auto* workspace = g_pCompositor->getWorkspaceByID(this->workspace_id);

	this->bar.tick();

	if (workspace != nullptr) {
		if (workspace->m_bHasFullscreenWindow) {
			if (this->bar.fade_opacity.goalf() != 0.0) this->bar.fade_opacity = 0.0;
		} else {
			if (this->bar.fade_opacity.goalf() != 1.0) this->bar.fade_opacity = 1.0;
		}
	}

	auto pos = this->pos.vec();
	auto size = this->size.vec();

	if (this->last_pos != pos || this->last_size != size) {
		CBox damage_box = {this->last_pos.x, this->last_pos.y, this->last_size.x, this->last_size.y};
		g_pHyprRenderer->damageBox(&damage_box);

		this->bar.damaged = true;
		this->last_pos = pos;
		this->last_size = size;
	}

	if (this->bar.destroy || this->bar.dirty) {
		// damage any area that could be covered by bar in/out animations
		size.y = size.y * 2 + *padding;
		if (*enter_from_top) {
			pos.y -= *padding;
		}

		CBox damage_box = {pos.x, pos.y, size.x, size.y};
		g_pHyprRenderer->damageBox(&damage_box);

		this->bar.damaged = true;
		this->bar.dirty = false;
	}
}

void Hy3TabGroup::renderTabBar() {
	static const auto* window_rounding =
	    &HyprlandAPI::getConfigValue(PHANDLE, "decoration:rounding")->intValue;
	static const auto* enter_from_top =
	    &HyprlandAPI::getConfigValue(PHANDLE, "plugin:hy3:tabs:from_top")->intValue;
	static const auto* padding =
	    &HyprlandAPI::getConfigValue(PHANDLE, "plugin:hy3:tabs:padding")->intValue;

	auto* monitor = g_pHyprOpenGL->m_RenderData.pMonitor;
	auto* workspace = g_pCompositor->getWorkspaceByID(this->workspace_id);
	auto scale = monitor->scale;

	auto monitor_size = monitor->vecSize;
	auto pos = this->pos.vec() - monitor->vecPosition;
	auto size = this->size.vec();

	if (workspace != nullptr) {
		pos = pos + workspace->m_vRenderOffset.vec();
	}

	auto scaled_pos = Vector2D(std::round(pos.x * scale), std::round(pos.y * scale));
	auto scaled_size = Vector2D(std::round(size.x * scale), std::round(size.y * scale));
	wlr_box box = {scaled_pos.x, scaled_pos.y, scaled_size.x, scaled_size.y};

	if (scaled_pos.x > monitor_size.x || scaled_pos.y > monitor_size.y
	    || scaled_pos.x + scaled_size.x < 0 || scaled_pos.y + scaled_size.y < 0)
		return;

	if (!this->bar.damaged) {
		pixman_region32 damage;
		pixman_region32_init(&damage);

		pixman_region32_intersect_rect(
		    &damage,
		    g_pHyprOpenGL->m_RenderData.damage.pixman(),
		    box.x,
		    box.y,
		    box.width,
		    box.height
		);

		this->bar.damaged = pixman_region32_not_empty(&damage);
		pixman_region32_fini(&damage);
	}

	if (!this->bar.damaged || this->bar.destroy) return;
	this->bar.damaged = false;

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

			auto wpos = window->m_vRealPosition.vec() - monitor->vecPosition;
			auto wsize = window->m_vRealSize.vec();

			CBox window_box = {wpos.x, wpos.y, wsize.x, wsize.y};
			// scaleBox(&window_box, scale);
			window_box.scale(scale);

			if (window_box.width > 0 && window_box.height > 0)
				g_pHyprOpenGL->renderRect(&window_box, CColor(0, 0, 0, 0), *window_rounding);
		}

		glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);

		glStencilMask(0x00);
		glStencilFunc(GL_EQUAL, 0, 0xff);
		glStencilOp(GL_KEEP, GL_KEEP, GL_KEEP);
	}

	auto fade_opacity =
	    this->bar.fade_opacity.fl() * (workspace == nullptr ? 1.0 : workspace->m_fAlpha.fl());

	auto render_entry = [&](Hy3TabBarEntry& entry) {
		Vector2D entry_pos = {
		    (pos.x + (entry.offset.fl() * size.x) + (*padding * 0.5)) * scale,
		    scaled_pos.y
		        + ((entry.vertical_pos.fl() * (size.y + *padding) * scale) * (*enter_from_top ? -1 : 1)
		        ),
		};
		Vector2D entry_size = {((entry.width.fl() * size.x) - *padding) * scale, scaled_size.y};
		if (entry_size.x < 0 || entry_size.y < 0 || fade_opacity == 0.0) return;

		CBox box = {
		    entry_pos.x,
		    entry_pos.y,
		    entry_size.x,
		    entry_size.y,
		};

		entry.prepareTexture(scale, box);
		g_pHyprOpenGL->renderTexture(entry.texture, &box, fade_opacity * entry.fade_opacity.fl());
	};

	for (auto& entry: this->bar.entries) {
		if (entry.focused.goalf() == 1.0) continue;
		render_entry(entry);
	}

	for (auto& entry: this->bar.entries) {
		if (entry.focused.goalf() == 0.0) continue;
		render_entry(entry);
	}

	{
		glClearStencil(0);
		glClear(GL_STENCIL_BUFFER_BIT);
		glDisable(GL_STENCIL_TEST);
		glStencilMask(0xff);
		glStencilFunc(GL_ALWAYS, 1, 0xff);
	}
}

void findOverlappingWindows(Hy3Node& node, float height, std::vector<CWindow*>& windows) {
	switch (node.data.type) {
	case Hy3NodeType::Window: windows.push_back(node.data.as_window); break;
	case Hy3NodeType::Group:
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
