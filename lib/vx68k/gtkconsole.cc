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
#include <gtk/gtk.h>
#include <algorithm>

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

const unsigned int TIMEOUT_INTERVAL = 100;

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

void
gtk_console::handle_destroy(GtkObject *o, gpointer data) throw ()
{
  I(o != NULL);
  GtkWidget *w = GTK_WIDGET(o);
  gtk_console *c = static_cast<gtk_console *>(data);

  vector<GtkWidget *>::iterator i
    = remove(c->widgets.begin(), c->widgets.end(), w);
  c->widgets.erase(i, c->widgets.end());
}

GtkWidget *
gtk_console::create_widget()
{
  GtkWidget *drawing_area = gtk_drawing_area_new();
  gtk_drawing_area_size(GTK_DRAWING_AREA(drawing_area), width, height);
  gtk_signal_connect(GTK_OBJECT(drawing_area), "destroy",
		     GTK_SIGNAL_FUNC(&handle_destroy), this);
  gtk_signal_connect(GTK_OBJECT(drawing_area), "expose_event",
		     GTK_SIGNAL_FUNC(&handle_expose_event), this);

  GtkStyle *style = gtk_widget_get_style(drawing_area);
  style->bg[0] = style->black;
  gtk_widget_set_style(drawing_area, style);

  widgets.push_back(drawing_area);
  return drawing_area;
}

gint
gtk_console::handle_timeout(gpointer data) throw ()
{
  gtk_console *c = static_cast<gtk_console *>(data);
  I(c != NULL);

  c->_m->get_image(0, 0, c->width, c->height, c->rgb_buf, c->row_size);

  for (vector<GtkWidget *>::const_iterator i = c->widgets.begin();
       i != c->widgets.end();
       ++i)
    {
      I(*i != NULL);
      gtk_widget_queue_draw(*i);
    }

  return true;
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
  for (vector<GtkWidget *>::iterator i = widgets.begin();
       i != widgets.end();
       ++i)
    {
      I(*i != NULL);
      gtk_signal_disconnect_by_data(GTK_OBJECT(*i), this);
    }

  gtk_timeout_remove(timeout);
  delete [] rgb_buf;
}

gtk_console::gtk_console(machine *m)
  : _m(m),
    width(768), height(512),
    row_size(768 * 3),
    rgb_buf(NULL),
    timeout(0)
{
  rgb_buf = new guchar [height * row_size];
  timeout = gtk_timeout_add(TIMEOUT_INTERVAL, &handle_timeout, this);
}

