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

#include "gtkapp.h"

#include <libintl.h>

#ifdef HAVE_NANA_H
# include <nana.h>
# include <cstdio>
#else
# include <cassert>
# define I assert
#endif

#define _(MSG) gettext(MSG)

void
gtk_console_window::notify_window_destroyed()
{
  window = NULL;
}

void
gtk_console_window::add_callback(console_callback *c)
{
  callback = c;
}

void
gtk_console_window::show()
{
  if (window != NULL)
    gtk_widget_show(window);
}

void
gtk_console_window::hide()
{
  if (window != NULL)
    gtk_widget_hide(window);
}

namespace
{
  GtkWidget *
  new_item(GtkMenuShell *msh, const gchar *label,
	   GtkAccelGroup *ag, int mods = 0, int flags = 0)
  {
    GtkWidget *mi = gtk_menu_item_new_with_label("");
    gtk_menu_shell_append(msh, mi);
    gtk_widget_show(mi);

    guint key = gtk_label_parse_uline(GTK_LABEL(GTK_BIN(mi)->child), label);
    gtk_widget_add_accelerator(mi, "activate_item", ag, key,
			       GdkModifierType(mods), GtkAccelFlags(flags));

    return mi;
  }

  GtkWidget *
  new_separator_item(GtkMenuShell *msh)
  {
    GtkWidget *mi = gtk_menu_item_new();
    gtk_menu_shell_append(msh, mi);
    gtk_widget_show(mi);
    gtk_widget_set_sensitive(mi, false);

    return mi;
  }

  GtkWidget *
  new_tearoff_item(GtkMenuShell *msh)
  {
    GtkWidget *tmi = gtk_tearoff_menu_item_new();
    gtk_menu_shell_append(msh, tmi);
    gtk_widget_show(tmi);

    return tmi;
  }

  void
  handle_window_destroy(gpointer data)
  {
    gtk_console_window *cw = static_cast<gtk_console_window *>(data);
    cw->notify_window_destroyed();
  }
} // namespace (unnamed)

gtk_console_window::~gtk_console_window()
{
  if (window != NULL)
    {
      gtk_object_weakunref(GTK_OBJECT(window), &handle_window_destroy, this);
      gtk_widget_destroy(GTK_WIDGET(window));
    }
}

gtk_console_window::gtk_console_window(GtkWidget *w)
  : callback(NULL),
    window(NULL),
    content(w)
{
  window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  try
    {
      gtk_window_set_policy(GTK_WINDOW(window), true, true, false);
      gtk_window_set_title(GTK_WINDOW(window), _("Virtual X68000"));

      GtkAccelGroup *window_ag = gtk_accel_group_new();
      gtk_window_add_accel_group(GTK_WINDOW(window), window_ag);

      GtkWidget *box1 = gtk_vbox_new(false, 0);
      gtk_container_add(GTK_CONTAINER(window), box1);
      gtk_widget_show(box1);

      /* Menu bar.  */
      menu_bar = gtk_menu_bar_new();
      gtk_box_pack_start(GTK_BOX(box1), menu_bar, false, false, 0);
      gtk_widget_show(menu_bar);

      /* Status bar.  */
      status_bar = gtk_statusbar_new();
      gtk_widget_show(status_bar);
      gtk_box_pack_end(GTK_BOX(box1), status_bar, false, false, 0);

      /* Drawing area.  */
      gtk_widget_show(content);
      gtk_box_pack_start(GTK_BOX(box1), content, true, true, 0);
      GdkGeometry content_geometry = {0, 0, 0, 0, 0, 0, 1, 1};
      gtk_window_set_geometry_hints(GTK_WINDOW(window), content,
				    &content_geometry, GDK_HINT_RESIZE_INC);
      gtk_window_set_focus(GTK_WINDOW(window), content);

      /* Menu bar items.  */
      file_item = new_item(GTK_MENU_SHELL(menu_bar), _("_File"), window_ag,
			   GDK_MOD1_MASK);
      help_item = new_item(GTK_MENU_SHELL(menu_bar), _("_Help"), window_ag,
			   GDK_MOD1_MASK);

      /* File menu.  */
      GtkWidget *file_menu = gtk_menu_new();
      gtk_menu_item_set_submenu(GTK_MENU_ITEM(file_item), file_menu);
      gtk_menu_set_title(GTK_MENU(file_menu), _("File"));
      GtkAccelGroup *file_menu_ag
	= gtk_menu_ensure_uline_accel_group(GTK_MENU(file_menu));

      /* File menu items.  */
      load_floppy_item = new_item(GTK_MENU_SHELL(file_menu),
				  _("_Load Floppy..."), file_menu_ag);
      eject_floppy_item = new_item(GTK_MENU_SHELL(file_menu),
				   _("_Eject Floppy"), file_menu_ag);
      new_separator_item(GTK_MENU_SHELL(file_menu));
      run_item = new_item(GTK_MENU_SHELL(file_menu),
			  _("_Run..."), file_menu_ag);
      new_separator_item(GTK_MENU_SHELL(file_menu));
      exit_item = new_item(GTK_MENU_SHELL(file_menu),
			   _("E_xit"), file_menu_ag);

      /* Help menu.  */
      GtkWidget *help_menu = gtk_menu_new();
      gtk_menu_item_set_submenu(GTK_MENU_ITEM(help_item), help_menu);
      gtk_menu_set_title(GTK_MENU(help_menu), _("Help"));
      GtkAccelGroup *help_menu_ag
	= gtk_menu_ensure_uline_accel_group(GTK_MENU(help_menu));

      /* Help menu items.  */
      about_item = new_item(GTK_MENU_SHELL(help_menu),
			    _("_About..."), help_menu_ag);

      /* Eject Floppy menu.  */
      GtkWidget *eject_floppy_menu = gtk_menu_new();
      gtk_menu_item_set_submenu(GTK_MENU_ITEM(eject_floppy_item),
				eject_floppy_menu);
      gtk_menu_set_title(GTK_MENU(help_menu), _("Eject Floppy"));
      GtkAccelGroup *eject_floppy_menu_ag
	= gtk_menu_ensure_uline_accel_group(GTK_MENU(eject_floppy_menu));

      /* Eject Floppy menu items.  */
      new_tearoff_item(GTK_MENU_SHELL(eject_floppy_menu));
      eject_floppy_0_item
	= new_item(GTK_MENU_SHELL(eject_floppy_menu),
		   _("Unit _0"), eject_floppy_menu_ag);
      eject_floppy_1_item
	= new_item(GTK_MENU_SHELL(eject_floppy_menu),
		   _("Unit _1"), eject_floppy_menu_ag);

      gtk_signal_connect(GTK_OBJECT(exit_item), "activate",
			 GTK_SIGNAL_FUNC(&gtk_main_quit), NULL);

      gtk_widget_set_sensitive(load_floppy_item, false);
      gtk_widget_set_sensitive(run_item, false);
      gtk_widget_set_sensitive(about_item, false);
      gtk_widget_set_sensitive(eject_floppy_0_item, false);
      gtk_widget_set_sensitive(eject_floppy_1_item, false);
    }
  catch (...)
    {
      gtk_widget_destroy(window);
      throw;
    }
  gtk_object_weakref(GTK_OBJECT(window), &handle_window_destroy, this);
}
