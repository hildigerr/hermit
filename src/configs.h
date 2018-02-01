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

#ifndef CONFIGS_H
#define CONFIGS_H

#include <gtk/gtk.h>
#include <gdk/gdk.h>

#include "termit_style.h"

enum HermitKbPolicy {HermitKbUseKeycode = 1, HermitKbUseKeysym = 2};

struct Configs
{
    gchar* default_window_title;
    gchar* default_tab_name;
    gchar* default_command;
    gchar* default_encoding;
    gchar* default_word_chars;
    guint scrollback_lines;
    guint cols;
    guint rows;
    VteTerminalEraseBinding default_bksp;
    VteTerminalEraseBinding default_delete;
    VteTerminalCursorBlinkMode default_blink;
    VteTerminalCursorShape default_shape;
    GArray* user_menus;         // UserMenu
    GArray* user_popup_menus;   // UserMenu
    GArray* key_bindings;       // KeyBinding
    GArray* mouse_bindings;     // MouseBinding
    GArray* matches;            // Match
    gboolean hide_single_tab;
    gboolean show_scrollbar;
    gboolean top_menu;
    gboolean hide_menubar;
    gboolean hide_tabbar;
    gboolean fill_tabbar;
    gboolean show_border;
    gboolean urgency_on_beep;
    gboolean allow_changing_title;
    gboolean audible_beep;
    gboolean visible_beep;
    int get_window_title_callback;
    int get_tab_title_callback;
    int get_statusbar_callback;
    enum HermitKbPolicy kb_policy;
    GtkPositionType tab_pos;
    struct HermitStyle style;
    GArray* default_tabs;       // TabInfo
};

struct Match
{
    gchar* pattern;
    GRegex* regex;
    GRegexMatchFlags flags;
    int tag;
    int lua_callback;
};
struct UserMenuItem
{
    gchar* name;
    gchar* accel;
    int lua_callback;
};
struct UserMenu
{
    gchar* name;
    GArray* items; // UserMenuItem
};

extern struct Configs configs;

void hermit_config_deinit();
void hermit_configs_set_defaults();
void hermit_config_load();

void hermit_config_trace();
void hermit_keys_trace();

const char* hermit_erase_binding_to_string(VteTerminalEraseBinding val);
VteTerminalEraseBinding hermit_erase_binding_from_string(const char* str);

const char* hermit_cursor_blink_mode_to_string(VteTerminalCursorBlinkMode val);
VteTerminalCursorBlinkMode hermit_cursor_blink_mode_from_string(const char* str);

const char* hermit_cursor_shape_to_string(VteTerminalCursorShape val);
VteTerminalCursorShape hermit_cursor_shape_from_string(const char* str);

#define HERMIT_USER_MENU_ITEM_DATA "hermit.umi_data"
#define HERMIT_TAB_DATA "hermit.tab_data"

#endif /* CONFIGS_H */

