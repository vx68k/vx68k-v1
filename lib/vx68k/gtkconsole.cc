/* vx68k - Virtual X68000
   Copyright (C) 1998, 1999 Hypercore Software Design, Ltd.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307, USA.  */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#undef const
#undef inline

#include <vx68k/gtk.h>
#include <gtk/gtkdrawingarea.h>
#include <gtk/gtksignal.h>

#ifdef HAVE_NANA_H
# include <nana.h>
# include <cstdio>
#else
# include <cassert>
# define I assert
#endif

using namespace vx68k::gtk;
using namespace vx68k;
using namespace std;

gint
gtk_console::handle_expose_event(GtkWidget *drawing_area,
				 GdkEventExpose *event, gpointer data)
  throw ()
{
  gtk_console *con = static_cast<gtk_console *>(data);
  I(con != NULL);

  GdkGC *gc = gdk_gc_new(drawing_area->window);
  gdk_draw_rgb_image(drawing_area->window, gc, 0, 0, con->width, con->height,
		     GDK_RGB_DITHER_NORMAL, con->rgb_buf, con->row_size);
  gdk_gc_unref(gc);

  return true;
}

GtkWidget *
gtk_console::create_widget()
{
  GtkWidget *drawing_area = gtk_drawing_area_new();
  gtk_signal_connect(GTK_OBJECT(drawing_area), "expose_event",
		     GTK_SIGNAL_FUNC(&handle_expose_event), this);
  gtk_drawing_area_size(GTK_DRAWING_AREA(drawing_area), width, height);
  return drawing_area;
}

void
gtk_console::get_b16_image(unsigned int, unsigned char *, size_t) const
{
}

void
gtk_console::get_k16_image(unsigned int, unsigned char *, size_t) const
{
}

gtk_console::~gtk_console()
{
  delete [] rgb_buf;
}

gtk_console::gtk_console()
  : width(768), height(512),
    row_size(768 * 3),
    rgb_buf(NULL)
{
  rgb_buf = new guchar [height * row_size];
}

