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
      unsigned int op = ec->fetchw(0);
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
  void addqw_a(int op, execution_context *ec)
    {
      assert(ec != NULL);
      int value = op >> 9 & 0x7;
      int reg = op & 0x7;
      if (value == 0)
	value = 8;
#ifdef TRACE_STEPS
      fprintf(stderr, " addqw #%d,%%a%d\n", value, reg);
#endif

      // XXX: The condition codes are not affected.
      (&ec->regs.a0)[reg] += value;

      ec->regs.pc += 2;
    }

  void beq(int op, execution_context *ec)
    {
      assert(ec != NULL);
      int len = 2;
      int disp = op & 0xff;
      if (disp == 0)
	{
	  len = 4;
	  disp = extsw(ec->fetchw(2));
	}
      else
	disp = extsb(disp);
#ifdef TRACE_STEPS
      fprintf(stderr, " beq 0x%lx\n", (unsigned long) (ec->regs.pc + 2 + disp));
#endif

      // XXX: The condition codes are not affected.
      ec->regs.pc += ec->regs.sr.eq() ? 2 + disp : len;
    }

  void bge(int op, execution_context *ec)
    {
      assert(ec != NULL);
      int len = 2;
      int disp = op & 0xff;
      if (disp == 0)
	{
	  len = 4;
	  disp = extsw(ec->fetchw(2));
	}
      else
	disp = extsb(disp);
#ifdef TRACE_STEPS
      fprintf(stderr, " bge 0x%lx\n", (unsigned long) (ec->regs.pc + 2 + disp));
#endif

      // XXX: The condition codes are not affected.
      ec->regs.pc += ec->regs.sr.ge() ? 2 + disp : len;
    }

  void bne(int op, execution_context *ec)
    {
      assert(ec != NULL);
      int len = 2;
      int disp = op & 0xff;
      if (disp == 0)
	{
	  len = 4;
	  disp = extsw(ec->fetchw(2));
	}
      else
	disp = extsb(disp);
#ifdef TRACE_STEPS
      fprintf(stderr, " bne 0x%lx\n", (unsigned long) (ec->regs.pc + 2 + disp));
#endif

      // XXX: The condition codes are not affected.
      ec->regs.pc += ec->regs.sr.ne() ? 2 + disp : len;
    }

  void bra(int op, execution_context *ec)
    {
      assert(ec != NULL);
      int len = 2;
      int disp = op & 0xff;
      if (disp == 0)
	{
	  len = 4;
	  disp = extsw(ec->fetchw(2));
	}
      else
	disp = extsb(disp);
#ifdef TRACE_STEPS
      fprintf(stderr, " bra 0x%lx\n", (unsigned long) (ec->regs.pc + 2 + disp));
#endif

      // XXX: The condition codes are not affected.
      ec->regs.pc += 2 + disp;
    }

  void bsr(int op, execution_context *ec)
    {
      assert(ec != NULL);
      int len = 2;
      int disp = op & 0xff;
      if (disp == 0)
	{
	  len = 4;
	  disp = extsw(ec->fetchw(2));
	}
      else
	disp = extsb(disp);
#ifdef TRACE_STEPS
      fprintf(stderr, " bsr 0x%lx\n", (unsigned long) (ec->regs.pc + 2 + disp));
#endif

      // XXX: The condition codes are not affected.
      int fc = ec->data_fc();
      ec->mem->putl(fc, ec->regs.a7 - 4, ec->regs.pc + len);
      ec->regs.a7 -= 4;
      ec->regs.pc += 2 + disp;
    }

  void clrw_predec(int op, execution_context *ec)
    {
      assert(ec != NULL);
      int reg = op & 0x7;
#ifdef TRACE_STEPS
      fprintf(stderr, " clrw %%a%d@-\n", reg);
#endif

      int fc = ec->data_fc();
      ec->mem->putw(fc, (&ec->regs.a0)[reg] - 2, 0);
      (&ec->regs.a0)[reg] -= 2;
      ec->regs.sr.set_cc(0);

      ec->regs.pc += 2;
    }

  void lea_offset_a(int op, execution_context *ec)
    {
      assert(ec != NULL);
      int s_reg = op & 0x7;
      int d_reg = op >> 9 & 0x7;
      int offset = extsw(ec->fetchw(2));
#ifdef TRACE_STEPS
      fprintf(stderr, " lea %%a%d@(%ld),%%a%d\n", s_reg, (long) offset, d_reg);
#endif

      // XXX: The condition codes are not affected.
      (&ec->regs.a0)[d_reg] = (&ec->regs.a0)[s_reg] + offset;

      ec->regs.pc += 4;
    }

  void lea_absl_a(int op, execution_context *ec)
    {
      assert(ec != NULL);
      int reg = op >> 9 & 0x7;
      uint32 address = ec->fetchl(2);
#ifdef TRACE_STEPS
      fprintf(stderr, " lea 0x%lx:l,%%a%d\n", (unsigned long) address, reg);
#endif

      // XXX: The condition codes are not affected.
      (&ec->regs.a0)[reg] = address;

      ec->regs.pc += 6;
    }

  void link_a(int op, execution_context *ec)
    {
      int reg = op & 0x0007;
      int disp = extsw(ec->fetchw(2));
#ifdef TRACE_STEPS
      fprintf(stderr, " link %%a%d,#%d\n", reg, disp);
#endif

      // XXX: The condition codes are not affected.
      int fc = ec->data_fc();
      ec->mem->putl(fc, ec->regs.a7 - 4, (&ec->regs.a0)[reg]);
      ec->regs.a7 -= 4;
      (&ec->regs.a0)[reg] = ec->regs.a7;
      ec->regs.a7 += disp;

      ec->regs.pc += 4;
    }

  void moveb_postinc_postinc(int op, execution_context *ec)
    {
      assert(ec != NULL);
      int s_reg = op & 0x7;
      int d_reg = op >> 9 & 0x7;
#ifdef TRACE_STEPS
      fprintf(stderr, " moveb %%a%d@+,%%a%d@+\n", s_reg, d_reg);
#endif

      int fc = ec->data_fc();
      int value = extsb(ec->mem->getb(fc, (&ec->regs.a0)[s_reg]));
      ec->mem->putb(fc, (&ec->regs.a0)[d_reg], value);
      (&ec->regs.a0)[s_reg] += 1;
      (&ec->regs.a0)[d_reg] += 1;
      ec->regs.sr.set_cc(value);

      ec->regs.pc += 2;
    }

  void movew_absl_d(int op, execution_context *ec)
    {
      assert(ec != NULL);
      int reg = op >> 9 & 0x7;
      uint32 address = ec->fetchl(2);
#ifdef TRACE_STEPS
      fprintf(stderr, " movew 0x%x,%%d%d\n", address, reg);
#endif

      int fc = ec->data_fc();
      int value = extsw(ec->mem->getw(fc, address));
      (&ec->regs.d0)[reg] = value;
      ec->regs.sr.set_cc(value);

      ec->regs.pc += 2 + 4;
    }

  void movew_d_predec(int op, execution_context *ec)
    {
      assert(ec != NULL);
      int s_reg = op & 0x7;
      int d_reg = op >> 9 & 0x7;
#ifdef TRACE_STEPS
      fprintf(stderr, " movew %%d%d,%%a%x@-\n", s_reg, d_reg);
#endif

      int fc = ec->data_fc();
      int value = extsw((&ec->regs.d0)[s_reg]);
      ec->mem->putw(fc, (&ec->regs.a0)[d_reg] - 2, value);
      (&ec->regs.a0)[d_reg] -= 2;
      ec->regs.sr.set_cc(value);

      ec->regs.pc += 2;
    }

  void movew_d_absl(int op, execution_context *ec)
    {
      assert(ec != NULL);
      int reg = op & 0x7;
      uint32 address = ec->fetchl(2);
#ifdef TRACE_STEPS
      fprintf(stderr, " movew %%d%d,0x%x\n", reg, address);
#endif

      int fc = ec->data_fc();
      int value = extsw((&ec->regs.d0)[reg]);
      ec->mem->putw(fc, address, value);
      ec->regs.sr.set_cc(value);

      ec->regs.pc += 2 + 4;
    }

  void movel_a_predec(int op, execution_context *ec)
    {
      assert(ec != NULL);
      int s_reg = op & 0x7;
      int d_reg = op >> 9 & 0x7;
#ifdef TRACE_STEPS
      fprintf(stderr, " movel %%a%d,%%a%d@-\n", s_reg, d_reg);
#endif

      int fc = ec->data_fc();
      int32 value = (&ec->regs.a0)[s_reg];
      ec->mem->putl(fc, (&ec->regs.a0)[d_reg] - 4, value);
      (&ec->regs.a0)[d_reg] -= 4;
      ec->regs.sr.set_cc(value);

      ec->regs.pc += 2;
    }

  void movel_postinc_a(int op, execution_context *ec)
    {
      assert(ec != NULL);
      int s_reg = op & 0x7;
      int d_reg = op >> 9 & 0x7;
#ifdef TRACE_STEPS
      fprintf(stderr, " movel %%a%d@+,%%a%d\n", s_reg, d_reg);
#endif

      // XXX: The condition codes are not affected.
      int fc = ec->data_fc();
      (&ec->regs.a0)[d_reg] = ec->mem->getl(fc, (&ec->regs.a0)[s_reg]);
      (&ec->regs.a0)[s_reg] += 4;

      ec->regs.pc += 2;
    }

  /* movem regs to EA (postdec).  */
  void moveml_r_predec(int op, execution_context *ec)
    {
      int reg = op & 0x0007;
      unsigned int bitmap = ec->fetchw(2);
#ifdef TRACE_STEPS
      fprintf(stderr, " moveml #0x%x,%%a%d@-\n", bitmap, reg);
#endif

      // XXX: The condition codes are not affected.
      int fc = ec->data_fc();
      for (int i = 0; i != 16; ++i)
	{
	  if (bitmap & 1 != 0)
	    {
	      ec->mem->putl(fc, (&ec->regs.a0)[reg] - 4,
			    (&ec->regs.d0)[15 - i]);
	      (&ec->regs.a0)[reg] -= 4;
	    }
	  bitmap >>= 1;
	}

      ec->regs.pc += 4;
    }

  void moveql_d(int op, execution_context *ec)
    {
      assert(ec != NULL);
      int value = op & 0xff;
      int reg = op >> 9 & 0x7;
      if (value >= 0x80)
	value -= 0x100;
#ifdef TRACE_STEPS
      fprintf(stderr, " moveql #%d,%%d%d\n", value, reg);
#endif
      
      (&ec->regs.d0)[reg] = value;
      ec->regs.sr.set_cc(value);

      ec->regs.pc += 2;
    }

  void pea_absl(int op, execution_context *ec)
    {
      assert(ec != NULL);
      uint32 address = ec->fetchl(2);
#ifdef TRACE_STEPS
      fprintf(stderr, " pea 0x%lx:l\n", (unsigned long) address);
#endif

      // XXX: The condition codes are not affected.
      int fc = ec->data_fc();
      ec->mem->putl(fc, ec->regs.a7 - 4, address);

      ec->regs.pc += 6;
    }

  void rts(int op, execution_context *ec)
    {
      assert(ec != NULL);
#ifdef TRACE_STEPS
      fprintf(stderr, " rts\n");
#endif

      // XXX: The condition codes are not affected.
      int fc = ec->data_fc();
      uint32 value = ec->mem->getl(fc, ec->regs.a7);
      ec->regs.a7 += 4;
      ec->regs.pc = value;
    }

  void subql_a(int op, execution_context *ec)
    {
      assert(ec != NULL);
      int value = op >> 9 & 0x7;
      int reg = op & 0x7;
      if (value == 0)
	value = 8;
#ifdef TRACE_STEPS
      fprintf(stderr, " subql #%d,%%a%d\n", value, reg);
#endif

      // XXX: The condition codes are not affected.
      (&ec->regs.a0)[reg] -= value;

      ec->regs.pc += 2;
    }

  void tstw_d(int op, execution_context *ec)
    {
      assert(ec != NULL);
      int reg = op & 0x7;
#ifdef TRACE_STEPS
      fprintf(stderr, " tstw %%d%d\n", reg);
#endif

      int value = extsw((&ec->regs.d0)[reg]);
      ec->regs.sr.set_cc(value);

      ec->regs.pc += 2;
    }

  void unlk_a(int op, execution_context *ec)
    {
      int reg = op & 0x0007;
#ifdef TRACE_STEPS
      fprintf(stderr, " unlk %%a%d\n", reg);
#endif

      // XXX: The condition codes are not affected.
      int fc = ec->data_fc();
      uint32 address = ec->mem->getl(fc, (&ec->regs.a0)[reg]);
      ec->regs.a7 = (&ec->regs.a0)[reg] + 4;
      (&ec->regs.a0)[reg] = address;

      ec->regs.pc += 2;
    }
} // (unnamed namespace)

/* Installs instructions into the execution unit.  */
void
exec_unit::install_instructions(exec_unit *eu)
{
  assert(eu != NULL);

  eu->set_instruction(0x10d8, 0x0e07, &moveb_postinc_postinc);
  eu->set_instruction(0x2058, 0x0e07, &movel_postinc_a);
  eu->set_instruction(0x2108, 0x0e07, &movel_a_predec);
  eu->set_instruction(0x3039, 0x0e00, &movew_absl_d);
  eu->set_instruction(0x3100, 0x0e07, &movew_d_predec);
  eu->set_instruction(0x33c0, 0x0007, &movew_d_absl);
  eu->set_instruction(0x41e8, 0x0e07, &lea_offset_a);
  eu->set_instruction(0x41f9, 0x0e00, &lea_absl_a);
  eu->set_instruction(0x4260, 0x0007, &clrw_predec);
  eu->set_instruction(0x4879, 0x0000, &pea_absl);
  eu->set_instruction(0x48e0, 0x0007, &moveml_r_predec);
  eu->set_instruction(0x4a40, 0x0007, &tstw_d);
  eu->set_instruction(0x4e50, 0x0007, &link_a);
  eu->set_instruction(0x4e58, 0x0007, &unlk_a);
  eu->set_instruction(0x4e75, 0x0000, &rts);
  eu->set_instruction(0x5048, 0x0e07, &addqw_a);
  eu->set_instruction(0x5188, 0x0e07, &subql_a);
  eu->set_instruction(0x6000, 0x00ff, &bra);
  eu->set_instruction(0x6100, 0x00ff, &bsr);
  eu->set_instruction(0x6600, 0x00ff, &bne);
  eu->set_instruction(0x6700, 0x00ff, &beq);
  eu->set_instruction(0x6c00, 0x00ff, &bge);
  eu->set_instruction(0x7000, 0x0eff, &moveql_d);
}

