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
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <config.h>
#include "termit.h"
#include "termit_style.h"
#include "termit_core_api.h"

static gboolean dlg_key_press(GtkWidget *widget, GdkEventKey *event, gpointer user_data)
{
    switch (event->keyval) {
    case GDK_KEY_Return:
        g_signal_emit_by_name(widget, "response", GTK_RESPONSE_OK, NULL);
        break;
    case GDK_KEY_Escape:
        g_signal_emit_by_name(widget, "response", GTK_RESPONSE_NONE, NULL);
        break;
    default:
        return FALSE;
    }
    return TRUE;
}

static void dlg_set_tab_color__(GtkColorButton *widget, gpointer user_data, void (*callback)(struct HermitTab*, const GdkColor*))
{
    if (!user_data) {
        ERROR("user_data is NULL");
        return;
    }
    struct HermitTab* pTab = (struct HermitTab*)user_data;
    GdkColor color = {};
    gtk_color_button_get_color(widget, &color);
    callback(pTab, &color);
}
static void dlg_set_foreground(GtkColorButton *widget, gpointer user_data)
{
    return dlg_set_tab_color__(widget, user_data, hermit_tab_set_color_foreground);
}
static void dlg_set_background(GtkColorButton *widget, gpointer user_data)
{
    return dlg_set_tab_color__(widget, user_data, hermit_tab_set_color_background);
}
static void dlg_set_font(GtkFontButton *widget, gpointer user_data)
{
    if (!user_data) {
        ERROR("user_data is NULL");
        return;
    }
    struct HermitTab* pTab = (struct HermitTab*)user_data;
    hermit_tab_set_font(pTab, gtk_font_button_get_font_name(widget));
}
static gboolean dlg_set_transparency(GtkSpinButton *btn, gpointer user_data)
{
    if (!user_data) {
        ERROR("user_data is NULL");
        return FALSE;
    }
    struct HermitTab* pTab = (struct HermitTab*)user_data;
    gdouble value = gtk_spin_button_get_value(btn);
    hermit_tab_set_transparency(pTab, value);
    return FALSE;
}
static gboolean dlg_set_audible_beep(GtkToggleButton *btn, gpointer user_data)
{
    if (!user_data) {
        ERROR("user_data is NULL");
        return FALSE;
    }
    struct HermitTab* pTab = (struct HermitTab*)user_data;
    gboolean value = gtk_toggle_button_get_active(btn);
    hermit_tab_set_audible_beep(pTab, value);
    return FALSE;
}
static gboolean dlg_set_visible_beep(GtkToggleButton *btn, gpointer user_data)
{
    if (!user_data) {
        ERROR("user_data is NULL");
        return FALSE;
    }
    struct HermitTab* pTab = (struct HermitTab*)user_data;
    gboolean value = gtk_toggle_button_get_active(btn);
    hermit_tab_set_visible_beep(pTab, value);
    return FALSE;
}
static gboolean dlg_set_apply_to_all_tabs(GtkToggleButton *btn, gpointer user_data)
{
    if (!user_data) {
        ERROR("user_data is NULL");
        return FALSE;
    }
    gboolean* flag = (gboolean*)user_data;
    *flag = gtk_toggle_button_get_active(btn);
    return FALSE;
}

struct HermitDlgHelper
{
    struct HermitTab* pTab;
    gchar* tab_title;
    gboolean handmade_tab_title;
    struct HermitStyle style;
    gboolean au_beep;
    gboolean vi_beep;
    // widgets with values
    GtkWidget* dialog;
    GtkWidget* entry_title;
    GtkWidget* btn_font;
    GtkWidget* btn_foreground;
    GtkWidget* btn_background;
    GtkWidget* btn_image_file;
    GtkWidget* scale_transparency;
    GtkWidget* audible_beep;
    GtkWidget* visible_beep;
    GtkWidget* btn_apply_to_all_tabs;
};

static struct HermitDlgHelper* hermit_dlg_helper_new(struct HermitTab* pTab)
{
    struct HermitDlgHelper* hlp = g_malloc0(sizeof(struct HermitDlgHelper));
    hlp->pTab = pTab;
    if (pTab->title) {
        hlp->handmade_tab_title = TRUE;
        hlp->tab_title = g_strdup(pTab->title);
    } else {
        hlp->tab_title = g_strdup(gtk_label_get_text(GTK_LABEL(pTab->tab_name)));
    }
    hermit_style_copy(&hlp->style, &pTab->style);
    hlp->au_beep = pTab->audible_beep;
    hlp->vi_beep = pTab->visible_beep;
    return hlp;
}

static void hermit_dlg_helper_free(struct HermitDlgHelper* hlp)
{
    g_free(hlp->tab_title);
    hermit_style_free(&hlp->style);
    g_free(hlp);
}

static void dlg_set_tab_default_values(struct HermitTab* pTab, struct HermitDlgHelper* hlp)
{
    if (hlp->tab_title)
        hermit_tab_set_title(pTab, hlp->tab_title);
    vte_terminal_set_default_colors(VTE_TERMINAL(pTab->vte));
    hermit_tab_set_font(pTab, hlp->style.font_name);
    hermit_tab_set_background_image(pTab, hlp->style.image_file);
    hermit_tab_set_color_foreground(pTab, hlp->style.foreground_color);
    hermit_tab_set_color_background(pTab, hlp->style.background_color);
    hermit_tab_set_transparency(pTab, hlp->style.transparency);
    hermit_tab_set_audible_beep(pTab, hlp->au_beep);
    hermit_tab_set_visible_beep(pTab, hlp->vi_beep);
    if (hlp->style.image_file) {
        gtk_file_chooser_select_filename(GTK_FILE_CHOOSER(hlp->btn_image_file), hlp->style.image_file);
    }
}

static void dlg_set_default_values(struct HermitDlgHelper* hlp)
{
    gtk_entry_set_text(GTK_ENTRY(hlp->entry_title), hlp->tab_title);
    gtk_font_button_set_font_name(GTK_FONT_BUTTON(hlp->btn_font), hlp->style.font_name);
    if (hlp->style.foreground_color) {
        gtk_color_button_set_color(GTK_COLOR_BUTTON(hlp->btn_foreground), hlp->style.foreground_color);
    }
    if (hlp->style.background_color) {
        gtk_color_button_set_color(GTK_COLOR_BUTTON(hlp->btn_background), hlp->style.background_color);
    }
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(hlp->scale_transparency), hlp->style.transparency);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hlp->audible_beep), hlp->au_beep);
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(hlp->visible_beep), hlp->vi_beep);
    if (hlp->style.image_file) {
        gtk_file_chooser_select_filename(GTK_FILE_CHOOSER(hlp->btn_image_file), hlp->style.image_file);
    } else {
        gtk_file_chooser_unselect_all(GTK_FILE_CHOOSER(hlp->btn_image_file));
    }
}

static void dlg_restore_defaults(GtkButton *button, gpointer user_data)
{
    struct HermitDlgHelper* hlp = (struct HermitDlgHelper*)user_data;
    dlg_set_default_values(hlp);
    dlg_set_tab_default_values(hlp->pTab, hlp);
}

static void dlg_set_image_file(GtkFileChooserButton *widget, gpointer user_data)
{
    struct HermitTab* pTab = (struct HermitTab*)user_data;
    hermit_tab_set_background_image(pTab, gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(widget)));
}

static gboolean dlg_clear_image_file(GtkWidget* widget, GdkEventKey* event, gpointer user_data)
{
    struct HermitTab* pTab = (struct HermitTab*)user_data;
    if (event->keyval == GDK_KEY_Delete) {
        if (pTab->style.image_file) {
            gtk_file_chooser_unselect_all(GTK_FILE_CHOOSER(widget));
            hermit_tab_set_background_image(pTab, NULL);
        }
    }
    return FALSE;
}

void hermit_preferences_dialog(struct HermitTab *pTab)
{
    // store font_name, foreground, background
    struct HermitDlgHelper* hlp = hermit_dlg_helper_new(pTab);

    GtkStockItem item = {};
    gtk_stock_lookup(GTK_STOCK_PREFERENCES, &item); // may be memory leak inside
    GtkWidget* dialog = gtk_dialog_new_with_buttons(item.label,
            GTK_WINDOW_TOPLEVEL,
            GTK_DIALOG_DESTROY_WITH_PARENT | GTK_DIALOG_MODAL,
            GTK_STOCK_CANCEL, GTK_RESPONSE_NONE,
            GTK_STOCK_OK, GTK_RESPONSE_OK,
            NULL);
    g_signal_connect(G_OBJECT(dialog), "key-press-event", G_CALLBACK(dlg_key_press), dialog);
    GtkWidget* dlg_table = gtk_table_new(5, 2, FALSE);

#define HERMIT_PREFERENCE_ROW(pref_name, widget) \
    gtk_table_attach(GTK_TABLE(dlg_table), gtk_label_new(pref_name), 0, 1, row, row + 1, 0, 0, 0, 0); \
    gtk_table_attach_defaults(GTK_TABLE(dlg_table), widget, 1, 2, row, row + 1); \
    hlp->widget = widget; \
    row++;
#define HERMIT_PREFERENCE_ROW2(pref_widget, widget) \
    gtk_table_attach(GTK_TABLE(dlg_table), pref_widget, 0, 1, row, row + 1, 0, 0, 0, 0); \
    gtk_table_attach_defaults(GTK_TABLE(dlg_table), widget, 1, 2, row, row + 1); \
    hlp->widget = widget; \
    row++;

    gboolean apply_to_all_tabs_flag = FALSE;
    GtkWidget* entry_title = gtk_entry_new();
    guint row = 0;
    { // tab title
        gtk_entry_set_text(GTK_ENTRY(entry_title), hlp->tab_title);
        HERMIT_PREFERENCE_ROW(_("Title"), entry_title);
    }

    // font selection
    GtkWidget* btn_font = gtk_font_button_new_with_font(pTab->style.font_name);
    g_signal_connect(btn_font, "font-set", G_CALLBACK(dlg_set_font), pTab);
    HERMIT_PREFERENCE_ROW(_("Font"), btn_font);

    // foreground
    GtkWidget* btn_foreground = (pTab->style.foreground_color)
        ? gtk_color_button_new_with_color(pTab->style.foreground_color)
        : gtk_color_button_new();
    g_signal_connect(btn_foreground, "color-set", G_CALLBACK(dlg_set_foreground), pTab);
    HERMIT_PREFERENCE_ROW(_("Foreground"), btn_foreground);

    // background
    GtkWidget* btn_background = (pTab->style.background_color)
        ? gtk_color_button_new_with_color(pTab->style.background_color)
        : gtk_color_button_new();
    g_signal_connect(btn_background, "color-set", G_CALLBACK(dlg_set_background), pTab);
    HERMIT_PREFERENCE_ROW(_("Background"), btn_background);

    // background image
    GtkWidget* btn_image_file = gtk_file_chooser_button_new(pTab->style.image_file,
        GTK_FILE_CHOOSER_ACTION_OPEN);
    GtkFileFilter* filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, _("images"));
    gtk_file_filter_add_mime_type(filter, "image/*");
    if (pTab->style.image_file) {
        gtk_file_chooser_select_filename(GTK_FILE_CHOOSER(btn_image_file), pTab->style.image_file);
    }
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(btn_image_file), filter);
    g_signal_connect(btn_image_file, "file-set", G_CALLBACK(dlg_set_image_file), pTab);
    g_signal_connect(btn_image_file, "key-press-event", G_CALLBACK(dlg_clear_image_file), pTab);
    GtkWidget* btn_switch_image_file = gtk_check_button_new_with_label(_("Background image"));
    if (pTab->style.image_file) {
        gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btn_switch_image_file), TRUE);
    }
    /*g_signal_connect(btn_switch_image_file, "toggled", G_CALLBACK(dlg_switch_image_file), btn_image_file);*/
    /*HERMIT_PREFERENCE_ROW2(btn_switch_image_file, btn_image_file);*/
    HERMIT_PREFERENCE_ROW(_("Image"), btn_image_file);

    // transparency
    GtkWidget* scale_transparency = gtk_spin_button_new_with_range(0, 1, 0.05);
    gtk_spin_button_set_value(GTK_SPIN_BUTTON(scale_transparency), pTab->style.transparency);
    g_signal_connect(scale_transparency, "value-changed", G_CALLBACK(dlg_set_transparency), pTab);
    HERMIT_PREFERENCE_ROW(_("Transparency"), scale_transparency);

    // audible_beep
    GtkWidget* audible_beep = gtk_check_button_new();
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(audible_beep), pTab->audible_beep);
    g_signal_connect(audible_beep, "toggled", G_CALLBACK(dlg_set_audible_beep), pTab);
    HERMIT_PREFERENCE_ROW(_("Audible beep"), audible_beep);

    // visible_beep
    GtkWidget* visible_beep = gtk_check_button_new();
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(visible_beep), pTab->visible_beep);
    g_signal_connect(visible_beep, "toggled", G_CALLBACK(dlg_set_visible_beep), pTab);
    HERMIT_PREFERENCE_ROW(_("Visible beep"), visible_beep);

    // apply to al tabs
    GtkWidget* btn_apply_to_all_tabs = gtk_check_button_new();
    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(btn_apply_to_all_tabs), FALSE);
    g_signal_connect(btn_apply_to_all_tabs, "toggled", G_CALLBACK(dlg_set_apply_to_all_tabs), &apply_to_all_tabs_flag);
    HERMIT_PREFERENCE_ROW(_("Apply to all tabs"), btn_apply_to_all_tabs);

    GtkWidget* btn_restore = gtk_button_new_from_stock(GTK_STOCK_REVERT_TO_SAVED);
    g_signal_connect(G_OBJECT(btn_restore), "clicked", G_CALLBACK(dlg_restore_defaults), hlp);
    gtk_table_attach(GTK_TABLE(dlg_table), btn_restore, 1, 2, row, row + 1, 0, 0, 0, 0);

    gtk_container_add(GTK_CONTAINER(gtk_dialog_get_content_area(GTK_DIALOG(dialog))), dlg_table);

    gtk_widget_show_all(dialog);
    if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_OK) {
        dlg_set_tab_default_values(pTab, hlp);
    } else {
        if (apply_to_all_tabs_flag) {
            gint page_num = gtk_notebook_get_n_pages(GTK_NOTEBOOK(hermit.notebook));
            gint i=0;
            for (; i<page_num; ++i) {
                HERMIT_GET_TAB_BY_INDEX(pTab, i, continue);
                dlg_set_font(GTK_FONT_BUTTON(btn_font), pTab);
                dlg_set_foreground(GTK_COLOR_BUTTON(btn_foreground), pTab);
                dlg_set_background(GTK_COLOR_BUTTON(btn_background), pTab);
                dlg_set_transparency(GTK_SPIN_BUTTON(scale_transparency), pTab);
                dlg_set_image_file(GTK_FILE_CHOOSER_BUTTON(btn_image_file), pTab);
                dlg_set_audible_beep(GTK_TOGGLE_BUTTON(audible_beep), pTab);
                dlg_set_visible_beep(GTK_TOGGLE_BUTTON(visible_beep), pTab);
            }
        }
        // insane title flag
        if (pTab->title ||
                (!pTab->title &&
                 strcmp(gtk_label_get_text(GTK_LABEL(pTab->tab_name)),
                     gtk_entry_get_text(GTK_ENTRY(entry_title))) != 0)) {
            hermit_tab_set_title(pTab, gtk_entry_get_text(GTK_ENTRY(entry_title)));
        }
    }
    hermit_dlg_helper_free(hlp);
    gtk_widget_destroy(dialog);
}
