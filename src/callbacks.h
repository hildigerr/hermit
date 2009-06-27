#ifndef CALLBACKS_H
#define CALLBACKS_H


gboolean termit_on_delete_event(GtkWidget *widget, GdkEvent *event, gpointer data);
void termit_on_destroy(GtkWidget *widget, gpointer data);
gboolean termit_on_popup(GtkWidget *, GdkEvent *);
gboolean termit_on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data);
void termit_on_activate_default(GtkWindow* wnd, gpointer user_data);
void termit_on_set_encoding(GtkWidget *, void *);
void termit_on_new_tab();
void termit_on_prev_tab();
void termit_on_next_tab();
void termit_on_close_tab();
void termit_on_paste();
void termit_on_copy();
gboolean termit_on_focus(GtkWidget *widget, GtkDirectionType arg1, gpointer user_data);
void termit_on_beep(VteTerminal *vte, gpointer user_data);
void termit_on_edit_preferences();
void termit_on_set_tab_name();
void termit_on_toggle_scrollbar();
void termit_on_child_exited();
void termit_on_exit();
void termit_on_switch_page(GtkNotebook *notebook, GtkNotebookPage *page, guint page_num, gpointer user_data);
void termit_on_user_menu_item_selected(GtkWidget *widget, void *data);
void termit_on_del_tab();
gint termit_on_double_click(GtkWidget *widget, GdkEventButton *event, gpointer func_data);
void termit_on_save_session();
void termit_on_load_session();
void termit_on_tab_title_changed(VteTerminal *vte, gpointer user_data);

#endif /* CALLBACKS_H */

