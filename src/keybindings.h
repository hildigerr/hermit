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

#ifndef KEYBINDINGS_H
#define KEYBINDINGS_H

#include <X11/Xlib.h>
#include <gtk/gtk.h>

struct KeyWithState
{
    guint state;
    guint keyval;
};

typedef void(*BindingCallback)();
struct KeyBinding
{
    gchar* name;
    struct KeyWithState kws;
    KeySym keycode;
    int lua_callback;
};

struct MouseBinding
{
    GdkEventType type;
    int lua_callback;
};

//void hermit_load_keys();
int hermit_parse_keys_str(const gchar* keybinding, struct KeyWithState* kws);
void hermit_keys_bind(const gchar* keys, int lua_callback);
void hermit_keys_unbind(const gchar* keys);
void hermit_mouse_bind(const gchar* mouse_event, int lua_callback);
void hermit_mouse_unbind(const gchar* mouse_event);
void hermit_keys_set_defaults();
gboolean hermit_key_event(GdkEventKey* event);
gboolean hermit_mouse_event(GdkEventButton* event);
#endif /* KEYBINDINGS_H */

