/* Virtual X68000 - X68000 virtual machine
   Copyright (C) 1998-2000 Hypercore Software Design, Ltd.

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
#include <gdk/gdkkeysyms.h>

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

const char *const BASE16_FONT_NAME
  = "-*-fixed-medium-r-normal--16-*-*-*-c-*-jisx0201.1976-0";
const char *const KANJI16_FONT_NAME
  = "-*-fixed-medium-r-normal--16-*-*-*-c-*-jisx0208.1983-0";

const unsigned int TIMEOUT_INTERVAL = 200;

namespace
{
  /* Helper object to synchronize GDK access.  */
  class gdk_threads_monitor
  {
  public:
    gdk_threads_monitor() {gdk_threads_enter();}
    ~gdk_threads_monitor() {gdk_threads_leave();}
  };
}

bool
gtk_console::handle_key_press_event(GtkWidget *drawing_area,
				    GdkEventKey *e)
{
  for (gchar *i = e->string + 0; i != e->string + e->length; ++i)
    {
      // FIXME character code translation?
      uint_type key = *i;
#ifdef HAVE_NANA_H
      L("gtk_console: Key press %#x\n", key);
#endif
      _m->queue_key(key);
    }

  switch (e->keyval)
    {
    case GDK_Shift_L:
    case GDK_Shift_R:
      _m->set_key_modifiers(0x01, 0x01);
      break;

    case GDK_Control_L:
    case GDK_Control_R:
      _m->set_key_modifiers(0x02, 0x02);
      break;

    default:
      break;
    }

  return true;
}

bool
gtk_console::handle_key_release_event(GtkWidget *drawing_area,
				      GdkEventKey *e)
{
  switch (e->keyval)
    {
    case GDK_Shift_L:
    case GDK_Shift_R:
      _m->set_key_modifiers(0x01, 0x00);
      break;

    case GDK_Control_L:
    case GDK_Control_R:
      _m->set_key_modifiers(0x02, 0x00);
      break;

    default:
      break;
    }

  return true;
}

bool
gtk_console::handle_expose_event(GtkWidget *drawing_area,
				 GdkEventExpose *e)
{
  GdkGC *gc = gdk_gc_new(drawing_area->window);
  gdk_gc_set_clip_rectangle(gc, &e->area);

  guchar *p = rgb_buf + e->area.y * row_size + e->area.x * 3;
  gdk_draw_rgb_image(drawing_area->window, gc,
		     e->area.x, e->area.y, e->area.width, e->area.height,
		     GDK_RGB_DITHER_NORMAL, p, row_size);

  gdk_gc_unref(gc);

  return true;
}

void
gtk_console::handle_destroy(GtkWidget *w)
{
  vector<GtkWidget *>::iterator i
    = remove(widgets.begin(), widgets.end(), w);
  widgets.erase(i, widgets.end());
}

namespace
{
  /* Handles a GDK expose event E.  This function is a glue for GTK.  */
  gint
  handle_expose_event(GtkWidget *w, GdkEventExpose *e,
		      gpointer data) throw ()
  {
    gtk_console *c = static_cast<gtk_console *>(data);
    I(c != NULL);

    return c->handle_expose_event(w, e);
  }

  /* Handles a destroy signal.  This function is a glue for GTK.  */
  void
  handle_destroy(GtkWidget *w, gpointer data) throw ()
  {
    gtk_console *c = static_cast<gtk_console *>(data);
    I(c != NULL);

    c->handle_destroy(w);
  }

  /* Delivers a GDK key press event E to the associated console.  */
  gint
  deliver_key_press_event(GtkWidget *w, GdkEventKey *e,
			  gpointer data) throw ()
  {
    gtk_console *c = static_cast<gtk_console *>(data);
    I(c != NULL);

    return c->handle_key_press_event(w, e);    
  }

  /* Delivers a GDK key release event E to the associated console.  */
  gint
  deliver_key_release_event(GtkWidget *w, GdkEventKey *e,
			  gpointer data) throw ()
  {
    gtk_console *c = static_cast<gtk_console *>(data);
    I(c != NULL);

    return c->handle_key_release_event(w, e);    
  }
} // (unnamed)

GtkWidget *
gtk_console::create_widget()
{
  GtkWidget *drawing_area = gtk_drawing_area_new();
  gtk_signal_connect(GTK_OBJECT(drawing_area), "destroy",
		     GTK_SIGNAL_FUNC(&::handle_destroy), this);
  gtk_signal_connect(GTK_OBJECT(drawing_area), "expose_event",
		     GTK_SIGNAL_FUNC(&::handle_expose_event), this);
  gtk_signal_connect(GTK_OBJECT(drawing_area), "key_press_event",
		     GTK_SIGNAL_FUNC(&deliver_key_press_event), this);
  gtk_signal_connect(GTK_OBJECT(drawing_area), "key_release_event",
		     GTK_SIGNAL_FUNC(&deliver_key_release_event), this);
  GTK_WIDGET_SET_FLAGS(drawing_area, GTK_CAN_FOCUS);
  gtk_widget_add_events(drawing_area,
			GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
  {
    GtkStyle *style = gtk_style_copy(gtk_widget_get_style(drawing_area));
    style->bg[GTK_STATE_NORMAL] = style->black;
    gtk_widget_set_style(drawing_area, style);
  }
  gtk_drawing_area_size(GTK_DRAWING_AREA(drawing_area), width, height);

  widgets.push_back(drawing_area);
  return drawing_area;
}

console::time_type
gtk_console::current_time() const
{
  guint32 t = gdk_time_get();
  return t;
}

void
gtk_console::update_area(int x, int y, int width, int height)
{
  GdkRectangle area;
  area.x = x;
  area.y = y;
  area.width = width;
  area.height = height;

  gdk_threads_enter();
  GdkRegion *new_region = gdk_region_union_with_rect(update_region, &area);
  gdk_region_destroy(update_region);
  update_region = new_region;
  gdk_threads_leave();
}

namespace
{
  unsigned char *
  base16_font_array()
  {
    unsigned char *base16_font = new unsigned char [256 * 16];

    GdkFont *f = gdk_font_load(BASE16_FONT_NAME);
    GdkPixmap *p = gdk_pixmap_new(NULL, 8, 256 * 16, 1);
    GdkGC *gc = gdk_gc_new(p);

    GdkColor zero = {0, 0x0000, 0x0000, 0x0000};
    GdkColor one = {1, 0xffff, 0xffff, 0xffff};

    gdk_gc_set_foreground(gc, &zero);
    gdk_draw_rectangle(p, gc, true, 0, 0, 8, 256 * 16);

    gdk_gc_set_foreground(gc, &one);
    for (unsigned int c = 0; c != 0x100; ++c)
      {
	char str[1];
	str[0] = c;

	gdk_draw_text(p, f, gc, 0, c * 16 + f->ascent, str, 1);
      }

    GdkImage *pi = gdk_image_get(p, 0, 0, 8, 256 * 16);
    for (int i = 0; i != 256 * 16; ++i)
      {
	unsigned int d = 0;
	for (int j = 0; j != 8; ++j)
	  {
	    if (gdk_image_get_pixel(pi, j, i) != 0)
	      d |= 0x80 >> j;
	  }
	base16_font[i] = d;
      }
    gdk_image_destroy(pi);

    gdk_gc_unref(gc);
    gdk_pixmap_unref(p);
    gdk_font_unref(f);

    return base16_font;
  }

  unsigned char *
  kanji16_font_array()
  {
    unsigned char *kanji16_font = new unsigned char [94 * 94 * 2 * 16];

    GdkFont *f = gdk_font_load(KANJI16_FONT_NAME);
    GdkPixmap *p = gdk_pixmap_new(NULL, 16, 94 * 16, 1);
    GdkGC *gc = gdk_gc_new(p);

    GdkColor zero = {0, 0x0000, 0x0000, 0x0000};
    GdkColor one = {1, 0xffff, 0xffff, 0xffff};
    unsigned char *wp = kanji16_font;
    for (unsigned int h = 0x21; h != 0x7f; ++h)
      {
	char str[2];
	str[0] = h;

	gdk_gc_set_foreground(gc, &zero);
	gdk_draw_rectangle(p, gc, true, 0, 0, 16, 94 * 16);

	gdk_gc_set_foreground(gc, &one);
	for (unsigned int c = 0x21; c != 0x7f; ++c)
	  {
	    str[1] = c;
	    gdk_draw_text(p, f, gc, 0, (c - 0x21) * 16 + f->ascent, str, 2);
	  }

	GdkImage *pi = gdk_image_get(p, 0, 0, 16, 94 * 16);
	for (int i = 0; i != 94 * 16; ++i)
	  {
	    unsigned int d = 0;
	    for (int j = 0; j != 8; ++j)
	      {
		if (gdk_image_get_pixel(pi, j, i) != 0)
		  d |= 0x80 >> j;
	      }
	    *wp++ = d;
	    d = 0;
	    for (int j = 8; j != 16; ++j)
	      {
		if (gdk_image_get_pixel(pi, j, i) != 0)
		  d |= 0x80 >> (j - 8);
	      }
	    *wp++ = d;
	  }
	gdk_image_destroy(pi);
      }

    gdk_gc_unref(gc);
    gdk_pixmap_unref(p);
    gdk_font_unref(f);

    return kanji16_font;
  }
} // (unnamed namespace)

void
gtk_console::get_b16_image(unsigned int c,
			   unsigned char *buf, size_t row_size) const
{
  if (primary_font != NULL)
    {
      for (int i = 0; i != 16; ++i)
	buf[i * row_size] = primary_font[c * 16 + i];
    }
}

void
gtk_console::get_k16_image(unsigned int c,
			   unsigned char *buf, size_t row_size) const
{
  if (kanji16_font != NULL)
    {
      unsigned int h = c >> 8;
      unsigned int l = c & 0xff;
      unsigned int p = (h - 0x21) * 94 + (l - 0x21);
      for (int i = 0; i != 16; ++i)
	{
	  buf[i * row_size + 0] = kanji16_font[(p * 16 + i) * 2 + 0];
	  buf[i * row_size + 1] = kanji16_font[(p * 16 + i) * 2 + 1];
	}
    }
}

bool
gtk_console::handle_timeout()
{
  machine::rectangle area;
  _m->update_image(rgb_buf, row_size, 768, 512, area);
  if (area.left_x != area.right_x && area.top_y != area.bottom_y)
    {
      gdk_threads_monitor mon;

      for (vector<GtkWidget *>::const_iterator i = widgets.begin();
	   i != widgets.end();
	   ++i)
	{
	  I(*i != NULL);
	  gtk_widget_queue_draw_area(*i, area.left_x, area.top_y,
				     area.right_x - area.left_x,
				     area.bottom_y - area.top_y);
	}
    }

  return true;
}

namespace
{
  /* Handles a timeout for the machine.  */
  gint
  handle_machine_timeout(gpointer data) throw ()
  {
    gtk_console *c = static_cast<gtk_console *>(data);
    I(c != NULL);

    guint32 t = gdk_time_get();
    c->check_machine_timers(t);

    return true;
  }

  /* Handles a timeout.  This function is a glue for GTK.  */
  gint
  handle_timeout(gpointer data) throw ()
  {
    gtk_console *c = static_cast<gtk_console *>(data);
    I(c != NULL);

    return c->handle_timeout();
  }
} // (unnamed namespace)

gtk_console::~gtk_console()
{
  delete [] kanji16_font;
  delete [] primary_font;

  gdk_threads_enter();

  for (vector<GtkWidget *>::iterator i = widgets.begin();
       i != widgets.end();
       ++i)
    {
      I(*i != NULL);
      gtk_signal_disconnect_by_data(GTK_OBJECT(*i), this);
    }

  gtk_timeout_remove(timeout);
  gtk_timeout_remove(machine_timeout);
  gdk_region_destroy(update_region);

  gdk_threads_leave();

  delete [] rgb_buf;
}

gtk_console::gtk_console(machine *m)
  : _m(m),
    width(768), height(512),
    row_size(768 * 3),
    rgb_buf(NULL),
    update_region(NULL),
    timeout(0),
    primary_font(NULL),
    kanji16_font(NULL)
{
  rgb_buf = new guchar [height * row_size];

  gdk_threads_enter();

  guint t = gdk_time_get();
  _m->check_timers(t);
  machine_timeout = gtk_timeout_add(10, &handle_machine_timeout, this);

  update_region = gdk_region_new();
  timeout = gtk_timeout_add(TIMEOUT_INTERVAL, &::handle_timeout, this);

  gdk_threads_leave();

  /* Retrieve font bitmap in the main thread.  */
  primary_font = base16_font_array();
  kanji16_font = kanji16_font_array();
}

