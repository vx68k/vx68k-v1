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

#include "vx68k/human.h"
#include "vx68k/memory.h"
#include "getopt.h"
#include <exception>
#include <cstdlib>
#include <cstdio>

#define _(MSG) (MSG)

using namespace std;
using namespace vx68k;

namespace
{
  int opt_help = false;
  int opt_version = false;

  const struct option longopts[] =
  {
    {"help", no_argument, &opt_help, true},
    {"version", no_argument, &opt_version, true},
    {NULL, 0, NULL, 0}
  };

  bool parse_options(int argc, char **argv)
    {
      int optc;
      do
	{
	  int index;
	  optc = getopt_long(argc, argv, "", longopts, &index);
	  switch (optc)
	    {
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
} // (unnamed namespace)

/* vx68k main.  */
int
main (int argc, char **argv)
{
  if (!parse_options(argc, argv))
    return EXIT_FAILURE;

  if (opt_version)
    {
      printf("vx68k (%s) %s\n", "Virtual X68000", VERSION);
      return EXIT_SUCCESS;
    }

  if (opt_help)
    {
      // FIXME.
      return EXIT_SUCCESS;
    }

  if (argc <= optind)
    {
      fprintf(stderr, _("Usage: %s FILE.X\n"), argv[0]);
      return EXIT_FAILURE;
    }

  try
    {
      const size_t MEMSIZE = 4 * 1024 * 1024; // FIXME
      x68k_address_space mem(MEMSIZE);

      human::dos env(&mem, MEMSIZE);
      return env.execute(argv[1], argv + 2); // FIXME
    }
  catch (exception &x)
    {
      fprintf(stderr, _("%s: Unhandled exception: %s\n"), argv[0], x.what());
      return EXIT_FAILURE;
    }
}

