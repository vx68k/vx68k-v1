/* -*- C++ -*- */
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

#include <gtk/gtk.h>

struct console_callback
{
  virtual void window_destroyed() = 0;
};

/* Console window for GTK+.  */
class gtk_console_window
{
private:
  console_callback *callback;

  GtkWidget *window;

  GtkWidget *menu_bar;
  GtkWidget *status_bar;
  GtkWidget *content;

  GtkWidget *file_item;
  GtkWidget *help_item;

  GtkWidget *load_floppy_item;
  GtkWidget *eject_floppy_item;
  GtkWidget *eject_floppy_0_item;
  GtkWidget *eject_floppy_1_item;
  GtkWidget *run_item;
  GtkWidget *exit_item;
  GtkWidget *about_item;

public:
  explicit gtk_console_window(GtkWidget *w);
  ~gtk_console_window();

public:
  void show();
  void hide();

public:
  void add_callback(console_callback *);

public:
  void notify_window_destroyed();
};
