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

/* Sets an instruction to operation codes.  */
void
exec_unit::set_instruction(int code, int mask, instruction_handler h,
			   instruction_data *data)
{
  I (code >= 0);
  I (code < 0x10000);
  code &= ~mask;
  for (int i = code; i <= (code | mask); ++i)
    {
      if ((i & ~mask) == code)
	{
#ifdef L
	  if (instructions[i].first != &illegal)
	    L("warning: Replacing instruction handler at 0x%04x\n", i);
#endif
	  instructions[i] = make_pair(h, data);
	}
    }
}

exec_unit::exec_unit()
{
  fill(instructions + 0, instructions + 0x10000,
       make_pair(&illegal, (instruction_data *) 0));
  install_instructions(*this);
}

/* Executes an illegal instruction.  */
void
exec_unit::illegal(unsigned int op, context &ec, instruction_data *data)
{
  throw illegal_instruction();
}

namespace
{
  using namespace condition;
  using namespace addressing;

  template <class Source> void
  addb(unsigned int op, context &ec, instruction_data *data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef L
    L(" addb %s", ea1.textb(ec));
    L(",%%d%u\n", reg2);
#endif

    sint_type value1 = ea1.getb(ec);
    sint_type value2 = extsb(ec.regs.d[reg2]);
    sint_type value = extsb(value2 + value1);
    const uint32_type MASK = 0xffu;
    ec.regs.d[reg2] = ec.regs.d[reg2] & ~MASK | uint32_type(value) & MASK;
    ec.regs.sr.set_cc(value); // FIXME.
    ea1.finishb(ec);

    ec.regs.pc += 2 + ea1.isize(2);
  }

  template <class Source> void
  addw(unsigned int op, context &ec, instruction_data *data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef L
    L(" addw %s", ea1.textw(ec));
    L(",%%d%u\n", reg2);
#endif

    sint_type value1 = ea1.getw(ec);
    sint_type value2 = extsw(ec.regs.d[reg2]);
    sint_type value = extsw(value2 + value1);
    const uint32_type MASK = 0xffffu;
    ec.regs.d[reg2] = ec.regs.d[reg2] & ~MASK | uint32_type(value) & MASK;
    ec.regs.sr.set_cc(value); // FIXME.
    ea1.finishw(ec);

    ec.regs.pc += 2 + ea1.isize(2);
  }

  template <class Destination> void
  addw_r(unsigned int op, context &ec, instruction_data *data)
  {
    Destination ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef L
    L(" addw %%d%u", reg2);
    L(",%s\n", ea1.textw(ec));
#endif

    sint_type value2 = extsw(ec.regs.d[reg2]);
    sint_type value1 = ea1.getw(ec);
    sint_type value = extsw(value1 + value2);
    ea1.putw(ec, value);
    ec.regs.sr.set_cc(value);	// FIXME.
    ea1.finishw(ec);

    ec.regs.pc += 2 + ea1.isize(2);
  }

  template <class Source> void
  addl(unsigned int op, context &ec, instruction_data *data)
    {
      Source ea1(op & 0x7, 2);
      int reg2 = op >> 9 & 0x7;
#ifdef L
      L(" addl %s", ea1.textl(ec));
      L(",%%d%d\n", reg2);
#endif

      int32 value1 = ea1.getl(ec);
      int32 value2 = extsl(ec.regs.d[reg2]);
      int32 value = extsl(value2 + value1);
      ec.regs.d[reg2] = value;
      ea1.finishl(ec);
      ec.regs.sr.set_cc(value); // FIXME.

      ec.regs.pc += 2 + ea1.isize(4);
    }

  template <class Destination> void
  addl_r(uint_type op, context &ec, instruction_data *data)
  {
    Destination ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef L
    L(" addl %%d%u", reg2);
    L(",%s\n", ea1.textl(ec));
#endif

    sint32_type value2 = extsl(ec.regs.d[reg2]);
    sint32_type value1 = ea1.getl(ec);
    sint32_type value = extsl(value1 + value2);
    ea1.putl(ec, value);
    ec.regs.sr.set_cc(value);	// FIXME.
    ea1.finishl(ec);

    ec.regs.pc += 2 + ea1.isize(4);
  }

  template <class Destination> void
  addal(unsigned int op, context &ec, instruction_data *data)
    {
      Destination ea1(op & 0x7, 2);
      int reg2 = op >> 9 & 0x7;
#ifdef L
      L(" addal %s", ea1.textl(ec));
      L(",%%a%d\n", reg2);
#endif

      int32 value1 = ea1.getl(ec);
      int32 value2 = extsl(ec.regs.a[reg2]);
      int32 value = extsl(value2 + value1);
      ec.regs.a[reg2] = value;
      ea1.finishl(ec);
      // XXX: The condition codes are not affected.

      ec.regs.pc += 2 + ea1.isize(4);
    }

  template <class Destination> void
  addil(unsigned int op, context &ec, instruction_data *data)
    {
      int32 value2 = extsl(ec.fetchl(2));
      Destination ea1(op & 0x7, 2 + 4);
#ifdef L
      L(" addil #%ld", (long) value2);
      L(",%s\n", ea1.textl(ec));
#endif

      int32 value1 = ea1.getl(ec);
      int32 value = extsl(value1 + value2);
      ea1.putl(ec, value);
      ea1.finishl(ec);
      ec.regs.sr.set_cc(value); // FIXME.

      ec.regs.pc += 2 + 4 + ea1.isize(4);
    }

  template <class Destination> void
  addqb(unsigned int op, context &ec, instruction_data *data)
    {
      Destination ea1(op & 0x7, 2);
      int value2 = op >> 9 & 0x7;
      if (value2 == 0)
	value2 = 8;
#ifdef L
      L(" addqb #%d", value2);
      L(",%s\n", ea1.textb(ec));
#endif

      int value1 = ea1.getb(ec);
      int value = extsb(value1 + value2);
      ea1.putb(ec, value);
      ea1.finishb(ec);
      ec.regs.sr.set_cc(value); // FIXME.

      ec.regs.pc += 2 + ea1.isize(2);
    }

  template <class Destination> void
  addqw(unsigned int op, context &ec, instruction_data *data)
    {
      Destination ea1(op & 0x7, 2);
      int value2 = op >> 9 & 0x7;
      if (value2 == 0)
	value2 = 8;
#ifdef L
      L(" addqw #%d", value2);
      L(",%s\n", ea1.textw(ec));
#endif

      int value1 = ea1.getw(ec);
      int value = extsw(value1 + value2);
      ea1.putw(ec, value);
      ea1.finishw(ec);
      ec.regs.sr.set_cc(value); // FIXME.

      ec.regs.pc += 2 + ea1.isize(2);
    }

  template <> void
  addqw<address_register>(unsigned int op, context &ec, instruction_data *data)
    {
      address_register ea1(op & 0x7, 2);
      int value2 = op >> 9 & 0x7;
      if (value2 == 0)
	value2 = 8;
#ifdef L
      L(" addqw #%d", value2);
      L(",%s\n", ea1.textw(ec));
#endif

      // XXX: The entire register is used.
      int32 value1 = ea1.getl(ec);
      int32 value = extsl(value1 + value2);
      ea1.putl(ec, value);
      ea1.finishl(ec);
      // XXX: The condition codes are not affected.

      ec.regs.pc += 2 + ea1.isize(2);
    }

  template <class Destination> void
  addql(unsigned int op, context &ec, instruction_data *data)
    {
      Destination ea1(op & 0x7, 2);
      int value2 = op >> 9 & 0x7;
      if (value2 == 0)
	value2 = 8;
#ifdef L
      L(" addql #%d", value2);
      L(",%s\n", ea1.textl(ec));
#endif

      int32 value1 = ea1.getl(ec);
      int32 value = extsl(value1 + value2);
      ea1.putl(ec, value);
      ea1.finishl(ec);
      ec.regs.sr.set_cc(value); // FIXME.

      ec.regs.pc += 2 + ea1.isize(4);
    }

  template <> void
  addql<address_register>(unsigned int op, context &ec, instruction_data *data)
    {
      address_register ea1(op & 0x7, 2);
      int value2 = op >> 9 & 0x7;
      if (value2 == 0)
	value2 = 8;
#ifdef L
      L(" addql #%d", value2);
      L(",%s\n", ea1.textl(ec));
#endif

      int32 value1 = ea1.getl(ec);
      int32 value = extsl(value1 + value2);
      ea1.putl(ec, value);
      ea1.finishl(ec);
      // XXX: The condition codes are not affected.

      ec.regs.pc += 2 + ea1.isize(4);
    }

  template <class Source> void
  andb(uint_type op, context &c, instruction_data *data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef L
    L(" andb %s", ea1.textb(c));
    L(",%%d%u\n", reg2);
#endif

    sint_type value1 = ea1.getb(c);
    sint_type value2 = extsb(c.regs.d[reg2]);
    sint_type value = extsb(uint_type(value2) & uint_type(value1));
    const uint32_type MASK = 0xffu;
    c.regs.d[reg2] = c.regs.d[reg2] & ~MASK | uint32_type(value) & MASK;
    c.regs.sr.set_cc(value);
    ea1.finishb(c);

    c.regs.pc += 2 + ea1.isize(2);
  }

  template <class Source> void
  andw(uint_type op, context &ec, instruction_data *data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef L
    L(" andw %s", ea1.textw(ec));
    L(",%%d%u\n", reg2);
#endif

    sint_type value1 = ea1.getw(ec);
    sint_type value2 = extsw(ec.regs.d[reg2]);
    sint_type value = extsw(uint_type(value2) & uint_type(value1));
    const uint32_type MASK = 0xffffu;
    ec.regs.d[reg2] = ec.regs.d[reg2] & ~MASK | uint32_type(value) & MASK;
    ec.regs.sr.set_cc(value);
    ea1.finishw(ec);

    ec.regs.pc += 2 + ea1.isize(2);
  }

  template <class Source> void
  andl(unsigned int op, context &ec, instruction_data *data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef L
    L(" andl %s", ea1.textl(ec));
    L(",%%d%u\n", reg2);
#endif

    sint32_type value1 = ea1.getl(ec);
    sint32_type value2 = extsl(ec.regs.d[reg2]);
    sint32_type value = extsl(uint32_type(value2) & uint32_type(value1));
    ec.regs.d[reg2] = value;
    ec.regs.sr.set_cc(value);
    ea1.finishl(ec);

    ec.regs.pc += 2 + ea1.isize(4);
  }

#if 0
  void andl_i_d(unsigned int op, context &ec, instruction_data *data)
    {
      int reg1 = op >> 9 & 0x7;
      uint32 val2 = ec.fetchl(2);
      VL((" andl #0x%lx,%%d%d\n", (unsigned long) val2, reg1));

      uint32 val = ec.regs.d[reg1] & val2;
      ec.regs.d[reg1] = val;
      ec.regs.sr.set_cc(val);

      ec.regs.pc += 2 + 4;
    }
#endif

  template <class Destination> void
  andiw(unsigned int op, context &ec, instruction_data *data)
  {
    sint_type value2 = extsw(ec.fetchw(2));
    Destination ea1(op & 0x7, 2 + 2);
#ifdef L
    L(" andiw #0x%x", uint_type(value2));
    L(",%s\n", ea1.textw(ec));
#endif

    sint_type value1 = ea1.getw(ec);
    sint_type value = extsw(uint_type(value1) & uint_type(value2));
    ea1.putw(ec, value);
    ec.regs.sr.set_cc(value);
    ea1.finishw(ec);

    ec.regs.pc += 2 + 2 + ea1.isize(2);
  }

  template <class Destination> void
  andil(unsigned int op, context &ec, instruction_data *data)
  {
    sint32_type value2 = extsl(ec.fetchl(2));
    Destination ea1(op & 0x7, 2 + 4);
#ifdef L
    L(" andil #0x%lx", (unsigned long) value2);
    L(",%s\n", ea1.textl(ec));
#endif

    sint32_type value1 = ea1.getl(ec);
    sint32_type value = extsl(uint32_type(value1) & uint32_type(value2));
    ea1.putl(ec, value);
    ec.regs.sr.set_cc(value);
    ea1.finishl(ec);

    ec.regs.pc += 2 + 4 + ea1.isize(4);
  }

  void
  asll_i(uint_type op, context &ec, instruction_data *data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int count = op >> 9 & 0x7;
#ifdef L
    L(" asll #%u", count);
    L(",%%d%u\n", reg1);
#endif

    sint32_type value1 = extsl(ec.regs.d[reg1]);
    sint32_type value = extsl(value1 << count);
    ec.regs.d[reg1] = value;
    ec.regs.sr.set_cc(value);	// FIXME.

    ec.regs.pc += 2;
  }

  void
  asll_r(unsigned int op, context &ec, instruction_data *data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef L
    L(" asll %%d%u", reg2);
    L(",%%d%u\n", reg1);
#endif

    uint_type count = ec.regs.d[reg2];
    sint32_type value1 = extsl(ec.regs.d[reg1]);
    sint32_type value = extsl(value1 << count);
    ec.regs.d[reg1] = value;
    ec.regs.sr.set_cc(value);	// FIXME.

    ec.regs.pc += 2;
  }

  void
  asrl_r(unsigned int op, context &ec, instruction_data *data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef L
    L(" asrl %%d%u", reg2);
    L(",%%d%u\n", reg1);
#endif

    uint_type count = ec.regs.d[reg2];
    sint32_type value1 = extsl(ec.regs.d[reg1]);
    sint32_type value = value1 >> count;
    ec.regs.d[reg1] = value;
    ec.regs.sr.set_cc_asr(value, value1, count);

    ec.regs.pc += 2;
  }

  template <class Condition> void 
  b(unsigned int op, context &ec, instruction_data *data)
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
    L(" b%s 0x%lx\n", cond.text(), (unsigned long) (ec.regs.pc + 2 + disp));
#endif

    // XXX: The condition codes are not affected by this instruction.
    ec.regs.pc += 2 + (cond(ec) ? disp : len);
  }

#if 0
  void bcc(unsigned int op, context &ec, instruction_data *data)
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

  void beq(unsigned int op, context &ec, instruction_data *data)
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

#if 0
  void bge(unsigned int op, context &ec, instruction_data *data)
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
	   context &ec, instruction_data *data)
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
      L(" bmi 0x%lx\n", (unsigned long) (ec.regs.pc + 2 + disp));
#endif

      // XXX: The condition codes are not affected.
      ec.regs.pc += ec.regs.sr.mi() ? 2 + disp : len;
    }
#endif

  void bne(unsigned int op, context &ec, instruction_data *data)
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

  void
  bclrl_i(unsigned int op, context &ec, instruction_data *data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int bit = ec.fetchw(2) & 0x1f;
#ifdef L
    L(" bclrl #%u", bit);
    L(",%%d%u\n", reg1);
#endif

    // This instruction affects only the Z bit of condition codes.
    uint32_type mask = uint32_type(1) << bit;
    uint32_type value1 = ec.regs.d[reg1];
    bool value = value1 & mask;
    ec.regs.d[reg1] = value1 & ~mask;
    ec.regs.sr.set_cc(value);	// FIXME.

    ec.regs.pc += 2 + 2;
  }

  void
  bra(unsigned int op, context &ec, instruction_data *data)
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

  void
  bsetl_i(uint_type op, context &ec, instruction_data *data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int bit = ec.fetchw(2) & 0x1f;
#ifdef L
    L(" bsetl #%u", bit);
    L(",%%d%u\n", reg1);
#endif

    // This instruction affects only the Z bit of condition codes.
    uint32_type mask = uint32_type(1) << bit;
    uint32_type value1 = ec.regs.d[reg1];
    bool value = value1 & mask;
    ec.regs.d[reg1] = value1 | mask;
    ec.regs.sr.set_cc(value);	// FIXME.

    ec.regs.pc += 2 + 2;
  }

  void
  bsr(unsigned int op, context &ec, instruction_data *data)
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

  template <class Destination> void
  btstb_i(uint_type op, context &c, instruction_data *data)
  {
    unsigned int bit = c.fetchw(2) & 0x7;
    Destination ea1(op & 0x7, 2 + 2);
#ifdef L
    L(" btstb #%u", bit);
    L(",%s\n", ea1.textb(c));
#endif

    // This instruction affects only the Z bit of condition codes.
    bool value = uint_type(ea1.getb(c)) & 1u << bit;
    c.regs.sr.set_cc(value);	// FIXME.

    c.regs.pc += 2 + 2 + ea1.isize(2);
  }

  void
  btstl_i(unsigned int op, context &ec, instruction_data *data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int bit = ec.fetchw(2) & 0x1f;
#ifdef L
    L(" btstl #%u", bit);
    L(",%%d%u\n", reg1);
#endif

    // This instruction affects only the Z bit of condition codes.
    bool value = ec.regs.d[reg1] & uint32_type(1) << bit;
    ec.regs.sr.set_cc(value);	// FIXME.

    ec.regs.pc += 2 + 2;
  }

  template <class Destination> void
  clrb(unsigned int op, context &ec, instruction_data *data)
    {
      Destination ea1(op & 0x7, 2);
      VL((" clrb %s\n", ea1.textb(ec)));

      ea1.putb(ec, 0);
      ea1.finishb(ec);
      ec.regs.sr.set_cc(0);

      ec.regs.pc += 2 + ea1.isize(2);
    }

  template <class Destination> void
  clrw(unsigned int op, context &ec, instruction_data *data)
    {
      Destination ea1(op & 0x7, 2);
      VL((" clrw %s\n", ea1.textw(ec)));

      ea1.putw(ec, 0);
      ea1.finishw(ec);
      ec.regs.sr.set_cc(0);

      ec.regs.pc += 2 + ea1.isize(2);
    }

#if 0
  template <> void clrw<address_register>(unsigned int, context &);
  // XXX: Address register cannot be the destination.
#endif

  template <class Destination> void
  clrl(unsigned int op, context &ec, instruction_data *data)
    {
      Destination ea1(op & 0x7, 2);
      VL((" clrl %s\n", ea1.textl(ec)));

      ea1.putl(ec, 0);
      ea1.finishl(ec);
      ec.regs.sr.set_cc(0);

      ec.regs.pc += 2 + ea1.isize(2);
    }

  template <class Source> void
  cmpb(unsigned int op, context &ec, instruction_data *data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef L
    L(" cmpb %s", ea1.textb(ec));
    L(",%%d%u\n", reg2);
#endif

    sint_type value1 = ea1.getb(ec);
    sint_type value2 = extsb(ec.regs.d[reg2]);
    sint_type value = extsb(value2 - value1);
    ec.regs.sr.set_cc_cmp(value, value2, value1);
    ea1.finishb(ec);

    ec.regs.pc += 2 + ea1.isize(2);
  }

  template <class Source> void
  cmpw(unsigned int op, context &ec, instruction_data *data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef L
    L(" cmpw %s", ea1.textw(ec));
    L(",%%d%u\n", reg2);
#endif

    sint_type value1 = ea1.getw(ec);
    sint_type value2 = extsw(ec.regs.d[reg2]);
    sint_type value = extsw(value2 - value1);
    ec.regs.sr.set_cc_cmp(value, value2, value1);
    ea1.finishw(ec);

    ec.regs.pc += 2 + ea1.isize(2);
  }

  template <class Source> void
  cmpl(unsigned int op, context &ec, instruction_data *data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef L
    L(" cmpl %s", ea1.textl(ec));
    L(",%%d%u\n", reg2);
#endif

    sint32_type value1 = ea1.getl(ec);
    sint32_type value2 = extsl(ec.regs.d[reg2]);
    sint32_type value = extsl(value2 - value1);
    ec.regs.sr.set_cc_cmp(value, value2, value1);
    ea1.finishl(ec);

    ec.regs.pc += 2 + ea1.isize(4);
  }

  template <class Source> void
  cmpaw(unsigned int op, context &ec, instruction_data *data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef L
    L(" cmpaw %s", ea1.textw(ec));
    L(",%%a%u\n", reg2);
#endif

    sint32_type value1 = ea1.getw(ec);
    sint32_type value2 = extsl(ec.regs.a[reg2]);
    sint32_type value = extsl(value2 - value1);
    ec.regs.sr.set_cc_cmp(value, value2, value1);
    ea1.finishw(ec);

    ec.regs.pc += 2 + ea1.isize(2);
  }

  template <class Source> void
  cmpal(unsigned int op, context &ec, instruction_data *data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef L
    L(" cmpal %s", ea1.textl(ec));
    L(",%%a%u\n", reg2);
#endif

    sint32_type value1 = ea1.getl(ec);
    sint32_type value2 = extsl(ec.regs.a[reg2]);
    sint32_type value = extsl(value2 - value1);
    ec.regs.sr.set_cc_cmp(value, value2, value1);
    ea1.finishl(ec);

    ec.regs.pc += 2 + ea1.isize(4);
  }

  template <class Destination> void
  cmpib(unsigned int op, context &ec, instruction_data *data)
  {
    sint_type value2 = extsb(ec.fetchw(2));
    Destination ea1(op & 0x7, 2 + 2);
#ifdef L
    L(" cmpib #0x%x", uint_type(value2));
    L(",%s\n", ea1.textb(ec));
#endif

    sint_type value1 = ea1.getb(ec);
    sint_type value = extsb(value1 - value2);
    ec.regs.sr.set_cc_cmp(value, value1, value2);
    ea1.finishb(ec);

    ec.regs.pc += 2 + 2 + ea1.isize(2);
  }

  template <class Destination> void
  cmpiw(unsigned int op, context &ec, instruction_data *data)
  {
    sint_type value2 = extsw(ec.fetchw(2));
    Destination ea1(op & 0x7, 2 + 2);
#ifdef L
    L(" cmpiw #0x%x", uint_type(value2));
    L(",%s\n", ea1.textw(ec));
#endif

    sint_type value1 = ea1.getw(ec);
    sint_type value = extsw(value1 - value2);
    ec.regs.sr.set_cc_cmp(value, value1, value2);
    ea1.finishw(ec);

    ec.regs.pc += 2 + 2 + ea1.isize(2);
  }

  template <class Source> void
  divuw(unsigned int op, context &ec, instruction_data *data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef L
    L(" divuw %s", ea1.textw(ec));
    L(",%%d%u\n", reg2);
#endif

    sint_type value1 = ea1.getw(ec);
    sint32_type value2 = extsw(ec.regs.d[reg2]);
    sint32_type value = uint32_type(value2) / (uint32_type(value1) & 0xffffu);
    sint32_type rem = uint32_type(value2) % (uint32_type(value1) & 0xffffu);
    ec.regs.d[reg2] = uint32_type(rem) << 16 | uint32_type(value) & 0xffffu;
    ec.regs.sr.set_cc(value); // FIXME.
    ea1.finishw(ec);

    ec.regs.pc += 2 + ea1.isize(2);
  }

  template <class Destination> void
  eorb_r(unsigned int op, context &ec, instruction_data *data)
  {
    Destination ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef L
    L(" eorb %%d%u", reg2);
    L(",%s\n", ea1.textb(ec));
#endif

    sint_type value1 = ea1.getb(ec);
    sint_type value2 = extsb(ec.regs.d[reg2]);
    sint_type value = extsb(uint_type(value1) ^ uint_type(value2));
    ea1.putb(ec, value);
    ec.regs.sr.set_cc(value);
    ea1.finishb(ec);

    ec.regs.pc += 2 + ea1.isize(2);
  }

  template <class Destination> void
  eorw_r(unsigned int op, context &ec, instruction_data *data)
  {
    Destination ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef L
    L(" eorw %%d%u", reg2);
    L(",%s\n", ea1.textw(ec));
#endif

    sint_type value1 = ea1.getw(ec);
    sint_type value2 = extsw(ec.regs.d[reg2]);
    sint_type value = extsw(uint_type(value1) ^ uint_type(value2));
    ea1.putw(ec, value);
    ec.regs.sr.set_cc(value);
    ea1.finishw(ec);

    ec.regs.pc += 2 + ea1.isize(2);
  }

  template <class Destination> void
  eoriw(unsigned int op, context &ec, instruction_data *data)
  {
    sint_type value2 = extsw(ec.fetchw(2));
    Destination ea1(op & 0x7, 2 + 2);
#ifdef L
    L(" eoriw #0x%x", uint_type(value2));
    L(",%s\n", ea1.textw(ec));
#endif

    sint_type value1 = ea1.getw(ec);
    sint_type value = extsw(uint_type(value1) ^ uint_type(value2));
    ea1.putw(ec, value);
    ec.regs.sr.set_cc(value);
    ea1.finishw(ec);

    ec.regs.pc += 2 + 2 + ea1.isize(2);
  }

  void
  dbf(unsigned int op, context &ec, instruction_data *data)
  {
    unsigned int reg1 = op & 0x7;
    int disp = extsw(ec.fetchw(2));
#ifdef L
    L(" dbf %%d%d", reg1);
    L(",0x%lx\n", (unsigned long) (ec.regs.pc + 2 + disp));
#endif

    // The condition codes are not affected by this instruction.
    sint_type value = extsw(ec.regs.d[reg1]) - 1;
    const uint32_type MASK = 0xffffu;
    ec.regs.d[reg1] = ec.regs.d[reg1] & ~MASK | uint32_type(value) & MASK;
    ec.regs.pc += 2 + (value != -1 ? disp : 2);
  }

  void
  extl(unsigned int op, context &ec, instruction_data *data)
  {
    unsigned int reg1 = op & 0x7;
#ifdef L
    L(" extl %%d%u\n", reg1);
#endif

    sint32_type value = extsw(ec.regs.d[reg1]);
    ec.regs.d[reg1] = value;
    ec.regs.sr.set_cc(value);

    ec.regs.pc += 2;
  }

  template <class Destination> void
  jsr(unsigned int op, context &ec, instruction_data *data)
    {
      Destination ea1(op & 0x7, 2);
#ifdef L
      L(" jsr %s\n", ea1.textw(ec));
#endif

      // XXX: The condition codes are not affected.
      uint32 address = ea1.address(ec);
      int fc = ec.data_fc();
      ec.mem->putl(fc, ec.regs.a[7] - 4, ec.regs.pc + 2 + ea1.isize(0));
      ec.regs.a[7] -= 4;
      ec.regs.pc = address;
    }

  template <class Destination> void
  lea(unsigned int op, context &ec, instruction_data *data)
    {
      Destination ea1(op & 0x7, 2);
      int reg2 = op >> 9 & 0x7;
#ifdef L
      L(" lea %s", ea1.textw(ec));
      L(",%%a%d\n", reg2);
#endif

      // XXX: The condition codes are not affected.
      uint32 address = ea1.address(ec);
      ec.regs.a[reg2] = address;

      ec.regs.pc += 2 + ea1.isize(0);
    }

  void
  link_a(unsigned int op, context &ec, instruction_data *data)
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

  void
  lslb_i(unsigned int op, context &ec, instruction_data *data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int count = op >> 9 & 0x7;
    if (count == 0)
      count = 8;
#ifdef L
    L(" lslb #%u", count);
    L(",%%d%u\n", reg1);
#endif

    sint_type value1 = extsb(ec.regs.d[reg1]);
    sint_type value = extsb(uint_type(value1) << count);
    const uint32_type MASK = ((uint32) 1u << 8) - 1;
    ec.regs.d[reg1] = ec.regs.d[reg1] & ~MASK | uint32_type(value) & MASK;
    ec.regs.sr.set_cc_lsl(value, value1, count + (32 - 8));

    ec.regs.pc += 2;
  }

  void
  lslw_i(unsigned int op, context &ec, instruction_data *data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int count = op >> 9 & 0x7;
    if (count == 0)
      count = 8;
#ifdef L
    L(" lslw #%u", count);
    L(",%%d%u\n", reg1);
#endif

    sint_type value1 = extsw(ec.regs.d[reg1]);
    sint_type value = extsw(uint_type(value1) << count);
    const uint32_type MASK = ((uint32) 1u << 16) - 1;
    ec.regs.d[reg1] = ec.regs.d[reg1] & ~MASK | uint32_type(value) & MASK;
    ec.regs.sr.set_cc_lsl(value, value1, count + (32 - 16));

    ec.regs.pc += 2;
  }

  void
  lslw_r(unsigned int op, context &ec, instruction_data *data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef L
    L(" lslw %%d%u", reg2);
    L(",%%d%u\n", reg1);
#endif

    uint_type count = ec.regs.d[reg2];
    sint_type value1 = extsw(ec.regs.d[reg1]);
    sint_type value = extsw(uint_type(value1) << count);
    const uint32_type MASK = ((uint32) 1u << 16) - 1;
    ec.regs.d[reg1] = ec.regs.d[reg1] & ~MASK | uint32_type(value) & MASK;
    ec.regs.sr.set_cc_lsl(value, value1, count + (32 - 16));

    ec.regs.pc += 2;
  }

  void
  lsll_i(unsigned int op, context &ec, instruction_data *data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int count = op >> 9 & 0x7;
    if (count == 0)
      count = 8;
#ifdef L
    L(" lsll #%u", count);
    L(",%%d%u\n", reg1);
#endif

    sint32_type value1 = extsl(ec.regs.d[reg1]);
    sint32_type value = extsl(uint32_type(value1) << count);
    ec.regs.d[reg1] = value;
    ec.regs.sr.set_cc_lsl(value, value1, count);

    ec.regs.pc += 2;
  }

  void
  lsll_r(unsigned int op, context &ec, instruction_data *data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef L
    L(" lsll %%d%u", reg2);
    L(",%%d%u\n", reg1);
#endif

    uint_type count = ec.regs.d[reg2];
    sint32_type value1 = extsl(ec.regs.d[reg1]);
    sint32_type value = extsl(uint_type(value1) << count);
    ec.regs.d[reg1] = value;
    ec.regs.sr.set_cc_lsl(value, value1, count);

    ec.regs.pc += 2;
  }

  void
  lsrw_i(unsigned int op, context &ec, instruction_data *data)
  {
    unsigned int reg1 = op & 0x7;
    uint_type count = op >> 9 & 0x7;
    if (count == 0)
      count = 8;
#ifdef L
    L(" lsrw #%u", count);
    L(",%%d%u\n", reg1);
#endif

    sint_type value1 = extsw(ec.regs.d[reg1]);
    sint_type value = extsw((uint_type(value1) & 0xffffu) >> count);
    const uint32_type MASK = (uint32_type(1) << 16) - 1;
    ec.regs.d[reg1] = ec.regs.d[reg1] & ~MASK | uint32_type(value) & MASK;
    ec.regs.sr.set_cc_lsr(value, value1, count);

    ec.regs.pc += 2;
  }

  void
  lsrw_r(unsigned int op, context &ec, instruction_data *data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef L
    L(" lsrw %%d%u", reg2);
    L(",%%d%u\n", reg1);
#endif

    uint_type count = ec.regs.d[reg2];
    sint_type value1 = extsw(ec.regs.d[reg1]);
    sint_type value = extsw((uint_type(value1) & 0xffffu) >> count);
    const uint32_type MASK = (uint32_type(1) << 16) - 1;
    ec.regs.d[reg1] = ec.regs.d[reg1] & ~MASK | uint32_type(value) & MASK;
    ec.regs.sr.set_cc_lsr(value, value1, count);

    ec.regs.pc += 2;
  }

  void
  lsrl_i(unsigned int op, context &ec, instruction_data *data)
  {
    unsigned int reg1 = op & 0x7;
    uint_type count = op >> 9 & 0x7;
    if (count == 0)
      count = 8;
#ifdef L
    L(" lsrl #%u", count);
    L(",%%d%u\n", reg1);
#endif

    sint32_type value1 = extsl(ec.regs.d[reg1]);
    sint32_type value = extsl((uint32_type(value1) & 0xffffffffu) >> count);
    ec.regs.d[reg1] = value;
    ec.regs.sr.set_cc_lsr(value, value1, count);

    ec.regs.pc += 2;
  }

  void
  lsrl_r(unsigned int op, context &ec, instruction_data *data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef L
    L(" lsrl %%d%u", reg2);
    L(",%%d%u\n", reg1);
#endif

    uint_type count = ec.regs.d[reg2];
    sint32_type value1 = extsl(ec.regs.d[reg1]);
    sint32_type value = extsl((uint32_type(value1) & 0xffffffffu) >> count);
    ec.regs.d[reg1] = value;
    ec.regs.sr.set_cc_lsr(value, value1, count);

    ec.regs.pc += 2;
  }

  template <class Source, class Destination> void
  moveb(unsigned int op, context &ec, instruction_data *data)
  {
    Source ea1(op & 0x7, 2);
    Destination ea2(op >> 9 & 0x7, 2 + ea1.isize(2));
#ifdef L
    L(" moveb %s", ea1.textb(ec));
    L(",%s\n", ea2.textb(ec));
#endif

    sint_type value = ea1.getb(ec);
    ea2.putb(ec, value);
    ec.regs.sr.set_cc(value);
    ea1.finishb(ec);
    ea2.finishb(ec);

    ec.regs.pc += 2 + ea1.isize(2) + ea2.isize(2);
  }

#if 0
  void moveb_d_postinc(unsigned int op, context &ec, instruction_data *data)
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

  void moveb_postinc_postinc(unsigned int op, context &ec, instruction_data *data)
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
#endif

  template <class Source, class Destination> void
  movew(unsigned int op, context &ec, instruction_data *data)
  {
    Source ea1(op & 0x7, 2);
    Destination ea2(op >> 9 & 0x7, 2 + ea1.isize(2));
#ifdef L
    L(" movew %s", ea1.textw(ec));
    L(",%s\n", ea2.textw(ec));
#endif

    sint_type value = ea1.getw(ec);
    ea2.putw(ec, value);
    ec.regs.sr.set_cc(value);
    ea1.finishw(ec);
    ea2.finishw(ec);

    ec.regs.pc += 2 + ea1.isize(2) + ea2.isize(2);
  }

#if 0
  void movew_d_predec(unsigned int op, context &ec, instruction_data *data)
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

  void movew_absl_predec(unsigned int op, context &ec, instruction_data *data)
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

  void movew_d_absl(unsigned int op, context &ec, instruction_data *data)
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
#endif

  template <class Source, class Destination> void
  movel(unsigned int op, context &ec, instruction_data *data)
    {
      Source ea1(op & 0x7, 2);
      Destination ea2(op >> 9 & 0x7, 2 + ea1.isize(4));
      VL((" movel %s", ea1.textl(ec)));
      VL((",%s\n", ea2.textl(ec)));

      int32 value = ea1.getl(ec);
      ea2.putl(ec, value);
      ea1.finishl(ec);
      ea2.finishl(ec);
      ec.regs.sr.set_cc(value);

      ec.regs.pc += 2 + ea1.isize(4) + ea2.isize(4);
    }

  template <class Source> void
  moveal(unsigned int op, context &ec, instruction_data *data)
    {
      Source ea1(op & 0x7, 2);
      address_register ea2(op >> 9 & 0x7, 2 + ea1.isize(4));
      VL((" moveal %s", ea1.textl(ec)));
      VL((",%s\n", ea2.textl(ec)));

      // XXX: The condition codes are not affected by this
      // instruction.
      int32 value = ea1.getl(ec);
      ea2.putl(ec, value);
      ea1.finishl(ec);
      ea2.finishl(ec);

      ec.regs.pc += 2 + ea1.isize(4) + ea2.isize(4);
    }

  /* movem regs to EA (postdec).  */
  void
  moveml_r_predec(unsigned int op, context &ec, instruction_data *data)
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
  template <class Source> void
  moveml_mr(unsigned int op, context &ec, instruction_data *data)
    {
      Source ea1(op & 0x7, 4);
      unsigned int bitmap = ec.fetchw(2);
#ifdef L
      L(" moveml %s", ea1.textl(ec));
      L(",#0x%04x\n", bitmap);
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
  template <> void
  moveml_mr<postinc_indirect>(unsigned int op, context &ec,
			      instruction_data *data)
    {
      int reg1 = op & 0x7;
      unsigned int bitmap = ec.fetchw(2);
#ifdef L
      L(" moveml %%a%d@+", reg1);
      L(",#0x%04x\n", bitmap);
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

  void
  moveql_d(unsigned int op, context &ec, instruction_data *data)
    {
      int value = extsb(op & 0xff);
      int reg = op >> 9 & 0x7;
      VL((" moveql #%d,%%d%d\n", value, reg));
      
      ec.regs.d[reg] = value;
      ec.regs.sr.set_cc(value);

      ec.regs.pc += 2;
    }

  template <class Source> void
  mulsw(unsigned int op, context &ec, instruction_data *data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef L
    L(" mulsw %s", ea1.textw(ec));
    L(",%%d%u\n", reg2);
#endif

    sint_type value1 = ea1.getw(ec);
    sint_type value2 = extsw(ec.regs.d[reg2]);
    sint32_type value = sint32_type(value2) * sint32_type(value1);
    ec.regs.d[reg2] = value;
    ec.regs.sr.set_cc(value); // FIXME.
    ea1.finishw(ec);

    ec.regs.pc += 2 + ea1.isize(2);
  }

  template <class Source> void
  muluw(unsigned int op, context &ec, instruction_data *data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef L
    L(" muluw %s", ea1.textw(ec));
    L(",%%d%u\n", reg2);
#endif

    sint_type value1 = ea1.getw(ec);
    sint_type value2 = extsw(ec.regs.d[reg2]);
    sint32_type value
      = (uint32_type(value2) & 0xffffu) * (uint32_type(value1) & 0xffffu);
    ec.regs.d[reg2] = value;
    ec.regs.sr.set_cc(value); // FIXME.
    ea1.finishw(ec);

    ec.regs.pc += 2 + ea1.isize(2);
  }

  template <class Destination> void
  negw(uint_type op, context &ec, instruction_data *data)
  {
    Destination ea1(op & 0x7, 2);
#ifdef L
    L(" negw %s\n", ea1.textw(ec));
#endif

    sint_type value1 = ea1.getw(ec);
    sint_type value = extsw(-value1);
    ea1.putw(ec, value);
    ec.regs.sr.set_cc_sub(value, 0, value1);
    ea1.finishw(ec);

    ec.regs.pc += 2 + ea1.isize(2);
  }

  template <class Destination> void
  negl(unsigned int op, context &ec, instruction_data *data)
  {
    Destination ea1(op & 0x7, 2);
#ifdef L
    L(" negl %s\n", ea1.textl(ec));
#endif

    sint32_type value1 = ea1.getl(ec);
    sint32_type value = extsl(-value1);
    ea1.putl(ec, value);
    ec.regs.sr.set_cc_sub(value, 0, value1);
    ea1.finishl(ec);

    ec.regs.pc += 2 + ea1.isize(4);
  }

  template <class Source> void
  orb(uint_type op, context &ec, instruction_data *data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef L
    L(" orb %s", ea1.textb(ec));
    L(",%%d%u\n", reg2);
#endif

    sint_type value1 = ea1.getb(ec);
    sint_type value2 = extsb(ec.regs.d[reg2]);
    sint_type value = extsb(uint_type(value2) | uint_type(value1));
    ec.regs.d[reg2]
      = ec.regs.d[reg2] & ~0xff | uint32_type(value) & 0xff;
    ec.regs.sr.set_cc(value);
    ea1.finishb(ec);

    ec.regs.pc += 2 + ea1.isize(2);
  }

  template <class Source> void
  orw(unsigned int op, context &ec, instruction_data *data)
  {
    Source ea1(op & 0x7, 2);
    int reg2 = op >> 9 & 0x7;
#ifdef L
    L(" orw %s", ea1.textw(ec));
    L(",%%d%d\n", reg2);
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

  template <class Source> void
  orl(unsigned int op, context &ec, instruction_data *data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef L
    L(" orl %s", ea1.textl(ec));
    L(",%%d%u\n", reg2);
#endif

    sint32_type value1 = ea1.getl(ec);
    sint32_type value2 = extsl(ec.regs.d[reg2]);
    sint32_type value = extsl(uint32_type(value2) | uint32_type(value1));
    ec.regs.d[reg2] = value;
    ec.regs.sr.set_cc(value);
    ea1.finishl(ec);

    ec.regs.pc += 2 + ea1.isize(4);
  }

  template <class Destination> void
  orib(unsigned int op, context &ec, instruction_data *data)
  {
    sint_type value2 = extsb(ec.fetchw(2));
    Destination ea1(op & 0x7, 2 + 2);
#ifdef L
    L(" orib #0x%x", uint_type(value2) & 0xff);
    L(",%s\n", ea1.textb(ec));
#endif

    sint_type value1 = ea1.getb(ec);
    sint_type value = extsb(uint_type(value1) | uint_type(value2));
    ea1.putb(ec, value);
    ec.regs.sr.set_cc(value);
    ea1.finishb(ec);

    ec.regs.pc += 2 + 2 + ea1.isize(2);
  }

  template <class Destination> void
  oriw(uint_type op, context &c, instruction_data *data)
  {
    sint_type value2 = extsw(c.fetchw(2));
    Destination ea1(op & 0x7, 2 + 2);
#ifdef L
    L(" oriw #0x%x", uint_type(value2) & 0xffffu);
    L(",%s\n", ea1.textw(c));
#endif

    sint_type value1 = ea1.getw(c);
    sint_type value = extsw(uint_type(value1) | uint_type(value2));
    ea1.putw(c, value);
    c.regs.sr.set_cc(value);
    ea1.finishw(c);

    c.regs.pc += 2 + 2 + ea1.isize(2);
  }

  template <class Destination> void
  pea(unsigned int op, context &ec, instruction_data *data)
    {
      Destination ea1(op & 0x7, 2);
#ifdef L
      L(" pea %s\n", ea1.textw(ec));
#endif

      // XXX: The condition codes are not affected.
      uint32 address = ea1.address(ec);
      int fc = ec.data_fc();
      ec.mem->putl(fc, ec.regs.a[7] - 4, address);
      ec.regs.a[7] -= 4;

      ec.regs.pc += 2 + ea1.isize(0);
    }

  void
  rolb_r(unsigned int op, context &ec, instruction_data *data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef L
    L(" rolb %%d%u", reg2);
    L(",%%d%u\n", reg1);
#endif

    uint_type count = ec.regs.d[reg2] & 0x7;
    sint_type value1 = extsb(ec.regs.d[reg1]);
    sint_type value = extsb(uint_type(value1) << count
			    | (uint_type(value1) & 0xff) >> 8 - count);
    const uint32_type MASK = 0xff;
    ec.regs.d[reg1] = ec.regs.d[reg1] & ~MASK | uint32_type(value) & MASK;
    ec.regs.sr.set_cc(value);	// FIXME.

    ec.regs.pc += 2;
  }

  void
  rolw_i(uint_type op, context &ec, instruction_data *data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int count = op >> 9 & 0x7;
#ifdef L
    L(" rolw #%u", count);
    L(",%%d%u\n", reg1);
#endif

    sint_type value1 = extsw(ec.regs.d[reg1]);
    sint_type value = extsw(uint_type(value1) << count
			    | (uint_type(value1) & 0xffffu) >> 16 - count);
    const uint32_type MASK = 0xffffu;
    ec.regs.d[reg1] = ec.regs.d[reg1] & ~MASK | uint32_type(value) & MASK;
    ec.regs.sr.set_cc(value);	// FIXME.

    ec.regs.pc += 2;
  }

  void
  rts(unsigned int op, context &ec, instruction_data *data)
    {
      VL((" rts\n"));

      // XXX: The condition codes are not affected.
      int fc = ec.data_fc();
      uint32 value = ec.mem->getl(fc, ec.regs.a[7]);
      ec.regs.a[7] += 4;
      ec.regs.pc = value;
    }

  template <class Condition, class Destination> void
  s_b(unsigned int op, context &ec, instruction_data *data)
  {
    Condition cond;
    Destination ea1(op & 0x7, 2);
#ifdef L
    L(" s%sb %s\n", cond.text(), ea1.textb(ec));
#endif

    // The condition codes are not affected by this instruction.
    ea1.putb(ec, cond(ec) ? -1 : 0);
    ea1.finishb(ec);

    ec.regs.pc += 2 + ea1.isize(2);
  }

  template <class Source> void
  subb(unsigned int op, context &ec, instruction_data *data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef L
    L(" subb %s", ea1.textb(ec));
    L(",%%d%u\n", reg2);
#endif

    sint_type value1 = ea1.getb(ec);
    sint_type value2 = extsb(ec.regs.d[reg2]);
    sint_type value = extsb(value2 - value1);
    const uint32_type MASK = 0xff;
    ec.regs.d[reg2] = ec.regs.d[reg2] & ~MASK | uint32_type(value) & MASK;
    ec.regs.sr.set_cc_sub(value, value2, value1);
    ea1.finishb(ec);

    ec.regs.pc += 2 + ea1.isize(2);
  }

#if 0
  void subb_postinc_d(unsigned int op, context &ec, instruction_data *data)
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
#endif

  template <class Source> void
  subw(unsigned int op, context &ec, instruction_data *data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef L
    L(" subw %s", ea1.textw(ec));
    L(",%%d%u\n", reg2);
#endif

    sint_type value1 = ea1.getw(ec);
    sint_type value2 = extsw(ec.regs.d[reg2]);
    sint_type value = extsw(value2 - value1);
    const uint32_type MASK = 0xffffu;
    ec.regs.d[reg2] = ec.regs.d[reg2] & ~MASK | uint32_type(value) & MASK;
    ec.regs.sr.set_cc_sub(value, value2, value1);
    ea1.finishw(ec);

    ec.regs.pc += 2 + ea1.isize(2);
  }

  template <class Destination> void
  subl(unsigned int op, context &ec, instruction_data *data)
    {
      Destination ea1(op & 0x7, 2);
      int reg2 = op >> 9 & 0x7;
      VL((" subl %s", ea1.textl(ec)));
      VL((",%%d%d\n", reg2));

      int32 value1 = ea1.getl(ec);
      int32 value2 = extsl(ec.regs.d[reg2]);
      int32 value = extsl(value2 - value1);
      ec.regs.d[reg2] = value;
      ea1.finishl(ec);
      ec.regs.sr.set_cc_sub(value, value2, value1);

      ec.regs.pc += 2 + ea1.isize(4);
    }

  template <class Destination> void
  subl_r(unsigned int op, context &ec, instruction_data *data)
  {
    Destination ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef L
    L(" subl %%d%u", reg2);
    L(",%s\n", ea1.textl(ec));
#endif

    sint32_type value2 = extsl(ec.regs.d[reg2]);
    sint32_type value1 = ea1.getl(ec);
    sint32_type value = extsl(value1 - value2);
    ea1.putl(ec, value);
    ec.regs.sr.set_cc_sub(value, value1, value2);
    ea1.finishl(ec);

    ec.regs.pc += 2 + ea1.isize(4);
  }

  template <class Source> void
  subal(unsigned int op, context &ec, instruction_data *data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef L
    L(" subal %s", ea1.textl(ec));
    L(",%%d%u\n", reg2);
#endif

    // The condition codes are not affected by this instruction.
    sint32_type value1 = ea1.getl(ec);
    sint32_type value2 = extsl(ec.regs.a[reg2]);
    sint32_type value = extsl(value2 - value1);
    ec.regs.a[reg2] = value;
    ea1.finishl(ec);

    ec.regs.pc += 2 + ea1.isize(4);
  }

  template <class Destination> void
  subib(unsigned int op, context &ec, instruction_data *data)
    {
      int value2 = extsb(ec.fetchw(2));
      Destination ea1(op & 0x7, 2 + 2);
#ifdef L
      L(" subib #%d", value2);
      L(",%s\n", ea1.textb(ec));
#endif

      int value1 = ea1.getb(ec);
      int value = extsb(value1 - value2);
      ea1.putb(ec, value);
      ea1.finishb(ec);
      ec.regs.sr.set_cc_sub(value, value1, value2);

      ec.regs.pc += 2 + 2 + ea1.isize(2);
    }

  template <class Destination> void
  subil(unsigned int op, context &ec, instruction_data *data)
  {
    sint32_type value2 = extsl(ec.fetchl(2));
    Destination ea1(op & 0x7, 2 + 4);
#ifdef L
    L(" subil #%ld", (long) value2);
    L(",%s\n", ea1.textl(ec));
#endif

    sint32_type value1 = ea1.getl(ec);
    sint32_type value = extsl(value1 - value2);
    ea1.putl(ec, value);
    ec.regs.sr.set_cc_sub(value, value1, value2);
    ea1.finishl(ec);

    ec.regs.pc += 2 + 4 + ea1.isize(4);
  }

  template <class Destination> void
  subqw(unsigned int op, context &ec, instruction_data *data)
  {
    Destination ea1(op & 0x7, 2);
    int value2 = op >> 9 & 0x7;
    if (value2 == 0)
      value2 = 8;
#ifdef L
    L(" subqw #%d", value2);
    L(",%s\n", ea1.textw(ec));
#endif

    sint_type value1 = ea1.getw(ec);
    sint_type value = extsw(value1 - value2);
    ea1.putw(ec, value);
    ec.regs.sr.set_cc_sub(value, value1, value2);
    ea1.finishw(ec);

    ec.regs.pc += 2 + ea1.isize(2);
  }

  template <> void
  subqw<address_register>(unsigned int op, context &ec, instruction_data *data)
  {
    address_register ea1(op & 0x7, 2);
    int value2 = op >> 9 & 0x7;
    if (value2 == 0)
      value2 = 8;
#ifdef L
    L(" subqw #%d", value2);
    L(",%s\n", ea1.textw(ec));
#endif

    // Condition codes are not affected by this instruction.
    sint_type value1 = ea1.getw(ec);
    sint_type value = extsw(value1 - value2);
    ea1.putw(ec, value);
    ea1.finishw(ec);

    ec.regs.pc += 2 + ea1.isize(2);
  }

  template <class Destination> void
  subql(uint_type op, context &ec, instruction_data *data)
  {
    Destination ea1(op & 0x7, 2);
    int value2 = op >> 9 & 0x7;
    if (value2 == 0)
      value2 = 8;
#ifdef L
    L(" subql #%d", value2);
    L(",%s\n", ea1.textl(ec));
#endif

    sint32_type value1 = ea1.getl(ec);
    sint32_type value = extsl(value1 - value2);
    ea1.putl(ec, value);
    ec.regs.sr.set_cc_sub(value, value1, value2);
    ea1.finishl(ec);

    ec.regs.pc += 2 + ea1.isize(4);
  }

  template <> void
  subql<address_register>(uint_type op, context &ec, instruction_data *data)
  {
    address_register ea1(op & 0x7, 2);
    int value2 = op >> 9 & 0x7;
    if (value2 == 0)
      value2 = 8;
#ifdef L
    L(" subql #%d", value2);
    L(",%s\n", ea1.textl(ec));
#endif

    // Condition codes are not affected by this instruction.
    sint32_type value1 = ea1.getl(ec);
    sint32_type value = extsl(value1 - value2);
    ea1.putl(ec, value);
    ea1.finishl(ec);

    ec.regs.pc += 2 + ea1.isize(4);
  }

#if 0
  void
  subql_d(unsigned int op, context &ec, instruction_data *data)
    {
      int reg1 = op & 0x7;
      int val2 = op >> 9 & 0x7;
      if (val2 == 0)
	val2 = 8;
      VL((" subql #%d,%%d%d\n", val2, reg1));

      int32 val1 = extsl(ec.regs.d[reg1]);
      int32 val = extsl(val1 - val2);
      ec.regs.d[reg1] = val;
      ec.regs.sr.set_cc_sub(val, val1, val2);

      ec.regs.pc += 2;
    }

  void
  subql_a(unsigned int op, context &ec, instruction_data *data)
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
#endif

  void
  swapw(unsigned int op, context &ec, instruction_data *data)
  {
    unsigned int reg1 = op & 0x7;
#ifdef L
    L(" swapw %%d%u\n", reg1);
#endif

    sint32_type value1 = extsl(ec.regs.d[reg1]);
    sint32_type value
      = extsl(uint32_type(value1) << 16 | uint32_type(value1) >> 16 & 0xffffu);
    ec.regs.d[reg1] = value;
    ec.regs.sr.set_cc(value);

    ec.regs.pc += 2;
  }

  template <class Destination> void
  tstb(unsigned int op, context &ec, instruction_data *data)
    {
      Destination ea1(op & 0x7, 2);
      VL((" tstb %s\n", ea1.textb(ec)));

      int value = ea1.getb(ec);
      ec.regs.sr.set_cc(value);
      ea1.finishb(ec);

      ec.regs.pc += 2 + ea1.isize(2);
    }

  template <class Destination> void
  tstw(unsigned int op, context &ec, instruction_data *data)
  {
    Destination ea1(op & 0x7, 2);
#ifdef L
    L(" tstw %s\n", ea1.textw(ec));
#endif

    sint_type value = ea1.getw(ec);
    ec.regs.sr.set_cc(value);
    ea1.finishw(ec);

    ec.regs.pc += 2 + ea1.isize(2);
  }

#if 0
  void tstw_d(unsigned int op, context &ec, instruction_data *data)
    {
      int reg = op & 0x7;
      VL((" tstw %%d%d\n", reg));

      int value = extsw(ec.regs.d[reg]);
      ec.regs.sr.set_cc(value);

      ec.regs.pc += 2;
    }
#endif

  template <class Destination> void
  tstl(unsigned int op, context &ec, instruction_data *data)
    {
      Destination ea1(op & 0x7, 2);
      VL((" tstl %s\n", ea1.textl(ec)));

      int32 value = ea1.getl(ec);
      ec.regs.sr.set_cc(value);
      ea1.finishl(ec);

      ec.regs.pc += 2 + ea1.isize(4);
    }

  void
  unlk_a(unsigned int op, context &ec, instruction_data *data)
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
  eu.set_instruction(0x0000, 0x0007, &orib<data_register>);
  eu.set_instruction(0x0010, 0x0007, &orib<indirect>);
  eu.set_instruction(0x0018, 0x0007, &orib<postinc_indirect>);
  eu.set_instruction(0x0020, 0x0007, &orib<predec_indirect>);
  eu.set_instruction(0x0028, 0x0007, &orib<disp_indirect>);
  eu.set_instruction(0x0030, 0x0007, &orib<indexed_indirect>);
  eu.set_instruction(0x0039, 0x0000, &orib<absolute_long>);
  eu.set_instruction(0x0040, 0x0007, &oriw<data_register>);
  eu.set_instruction(0x0050, 0x0007, &oriw<indirect>);
  eu.set_instruction(0x0058, 0x0007, &oriw<postinc_indirect>);
  eu.set_instruction(0x0060, 0x0007, &oriw<predec_indirect>);
  eu.set_instruction(0x0068, 0x0007, &oriw<disp_indirect>);
  eu.set_instruction(0x0070, 0x0007, &oriw<indexed_indirect>);
  eu.set_instruction(0x0079, 0x0000, &oriw<absolute_long>);
  eu.set_instruction(0x0240, 0x0007, &andiw<data_register>);
  eu.set_instruction(0x0250, 0x0007, &andiw<indirect>);
  eu.set_instruction(0x0258, 0x0007, &andiw<postinc_indirect>);
  eu.set_instruction(0x0260, 0x0007, &andiw<predec_indirect>);
  eu.set_instruction(0x0268, 0x0007, &andiw<disp_indirect>);
  eu.set_instruction(0x0270, 0x0007, &andiw<indexed_indirect>);
  eu.set_instruction(0x0279, 0x0000, &andiw<absolute_long>);
  eu.set_instruction(0x0280, 0x0007, &andil<data_register>);
  eu.set_instruction(0x0290, 0x0007, &andil<indirect>);
  eu.set_instruction(0x0298, 0x0007, &andil<postinc_indirect>);
  eu.set_instruction(0x02a0, 0x0007, &andil<predec_indirect>);
  eu.set_instruction(0x02a8, 0x0007, &andil<disp_indirect>);
  eu.set_instruction(0x02b0, 0x0007, &andil<indexed_indirect>);
  eu.set_instruction(0x02b9, 0x0000, &andil<absolute_long>);
  eu.set_instruction(0x0400, 0x0007, &subib<data_register>);
  eu.set_instruction(0x0410, 0x0007, &subib<indirect>);
  eu.set_instruction(0x0418, 0x0007, &subib<postinc_indirect>);
  eu.set_instruction(0x0420, 0x0007, &subib<predec_indirect>);
  eu.set_instruction(0x0428, 0x0007, &subib<disp_indirect>);
  eu.set_instruction(0x0439, 0x0000, &subib<absolute_long>);
  eu.set_instruction(0x0480, 0x0007, &subil<data_register>);
  eu.set_instruction(0x0490, 0x0007, &subil<indirect>);
  eu.set_instruction(0x0498, 0x0007, &subil<postinc_indirect>);
  eu.set_instruction(0x04a0, 0x0007, &subil<predec_indirect>);
  eu.set_instruction(0x04a8, 0x0007, &subil<disp_indirect>);
  eu.set_instruction(0x04b0, 0x0007, &subil<indexed_indirect>);
  eu.set_instruction(0x04b9, 0x0000, &subil<absolute_long>);
  eu.set_instruction(0x0680, 0x0007, &addil<data_register>);
  eu.set_instruction(0x0690, 0x0007, &addil<indirect>);
  eu.set_instruction(0x0698, 0x0007, &addil<postincrement_indirect>);
  eu.set_instruction(0x06a0, 0x0007, &addil<predecrement_indirect>);
  eu.set_instruction(0x0800, 0x0007, &btstl_i);
  eu.set_instruction(0x0810, 0x0007, &btstb_i<indirect>);
  eu.set_instruction(0x0818, 0x0007, &btstb_i<postinc_indirect>);
  eu.set_instruction(0x0820, 0x0007, &btstb_i<predec_indirect>);
  eu.set_instruction(0x0828, 0x0007, &btstb_i<disp_indirect>);
  eu.set_instruction(0x0830, 0x0007, &btstb_i<indexed_indirect>);
  eu.set_instruction(0x0839, 0x0000, &btstb_i<absolute_long>);
  eu.set_instruction(0x0880, 0x0007, &bclrl_i);
  eu.set_instruction(0x08c0, 0x0007, &bsetl_i);
  eu.set_instruction(0x0a40, 0x0007, &eoriw<data_register>);
  eu.set_instruction(0x0a50, 0x0007, &eoriw<indirect>);
  eu.set_instruction(0x0a58, 0x0007, &eoriw<postinc_indirect>);
  eu.set_instruction(0x0a60, 0x0007, &eoriw<predec_indirect>);
  eu.set_instruction(0x0a68, 0x0007, &eoriw<disp_indirect>);
  eu.set_instruction(0x0a79, 0x0000, &eoriw<absolute_long>);
  eu.set_instruction(0x0c00, 0x0007, &cmpib<data_register>);
  eu.set_instruction(0x0c10, 0x0007, &cmpib<indirect>);
  eu.set_instruction(0x0c18, 0x0007, &cmpib<postinc_indirect>);
  eu.set_instruction(0x0c20, 0x0007, &cmpib<predec_indirect>);
  eu.set_instruction(0x0c28, 0x0007, &cmpib<disp_indirect>);
  eu.set_instruction(0x0c39, 0x0000, &cmpib<absolute_long>);
  eu.set_instruction(0x0c40, 0x0007, &cmpiw<data_register>);
  eu.set_instruction(0x0c50, 0x0007, &cmpiw<indirect>);
  eu.set_instruction(0x0c58, 0x0007, &cmpiw<postinc_indirect>);
  eu.set_instruction(0x0c60, 0x0007, &cmpiw<predec_indirect>);
  eu.set_instruction(0x0c68, 0x0007, &cmpiw<disp_indirect>);
  eu.set_instruction(0x0c70, 0x0007, &cmpiw<indexed_indirect>);
  eu.set_instruction(0x0c79, 0x0000, &cmpiw<absolute_long>);
  eu.set_instruction(0x1000, 0x0e07, &moveb<data_register, data_register>);
  eu.set_instruction(0x1010, 0x0e07, &moveb<indirect, data_register>);
  eu.set_instruction(0x1018, 0x0e07, &moveb<postinc_indirect, data_register>);
  eu.set_instruction(0x1020, 0x0e07, &moveb<predec_indirect, data_register>);
  eu.set_instruction(0x1028, 0x0e07, &moveb<disp_indirect, data_register>);
  eu.set_instruction(0x1030, 0x0e07, &moveb<indexed_indirect, data_register>);
  eu.set_instruction(0x1039, 0x0e00, &moveb<absolute_long, data_register>);
  eu.set_instruction(0x103a, 0x0e00, &moveb<disp_pc, data_register>);
  eu.set_instruction(0x103b, 0x0e00, &moveb<indexed_pc_indirect, data_register>);
  eu.set_instruction(0x103c, 0x0e00, &moveb<immediate, data_register>);
  eu.set_instruction(0x1080, 0x0e07, &moveb<data_register, indirect>);
  eu.set_instruction(0x1090, 0x0e07, &moveb<indirect, indirect>);
  eu.set_instruction(0x1098, 0x0e07, &moveb<postinc_indirect, indirect>);
  eu.set_instruction(0x10a0, 0x0e07, &moveb<predec_indirect, indirect>);
  eu.set_instruction(0x10a8, 0x0e07, &moveb<disp_indirect, indirect>);
  eu.set_instruction(0x10b9, 0x0e00, &moveb<absolute_long, indirect>);
  eu.set_instruction(0x10ba, 0x0e00, &moveb<disp_pc, indirect>);
  eu.set_instruction(0x10bc, 0x0e00, &moveb<immediate, indirect>);
  eu.set_instruction(0x10c0, 0x0e07, &moveb<data_register, postinc_indirect>);
  eu.set_instruction(0x10d0, 0x0e07, &moveb<indirect, postinc_indirect>);
  eu.set_instruction(0x10d8, 0x0e07, &moveb<postinc_indirect, postinc_indirect>);
  eu.set_instruction(0x10e0, 0x0e07, &moveb<predec_indirect, postinc_indirect>);
  eu.set_instruction(0x10e8, 0x0e07, &moveb<disp_indirect, postinc_indirect>);
  eu.set_instruction(0x10f0, 0x0e07, &moveb<indexed_indirect, postinc_indirect>);
  eu.set_instruction(0x10f9, 0x0e00, &moveb<absolute_long, postinc_indirect>);
  eu.set_instruction(0x10fa, 0x0e00, &moveb<disp_pc, postinc_indirect>);
  eu.set_instruction(0x10fc, 0x0e00, &moveb<immediate, postinc_indirect>);
  eu.set_instruction(0x1100, 0x0e07, &moveb<data_register, predec_indirect>);
  eu.set_instruction(0x1110, 0x0e07, &moveb<indirect, predec_indirect>);
  eu.set_instruction(0x1118, 0x0e07, &moveb<postinc_indirect, predec_indirect>);
  eu.set_instruction(0x1120, 0x0e07, &moveb<predec_indirect, predec_indirect>);
  eu.set_instruction(0x1128, 0x0e07, &moveb<disp_indirect, predec_indirect>);
  eu.set_instruction(0x1139, 0x0e00, &moveb<absolute_long, predec_indirect>);
  eu.set_instruction(0x113a, 0x0e00, &moveb<disp_pc, predec_indirect>);
  eu.set_instruction(0x113c, 0x0e00, &moveb<immediate, predec_indirect>);
  eu.set_instruction(0x1140, 0x0e07, &moveb<data_register, disp_indirect>);
  eu.set_instruction(0x1150, 0x0e07, &moveb<indirect, disp_indirect>);
  eu.set_instruction(0x1158, 0x0e07, &moveb<postinc_indirect, disp_indirect>);
  eu.set_instruction(0x1160, 0x0e07, &moveb<predec_indirect, disp_indirect>);
  eu.set_instruction(0x1168, 0x0e07, &moveb<disp_indirect, disp_indirect>);
  eu.set_instruction(0x1170, 0x0e07, &moveb<indexed_indirect, disp_indirect>);
  eu.set_instruction(0x1179, 0x0e00, &moveb<absolute_long, disp_indirect>);
  eu.set_instruction(0x117a, 0x0e00, &moveb<disp_pc, disp_indirect>);
  eu.set_instruction(0x117c, 0x0e00, &moveb<immediate, disp_indirect>);
  eu.set_instruction(0x1180, 0x0e07, &moveb<data_register, indexed_indirect>);
  eu.set_instruction(0x1190, 0x0e07, &moveb<indirect, indexed_indirect>);
  eu.set_instruction(0x1198, 0x0e07, &moveb<postinc_indirect, indexed_indirect>);
  eu.set_instruction(0x11a0, 0x0e07, &moveb<predec_indirect, indexed_indirect>);
  eu.set_instruction(0x11a8, 0x0e07, &moveb<disp_indirect, indexed_indirect>);
  eu.set_instruction(0x11b0, 0x0e07, &moveb<indexed_indirect, indexed_indirect>);
  eu.set_instruction(0x11b9, 0x0e00, &moveb<absolute_long, indexed_indirect>);
  eu.set_instruction(0x11bc, 0x0e00, &moveb<immediate, indexed_indirect>);
  eu.set_instruction(0x13c0, 0x0007, &moveb<data_register, absolute_long>);
  eu.set_instruction(0x2000, 0x0e07, &movel<data_register, data_register>);
  eu.set_instruction(0x2008, 0x0e07, &movel<address_register, data_register>);
  eu.set_instruction(0x2010, 0x0e07, &movel<indirect, data_register>);
  eu.set_instruction(0x2018, 0x0e07, &movel<postincrement_indirect, data_register>);
  eu.set_instruction(0x2020, 0x0e07, &movel<predecrement_indirect, data_register>);
  eu.set_instruction(0x2028, 0x0e07, &movel<disp_indirect, data_register>);
  eu.set_instruction(0x2039, 0x0e00, &movel<absolute_long, data_register>);
  eu.set_instruction(0x203a, 0x0e00, &movel<disp_pc, data_register>);
  eu.set_instruction(0x203c, 0x0e00, &movel<immediate, data_register>);
  eu.set_instruction(0x2040, 0x0e07, &moveal<data_register>);
  eu.set_instruction(0x2048, 0x0e07, &moveal<address_register>);
  eu.set_instruction(0x2050, 0x0e07, &moveal<indirect>);
  eu.set_instruction(0x2058, 0x0e07, &moveal<postinc_indirect>);
  eu.set_instruction(0x2060, 0x0e07, &moveal<predec_indirect>);
  eu.set_instruction(0x2068, 0x0e07, &moveal<disp_indirect>);
  eu.set_instruction(0x2079, 0x0e00, &moveal<absolute_long>);
  eu.set_instruction(0x207a, 0x0e00, &moveal<disp_pc>);
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
  eu.set_instruction(0x2170, 0x0e07, &movel<indexed_indirect, disp_indirect>);
  eu.set_instruction(0x2179, 0x0e00, &movel<absolute_long, disp_indirect>);
  eu.set_instruction(0x217c, 0x0e00, &movel<immediate, disp_indirect>);
  eu.set_instruction(0x2180, 0x0e07, &movel<data_register, indexed_indirect>);
  eu.set_instruction(0x2188, 0x0e07, &movel<address_register, indexed_indirect>);
  eu.set_instruction(0x2190, 0x0e07, &movel<indirect, indexed_indirect>);
  eu.set_instruction(0x2198, 0x0e07, &movel<postinc_indirect, indexed_indirect>);
  eu.set_instruction(0x21a0, 0x0e07, &movel<predec_indirect, indexed_indirect>);
  eu.set_instruction(0x21a8, 0x0e07, &movel<disp_indirect, indexed_indirect>);
  eu.set_instruction(0x21b0, 0x0e07, &movel<indexed_indirect, indexed_indirect>);
  eu.set_instruction(0x21b9, 0x0e00, &movel<absolute_long, indexed_indirect>);
  eu.set_instruction(0x21bc, 0x0e00, &movel<immediate, indexed_indirect>);
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
  eu.set_instruction(0x3018, 0x0e07, &movew<postinc_indirect, data_register>);
  eu.set_instruction(0x3020, 0x0e07, &movew<predec_indirect, data_register>);
  eu.set_instruction(0x3028, 0x0e07, &movew<disp_indirect, data_register>);
  eu.set_instruction(0x3030, 0x0e07, &movew<indexed_indirect, data_register>);
  eu.set_instruction(0x3039, 0x0e00, &movew<absolute_long, data_register>);
  eu.set_instruction(0x303a, 0x0e00, &movew<disp_pc, data_register>);
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
  eu.set_instruction(0x3100, 0x0e07, &movew<data_register, predec_indirect>);
  eu.set_instruction(0x3108, 0x0e07, &movew<address_register, predec_indirect>);
  eu.set_instruction(0x3110, 0x0e07, &movew<indirect, predec_indirect>);
  eu.set_instruction(0x3118, 0x0e07, &movew<postinc_indirect, predec_indirect>);
  eu.set_instruction(0x3120, 0x0e07, &movew<predec_indirect, predec_indirect>);
  eu.set_instruction(0x3128, 0x0e07, &movew<disp_indirect, predec_indirect>);
  eu.set_instruction(0x3139, 0x0e00, &movew<absolute_long, predec_indirect>);
  eu.set_instruction(0x313a, 0x0e00, &movew<disp_pc, predec_indirect>);
  eu.set_instruction(0x313c, 0x0e00, &movew<immediate, predec_indirect>);
  eu.set_instruction(0x3140, 0x0e07, &movew<data_register, disp_indirect>);
  eu.set_instruction(0x3148, 0x0e07, &movew<address_register, disp_indirect>);
  eu.set_instruction(0x3150, 0x0e07, &movew<indirect, disp_indirect>);
  eu.set_instruction(0x3158, 0x0e07, &movew<postinc_indirect, disp_indirect>);
  eu.set_instruction(0x3160, 0x0e07, &movew<predec_indirect, disp_indirect>);
  eu.set_instruction(0x3168, 0x0e07, &movew<disp_indirect, disp_indirect>);
  eu.set_instruction(0x3179, 0x0e00, &movew<absolute_long, disp_indirect>);
  eu.set_instruction(0x317a, 0x0e00, &movew<disp_pc, disp_indirect>);
  eu.set_instruction(0x317c, 0x0e00, &movew<immediate, disp_indirect>);
  eu.set_instruction(0x3180, 0x0e07, &movew<data_register, indexed_indirect>);
  eu.set_instruction(0x3188, 0x0e07, &movew<address_register, indexed_indirect>);
  eu.set_instruction(0x3190, 0x0e07, &movew<indirect, indexed_indirect>);
  eu.set_instruction(0x3198, 0x0e07, &movew<postinc_indirect, indexed_indirect>);
  eu.set_instruction(0x31a0, 0x0e07, &movew<predec_indirect, indexed_indirect>);
  eu.set_instruction(0x31a8, 0x0e07, &movew<disp_indirect, indexed_indirect>);
  eu.set_instruction(0x31b0, 0x0e07, &movew<indexed_indirect, indexed_indirect>);
  eu.set_instruction(0x31b9, 0x0e00, &movew<absolute_long, indexed_indirect>);
  eu.set_instruction(0x31bc, 0x0e00, &movew<immediate, indexed_indirect>);
  eu.set_instruction(0x33c0, 0x0007, &movew<data_register, absolute_long>);
  eu.set_instruction(0x33c8, 0x0007, &movew<address_register, absolute_long>);
  eu.set_instruction(0x33d0, 0x0007, &movew<indirect, absolute_long>);
  eu.set_instruction(0x33d8, 0x0007, &movew<postinc_indirect, absolute_long>);
  eu.set_instruction(0x33e0, 0x0007, &movew<predec_indirect, absolute_long>);
  eu.set_instruction(0x33e8, 0x0007, &movew<disp_indirect, absolute_long>);
  eu.set_instruction(0x33f0, 0x0007, &movew<indexed_indirect, absolute_long>);
  eu.set_instruction(0x33f9, 0x0000, &movew<absolute_long, absolute_long>);
  eu.set_instruction(0x33fc, 0x0000, &movew<immediate, absolute_long>);
  eu.set_instruction(0x4190, 0x0e07, &lea<indirect>);
  eu.set_instruction(0x41e8, 0x0e07, &lea<disp_indirect>);
  eu.set_instruction(0x41f0, 0x0e07, &lea<indexed_indirect>);
  eu.set_instruction(0x41f9, 0x0e00, &lea<absolute_long>);
  eu.set_instruction(0x41fa, 0x0e00, &lea<disp_pc>);
  eu.set_instruction(0x41fb, 0x0e00, &lea<indexed_pc_indirect>);
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
  eu.set_instruction(0x4440, 0x0007, &negw<data_register>);
  eu.set_instruction(0x4450, 0x0007, &negw<indirect>);
  eu.set_instruction(0x4458, 0x0007, &negw<postinc_indirect>);
  eu.set_instruction(0x4460, 0x0007, &negw<predec_indirect>);
  eu.set_instruction(0x4468, 0x0007, &negw<disp_indirect>);
  eu.set_instruction(0x4470, 0x0007, &negw<indexed_indirect>);
  eu.set_instruction(0x4479, 0x0000, &negw<absolute_long>);
  eu.set_instruction(0x4480, 0x0007, &negl<data_register>);
  eu.set_instruction(0x4490, 0x0007, &negl<indirect>);
  eu.set_instruction(0x4498, 0x0007, &negl<postinc_indirect>);
  eu.set_instruction(0x44a0, 0x0007, &negl<predec_indirect>);
  eu.set_instruction(0x44a8, 0x0007, &negl<disp_indirect>);
  eu.set_instruction(0x44b0, 0x0007, &negl<indexed_indirect>);
  eu.set_instruction(0x44b9, 0x0000, &negl<absolute_long>);
  eu.set_instruction(0x4840, 0x0007, &swapw);
  eu.set_instruction(0x4850, 0x0007, &pea<indirect>);
  eu.set_instruction(0x4868, 0x0007, &pea<disp_indirect>);
  eu.set_instruction(0x4878, 0x0000, &pea<absolute_short>);
  eu.set_instruction(0x4879, 0x0000, &pea<absolute_long>);
  eu.set_instruction(0x487a, 0x0000, &pea<disp_pc>);
  eu.set_instruction(0x48c0, 0x0007, &extl);
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
  eu.set_instruction(0x4a40, 0x0007, &tstw<data_register>);
  eu.set_instruction(0x4a50, 0x0007, &tstw<indirect>);
  eu.set_instruction(0x4a58, 0x0007, &tstw<postinc_indirect>);
  eu.set_instruction(0x4a60, 0x0007, &tstw<predec_indirect>);
  eu.set_instruction(0x4a68, 0x0007, &tstw<disp_indirect>);
  eu.set_instruction(0x4a70, 0x0007, &tstw<indexed_indirect>);
  eu.set_instruction(0x4a79, 0x0000, &tstw<absolute_long>);
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
  eu.set_instruction(0x5018, 0x0e07, &addqb<postinc_indirect>);
  eu.set_instruction(0x5020, 0x0e07, &addqb<predec_indirect>);
  eu.set_instruction(0x5028, 0x0e07, &addqb<disp_indirect>);
  eu.set_instruction(0x5030, 0x0e07, &addqb<indexed_indirect>);
  eu.set_instruction(0x5039, 0x0e00, &addqb<absolute_long>);
  eu.set_instruction(0x5040, 0x0e07, &addqw<data_register>);
  eu.set_instruction(0x5048, 0x0e07, &addqw<address_register>);
  eu.set_instruction(0x5050, 0x0e07, &addqw<indirect>);
  eu.set_instruction(0x5058, 0x0e07, &addqw<postinc_indirect>);
  eu.set_instruction(0x5060, 0x0e07, &addqw<predec_indirect>);
  eu.set_instruction(0x5068, 0x0e07, &addqw<disp_indirect>);
  eu.set_instruction(0x5070, 0x0e07, &addqw<indexed_indirect>);
  eu.set_instruction(0x5079, 0x0e00, &addqw<absolute_long>);
  eu.set_instruction(0x5080, 0x0e07, &addql<data_register>);
  eu.set_instruction(0x5088, 0x0e07, &addql<address_register>);
  eu.set_instruction(0x5090, 0x0e07, &addql<indirect>);
  eu.set_instruction(0x5098, 0x0e07, &addql<postinc_indirect>);
  eu.set_instruction(0x50a0, 0x0e07, &addql<predec_indirect>);
  eu.set_instruction(0x50a8, 0x0e07, &addql<disp_indirect>);
  eu.set_instruction(0x50b0, 0x0e07, &addql<indexed_indirect>);
  eu.set_instruction(0x50b9, 0x0e00, &addql<absolute_long>);
  eu.set_instruction(0x50c0, 0x0007, &s_b<t, data_register>);
  eu.set_instruction(0x50d0, 0x0007, &s_b<t, indirect>);
  eu.set_instruction(0x50d8, 0x0007, &s_b<t, postinc_indirect>);
  eu.set_instruction(0x50e0, 0x0007, &s_b<t, predec_indirect>);
  eu.set_instruction(0x50e8, 0x0007, &s_b<t, disp_indirect>);
  eu.set_instruction(0x50f0, 0x0007, &s_b<t, indexed_indirect>);
  eu.set_instruction(0x50f9, 0x0000, &s_b<t, absolute_long>);
  eu.set_instruction(0x5140, 0x0e07, &subqw<data_register>);
  eu.set_instruction(0x5148, 0x0e07, &subqw<address_register>);
  eu.set_instruction(0x5150, 0x0e07, &subqw<indirect>);
  eu.set_instruction(0x5158, 0x0e07, &subqw<postinc_indirect>);
  eu.set_instruction(0x5160, 0x0e07, &subqw<predec_indirect>);
  eu.set_instruction(0x5168, 0x0e07, &subqw<disp_indirect>);
  eu.set_instruction(0x5179, 0x0e00, &subqw<absolute_long>);
  eu.set_instruction(0x5180, 0x0e07, &subql<data_register>);
  eu.set_instruction(0x5188, 0x0e07, &subql<address_register>);
  eu.set_instruction(0x5190, 0x0e07, &subql<indirect>);
  eu.set_instruction(0x5198, 0x0e07, &subql<postinc_indirect>);
  eu.set_instruction(0x51a0, 0x0e07, &subql<predec_indirect>);
  eu.set_instruction(0x51a8, 0x0e07, &subql<disp_indirect>);
  eu.set_instruction(0x51b0, 0x0e07, &subql<indexed_indirect>);
  eu.set_instruction(0x51b9, 0x0e00, &subql<absolute_long>);
  eu.set_instruction(0x51c0, 0x0007, &s_b<f, data_register>);
  eu.set_instruction(0x51c8, 0x0007, &dbf);
  eu.set_instruction(0x51d0, 0x0007, &s_b<f, indirect>);
  eu.set_instruction(0x51d8, 0x0007, &s_b<f, postinc_indirect>);
  eu.set_instruction(0x51e0, 0x0007, &s_b<f, predec_indirect>);
  eu.set_instruction(0x51e8, 0x0007, &s_b<f, disp_indirect>);
  eu.set_instruction(0x51f0, 0x0007, &s_b<f, indexed_indirect>);
  eu.set_instruction(0x51f9, 0x0000, &s_b<f, absolute_long>);
  eu.set_instruction(0x6000, 0x00ff, &bra);
  eu.set_instruction(0x6100, 0x00ff, &bsr);
  eu.set_instruction(0x6200, 0x00ff, &b<hi>);
  eu.set_instruction(0x6300, 0x00ff, &b<ls>);
  eu.set_instruction(0x6400, 0x00ff, &b<cc>);
  eu.set_instruction(0x6500, 0x00ff, &b<cs>);
  eu.set_instruction(0x6600, 0x00ff, &bne);
  eu.set_instruction(0x6700, 0x00ff, &beq);
  eu.set_instruction(0x6a00, 0x00ff, &b<pl>);
  eu.set_instruction(0x6b00, 0x00ff, &b<mi>);
  eu.set_instruction(0x6c00, 0x00ff, &b<ge>);
  eu.set_instruction(0x6d00, 0x00ff, &b<lt>);
  eu.set_instruction(0x6e00, 0x00ff, &b<gt>);
  eu.set_instruction(0x6f00, 0x00ff, &b<le>);
  eu.set_instruction(0x7000, 0x0eff, &moveql_d);
  eu.set_instruction(0x8000, 0x0e07, &orb<data_register>);
  eu.set_instruction(0x8010, 0x0e07, &orb<indirect>);
  eu.set_instruction(0x8018, 0x0e07, &orb<postinc_indirect>);
  eu.set_instruction(0x8020, 0x0e07, &orb<predec_indirect>);
  eu.set_instruction(0x8028, 0x0e07, &orb<disp_indirect>);
  eu.set_instruction(0x8030, 0x0e07, &orb<indexed_indirect>);
  eu.set_instruction(0x8039, 0x0e00, &orb<absolute_long>);
  eu.set_instruction(0x803c, 0x0e00, &orb<immediate>);
  eu.set_instruction(0x8040, 0x0e07, &orw<data_register>);
  eu.set_instruction(0x8050, 0x0e07, &orw<indirect>);
  eu.set_instruction(0x8058, 0x0e07, &orw<postinc_indirect>);
  eu.set_instruction(0x8060, 0x0e07, &orw<predec_indirect>);
  eu.set_instruction(0x8068, 0x0e07, &orw<disp_indirect>);
  eu.set_instruction(0x8079, 0x0e00, &orw<absolute_long>);
  eu.set_instruction(0x807a, 0x0e00, &orw<disp_pc>);
  eu.set_instruction(0x807c, 0x0e00, &orw<immediate>);
  eu.set_instruction(0x80c0, 0x0e07, &divuw<data_register>);
  eu.set_instruction(0x80d0, 0x0e07, &divuw<indirect>);
  eu.set_instruction(0x80d8, 0x0e07, &divuw<postinc_indirect>);
  eu.set_instruction(0x80e0, 0x0e07, &divuw<predec_indirect>);
  eu.set_instruction(0x80e8, 0x0e07, &divuw<disp_indirect>);
  eu.set_instruction(0x80f0, 0x0e07, &divuw<indexed_indirect>);
  eu.set_instruction(0x80f9, 0x0e00, &divuw<absolute_long>);
  eu.set_instruction(0x80fc, 0x0e00, &divuw<immediate>);
  eu.set_instruction(0x9000, 0x0e07, &subb<data_register>);
  eu.set_instruction(0x9010, 0x0e07, &subb<indirect>);
  eu.set_instruction(0x9018, 0x0e07, &subb<postinc_indirect>);
  eu.set_instruction(0x9020, 0x0e07, &subb<predec_indirect>);
  eu.set_instruction(0x9028, 0x0e07, &subb<disp_indirect>);
  eu.set_instruction(0x9039, 0x0e00, &subb<absolute_long>);
  eu.set_instruction(0x903a, 0x0e00, &subb<disp_pc>);
  eu.set_instruction(0x903c, 0x0e00, &subb<immediate>);
  eu.set_instruction(0x9040, 0x0e07, &subw<data_register>);
  eu.set_instruction(0x9048, 0x0e07, &subw<address_register>);
  eu.set_instruction(0x9050, 0x0e07, &subw<indirect>);
  eu.set_instruction(0x9058, 0x0e07, &subw<postinc_indirect>);
  eu.set_instruction(0x9060, 0x0e07, &subw<predec_indirect>);
  eu.set_instruction(0x9068, 0x0e07, &subw<disp_indirect>);
  eu.set_instruction(0x9079, 0x0e00, &subw<absolute_long>);
  eu.set_instruction(0x907a, 0x0e00, &subw<disp_pc>);
  eu.set_instruction(0x907c, 0x0e00, &subw<immediate>);
  eu.set_instruction(0x9080, 0x0e07, &subl<data_register>);
  eu.set_instruction(0x9088, 0x0e07, &subl<address_register>);
  eu.set_instruction(0x9090, 0x0e07, &subl<indirect>);
  eu.set_instruction(0x9098, 0x0e07, &subl<postinc_indirect>);
  eu.set_instruction(0x90a0, 0x0e07, &subl<predec_indirect>);
  eu.set_instruction(0x90a8, 0x0e07, &subl<disp_indirect>);
  eu.set_instruction(0x90b0, 0x0e07, &subl<indexed_indirect>);
  eu.set_instruction(0x90b9, 0x0e00, &subl<absolute_long>);
  eu.set_instruction(0x90bc, 0x0e00, &subl<immediate>);
  eu.set_instruction(0x9190, 0x0e07, &subl_r<indirect>);
  eu.set_instruction(0x9198, 0x0e07, &subl_r<postinc_indirect>);
  eu.set_instruction(0x91a0, 0x0e07, &subl_r<predec_indirect>);
  eu.set_instruction(0x91a8, 0x0e07, &subl_r<disp_indirect>);
  eu.set_instruction(0x91b9, 0x0e00, &subl_r<absolute_long>);
  eu.set_instruction(0x91c0, 0x0e07, &subal<data_register>);
  eu.set_instruction(0x91c8, 0x0e07, &subal<address_register>);
  eu.set_instruction(0x91d0, 0x0e07, &subal<indirect>);
  eu.set_instruction(0x91d8, 0x0e07, &subal<postinc_indirect>);
  eu.set_instruction(0x91e0, 0x0e07, &subal<predec_indirect>);
  eu.set_instruction(0x91e8, 0x0e07, &subal<disp_indirect>);
  eu.set_instruction(0x91f0, 0x0e07, &subal<indexed_indirect>);
  eu.set_instruction(0x91f9, 0x0e00, &subal<absolute_long>);
  eu.set_instruction(0x91fc, 0x0e00, &subal<immediate>);
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
  eu.set_instruction(0xb0c0, 0x0e07, &cmpaw<data_register>);
  eu.set_instruction(0xb0c8, 0x0e07, &cmpaw<address_register>);
  eu.set_instruction(0xb0d0, 0x0e07, &cmpaw<indirect>);
  eu.set_instruction(0xb0d8, 0x0e07, &cmpaw<postinc_indirect>);
  eu.set_instruction(0xb0e0, 0x0e07, &cmpaw<predec_indirect>);
  eu.set_instruction(0xb0e8, 0x0e07, &cmpaw<disp_indirect>);
  eu.set_instruction(0xb0f0, 0x0e07, &cmpaw<indexed_indirect>);  
  eu.set_instruction(0xb0f9, 0x0e00, &cmpaw<absolute_long>);
  eu.set_instruction(0xb0fa, 0x0e00, &cmpaw<disp_pc>);
  eu.set_instruction(0xb0fc, 0x0e00, &cmpaw<immediate>);
  eu.set_instruction(0xb100, 0x0e07, &eorb_r<data_register>);
  eu.set_instruction(0xb110, 0x0e07, &eorb_r<indirect>);
  eu.set_instruction(0xb118, 0x0e07, &eorb_r<postinc_indirect>);
  eu.set_instruction(0xb120, 0x0e07, &eorb_r<predec_indirect>);
  eu.set_instruction(0xb128, 0x0e07, &eorb_r<disp_indirect>);
  eu.set_instruction(0xb130, 0x0e07, &eorb_r<indexed_indirect>);
  eu.set_instruction(0xb139, 0x0e00, &eorb_r<absolute_long>);
  eu.set_instruction(0xb140, 0x0e07, &eorw_r<data_register>);
  eu.set_instruction(0xb150, 0x0e07, &eorw_r<indirect>);
  eu.set_instruction(0xb158, 0x0e07, &eorw_r<postinc_indirect>);
  eu.set_instruction(0xb160, 0x0e07, &eorw_r<predec_indirect>);
  eu.set_instruction(0xb168, 0x0e07, &eorw_r<disp_indirect>);
  eu.set_instruction(0xb170, 0x0e07, &eorw_r<indexed_indirect>);
  eu.set_instruction(0xb179, 0x0e00, &eorw_r<absolute_long>);
  eu.set_instruction(0xb1c0, 0x0e07, &cmpal<data_register>);
  eu.set_instruction(0xb1c8, 0x0e07, &cmpal<address_register>);
  eu.set_instruction(0xb1d0, 0x0e07, &cmpal<indirect>);
  eu.set_instruction(0xb1d8, 0x0e07, &cmpal<postinc_indirect>);
  eu.set_instruction(0xb1e0, 0x0e07, &cmpal<predec_indirect>);
  eu.set_instruction(0xb1e8, 0x0e07, &cmpal<disp_indirect>);
  eu.set_instruction(0xb1f9, 0x0e00, &cmpal<absolute_long>);
  eu.set_instruction(0xb1fa, 0x0e00, &cmpal<disp_pc>);
  eu.set_instruction(0xb1fc, 0x0e00, &cmpal<immediate>);
  eu.set_instruction(0xc000, 0x0e07, &andb<data_register>);
  eu.set_instruction(0xc010, 0x0e07, &andb<indirect>);
  eu.set_instruction(0xc018, 0x0e07, &andb<postinc_indirect>);
  eu.set_instruction(0xc020, 0x0e07, &andb<predec_indirect>);
  eu.set_instruction(0xc028, 0x0e07, &andb<disp_indirect>);
  eu.set_instruction(0xc030, 0x0e07, &andb<indexed_indirect>);
  eu.set_instruction(0xc039, 0x0e00, &andb<absolute_long>);
  eu.set_instruction(0xc07c, 0x0e00, &andw<immediate>);
  eu.set_instruction(0xc040, 0x0e07, &andw<data_register>);
  eu.set_instruction(0xc050, 0x0e07, &andw<indirect>);
  eu.set_instruction(0xc058, 0x0e07, &andw<postinc_indirect>);
  eu.set_instruction(0xc060, 0x0e07, &andw<predec_indirect>);
  eu.set_instruction(0xc068, 0x0e07, &andw<disp_indirect>);
  eu.set_instruction(0xc070, 0x0e07, &andw<indexed_indirect>);
  eu.set_instruction(0xc079, 0x0e00, &andw<absolute_long>);
  eu.set_instruction(0xc07c, 0x0e00, &andw<immediate>);
  eu.set_instruction(0xc080, 0x0e07, &andl<data_register>);
  eu.set_instruction(0xc090, 0x0e07, &andl<indirect>);
  eu.set_instruction(0xc098, 0x0e07, &andl<postinc_indirect>);
  eu.set_instruction(0xc0a0, 0x0e07, &andl<predec_indirect>);
  eu.set_instruction(0xc0a8, 0x0e07, &andl<disp_indirect>);
  eu.set_instruction(0xc0b0, 0x0e07, &andl<indexed_indirect>);
  eu.set_instruction(0xc0b9, 0x0e00, &andl<absolute_long>);
  eu.set_instruction(0xc0bc, 0x0e00, &andl<immediate>);
  eu.set_instruction(0xc0c0, 0x0e07, &muluw<data_register>);
  eu.set_instruction(0xc0d0, 0x0e07, &muluw<indirect>);
  eu.set_instruction(0xc0d8, 0x0e07, &muluw<postinc_indirect>);
  eu.set_instruction(0xc0e0, 0x0e07, &muluw<predec_indirect>);
  eu.set_instruction(0xc0e8, 0x0e07, &muluw<disp_indirect>);
  eu.set_instruction(0xc0f0, 0x0e07, &muluw<indexed_indirect>);
  eu.set_instruction(0xc0f9, 0x0e00, &muluw<absolute_long>);
  eu.set_instruction(0xc0fc, 0x0e00, &muluw<immediate>);
  eu.set_instruction(0xc1c0, 0x0e07, &mulsw<data_register>);
  eu.set_instruction(0xc1d0, 0x0e07, &mulsw<indirect>);
  eu.set_instruction(0xc1d8, 0x0e07, &mulsw<postinc_indirect>);
  eu.set_instruction(0xc1e0, 0x0e07, &mulsw<predec_indirect>);
  eu.set_instruction(0xc1e8, 0x0e07, &mulsw<disp_indirect>);
  eu.set_instruction(0xc1f0, 0x0e07, &mulsw<indexed_indirect>);
  eu.set_instruction(0xc1f9, 0x0e00, &mulsw<absolute_long>);
  eu.set_instruction(0xc1fc, 0x0e00, &mulsw<immediate>);
  eu.set_instruction(0xd000, 0x0e07, &addb<data_register>);
  eu.set_instruction(0xd010, 0x0e07, &addb<indirect>);
  eu.set_instruction(0xd018, 0x0e07, &addb<postinc_indirect>);
  eu.set_instruction(0xd020, 0x0e07, &addb<predec_indirect>);
  eu.set_instruction(0xd028, 0x0e07, &addb<disp_indirect>);
  eu.set_instruction(0xd039, 0x0e00, &addb<absolute_long>);
  eu.set_instruction(0xd03a, 0x0e00, &addb<disp_pc>);
  eu.set_instruction(0xd03c, 0x0e00, &addb<immediate>);
  eu.set_instruction(0xd040, 0x0e07, &addw<data_register>);
  eu.set_instruction(0xd048, 0x0e07, &addw<address_register>);
  eu.set_instruction(0xd050, 0x0e07, &addw<indirect>);
  eu.set_instruction(0xd058, 0x0e07, &addw<postinc_indirect>);
  eu.set_instruction(0xd060, 0x0e07, &addw<predec_indirect>);
  eu.set_instruction(0xd068, 0x0e07, &addw<disp_indirect>);
  eu.set_instruction(0xd070, 0x0e07, &addw<indexed_indirect>);
  eu.set_instruction(0xd079, 0x0e00, &addw<absolute_long>);
  eu.set_instruction(0xd07c, 0x0e00, &addw<immediate>);
  eu.set_instruction(0xd080, 0x0e07, &addl<data_register>);
  eu.set_instruction(0xd088, 0x0e07, &addl<address_register>);
  eu.set_instruction(0xd090, 0x0e07, &addl<indirect>);
  eu.set_instruction(0xd098, 0x0e07, &addl<postinc_indirect>);
  eu.set_instruction(0xd0a0, 0x0e07, &addl<predec_indirect>);
  eu.set_instruction(0xd0a8, 0x0e07, &addl<disp_indirect>);
  eu.set_instruction(0xd0b0, 0x0e07, &addl<indexed_indirect>);
  eu.set_instruction(0xd0b9, 0x0e00, &addl<absolute_long>);
  eu.set_instruction(0xd0bc, 0x0e00, &addl<immediate>);
  eu.set_instruction(0xd150, 0x0e07, &addw_r<indirect>);
  eu.set_instruction(0xd158, 0x0e07, &addw_r<postinc_indirect>);
  eu.set_instruction(0xd160, 0x0e07, &addw_r<predec_indirect>);
  eu.set_instruction(0xd168, 0x0e07, &addw_r<disp_indirect>);
  eu.set_instruction(0xd170, 0x0e07, &addw_r<indexed_indirect>);
  eu.set_instruction(0xd179, 0x0e00, &addw_r<absolute_long>);
  eu.set_instruction(0xd190, 0x0e07, &addl_r<indirect>);
  eu.set_instruction(0xd198, 0x0e07, &addl_r<postinc_indirect>);
  eu.set_instruction(0xd1a0, 0x0e07, &addl_r<predec_indirect>);
  eu.set_instruction(0xd1a8, 0x0e07, &addl_r<disp_indirect>);
  eu.set_instruction(0xd1b0, 0x0e07, &addl_r<indexed_indirect>);
  eu.set_instruction(0xd1b9, 0x0e00, &addl_r<absolute_long>);
  eu.set_instruction(0xd1c0, 0x0e07, &addal<data_register>);
  eu.set_instruction(0xd1c8, 0x0e07, &addal<address_register>);
  eu.set_instruction(0xd1d0, 0x0e07, &addal<indirect>);
  eu.set_instruction(0xd1d8, 0x0e07, &addal<postincrement_indirect>);
  eu.set_instruction(0xd1e0, 0x0e07, &addal<predecrement_indirect>);
  eu.set_instruction(0xd1e8, 0x0e07, &addal<disp_indirect>);
  eu.set_instruction(0xd1f0, 0x0e07, &addal<indexed_indirect>);
  eu.set_instruction(0xd1f9, 0x0e00, &addal<absolute_long>);
  eu.set_instruction(0xd1fc, 0x0e00, &addal<immediate>);
  eu.set_instruction(0xe048, 0x0e07, &lsrw_i);
  eu.set_instruction(0xe068, 0x0e07, &lsrw_r);
  eu.set_instruction(0xe088, 0x0e07, &lsrl_i);
  eu.set_instruction(0xe0a0, 0x0e07, &asrl_r);
  eu.set_instruction(0xe0a8, 0x0e07, &lsrl_r);
  eu.set_instruction(0xe108, 0x0e07, &lslb_i);
  eu.set_instruction(0xe138, 0x0e07, &rolb_r);
  eu.set_instruction(0xe148, 0x0e07, &lslw_i);
  eu.set_instruction(0xe158, 0x0e07, &rolw_i);
  eu.set_instruction(0xe168, 0x0e07, &lslw_r);
  eu.set_instruction(0xe180, 0x0e07, &asll_i);
  eu.set_instruction(0xe188, 0x0e07, &lsll_i);
  eu.set_instruction(0xe1a0, 0x0e07, &asll_r);
  eu.set_instruction(0xe1a8, 0x0e07, &lsll_r);
}

