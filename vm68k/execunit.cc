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

#include <algorithm>
#include <cstdio>
#include <cassert>

using namespace vm68k;
using namespace std;

/* Executes code.  */
void
exec_unit::execute(execution_context *ec) const
{
  assert (ec != NULL);
  for (;;)
    {
      int fc = 1 ? SUPER_PROGRAM : USER_PROGRAM; // FIXME.
      unsigned int op = ec->mem->getw (fc, ec->regs.pc);
      assert (op < 0x10000);
      instruction[op](op, ec);
    }
}

/* Sets an instruction to operation codes.  */
void
exec_unit::set_instruction(int code, int mask, insn_handler h)
{
  assert (code >= 0);
  assert (code < 0x10000);
  code &= ~mask;
  for (int i = code; i <= (code | mask); ++i)
    {
      if ((i & ~mask) == code)
	instruction[i] = h;
    }
}

exec_unit::exec_unit()
{
  fill(instruction + 0, instruction + 0x10000, &illegal);
  install_instructions(this);
}

/* Executes an illegal instruction.  */
void
exec_unit::illegal(int op, execution_context *)
{
  throw illegal_instruction();
}

namespace
{

  void link(int op, execution_context *ec)
    {
      int reg = op & 0x0007;
      int fc = 1 ? SUPER_PROGRAM : USER_PROGRAM; // FIXME.
      int32 disp = ec->mem->getw(fc, ec->regs.pc + 2);
      if (disp >= 0x8000)
	disp -= 0x10000;
      fprintf(stderr, " link a%d,#%d\n", reg, disp);

      // FIXME.
      // ec->mem->putl(fc, ec->regs.a[7] - 4, ec->regs.a[reg]);
      ec->regs.a[7] -= 4;
      ec->regs.a[reg] = ec->regs.a[7];
      ec->regs.a[7] += disp;

      ec->regs.pc += 4;
    }

  /* movem regs to EA (postdec).  */
  void movem_l_r_predec(int op, execution_context *ec)
    {
      int reg = op & 0x0007;
      int fc = 1 ? SUPER_PROGRAM : USER_PROGRAM; // FIXME.
      unsigned int bitmap = ec->mem->getw(fc, ec->regs.pc + 2);
      fprintf(stderr, " movem.l #0x%x,-(a%d)\n", bitmap, reg);

      for (int i = 0; i != 16; ++i)
	{
	  if (bitmap & 1 != 0)
	    {
	      // FIXME.
	      // ec->mem->putl(fc, ec->regs.a[reg] - 4, ec->regs.d[15 - i]);
	      ec->regs.a[reg] -= 4;
	    }
	  bitmap >>= 1;
	}

      ec->regs.pc += 4;
    }

} // (unnamed namespace)

/* Installs instructions into the execution unit.  */
void
exec_unit::install_instructions(exec_unit *eu)
{
  assert(eu != NULL);
  eu->set_instruction(0x48e0, 0x0007, &movem_l_r_predec);
  eu->set_instruction(0x4e50, 0x0007, &link);
}

