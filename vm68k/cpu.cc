/* vx68k - Virtual X68000
   Copyright (C) 1998 Hypercore Software Design, Ltd.

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.  */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#undef const
#undef inline

#include "vm68k/cpu.h"

#include <algorithm>
#include <cassert>

execution_context::execution_context (memory *m)
  : mem (m)
{
}

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

void
cpu::set_exception_listener (exception_listener *l)
{
  context.exception = l;
}

void
cpu::run ()
{
  for (;;)
    {
      int fc = 1 ? SUPER_PROGRAM : USER_PROGRAM;
      int w = context.mem->read16 (fc, context.regs.pc);
      assert (w >= 0 && w < 0x10000);
      insn[w] (w, &context);
    }
}

void
cpu::set_handlers (int code, int mask, insn_handler h)
{
  assert (code >= 0);
  assert (code < 0x10000);
  code &= ~mask;
  for (int i = code; i <= (code | mask); ++i)
    {
      if ((i & ~mask) == code)
	insn[i] = h;
    }
}

/* Execute an illegal instruction.  */
void
cpu::illegal_insn (int, execution_context *)
{
  // Notify listener.  The listener must handle this case.
  abort ();			// FIXME
}

cpu::cpu (memory *m)
  : context (m)
{
  std::fill (insn + 0, insn + 0x10000, &illegal_insn);
}

