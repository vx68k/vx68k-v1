/* -*- C++ -*- */
/* Virtual X68000 - X68000 virtual machine
   Copyright (C) 1998-2000 Hypercore Software Design, Ltd.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifndef _VX68K_GTK_H
#define _VX68K_GTK_H 1

#include <vx68k/machine.h>
#include <gtk/gtkwidget.h>
#include <vector>

namespace vx68k
{
  using namespace std;

  namespace gtk
  {
    /* Console for GTK+.  */
    class gtk_console
      : public virtual console
    {
    protected:

    private:
      machine *_m;
      unsigned int width, height;
#ifdef RGB
      size_t row_size;
      guchar *rgb_buf;
#else
      GdkImage *image;
      vector<guint32> ctable;
#endif

    private:
      guint machine_timeout;

      guint timeout;
      vector<GtkWidget *> widgets;

      unsigned char *primary_font;
      unsigned char *kanji16_font;

    public:
      explicit gtk_console(machine *);
      ~gtk_console();

    public:
      /* Returns the current time in milliseconds.  */
      time_type current_time() const;

      void get_b16_image(unsigned int, unsigned char *, size_t) const;
      void get_k16_image(unsigned int, unsigned char *, size_t) const;

    public:
      void check_machine_timers(uint32_type t) {_m->check_timers(t);}

      void set_mouse_state(unsigned int b, bool s) {_m->set_mouse_state(b, s);}
      void set_mouse_position(int x, int y) {_m->set_mouse_position(x, y);}

      /* Handles a timeout.  */
      bool handle_timeout();

      GtkWidget *create_widget();

      /* Handles a destroy signal on widget W.  */
      void handle_destroy(GtkWidget *W);

      /* Handles a GDK expose event E on widget W.  */
      bool handle_expose_event(GtkWidget *w, GdkEventExpose *e);

      /* Handles a GDK key press event E on widget W.  */
      bool handle_key_press_event(GtkWidget *w, GdkEventKey *e);

      /* Handles a GDK key release event E on widget W.  */
      bool handle_key_release_event(GtkWidget *w, GdkEventKey *e);
    };
  } // gtk
} // vx68k

#endif /* not _VX68K_GTK_H */

