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

#ifndef CALLBACKS_H
#define CALLBACKS_H

gboolean hermit_on_delete_event(GtkWidget *widget, GdkEvent *event, gpointer data);
void hermit_on_destroy(GtkWidget *widget, gpointer data);
gboolean hermit_on_popup(GtkWidget *, GdkEvent *);
gboolean hermit_on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data);
gboolean hermit_on_focus(GtkWidget *widget, GtkDirectionType arg1, gpointer user_data);
void hermit_on_beep(VteTerminal *vte, gpointer user_data);
void hermit_on_edit_preferences();
void hermit_on_set_tab_name();
void hermit_on_toggle_scrollbar();
void hermit_on_child_exited();
void hermit_on_exit();
void hermit_on_switch_page(GtkNotebook *notebook, gpointer arg, guint page, gpointer user_data);
void hermit_on_menu_item_selected(GtkWidget *widget, void *data);
void hermit_on_del_tab();
gint hermit_on_double_click(GtkWidget *widget, GdkEventButton *event, gpointer func_data);
void hermit_on_save_session();
void hermit_on_load_session();
void hermit_on_tab_title_changed(VteTerminal *vte, gpointer user_data);
void hermit_on_toggle_search(GtkToggleButton*, gpointer);
void hermit_on_find_next(GtkButton*, gpointer);
void hermit_on_find_prev(GtkButton*, gpointer);
gboolean hermit_on_search_keypress(GtkWidget *widget, GdkEventKey *event, gpointer user_data);

#endif /* CALLBACKS_H */
