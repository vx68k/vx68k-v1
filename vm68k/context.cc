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

#include <vm68k/cpu.h>

#ifdef HAVE_NANA_H
# include <nana.h>
# include <cstdio>
#else
# include <cassert>
# define I assert
#endif

using namespace vm68k;
using namespace std;

void
context::set_supervisor_state(bool state)
{
  if (state)
    {
      if (!regs.sr.supervisor_state())
	{
	  regs.usp = regs.a[7];
	  regs.sr.set_s_bit(true);
	  regs.a[7] = regs.ssp;

	  pfc = SUPER_PROGRAM;
	  dfc = SUPER_DATA;
	}
    }
  else
    {
      if (regs.sr.supervisor_state())
	{
	  regs.ssp = regs.a[7];
	  regs.sr.set_s_bit(false);
	  regs.a[7] = regs.usp;

	  pfc = USER_PROGRAM;
	  dfc = USER_DATA;
	}
    }
}

context::context(address_space *m)
  : mem(m),
    pfc(regs.sr.supervisor_state() ? SUPER_PROGRAM : USER_PROGRAM),
    dfc(regs.sr.supervisor_state() ? SUPER_DATA : USER_DATA)
{
}

