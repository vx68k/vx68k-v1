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
#include "getopt.h"

#include <vx68k/human.h>
#include <vx68k/gtk.h>
#include <vx68k/version.h>

#include <gtk/gtk.h>
#include <libintl.h>
#include <pthread.h>
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#else
# define O_RDWR 2
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <memory>
#include <stdexcept>
#include <csignal>
#include <cstdlib>
#include <cstdio>

#ifdef HAVE_NANA_H
# include <nana.h>
# include <cstdio>
#else
# include <cassert>
# define I assert
#endif

#define _(MSG) gettext(MSG)

extern char **environ;

using namespace vx68k::gtk;
using namespace vx68k;
using namespace std;

#define PROGRAM "Virtual X68000"

#define COPYRIGHT_YEAR "1998-2000"

/* Application.  */
class gtk_app: public virtual console_callback
{
public:
  /* Program options.  */
  static size_t opt_memory_size;
  static int opt_single_threaded;
  static int opt_debug_level;

protected:
  static void *run_machine_thread(void *) throw ();

private:
  machine vm;
  gtk_console con;

  /* Exit status of the VM.  */
  int vm_status;

  /* Program arguments for the VM.  */
  const char *const *vm_args;

  /* Thread that is executing the VM.  This member is set to
     pthread_self() when no thread is running.  */
  pthread_t vm_thread;

  /* Main console window of this application.  */
  gtk_console_window *main_window;

public:
  gtk_app();

public:
  void window_closed();

protected:
  void run_machine();

public:
  /* Runs a boot thread.  */
  void run_boot() throw ();

  /* Boots the first floppy on the VM.  */
  void boot();

  /* Runs a DOS program on the VM.  */
  void run(const char *const *args);

  /* Waits for the program to exit.  */
  void join(int *st);

public:
  /* Loads an image file on a FD unit.  */
  void load_fd_image(unsigned int u, int fildes)
  {vm.load_fd(u, fildes);}

public:
  gtk_console_window *create_window();
};

size_t gtk_app::opt_memory_size = 0;
int gtk_app::opt_single_threaded = false;
int gtk_app::opt_debug_level = 0;

void
gtk_app::run_boot() throw ()
{
  try
    {
      vm.boot();
    }
  catch (illegal_instruction_exception &e)
    {
      /* NOTE: This exception should have the address information.  */
      context *c = vm.master_context();
      uint16_type op = word_size::uget(*c->mem, memory::SUPER_DATA, c->regs.pc);
      char buf[sizeof "Illegal instruction 0x1234 at 0x12345678"];
      sprintf(buf, "Illegal instruction 0x%04x at 0x%08lx",
	      op, c->regs.pc + 0UL);
      main_window->set_status_text(buf);
    }
  catch (memory_exception &x)
    {
      char buf[sizeof "Address error at 0x12345678 (status=0x12)"];
      if (x.vecno() == 3u)
	sprintf(buf, "Address error at 0x%08lx (status=0x%02x)",
		x._address + 0UL, x._status);
      else
	sprintf(buf, "Bus error at 0x%08lx (status=0x%02x)",
		x._address + 0UL, x._status);
      main_window->set_status_text(buf);
    }
  catch (exception &x)
    {
      fprintf(stderr, _("Unhandled exception in thread: %s\n"), x.what());
    }
}

void
gtk_app::run_machine()
{
  human::dos env(&vm);
  if (opt_debug_level > 0)
    env.set_debug_level(1);

  human::dos_exec_context *c = env.create_context();
  {
    human::shell p(c);
    vm_status = p.exec(vm_args[0], vm_args + 1, environ);
  }
  delete c;
}

void *
gtk_app::run_machine_thread(void *data)
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
      gtk_app *app = static_cast<gtk_app *>(data);
      I(app != NULL);
      app->run_machine();
    }
  catch (exception &x)
    {
      fprintf(stderr, _("Unhandled exception in thread: %s\n"), x.what());
    }

  return NULL;
}

namespace
{
  void *
  call_run_boot(void *data) throw ()
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

    gtk_app *app = static_cast<gtk_app *>(data);
    app->run_boot();
    return app;
  }
}

void
gtk_app::boot()
{
  vm_status = 0;

  if (opt_single_threaded)
    run_boot();
  else
    pthread_create(&vm_thread, NULL, &call_run_boot, this);
}

void
gtk_app::run(const char *const *args)
{
  vm_args = args;
  vm_status = 0;

  if (opt_single_threaded)
    run_machine();
  else
    pthread_create(&vm_thread, NULL, &run_machine_thread, this);
}

void
gtk_app::join(int *status)
{
  if (vm_thread != pthread_self())
    {
      pthread_cancel(vm_thread);
      pthread_join(vm_thread, NULL);
      vm_thread = pthread_self();
    }

  if (status != NULL)
    *status = vm_status;
}

/* Window management.  */

gtk_console_window *
gtk_app::create_window()
{
  if (main_window == NULL)
    {
      main_window = new gtk_console_window(con.create_widget());
      main_window->add_callback(this);
    }

  return main_window;
}

void
gtk_app::window_closed()
{
  gtk_main_quit();
}

const size_t MEMSIZE = 4 * 1024 * 1024; // FIXME

gtk_app::gtk_app()
  : vm(opt_memory_size > 0 ? opt_memory_size : MEMSIZE),
    con(&vm),
    main_window(NULL)
{
  vm_thread = pthread_self();
  gtk_widget_set_default_visual(gdk_rgb_get_visual());
  vm.connect(&con);
}

namespace
{
  /* Boot mode.  */
  int opt_boot = false;

  /* File names of FD images.  */
  const char *opt_fd_images[2] = {"", ""};

  int opt_help = false;
  int opt_version = false;

  bool
  parse_options(int argc, char **argv)
  {
    static const struct option longopts[]
      = {{"boot", no_argument, &opt_boot, true},
	 {"fd0-image", required_argument, NULL, '0'},
	 {"fd1-image", required_argument, NULL, '1'},
	 {"memory-size", required_argument, NULL, 'm'},
	 {"one-thread", no_argument, &gtk_app::opt_single_threaded, true},
	 {"debug", no_argument, &gtk_app::opt_debug_level, 1},
	 {"help", no_argument, &opt_help, true},
	 {"version", no_argument, &opt_version, true},
	 {NULL, 0, NULL, 0}};

    for (;;)
      {
	int index;
	int opt = getopt_long(argc, argv, "0:1:bm:", longopts, &index);
	if (opt == -1)		// no more options
	  break;

	switch (opt)
	  {
	  case '0':
	    opt_fd_images[0] = optarg;
	    break;

	  case '1':
	    opt_fd_images[1] = optarg;
	    break;

	  case 'b':
	    opt_boot = true;
	    break;

	  case 'm':
	    {
	      int mega = atoi(optarg);
	      if (mega < 1 || mega > 12)
		{
		  fprintf(stderr, _("%s: invalid memory size `%s'\n"),
			  argv[0], optarg);
		  return false;
		}

	      gtk_app::opt_memory_size = mega * 1024 * 1024;
	    }
	  break;

	  case 0:		// long option
	    break;

	  case '?':		// unknown option
	    return false;

	  default:
	    // logic error
	    abort();
	  }
      }

    return true;
  }

  void
  display_help(const char *arg0)
  {
    // XXX `--debug' is undocumented
    printf(_("Usage: %s [OPTION]... [--] [COMMAND [ARGUMENT]...]\n"), arg0);
    printf(_("Run X68000 COMMAND on a virtual machine.\n"));
    printf("\n");
    printf(_("  -b, --boot            boot from media\n"));
    printf(_("  -0, --fd0-image=FILE  load FILE on FD unit 0 as an image\n"));
    printf(_("  -1, --fd1-image=FILE  load FILE on FD unit 1 as an image\n"));
    printf(_("  -m, --memory-size=N   allocate N megabytes for main memory\n"));
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

#ifdef LOCALEDIR
  bindtextdomain(PACKAGE, LOCALEDIR);
#endif
  textdomain(PACKAGE);
    
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

#if 0
  if (argc <= optind)
    {
      fprintf(stderr, _("%s: missing command argument\n"), argv[0]);
      fprintf(stderr, _("Try `%s --help' for more information.\n"), argv[0]);
      return EXIT_FAILURE;
    }
#endif

  try
    {
      gtk_app app;

      auto_ptr<gtk_console_window> window(app.create_window());
      window->show();

      for (int u = 0; u != 2; ++u)
	if (opt_fd_images[u][0] != '\0')
	  {
	    int fildes = open(opt_fd_images[u], O_RDWR);
	    if (fildes == -1)
	      {
		perror(opt_fd_images[u]);
		return EXIT_FAILURE;
	      }

	    try
	      {
		app.load_fd_image(u, fildes);
	      }
	    catch (...)
	      {
		close(fildes);
		throw;
	      }
	  }

      if (opt_boot)
	{
	  app.boot();
	}
      else
	{
	  if (optind < argc)
	    app.run(argv + optind);
	}

      gdk_threads_enter();
      gtk_main();
      gdk_threads_leave();

      int status;
      app.join(&status);
      return status;
    }
  catch (exception &x)
    {
      fprintf(stderr, _("%s: unhandled exception: %s\n"), argv[0], x.what());
      return EXIT_FAILURE;
    }
}
