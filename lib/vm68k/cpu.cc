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

#include <cstdio>

#include "debug.h"

using namespace vm68k;
using namespace std;

void
execution_context::run()
{
  for (;;)
    {
      unsigned int op = fetchw(0);
      I(eu != NULL);
      eu->dispatch(op, this);
    }
}

execution_context::execution_context(exec_unit *e, address_space *m)
  : mem(m),
    eu(e)
{
}

#if 0
/* Set PC.  */
void
cpu::set_pc (uint32 addr)
{
  if (addr & 1 != 0)
    {
      if (context.exception != NULL)
	context.exception->address_error (&context.regs, context.mem);
      /* Fail to trap address error.  */
      abort ();
    }

  context.regs.pc = addr;
}
#endif

void
cpu::set_exception_listener (exception_listener *l)
{
#if 0
  context.exception = l;
#endif
}

void
cpu::run (execution_context *ec)
{
  I(ec != NULL);
  try
    {
      ec->run();
    }
  catch (bus_error &e)
    {
      // We should handle this in a callback function.
      fprintf(stderr, "vm68k bus error: status = 0x%x, address = 0x%lx\n",
	      e.status, (unsigned long) e.address);
      abort ();
    }
}

void
cpu::set_handlers (int code, int mask, exec_unit::insn_handler h)
{
  I(code >= 0);
  I(code < 0x10000);
  eu.set_instruction(code, mask, h);
}

cpu::cpu ()
{
}

