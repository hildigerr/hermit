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

#include <sys/wait.h>
#include <stdint.h>
#include <string.h>
#include <gdk/gdkkeysyms.h>

#include "termit.h"
#include "configs.h"
#include "sessions.h"
#include "termit_core_api.h"
#include "termit_style.h"
#include "lua_api.h"
#include "keybindings.h"
#include "callbacks.h"

static gboolean confirm_exit()
{
    if (gtk_notebook_get_n_pages(GTK_NOTEBOOK(hermit.notebook)) <= 1)
        return FALSE;

    GtkWidget *dlg = gtk_message_dialog_new(
        GTK_WINDOW(hermit.main_window), 
        GTK_DIALOG_MODAL, 
        GTK_MESSAGE_QUESTION,
        GTK_BUTTONS_YES_NO, _("Several tabs are opened.\nClose anyway?"));
    gint response = gtk_dialog_run(GTK_DIALOG(dlg));
    gtk_widget_destroy(dlg);
    if (response == GTK_RESPONSE_YES)
        return FALSE;
    else
        return TRUE;
}

gboolean hermit_on_delete_event(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    return confirm_exit();
}

void hermit_on_destroy(GtkWidget *widget, gpointer data)
{
    hermit_quit();
}

void hermit_on_tab_title_changed(VteTerminal *vte, gpointer user_data)
{
    if (!configs.allow_changing_title)
        return;
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(hermit.notebook));
    HERMIT_GET_TAB_BY_INDEX(pTab, page, return);

    if (pTab->custom_tab_name)
        return;

    hermit_tab_set_title(pTab, vte_terminal_get_window_title(VTE_TERMINAL(pTab->vte)));
}

gboolean hermit_on_search_keypress(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
    switch (event->keyval) {
    case GDK_KEY_Return: {
        if (event->state == GDK_CONTROL_MASK) {
            hermit_on_find_prev(NULL, NULL);
        } else {
            hermit_on_find_next(NULL, NULL);
        }
        return TRUE;
    }
    case GDK_KEY_Escape: {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hermit.b_toggle_search), FALSE);
        return TRUE;
    }
    default:
        return FALSE;
    }
    return FALSE;
}

static void hermit_search_prepare_regex(const gchar* searchRegex)
{
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(hermit.notebook));
    HERMIT_GET_TAB_BY_INDEX(pTab, page, return);
    if (strlen(searchRegex) == 0) {
        vte_terminal_search_set_gregex(VTE_TERMINAL(pTab->vte), NULL);
    } else {
        GRegex* currSearchRegex = vte_terminal_search_get_gregex(VTE_TERMINAL(pTab->vte));
        if (!currSearchRegex || strcmp(searchRegex, g_regex_get_pattern(currSearchRegex)) != 0) {
            GError* err = NULL;
            GRegex* regex = g_regex_new(searchRegex, 0, 0, &err);
            if (err) {
                TRACE("failed to compile regex [%s]: skipping", searchRegex);
                return;
            }
            vte_terminal_search_set_gregex(VTE_TERMINAL(pTab->vte), regex);
        }
    }
}

void hermit_on_find_next(GtkButton* btn, gpointer user_data)
{
    hermit_search_prepare_regex(gtk_entry_get_text(GTK_ENTRY(hermit.search_entry)));
    hermit_search_find_next();
}

void hermit_on_find_prev(GtkButton* btn, gpointer user_data)
{
    hermit_search_prepare_regex(gtk_entry_get_text(GTK_ENTRY(hermit.search_entry)));
    hermit_search_find_prev();
}

void hermit_on_toggle_search(GtkToggleButton* tb, gpointer user_data)
{
    hermit_toggle_search();
}

void hermit_on_toggle_scrollbar()
{
    TRACE_MSG(__FUNCTION__);
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(hermit.notebook));
    HERMIT_GET_TAB_BY_INDEX(pTab, page, return);

    if (pTab->scrollbar_is_shown)
        gtk_widget_hide(GTK_WIDGET(pTab->scrollbar));
    else
        gtk_widget_show(GTK_WIDGET(pTab->scrollbar));
    pTab->scrollbar_is_shown = !pTab->scrollbar_is_shown;
}

void hermit_on_child_exited()
{
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(hermit.notebook));
    HERMIT_GET_TAB_BY_INDEX(pTab, page, return);

    TRACE("waiting for pid %d", pTab->pid);

    int status = 0;
    waitpid(pTab->pid, &status, WNOHANG);
    /* TODO: check wait return */    

    hermit_close_tab();
}

static int hermit_cursor_under_match(const GdkEventButton* ev, char** matchedText)
{
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(hermit.notebook));
    HERMIT_GET_TAB_BY_INDEX(pTab, page, return -1);

    glong column = ((glong) (ev->x) / vte_terminal_get_char_width(VTE_TERMINAL(pTab->vte)));
    glong row = ((glong) (ev->y) / vte_terminal_get_char_height(VTE_TERMINAL(pTab->vte)));
    int tag = -1;
    *matchedText = vte_terminal_match_check(VTE_TERMINAL(pTab->vte), column, row, &tag);
    TRACE("column=%ld row=%ld matchedText=[%s] tag=%d", column, row, *matchedText, tag);
    return tag;
}

static struct Match* get_match_by_tag(GArray* matches, int tag)
{
    guint i = 0;
    for (; i<matches->len; ++i) {
        struct Match* match = &g_array_index(matches, struct Match, i);
        if (match->tag == tag)
            return match;
    }
    return NULL;
}

gboolean hermit_on_popup(GtkWidget *widget, GdkEvent *event)
{
    if (event->type != GDK_BUTTON_PRESS)
        return FALSE;

    GdkEventButton *event_button = (GdkEventButton *) event;
    if (event_button->button == 3) {
        GtkMenu *menu = GTK_MENU(hermit.menu);
        gtk_menu_popup (menu, NULL, NULL, NULL, NULL, 
                          event_button->button, event_button->time);
        return TRUE;
    } else if (event_button->button == 1) {
        gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(hermit.notebook));
        HERMIT_GET_TAB_BY_INDEX(pTab, page, return FALSE);
        char* matchedText = NULL;
        int matchTag = hermit_cursor_under_match(event_button, &matchedText);
        if (!matchedText)
            return FALSE;
        struct Match* match = get_match_by_tag(pTab->matches, matchTag);
        if (!match) {
            g_free(matchedText);
            return FALSE;
        }
        TRACE("tag=%d match=[%s]", matchTag, match->pattern);
        hermit_lua_domatch(match->lua_callback, matchedText);
        g_free(matchedText);
    }

    return FALSE;
}

static gboolean dlg_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
    switch (event->keyval) {
    case GDK_KEY_Return:
        g_signal_emit_by_name(widget, "response", GTK_RESPONSE_ACCEPT, NULL);
        break;
    case GDK_KEY_Escape:
        g_signal_emit_by_name(widget, "response", GTK_RESPONSE_REJECT, NULL);
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

void hermit_on_beep(VteTerminal *vte, gpointer user_data)
{
    struct HermitTab* pTab = (struct HermitTab*)user_data;
    if (!pTab) {
        ERROR("pTab is NULL");
        return;
    }
    if (!gtk_window_has_toplevel_focus(GTK_WINDOW(hermit.main_window))) {
        if (configs.urgency_on_bell) {
            gtk_window_set_urgency_hint(GTK_WINDOW(hermit.main_window), TRUE);
            gchar* marked_title = g_strdup_printf("<b>%s</b>", gtk_label_get_text(GTK_LABEL(pTab->tab_name)));
            gtk_label_set_markup(GTK_LABEL(pTab->tab_name), marked_title);
            g_free(marked_title);
        }
    }
}

gboolean hermit_on_focus(GtkWidget *widget, GtkDirectionType arg1, gpointer user_data)
{
    struct HermitTab* pTab = (struct HermitTab*)user_data;
    if (!pTab) {
        ERROR("pTab is NULL");
        return FALSE;
    }
    if (gtk_window_get_urgency_hint(GTK_WINDOW(hermit.main_window))) {
        gtk_window_set_urgency_hint(GTK_WINDOW(hermit.main_window), FALSE);
        gtk_label_set_markup(GTK_LABEL(pTab->tab_name), gtk_label_get_text(GTK_LABEL(pTab->tab_name)));
        gtk_label_set_use_markup(GTK_LABEL(pTab->tab_name), FALSE);
    }
    return FALSE;
}

void hermit_on_set_tab_name()
{
    GtkWidget *dlg = gtk_dialog_new_with_buttons(
        _("Tab name"), 
        GTK_WINDOW(hermit.main_window), 
        GTK_DIALOG_MODAL, 
        GTK_STOCK_OK, GTK_RESPONSE_ACCEPT,
        GTK_STOCK_CANCEL, GTK_RESPONSE_REJECT,
        NULL);
    gtk_dialog_set_default_response(GTK_DIALOG(dlg), GTK_RESPONSE_ACCEPT);
    gtk_window_set_modal(GTK_WINDOW(dlg), TRUE);

    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(hermit.notebook));
    HERMIT_GET_TAB_BY_INDEX(pTab, page, return);
    GtkWidget *label = gtk_label_new(_("Tab name"));
    GtkWidget *entry = gtk_entry_new();
    gtk_entry_set_text(
        GTK_ENTRY(entry), 
        gtk_notebook_get_tab_label_text(GTK_NOTEBOOK(hermit.notebook), pTab->hbox));

    GtkWidget *hbox = gtk_hbox_new(FALSE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), label, FALSE, FALSE, 5);
    gtk_box_pack_start(GTK_BOX(hbox), entry, TRUE, FALSE, 5);

    g_signal_connect(G_OBJECT(dlg), "key-press-event", G_CALLBACK(dlg_key_press), dlg);

    gtk_box_pack_start(GTK_BOX(gtk_dialog_get_content_area(GTK_DIALOG(dlg))), hbox, FALSE, FALSE, 10);
    gtk_widget_show_all(dlg);

    if (GTK_RESPONSE_ACCEPT == gtk_dialog_run(GTK_DIALOG(dlg))) {
        hermit_tab_set_title(pTab, gtk_entry_get_text(GTK_ENTRY(entry)));
        pTab->custom_tab_name = TRUE;
    }
    gtk_widget_destroy(dlg);
}

void hermit_preferences_dialog(struct HermitTab *style);
void hermit_on_edit_preferences()
{
    gint page = gtk_notebook_get_current_page(GTK_NOTEBOOK(hermit.notebook));
    HERMIT_GET_TAB_BY_INDEX(pTab, page, return);
    hermit_preferences_dialog(pTab);
}
void hermit_on_exit()
{
    if (confirm_exit() == FALSE)
        hermit_quit();
}

void hermit_on_switch_page(GtkNotebook *notebook, gpointer arg, guint page, gpointer user_data)
{
    HERMIT_GET_TAB_BY_INDEX(pTab, page, return);
    TRACE("%s page=%d vte=%p", __FUNCTION__, page, pTab->vte);
    // it seems that set_active eventually calls toggle callback
    /*((GtkCheckMenuItem*)hermit.mi_show_scrollbar)->active = pTab->scrollbar_is_shown;*/
    gpointer pHandlerId = g_object_get_data(G_OBJECT(hermit.mi_show_scrollbar), "handlerId");
    if (pHandlerId) {
        g_signal_handler_disconnect(G_OBJECT(hermit.mi_show_scrollbar), *((gulong*)pHandlerId));
    }
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(hermit.mi_show_scrollbar), pTab->scrollbar_is_shown);
    hermit_set_show_scrollbar_signal(hermit.mi_show_scrollbar, pHandlerId);

    hermit_set_statusbar_message(page);
    if (configs.allow_changing_title)
        hermit_set_window_title(pTab->title);
}

gint hermit_on_double_click(GtkWidget *widget, GdkEventButton *event, gpointer func_data)
{
    TRACE_MSG(__FUNCTION__);
    hermit_mouse_event(event);
    return FALSE;
}

static gchar* hermit_get_xdg_data_path()
{
    gchar* fullPath = NULL;
    const gchar *dataHome = g_getenv("XDG_DATA_HOME");
    if (dataHome)
        fullPath = g_strdup_printf("%s/hermit", dataHome);
    else
        fullPath = g_strdup_printf("%s/.local/share/hermit", g_getenv("HOME"));
    TRACE("XDG_DATA_PATH=%s", fullPath);
    return fullPath;
}

void hermit_on_save_session()
{
    gchar* fullPath = hermit_get_xdg_data_path();
    GtkWidget* dlg = gtk_file_chooser_dialog_new(
        _("Save session"),
        GTK_WINDOW(hermit.main_window),
        GTK_FILE_CHOOSER_ACTION_SAVE,
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
        NULL);
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dlg), TRUE);

    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dlg), fullPath);
    gtk_file_chooser_set_current_name(GTK_FILE_CHOOSER(dlg), "New session");

    if (gtk_dialog_run(GTK_DIALOG(dlg)) != GTK_RESPONSE_ACCEPT) {
        gtk_widget_destroy(dlg);
        g_free(fullPath);
        return;
    }

    gchar* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
    hermit_save_session(filename);

    g_free(filename);
    gtk_widget_destroy(dlg);
    g_free(fullPath);
}

void hermit_on_load_session()
{
    gchar* fullPath = hermit_get_xdg_data_path();

    GtkWidget* dlg = gtk_file_chooser_dialog_new(
        _("Open session"), 
        GTK_WINDOW(hermit.main_window), 
        GTK_FILE_CHOOSER_ACTION_OPEN, 
        GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
        GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
        NULL);
    gtk_file_chooser_set_do_overwrite_confirmation(GTK_FILE_CHOOSER(dlg), TRUE);
    gtk_file_chooser_set_current_folder(GTK_FILE_CHOOSER(dlg), fullPath);

    if (gtk_dialog_run(GTK_DIALOG(dlg)) != GTK_RESPONSE_ACCEPT)
        goto free_dlg;

    gchar* filename = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dlg));
    hermit_load_session(filename);
    g_free(filename);
free_dlg:
    gtk_widget_destroy(dlg);
    g_free(fullPath);
}

void hermit_on_menu_item_selected(GtkWidget *widget, void *data)
{
    struct UserMenuItem* pMi = (struct UserMenuItem*)g_object_get_data(G_OBJECT(widget),
            HERMIT_USER_MENU_ITEM_DATA);
    if (pMi && pMi->lua_callback) {
        hermit_lua_dofunction(pMi->lua_callback);
    }
}

gboolean hermit_on_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
    return hermit_key_event(event);
}

