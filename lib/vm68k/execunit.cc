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

#include <vm68k/addressing.h>
#include <vm68k/condition.h>
#include <vm68k/cpu.h>

#include <algorithm>

#ifdef HAVE_NANA_H
# include <nana.h>
# include <cstdio>
#else
# include <cassert>
# define I assert
# define VL(EXPR)
#endif

using namespace vm68k;
using namespace std;

/* Dispatches for instructions.  */
void
exec_unit::dispatch(unsigned int op, context &ec) const
{
  I(op < 0x10000);
  instruction[op](op, ec);
}

/* Sets an instruction to operation codes.  */
void
exec_unit::set_instruction(int code, int mask, instruction_handler h)
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
  install_instructions(*this);
}

/* Executes an illegal instruction.  */
void
exec_unit::illegal(unsigned int op, context &)
{
  throw illegal_instruction();
}

namespace
{
  using namespace condition;
  using namespace addressing;

  void addw_off_d(unsigned int op, context &ec)
    {
      int s_reg = op & 0x7;
      int d_reg = op >> 9 & 0x7;
      int s_off = extsw(ec.fetchw(2));
      uint32 s_addr = ec.regs.a[s_reg] + s_off;
      VL((" addw %%a%d@(%d),%%d%d |0x%lx,*\n",
	  s_reg, s_off, d_reg, (unsigned long) s_addr));

      int fc = ec.data_fc();
      int value1 = extsw(ec.regs.d[d_reg]);
      int value2 = extsw(ec.mem->getw(fc, s_addr));
      int value = extsw(value1 + value2);
      const uint32 MASK = ((uint32) 1u << 16) - 1;
      ec.regs.d[d_reg] = ec.regs.d[d_reg] & ~MASK | (uint32) value & MASK;
      ec.regs.sr.set_cc(value); // FIXME.

      ec.regs.pc += 2 + 2;
    }

  template <class Source>
    void addl(unsigned int op, context &ec)
    {
      Source ea1(op & 0x7, 2);
      int reg2 = op >> 9 & 0x7;
      VL((" addl %s", ea1.textl(ec)));
      VL((",%%d%d", reg2));
      VL(("\t| 0x%04x, %%pc = 0x%lx\n", op, (unsigned long) ec.regs.pc));

      int32 value1 = ea1.getl(ec);
      int32 value2 = extsl(ec.regs.d[reg2]);
      int32 value = extsl(value2 + value1);
      ec.regs.d[reg2] = value;
      ea1.finishl(ec);
      ec.regs.sr.set_cc(value); // FIXME.

      ec.regs.pc += 2 + ea1.isize(4);
    }

  template <class Destination> void addal(unsigned int op, context &ec)
    {
      Destination ea1(op & 0x7, 2);
      int reg2 = op >> 9 & 0x7;
      VL((" addal %s", ea1.textl(ec)));
      VL((",%%a%d", reg2));
      VL((" | %%pc = 0x%lx\n", (unsigned long) ec.regs.pc));

      int32 value1 = ea1.getl(ec);
      int32 value2 = extsl(ec.regs.a[reg2]);
      int32 value = extsl(value2 + value1);
      ec.regs.a[reg2] = value;
      ea1.finishl(ec);
      // XXX: The condition codes are not affected.

      ec.regs.pc += 2 + ea1.isize(4);
    }

  template <class Destination> void addil(unsigned int op, context &ec)
    {
      int32 value2 = extsl(ec.fetchl(2));
      Destination ea1(op & 0x7, 2 + 4);
      VL((" addil #%ld,*\n", (long) value2));

      int32 value1 = ea1.getl(ec);
      int32 value = extsl(value1 + value2);
      ea1.putl(ec, value);
      ea1.finishl(ec);
      ec.regs.sr.set_cc(value); // FIXME.

      ec.regs.pc += 2 + 4 + ea1.isize(4);
    }

  template <> void addil<address_register>(unsigned int, context &);
  // XXX: Address register cannot be the destination.

#if 0
  void addil_d(unsigned int op, context &ec)
    {
      int d_reg = op & 0x7;
      int32 value2 = extsl(ec.fetchl(2));
      VL((" addil #%ld,%%d%d\n", (long) value2, d_reg));

      int32 value1 = extsl(ec.regs.d[d_reg]);
      int32 value = extsl(value1 + value2);
      ec.regs.d[d_reg] = value;
      ec.regs.sr.set_cc(value); // FIXME.

      ec.regs.pc += 2 + 4;
    }
#endif /* 0 */

  template <class Destination> void addqb(unsigned int op, context &ec)
    {
      Destination ea1(op & 0x7, 2);
      int value2 = op >> 9 & 0x7;
      if (value2 == 0)
	value2 = 8;
      VL((" addqb #%d,*\n", value2));

      int value1 = ea1.getb(ec);
      int value = extsb(value1 + value2);
      ea1.putb(ec, value);
      ea1.finishb(ec);
      ec.regs.sr.set_cc(value); // FIXME.

      ec.regs.pc += 2 + ea1.isize(2);
    }

#if 0
  void addqb_d(unsigned int op, context &ec)
    {
      int reg1 = op & 0x7;
      int val2 = op >> 9 & 0x7;
      if (val2 == 0)
	val2 = 8;
      VL((" addqb #%d,%%d%d\n", val2, reg1));

      int val1 = extsb(ec.regs.d[reg1]);
      int val = extsb(val1 + val2);
      const uint32 MASK = ((uint32) 1u << 8) - 1;
      ec.regs.d[reg1] = ec.regs.d[reg1] & ~MASK | (uint32) val & MASK;
      ec.regs.sr.set_cc(val); // FIXME.

      ec.regs.pc += 2;
    }
#endif /* 0 */

  template <class Destination> void addqw(unsigned int op, context &ec)
    {
      Destination ea1(op & 0x7, 2);
      int value2 = op >> 9 & 0x7;
      if (value2 == 0)
	value2 = 8;
      VL((" addqw #%d,*\n", value2));

      int value1 = ea1.getw(ec);
      int value = extsw(value1 + value2);
      ea1.putw(ec, value);
      ea1.finishw(ec);
      ec.regs.sr.set_cc(value); // FIXME.

      ec.regs.pc += 2 + ea1.isize(2);
    }

  template <> void addqw<address_register>(unsigned int op, context &ec)
    {
      address_register ea1(op & 0x7, 2);
      int value2 = op >> 9 & 0x7;
      if (value2 == 0)
	value2 = 8;
      VL((" addqw #%d,*\n", value2));

      // XXX: The entire register is used.
      int32 value1 = ea1.getl(ec);
      int32 value = extsl(value1 + value2);
      ea1.putl(ec, value);
      ea1.finishl(ec);
      // XXX: The condition codes are not affected.

      ec.regs.pc += 2 + ea1.isize(2);
    }

#if 0
  void addqw_a(unsigned int op, context &ec)
    {
      int value = op >> 9 & 0x7;
      int reg = op & 0x7;
      if (value == 0)
	value = 8;
      VL((" addqw #%d,%%a%d\n", value, reg));

      // XXX: The condition codes are not affected.
      ec.regs.a[reg] += value;

      ec.regs.pc += 2;
    }
#endif /* 0 */

  template <class Destination> void addql(unsigned int op, context &ec)
    {
      Destination ea1(op & 0x7, 2);
      int value2 = op >> 9 & 0x7;
      if (value2 == 0)
	value2 = 8;
      VL((" addql #%d,*\n", value2));

      int32 value1 = ea1.getl(ec);
      int32 value = extsl(value1 + value2);
      ea1.putl(ec, value);
      ea1.finishl(ec);
      ec.regs.sr.set_cc(value); // FIXME.

      ec.regs.pc += 2 + ea1.isize(4);
    }

  template <> void addql<address_register>(unsigned int op, context &ec)
    {
      address_register ea1(op & 0x7, 2);
      int value2 = op >> 9 & 0x7;
      if (value2 == 0)
	value2 = 8;
      VL((" addql #%d,*\n", value2));

      int32 value1 = ea1.getl(ec);
      int32 value = extsl(value1 + value2);
      ea1.putl(ec, value);
      ea1.finishl(ec);
      // XXX: The condition codes are not affected.

      ec.regs.pc += 2 + ea1.isize(4);
    }

#if 0
  void addql_d(unsigned int op, context &ec)
    {
      int reg1 = op & 0x7;
      int val2 = op >> 9 & 0x7;
      if (val2 == 0)
	val2 = 8;
      VL((" addql #%d,%%d%d\n", val2, reg1));

      int32 val1 = extsl(ec.regs.d[reg1]);
      int32 val = extsl(val1 + val2);
      ec.regs.d[reg1] = val;
      ec.regs.sr.set_cc(val); // FIXME.

      ec.regs.pc += 2;
    }
#endif /* 0 */

  void andl_i_d(unsigned int op, context &ec)
    {
      int reg1 = op >> 9 & 0x7;
      uint32 val2 = ec.fetchl(2);
      VL((" andl #0x%lx,%%d%d\n", (unsigned long) val2, reg1));

      uint32 val = ec.regs.d[reg1] & val2;
      ec.regs.d[reg1] = val;
      ec.regs.sr.set_cc(val);

      ec.regs.pc += 2 + 4;
    }

  template <class Condition> void 
  b(unsigned int op, context &ec)
  {
    Condition cond;
    sint_type disp = op & 0xff;
    size_t len;
    if (disp == 0)
      {
	disp = extsw(ec.fetchw(2));
	len = 2;
      }
    else
      {
	disp = extsb(disp);
	len = 0;
      }
#ifdef L
    L(" b%s 0x%lx", cond.text(), (unsigned long) (ec.regs.pc + 2 + disp));
    L("\t| 0x%04x, %%pc = 0x%lx\n", op, (unsigned long) ec.regs.pc);
#endif

    // XXX: The condition codes are not affected by this instruction.
    ec.regs.pc += 2 + (cond(ec) ? disp : len);
  }

#if 0
  void bcc(unsigned int op, context &ec)
    {
      int len = 2;
      int disp = op & 0xff;
      if (disp == 0)
	{
	  len = 4;
	  disp = extsw(ec.fetchw(2));
	}
      else
	disp = extsb(disp);
      VL((" bcc 0x%lx\n", (unsigned long) (ec.regs.pc + 2 + disp)));

      // XXX: The condition codes are not affected.
      ec.regs.pc += ec.regs.sr.cc() ? 2 + disp : len;
    }
#endif

  void beq(unsigned int op, context &ec)
    {
      int len = 2;
      int disp = op & 0xff;
      if (disp == 0)
	{
	  len = 4;
	  disp = extsw(ec.fetchw(2));
	}
      else
	disp = extsb(disp);
      VL((" beq 0x%lx\n", (unsigned long) (ec.regs.pc + 2 + disp)));

      // XXX: The condition codes are not affected.
      ec.regs.pc += ec.regs.sr.eq() ? 2 + disp : len;
    }

  void bge(unsigned int op, context &ec)
    {
      int len = 2;
      int disp = op & 0xff;
      if (disp == 0)
	{
	  len = 4;
	  disp = extsw(ec.fetchw(2));
	}
      else
	disp = extsb(disp);
      VL((" bge 0x%lx\n", (unsigned long) (ec.regs.pc + 2 + disp)));

      // XXX: The condition codes are not affected.
      ec.regs.pc += ec.regs.sr.ge() ? 2 + disp : len;
    }

  void bmi(unsigned int op,
	   context &ec)
    {
      int len = 2;
      int disp = op & 0xff;
      if (disp == 0)
	{
	  len = 4;
	  disp = extsw(ec.fetchw(2));
	}
      else
	disp = extsb(disp);
#ifdef L
      L(" bmi 0x%lx", (unsigned long) (ec.regs.pc + 2 + disp));
      L("\t| 0x%04x, %%pc = 0x%lx\n", op, (unsigned long) ec.regs.pc);
#endif

      // XXX: The condition codes are not affected.
      ec.regs.pc += ec.regs.sr.mi() ? 2 + disp : len;
    }

  void bne(unsigned int op, context &ec)
    {
      int len = 2;
      int disp = op & 0xff;
      if (disp == 0)
	{
	  len = 4;
	  disp = extsw(ec.fetchw(2));
	}
      else
	disp = extsb(disp);
      VL((" bne 0x%lx\n", (unsigned long) (ec.regs.pc + 2 + disp)));

      // XXX: The condition codes are not affected.
      ec.regs.pc += ec.regs.sr.ne() ? 2 + disp : len;
    }

  void bra(unsigned int op, context &ec)
    {
      int len = 2;
      int disp = op & 0xff;
      if (disp == 0)
	{
	  len = 4;
	  disp = extsw(ec.fetchw(2));
	}
      else
	disp = extsb(disp);
      VL((" bra 0x%lx\n", (unsigned long) (ec.regs.pc + 2 + disp)));

      // XXX: The condition codes are not affected.
      ec.regs.pc += 2 + disp;
    }

  void bsr(unsigned int op, context &ec)
    {
      int len = 2;
      int disp = op & 0xff;
      if (disp == 0)
	{
	  len = 4;
	  disp = extsw(ec.fetchw(2));
	}
      else
	disp = extsb(disp);
      VL((" bsr 0x%lx\n", (unsigned long) (ec.regs.pc + 2 + disp)));

      // XXX: The condition codes are not affected.
      int fc = ec.data_fc();
      ec.mem->putl(fc, ec.regs.a[7] - 4, ec.regs.pc + len);
      ec.regs.a[7] -= 4;
      ec.regs.pc += 2 + disp;
    }

  template <class Destination> void clrb(unsigned int op, context &ec)
    {
      Destination ea1(op & 0x7, 2);
      VL((" clrb %s", ea1.textb(ec)));
      VL((" | %%pc = 0x%lx\n", (unsigned long) ec.regs.pc));

      ea1.putb(ec, 0);
      ea1.finishb(ec);
      ec.regs.sr.set_cc(0);

      ec.regs.pc += 2 + ea1.isize(2);
    }

  template <class Destination> void clrw(unsigned int op, context &ec)
    {
      Destination ea1(op & 0x7, 2);
      VL((" clrw %s", ea1.textw(ec)));
      VL((" | %%pc = 0x%lx\n", (unsigned long) ec.regs.pc));

      ea1.putw(ec, 0);
      ea1.finishw(ec);
      ec.regs.sr.set_cc(0);

      ec.regs.pc += 2 + ea1.isize(2);
    }

  template <> void clrw<address_register>(unsigned int, context &);
  // XXX: Address register cannot be the destination.

  template <class Destination>
    void clrl(unsigned int op, context &ec)
    {
      Destination ea1(op & 0x7, 2);
      VL((" clrl %s", ea1.textw(ec)));
      VL(("\t| 0x%04x, %%pc = 0x%lx\n", op, (unsigned long) ec.regs.pc));

      ea1.putl(ec, 0);
      ea1.finishl(ec);
      ec.regs.sr.set_cc(0);

      ec.regs.pc += 2 + ea1.isize(2);
    }

  template <class Source> void
  cmpb(unsigned int op, context &ec)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef L
    L(" cmpb %s", ea1.textb(ec));
    L(",%%d%d", reg2);
    L("\t| 0x%04x, %%pc = 0x%lx\n", op, (unsigned long) ec.regs.pc);
#endif

    sint_type value1 = ea1.getb(ec);
    sint_type value2 = extsb(ec.regs.d[reg2]);
    sint_type value = extsb(value2 - value1);
    ec.regs.sr.set_cc_cmp(value, value2, value1);
    ea1.finishb(ec);

    ec.regs.pc += 2 + ea1.isize(2);
  }

  template <class Source> void
  cmpw(unsigned int op, context &ec)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef L
    L(" cmpw %s", ea1.textw(ec));
    L(",%%d%d", reg2);
    L("\t| 0x%04x, %%pc = 0x%lx\n", op, (unsigned long) ec.regs.pc);
#endif

    sint_type value1 = ea1.getw(ec);
    sint_type value2 = extsw(ec.regs.d[reg2]);
    sint_type value = extsw(value2 - value1);
    ec.regs.sr.set_cc_cmp(value, value2, value1);
    ea1.finishw(ec);

    ec.regs.pc += 2 + ea1.isize(2);
  }

  template <class Source> void
  cmpl(unsigned int op, context &ec)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef L
    L(" cmpl %s", ea1.textl(ec));
    L(",%%d%d", reg2);
    L("\t| 0x%04x, %%pc = 0x%lx\n", op, (unsigned long) ec.regs.pc);
#endif

    sint32_type value1 = ea1.getl(ec);
    sint32_type value2 = extsl(ec.regs.d[reg2]);
    sint32_type value = extsl(value2 - value1);
    ec.regs.sr.set_cc_cmp(value, value2, value1);
    ea1.finishl(ec);

    ec.regs.pc += 2 + ea1.isize(4);
  }

  template <class Destination> void
  cmpib(unsigned int op, context &ec)
  {
    sint_type value2 = extsb(ec.fetchw(2));
    Destination ea1(op & 0x7, 2 + 2);
#ifdef L
    L(" cmpib #0x%x", uint_type(value2));
    L(",%s", ea1.textb(ec));
    L("\t| 0x%04x, %%pc = 0x%lx\n", op, (unsigned long) ec.regs.pc);
#endif

    sint_type value1 = ea1.getb(ec);
    sint_type value = extsb(value1 - value2);
    ec.regs.sr.set_cc_cmp(value, value1, value2);
    ea1.finishb(ec);

    ec.regs.pc += 2 + 2 + ea1.isize(2);
  }

  template <class Destination> void
  eoriw(unsigned int op, context &ec)
  {
    sint_type value2 = ec.fetchw(2);
    Destination ea1(op & 0x7, 2 + 2);
#ifdef L
    L(" eoriw #0x%lx", long(value2));
    L(",%s", ea1.textw(ec));
    L("\t| 0x%04x, %%pc = 0x%lx\n", op, (unsigned long) ec.regs.pc);
#endif

    sint_type value1 = ea1.getw(ec);
    sint_type value = extsw(uint_type(value1) ^ uint_type(value2));
    ea1.putw(ec, value);
    ec.regs.sr.set_cc(value);
    ea1.finishw(ec);

    ec.regs.pc += 2 + 2 + ea1.isize(2);
  }

  void dbf_d(unsigned int op, context &ec)
    {
      int reg = op & 0x7;
      int disp = extsw(ec.fetchw(2));
      VL((" dbf %%d%d,0x%lx\n",
	  reg, (unsigned long) (ec.regs.pc + 2 + disp)));

      // XXX: The condition codes are not affected.
      int32 value = extsl(ec.regs.d[reg]) - 1;
      ec.regs.d[reg] = value;
      ec.regs.pc += value != -1 ? 2 + disp : 2 + 2;
    }

  template <class Destination> void jsr(unsigned int op,
					context &ec)
    {
      Destination ea1(op & 0x7, 2);
#ifdef L
      L(" jsr %s", ea1.textw(ec));
      L("\t| 0x%04x, %%pc = 0x%lx\n", op, (unsigned long) ec.regs.pc);
#endif

      // XXX: The condition codes are not affected.
      uint32 address = ea1.address(ec);
      int fc = ec.data_fc();
      ec.mem->putl(fc, ec.regs.a[7] - 4, ec.regs.pc + 2 + ea1.isize(0));
      ec.regs.a[7] -= 4;
      ec.regs.pc = address;
    }

  template <class Destination> void lea(unsigned int op,
					context &ec)
    {
      Destination ea1(op & 0x7, 2);
      int reg2 = op >> 9 & 0x7;
#ifdef L
      L(" lea %s", ea1.textw(ec));
      L(",%%a%d", reg2);
      L("\t| 0x%04x, %%pc = 0x%lx\n", op, (unsigned long) ec.regs.pc);
#endif

      // XXX: The condition codes are not affected.
      uint32 address = ea1.address(ec);
      ec.regs.a[reg2] = address;

      ec.regs.pc += 2 + ea1.isize(0);
    }

  void link_a(unsigned int op, context &ec)
    {
      int reg = op & 0x0007;
      int disp = extsw(ec.fetchw(2));
      VL((" link %%a%d,#%d\n", reg, disp));

      // XXX: The condition codes are not affected.
      int fc = ec.data_fc();
      ec.mem->putl(fc, ec.regs.a[7] - 4, ec.regs.a[reg]);
      ec.regs.a[7] -= 4;
      ec.regs.a[reg] = ec.regs.a[7];
      ec.regs.a[7] += disp;

      ec.regs.pc += 4;
    }

  void lslw_i_d(unsigned int op, context &ec)
    {
      int reg1 = op & 0x7;
      int val2 = op >> 9 & 0x7;
      if (val2 == 0)
	val2 = 8;
      VL((" lslw #%d,%%d%d\n", val2, reg1));

      unsigned int val = ec.regs.d[reg1] << val2;
      const uint32 MASK = ((uint32) 1u << 16) - 1;
      ec.regs.d[reg1] = ec.regs.d[reg1] & ~MASK | (uint32) val & MASK;
      ec.regs.sr.set_cc(val);	// FIXME.

      ec.regs.pc += 2;
    }

  void lsll_i_d(unsigned int op, context &ec)
    {
      int reg1 = op & 0x7;
      int val2 = op >> 9 & 0x7;
      if (val2 == 0)
	val2 = 8;
      VL((" lsll #%d,%%d%d\n", val2, reg1));

      uint32 val = ec.regs.d[reg1] << val2;
      ec.regs.d[reg1] = val;
      ec.regs.sr.set_cc(val);	// FIXME.

      ec.regs.pc += 2;
    }

  void
  lsrw_i(unsigned int op, context &ec)
  {
    unsigned int reg1 = op & 0x7;
    uint_type count = op >> 9 & 0x7;
    if (count == 0)
      count = 8;
#ifdef L
    L(" lsrw #%u", count);
    L(",%%d%u", reg1);
    L("\t| 0x%04x, %%pc = 0x%lx\n", op, (unsigned long) ec.regs.pc);
#endif

    sint_type value1 = extsw(ec.regs.d[reg1]);
    sint_type value = extsw(uint_type(value1) >> count);
    const uint32_type MASK = (uint32_type(1) << 16) - 1;
    ec.regs.d[reg1] = ec.regs.d[reg1] & ~MASK | uint32_type(value) & MASK;
    ec.regs.sr.set_cc_lsr(value, value1, count);

    ec.regs.pc += 2;
  }

  void
  lsrl_i(unsigned int op, context &ec)
  {
    unsigned int reg1 = op & 0x7;
    uint_type count = op >> 9 & 0x7;
    if (count == 0)
      count = 8;
#ifdef L
    L(" lsrl #%u", count);
    L(",%%d%u\n", reg1);
    L("\t| 0x%04x, %%pc = 0x%lx\n", op, (unsigned long) ec.regs.pc);
#endif

    sint32_type value1 = extsl(ec.regs.d[reg1]);
    sint32_type value = extsl(uint32_type(value1) >> count);
    ec.regs.d[reg1] = value;
    ec.regs.sr.set_cc_lsr(value, value1, count);

    ec.regs.pc += 2;
  }

  template <class Source, class Destination>
    void moveb(unsigned int op, context &ec)
    {
      Source ea1(op & 0x7, 2);
      Destination ea2(op >> 9 & 0x7, 2 + ea1.isize(2));
      VL((" moveb %s", ea1.textb(ec)));
      VL((",%s", ea2.textb(ec)));
      VL((" | %%pc = 0x%lx\n", (unsigned long) ec.regs.pc));

      int value = ea1.getb(ec);
      ea2.putb(ec, value);
      ea1.finishb(ec);
      ea2.finishb(ec);
      ec.regs.sr.set_cc(value);

      ec.regs.pc += 2 + ea1.isize(2) + ea2.isize(2);
    }

  void moveb_d_postinc(unsigned int op, context &ec)
    {
      int s_reg = op & 0x7;
      int d_reg = op >> 9 & 0x7;
      uint32 d_addr = ec.regs.a[d_reg];
      VL((" moveb %%d%d,%%a%d@+ |*,0x%lx\n",
	  s_reg, d_reg, (unsigned long) d_addr));

      int fc = ec.data_fc();
      int val = extsb(ec.regs.d[s_reg]);
      ec.mem->putb(fc, d_addr, val);
      ec.regs.a[d_reg] = d_addr + 1;
      ec.regs.sr.set_cc(val);

      ec.regs.pc += 2;
    }

  void moveb_postinc_postinc(unsigned int op, context &ec)
    {
      int s_reg = op & 0x7;
      uint32 s_addr = ec.regs.a[s_reg];
      int d_reg = op >> 9 & 0x7;
      uint32 d_addr = ec.regs.a[d_reg];
      VL((" moveb %%a%d@+,%%a%d@+ |0x%lx,0x%lx\n",
	  s_reg, d_reg, (unsigned long) s_addr, (unsigned long) d_addr));

      int fc = ec.data_fc();
      int value = extsb(ec.mem->getb(fc, s_addr));
      ec.mem->putb(fc, d_addr, value);
      ec.regs.a[s_reg] = s_addr + 1;
      ec.regs.a[d_reg] = d_addr + 1;
      ec.regs.sr.set_cc(value);

      ec.regs.pc += 2;
    }

  template <class Source, class Destination> void movew(unsigned int op,
							context &ec)
    {
      Source ea1(op & 0x7, 2);
      Destination ea2(op >> 9 & 0x7, 2 + ea1.isize(2));
#ifdef L
      L(" movew %s", ea1.textw(ec));
      L(",%s", ea2.textw(ec));
      L("\t| 0x%04x, %%pc = 0x%lx\n", op, (unsigned long) ec.regs.pc);
#endif

      int value = ea1.getw(ec);
      ea2.putw(ec, value);
      ea1.finishw(ec);
      ea2.finishw(ec);
      ec.regs.sr.set_cc(value);

      ec.regs.pc += 2 + ea1.isize(2) + ea2.isize(2);
    }

  void movew_d_predec(unsigned int op, context &ec)
    {
      int s_reg = op & 0x7;
      int d_reg = op >> 9 & 0x7;
      uint32 d_addr = ec.regs.a[d_reg] - 2;
      VL((" movew %%d%d,%%a%x@- |*,0x%lx\n",
	  s_reg, d_reg, (unsigned long) d_addr));

      int fc = ec.data_fc();
      int value = extsw(ec.regs.d[s_reg]);
      ec.mem->putw(fc, d_addr, value);
      ec.regs.a[d_reg] = d_addr;
      ec.regs.sr.set_cc(value);

      ec.regs.pc += 2;
    }

  void movew_absl_predec(unsigned int op, context &ec)
    {
      int d_reg = op >> 9 & 0x7;
      uint32 d_addr = ec.regs.a[d_reg] - 2;
      uint32 s_addr = ec.fetchl(2);
      VL((" movew 0x%lx,%%a%x@- |*,0x%lx\n",
	  (unsigned long) s_addr, d_reg, (unsigned long) d_addr));

      int fc = ec.data_fc();
      int value = extsw(ec.mem->getw(fc, s_addr));
      ec.mem->putw(fc, d_addr, value);
      ec.regs.a[d_reg] = d_addr;
      ec.regs.sr.set_cc(value);

      ec.regs.pc += 2 + 4;
    }

  void movew_d_absl(unsigned int op, context &ec)
    {
      int reg = op & 0x7;
      uint32 address = ec.fetchl(2);
      VL((" movew %%d%d,0x%x\n", reg, address));

      int fc = ec.data_fc();
      int value = extsw(ec.regs.d[reg]);
      ec.mem->putw(fc, address, value);
      ec.regs.sr.set_cc(value);

      ec.regs.pc += 2 + 4;
    }

  template <class Source, class Destination>
    void movel(unsigned int op, context &ec)
    {
      Source ea1(op & 0x7, 2);
      Destination ea2(op >> 9 & 0x7, 2 + ea1.isize(4));
      VL((" movel %s", ea1.textl(ec)));
      VL((",%s", ea2.textl(ec)));
      VL(("\t| 0x%04x, %%pc = 0x%lx\n", op, (unsigned long) ec.regs.pc));

      int32 value = ea1.getl(ec);
      ea2.putl(ec, value);
      ea1.finishl(ec);
      ea2.finishl(ec);
      ec.regs.sr.set_cc(value);

      ec.regs.pc += 2 + ea1.isize(4) + ea2.isize(4);
    }

  template <class Source>
    void moveal(unsigned int op, context &ec)
    {
      Source ea1(op & 0x7, 2);
      address_register ea2(op >> 9 & 0x7, 2 + ea1.isize(4));
      VL((" moveal %s", ea1.textl(ec)));
      VL((",%s", ea2.textl(ec)));
      VL(("\t| 0x%04x, %%pc = 0x%lx\n", op, (unsigned long) ec.regs.pc));

      // XXX: The condition codes are not affected by this
      // instruction.
      int32 value = ea1.getl(ec);
      ea2.putl(ec, value);
      ea1.finishl(ec);
      ea2.finishl(ec);

      ec.regs.pc += 2 + ea1.isize(4) + ea2.isize(4);
    }

  /* movem regs to EA (postdec).  */
  void moveml_r_predec(unsigned int op, context &ec)
    {
      int reg = op & 0x0007;
      unsigned int bitmap = ec.fetchw(2);
      VL((" moveml #0x%x,%%a%d@-\n", bitmap, reg));

      // XXX: The condition codes are not affected.
      uint32 address = ec.regs.a[reg];
      int fc = ec.data_fc();
      for (uint32 *i = ec.regs.a + 8; i != ec.regs.a + 0; --i)
	{
	  if (bitmap & 1 != 0)
	    {
	      ec.mem->putl(fc, address - 4, *(i - 1));
	      address -= 4;
	    }
	  bitmap >>= 1;
	}
      for (uint32 *i = ec.regs.d + 8; i != ec.regs.d + 0; --i)
	{
	  if (bitmap & 1 != 0)
	    {
	      ec.mem->putl(fc, address - 4, *(i - 1));
	      address -= 4;
	    }
	  bitmap >>= 1;
	}
      ec.regs.a[reg] = address;

      ec.regs.pc += 2 + 2;
    }

  /* moveml instruction (memory to register) */
  template <class Source> void moveml_mr(unsigned int op,
					 context &ec)
    {
      Source ea1(op & 0x7, 4);
      unsigned int bitmap = ec.fetchw(2);
#ifdef L
      L(" moveml %s", ea1.textl(ec));
      L(",#0x%04x", bitmap);
      L("\t| 0x%04x, %%pc = 0x%lx\n", op, (unsigned long) ec.regs.pc);
#endif

      // XXX: The condition codes are not affected.
      uint32 address = ea1.address(ec);
      int fc = ec.data_fc();
      for (uint32 *i = ec.regs.d + 0; i != ec.regs.d + 8; ++i)
	{
	  if (bitmap & 1 != 0)
	    {
	      *i = ec.mem->getl(fc, address);
	      address += 4;
	    }
	  bitmap >>= 1;
	}
      for (uint32 *i = ec.regs.a + 0; i != ec.regs.a + 8; ++i)
	{
	  if (bitmap & 1 != 0)
	    {
	      *i = ec.mem->getl(fc, address);
	      address += 4;
	    }
	  bitmap >>= 1;
	}

      ec.regs.pc += 4 + ea1.isize(4);
    }

  /* moveml (postinc) */
  template <> void moveml_mr<postinc_indirect>(unsigned int op,
					       context &ec)
    {
      int reg1 = op & 0x7;
      unsigned int bitmap = ec.fetchw(2);
#ifdef L
      L(" moveml %%a%d@+", reg1);
      L(",#0x%04x", bitmap);
      L("\t| 0x%04x, %%pc = 0x%lx\n", op, (unsigned long) ec.regs.pc);
#endif

      // XXX: The condition codes are not affected.
      uint32 address = ec.regs.a[reg1];
      int fc = ec.data_fc();
      for (uint32 *i = ec.regs.d + 0; i != ec.regs.d + 8; ++i)
	{
	  if (bitmap & 1 != 0)
	    {
	      *i = ec.mem->getl(fc, address);
	      address += 4;
	    }
	  bitmap >>= 1;
	}
      for (uint32 *i = ec.regs.a + 0; i != ec.regs.a + 8; ++i)
	{
	  if (bitmap & 1 != 0)
	    {
	      *i = ec.mem->getl(fc, address);
	      address += 4;
	    }
	  bitmap >>= 1;
	}
      ec.regs.a[reg1] = address;

      ec.regs.pc += 4;
    }

  void moveql_d(unsigned int op, context &ec)
    {
      int value = extsb(op & 0xff);
      int reg = op >> 9 & 0x7;
      VL((" moveql #%d,%%d%d\n", value, reg));
      
      ec.regs.d[reg] = value;
      ec.regs.sr.set_cc(value);

      ec.regs.pc += 2;
    }

  template <class Source> void
  orw(unsigned int op, context &ec)
  {
    Source ea1(op & 0x7, 2);
    int reg2 = op >> 9 & 0x7;
#ifdef L
    L(" orw %s", ea1.textw(ec));
    L(",%%d%d", reg2);
    L("\t| 0x%04x, %%pc = 0x%lx\n", op, (unsigned long) ec.regs.pc);
#endif

    uint_type value1 = ea1.getw(ec);
    uint_type value2 = ec.regs.d[reg2];
    uint_type value = value2 | value1;
    ec.regs.d[reg2]
      = ec.regs.d[reg2] & ~0xffff | uint32_type(value) & 0xffff;
    ec.regs.sr.set_cc(value);
    ea1.finishw(ec);

    ec.regs.pc += 2 + ea1.isize(2);
  }

  template <class Destination> void pea(unsigned int op,
					context &ec)
    {
      Destination ea1(op & 0x7, 2);
#ifdef L
      L(" pea %s", ea1.textw(ec));
      L("\t| 0x%04x, %%pc = 0x%lx\n", op, (unsigned long) ec.regs.pc);
#endif

      // XXX: The condition codes are not affected.
      uint32 address = ea1.address(ec);
      int fc = ec.data_fc();
      ec.mem->putl(fc, ec.regs.a[7] - 4, address);
      ec.regs.a[7] -= 4;

      ec.regs.pc += 2 + ea1.isize(0);
    }

  void rts(unsigned int op, context &ec)
    {
      VL((" rts\n"));

      // XXX: The condition codes are not affected.
      int fc = ec.data_fc();
      uint32 value = ec.mem->getl(fc, ec.regs.a[7]);
      ec.regs.a[7] += 4;
      ec.regs.pc = value;
    }

  void subb_postinc_d(unsigned int op, context &ec)
    {
      int reg1 = op & 0x7;
      int reg2 = op >> 9 & 0x7;
      uint32 addr1 = ec.regs.a[reg1];
      VL((" subb %%a%d@+,%%d%d |0x%lx,*\n",
	  reg1, reg2, (unsigned long) addr1));

      int fc = ec.data_fc();
      int val1 = extsb(ec.mem->getb(fc, addr1));
      int val2 = extsb(ec.regs.d[reg2]);
      int val = extsb(val2 - val1);
      const uint32 MASK = ((uint32) 1u << 8) - 1;
      ec.regs.d[reg2] = ec.regs.d[reg2] & ~MASK | (uint32) val & MASK;
      ec.regs.a[reg1] = addr1 + 1;
      ec.regs.sr.set_cc(val);	// FIXME.

      ec.regs.pc += 2;
    }

  template <class Destination> void subl(unsigned int op, context &ec)
    {
      Destination ea1(op & 0x7, 2);
      int reg2 = op >> 9 & 0x7;
      VL((" subl %s", ea1.textl(ec)));
      VL((",%%d%d", reg2));
      VL((" | %%pc = 0x%lx\n", (unsigned long) ec.regs.pc));

      int32 value1 = ea1.getl(ec);
      int32 value2 = extsl(ec.regs.d[reg2]);
      int32 value = extsl(value2 - value1);
      ec.regs.d[reg2] = value;
      ea1.finishl(ec);
      ec.regs.sr.set_cc(value); // FIXME.

      ec.regs.pc += 2 + ea1.isize(4);
    }

  template <class Destination> void subib(unsigned int op,
					  context &ec)
    {
      int value2 = extsb(ec.fetchw(2));
      Destination ea1(op & 0x7, 2 + 2);
#ifdef L
      L(" subib #%d", value2);
      L(",%s", ea1.textb(ec));
      L("\t| 0x%04x, %%pc = 0x%lx\n", op, (unsigned long) ec.regs.pc);
#endif

      int value1 = ea1.getb(ec);
      int value = extsb(value1 - value2);
      ea1.putb(ec, value);
      ea1.finishb(ec);
      ec.regs.sr.set_cc(value); // FIXME.

      ec.regs.pc += 2 + 2 + ea1.isize(2);
    }

  void subql_d(unsigned int op, context &ec)
    {
      int reg1 = op & 0x7;
      int val2 = op >> 9 & 0x7;
      if (val2 == 0)
	val2 = 8;
      VL((" subql #%d,%%d%d\n", val2, reg1));

      int32 val1 = extsl(ec.regs.d[reg1]);
      int32 val = extsl(val1 - val2);
      ec.regs.d[reg1] = val;
      ec.regs.sr.set_cc(val); // FIXME.

      ec.regs.pc += 2;
    }

  void subql_a(unsigned int op, context &ec)
    {
      int value = op >> 9 & 0x7;
      int reg = op & 0x7;
      if (value == 0)
	value = 8;
      VL((" subql #%d,%%a%d\n", value, reg));

      // XXX: The condition codes are not affected.
      ec.regs.a[reg] -= value;

      ec.regs.pc += 2;
    }

  template <class Destination>
    void tstb(unsigned int op, context &ec)
    {
      Destination ea1(op & 0x7, 2);
      VL((" tstb %s", ea1.textb(ec)));
      VL(("\t| 0x%04x, %%pc = 0x%lx\n", op, (unsigned long) ec.regs.pc));

      int value = ea1.getb(ec);
      ec.regs.sr.set_cc(value);
      ea1.finishb(ec);

      ec.regs.pc += 2 + ea1.isize(2);
    }

  void tstw_d(unsigned int op, context &ec)
    {
      int reg = op & 0x7;
      VL((" tstw %%d%d\n", reg));

      int value = extsw(ec.regs.d[reg]);
      ec.regs.sr.set_cc(value);

      ec.regs.pc += 2;
    }

  template <class Destination>
    void tstl(unsigned int op, context &ec)
    {
      Destination ea1(op & 0x7, 2);
      VL((" tstl %s", ea1.textl(ec)));
      VL(("\t| 0x%04x, %%pc = 0x%lx\n", op, (unsigned long) ec.regs.pc));

      int32 value = ea1.getl(ec);
      ec.regs.sr.set_cc(value);
      ea1.finishl(ec);

      ec.regs.pc += 2 + ea1.isize(4);
    }

  void unlk_a(unsigned int op, context &ec)
    {
      int reg = op & 0x0007;
      VL((" unlk %%a%d\n", reg));

      // XXX: The condition codes are not affected.
      int fc = ec.data_fc();
      uint32 address = ec.mem->getl(fc, ec.regs.a[reg]);
      ec.regs.a[7] = ec.regs.a[reg] + 4;
      ec.regs.a[reg] = address;

      ec.regs.pc += 2;
    }
} // (unnamed namespace)

/* Installs instructions into the execution unit.  */
void
exec_unit::install_instructions(exec_unit &eu)
{
  eu.set_instruction(0x0400, 0x0007, &subib<data_register>);
  eu.set_instruction(0x0410, 0x0007, &subib<indirect>);
  eu.set_instruction(0x0418, 0x0007, &subib<postinc_indirect>);
  eu.set_instruction(0x0420, 0x0007, &subib<predec_indirect>);
  eu.set_instruction(0x0428, 0x0007, &subib<disp_indirect>);
  eu.set_instruction(0x0439, 0x0000, &subib<absolute_long>);
  eu.set_instruction(0x0680, 0x0007, &addil<data_register>);
  eu.set_instruction(0x0690, 0x0007, &addil<indirect>);
  eu.set_instruction(0x0698, 0x0007, &addil<postincrement_indirect>);
  eu.set_instruction(0x06a0, 0x0007, &addil<predecrement_indirect>);
  eu.set_instruction(0x0c00, 0x0007, &cmpib<data_register>);
  eu.set_instruction(0x0c10, 0x0007, &cmpib<indirect>);
  eu.set_instruction(0x0c18, 0x0007, &cmpib<postincrement_indirect>);
  eu.set_instruction(0x0c20, 0x0007, &cmpib<predecrement_indirect>);
  eu.set_instruction(0x0a40, 0x0007, &eoriw<data_register>);
  eu.set_instruction(0x0a50, 0x0007, &eoriw<indirect>);
  eu.set_instruction(0x0a58, 0x0007, &eoriw<postinc_indirect>);
  eu.set_instruction(0x0a60, 0x0007, &eoriw<predec_indirect>);
  eu.set_instruction(0x0a68, 0x0007, &eoriw<disp_indirect>);
  eu.set_instruction(0x0a79, 0x0000, &eoriw<absolute_long>);
  eu.set_instruction(0x1000, 0x0e07, &moveb<data_register, data_register>);
  eu.set_instruction(0x1010, 0x0e07, &moveb<indirect, data_register>);
  eu.set_instruction(0x1018, 0x0e07, &moveb<postincrement_indirect, data_register>);
  eu.set_instruction(0x1020, 0x0e07, &moveb<predecrement_indirect, data_register>);
  eu.set_instruction(0x1028, 0x0e07, &moveb<disp_indirect, data_register>);
  eu.set_instruction(0x1039, 0x0e00, &moveb<absolute_long, data_register>);
  eu.set_instruction(0x103a, 0x0e00, &moveb<disp_pc, data_register>);
  eu.set_instruction(0x103c, 0x0e00, &moveb<immediate, data_register>);
  eu.set_instruction(0x10c0, 0x0e07, &moveb_d_postinc);
  eu.set_instruction(0x10d8, 0x0e07, &moveb_postinc_postinc);
  eu.set_instruction(0x1140, 0x0e07, &moveb<data_register, disp_indirect>);
  eu.set_instruction(0x1150, 0x0e07, &moveb<indirect, disp_indirect>);
  eu.set_instruction(0x1158, 0x0e07, &moveb<postinc_indirect, disp_indirect>);
  eu.set_instruction(0x1160, 0x0e07, &moveb<predec_indirect, disp_indirect>);
  eu.set_instruction(0x1168, 0x0e07, &moveb<disp_indirect, disp_indirect>);
  eu.set_instruction(0x1179, 0x0e00, &moveb<absolute_long, disp_indirect>);
  eu.set_instruction(0x117a, 0x0e00, &moveb<disp_pc, disp_indirect>);
  eu.set_instruction(0x117c, 0x0e00, &moveb<immediate, disp_indirect>);
  eu.set_instruction(0x13c0, 0x0007, &moveb<data_register, absolute_long>);
  eu.set_instruction(0x2000, 0x0e07, &movel<data_register, data_register>);
  eu.set_instruction(0x2008, 0x0e07, &movel<address_register, data_register>);
  eu.set_instruction(0x2010, 0x0e07, &movel<indirect, data_register>);
  eu.set_instruction(0x2018, 0x0e07, &movel<postincrement_indirect, data_register>);
  eu.set_instruction(0x2020, 0x0e07, &movel<predecrement_indirect, data_register>);
  eu.set_instruction(0x2028, 0x0e07, &movel<disp_indirect, data_register>);
  eu.set_instruction(0x2039, 0x0e00, &movel<absolute_long, data_register>);
  eu.set_instruction(0x203c, 0x0e00, &movel<immediate, data_register>);
  eu.set_instruction(0x2040, 0x0e07, &moveal<data_register>);
  eu.set_instruction(0x2048, 0x0e07, &moveal<address_register>);
  eu.set_instruction(0x2050, 0x0e07, &moveal<indirect>);
  eu.set_instruction(0x2058, 0x0e07, &moveal<postinc_indirect>);
  eu.set_instruction(0x2060, 0x0e07, &moveal<predec_indirect>);
  eu.set_instruction(0x2068, 0x0e07, &moveal<disp_indirect>);
  eu.set_instruction(0x2079, 0x0e00, &moveal<absolute_long>);
  eu.set_instruction(0x207c, 0x0e00, &moveal<immediate>);
  eu.set_instruction(0x2080, 0x0e07, &movel<data_register, indirect>);
  eu.set_instruction(0x2088, 0x0e07, &movel<address_register, indirect>);
  eu.set_instruction(0x2090, 0x0e07, &movel<indirect, indirect>);
  eu.set_instruction(0x2098, 0x0e07, &movel<postinc_indirect, indirect>);
  eu.set_instruction(0x20a0, 0x0e07, &movel<predec_indirect, indirect>);
  eu.set_instruction(0x20a8, 0x0e07, &movel<disp_indirect, indirect>);
  eu.set_instruction(0x20b9, 0x0e00, &movel<absolute_long, indirect>);
  eu.set_instruction(0x20bc, 0x0e00, &movel<immediate, indirect>);
  eu.set_instruction(0x20c0, 0x0e07, &movel<data_register, postinc_indirect>);
  eu.set_instruction(0x20c8, 0x0e07, &movel<address_register, postinc_indirect>);
  eu.set_instruction(0x20d0, 0x0e07, &movel<indirect, postinc_indirect>);
  eu.set_instruction(0x20d8, 0x0e07, &movel<postinc_indirect, postinc_indirect>);
  eu.set_instruction(0x20e0, 0x0e07, &movel<predec_indirect, postinc_indirect>);
  eu.set_instruction(0x20e8, 0x0e07, &movel<disp_indirect, postinc_indirect>);
  eu.set_instruction(0x20f9, 0x0e00, &movel<absolute_long, postinc_indirect>);
  eu.set_instruction(0x20fc, 0x0e00, &movel<immediate, postinc_indirect>);
  eu.set_instruction(0x2100, 0x0e07, &movel<data_register, predec_indirect>);
  eu.set_instruction(0x2108, 0x0e07, &movel<address_register, predec_indirect>);
  eu.set_instruction(0x2110, 0x0e07, &movel<indirect, predec_indirect>);
  eu.set_instruction(0x2118, 0x0e07, &movel<postinc_indirect, predec_indirect>);
  eu.set_instruction(0x2120, 0x0e07, &movel<predec_indirect, predec_indirect>);
  eu.set_instruction(0x2128, 0x0e07, &movel<disp_indirect, predec_indirect>);
  eu.set_instruction(0x2139, 0x0e00, &movel<absolute_long, predec_indirect>);
  eu.set_instruction(0x213c, 0x0e00, &movel<immediate, predec_indirect>);
  eu.set_instruction(0x2140, 0x0e07, &movel<data_register, disp_indirect>);
  eu.set_instruction(0x2148, 0x0e07, &movel<address_register, disp_indirect>);
  eu.set_instruction(0x2150, 0x0e07, &movel<indirect, disp_indirect>);
  eu.set_instruction(0x2158, 0x0e07, &movel<postinc_indirect, disp_indirect>);
  eu.set_instruction(0x2160, 0x0e07, &movel<predec_indirect, disp_indirect>);
  eu.set_instruction(0x2168, 0x0e07, &movel<disp_indirect, disp_indirect>);
  eu.set_instruction(0x2179, 0x0e00, &movel<absolute_long, disp_indirect>);
  eu.set_instruction(0x217c, 0x0e00, &movel<immediate, disp_indirect>);
  eu.set_instruction(0x23c0, 0x0007, &movel<data_register, absolute_long>);
  eu.set_instruction(0x23c8, 0x0007, &movel<address_register, absolute_long>);
  eu.set_instruction(0x23d0, 0x0007, &movel<indirect, absolute_long>);
  eu.set_instruction(0x23d8, 0x0007, &movel<postinc_indirect, absolute_long>);
  eu.set_instruction(0x23e0, 0x0007, &movel<predec_indirect, absolute_long>);
  eu.set_instruction(0x23e8, 0x0007, &movel<disp_indirect, absolute_long>);
  eu.set_instruction(0x23fc, 0x0000, &movel<immediate, absolute_long>);
  eu.set_instruction(0x3000, 0x0e07, &movew<data_register, data_register>);
  eu.set_instruction(0x3008, 0x0e07, &movew<address_register, data_register>);
  eu.set_instruction(0x3010, 0x0e07, &movew<indirect, data_register>);
  eu.set_instruction(0x3018, 0x0e07, &movew<postincrement_indirect, data_register>);
  eu.set_instruction(0x3020, 0x0e07, &movew<predecrement_indirect, data_register>);
  eu.set_instruction(0x3028, 0x0e07, &movew<disp_indirect, data_register>);
  eu.set_instruction(0x3039, 0x0e00, &movew<absolute_long, data_register>);
  eu.set_instruction(0x303c, 0x0e00, &movew<immediate, data_register>);
  eu.set_instruction(0x3080, 0x0e07, &movew<data_register, indirect>);
  eu.set_instruction(0x3088, 0x0e07, &movew<address_register, indirect>);
  eu.set_instruction(0x3090, 0x0e07, &movew<indirect, indirect>);
  eu.set_instruction(0x3098, 0x0e07, &movew<postinc_indirect, indirect>);
  eu.set_instruction(0x30a0, 0x0e07, &movew<predec_indirect, indirect>);
  eu.set_instruction(0x30a8, 0x0e07, &movew<disp_indirect, indirect>);
  eu.set_instruction(0x30b9, 0x0e00, &movew<absolute_long, indirect>);
  eu.set_instruction(0x30bc, 0x0e00, &movew<immediate, indirect>);
  eu.set_instruction(0x30c0, 0x0e07, &movew<data_register, postinc_indirect>);
  eu.set_instruction(0x30c8, 0x0e07, &movew<address_register, postinc_indirect>);
  eu.set_instruction(0x30d0, 0x0e07, &movew<indirect, postinc_indirect>);
  eu.set_instruction(0x30d8, 0x0e07, &movew<postinc_indirect, postinc_indirect>);
  eu.set_instruction(0x30e0, 0x0e07, &movew<predec_indirect, postinc_indirect>);
  eu.set_instruction(0x30e8, 0x0e07, &movew<disp_indirect, postinc_indirect>);
  eu.set_instruction(0x30f9, 0x0e00, &movew<absolute_long, postinc_indirect>);
  eu.set_instruction(0x30fc, 0x0e00, &movew<immediate, postinc_indirect>);
  eu.set_instruction(0x3100, 0x0e07, &movew_d_predec);
  eu.set_instruction(0x3139, 0x0e00, &movew_absl_predec);
  eu.set_instruction(0x33c0, 0x0007, &movew_d_absl);
  eu.set_instruction(0x4190, 0x0e07, &lea<indirect>);
  eu.set_instruction(0x41e8, 0x0e07, &lea<disp_indirect>);
  eu.set_instruction(0x41f9, 0x0e00, &lea<absolute_long>);
  eu.set_instruction(0x4200, 0x0007, &clrb<data_register>);
  eu.set_instruction(0x4210, 0x0007, &clrb<indirect>);
  eu.set_instruction(0x4218, 0x0007, &clrb<postincrement_indirect>);
  eu.set_instruction(0x4220, 0x0007, &clrb<predecrement_indirect>);
  eu.set_instruction(0x4228, 0x0007, &clrb<disp_indirect>);
  eu.set_instruction(0x4239, 0x0000, &clrb<absolute_long>);
  eu.set_instruction(0x4240, 0x0007, &clrw<data_register>);
  eu.set_instruction(0x4250, 0x0007, &clrw<indirect>);
  eu.set_instruction(0x4258, 0x0007, &clrw<postincrement_indirect>);
  eu.set_instruction(0x4260, 0x0007, &clrw<predecrement_indirect>);
  eu.set_instruction(0x4268, 0x0007, &clrw<disp_indirect>);
  eu.set_instruction(0x4279, 0x0000, &clrw<absolute_long>);
  eu.set_instruction(0x4280, 0x0007, &clrl<data_register>);
  eu.set_instruction(0x4290, 0x0007, &clrl<indirect>);
  eu.set_instruction(0x4298, 0x0007, &clrl<postincrement_indirect>);
  eu.set_instruction(0x42a0, 0x0007, &clrl<predecrement_indirect>);
  eu.set_instruction(0x42a8, 0x0007, &clrl<disp_indirect>);
  eu.set_instruction(0x42b9, 0x0000, &clrl<absolute_long>);
  eu.set_instruction(0x4850, 0x0007, &pea<indirect>);
  eu.set_instruction(0x4868, 0x0007, &pea<disp_indirect>);
  eu.set_instruction(0x4879, 0x0000, &pea<absolute_long>);
  eu.set_instruction(0x487a, 0x0000, &pea<disp_pc>);
  eu.set_instruction(0x48e0, 0x0007, &moveml_r_predec);
  eu.set_instruction(0x4cd0, 0x0007, &moveml_mr<indirect>);
  eu.set_instruction(0x4cd8, 0x0007, &moveml_mr<postinc_indirect>);
  eu.set_instruction(0x4ce8, 0x0007, &moveml_mr<disp_indirect>);
  eu.set_instruction(0x4cf9, 0x0000, &moveml_mr<absolute_long>);
  eu.set_instruction(0x4cfa, 0x0000, &moveml_mr<disp_pc>);
  eu.set_instruction(0x4a00, 0x0007, &tstb<data_register>);
  eu.set_instruction(0x4a10, 0x0007, &tstb<indirect>);
  eu.set_instruction(0x4a18, 0x0007, &tstb<postinc_indirect>);
  eu.set_instruction(0x4a20, 0x0007, &tstb<predec_indirect>);
  eu.set_instruction(0x4a28, 0x0007, &tstb<disp_indirect>);
  eu.set_instruction(0x4a39, 0x0000, &tstb<absolute_long>);
  eu.set_instruction(0x4a40, 0x0007, &tstw_d);
  eu.set_instruction(0x4a80, 0x0007, &tstl<data_register>);
  eu.set_instruction(0x4a90, 0x0007, &tstl<indirect>);
  eu.set_instruction(0x4a98, 0x0007, &tstl<postinc_indirect>);
  eu.set_instruction(0x4aa0, 0x0007, &tstl<predec_indirect>);
  eu.set_instruction(0x4aa8, 0x0007, &tstl<disp_indirect>);
  eu.set_instruction(0x4ab9, 0x0000, &tstl<absolute_long>);
  eu.set_instruction(0x4e50, 0x0007, &link_a);
  eu.set_instruction(0x4e58, 0x0007, &unlk_a);
  eu.set_instruction(0x4e75, 0x0000, &rts);
  eu.set_instruction(0x4e90, 0x0007, &jsr<indirect>);
  eu.set_instruction(0x4ea8, 0x0007, &jsr<disp_indirect>);
  eu.set_instruction(0x4eb9, 0x0000, &jsr<absolute_long>);
  eu.set_instruction(0x5000, 0x0e07, &addqb<data_register>);
  eu.set_instruction(0x5010, 0x0e07, &addqb<indirect>);
  eu.set_instruction(0x5018, 0x0e07, &addqb<postincrement_indirect>);
  eu.set_instruction(0x5020, 0x0e07, &addqb<predecrement_indirect>);
  eu.set_instruction(0x5040, 0x0e07, &addqw<data_register>);
  eu.set_instruction(0x5048, 0x0e07, &addqw<address_register>);
  eu.set_instruction(0x5050, 0x0e07, &addqw<indirect>);
  eu.set_instruction(0x5058, 0x0e07, &addqw<postincrement_indirect>);
  eu.set_instruction(0x5060, 0x0e07, &addqw<predecrement_indirect>);
  eu.set_instruction(0x5080, 0x0e07, &addql<data_register>);
  eu.set_instruction(0x5088, 0x0e07, &addql<address_register>);
  eu.set_instruction(0x5090, 0x0e07, &addql<indirect>);
  eu.set_instruction(0x5098, 0x0e07, &addql<postincrement_indirect>);
  eu.set_instruction(0x50a0, 0x0e07, &addql<predecrement_indirect>);
  eu.set_instruction(0x5180, 0x0e07, &subql_d);
  eu.set_instruction(0x5188, 0x0e07, &subql_a);
  eu.set_instruction(0x51c8, 0x0007, &dbf_d);
  eu.set_instruction(0x6000, 0x00ff, &bra);
  eu.set_instruction(0x6100, 0x00ff, &bsr);
  eu.set_instruction(0x6200, 0x00ff, &b<hi>);
  eu.set_instruction(0x6300, 0x00ff, &b<ls>);
  eu.set_instruction(0x6400, 0x00ff, &b<cc>);
  eu.set_instruction(0x6500, 0x00ff, &b<cs>);
  eu.set_instruction(0x6600, 0x00ff, &bne);
  eu.set_instruction(0x6700, 0x00ff, &beq);
  eu.set_instruction(0x6b00, 0x00ff, &bmi);
  eu.set_instruction(0x6c00, 0x00ff, &bge);
  eu.set_instruction(0x7000, 0x0eff, &moveql_d);
  eu.set_instruction(0x8040, 0x0e07, &orw<data_register>);
  eu.set_instruction(0x8050, 0x0e07, &orw<indirect>);
  eu.set_instruction(0x8058, 0x0e07, &orw<postinc_indirect>);
  eu.set_instruction(0x8060, 0x0e07, &orw<predec_indirect>);
  eu.set_instruction(0x8068, 0x0e07, &orw<disp_indirect>);
  eu.set_instruction(0x8079, 0x0e00, &orw<absolute_long>);
  eu.set_instruction(0x807a, 0x0e00, &orw<disp_pc>);
  eu.set_instruction(0x807c, 0x0e00, &orw<immediate>);
  eu.set_instruction(0x9018, 0x0e07, &subb_postinc_d);
  eu.set_instruction(0x9080, 0x0e07, &subl<data_register>);
  eu.set_instruction(0x9088, 0x0e07, &subl<address_register>);
  eu.set_instruction(0x9090, 0x0e07, &subl<indirect>);
  eu.set_instruction(0x9098, 0x0e07, &subl<postincrement_indirect>);
  eu.set_instruction(0x90a0, 0x0e07, &subl<predecrement_indirect>);
  eu.set_instruction(0x90b9, 0x0e00, &subl<absolute_long>);
  eu.set_instruction(0x90bc, 0x0e00, &subl<immediate>);
  eu.set_instruction(0xb000, 0x0e07, &cmpb<data_register>);
  eu.set_instruction(0xb010, 0x0e07, &cmpb<indirect>);
  eu.set_instruction(0xb018, 0x0e07, &cmpb<postinc_indirect>);
  eu.set_instruction(0xb020, 0x0e07, &cmpb<predec_indirect>);
  eu.set_instruction(0xb028, 0x0e07, &cmpb<disp_indirect>);
  eu.set_instruction(0xb039, 0x0e00, &cmpb<absolute_long>);
  eu.set_instruction(0xb03c, 0x0e00, &cmpb<immediate>);
  eu.set_instruction(0xb040, 0x0e07, &cmpw<data_register>);
  eu.set_instruction(0xb048, 0x0e07, &cmpw<address_register>);
  eu.set_instruction(0xb050, 0x0e07, &cmpw<indirect>);
  eu.set_instruction(0xb058, 0x0e07, &cmpw<postincrement_indirect>);
  eu.set_instruction(0xb060, 0x0e07, &cmpw<predecrement_indirect>);
  eu.set_instruction(0xb068, 0x0e07, &cmpw<disp_indirect>);
  eu.set_instruction(0xb079, 0x0e00, &cmpw<absolute_long>);
  eu.set_instruction(0xb07c, 0x0e00, &cmpw<immediate>);
  eu.set_instruction(0xb080, 0x0e07, &cmpl<data_register>);
  eu.set_instruction(0xb088, 0x0e07, &cmpl<address_register>);
  eu.set_instruction(0xb090, 0x0e07, &cmpl<indirect>);
  eu.set_instruction(0xb098, 0x0e07, &cmpl<postincrement_indirect>);
  eu.set_instruction(0xb0a0, 0x0e07, &cmpl<predecrement_indirect>);
  eu.set_instruction(0xb0a8, 0x0e07, &cmpl<disp_indirect>);
  eu.set_instruction(0xb0b9, 0x0e00, &cmpl<absolute_long>);
  eu.set_instruction(0xb0bc, 0x0e00, &cmpl<immediate>);
  eu.set_instruction(0xc0bc, 0x0e00, &andl_i_d);
  eu.set_instruction(0xd068, 0x0e07, &addw_off_d);
  eu.set_instruction(0xd080, 0x0e07, &addl<data_register>);
  eu.set_instruction(0xd088, 0x0e07, &addl<address_register>);
  eu.set_instruction(0xd090, 0x0e07, &addl<indirect>);
  eu.set_instruction(0xd098, 0x0e07, &addl<postinc_indirect>);
  eu.set_instruction(0xd0a0, 0x0e07, &addl<predec_indirect>);
  eu.set_instruction(0xd0a8, 0x0e07, &addl<disp_indirect>);
  eu.set_instruction(0xd0b9, 0x0e00, &addl<absolute_long>);
  eu.set_instruction(0xd0bc, 0x0e00, &addl<immediate>);
  eu.set_instruction(0xd1c0, 0x0e07, &addal<data_register>);
  eu.set_instruction(0xd1c8, 0x0e07, &addal<address_register>);
  eu.set_instruction(0xd1d0, 0x0e07, &addal<indirect>);
  eu.set_instruction(0xd1d8, 0x0e07, &addal<postincrement_indirect>);
  eu.set_instruction(0xd1e0, 0x0e07, &addal<predecrement_indirect>);
  eu.set_instruction(0xd1e8, 0x0e07, &addal<disp_indirect>);
  eu.set_instruction(0xd1f9, 0x0e07, &addal<absolute_long>);
  eu.set_instruction(0xd1fc, 0x0e07, &addal<immediate>);
  eu.set_instruction(0xe048, 0x0e07, &lsrw_i);
  eu.set_instruction(0xe088, 0x0e07, &lsrl_i);
  eu.set_instruction(0xe148, 0x0e07, &lslw_i_d);
  eu.set_instruction(0xe188, 0x0e07, &lsll_i_d);
}

