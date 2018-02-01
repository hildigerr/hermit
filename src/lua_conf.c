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
#include <X11/Xlib.h> // XParseGeometry
#include <glib.h>

#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "termit.h"
#include "termit_core_api.h"
#include "configs.h"
#include "keybindings.h"
#include "lua_api.h"

extern struct HermitData hermit;
extern struct Configs configs;

lua_State* L = NULL;

void hermit_lua_close()
{
    lua_close(L);
}

static void trace_menus(GArray* menus)
{
#ifdef DEBUG
    guint i = 0;
    for (; i<menus->len; ++i) {
        struct UserMenu* um = &g_array_index(menus, struct UserMenu, i);
        TRACE("%s items %d", um->name, um->items->len);
        guint j = 0;
        for (; j<um->items->len; ++j) {
            struct UserMenuItem* umi = &g_array_index(um->items, struct UserMenuItem, j);
            TRACE("  %s: (%d) [%s]", umi->name, umi->lua_callback, umi->accel);
        }
    }
#endif
}

void hermit_config_get_string(gchar** opt, lua_State* ls, int index)
{
    if (!lua_isnil(ls, index) && lua_isstring(ls, index)) {
        if (*opt)
            g_free(*opt);
        *opt = g_strdup(lua_tostring(ls, index));
    }
}

void hermit_config_get_double(double* opt, lua_State* ls, int index)
{
    if (!lua_isnil(ls, index) && lua_isnumber(ls, index))
        *opt = lua_tonumber(ls, index);
}

void hermit_config_getuint(guint* opt, lua_State* ls, int index)
{
    if (!lua_isnil(ls, index) && lua_isnumber(ls, index))
        *opt = lua_tointeger(ls, index);
}

void hermit_config_get_boolean(gboolean* opt, lua_State* ls, int index)
{
    if (!lua_isnil(ls, index) && lua_isboolean(ls, index))
        *opt = lua_toboolean(ls, index);
}

void hermit_config_get_function(int* opt, lua_State* ls, int index)
{
    if (!lua_isnil(ls, index) && lua_isfunction(ls, index)) {
        *opt = luaL_ref(ls, LUA_REGISTRYINDEX); // luaL_ref pops value so we restore stack size
        lua_pushnil(ls);
    }
}

void hermit_config_get_color(GdkColor** opt, lua_State* ls, int index)
{
    gchar* color_str = NULL;
    hermit_config_get_string(&color_str, ls, index);
    if (color_str) {
        GdkColor color = {};
        if (gdk_color_parse(color_str, &color) == TRUE) {
            *opt = gdk_color_copy(&color);
            TRACE("color_str=%s", color_str);
        }
    }
    g_free(color_str);
}

void hermit_config_get_erase_binding(VteTerminalEraseBinding* opt, lua_State* ls, int index)
{
    gchar* str = NULL;
    hermit_config_get_string(&str, ls, index);
    *opt = hermit_erase_binding_from_string(str);
    g_free(str);
}

void hermit_config_get_cursor_blink_mode(VteTerminalCursorBlinkMode* opt, lua_State* ls, int index)
{
    gchar* str = NULL;
    hermit_config_get_string(&str, ls, index);
    *opt = hermit_cursor_blink_mode_from_string(str);
    g_free(str);
}

void hermit_config_get_cursor_shape(VteTerminalCursorShape* opt, lua_State* ls, int index)
{
    gchar* str = NULL;
    hermit_config_get_string(&str, ls, index);
    *opt = hermit_cursor_shape_from_string(str);
    g_free(str);
}

static void matchesLoader(const gchar* pattern, struct lua_State* ls, int index, void* data)
{
    TRACE("pattern=%s index=%d data=%p", pattern, index, data);
    if (!lua_isfunction(ls, index)) {
        ERROR("match [%s] without function: skipping", pattern);
        return;
    }
    GArray* matches = (GArray*)data;
    struct Match match = {};
    GError* err = NULL;
    match.regex = g_regex_new(pattern, 0, 0, &err);
    if (err) {
        TRACE("failed to compile regex [%s]: skipping", pattern);
        return;
    }
    match.flags = 0;
    match.pattern = g_strdup(pattern);
    hermit_config_get_function(&match.lua_callback, ls, index);
    g_array_append_val(matches, match);
}

struct ColormapHelper
{
    GdkColor* colors;
    int idx;
};

static void colormapLoader(const gchar* name, lua_State* ls, int index, void* data)
{
    struct ColormapHelper* ch = (struct ColormapHelper*)data;
    if (!lua_isnil(ls, index) && lua_isstring(ls, index)) {
        const gchar* colorStr = lua_tostring(ls, index);
        if (!gdk_color_parse(colorStr, &(ch->colors[ch->idx]))) {
            ERROR("failed to parse color: %s %d - %s", name, ch->idx, colorStr);
        }
    } else {
        ERROR("invalid type in colormap: skipping");
    }
    ++ch->idx;
}

static void tabsLoader(const gchar* name, lua_State* ls, int index, void* data)
{
    if (lua_istable(ls, index)) {
        GArray* tabs = (GArray*)data;
        struct TabInfo ti = {};
        if (hermit_lua_load_table(ls, hermit_lua_tab_loader, index, &ti)
                != HERMIT_LUA_TABLE_LOADER_OK) {
            ERROR("failed to load tab: %s %s", name, lua_tostring(ls, 3));
        } else {
            g_array_append_val(tabs, ti);
        }
    } else {
        ERROR("unknown type instead if tab table: skipping");
        lua_pop(ls, 1);
    }
}

void hermit_lua_load_colormap(lua_State* ls, int index, GdkColor** colors, glong* sz)
{
    if (lua_isnil(ls, index) || !lua_istable(ls, index)) {
        ERROR("invalid colormap type");
        return;
    }
#if LUA_VERSION_NUM > 501
    const int size = lua_rawlen(ls, index);
#else
    const int size = lua_objlen(ls, index);
#endif // LUA_VERSION_NUM
    if ((size != 8) && (size != 16) && (size != 24)) {
        ERROR("bad colormap length: %d", size);
        return;
    }
    struct ColormapHelper ch = {};
    ch.colors = g_malloc0(size * sizeof(GdkColor));
    if (hermit_lua_load_table(ls, colormapLoader, index, &ch)
            == HERMIT_LUA_TABLE_LOADER_OK) {
        if (*colors) {
            g_free(*colors);
        }
        *colors = ch.colors;
        *sz = size;
    } else {
        ERROR("failed to load colormap");
        return;
    }
    TRACE("colormap loaded: size=%ld", *sz);
}

static void hermit_config_get_position(GtkPositionType* pos, lua_State* ls, int index)
{
    if (!lua_isnil(ls, index) && lua_isstring(ls, index)) {
        const char* str = lua_tostring(ls, index);
        if (strcmp(str, "Top") == 0) {
            *pos = GTK_POS_TOP;
        } else if (strcmp(str, "Bottom") == 0) {
            *pos = GTK_POS_BOTTOM;
        } else if (strcmp(str, "Left") == 0) {
            *pos = GTK_POS_LEFT;
        } else if (strcmp(str, "Right") == 0) {
            *pos = GTK_POS_RIGHT;
        } else {
            ERROR("unknown pos: [%s]", str);
        }
    }
}

void hermit_lua_options_loader(const gchar* name, lua_State* ls, int index, void* data)
{
    struct Configs* p_cfg = (struct Configs*)data;
    if (!strcmp(name, "tabName"))
        hermit_config_get_string(&(p_cfg->default_tab_name), ls, index);
    else if (!strcmp(name, "windowTitle"))
        hermit_config_get_string(&(p_cfg->default_window_title), ls, index);
    else if (!strcmp(name, "encoding"))
        hermit_config_get_string(&(p_cfg->default_encoding), ls, index);
    else if (!strcmp(name, "wordChars"))
        hermit_config_get_string(&(p_cfg->default_word_chars), ls, index);
    else if (!strcmp(name, "font"))
        hermit_config_get_string(&(p_cfg->style.font_name), ls, index);
    else if (!strcmp(name, "foregroundColor")) 
        hermit_config_get_color(&p_cfg->style.foreground_color, ls, index);
    else if (!strcmp(name, "backgroundColor")) 
        hermit_config_get_color(&p_cfg->style.background_color, ls, index);
    else if (!strcmp(name, "showScrollbar"))
        hermit_config_get_boolean(&(p_cfg->show_scrollbar), ls, index);
    else if (!strcmp(name, "transparency"))
        hermit_config_get_double(&(p_cfg->style.transparency), ls, index);
    else if (!strcmp(name, "imageFile"))
        hermit_config_get_string(&(p_cfg->style.image_file), ls, index);
    else if (!strcmp(name, "fillTabbar"))
        hermit_config_get_boolean(&(p_cfg->fill_tabbar), ls, index);
    else if (!strcmp(name, "hideSingleTab"))
        hermit_config_get_boolean(&(p_cfg->hide_single_tab), ls, index);
    else if (!strcmp(name, "topMenu"))
        hermit_config_get_boolean(&(p_cfg->top_menu), ls, index);
    else if (!strcmp(name, "hideMenubar"))
        hermit_config_get_boolean(&(p_cfg->hide_menubar), ls, index);
    else if (!strcmp(name, "hideTabbar"))
        hermit_config_get_boolean(&(p_cfg->hide_tabbar), ls, index);
    else if (!strcmp(name, "showBorder"))
        hermit_config_get_boolean(&(p_cfg->show_border), ls, index);
    else if (!strcmp(name, "scrollbackLines"))
        hermit_config_getuint(&(p_cfg->scrollback_lines), ls, index);
    else if (!strcmp(name, "allowChangingTitle"))
        hermit_config_get_boolean(&(p_cfg->allow_changing_title), ls, index);
    else if (!strcmp(name, "audibleBell"))
        hermit_config_get_boolean(&(p_cfg->audible_bell), ls, index);
    else if (!strcmp(name, "visibleBell"))
        hermit_config_get_boolean(&(p_cfg->visible_bell), ls, index);
    else if (!strcmp(name, "urgencyOnBell"))
        hermit_config_get_boolean(&(p_cfg->urgency_on_bell), ls, index);
    else if (!strcmp(name, "getWindowTitle"))
        hermit_config_get_function(&(p_cfg->get_window_title_callback), ls, index);
    else if (!strcmp(name, "tabPos"))
        hermit_config_get_position(&(p_cfg->tab_pos), ls, index);
    else if (!strcmp(name, "getTabTitle"))
        hermit_config_get_function(&(p_cfg->get_tab_title_callback), ls, index);
    else if (!strcmp(name, "setStatusbar"))
        hermit_config_get_function(&(p_cfg->get_statusbar_callback), ls, index);
    else if (!strcmp(name, "backspaceBinding"))
        hermit_config_get_erase_binding(&(p_cfg->default_bksp), ls, index);
    else if (!strcmp(name, "deleteBinding"))
        hermit_config_get_erase_binding(&(p_cfg->default_delete), ls, index);
    else if (!strcmp(name, "cursorBlinkMode"))
        hermit_config_get_cursor_blink_mode(&(p_cfg->default_blink), ls, index);
    else if (!strcmp(name, "cursorShape"))
        hermit_config_get_cursor_shape(&(p_cfg->default_shape), ls, index);
    else if (!strcmp(name, "colormap")) {
        hermit_lua_load_colormap(ls, index, &configs.style.colors, &configs.style.colors_size);
    } else if (!strcmp(name, "matches")) {
        if (hermit_lua_load_table(ls, matchesLoader, index, configs.matches)
                != HERMIT_LUA_TABLE_LOADER_OK) {
            ERROR("failed to load matches");
        }
    } else if (!strcmp(name, "geometry")) {
        gchar* geometry_str = NULL;
        hermit_config_get_string(&geometry_str, ls, index);
        if (geometry_str) {
            unsigned int cols = 0, rows = 0;
            int tmp1 = 0, tmp2 = 0;
            XParseGeometry(geometry_str, &tmp1, &tmp2, &cols, &rows);
            if ((cols != 0) && (rows != 0)) {
                p_cfg->cols = cols;
                p_cfg->rows = rows;
            }
        }
        g_free(geometry_str);
    } else if (!strcmp(name, "tabs")) {
        if (lua_istable(ls, index)) {
            if (!configs.default_tabs) {
                configs.default_tabs = g_array_new(FALSE, TRUE, sizeof(struct TabInfo));
            }
            TRACE("tabs at index: %d tabs.size=%d", index, configs.default_tabs->len);
            if (hermit_lua_load_table(ls, tabsLoader, index, configs.default_tabs)
                    != HERMIT_LUA_TABLE_LOADER_OK) {
                ERROR("openTab failed");
            }
        } else {
            ERROR("expecting table");
        }
    }
}

static void hermit_lua_add_package_path(const gchar* path)
{
    gchar* luaCmd = g_strdup_printf("package.path = package.path .. \";%s/?.lua\"", path);
    int s = luaL_dostring(L, luaCmd);
    hermit_lua_report_error(__FILE__, __LINE__, s);
    g_free(luaCmd);
}

static gchar** hermit_system_path()
{
    const gchar *configSystem = g_getenv("XDG_CONFIG_DIRS");
    gchar* xdgConfigDirs = NULL;
    if (configSystem) {
        xdgConfigDirs = g_strdup_printf("%s:/etc/xdg", configSystem);
    } else {
        xdgConfigDirs = g_strdup("/etc/xdg");
    }
    gchar** systemPaths = g_strsplit(xdgConfigDirs, ":", 0);
    g_free(xdgConfigDirs);
    return systemPaths;
}

static gchar* hermit_user_path()
{
    const gchar *configHome = g_getenv("XDG_CONFIG_HOME");
    if (configHome)
        return g_strdup(configHome);
    else
        return g_strdup_printf("%s/.config", g_getenv("HOME"));
}

static void load_init(const gchar* initFile)
{
    const gchar *configFile = "rc.lua";
    gchar** systemPaths = hermit_system_path();
    guint i = 0;
    gchar* systemPath = systemPaths[i];
    while (systemPath) {
        if (g_file_test(systemPath, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR) == TRUE) {
            hermit_lua_add_package_path(systemPath);
        }
        systemPath = systemPaths[++i];
    }
    gchar* userPath = hermit_user_path();
    hermit_lua_add_package_path(userPath);

    gchar* fullPath = NULL;
    if (initFile) {
        fullPath = g_strdup(initFile);
    } else {
        fullPath = g_strdup_printf("%s/hermit/%s", userPath, configFile);
        if (g_file_test(fullPath, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR) == FALSE) {
            TRACE("%s not found", fullPath);
            g_free(fullPath);

            i = 0;
            gchar* systemPath = systemPaths[i];
            while (systemPath) {
                fullPath = g_strdup_printf("%s/hermit/%s", systemPath, configFile);
                if (g_file_test(fullPath, G_FILE_TEST_EXISTS | G_FILE_TEST_IS_REGULAR) == FALSE) {
                    TRACE("%s not found", fullPath);
                    g_free(fullPath);
                    fullPath = NULL;
                } else {
                    break;
                }
                systemPath = systemPaths[++i];
            }
        }
    }
    g_strfreev(systemPaths);
    g_free(userPath);
    if (fullPath) {
        TRACE("using config: %s", fullPath);
        int s = luaL_loadfile(L, fullPath);
        hermit_lua_report_error(__FILE__, __LINE__, s);
        g_free(fullPath);

        s = lua_pcall(L, 0, LUA_MULTRET, 0);
        hermit_lua_report_error(__FILE__, __LINE__, s);
    } else {
        ERROR("config file %s not found", configFile);
    }
}

int hermit_lua_fill_tab(int tab_index, lua_State* ls)
{
    HERMIT_GET_TAB_BY_INDEX(pTab, tab_index, return 0);
    lua_newtable(ls);
    HERMIT_TAB_ADD_STRING("title", pTab->title);
    HERMIT_TAB_ADD_STRING("command", pTab->argv[0]);
    HERMIT_TAB_ADD_STRING("argv", "");
    // FIXME: add argv
    HERMIT_TAB_ADD_STRING("encoding", pTab->encoding);
    gchar* working_dir = hermit_get_pid_dir(pTab->pid);
    HERMIT_TAB_ADD_STRING("workingDir", working_dir);
    g_free(working_dir);
    HERMIT_TAB_ADD_NUMBER("pid", pTab->pid);
    HERMIT_TAB_ADD_STRING("font", pTab->style.font_name);
    HERMIT_TAB_ADD_NUMBER("fontSize", pango_font_description_get_size(pTab->style.font)/PANGO_SCALE);
    HERMIT_TAB_ADD_STRING("backspaceBinding", hermit_erase_binding_to_string(pTab->bksp_binding));
    HERMIT_TAB_ADD_STRING("deleteBinding", hermit_erase_binding_to_string(pTab->delete_binding));
    return 1;
}

static int hermit_lua_tabs_index(lua_State* ls)
{
    if (lua_isnumber(ls, 1)) {
        TRACE_MSG("index is not number: skipping");
        return 0;
    }
    int tab_index =  lua_tointeger(ls, -1);
    TRACE("tab_index:%d", tab_index);
    return hermit_lua_fill_tab(tab_index - 1, ls);
}

static int hermit_lua_tabs_newindex(lua_State* ls)
{
    ERROR("'tabs' is read-only variable");
    return 0;
}

static void hermit_lua_init_tabs()
{
    lua_newtable(L);
    luaL_newmetatable(L, "tabs_meta");
    lua_pushcfunction(L, hermit_lua_tabs_index);
    lua_setfield(L, -2, "__index");
    lua_pushcfunction(L, hermit_lua_tabs_newindex);
    lua_setfield(L, -2, "__newindex");
    lua_setmetatable(L, -2);
    lua_setglobal(L, "tabs");
}

static const gchar* hermit_init_file = NULL;

void hermit_lua_load_config()
{
    load_init(hermit_init_file);
    hermit_config_trace();

    trace_menus(configs.user_menus);
    trace_menus(configs.user_popup_menus);
}

void hermit_lua_init(const gchar* initFile)
{
    L = luaL_newstate();
    luaL_openlibs(L);

    if (!hermit_init_file)
        hermit_init_file = g_strdup(initFile);
    hermit_lua_init_tabs();
    hermit_lua_init_api();
    hermit_keys_set_defaults();
    hermit_lua_load_config();
}

