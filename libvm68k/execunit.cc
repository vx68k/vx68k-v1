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

  void bsr(int op, execution_context *ec)
    {
      assert(ec != NULL);
      int len = 2;
      int32 disp = op & 0xff;
      if (disp == 0)
	{
	  len = 4;
	  int fc = 1 ? SUPER_PROGRAM : USER_PROGRAM; // FIXME.
	  disp = ec->mem->getw(fc, ec->regs.pc + 2);
	  if (disp >= 0x8000)
	    disp -= 0x10000;
	}
      else if (disp >= 0x80)
	disp -= 0x100;
#ifdef TRACE_STEPS
      fprintf(stderr, " bsr 0x%lx\n", (unsigned long) (ec->regs.pc + 2 + disp));
#endif

      int fc = 1 ? SUPER_PROGRAM : USER_PROGRAM; // FIXME.
      ec->mem->putl(fc, ec->regs.a[7], ec->regs.pc + len);
      ec->regs.a[7] -= 4;
      ec->regs.pc += 2 + disp;

      // Condition code is not affected.
    }

  void lea_absl_a(int op, execution_context *ec)
    {
      assert(ec != NULL);
      int reg = op >> 9 & 0x7;
      int fc = 1 ? SUPER_PROGRAM : USER_PROGRAM; // FIXME.
      uint32 address = ec->mem->getl(fc, ec->regs.pc + 2);
#ifdef TRACE_STEPS
      fprintf(stderr, " lea 0x%lx:l,%%a%d\n", (unsigned long) address, reg);
#endif

      ec->regs.a[reg] = address;

      // Condition code is not affected.
      ec->regs.pc += 6;
    }

  void link(int op, execution_context *ec)
    {
      int reg = op & 0x0007;
      int fc = 1 ? SUPER_PROGRAM : USER_PROGRAM; // FIXME.
      int32 disp = ec->mem->getw(fc, ec->regs.pc + 2);
      if (disp >= 0x8000)
	disp -= 0x10000;
#ifdef TRACE_STEPS
      fprintf(stderr, " link %%a%d,#%d\n", reg, disp);
#endif

      // FIXME.
      ec->mem->putl(fc, ec->regs.a[7] - 4, ec->regs.a[reg]);
      ec->regs.a[7] -= 4;
      ec->regs.a[reg] = ec->regs.a[7];
      ec->regs.a[7] += disp;

      ec->regs.pc += 4;
    }

  void movel_a_predec(int op, execution_context *ec)
    {
      assert(ec != NULL);
      int s_reg = op & 0x7;
      int d_reg = op >> 9 & 0x7;
#ifdef TRACE_STEPS
      fprintf(stderr, " movel %%a%d,%%a%d@-\n", s_reg, d_reg);
#endif

      // FIXME.
      int fc = 1 ? SUPER_DATA : USER_DATA; // FIXME.
      ec->mem->putl(fc, ec->regs.a[d_reg] - 4, ec->regs.a[s_reg]);
      ec->regs.a[d_reg] -= 4;

      ec->regs.pc += 2;
    }

  /* movem regs to EA (postdec).  */
  void moveml_r_predec(int op, execution_context *ec)
    {
      int reg = op & 0x0007;
      int fc = 1 ? SUPER_PROGRAM : USER_PROGRAM; // FIXME.
      unsigned int bitmap = ec->mem->getw(fc, ec->regs.pc + 2);
#ifdef TRACE_STEPS
      fprintf(stderr, " moveml #0x%x,%%a%d@-\n", bitmap, reg);
#endif

      for (int i = 0; i != 16; ++i)
	{
	  if (bitmap & 1 != 0)
	    {
	      // FIXME.
	      ec->mem->putl(fc, ec->regs.a[reg] - 4, ec->regs.d[15 - i]);
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
  eu->set_instruction(0x2108, 0x0e07, &movel_a_predec);
  eu->set_instruction(0x41f9, 0x0e00, &lea_absl_a);
  eu->set_instruction(0x48e0, 0x0007, &moveml_r_predec);
  eu->set_instruction(0x4e50, 0x0007, &link);
  eu->set_instruction(0x6100, 0x00ff, &bsr);
}

