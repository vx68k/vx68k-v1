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

/* Load Floppy dialog for GTK+.  */
class gtk_load_floppy_dialog
{
private:
  GtkWidget *file_dialog;

public:
  explicit gtk_load_floppy_dialog(GtkWindow *);
  ~gtk_load_floppy_dialog();
};

/* About dialog window for GTK+.  */
class gtk_about_window
{
private:
  /* Window widget or null.  If this member is null, no window is open
     for this object.  */
  GtkWidget *window;

  /* OK button.  */
  GtkWidget *ok_button;

public:
  /* Constructs this object.  */
  gtk_about_window();

  /* Destructs this object.  */
  ~gtk_about_window();

public:
  /* Opens a new window for this object.  If there is already a window
     open, the window will be closed before a new one is opened.  */
  void open(GtkWindow *parent);

  /* Closes the window for this object if one is open.  */
  void close();

public:
  /* Notifies the window is destroyed.  */
  void notify_window_destroyed();
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

  gtk_about_window aw;

public:
  explicit gtk_console_window(GtkWidget *w);
  ~gtk_console_window();

public:
  void show();
  void hide();

public:
  void show_about();

public:
  void add_callback(console_callback *);

public:
  void notify_window_destroyed();
};
