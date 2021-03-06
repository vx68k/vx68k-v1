/* Virtual X68000 - X68000 virtual machine
   Copyright (C) 1998-2001 Hypercore Software Design, Ltd.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or (at
   your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
   USA.  */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#undef const			// C++ must have `const'.

#define _GNU_SOURCE 1
#define _POSIX_C_SOURCE 199506L // We want POSIX.1c if not GNU.

#ifdef HAVE_NANA_H
# include <nana.h>
#else
# include <cassert>
# define I assert
#endif

#include "vx68k/gtk.h"

#include "gtk_gl.h"
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <GL/glu.h>
#include <algorithm>
#ifdef L
# include <cstdio>
#endif

namespace vx68k
{
  namespace gtk
  {
    using namespace std;

    const char *const BASE16_FONT_NAME
    = "-*-fixed-medium-r-normal--16-*-*-*-c-*-jisx0201.1976-0";
    const char *const KANJI16_FONT_NAME
    = "-*-fixed-medium-r-normal--16-*-*-*-c-*-jisx0208.1983-0";

    const unsigned int TIMEOUT_INTERVAL = 40;
    const unsigned int SCREEN_CHECK_INTERVAL = 25;

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
	  uint16_type key = *i;
#ifdef L
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
      if (_context == 0)
	{
	  _context = gl::create_context(gl::best_visual());
	  gl::make_current(_context, drawing_area->window);

	  glEnable(GL_CULL_FACE);
	  //glFrontFace(GL_CW);
	  glCullFace(GL_BACK);

	  //glEnable(GL_BLEND);

	  glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);
	  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, 1024, 1024, 0,
		       GL_RGB, GL_UNSIGNED_BYTE, 0);
	  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	  glMatrixMode(GL_PROJECTION);
	  glLoadIdentity();
	  gluOrtho2D(0., 768., 0., 512.);
	  glMatrixMode(GL_MODELVIEW);
	  glLoadIdentity();
	  glScalef(1., -1., -1.); // Reverses the y- and z-axes.
	  glTranslatef(0., -512., 0);

	  GLenum gl_error = glGetError();
	  if (gl_error != GL_NO_ERROR)
	    g_error("OpenGL error: %s\n", gluErrorString(gl_error));
	}

#if 0
      GdkGC *gc = gdk_gc_new(drawing_area->window);
      gdk_gc_set_clip_rectangle(gc, &e->area);

      int x = e->area.x;
      int y = e->area.y;
      if (x < int(width) && y < int(height))
	{
	  unsigned int w = e->area.width;
	  unsigned int h = e->area.height;
	  if (w > width - x)
	    w = width - x;
	  if (h > height - y)
	    h = height - y;

	  guchar *p = rgb_buf + e->area.y * row_size + e->area.x * 3;
	  gdk_draw_rgb_image(drawing_area->window, gc, x, y, w, h,
			     GDK_RGB_DITHER_NORMAL, p, row_size);
	}

      gdk_gc_unref(gc);
#endif

      gl::make_current(_context, drawing_area->window);
      int width, height;
      gdk_window_get_size(drawing_area->window, &width, &height);
      glViewport(0, 0, width, height);

      guchar *p = rgb_buf + e->area.y * row_size;
      glTexSubImage2D(GL_TEXTURE_2D, 0, 0, e->area.y, 768, e->area.height,
      		      GL_RGB, GL_UNSIGNED_BYTE, p);

      glClear(GL_COLOR_BUFFER_BIT);

      glEnable(GL_TEXTURE_2D);
      glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_REPLACE);

      glBegin(GL_QUADS);
      glTexCoord2f(0., 0.);
      glVertex2i(0, 0);
      glTexCoord2f(0., 512. / 1024.);
      glVertex2i(0, 512);
      glTexCoord2f(768. / 1024., 512. / 1024.);
      glVertex2i(768, 512);
      glTexCoord2f(768. / 1024., 0.);
      glVertex2i(768, 0);
      glEnd();

      glDisable(GL_TEXTURE_2D);

      GLenum gl_error = glGetError();
      if (gl_error != GL_NO_ERROR)
	g_error("OpenGL error: %s\n", gluErrorString(gl_error));

      gl::swap_buffers(drawing_area->window);
      gl::make_current(0, 0);

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
      call_expose_event_handler(GtkWidget *w, GdkEventExpose *e,
				gpointer data) throw ()
      {
	gtk_console *c = static_cast<gtk_console *>(data);
	I(c != NULL);

	return c->handle_expose_event(w, e);
      }

      /* Handles a destroy signal.  This function is a glue for GTK.  */
      void
      call_destroy_handler(GtkWidget *w, gpointer data) throw ()
      {
	gtk_console *c = static_cast<gtk_console *>(data);
	I(c != NULL);

	c->handle_destroy(w);
      }

      /* Handles a GDK motion event.  */
      gint
      handle_motion_event(GtkWidget *w, GdkEventMotion *e, gpointer data) throw ()
      {
	gtk_console *c = static_cast<gtk_console *>(data);
	I(c != NULL);

	c->set_mouse_position(e->x, e->y);

	return true;
      }

      /* Handles a GDK button press or release event.  */
      gint
      handle_button_event(GtkWidget *w, GdkEventButton *e, gpointer data) throw ()
      {
	gtk_console *c = static_cast<gtk_console *>(data);
	I(c != NULL);

	bool state = e->type != GDK_BUTTON_RELEASE;
	if (state)
	  gtk_grab_add(w);
	else
	  gtk_grab_remove(w);

	switch (e->button)
	  {
	  case 1:
	    c->set_mouse_state(0, state);
	    break;

	  case 3:
	    c->set_mouse_state(1, state);
	    break;
	  }

	return true;
      }

      /* Delivers a GDK key press event E to the associated console.  */
      gint
      call_key_press_event_handler(GtkWidget *w, GdkEventKey *e,
				   gpointer data) throw ()
      {
	gtk_console *c = static_cast<gtk_console *>(data);
	I(c != NULL);

	return c->handle_key_press_event(w, e);    
      }

      /* Delivers a GDK key release event E to the associated console.  */
      gint
      call_key_release_event_handler(GtkWidget *w, GdkEventKey *e,
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
      gtk_widget_push_visual(gl::best_visual());

      GtkWidget *drawing_area = gtk_drawing_area_new();
      gtk_signal_connect(GTK_OBJECT(drawing_area), "destroy",
			 GTK_SIGNAL_FUNC(&call_destroy_handler), this);
      gtk_signal_connect(GTK_OBJECT(drawing_area), "expose_event",
			 GTK_SIGNAL_FUNC(&call_expose_event_handler), this);
      gtk_signal_connect(GTK_OBJECT(drawing_area), "motion_notify_event",
			 GTK_SIGNAL_FUNC(&handle_motion_event), this);
      gtk_signal_connect(GTK_OBJECT(drawing_area), "button_press_event",
			 GTK_SIGNAL_FUNC(&handle_button_event), this);
      gtk_signal_connect(GTK_OBJECT(drawing_area), "button_release_event",
			 GTK_SIGNAL_FUNC(&handle_button_event), this);
      gtk_signal_connect(GTK_OBJECT(drawing_area), "key_press_event",
			 GTK_SIGNAL_FUNC(&call_key_press_event_handler), this);
      gtk_signal_connect(GTK_OBJECT(drawing_area), "key_release_event",
			 GTK_SIGNAL_FUNC(&call_key_release_event_handler), this);
      GTK_WIDGET_SET_FLAGS(drawing_area, GTK_CAN_FOCUS);
      gtk_widget_add_events(drawing_area,
			    GDK_POINTER_MOTION_MASK
			    | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK
			    | GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
      {
	GtkStyle *style = gtk_style_copy(gtk_widget_get_style(drawing_area));
	style->bg[GTK_STATE_NORMAL] = style->black;
	gtk_widget_set_style(drawing_area, style);
      }
      gtk_drawing_area_size(GTK_DRAWING_AREA(drawing_area), width, height);

      gtk_widget_pop_visual();

      widgets.push_back(drawing_area);
      return drawing_area;
    }
    
    console::time_type
    gtk_console::current_time() const
    {
      guint32 t = gdk_time_get();
      return t;
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

    namespace
    {
      unsigned char rgb_table[0x10000 * 3];
      bool rgb_once;

      class pixel_iterator: public output_iterator
      {
      private:
	guchar *rgb_ptr;

      public:
	pixel_iterator(guchar *p): rgb_ptr(p) {}

      public:
	bool operator==(const pixel_iterator &another) const
	{
	  return rgb_ptr == another.rgb_ptr;
	}

	pixel_iterator &operator*()
	{
	  return *this;
	}

	pixel_iterator &operator=(uint16_type p)
	{
	  unsigned char *rgb = rgb_table + p * 3;
	  rgb_ptr[0] = rgb[0];
	  rgb_ptr[1] = rgb[1];
	  rgb_ptr[2] = rgb[2];

	  return *this;
	}

	pixel_iterator &operator++()
	{
	  rgb_ptr += 3;
	  return *this;
	}

	pixel_iterator operator++(int)
	{
	  pixel_iterator tmp = *this;
	  ++*this;
	  return tmp;
	}
      };

      inline bool
      operator!=(const pixel_iterator &x, const pixel_iterator &y)
      {
	return !(x == y);
      }

      class row
      {
      public:
	typedef pixel_iterator::value_type value_type;
	typedef pixel_iterator iterator;

      private:
	unsigned int width;
	size_t row_size;
	guchar *rgb_ptr;

      public:
	row(guchar *p, unsigned int w, size_t n)
	  : width(w), row_size(n), rgb_ptr(p) {}

      public:
	bool operator==(const row &another) const
	{
	  return rgb_ptr == another.rgb_ptr;
	}

      public:
	pixel_iterator begin()
	{
	  return pixel_iterator(rgb_ptr);
	}
	pixel_iterator end()
	{
	  return pixel_iterator(rgb_ptr + width * 3);
	}

      public:
	void next()
	{
	  rgb_ptr += row_size;
	}
      };

      class row_iterator: public input_iterator<row, ptrdiff_t>
      {
      private:
	row current;

      public:
	row_iterator(guchar *ptr, unsigned int w, size_t n)
	  : current(ptr, w, n) {}

      public:
	bool operator==(const row_iterator &another) const
	{return current == another.current;}
	const row &operator*() const {return current;}
	const row *operator->() const {return &current;}

      public:
	row_iterator &operator++() {current.next();  return *this;}
      };
    }

    bool
    gtk_console::handle_timeout()
    {
      --counter;
      bool u = counter == 0;
      if (u)
	{
	  counter = SCREEN_CHECK_INTERVAL;
	  u = _m->screen_changed();
	}

      gdk_threads_monitor mon;
      for (unsigned int y = 0; y != height; ++y)
	{
	  if (_m->row_changed(y) || u)
	    {
	      _m->scan_row(y, pixel_iterator(rgb_buf + y * row_size),
			   pixel_iterator(rgb_buf + y * row_size + width * 3));

	      for (vector<GtkWidget *>::const_iterator i = widgets.begin();
		   i != widgets.end(); ++i)
		{
		  I(*i != NULL);
		  gtk_widget_queue_draw_area(*i, 0, y, width, y + 1);
		}
	    }
	}
      return true;
    }

    GdkVisual *
    gtk_console::best_visual()
    {
      return gl::best_visual();
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
      call_timeout_handler(gpointer data) throw ()
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

      gl::destroy_context(_context);

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

      gdk_threads_leave();

      delete [] rgb_buf;
    }

    gtk_console::gtk_console(machine *m)
      : _m(m),
	width(768), height(512),
	_context(0),
	row_size(768 * 3),
	rgb_buf(NULL),
	counter(1),
	timeout(0),
	primary_font(NULL),
	kanji16_font(NULL)
    {
      if (!rgb_once++)
	{
	  for (uint32_type i = 0; i != 0x10000; ++i)
	    {
	      unsigned int x = i & 0x1;
	      rgb_table[i * 3] = (i >> 5 & 0x3e | x) * 0xff / 0x3f;
	      rgb_table[i * 3 + 1] = (i >> 10 & 0x3e | x) * 0xff / 0x3f;
	      rgb_table[i * 3 + 2] = (i & 0x3f) * 0xff / 0x3f;
	    }
	}

      rgb_buf = new guchar [height * row_size];

      gdk_threads_enter();

      guint t = gdk_time_get();
      _m->check_timers(t);
      machine_timeout = gtk_timeout_add(10, &handle_machine_timeout, this);

      timeout = gtk_timeout_add(TIMEOUT_INTERVAL, &call_timeout_handler, this);

      gdk_threads_leave();

      /* Retrieve font bitmap in the main thread.  */
      primary_font = base16_font_array();
      kanji16_font = kanji16_font_array();
    }
  }
}
