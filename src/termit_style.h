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

#ifndef HERMIT_STYLE_H
#define HERMIT_STYLE_H

#include <config.h>

#include <gtk/gtk.h>
#include <gdk/gdk.h>

struct HermitStyle
{
    gchar* font_name;
    PangoFontDescription* font;
    GdkColor* foreground_color;
    GdkColor* background_color;
    gchar *image_file;
    GdkColor* colors;
    glong colors_size;
    gdouble transparency;
};

void hermit_style_init(struct HermitStyle* style);
void hermit_style_copy(struct HermitStyle* dest, const struct HermitStyle* src);
void hermit_style_free(struct HermitStyle* style);

#endif /* HERMIT_STYLE_H */

