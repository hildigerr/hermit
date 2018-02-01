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
#include <lua.h>
#include <lualib.h>
#include <lauxlib.h>

#include "termit.h"
#include "termit_core_api.h"
#include "keybindings.h"
#include "configs.h"
#include "callbacks.h"
#include "lua_api.h"

extern lua_State* L;

HermitLuaTableLoaderResult hermit_lua_load_table(lua_State* ls, HermitLuaTableLoaderFunc func,
        const int tableIndex, void* data)
{
    if (!lua_istable(ls, tableIndex)) {
        ERROR("not a table");
        return HERMIT_LUA_TABLE_LOADER_FAILED;
    }

    lua_pushnil(ls);
    while (lua_next(ls, tableIndex) != 0) {
        if (lua_isnumber(ls, tableIndex + 1)) {
            func("0", ls, tableIndex + 2, data);
        } else if (lua_isstring(ls, tableIndex + 1)) {
            const char* name = lua_tostring(ls, tableIndex + 1);
            func(name, ls, tableIndex + 2, data);
        } else {
            ERROR("neither number nor string value found - skipping");
            lua_pop(ls, 1);
        }
        lua_pop(ls, 1);
    }
    return HERMIT_LUA_TABLE_LOADER_OK;
}


void hermit_lua_execute(const gchar* cmd)
{
    TRACE("executing script: %s", cmd);
    int s = luaL_dostring(L, cmd);
    hermit_lua_report_error(__FILE__, __LINE__, s);
}

void hermit_lua_report_error(const char* file, int line, int status)
{
    if (status == 0)
        return;
    TRACE_MSG("lua error:");
    g_fprintf(stderr, "%s:%d %s\n", file, line, lua_tostring(L, -1));
    lua_pop(L, 1);
}

static int hermit_lua_setOptions(lua_State* ls)
{
    TRACE_MSG(__FUNCTION__);
    hermit_lua_load_table(ls, hermit_lua_options_loader, 1, &configs);
    if (configs.default_tabs) {
        guint i = 0;
        for (; i < configs.default_tabs->len; ++i) {
            struct TabInfo* ti = &g_array_index(configs.default_tabs, struct TabInfo, i);
            hermit_append_tab_with_details(ti);
            g_free(ti->name);
            g_strfreev(ti->argv);
            g_free(ti->encoding);
            g_free(ti->working_dir);
        }
        g_array_free(configs.default_tabs, TRUE);
        configs.default_tabs = NULL;
    }
    return 0;
}

int hermit_lua_dofunction(int f)
{
    lua_State* ls = L;
    if(f != LUA_REFNIL) {
        lua_rawgeti(ls, LUA_REGISTRYINDEX, f);
        if (lua_pcall(ls, 0, 0, 0)) {
            TRACE("error running function (%d): %s", f, lua_tostring(ls, -1));
            lua_pop(ls, 1);
            return 0;
        }
        return 1;
    }
    return 0;
}

int hermit_lua_dofunction2(int f, const char* arg1)
{
    lua_State* ls = L;
    if(f != LUA_REFNIL) {
        lua_rawgeti(ls, LUA_REGISTRYINDEX, f);
        lua_pushstring(ls, arg1);
        if (lua_pcall(ls, 1, 0, 0)) {
            TRACE("error running function (%d): %s", f, lua_tostring(ls, -1));
            lua_pop(ls, 1);
            return 0;
        }
        return 1;
    }
    return 0;
}

int hermit_lua_domatch(int f, const gchar* matchedText)
{
    lua_State* ls = L;
    if(f != LUA_REFNIL) {
        lua_rawgeti(ls, LUA_REGISTRYINDEX, f);
        lua_pushstring(ls, matchedText);
        if (lua_pcall(ls, 1, 0, 0)) {
            TRACE("error running function (%d: %s): %s", f, matchedText, lua_tostring(ls, -1));
            lua_pop(ls, 1);
            return 0;
        }
        return 1;
    }
    return 0;
}

void hermit_lua_unref(int* lua_callback)
{
    if (*lua_callback) {
        luaL_unref(L, LUA_REGISTRYINDEX, *lua_callback);
        *lua_callback = 0;
    }
}

gchar* hermit_lua_getStatusbarCallback(int f, guint page)
{
    lua_State* ls = L;
    if(f != LUA_REFNIL) {
        lua_rawgeti(ls, LUA_REGISTRYINDEX, f);
        lua_pushnumber(ls, page);
        if (lua_pcall(ls, 1, 1, 0)) {
            TRACE("error running function: %s", lua_tostring(ls, -1));
            lua_pop(ls, 1);
            return NULL;
        }
        if (lua_isstring(ls, -1))
            return g_strdup(lua_tostring(ls, -1));
    }
    return NULL;
}

gchar* hermit_lua_getTitleCallback(int f, const gchar* title)
{
    lua_State* ls = L;
    if(f != LUA_REFNIL) {
        lua_rawgeti(ls, LUA_REGISTRYINDEX, f);
        lua_pushstring(ls, title);
        if (lua_pcall(ls, 1, 1, 0)) {
            TRACE("error running function: %s", lua_tostring(ls, -1));
            lua_pop(ls, 1);
            return NULL;
        }
        if (lua_isstring(ls, -1))
            return g_strdup(lua_tostring(ls, -1));
    }
    return NULL;
}

static int hermit_lua_bindKey(lua_State* ls)
{
    TRACE_MSG(__FUNCTION__);
    if (lua_isnil(ls, 1)) {
        TRACE_MSG("nil args: skipping");
    } else if (!lua_isstring(ls, 1)) {
        TRACE_MSG("bad args: skipping");
    } else if (lua_isnil(ls, 2)) {
        const char* keybinding = lua_tostring(ls, 1);
        hermit_keys_unbind(keybinding);
        TRACE("unbindKey: %s", keybinding);
    } else if (lua_isfunction(ls, 2)) {
        const char* keybinding = lua_tostring(ls, 1);
        int func = luaL_ref(ls, LUA_REGISTRYINDEX);
        hermit_keys_bind(keybinding, func);
        TRACE("bindKey: %s - %d", keybinding, func);
    }
    return 0;
}

static int hermit_lua_bindMouse(lua_State* ls)
{
    if (lua_isnil(ls, 1)) {
        TRACE_MSG("nil args: skipping");
    } else if (!lua_isstring(ls, 1)) {
        TRACE_MSG("bad args: skipping");
    } else if (lua_isnil(ls, 2)) {
        const char* mousebinding = lua_tostring(ls, 1);
        hermit_mouse_unbind(mousebinding);
        TRACE("unbindMouse: %s", mousebinding);
    } else if (lua_isfunction(ls, 2)) {
        const char* mousebinding = lua_tostring(ls, 1);
        int func = luaL_ref(ls, LUA_REGISTRYINDEX);
        hermit_mouse_bind(mousebinding, func);
        TRACE("bindMouse: %s - %d", mousebinding, func);
    }
    return 0;
}

static int hermit_lua_toggleMenubar(lua_State* ls)
{
    hermit_toggle_menubar();
    return 0;
}

static int hermit_lua_toggleTabbar(lua_State* ls)
{
    hermit_toggle_tabbar();
    return 0;
}

void hermit_lua_tab_loader(const gchar* name, lua_State* ls, int index, void* data)
{
    struct TabInfo* ti = (struct TabInfo*)data;
    if (strcmp(name, "title") == 0) {
        hermit_config_get_string(&ti->name, ls, index);
    } else if (strcmp(name, "command") == 0) {
        if (ti->argv == NULL) {
            ti->argv = (gchar**)g_new0(gchar*, 2);
        }
        hermit_config_get_string(&ti->argv[0], ls, index);
    } else if (strcmp(name, "encoding") == 0) {
        hermit_config_get_string(&ti->encoding, ls, index);
    } else if (strcmp(name, "backspaceBinding") == 0) {
        hermit_config_get_erase_binding(&ti->bksp_binding, ls, index);
    } else if (strcmp(name, "deleteBinding") == 0) {
        hermit_config_get_erase_binding(&ti->delete_binding, ls, index);
    } else if (strcmp(name, "cursorBlinkMode") == 0) {
        hermit_config_get_cursor_blink_mode(&ti->cursor_blink_mode, ls, index);
    } else if (strcmp(name, "cursorShape") == 0) {
        hermit_config_get_cursor_shape(&ti->cursor_shape, ls, index);
    } else if (strcmp(name, "workingDir") == 0) {
        hermit_config_get_string(&ti->working_dir, ls, index);
    }
}

static int hermit_lua_nextTab(lua_State* ls)
{
    TRACE_MSG(__FUNCTION__);
    hermit_next_tab();
    return 0;
}

static int hermit_lua_prevTab(lua_State* ls)
{
    TRACE_MSG(__FUNCTION__);
    hermit_prev_tab();
    return 0;
}

static int hermit_lua_copy(lua_State* ls)
{
    TRACE_MSG(__FUNCTION__);
    hermit_copy();
    return 0;
}

static int hermit_lua_paste(lua_State* ls)
{
    TRACE_MSG(__FUNCTION__);
    hermit_paste();
    return 0;
}

static int hermit_lua_openTab(lua_State* ls)
{
    TRACE_MSG(__FUNCTION__);
    if (lua_istable(ls, 1)) {
        struct TabInfo ti = {};
        if (hermit_lua_load_table(ls, hermit_lua_tab_loader, 1, &ti)
                != HERMIT_LUA_TABLE_LOADER_OK) {
            ERROR("openTab failed");
            return 0;
        }
        hermit_append_tab_with_details(&ti);
        g_free(ti.name);
        g_strfreev(ti.argv);
        g_free(ti.encoding);
        g_free(ti.working_dir);
    } else {
        hermit_append_tab();
    }
    return 0;
}

static int hermit_lua_closeTab(lua_State* ls)
{
    hermit_close_tab();
    TRACE_FUNC;
    return 0;
}

static int hermit_lua_setKbPolicy(lua_State* ls)
{
    if (lua_isnil(ls, 1)) {
        TRACE_MSG("no kbPolicy defined: skipping");
        return 0;
    } else if (!lua_isstring(ls, 1)) {
        TRACE_MSG("kbPolicy is not string: skipping");
        return 0;
    }
    const gchar* val = lua_tostring(ls, 1);
    TRACE("setKbPolicy: %s", val);
    if (!strcmp(val, "keycode"))
        hermit_set_kb_policy(HermitKbUseKeycode);
    else if (!strcmp(val, "keysym"))
        hermit_set_kb_policy(HermitKbUseKeysym);
    else {
        ERROR("unknown kbPolicy: %s", val);
    }
    return 0;
}

static void menuItemLoader(const gchar* name, lua_State* ls, int index, void* data)
{
    struct UserMenuItem* umi = (struct UserMenuItem*)data;
    if (!strcmp(name, "name") && lua_isstring(ls, index)) {
        const gchar* value = lua_tostring(ls, index);
        umi->name = g_strdup(value);
    } else if (!strcmp(name, "action") && lua_isfunction(ls, index)) {
        umi->lua_callback = luaL_ref(ls, LUA_REGISTRYINDEX);
        lua_pushnil(ls); // luaL_ref pops value so we restore stack size
    } else if (!strcmp(name, "accel") && lua_isstring(ls, index)) {
        const gchar* value = lua_tostring(ls, index);
        umi->accel = g_strdup(value);
    }
}

static void menuLoader(const gchar* name, lua_State* ls, int index, void* data)
{
    struct UserMenu* um = (struct UserMenu*)data;
    if (lua_istable(ls, index)) {
        struct UserMenuItem umi = {};
        if (hermit_lua_load_table(ls, menuItemLoader, index, &umi) == HERMIT_LUA_TABLE_LOADER_OK) {
            g_array_append_val(um->items, umi);
        } else {
            ERROR("failed to load item: %s.%s", name, lua_tostring(ls, 3));
        }
    } else {
        ERROR("unknown type instead if menu table: skipping");
        lua_pop(ls, 1);
    }
}

static int loadMenu(lua_State* ls, GArray* menus)
{
    if (lua_isnil(ls, 1) || lua_isnil(ls, 2)) {
        TRACE_MSG("menu not defined: skipping");
        return -1;
    } else if (!lua_istable(ls, 1) || !lua_isstring(ls, 2)) {
        TRACE_MSG("menu is not table: skipping");
        return -1;
    }
    struct UserMenu um = {};
    um.name = g_strdup(lua_tostring(ls, 2));
    lua_pop(ls, 1);
    um.items = g_array_new(FALSE, TRUE, sizeof(struct UserMenuItem));
    if (hermit_lua_load_table(ls, menuLoader, 1, &um) != HERMIT_LUA_TABLE_LOADER_OK) {
        ERROR("addMenu failed");
    }
    if (um.items->len > 0) {
        g_array_append_val(menus, um);
    } else {
        g_free(um.name);
        g_array_free(um.items, TRUE);
    }
    return 0;
}

static int hermit_lua_addMenu(lua_State* ls)
{
    if (loadMenu(ls, configs.user_menus) < 0) {
        ERROR("addMenu failed");
    }
    return 0;
}

static int hermit_lua_addPopupMenu(lua_State* ls)
{
    if (loadMenu(ls, configs.user_popup_menus) < 0) {
        ERROR("addMenu failed");
    }
    return 0;
}

static int hermit_lua_setColormap(lua_State* ls)
{
    const gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(hermit.notebook));
    HERMIT_GET_TAB_BY_INDEX(pTab, page, return 0);
    hermit_lua_load_colormap(ls, 1, &pTab->style.colors, &pTab->style.colors_size);
    hermit_tab_apply_colors(pTab);
    return 0;
}

static int hermit_lua_setEncoding(lua_State* ls)
{
    if (lua_isnil(ls, 1)) {
        TRACE_MSG("no encoding defined: skipping");
        return 0;
    } else if (!lua_isstring(ls, 1)) {
        TRACE_MSG("encoding is not string: skipping");
        return 0;
    }
    const gchar* val = lua_tostring(ls, 1);
    hermit_set_encoding(val);
    return 0;
}

static int hermit_lua_activateTab(lua_State* ls)
{
    if (lua_isnil(ls, 1)) {
        TRACE_MSG("no tabNum defined: skipping");
        return 0;
    } else if (!lua_isnumber(ls, 1)) {
        TRACE_MSG("tabNum is not number: skipping");
        return 0;
    }
    int tab_index =  lua_tointeger(ls, 1);
    hermit_activate_tab(tab_index - 1);
    return 0;
}

static int hermit_lua_currentTab(lua_State* ls)
{
    return hermit_lua_fill_tab(hermit_get_current_tab_index(), ls);
}

static int hermit_lua_currentTabIndex(lua_State* ls)
{
    lua_pushinteger(ls, hermit_get_current_tab_index() + 1);
    return 1;
}

static int hermit_lua_loadSessionDialog(lua_State* ls)
{
    hermit_on_load_session();
    return 0;
}

static int hermit_lua_saveSessionDialog(lua_State* ls)
{
    hermit_on_save_session();
    return 0;
}

static int hermit_lua_preferencesDialog(lua_State* ls)
{
    hermit_on_edit_preferences();
    return 0;
}

static int hermit_lua_setTabTitleDialog(lua_State* ls)
{
    hermit_on_set_tab_name();
    return 0;
}

static int hermit_lua_setTabPos(lua_State* ls)
{
    if (lua_isnil(ls, 1)) {
        TRACE_MSG("no tabNum defined: skipping");
        return 0;
    } else if (!lua_isnumber(ls, 1)) {
        TRACE_MSG("tabNum is not number: skipping");
        return 0;
    }
    int tab_index =  lua_tointeger(ls, 1);
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(hermit.notebook));
    HERMIT_GET_TAB_BY_INDEX(pTab, page, return 0);
    hermit_tab_set_pos(pTab, tab_index - 1);
    return 0;
}

static int hermit_lua_setTabTitle(lua_State* ls)
{
    if (lua_isnil(ls, 1)) {
        TRACE_MSG("no tabName defined: skipping");
        return 0;
    } else if (!lua_isstring(ls, 1)) {
        TRACE_MSG("tabName is not string: skipping");
        return 0;
    }
    const gchar* val = lua_tostring(ls, 1);
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(hermit.notebook));
    HERMIT_GET_TAB_BY_INDEX(pTab, page, return 0);
    hermit_tab_set_title(pTab, val);
    pTab->custom_tab_name = TRUE;
    return 0;
}

static int hermit_lua_setWindowTitle(lua_State* ls)
{
    if (lua_isnil(ls, 1)) {
        TRACE_MSG("no title defined: skipping");
        return 0;
    } else if (!lua_isstring(ls, 1)) {
        TRACE_MSG("title is not string: skipping");
        return 0;
    }
    const gchar* val = lua_tostring(ls, 1);
    hermit_set_window_title(val);
    return 0;
}

static int hermit_lua_setTabColor__(lua_State* ls, void (*callback)(gint, const GdkColor*))
{
    if (lua_isnil(ls, 1)) {
        TRACE_MSG("no color defined: skipping");
        return 0;
    } else if (!lua_isstring(ls, 1)) {
        TRACE_MSG("color is not string: skipping");
        return 0;
    }
    const gchar* val = lua_tostring(ls, 1);
    GdkColor color;
    if (gdk_color_parse(val, &color) == TRUE)
        callback(-1, &color);
    return 0;
}

static int hermit_lua_setTabForegroundColor(lua_State* ls)
{
    return hermit_lua_setTabColor__(ls, &hermit_tab_set_color_foreground_by_index);
}

static int hermit_lua_setTabBackgroundColor(lua_State* ls)
{
    return hermit_lua_setTabColor__(ls, &hermit_tab_set_color_background_by_index);
}

static int hermit_lua_setTabFont(lua_State* ls)
{
    if (lua_isnil(ls, 1)) {
        TRACE_MSG("no font defined: skipping");
        return 0;
    } else if (!lua_isstring(ls, 1)) {
        TRACE_MSG("font is not string: skipping");
        return 0;
    }
    const gchar* val = lua_tostring(ls, 1);
    hermit_tab_set_font_by_index(-1, val);
    return 0;
}

static int hermit_lua_spawn(lua_State* ls)
{
    if (lua_isnil(ls, 1)) {
        TRACE_MSG("no command defined: skipping");
        return 0;
    } else if (!lua_isstring(ls, 1)) {
        TRACE_MSG("command is not string: skipping");
        return 0;
    }
    GError *err = NULL;
    const gchar* val = lua_tostring(ls, 1);
    g_spawn_command_line_async(val, &err);
    return 0;
}

static int hermit_lua_forEachRow(lua_State* ls)
{
    if (lua_isnil(ls, 1)) {
        TRACE_MSG("no function defined: skipping");
        return 0;
    } else if (!lua_isfunction(ls, 1)) {
        TRACE_MSG("1 arg is not function: skipping");
        return 0;
    }
    int func = luaL_ref(ls, LUA_REGISTRYINDEX);
    hermit_for_each_row(func);
    hermit_lua_unref(&func);
    return 0;
}

static int hermit_lua_forEachVisibleRow(lua_State* ls)
{
    if (lua_isnil(ls, 1)) {
        TRACE_MSG("no function defined: skipping");
        return 0;
    } else if (!lua_isfunction(ls, 1)) {
        TRACE_MSG("1 arg is not function: skipping");
        return 0;
    }
    int func = luaL_ref(ls, LUA_REGISTRYINDEX);
    hermit_for_each_visible_row(func);
    hermit_lua_unref(&func);
    return 0;
}

static int hermit_lua_feed(lua_State* ls)
{
    if (lua_isnil(ls, 1)) {
        TRACE_MSG("no data defined: skipping");
        return 0;
    } else if (!lua_isstring(ls, 1)) {
        TRACE_MSG("data is not string: skipping");
        return 0;
    }
    const gchar* val = lua_tostring(ls, 1);
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(hermit.notebook));
    HERMIT_GET_TAB_BY_INDEX(pTab, page, return 0);
    hermit_tab_feed(pTab, val);
    return 0;
}

static int hermit_lua_feedChild(lua_State* ls)
{
    if (lua_isnil(ls, 1)) {
        TRACE_MSG("no data defined: skipping");
        return 0;
    } else if (!lua_isstring(ls, 1)) {
        TRACE_MSG("data is not string: skipping");
        return 0;
    }
    const gchar* val = lua_tostring(ls, 1);
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(hermit.notebook));
    HERMIT_GET_TAB_BY_INDEX(pTab, page, return 0);
    hermit_tab_feed_child(pTab, val);
    return 0;
}

static int hermit_lua_findNext(lua_State* ls)
{
    hermit_search_find_next();
    return 0;
}

static int hermit_lua_findPrev(lua_State* ls)
{
    hermit_search_find_prev();
    return 0;
}

static int hermit_lua_findDlg(lua_State* ls)
{
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hermit.b_toggle_search), TRUE);
    return 0;
}

static int hermit_lua_selection(lua_State* ls)
{
    gchar* buff = hermit_get_selection();
    if (!buff)
        return 0;
    lua_pushstring(ls, buff);
    g_free(buff);
    return 1;
}

static int hermit_lua_reconfigure(lua_State* ls)
{
    hermit_reconfigure();
    hermit_config_trace();
    return 0;
}

static int hermit_lua_quit(lua_State* ls)
{
    hermit_on_exit();
    return 0;
}

struct HermitLuaFunction
{
    const char* lua_func_name;
    lua_CFunction c_func;
    int lua_func;
} functions[] = {
    {"activateTab", hermit_lua_activateTab, 0},
    {"addMenu", hermit_lua_addMenu, 0},
    {"addPopupMenu", hermit_lua_addPopupMenu, 0},
    {"bindKey", hermit_lua_bindKey, 0},
    {"bindMouse", hermit_lua_bindMouse, 0},
    {"closeTab", hermit_lua_closeTab, 0},
    {"copy", hermit_lua_copy, 0},
    {"currentTab", hermit_lua_currentTab, 0},
    {"currentTabIndex", hermit_lua_currentTabIndex, 0},
    {"feed", hermit_lua_feed, 0},
    {"feedChild", hermit_lua_feedChild, 0},
    {"findDlg", hermit_lua_findDlg, 0},
    {"findNext", hermit_lua_findNext, 0},
    {"findPrev", hermit_lua_findPrev, 0},
    {"forEachRow", hermit_lua_forEachRow, 0},
    {"forEachVisibleRow", hermit_lua_forEachVisibleRow, 0},
    {"loadSessionDlg", hermit_lua_loadSessionDialog, 0},
    {"nextTab", hermit_lua_nextTab, 0},
    {"openTab", hermit_lua_openTab, 0},
    {"paste", hermit_lua_paste, 0},
    {"preferencesDlg", hermit_lua_preferencesDialog, 0},
    {"prevTab", hermit_lua_prevTab, 0},
    {"quit", hermit_lua_quit, 0},
    {"reconfigure", hermit_lua_reconfigure, 0},
    {"saveSessionDlg", hermit_lua_saveSessionDialog, 0},
    {"selection", hermit_lua_selection, 0},
    {"setColormap", hermit_lua_setColormap, 0},
    {"setEncoding", hermit_lua_setEncoding, 0},
    {"setKbPolicy", hermit_lua_setKbPolicy, 0},
    {"setOptions", hermit_lua_setOptions, 0},
    {"setTabBackgroundColor", hermit_lua_setTabBackgroundColor, 0},
    {"setTabFont", hermit_lua_setTabFont, 0},
    {"setTabForegroundColor", hermit_lua_setTabForegroundColor, 0},
    {"setTabPos", hermit_lua_setTabPos, 0},
    {"setTabTitle", hermit_lua_setTabTitle, 0},
    {"setTabTitleDlg", hermit_lua_setTabTitleDialog, 0},
    {"setWindowTitle", hermit_lua_setWindowTitle, 0},
    {"spawn", hermit_lua_spawn, 0},
    {"toggleMenubar", hermit_lua_toggleMenubar, 0},
    {"toggleTabbar", hermit_lua_toggleTabbar, 0}
};

int hermit_get_lua_func(const char* name)
{
    unsigned short i = 0;
    for (; i < sizeof(functions)/sizeof(functions[0]); ++i) {
        if (strcmp(name, functions[i].lua_func_name) == 0) {
            return functions[i].lua_func;
        }
    }
    ERROR("not found lua function by name [%s]", name);
    return 0;
}


void hermit_lua_init_api()
{
    unsigned short i = 0;
    for (; i < sizeof(functions)/sizeof(functions[0]); ++i) {
        lua_pushcfunction(L, functions[i].c_func);
        functions[i].lua_func = luaL_ref(L, LUA_REGISTRYINDEX);
        lua_rawgeti(L, LUA_REGISTRYINDEX, functions[i].lua_func);
        lua_setglobal(L, functions[i].lua_func_name);
        TRACE("%s [%d]", functions[i].lua_func_name, functions[i].lua_func);
    }
}

