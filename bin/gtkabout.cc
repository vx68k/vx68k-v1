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

#include <vx68k/version.h>
#include <libintl.h>

#ifdef HAVE_NANA_H
# include <nana.h>
# include <cstdio>
#else
# include <cassert>
# define I assert
#endif

using namespace std;

#define _(MSG) gettext(MSG)

#define COPYRIGHT_YEAR "1998-2000"

void
gtk_about_window::notify_window_destroyed()
{
  window = NULL;
}

void
gtk_about_window::close()
{
  if (window != NULL)
    gtk_widget_destroy(GTK_WIDGET(window));
}

namespace
{
  /* Handles a destroy notification on a window.  */
  void
  handle_window_destroy(gpointer data) throw ()
  {
    gtk_about_window *aw = static_cast<gtk_about_window *>(data);
    aw->notify_window_destroyed();
  }

  /* Handles a clicked signal on an OK button.  */
  void
  handle_ok_button_clicked(GtkButton *button, gpointer data) throw ()
  {
    gtk_about_window *aw = static_cast<gtk_about_window *>(data);
    aw->close();
  }
} // namespace (unnamed)

void
gtk_about_window::open(GtkWindow *parent)
{
  close();

  window = gtk_dialog_new();
  gtk_object_weakref(GTK_OBJECT(window), &handle_window_destroy, this);
  gtk_widget_show(window);
  gtk_window_set_title(GTK_WINDOW(window), _("About Virtual X68000"));
  gtk_window_set_policy(GTK_WINDOW(window), false, false, false);
  gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
  gtk_window_set_modal(GTK_WINDOW(window), true);
  if (parent != NULL)
    gtk_window_set_transient_for(GTK_WINDOW(window), parent);

  /* Layout.  */

  GtkWidget *box1 = gtk_hbox_new(false, 10);
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->vbox), box1, false, false, 0);
  gtk_widget_show(box1);
  gtk_container_set_border_width(GTK_CONTAINER(box1), 20);

  GtkWidget *box2 = gtk_vbox_new(false, 10);
  gtk_box_pack_end(GTK_BOX(box1), box2, false, false, 0);
  gtk_widget_show(box2);

  /* Components.  */

  const char *version_format = _("Virtual X68000 %s (library %s)");
  const char *library_version = vx68k::library_version();
  char *version;
#ifdef HAVE_ASPRINTF
  asprintf(&version, version_format, VERSION, library_version);
#else
  version = static_cast<char *>(malloc(strlen(version_format) - 2 * 2
				       + strlen(VERSION)
				       + strlen(library_version) + 1));
  sprintf(version, version_format, VERSION, library_version);
#endif

  GtkWidget *version_label = gtk_label_new(version);
  gtk_box_pack_start(GTK_BOX(box2), version_label, false, false, 0);
  gtk_widget_show(version_label);
  gtk_misc_set_alignment(GTK_MISC(version_label), 0.0, 0.5);
  gtk_label_set_justify(GTK_LABEL(version_label), GTK_JUSTIFY_LEFT);
  free(version);

  const char *copyright_format
    = _("Copyright (C) %s Hypercore Software Design, Ltd.");
  char *copyright;
#ifdef HAVE_ASPRINTF
  asprintf(&copyright, copyright_format, COPYRIGHT_YEAR);
#else
  copyright = static_cast<char *>(malloc(strlen(copyright_format) - 2
					 + strlen(COPYRIGHT_YEAR) + 1));
  sprintf(copyright, copyright_format, COPYRIGHT_YEAR);
#endif

  GtkWidget *copyright_label = gtk_label_new(copyright);
  gtk_box_pack_start(GTK_BOX(box2), copyright_label, false, false, 0);
  gtk_widget_show(copyright_label);
  gtk_misc_set_alignment(GTK_MISC(copyright_label), 0.0, 0.5);
  gtk_label_set_justify(GTK_LABEL(copyright_label), GTK_JUSTIFY_LEFT);
  free(copyright);

  const char *notice1
    = _("This is free software; see the source for copying conditions.\n"
	"There is NO WARRANTY; not even for MERCHANTABILITY\n"
	"or FITNESS FOR A PARTICULAR PURPOSE.");
  GtkWidget *notice1_label = gtk_label_new(notice1);
  gtk_box_pack_start(GTK_BOX(box2), notice1_label, false, false, 0);
  gtk_widget_show(notice1_label);
  gtk_misc_set_alignment(GTK_MISC(notice1_label), 0.0, 0.5);
  gtk_label_set_justify(GTK_LABEL(notice1_label), GTK_JUSTIFY_LEFT);
  // gtk_label_set_line_wrap(GTK_LABEL(notice1_label), true);

  const char *report_bugs
    = _("Send comments or report bugs to <vx68k@lists.hypercore.co.jp>.");
  GtkWidget *report_bugs_label = gtk_label_new(report_bugs);
  gtk_box_pack_start(GTK_BOX(box2), report_bugs_label, false, false, 0);
  gtk_widget_show(report_bugs_label);
  gtk_misc_set_alignment(GTK_MISC(report_bugs_label), 0.0, 0.5);
  gtk_label_set_justify(GTK_LABEL(report_bugs_label), GTK_JUSTIFY_LEFT);
  gtk_label_set_line_wrap(GTK_LABEL(report_bugs_label), true);

  /* An OK button.  */
  GtkWidget *ok_button = gtk_button_new_with_label(_("OK"));
  gtk_box_pack_start(GTK_BOX(GTK_DIALOG(window)->action_area),
		     ok_button, false, false, 0);
  gtk_widget_show(ok_button);
  gtk_window_set_focus(GTK_WINDOW(window), ok_button);

  /* Signals.  */

  gtk_signal_connect(GTK_OBJECT(ok_button), "clicked",
		     GTK_SIGNAL_FUNC(&handle_ok_button_clicked), this);
}

gtk_about_window::~gtk_about_window()
{
  this->close();
}

gtk_about_window::gtk_about_window()
  : window(NULL)
{
}
