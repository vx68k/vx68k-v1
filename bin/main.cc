/* vx68k - Virtual X68000
   Copyright (C) 1998, 2000 Hypercore Software Design, Ltd.

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

#include "getopt.h"

#include <vx68k/human.h>
#include <vx68k/gtk.h>

#include <gtk/gtk.h>
#include <pthread.h>
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <exception>
#include <csignal>
#include <cstdlib>
#include <cstdio>

#define _(MSG) (MSG)

#ifdef HAVE_NANA_H
# include <nana.h>
# include <cstdio>
#else
# include <cassert>
# define I assert
#endif

extern char **environ;

using namespace vx68k::gtk;
using namespace vx68k;
using namespace std;

#define PROGRAM "Virtual X68000"

#define COPYRIGHT_YEAR "1998, 2000"

class vx68k_app
{
public:
  /* Program options.  */
  static size_t opt_memory_size;
  static int opt_single_threaded;
  static int opt_debug_level;

protected:
  static void *run_machine_thread(void *) throw ();

private:
  const char *const *args;
  machine vm;
  gtk_console con;
  int status;

  /* Main window of this application.  */
  GtkWidget *main_window;

public:
  vx68k_app(const char *const *args);

protected:
  void run_machine();

public:
  void run();
  int exit_status() const
    {return status;}
  GtkWidget *create_window();

public:
  /* Shows the about dialog and returns immediately.  */
  void show_about_dialog();
};

size_t vx68k_app::opt_memory_size = 0;
int vx68k_app::opt_single_threaded = false;
int vx68k_app::opt_debug_level = 0;

void
vx68k_app::run_machine()
{
  human::dos env(&vm);
  if (opt_debug_level > 0)
    env.set_debug_level(1);

  human::dos_exec_context *c = env.create_context();
  {
    human::shell p(c);
    status = p.exec(args[0], args + 1, environ);
  }
  delete c;
}

void *
vx68k_app::run_machine_thread(void *data)
  throw ()
{
  sigset_t sigs;
  sigemptyset(&sigs);
#ifdef SIGHUP
  sigaddset(&sigs, SIGHUP);
#endif
#ifdef SIGINT
  sigaddset(&sigs, SIGINT);
#endif
#ifdef SIGTERM
  sigaddset(&sigs, SIGTERM);
#endif
  pthread_sigmask(SIG_BLOCK, &sigs, NULL);
  try
    {
      vx68k_app *app = static_cast<vx68k_app *>(data);
      I(app != NULL);
      app->run_machine();
    }
  catch (exception &x)
    {
      fprintf(stderr, _("Unhandled exception in thread: %s\n"), x.what());
    }

  return NULL;
}

void
vx68k_app::run()
{
  if (opt_single_threaded)
    {
      run_machine();

      gdk_threads_enter();
      gtk_main();
      gdk_threads_leave();
    }
  else
    {
      pthread_t vm_thread;
      pthread_create(&vm_thread, NULL, &run_machine_thread, this);

      gdk_threads_enter();
      gtk_main();
      gdk_threads_leave();

      pthread_join(vm_thread, NULL);
    }
}

/* Window management.  */

namespace
{
  void
  handle_about_ok_clicked(GtkButton *button, gpointer data) throw ()
  {
    GtkWidget *dialog = gtk_widget_get_toplevel(GTK_WIDGET(button));

    gtk_widget_destroy(dialog);
  }
} // namespace (unnamed)

void
vx68k_app::show_about_dialog()
{
  GtkWidget *dialog = gtk_dialog_new();

  try
    {
      gtk_window_set_policy(GTK_WINDOW(dialog), false, false, false);
      gtk_window_set_position(GTK_WINDOW(dialog), GTK_WIN_POS_CENTER);
      gtk_window_set_modal(GTK_WINDOW(dialog), true);
      if (main_window != NULL)
	gtk_window_set_transient_for(GTK_WINDOW(dialog),
				     GTK_WINDOW(main_window));

      const char *title_format = _("About %s");
      char *title;
#ifdef HAVE_ASPRINTF
      asprintf(&title, title_format, PROGRAM);
#else
      title = static_cast<char *>(malloc(strlen(title_format) - 2
					 + strlen(PROGRAM) + 1));
      sprintf(title, title_format, PROGRAM);
#endif

      gtk_window_set_title(GTK_WINDOW(dialog), title);
      free(title);

      GtkWidget *vbox1 = gtk_vbox_new(false, 10);
      gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->vbox), vbox1,
			 false, false, 0);

      gtk_widget_show(vbox1);
      gtk_container_set_border_width(GTK_CONTAINER(vbox1), 20);
      {
	const char *version_format = _("%s %s");
	char *version;
#ifdef HAVE_ASPRINTF
	asprintf(&version, version_format, PROGRAM, VERSION);
#else
	version = static_cast<char *>(malloc(strlen(version_format) - 2 * 2
					     + strlen(PROGRAM)
					     + strlen(VERSION) + 1));
	sprintf(version, version_format, PROGRAM, VERSION);
#endif

	GtkWidget *version_label = gtk_label_new(version);
	gtk_box_pack_start(GTK_BOX(vbox1), version_label, false, false, 0);

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
	gtk_box_pack_start(GTK_BOX(vbox1), copyright_label, false, false, 0);

	gtk_widget_show(copyright_label);
	gtk_misc_set_alignment(GTK_MISC(copyright_label), 0.0, 0.5);
	gtk_label_set_justify(GTK_LABEL(copyright_label), GTK_JUSTIFY_LEFT);
	free(copyright);

	const char *notice1
	  = _("This is free software; see the source for copying conditions.\n"
	      "There is NO WARRANTY; not even for MERCHANTABILITY\n"
	      "or FITNESS FOR A PARTICULAR PURPOSE.");
	GtkWidget *notice1_label = gtk_label_new(notice1);
	gtk_box_pack_start(GTK_BOX(vbox1), notice1_label, false, false, 0);

	gtk_widget_show(notice1_label);
	gtk_misc_set_alignment(GTK_MISC(notice1_label), 0.0, 0.5);
	gtk_label_set_justify(GTK_LABEL(notice1_label), GTK_JUSTIFY_LEFT);
	// gtk_label_set_line_wrap(GTK_LABEL(notice1_label), true);

	const char *report_bugs
	  = _("Send comments or report bugs to <vx68k@lists.hypercore.co.jp>.");
	GtkWidget *report_bugs_label = gtk_label_new(report_bugs);
	gtk_box_pack_start(GTK_BOX(vbox1), report_bugs_label, false, false, 0);

	gtk_widget_show(report_bugs_label);
	gtk_misc_set_alignment(GTK_MISC(report_bugs_label), 0.0, 0.5);
	gtk_label_set_justify(GTK_LABEL(report_bugs_label), GTK_JUSTIFY_LEFT);
	gtk_label_set_line_wrap(GTK_LABEL(report_bugs_label), true);
      }

      const char *ok = _("OK");
      GtkWidget *ok_button = gtk_button_new_with_label(ok);
      gtk_box_pack_start(GTK_BOX(GTK_DIALOG(dialog)->action_area),
			 ok_button, false, false, 0);

      gtk_widget_show(ok_button);
      gtk_window_set_focus(GTK_WINDOW(dialog), ok_button);
      gtk_signal_connect(GTK_OBJECT(ok_button), "clicked",
			 GTK_SIGNAL_FUNC(&handle_about_ok_clicked), this);
    }
  catch (...)
    {
      gtk_widget_destroy(dialog);
      throw;
    }

  gtk_widget_show(dialog);
}

namespace
{
  /* Handles a `run' command.  */
  void
  handle_run_command(gpointer data, guint i, GtkWidget *item) throw ()
  {
    g_message("`run' command is not implemented yet");
  }

  /* Handles a `FD load' command.  */
  void
  handle_fd_load_command(gpointer data, guint i, GtkWidget *item) throw ()
  {
    g_message("`FD load' command is not implemented yet");
  }

  /* Handles a `FD eject' command.  */
  void
  handle_fd_eject_command(gpointer data, guint i, GtkWidget *item) throw ()
  {
    g_message("`FD eject' command is not implemented yet");
  }

  /* Handles an `about' command.  */
  void
  handle_about_command(gpointer data, guint, GtkWidget *item) throw ()
  {
    vx68k_app *app = static_cast<vx68k_app *>(data);

    app->show_about_dialog();
  }
} // namespace (unnamed)

GtkWidget *
vx68k_app::create_window()
{
  if (main_window == NULL)
    {
      main_window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
      gtk_window_set_title(GTK_WINDOW(main_window), PROGRAM);
      gtk_signal_connect(GTK_OBJECT(main_window), "delete_event",
			 GTK_SIGNAL_FUNC(&gtk_main_quit), this);

      GtkAccelGroup *ag1 = gtk_accel_group_new();
      gtk_window_add_accel_group(GTK_WINDOW(main_window), ag1);
      gtk_accel_group_unref(ag1);

      {
	GtkWidget *vbox = gtk_vbox_new(false, 0);
	gtk_widget_show(vbox);
	gtk_container_add(GTK_CONTAINER(main_window), vbox);
	{
	  GtkItemFactory *ifactory
	    = gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<Window>", ag1);
#define ITEM_FACTORY_CALLBACK(f) (reinterpret_cast<GtkItemFactoryCallback>(f))
	  GtkItemFactoryEntry entries[]
	    = {{_("/_File/_Run..."), NULL,
		ITEM_FACTORY_CALLBACK(&handle_run_command), 0, "<Item>"},
	       {_("/_File/"), NULL, NULL, 0, "<Separator>"},
	       {_("/_File/FD _0/_Load..."), NULL,
		ITEM_FACTORY_CALLBACK(&handle_fd_load_command), 0, "<Item>"},
	       {_("/_File/FD _0/_Eject"), NULL,
		ITEM_FACTORY_CALLBACK(&handle_fd_eject_command), 0, "<Item>"},
	       {_("/_File/FD _1/_Load..."), NULL,
		ITEM_FACTORY_CALLBACK(&handle_fd_load_command), 1, "<Item>"},
	       {_("/_File/FD _1/_Eject"), NULL,
		ITEM_FACTORY_CALLBACK(&handle_fd_eject_command), 1, "<Item>"},
	       {_("/_File/"), NULL, NULL, 0, "<Separator>"},
	       {_("/_File/E_xit"), NULL,
		ITEM_FACTORY_CALLBACK(&gtk_main_quit), 1, "<Item>"},
	       {_("/_Help/_About..."), NULL,
		ITEM_FACTORY_CALLBACK(&handle_about_command), 0, "<Item>"}};
#undef ITEM_FACTORY_CALLBACK
	  gtk_item_factory_create_items(ifactory,
					sizeof entries / sizeof entries[0],
					entries, this);

	  gtk_widget_show(ifactory->widget);
	  gtk_box_pack_start(GTK_BOX(vbox), ifactory->widget, false, false, 0);
	  //gtk_object_unref(GTK_OBJECT(ifactory));
	}

	GtkWidget *statusbar = gtk_statusbar_new();
	gtk_widget_show(statusbar);
	gtk_box_pack_end(GTK_BOX(vbox), statusbar, false, false, 0);

	GtkWidget *console_widget = con.create_widget();
	gtk_widget_show(console_widget);
#if 0
	{
	  GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
	  gtk_widget_show(scrolled_window);
	  gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, true, true, 0);

	  gtk_scrolled_window_add_with_viewport
	    (GTK_SCROLLED_WINDOW(scrolled_window), console_widget);

	  GdkGeometry geometry = {0, 0, 0, 0, 0, 0, 1, 1};
	  gtk_window_set_geometry_hints(GTK_WINDOW(main_window),
					scrolled_window,
					&geometry, GDK_HINT_RESIZE_INC);
	}
#else
	gtk_box_pack_start(GTK_BOX(vbox), console_widget, true, true, 0);

	GdkGeometry geometry = {0, 0, 0, 0, 0, 0, 1, 1};
	gtk_window_set_geometry_hints(GTK_WINDOW(main_window),
				      console_widget,
				      &geometry, GDK_HINT_RESIZE_INC);
#endif

	gtk_widget_grab_focus(console_widget);
      }
    }

  return main_window;
}

const size_t MEMSIZE = 4 * 1024 * 1024; // FIXME

vx68k_app::vx68k_app(const char *const *a)
  : args(a),
    vm(opt_memory_size > 0 ? opt_memory_size : MEMSIZE),
    con(&vm),
    main_window(NULL)
{
  gtk_widget_set_default_visual(gdk_rgb_get_visual());
  vm.connect(&con);
}

namespace
{
  int opt_help = false;
  int opt_version = false;

  bool
  parse_options(int argc, char **argv)
  {
    static const struct option longopts[] =
    {
      {"memory-size", required_argument, NULL, 'm'},
      {"one-thread", no_argument, &vx68k_app::opt_single_threaded, true},
      {"help", no_argument, &opt_help, true},
      {"version", no_argument, &opt_version, true},
      {"debug", no_argument, &vx68k_app::opt_debug_level, 1},
      {NULL, 0, NULL, 0}
    };

    int optc;
    do
      {
	int index;
	optc = getopt_long(argc, argv, "m:", longopts, &index);
	switch (optc)
	  {
	  case 'm':
	    {
	      int mega = atoi(optarg);
	      if (mega < 1 || mega > 12)
		{
		  fprintf(stderr, _("%s: invalid memory size `%s'\n"),
			  argv[0], optarg);
		  return false;
		}

	      vx68k_app::opt_memory_size = mega * 1024 * 1024;
	    }
	  break;

	  case '?':		// unknown option
	    return false;
	  case 0:		// long option
	  case -1:		// no more options
	    break;
	  default:
	    abort();
	  }
      }
    while (optc != -1);

    return true;
  }

  void
  display_help(const char *arg0)
  {
    // XXX `--debug' is undocumented
    printf(_("Usage: %s [OPTION]... [--] COMMAND [ARGUMENT]...\n"), arg0);
    printf(_("Run X68000 COMMAND on a virtual machine.\n"));
    printf("\n");
    printf(_("  -M, --memory-size=N   allocate N megabytes for main memory\n"));
    printf(_("      --one-thread      run in one thread\n"));
    printf(_("      --help            display this help and exit\n"));
    printf(_("      --version         output version information and exit\n"));
    printf("\n");
    printf(_("Report bugs to <vx68k@lists.hypercore.co.jp>.\n"));
  }
} // (unnamed)

/* vx68k main.  */
int
main(int argc, char **argv)
{
  g_thread_init(NULL);
  gtk_set_locale();
  gtk_init(&argc, &argv);

  if (!parse_options(argc, argv))
    {
      fprintf(stderr, _("Try `%s --help' for more information.\n"), argv[0]);
      return EXIT_FAILURE;
    }

  if (opt_version)
    {
      printf("%s %s\n", PACKAGE, VERSION);
      printf(_("Copyright (C) %s Hypercore Software Design, Ltd.\n"),
	     COPYRIGHT_YEAR);
      return EXIT_SUCCESS;
    }

  if (opt_help)
    {
      display_help(argv[0]);
      return EXIT_SUCCESS;
    }

  if (argc <= optind)
    {
      fprintf(stderr, _("%s: missing command argument\n"), argv[0]);
      fprintf(stderr, _("Try `%s --help' for more information.\n"), argv[0]);
      return EXIT_FAILURE;
    }

  try
    {
      vx68k_app app(argv + optind);

      GtkWidget *window = app.create_window();
      gtk_widget_show(window);

      app.run();
      return app.exit_status();
    }
  catch (exception &x)
    {
      fprintf(stderr, _("%s: unhandled exception: %s\n"), argv[0], x.what());
      return EXIT_FAILURE;
    }
}
