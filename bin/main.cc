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

public:
  vx68k_app(const char *const *args);

protected:
  void run_machine();

public:
  void run();
  int exit_status() const
    {return status;}
  GtkWidget *create_window();
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

GtkWidget *
vx68k_app::create_window()
{
  GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
  gtk_signal_connect(GTK_OBJECT(window), "delete_event",
		     GTK_SIGNAL_FUNC(&gtk_main_quit), NULL);
  gtk_widget_show(window);
  {
    GtkWidget *vbox = gtk_vbox_new(false, 0);
    gtk_widget_show(vbox);
    gtk_container_add(GTK_CONTAINER(window), vbox);
    {
      GtkItemFactory *ifactory
	= gtk_item_factory_new(GTK_TYPE_MENU_BAR, "<Window>", NULL);
      GtkItemFactoryEntry entries[]
	= {{_("/_File/E_xit"), NULL, &gtk_main_quit, 1, "<Item>"},
	   {_("/_Help/_About..."), NULL, NULL, 0, "<Item>"}};
      gtk_item_factory_create_items(ifactory,
				    sizeof entries / sizeof entries[0],
				    entries, window);
      gtk_widget_show(ifactory->widget);
      gtk_box_pack_start(GTK_BOX(vbox), ifactory->widget, false, false, 0);
      //gtk_object_unref(GTK_OBJECT(ifactory));
    }
    {
      GtkWidget *statusbar = gtk_statusbar_new();
      gtk_widget_show(statusbar);
      gtk_box_pack_end(GTK_BOX(vbox), statusbar, false, false, 0);
    }
    {
      GtkWidget *console_widget = con.create_widget();
      gtk_widget_show(console_widget);

#if 0
      GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
      {
	gtk_widget_show(console_widget);
	gtk_scrolled_window_add_with_viewport
	  (GTK_SCROLLED_WINDOW(scrolled_window), console_widget);
      }
      gtk_widget_show(scrolled_window);
      gtk_box_pack_start(GTK_BOX(vbox), scrolled_window, true, true, 0);

      GdkGeometry geometry = {0, 0, 0, 0, 0, 0, 1, 1};
      gtk_window_set_geometry_hints(GTK_WINDOW(window),
				    scrolled_window,
				    &geometry, GDK_HINT_RESIZE_INC);
#else
      gtk_box_pack_start(GTK_BOX(vbox), console_widget, true, true, 0);

      GdkGeometry geometry = {0, 0, 0, 0, 0, 0, 1, 1};
      gtk_window_set_geometry_hints(GTK_WINDOW(window),
				    console_widget,
				    &geometry, GDK_HINT_RESIZE_INC);
#endif

      gtk_widget_grab_focus(console_widget);
    }
  }

  return window;
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

const size_t MEMSIZE = 4 * 1024 * 1024; // FIXME

vx68k_app::vx68k_app(const char *const *a)
  : args(a),
    vm(opt_memory_size > 0 ? opt_memory_size : MEMSIZE),
    con(&vm)
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
      fprintf(stderr, _("%s: Unhandled exception: %s\n"), argv[0], x.what());
      return EXIT_FAILURE;
    }
}
