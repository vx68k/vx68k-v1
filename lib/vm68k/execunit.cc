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

#include "debug.h"

using namespace vm68k;
using namespace std;

/* Dispatches for instructions.  */
void
exec_unit::dispatch(unsigned int op, execution_context *ec) const
{
  I(op < 0x10000);
  instruction[op](op, ec);
}

/* Sets an instruction to operation codes.  */
void
exec_unit::set_instruction(int code, int mask, insn_handler h)
{
  I (code >= 0);
  I (code < 0x10000);
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
  void addw_off_d(int op, execution_context *ec)
    {
      I(ec != NULL);
      int s_reg = op & 0x7;
      int d_reg = op >> 9 & 0x7;
      int s_off = extsw(ec->fetchw(2));
      uint32 s_addr = ec->regs.a[s_reg] + s_off;
      VL((" addw %%a%d@(%d),%%d%d |0x%lx,*\n",
	  s_reg, s_off, d_reg, (unsigned long) s_addr));

      int fc = ec->data_fc();
      int value1 = extsw(ec->regs.d[d_reg]);
      int value2 = extsw(ec->mem->getw(fc, s_addr));
      int value = extsw(value1 + value2);
      const uint32 MASK = ((uint32) 1u << 16) - 1;
      ec->regs.d[d_reg] = ec->regs.d[d_reg] & ~MASK | (uint32) value & MASK;
      ec->regs.sr.set_cc(value); // FIXME.

      ec->regs.pc += 2 + 2;
    }

  void addil_d(int op, execution_context *ec)
    {
      I(ec != NULL);
      int d_reg = op & 0x7;
      int32 value2 = extsl(ec->fetchl(2));
      VL((" addil #%ld,%%d%d\n", (long) value2, d_reg));

      int fc = ec->data_fc();
      int32 value1 = extsl(ec->regs.d[d_reg]);
      int32 value = extsw(value1 + value2);
      ec->regs.d[d_reg] = value;
      ec->regs.sr.set_cc(value); // FIXME.

      ec->regs.pc += 2 + 4;
    }

  void addqw_a(int op, execution_context *ec)
    {
      I(ec != NULL);
      int value = op >> 9 & 0x7;
      int reg = op & 0x7;
      if (value == 0)
	value = 8;
      VL((" addqw #%d,%%a%d\n", value, reg));

      // XXX: The condition codes are not affected.
      ec->regs.a[reg] += value;

      ec->regs.pc += 2;
    }

  void beq(int op, execution_context *ec)
    {
      I(ec != NULL);
      int len = 2;
      int disp = op & 0xff;
      if (disp == 0)
	{
	  len = 4;
	  disp = extsw(ec->fetchw(2));
	}
      else
	disp = extsb(disp);
      VL((" beq 0x%lx\n", (unsigned long) (ec->regs.pc + 2 + disp)));

      // XXX: The condition codes are not affected.
      ec->regs.pc += ec->regs.sr.eq() ? 2 + disp : len;
    }

  void bge(int op, execution_context *ec)
    {
      I(ec != NULL);
      int len = 2;
      int disp = op & 0xff;
      if (disp == 0)
	{
	  len = 4;
	  disp = extsw(ec->fetchw(2));
	}
      else
	disp = extsb(disp);
      VL((" bge 0x%lx\n", (unsigned long) (ec->regs.pc + 2 + disp)));

      // XXX: The condition codes are not affected.
      ec->regs.pc += ec->regs.sr.ge() ? 2 + disp : len;
    }

  void bne(int op, execution_context *ec)
    {
      I(ec != NULL);
      int len = 2;
      int disp = op & 0xff;
      if (disp == 0)
	{
	  len = 4;
	  disp = extsw(ec->fetchw(2));
	}
      else
	disp = extsb(disp);
      VL((" bne 0x%lx\n", (unsigned long) (ec->regs.pc + 2 + disp)));

      // XXX: The condition codes are not affected.
      ec->regs.pc += ec->regs.sr.ne() ? 2 + disp : len;
    }

  void bra(int op, execution_context *ec)
    {
      I(ec != NULL);
      int len = 2;
      int disp = op & 0xff;
      if (disp == 0)
	{
	  len = 4;
	  disp = extsw(ec->fetchw(2));
	}
      else
	disp = extsb(disp);
      VL((" bra 0x%lx\n", (unsigned long) (ec->regs.pc + 2 + disp)));

      // XXX: The condition codes are not affected.
      ec->regs.pc += 2 + disp;
    }

  void bsr(int op, execution_context *ec)
    {
      I(ec != NULL);
      int len = 2;
      int disp = op & 0xff;
      if (disp == 0)
	{
	  len = 4;
	  disp = extsw(ec->fetchw(2));
	}
      else
	disp = extsb(disp);
      VL((" bsr 0x%lx\n", (unsigned long) (ec->regs.pc + 2 + disp)));

      // XXX: The condition codes are not affected.
      int fc = ec->data_fc();
      ec->mem->putl(fc, ec->regs.a[7] - 4, ec->regs.pc + len);
      ec->regs.a[7] -= 4;
      ec->regs.pc += 2 + disp;
    }

  void clrw_predec(int op, execution_context *ec)
    {
      I(ec != NULL);
      int reg = op & 0x7;
      uint32 d_addr = ec->regs.a[reg] - 2;
      VL((" clrw %%a%d@- |0x%lx\n", reg, (unsigned long) d_addr));

      int fc = ec->data_fc();
      ec->mem->putw(fc, d_addr, 0);
      ec->regs.a[reg] = d_addr;
      ec->regs.sr.set_cc(0);

      ec->regs.pc += 2;
    }

  void lea_offset_a(int op, execution_context *ec)
    {
      I(ec != NULL);
      int s_reg = op & 0x7;
      int d_reg = op >> 9 & 0x7;
      int offset = extsw(ec->fetchw(2));
      VL((" lea %%a%d@(%ld),%%a%d\n", s_reg, (long) offset, d_reg));

      // XXX: The condition codes are not affected.
      ec->regs.a[d_reg] = ec->regs.a[s_reg] + offset;

      ec->regs.pc += 4;
    }

  void lea_absl_a(int op, execution_context *ec)
    {
      I(ec != NULL);
      int reg = op >> 9 & 0x7;
      uint32 address = ec->fetchl(2);
      VL((" lea 0x%lx:l,%%a%d\n", (unsigned long) address, reg));

      // XXX: The condition codes are not affected.
      ec->regs.a[reg] = address;

      ec->regs.pc += 6;
    }

  void link_a(int op, execution_context *ec)
    {
      int reg = op & 0x0007;
      int disp = extsw(ec->fetchw(2));
      VL((" link %%a%d,#%d\n", reg, disp));

      // XXX: The condition codes are not affected.
      int fc = ec->data_fc();
      ec->mem->putl(fc, ec->regs.a[7] - 4, ec->regs.a[reg]);
      ec->regs.a[7] -= 4;
      ec->regs.a[reg] = ec->regs.a[7];
      ec->regs.a[7] += disp;

      ec->regs.pc += 4;
    }

  void moveb_postinc_postinc(int op, execution_context *ec)
    {
      I(ec != NULL);
      int s_reg = op & 0x7;
      uint32 s_addr = ec->regs.a[s_reg];
      int d_reg = op >> 9 & 0x7;
      uint32 d_addr = ec->regs.a[d_reg];
      VL((" moveb %%a%d@+,%%a%d@+ |0x%lx,0x%lx\n",
	  s_reg, d_reg, (unsigned long) s_addr, (unsigned long) d_addr));

      int fc = ec->data_fc();
      int value = extsb(ec->mem->getb(fc, s_addr));
      ec->mem->putb(fc, d_addr, value);
      ec->regs.a[s_reg] = s_addr + 1;
      ec->regs.a[d_reg] = d_addr + 1;
      ec->regs.sr.set_cc(value);

      ec->regs.pc += 2;
    }

  void movew_off_d(int op, execution_context *ec)
    {
      I(ec != NULL);
      int s_reg = op & 0x7;
      int d_reg = op >> 9 & 0x7;
      int s_off = extsw(ec->fetchw(2));
      uint32 s_addr = ec->regs.a[s_reg] + s_off;
      VL((" movew %%a%d@(%d),%%d%d |0x%lx,*\n",
	  s_reg, s_off, d_reg, (unsigned long) s_addr));

      int fc = ec->data_fc();
      int value = extsw(ec->mem->getw(fc, s_addr));
      const uint32 MASK = ((uint32) 1u << 16) - 1;
      ec->regs.d[d_reg] = ec->regs.d[d_reg] & ~MASK | (uint32) value & MASK;
      ec->regs.sr.set_cc(value);

      ec->regs.pc += 2 + 2;
    }

  void movew_absl_d(int op, execution_context *ec)
    {
      I(ec != NULL);
      int d_reg = op >> 9 & 0x7;
      uint32 address = ec->fetchl(2);
      VL((" movew 0x%lx,%%d%d\n", (unsigned long) address, d_reg));

      int fc = ec->data_fc();
      int value = extsw(ec->mem->getw(fc, address));
      const uint32 MASK = ((uint32) 1u << 16) - 1;
      ec->regs.d[d_reg] = ec->regs.d[d_reg] & ~MASK | (uint32) value & MASK;
      ec->regs.sr.set_cc(value);

      ec->regs.pc += 2 + 4;
    }

  void movew_d_predec(int op, execution_context *ec)
    {
      I(ec != NULL);
      int s_reg = op & 0x7;
      int d_reg = op >> 9 & 0x7;
      uint32 d_addr = ec->regs.d[d_reg] - 2;
      VL((" movew %%d%d,%%a%x@- |*,0x%lx\n",
	  s_reg, d_reg, (unsigned long) d_addr));

      int fc = ec->data_fc();
      int value = extsw(ec->regs.d[s_reg]);
      ec->mem->putw(fc, d_addr, value);
      ec->regs.a[d_reg] = d_addr;
      ec->regs.sr.set_cc(value);

      ec->regs.pc += 2;
    }

  void movew_absl_predec(int op, execution_context *ec)
    {
      I(ec != NULL);
      int d_reg = op >> 9 & 0x7;
      uint32 d_addr = ec->regs.a[d_reg] - 2;
      uint32 s_addr = ec->fetchl(2);
      VL((" movew 0x%lx,%%a%x@- |*,0x%lx\n",
	  (unsigned long) s_addr, d_reg, (unsigned long) d_addr));

      int fc = ec->data_fc();
      int value = extsw(ec->mem->getw(fc, s_addr));
      ec->mem->putw(fc, d_addr, value);
      ec->regs.a[d_reg] = d_addr;
      ec->regs.sr.set_cc(value);

      ec->regs.pc += 2 + 4;
    }

  void movew_d_absl(int op, execution_context *ec)
    {
      I(ec != NULL);
      int reg = op & 0x7;
      uint32 address = ec->fetchl(2);
      VL((" movew %%d%d,0x%x\n", reg, address));

      int fc = ec->data_fc();
      int value = extsw(ec->regs.d[reg]);
      ec->mem->putw(fc, address, value);
      ec->regs.sr.set_cc(value);

      ec->regs.pc += 2 + 4;
    }

  void movel_off_d(int op, execution_context *ec)
    {
      I(ec != NULL);
      int s_reg = op & 0x7;
      int d_reg = op >> 9 & 0x7;
      int s_off = extsw(ec->fetchw(2));
      uint32 s_addr = ec->regs.a[s_reg] + s_off;
      VL((" movel %%a%d@(%d),%%d%d |0x%lx,*\n",
	  s_reg, s_off, d_reg, (unsigned long) s_addr));

      int fc = ec->data_fc();
      int32 value = extsl(ec->mem->getl(fc, s_addr));
      ec->regs.d[d_reg] = value;
      ec->regs.sr.set_cc(value);

      ec->regs.pc += 2 + 2;
    }

  void movel_d_predec(int op, execution_context *ec)
    {
      I(ec != NULL);
      int s_reg = op & 0x7;
      int d_reg = op >> 9 & 0x7;
      uint32 d_addr = ec->regs.a[d_reg] - 4;
      VL((" movel %%d%d,%%a%d@- |*,0x%lx\n",
	  s_reg, d_reg, (unsigned long) d_addr));

      int fc = ec->data_fc();
      int32 value = extsl(ec->regs.d[s_reg]);
      ec->mem->putl(fc, d_addr, value);
      ec->regs.a[d_reg] = d_addr;
      ec->regs.sr.set_cc(value);

      ec->regs.pc += 2;
    }

  void movel_a_predec(int op, execution_context *ec)
    {
      I(ec != NULL);
      int s_reg = op & 0x7;
      int d_reg = op >> 9 & 0x7;
      uint32 d_addr = ec->regs.a[d_reg] - 4;
      VL((" movel %%a%d,%%a%d@- |*,0x%lx\n",
	  s_reg, d_reg, (unsigned long) d_addr));

      int fc = ec->data_fc();
      int32 value = extsl(ec->regs.a[s_reg]);
      ec->mem->putl(fc, d_addr, value);
      ec->regs.a[d_reg] = d_addr;
      ec->regs.sr.set_cc(value);

      ec->regs.pc += 2;
    }

  void movel_postinc_a(int op, execution_context *ec)
    {
      I(ec != NULL);
      int s_reg = op & 0x7;
      uint32 s_addr = ec->regs.a[s_reg];
      int d_reg = op >> 9 & 0x7;
      VL((" movel %%a%d@+,%%a%d |0x%lx,*\n",
	  s_reg, d_reg, (unsigned long) s_addr));

      // XXX: The condition codes are not affected.
      int fc = ec->data_fc();
      ec->regs.a[d_reg] = ec->mem->getl(fc, s_addr);
      ec->regs.a[s_reg] = s_addr + 4;

      ec->regs.pc += 2;
    }

  void movel_i_predec(int op, execution_context *ec)
    {
      I(ec != NULL);
      int d_reg = op >> 9 & 0x7;
      uint32 d_addr = ec->regs.a[d_reg] - 4;
      int32 value = extsl(ec->fetchl(2));
      VL((" movel #%ld,%%a%d@- |*,0x%lx\n",
	  (long) value, d_reg, (unsigned long) d_addr));

      int fc = ec->data_fc();
      ec->mem->putl(fc, d_addr, value);
      ec->regs.a[d_reg] = d_addr;
      ec->regs.sr.set_cc(value);

      ec->regs.pc += 2 + 4;
    }

  void movel_d_absl(int op, execution_context *ec)
    {
      I(ec != NULL);
      int s_reg = op & 0x7;
      uint32 d_addr = ec->fetchl(2);
      VL((" movel %%d%d,0x%x\n", s_reg, d_addr));

      int fc = ec->data_fc();
      int32 value = extsl(ec->regs.d[s_reg]);
      ec->mem->putw(fc, d_addr, value);
      ec->regs.sr.set_cc(value);

      ec->regs.pc += 2 + 4;
    }

  void movel_a_absl(int op, execution_context *ec)
    {
      I(ec != NULL);
      int s_reg = op & 0x7;
      uint32 d_addr = ec->fetchl(2);
      VL((" movel %%a%d,0x%x\n", s_reg, d_addr));

      int fc = ec->data_fc();
      int32 value = extsl(ec->regs.a[s_reg]);
      ec->mem->putw(fc, d_addr, value);
      ec->regs.sr.set_cc(value);

      ec->regs.pc += 2 + 4;
    }

  /* movem regs to EA (postdec).  */
  void moveml_r_predec(int op, execution_context *ec)
    {
      int reg = op & 0x0007;
      unsigned int bitmap = ec->fetchw(2);
      VL((" moveml #0x%x,%%a%d@-\n", bitmap, reg));

      // XXX: The condition codes are not affected.
      uint32 address = ec->regs.a[reg];
      int fc = ec->data_fc();
      for (uint32 *i = ec->regs.a + 8; i != ec->regs.a + 0; --i)
	{
	  if (bitmap & 1 != 0)
	    {
	      ec->mem->putl(fc, address - 4, *(i - 1));
	      address -= 4;
	    }
	  bitmap >>= 1;
	}
      for (uint32 *i = ec->regs.d + 8; i != ec->regs.d + 0; --i)
	{
	  if (bitmap & 1 != 0)
	    {
	      ec->mem->putl(fc, address - 4, *(i - 1));
	      address -= 4;
	    }
	  bitmap >>= 1;
	}
      ec->regs.a[reg] = address;

      ec->regs.pc += 2 + 2;
    }

  void moveml_postinc_r(int op, execution_context *ec)
    {
      int reg = op & 0x0007;
      unsigned int bitmap = ec->fetchw(2);
      VL((" moveml %%a%d@+,#0x%x\n", reg, bitmap));

      // XXX: The condition codes are not affected.
      uint32 address = ec->regs.a[reg];
      int fc = ec->data_fc();
      for (uint32 *i = ec->regs.d + 0; i != ec->regs.d + 8; ++i)
	{
	  if (bitmap & 1 != 0)
	    {
	      *i = ec->mem->getl(fc, address);
	      address += 4;
	    }
	  bitmap >>= 1;
	}
      for (uint32 *i = ec->regs.a + 0; i != ec->regs.a + 8; ++i)
	{
	  if (bitmap & 1 != 0)
	    {
	      *i = ec->mem->getl(fc, address);
	      address += 4;
	    }
	  bitmap >>= 1;
	}
      ec->regs.a[reg] = address;

      ec->regs.pc += 2 + 2;
    }

  void moveql_d(int op, execution_context *ec)
    {
      I(ec != NULL);
      int value = extsb(op & 0xff);
      int reg = op >> 9 & 0x7;
      VL((" moveql #%d,%%d%d\n", value, reg));
      
      ec->regs.d[reg] = value;
      ec->regs.sr.set_cc(value);

      ec->regs.pc += 2;
    }

  void pea_absl(int op, execution_context *ec)
    {
      I(ec != NULL);
      uint32 address = ec->fetchl(2);
      VL((" pea 0x%lx:l\n", (unsigned long) address));

      // XXX: The condition codes are not affected.
      int fc = ec->data_fc();
      ec->mem->putl(fc, ec->regs.a[7] - 4, address);
      ec->regs.a[7] -= 4;

      ec->regs.pc += 2 + 4;
    }

  void rts(int op, execution_context *ec)
    {
      I(ec != NULL);
      VL((" rts\n"));

      // XXX: The condition codes are not affected.
      int fc = ec->data_fc();
      uint32 value = ec->mem->getl(fc, ec->regs.a[7]);
      ec->regs.a[7] += 4;
      ec->regs.pc = value;
    }

  void subql_a(int op, execution_context *ec)
    {
      I(ec != NULL);
      int value = op >> 9 & 0x7;
      int reg = op & 0x7;
      if (value == 0)
	value = 8;
      VL((" subql #%d,%%a%d\n", value, reg));

      // XXX: The condition codes are not affected.
      ec->regs.a[reg] -= value;

      ec->regs.pc += 2;
    }

  void tstw_d(int op, execution_context *ec)
    {
      I(ec != NULL);
      int reg = op & 0x7;
      VL((" tstw %%d%d\n", reg));

      int value = extsw(ec->regs.d[reg]);
      ec->regs.sr.set_cc(value);

      ec->regs.pc += 2;
    }

  void unlk_a(int op, execution_context *ec)
    {
      int reg = op & 0x0007;
      VL((" unlk %%a%d\n", reg));

      // XXX: The condition codes are not affected.
      int fc = ec->data_fc();
      uint32 address = ec->mem->getl(fc, ec->regs.a[reg]);
      ec->regs.a[7] = ec->regs.a[reg] + 4;
      ec->regs.a[reg] = address;

      ec->regs.pc += 2;
    }
} // (unnamed namespace)

/* Installs instructions into the execution unit.  */
void
exec_unit::install_instructions(exec_unit *eu)
{
  I(eu != NULL);

  eu->set_instruction(0x0680, 0x0007, &addil_d);
  eu->set_instruction(0x10d8, 0x0e07, &moveb_postinc_postinc);
  eu->set_instruction(0x2028, 0x0e07, &movel_off_d);
  eu->set_instruction(0x2058, 0x0e07, &movel_postinc_a);
  eu->set_instruction(0x2100, 0x0e07, &movel_d_predec);
  eu->set_instruction(0x2108, 0x0e07, &movel_a_predec);
  eu->set_instruction(0x213c, 0x0e00, &movel_i_predec);
  eu->set_instruction(0x23c0, 0x0007, &movel_d_absl);
  eu->set_instruction(0x23c8, 0x0007, &movel_a_absl);
  eu->set_instruction(0x3028, 0x0e07, &movew_off_d);
  eu->set_instruction(0x3039, 0x0e00, &movew_absl_d);
  eu->set_instruction(0x3100, 0x0e07, &movew_d_predec);
  eu->set_instruction(0x3139, 0x0e00, &movew_absl_predec);
  eu->set_instruction(0x33c0, 0x0007, &movew_d_absl);
  eu->set_instruction(0x41e8, 0x0e07, &lea_offset_a);
  eu->set_instruction(0x41f9, 0x0e00, &lea_absl_a);
  eu->set_instruction(0x4260, 0x0007, &clrw_predec);
  eu->set_instruction(0x4879, 0x0000, &pea_absl);
  eu->set_instruction(0x48e0, 0x0007, &moveml_r_predec);
  eu->set_instruction(0x4cd8, 0x0007, &moveml_postinc_r);
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
  eu->set_instruction(0xd068, 0x0e07, &addw_off_d);
}

