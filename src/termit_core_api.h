/*  Copyright (C) 2007-2010, Evgeny Ratnikov
    Copyright (C) 2018, Roberto Vergaray

    This file is part of hermit, forked from termit.
    hermit is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License version 2 
    as published by the Free Software Foundation.
    hermit is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.
    You should have received a copy of the GNU General Public License
    along with hermit. If not, see <http://www.gnu.org/licenses/>.*/

#ifndef HERMIT_CORE_API_H
#define HERMIT_CORE_API_H

#include "termit.h"
#include "configs.h"

void hermit_reconfigure();
void hermit_after_show_all();

void hermit_append_tab();
void hermit_append_tab_with_command(gchar** argv);
void hermit_append_tab_with_details(const struct TabInfo*);

void hermit_activate_tab(gint tab_index);
void hermit_prev_tab();
void hermit_next_tab();
void hermit_paste();
void hermit_copy();
gchar* hermit_get_selection();
void hermit_search_find_next();
void hermit_search_find_prev();
void hermit_for_each_row(int lua_callback);
void hermit_for_each_visible_row(int lua_callback);
void hermit_close_tab();
void hermit_toggle_menubar();
void hermit_toggle_tabbar();
void hermit_toggle_search();
void hermit_set_window_title(const gchar* title);
void hermit_set_statusbar_message(guint page);
void hermit_set_encoding(const gchar* encoding);
void hermit_quit();

void hermit_tab_feed(struct HermitTab* pTab, const gchar* data);
void hermit_tab_feed_child(struct HermitTab* pTab, const gchar* data);
void hermit_tab_set_font(struct HermitTab* pTab, const gchar* font_name);
void hermit_tab_set_font_by_index(gint tab_index, const gchar* font_name);
void hermit_tab_set_transparency(struct HermitTab* pTab, gdouble transparency);
void hermit_tab_set_style(gint tab_index, const struct HermitStyle*);
void hermit_tab_apply_colors(struct HermitTab* pTab);
void hermit_tab_set_color_foreground(struct HermitTab* pTab, const GdkColor* p_color);
void hermit_tab_set_color_background(struct HermitTab* pTab, const GdkColor* p_color);
void hermit_tab_set_color_foreground_by_index(gint tab_index, const GdkColor*);
void hermit_tab_set_color_background_by_index(gint tab_index, const GdkColor*);
void hermit_tab_set_background_image(struct HermitTab* pTab, const gchar* image_file);
void hermit_tab_set_title(struct HermitTab* pTab, const gchar* title);
void hermit_tab_set_audible_beep(struct HermitTab* pTab, gboolean audible_beep);
void hermit_tab_set_visible_beep(struct HermitTab* pTab, gboolean visible_beep);
void hermit_tab_set_pos(struct HermitTab* pTab, int newPos);

int hermit_get_current_tab_index();
gchar* hermit_get_pid_dir(pid_t pid);

/**
 * function to switch key processing policy
 * keycodes - kb layout independent
 * keysyms - kb layout dependent
 * */
void hermit_set_kb_policy(enum HermitKbPolicy kbp);
void hermit_set_show_scrollbar_signal(GtkWidget* menuItem, gpointer pHanderId);

#endif /* HERMIT_CORE_API_H */

