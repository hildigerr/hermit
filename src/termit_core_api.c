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

#include <string.h>
#include <stdlib.h>
#include <math.h>
#include "termit.h"
#include "configs.h"
#include "callbacks.h"
#include "lua_api.h"
#include "keybindings.h"
#include "termit_core_api.h"
#include "termit_style.h"

void hermit_create_popup_menu();
void hermit_create_menubar();

static void hermit_hide_scrollbars()
{
    gint page_num = gtk_notebook_get_n_pages(GTK_NOTEBOOK(hermit.notebook));
    gint i=0;
    for (; i<page_num; ++i) {
        HERMIT_GET_TAB_BY_INDEX(pTab, i, continue);
        if (!pTab->scrollbar_is_shown)
            gtk_widget_hide(pTab->scrollbar);
    }
}

void hermit_tab_set_color_foreground(struct HermitTab* pTab, const GdkColor* p_color)
{
    if (p_color) {
        pTab->style.foreground_color = gdk_color_copy(p_color);
        vte_terminal_set_color_foreground(VTE_TERMINAL(pTab->vte), pTab->style.foreground_color);
        if (pTab->style.foreground_color) {
            vte_terminal_set_color_bold(VTE_TERMINAL(pTab->vte), pTab->style.foreground_color);
        }
    }
}

void hermit_tab_set_color_background(struct HermitTab* pTab, const GdkColor* p_color)
{
    if (p_color) {
        pTab->style.background_color = gdk_color_copy(p_color);
        vte_terminal_set_color_background(VTE_TERMINAL(pTab->vte), pTab->style.background_color);
    }
}

void hermit_tab_apply_colors(struct HermitTab* pTab)
{
    if (pTab->style.colors) {
        vte_terminal_set_colors(VTE_TERMINAL(pTab->vte), NULL, NULL, pTab->style.colors, pTab->style.colors_size);
        if (pTab->style.foreground_color) {
            vte_terminal_set_color_bold(VTE_TERMINAL(pTab->vte), pTab->style.foreground_color);
        }
    }
    if (pTab->style.foreground_color) {
        vte_terminal_set_color_foreground(VTE_TERMINAL(pTab->vte), pTab->style.foreground_color);
    }
    if (pTab->style.background_color) {
        vte_terminal_set_color_background(VTE_TERMINAL(pTab->vte), pTab->style.background_color);
    }
}

void hermit_tab_feed(struct HermitTab* pTab, const gchar* data)
{
    vte_terminal_feed(VTE_TERMINAL(pTab->vte), data, strlen(data));
}

void hermit_tab_feed_child(struct HermitTab* pTab, const gchar* data)
{
    vte_terminal_feed_child(VTE_TERMINAL(pTab->vte), data, strlen(data));
}

static void hermit_set_colors()
{
    gint page_num = gtk_notebook_get_n_pages(GTK_NOTEBOOK(hermit.notebook));
    gint i=0;
    for (; i<page_num; ++i) {
        HERMIT_GET_TAB_BY_INDEX(pTab, i, continue);
        hermit_tab_apply_colors(pTab);
    }
}

static void hermit_set_fonts()
{
    gint page_num = gtk_notebook_get_n_pages(GTK_NOTEBOOK(hermit.notebook));
    gint i=0;
    gint minWidth = 0, minHeight = 0;
    for (; i<page_num; ++i) {
        HERMIT_GET_TAB_BY_INDEX(pTab, i, continue);
        vte_terminal_set_font(VTE_TERMINAL(pTab->vte), pTab->style.font);
        GtkBorder* border;
        gtk_widget_style_get(GTK_WIDGET(pTab->vte), "inner-border", &border, NULL);
        gint w = vte_terminal_get_char_width(VTE_TERMINAL(pTab->vte)) * configs.cols + border->left + border->right;
        gint h = vte_terminal_get_char_height(VTE_TERMINAL(pTab->vte)) * configs.rows + border->top + border->bottom;
        if (w > minWidth) minWidth = w;
        if (h > minHeight) minHeight = h;
    }
    gint oldWidth, oldHeight;
    gtk_window_get_size(GTK_WINDOW(hermit.main_window), &oldWidth, &oldHeight);
    const gint width = (minWidth > oldWidth) ? minWidth : oldWidth;
    const gint height = (minHeight > oldHeight) ? minHeight : oldHeight;
    gtk_window_resize(GTK_WINDOW(hermit.main_window), width, height);

    GdkGeometry geom;
    geom.min_width = minWidth;
    geom.min_height = minHeight;
    gtk_window_set_geometry_hints(GTK_WINDOW(hermit.main_window), hermit.main_window, &geom, GDK_HINT_MIN_SIZE);
}

gchar* hermit_get_pid_dir(pid_t pid)
{
    gchar* file = g_strdup_printf("/proc/%d/cwd", pid);
    gchar* link = g_file_read_link(file, NULL);
    g_free(file);
    return link;
}


void hermit_toggle_menubar()
{
    if (configs.hide_menubar) {
        gtk_widget_hide(GTK_WIDGET(hermit.hbox));
    } else {
        gtk_widget_show(GTK_WIDGET(hermit.hbox));
    }
    configs.hide_menubar = !configs.hide_menubar;
}

static void hermit_check_tabbar()
{
    if (!configs.hide_tabbar) {
        if (configs.hide_single_tab && gtk_notebook_get_n_pages(GTK_NOTEBOOK(hermit.notebook)) == 1) {
            gtk_notebook_set_show_tabs(GTK_NOTEBOOK(hermit.notebook), FALSE);
        } else {
            gtk_notebook_set_show_tabs(GTK_NOTEBOOK(hermit.notebook), TRUE);
        }
    } else {
        gtk_notebook_set_show_tabs(GTK_NOTEBOOK(hermit.notebook), FALSE);
    }
}

void hermit_toggle_tabbar()
{
    gtk_notebook_set_show_tabs(GTK_NOTEBOOK(hermit.notebook), configs.hide_tabbar);
    configs.hide_tabbar = !configs.hide_tabbar;
}

void hermit_toggle_search()
{
    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(hermit.b_toggle_search))) {
        gtk_widget_show(hermit.search_entry);
        gtk_widget_show(hermit.b_find_prev);
        gtk_widget_show(hermit.b_find_next);
        gtk_window_set_focus(GTK_WINDOW(hermit.main_window), hermit.search_entry);
    } else {
        gtk_widget_hide(hermit.search_entry);
        gtk_widget_hide(hermit.b_find_prev);
        gtk_widget_hide(hermit.b_find_next);
        gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(hermit.notebook));
        HERMIT_GET_TAB_BY_INDEX(pTab, page, return);
        gtk_window_set_focus(GTK_WINDOW(hermit.main_window), pTab->vte);
    }
}

void hermit_after_show_all()
{
    hermit_set_fonts();
    hermit_hide_scrollbars();
    hermit_set_colors();
    hermit_toggle_menubar();
    hermit_check_tabbar();
    hermit_toggle_search();
}

void hermit_reconfigure()
{
    gtk_widget_destroy(hermit.menu);
    gtk_container_remove((configs.top_menu)?GTK_CONTAINER(hermit.vbox):GTK_CONTAINER(hermit.hbox), hermit.menu_bar);

    hermit_config_deinit();
    hermit_configs_set_defaults();
    hermit_keys_set_defaults();
    hermit_lua_load_config();

    hermit_create_popup_menu();
    hermit_create_menubar();
    gtk_box_pack_start((configs.top_menu)?GTK_BOX(hermit.vbox):GTK_BOX(hermit.hbox), hermit.menu_bar, FALSE, 0, 0);
    gtk_box_reorder_child((configs.top_menu)?GTK_BOX(hermit.vbox):GTK_BOX(hermit.hbox), hermit.menu_bar, 0);
    gtk_widget_show_all(hermit.main_window);
    hermit_after_show_all();
}

void hermit_set_statusbar_message(guint page)
{
    HERMIT_GET_TAB_BY_INDEX(pTab, page, return);
    TRACE("%s page=%d get_statusbar_callback=%d", __FUNCTION__, page, configs.get_statusbar_callback);
    if (configs.get_statusbar_callback) {
        gchar* statusbarMessage = hermit_lua_getStatusbarCallback(configs.get_statusbar_callback, page);
        TRACE("statusbarMessage=[%s]", statusbarMessage);
        gtk_statusbar_push(GTK_STATUSBAR(hermit.statusbar), 0, statusbarMessage);
        g_free(statusbarMessage);
    } else {
        gtk_statusbar_push(GTK_STATUSBAR(hermit.statusbar), 0, vte_terminal_get_encoding(VTE_TERMINAL(pTab->vte)));
    }
}

static void hermit_del_tab()
{
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(hermit.notebook));

    HERMIT_GET_TAB_BY_INDEX(pTab, page, return);
    TRACE("%s pid=%d", __FUNCTION__, pTab->pid);
    g_array_free(pTab->matches, TRUE);
    g_free(pTab->encoding);
    g_strfreev(pTab->argv);
    g_free(pTab->title);
    hermit_style_free(&pTab->style);
    g_free(pTab);
    gtk_notebook_remove_page(GTK_NOTEBOOK(hermit.notebook), page);

    hermit_check_tabbar();
}

static void hermit_tab_add_matches(struct HermitTab* pTab, GArray* matches)
{
    guint i = 0;
    for (; i<matches->len; ++i) {
        struct Match* match = &g_array_index(matches, struct Match, i);
        struct Match tabMatch = {};
        tabMatch.lua_callback = match->lua_callback;
        tabMatch.pattern = match->pattern;
        tabMatch.tag = vte_terminal_match_add_gregex(VTE_TERMINAL(pTab->vte), match->regex, match->flags);
        vte_terminal_match_set_cursor_type(VTE_TERMINAL(pTab->vte), tabMatch.tag, GDK_HAND2);
        g_array_append_val(pTab->matches, tabMatch);
    }
}

void hermit_search_find_next()
{
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(hermit.notebook));
    HERMIT_GET_TAB_BY_INDEX(pTab, page, return);
    TRACE("%s tab=%p page=%d", __FUNCTION__, pTab, page);
    vte_terminal_search_find_next(VTE_TERMINAL(pTab->vte));
}

void hermit_search_find_prev()
{
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(hermit.notebook));
    HERMIT_GET_TAB_BY_INDEX(pTab, page, return);
    TRACE("%s tab=%p page=%d", __FUNCTION__, pTab, page);
    vte_terminal_search_find_previous(VTE_TERMINAL(pTab->vte));
}

void hermit_tab_set_transparency(struct HermitTab* pTab, gdouble transparency)
{
    pTab->style.transparency = transparency;
    if (transparency) {
        if (pTab->style.image_file == NULL) {
            vte_terminal_set_background_transparent(VTE_TERMINAL(pTab->vte), TRUE);
        } else {
            vte_terminal_set_background_transparent(VTE_TERMINAL(pTab->vte), FALSE);
        }
        vte_terminal_set_background_saturation(VTE_TERMINAL(pTab->vte), pTab->style.transparency);
    } else {
        vte_terminal_set_background_saturation(VTE_TERMINAL(pTab->vte), pTab->style.transparency);
        vte_terminal_set_background_transparent(VTE_TERMINAL(pTab->vte), FALSE);
    }
}

static void hermit_for_each_row_execute(struct HermitTab* pTab, glong row_start, glong row_end, int lua_callback)
{
    glong i = row_start;
    for (; i < row_end; ++i) {
        char* str = vte_terminal_get_text_range(VTE_TERMINAL(pTab->vte), i, 0, i, 500, NULL, &lua_callback, NULL);
        str[strlen(str) - 1] = '\0';
        hermit_lua_dofunction2(lua_callback, str);
        free(str);
    }
}

void hermit_for_each_row(int lua_callback)
{
    TRACE("%s lua_callback=%d", __FUNCTION__, lua_callback);
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(hermit.notebook));
    HERMIT_GET_TAB_BY_INDEX(pTab, page, return);
    GtkAdjustment* adj = gtk_range_get_adjustment(GTK_RANGE(pTab->scrollbar));
    const glong rows_total = gtk_adjustment_get_upper(adj);
    hermit_for_each_row_execute(pTab, 0, rows_total, lua_callback);
}

void hermit_for_each_visible_row(int lua_callback)
{
    TRACE("%s lua_callback=%d", __FUNCTION__, lua_callback);
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(hermit.notebook));
    HERMIT_GET_TAB_BY_INDEX(pTab, page, return);
    GtkAdjustment* adj = gtk_range_get_adjustment(GTK_RANGE(pTab->scrollbar));
    const glong row_start = ceil(gtk_adjustment_get_value(adj));
    const glong page_size = vte_terminal_get_row_count(VTE_TERMINAL(pTab->vte));
    hermit_for_each_row_execute(pTab, row_start, row_start + page_size, lua_callback);
}

void hermit_tab_set_audible_beep(struct HermitTab* pTab, gboolean audible_beep)
{
    pTab->audible_beep = audible_beep;
    vte_terminal_set_audible_bell(VTE_TERMINAL(pTab->vte), audible_beep);
}

void hermit_tab_set_visible_beep(struct HermitTab* pTab, gboolean visible_beep)
{
    pTab->visible_beep = visible_beep;
    vte_terminal_set_visible_bell(VTE_TERMINAL(pTab->vte), visible_beep);
}

void hermit_tab_set_pos(struct HermitTab* pTab, int newPos)
{
    gint index = gtk_notebook_get_current_page(GTK_NOTEBOOK(hermit.notebook));
    gint pagesCnt = gtk_notebook_get_n_pages(GTK_NOTEBOOK(hermit.notebook));
    if (newPos < 0) {
        newPos = pagesCnt - 1;
    } else if (newPos >= pagesCnt) {
        newPos = 0;
    }
    gtk_notebook_reorder_child(GTK_NOTEBOOK(hermit.notebook), gtk_notebook_get_nth_page(GTK_NOTEBOOK(hermit.notebook), index), newPos);
}

void hermit_append_tab_with_details(const struct TabInfo* ti)
{
    struct HermitTab* pTab = g_malloc0(sizeof(struct HermitTab));
    hermit_style_copy(&pTab->style, &configs.style);
    if (ti->name) {
        pTab->tab_name = gtk_label_new(ti->name);
        pTab->custom_tab_name = TRUE;
    } else {
        gchar* label_text = g_strdup_printf("%s %d", configs.default_tab_name, hermit.tab_max_number++);
        pTab->tab_name = gtk_label_new(label_text);
        g_free(label_text);
        pTab->custom_tab_name = FALSE;
    }
    if (configs.tab_pos == GTK_POS_RIGHT) {
        gtk_label_set_angle(GTK_LABEL(pTab->tab_name), 270);
    } else if (configs.tab_pos == GTK_POS_LEFT) {
        gtk_label_set_angle(GTK_LABEL(pTab->tab_name), 90);
    }
    pTab->encoding = (ti->encoding) ? g_strdup(ti->encoding) : g_strdup(configs.default_encoding);
    pTab->bksp_binding = ti->bksp_binding;
    pTab->delete_binding = ti->delete_binding;
    pTab->cursor_blink_mode = ti->cursor_blink_mode;
    pTab->cursor_shape = ti->cursor_shape;
    pTab->hbox = gtk_hbox_new(FALSE, 0);
    pTab->vte = vte_terminal_new();
    VteTerminal* vte = VTE_TERMINAL(pTab->vte);

    vte_terminal_set_size(vte, configs.cols, configs.rows);
    vte_terminal_set_scrollback_lines(vte, configs.scrollback_lines);
    if (configs.default_word_chars)
        vte_terminal_set_word_chars(vte, configs.default_word_chars);
    vte_terminal_set_mouse_autohide(vte, TRUE);
    vte_terminal_set_backspace_binding(vte, pTab->bksp_binding);
    vte_terminal_set_delete_binding(vte, pTab->delete_binding);
    vte_terminal_set_cursor_blink_mode(vte, pTab->cursor_blink_mode);
    vte_terminal_set_cursor_shape(vte, pTab->cursor_shape);
    vte_terminal_search_set_wrap_around(vte, TRUE);

    guint l = 0;
    if (ti->argv == NULL) {
        l = 1;
        pTab->argv = (gchar**)g_new0(gchar*, 2);
        pTab->argv[0] = g_strdup(configs.default_command);
    } else {
        while (ti->argv[l] != NULL) {
            ++l;
        }
        pTab->argv = (gchar**)g_new0(gchar*, l + 1);
        guint i = 0;
        for (; i < l; ++i) {
            pTab->argv[i] = g_strdup(ti->argv[i]);
        }
    }
    g_assert(l >= 1);
    /* parse command */
    GError *cmd_err = NULL;
    if (l == 1) { // arguments may be in one compound string
        gchar **cmd_argv;
        if (!g_shell_parse_argv(pTab->argv[0], NULL, &cmd_argv, &cmd_err)) {
            ERROR("%s", _("Cannot parse command. Creating tab with shell"));
            g_error_free(cmd_err);
            return;
        }
        g_strfreev(pTab->argv);
        pTab->argv = cmd_argv;
    }
    gchar *cmd_path = NULL;
    cmd_path = g_find_program_in_path(pTab->argv[0]);

    TRACE("command=%s cmd_path=%s", pTab->argv[0], cmd_path);
    if (cmd_path != NULL) {
        g_free(pTab->argv[0]);
        pTab->argv[0] = g_strdup(cmd_path);
        g_free(cmd_path);
    }
#if VTE_CHECK_VERSION(0, 26, 0) > 0
    if (vte_terminal_fork_command_full(vte,
            VTE_PTY_DEFAULT,
            ti->working_dir,
            pTab->argv, NULL,
            0, NULL, NULL,
            &pTab->pid,
            &cmd_err) != TRUE) {
        ERROR("failed to open tab: %s", cmd_err->message);
        g_error_free(cmd_err);
        return;
    }
#else
    pTab->pid = vte_terminal_fork_command(vte,
            pTab->argv[0], pTab->argv + 1, NULL, ti->working_dir, TRUE, TRUE, TRUE);
#endif // version >= 0.26

    g_signal_connect(G_OBJECT(pTab->vte), "beep", G_CALLBACK(hermit_on_beep), pTab);
    g_signal_connect(G_OBJECT(pTab->vte), "focus-in-event", G_CALLBACK(hermit_on_focus), pTab);
    g_signal_connect(G_OBJECT(pTab->vte), "window-title-changed", G_CALLBACK(hermit_on_tab_title_changed), NULL);

    g_signal_connect(G_OBJECT(pTab->vte), "child-exited", G_CALLBACK(hermit_on_child_exited), NULL);
//    g_signal_connect(G_OBJECT(pTab->vte), "eof", G_CALLBACK(hermit_eof), NULL);
    g_signal_connect_swapped(G_OBJECT(pTab->vte), "button-press-event", G_CALLBACK(hermit_on_popup), NULL);

    vte_terminal_set_encoding(vte, pTab->encoding);

    pTab->matches = g_array_new(FALSE, TRUE, sizeof(struct Match));
    hermit_tab_add_matches(pTab, configs.matches);
    hermit_tab_set_transparency(pTab, pTab->style.transparency);
    vte_terminal_set_font(vte, pTab->style.font);

    gint index = gtk_notebook_append_page(GTK_NOTEBOOK(hermit.notebook), pTab->hbox, pTab->tab_name);
    if (index == -1) {
        ERROR("%s", _("Cannot create a new tab"));
        return;
    }
    if (configs.fill_tabbar) {
        GValue val = {};
        g_value_init(&val, G_TYPE_BOOLEAN);
        g_value_set_boolean(&val, TRUE);
        gtk_container_child_set_property(GTK_CONTAINER(hermit.notebook), pTab->hbox, "tab-expand", &val);
        gtk_container_child_set_property(GTK_CONTAINER(hermit.notebook), pTab->hbox, "tab-fill", &val);
    }

    hermit_tab_set_audible_beep(pTab, configs.audible_beep);
    hermit_tab_set_visible_beep(pTab, configs.visible_beep);

    pTab->scrollbar = gtk_vscrollbar_new(vte_terminal_get_adjustment(VTE_TERMINAL(pTab->vte)));

    gtk_box_pack_start(GTK_BOX(pTab->hbox), pTab->vte, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(pTab->hbox), pTab->scrollbar, FALSE, FALSE, 0);
    GtkWidget* tabWidget = gtk_notebook_get_nth_page(GTK_NOTEBOOK(hermit.notebook), index);
    if (!tabWidget) {
        ERROR("tabWidget is NULL");
        return;
    }
    g_object_set_data(G_OBJECT(tabWidget), HERMIT_TAB_DATA, pTab);

    if (index == 0) { // there is no "switch-page" signal on the first page
        hermit_set_statusbar_message(index);
    }
    pTab->scrollbar_is_shown = configs.show_scrollbar;
    gtk_widget_show_all(hermit.notebook);

    if (pTab->style.image_file == NULL) {
        vte_terminal_set_background_image(vte, NULL);
    } else {
        vte_terminal_set_background_image_file(vte, pTab->style.image_file);
    }
    hermit_tab_apply_colors(pTab);

    gtk_notebook_set_current_page(GTK_NOTEBOOK(hermit.notebook), index);
    gtk_notebook_set_tab_reorderable(GTK_NOTEBOOK(hermit.notebook), pTab->hbox, TRUE);
    gtk_window_set_focus(GTK_WINDOW(hermit.main_window), pTab->vte);

    hermit_check_tabbar();
    hermit_hide_scrollbars();
}

void hermit_append_tab_with_command(gchar** argv)
{
    struct TabInfo ti = {};
    ti.bksp_binding = configs.default_bksp;
    ti.delete_binding = configs.default_delete;
    ti.cursor_blink_mode = configs.default_blink;
    ti.cursor_shape = configs.default_shape;
    ti.argv = argv;
    hermit_append_tab_with_details(&ti);
}

void hermit_append_tab()
{
    hermit_append_tab_with_command(NULL);
}

void hermit_set_encoding(const gchar* encoding)
{
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(hermit.notebook));
    HERMIT_GET_TAB_BY_INDEX(pTab, page, return);
    TRACE("%s tab=%p page=%d encoding=%s", __FUNCTION__, pTab, page, encoding);
    vte_terminal_set_encoding(VTE_TERMINAL(pTab->vte), encoding);
    g_free(pTab->encoding);
    pTab->encoding = g_strdup(encoding);
    hermit_set_statusbar_message(page);
}

void hermit_tab_set_title(struct HermitTab* pTab, const gchar* title)
{
    gchar* tmp_title = g_strdup(title);
    if (configs.get_tab_title_callback) {
        gchar* lua_title = hermit_lua_getTitleCallback(configs.get_tab_title_callback, title);
        if (!lua_title) {
            ERROR("hermit_lua_getTitleCallback(%s) failed", title);
            g_free(tmp_title);
            return;
        }
        g_free(tmp_title);
        tmp_title = lua_title;
    }
    if (pTab->title)
        g_free(pTab->title);
    pTab->title = tmp_title;
    gtk_label_set_text(GTK_LABEL(pTab->tab_name), pTab->title);
    hermit_set_window_title(title);
}

void hermit_set_default_colors()
{
    gint page_num = gtk_notebook_get_n_pages(GTK_NOTEBOOK(hermit.notebook));
    gint i=0;
    for (; i<page_num; ++i) {
        HERMIT_GET_TAB_BY_INDEX(pTab, i, continue);
        vte_terminal_set_default_colors(VTE_TERMINAL(pTab->vte));
    }
}

void hermit_tab_set_font(struct HermitTab* pTab, const gchar* font_name)
{
    if (pTab->style.font_name) {
        g_free(pTab->style.font_name);
    }
    pTab->style.font_name = g_strdup(font_name);

    if (pTab->style.font) {
        pango_font_description_free(pTab->style.font);
    }
    pTab->style.font = pango_font_description_from_string(font_name);
    vte_terminal_set_font(VTE_TERMINAL(pTab->vte), pTab->style.font);
}

void hermit_tab_set_font_by_index(gint tab_index, const gchar* font_name)
{
    TRACE("%s: tab_index=%d font=%s", __FUNCTION__, tab_index, font_name);
    if (tab_index < 0) {
        tab_index = gtk_notebook_get_current_page(GTK_NOTEBOOK(hermit.notebook));
    }
    HERMIT_GET_TAB_BY_INDEX(pTab, tab_index, return);
    hermit_tab_set_font(pTab, font_name);
}

void hermit_tab_set_background_image(struct HermitTab* pTab, const gchar* image_file)
{
    TRACE("pTab->image_file=%s image_file=%s", pTab->style.image_file, image_file);
    if (pTab->style.image_file) {
        g_free(pTab->style.image_file);
    }
    if (image_file == NULL) {
        pTab->style.image_file = NULL;
        vte_terminal_set_background_transparent(VTE_TERMINAL(pTab->vte), TRUE);
        vte_terminal_set_background_image(VTE_TERMINAL(pTab->vte), NULL);
    } else {
        pTab->style.image_file = g_strdup(image_file);
        vte_terminal_set_background_transparent(VTE_TERMINAL(pTab->vte), FALSE);
        vte_terminal_set_background_image_file(VTE_TERMINAL(pTab->vte), pTab->style.image_file);
    }
}

static void hermit_set_color__(gint tab_index, const GdkColor* p_color, void (*callback)(struct HermitTab*, const GdkColor*))
{
    TRACE("%s: tab_index=%d color=%p", __FUNCTION__, tab_index, p_color);
    if (!p_color) {
        TRACE_MSG("p_color is NULL");
        return;
    }
    if (tab_index < 0) {
        tab_index = gtk_notebook_get_current_page(GTK_NOTEBOOK(hermit.notebook));
    }
    HERMIT_GET_TAB_BY_INDEX(pTab, tab_index, return);
    callback(pTab, p_color);
}

void hermit_tab_set_color_foreground_by_index(gint tab_index, const GdkColor* p_color)
{
    hermit_set_color__(tab_index, p_color, hermit_tab_set_color_foreground);
}

void hermit_tab_set_color_background_by_index(gint tab_index, const GdkColor* p_color)
{
    hermit_set_color__(tab_index, p_color, hermit_tab_set_color_background);
}

void hermit_paste()
{
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(hermit.notebook));
    HERMIT_GET_TAB_BY_INDEX(pTab, page, return);
    vte_terminal_paste_clipboard(VTE_TERMINAL(pTab->vte));
}

void hermit_copy()
{
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(hermit.notebook));
    HERMIT_GET_TAB_BY_INDEX(pTab, page, return);
    vte_terminal_copy_clipboard(VTE_TERMINAL(pTab->vte));
}

static void clipboard_received_text(GtkClipboard *clipboard, const gchar *text, gpointer data)
{
    if (text) {
        gchar** d = (gchar**)data;
        *d = g_strdup(text);
    }
}

gchar* hermit_get_selection()
{
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(hermit.notebook));
    HERMIT_GET_TAB_BY_INDEX(pTab, page, return NULL);
    if (vte_terminal_get_has_selection(VTE_TERMINAL(pTab->vte)) == FALSE) {
        return NULL;
    }
    GtkClipboard* clip = gtk_clipboard_get(GDK_SELECTION_PRIMARY);
    gchar* text = NULL;
    gtk_clipboard_request_text(clip, clipboard_received_text, &text);
    if (!text)
        return NULL;
    return text;
}

void hermit_close_tab()
{
    hermit_del_tab();
    if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(hermit.notebook)) == 0)
        hermit_quit();
}

void hermit_activate_tab(gint tab_index)
{
    if (tab_index < 0)
    {
        TRACE("tab_index(%d) < 0: skipping", tab_index);
        return;
    }
    if (tab_index >= gtk_notebook_get_n_pages(GTK_NOTEBOOK(hermit.notebook)))
    {
        TRACE("tab_index(%d) > n_pages: skipping", tab_index);
        return;
    }
    gtk_notebook_set_current_page(GTK_NOTEBOOK(hermit.notebook), tab_index);
}

void hermit_prev_tab()
{
    gint index = gtk_notebook_get_current_page(GTK_NOTEBOOK(hermit.notebook));
    index = (index) ? index - 1 : gtk_notebook_get_n_pages(GTK_NOTEBOOK(hermit.notebook)) - 1;
    hermit_activate_tab(index);
}

void hermit_next_tab()
{
    gint index = gtk_notebook_get_current_page(GTK_NOTEBOOK(hermit.notebook));
    index = (index == gtk_notebook_get_n_pages(GTK_NOTEBOOK(hermit.notebook)) - 1)
        ? 0 : index + 1;
    hermit_activate_tab(index);
}

void hermit_quit()
{
    while (gtk_notebook_get_n_pages(GTK_NOTEBOOK(hermit.notebook)) > 0)
        hermit_del_tab();
    
    gtk_main_quit();
}

int hermit_get_current_tab_index()
{
    return gtk_notebook_get_current_page(GTK_NOTEBOOK(hermit.notebook));
}

void hermit_set_kb_policy(enum HermitKbPolicy kbp)
{
    configs.kb_policy = kbp;
}

void hermit_set_window_title(const gchar* title)
{
    if (!title)
        return;
    if (!configs.get_window_title_callback) {
        gchar* window_title = g_strdup_printf("%s: %s", configs.default_window_title, title);
        gtk_window_set_title(GTK_WINDOW(hermit.main_window), window_title);
        g_free(window_title);
    } else {
        gchar* window_title = hermit_lua_getTitleCallback(configs.get_window_title_callback, title);
        if (!window_title) {
            ERROR("hermit_lua_getTitleCallback(%s) failed", title);
            return;
        }
        gtk_window_set_title(GTK_WINDOW(hermit.main_window), window_title);
        g_free(window_title);
    }
}

void hermit_set_show_scrollbar_signal(GtkWidget* menuItem, gpointer pHandlerId)
{
    gulong handlerId = g_signal_connect(G_OBJECT(menuItem), "toggled",
            G_CALLBACK(hermit_on_toggle_scrollbar), NULL);
    if (pHandlerId == NULL) {
        pHandlerId = g_malloc(sizeof(handlerId));
    }
    memcpy(pHandlerId, &handlerId, sizeof(handlerId));
    g_object_set_data(G_OBJECT(menuItem), "handlerId", pHandlerId);
}

