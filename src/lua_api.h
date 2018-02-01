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

#ifndef HERMIT_LUA_API_H
#define HERMIT_LUA_API_H

struct lua_State;

void hermit_lua_init_api();
void hermit_lua_close();
void hermit_lua_report_error(const char* file, int line, int status);
void hermit_lua_init(const gchar* initFile);
void hermit_lua_load_config();
void hermit_lua_execute(const gchar* cmd);
int hermit_lua_dofunction(int f);
int hermit_lua_dofunction2(int f, const char* arg1);
int hermit_lua_domatch(int f, const gchar* matchedText);
void hermit_lua_unref(int* lua_callback);
gchar* hermit_lua_getTitleCallback(int f, const gchar* title);
gchar* hermit_lua_getStatusbarCallback(int f, guint page);
int hermit_get_lua_func(const char* name);

typedef enum {HERMIT_LUA_TABLE_LOADER_OK, HERMIT_LUA_TABLE_LOADER_FAILED} HermitLuaTableLoaderResult;
typedef void (*HermitLuaTableLoaderFunc)(const gchar*, struct lua_State*, int, void*);
HermitLuaTableLoaderResult hermit_lua_load_table(struct lua_State* ls, HermitLuaTableLoaderFunc func,
        const int tableIndex, void* data);
int hermit_lua_fill_tab(int tab_index, struct lua_State* ls);
/**
 * Loaders
 * */
void hermit_lua_options_loader(const gchar* name, struct lua_State* ls, int index, void* data);
void hermit_lua_keys_loader(const gchar* name, struct lua_State* ls, int index, void* data);
void hermit_lua_tab_loader(const gchar* name, struct lua_State* ls, int index, void* data);
void hermit_lua_load_colormap(struct lua_State* ls, int index, GdkColor** colors, glong* sz);
void hermit_config_get_string(gchar** opt, struct lua_State* ls, int index);
void hermit_config_get_double(double* opt, struct lua_State* ls, int index);
void hermit_config_getuint(guint* opt, struct lua_State* ls, int index);
void hermit_config_get_boolean(gboolean* opt, struct lua_State* ls, int index);
void hermit_config_get_function(int* opt, struct lua_State* ls, int index);
void hermit_config_get_color(GdkColor** opt, struct lua_State* ls, int index);
void hermit_config_get_erase_binding(VteTerminalEraseBinding* opt, struct lua_State* ls, int index);
void hermit_config_get_cursor_blink_mode(VteTerminalCursorBlinkMode* opt, struct lua_State* ls, int index);
void hermit_config_get_cursor_shape(VteTerminalCursorShape* opt, struct lua_State* ls, int index);

#define HERMIT_TAB_ADD_NUMBER(name, value) {\
    lua_pushstring(ls, name); \
    lua_pushnumber(ls, value); \
    lua_rawset(ls, -3); \
}
#define HERMIT_TAB_ADD_STRING(name, value) {\
    lua_pushstring(ls, name); \
    lua_pushstring(ls, value); \
    lua_rawset(ls, -3); \
}
#define HERMIT_TAB_ADD_VOID(name, value) {\
    lua_pushstring(ls, name); \
    lua_pushlightuserdata(ls, value); \
    lua_rawset(ls, -3); \
}
#define HERMIT_TAB_ADD_BOOLEAN(name, value) {\
    lua_pushstring(ls, name); \
    lua_pushboolean(ls, value); \
    lua_rawset(ls, -3); \
}
#define HERMIT_TAB_ADD_CALLBACK(name, value) {\
    lua_pushstring(ls, name); \
    lua_rawgeti(ls, LUA_REGISTRYINDEX, value); \
    lua_rawset(ls, -3); \
}
#endif /* HERMIT_LUA_API_H */

