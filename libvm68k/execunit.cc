/* Virtual X68000 - X68000 virtual machine
   Copyright (C) 1998-2000 Hypercore Software Design, Ltd.

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
# undef TRACE_INSTRUCTIONS
#endif

using vm68k::exec_unit;
using vm68k::context;
using vm68k::byte_size;
using vm68k::word_size;
using vm68k::long_word_size;
using vm68k::privilege_violation_exception;
using namespace vm68k::types;
using namespace std;

void
exec_unit::run(context &c) const
{
  for (;;)
    {
      if (c.interrupted())
	c.handle_interrupts();
      else
	{
#ifdef TRACE_INSTRUCTIONS
# ifdef DUMP_REGISTERS
	  for (unsigned int i = 0; i != 8; ++i)
	    {
	      L("| %%d%u = 0x%08lx, %%a%u = 0x%08lx\n",
		i, (unsigned long) c.regs.d[i],
		i, (unsigned long) c.regs.a[i]);
	    }
# endif
	  L("| %#lx: %#06x\n", (unsigned long) c.regs.pc,
	    c.fetch(word_size(), 0));
#endif
	  step(c);
	}
    }
}

void
exec_unit::set_instruction(int code, int mask, const instruction_type &in)
{
  I (code >= 0);
  I (code < 0x10000);
  code &= ~mask;
  for (int i = code; i <= (code | mask); ++i)
    {
      if ((i & ~mask) == code)
	{
	  instruction_type old_value = set_instruction(i, in);
#ifdef L
	  if (old_value.first != &illegal)
	    L("warning: Replacing instruction handler at 0x%04x\n", i);
#endif
	}
    }
}

namespace
{
  using vm68k::SUPER_DATA;
  using namespace vm68k::condition;
  using namespace vm68k::addressing;

  /* Returns the signed 8-bit value that is equivalent to unsigned
     value VALUE.  */
  inline int
  extsb(unsigned int value)
  {
    const unsigned int N = 1u << 7;
    const unsigned int M = (N << 1) - 1;
    value &= M;
    return value >= N ? -int(M - value) - 1 : int(value);
  }

  /* Returns the signed 16-bit value that is equivalent to unsigned
     value VALUE.  */
  inline sint_type
  extsw(uint_type value)
  {
    const uint_type N = uint_type(1) << 15;
    const uint_type M = (N << 1) - 1;
    value &= M;
    return value >= N ? -sint_type(M - value) - 1 : sint_type(value);
  }

  /* Returns the signed 32-bit value that is equivalent to unsigned
     value VALUE.  */
  inline sint32_type
  extsl(uint32_type value)
  {
    const uint32_type N = uint32_type(1) << 31;
    const uint32_type M = (N << 1) - 1;
    value &= M;
    return value >= N ? -sint32_type(M - value) - 1 : sint32_type(value);
  }

  /* Handles an ADD instruction.  */
  template <class Size, class Source> void
  m68k_add(uint_type op, context &c, unsigned long data)
  {
    typedef typename Size::uvalue_type uvalue_type;
    typedef typename Size::svalue_type svalue_type;

    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef TRACE_INSTRUCTIONS
    L(" add%s %s", Size::suffix(), ea1.text(c));
    L(",%%d%u\n", reg2);
#endif

    svalue_type value1 = ea1.get(c);
    svalue_type value2 = Size::svalue(Size::get(c.regs.d[reg2]));
    svalue_type value = Size::svalue(Size::get(value2 + value1));
    Size::put(c.regs.d[reg2], value);
    c.regs.ccr.set_cc_as_add(value, value2, value1);
    ea1.finish(c);

    c.regs.pc += 2 + ea1.extension_size();
  }

  /* Handles an ADD instruction (memory destination).  */
  template <class Size, class Destination> void
  m68k_add_m(uint_type op, context &c, unsigned long data)
  {
    typedef typename Size::uvalue_type uvalue_type;
    typedef typename Size::svalue_type svalue_type;

    Destination ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef TRACE_INSTRUCTIONS
    L(" add%s %%d%u,", Size::suffix(), reg2);
    L("%s\n", ea1.text(c));
#endif

    svalue_type value2 = Size::svalue(Size::get(c.regs.d[reg2]));
    svalue_type value1 = ea1.get(c);
    svalue_type value = Size::svalue(Size::get(value1 + value2));
    ea1.put(c, value);
    c.regs.ccr.set_cc_as_add(value, value1, value2);
    ea1.finish(c);

    c.regs.pc += 2 + ea1.extension_size();
  }

  /* Handles an ADDA instruction.  */
  template <class Size, class Source> void
  m68k_adda(uint_type op, context &c, unsigned long data)
  {
    typedef long_word_size::uvalue_type uvalue_type;
    typedef long_word_size::svalue_type svalue_type;

    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef TRACE_INSTRUCTIONS
    L(" adda%s %s", Size::suffix(), ea1.text(c));
    L(",%%a%u\n", reg2);
#endif

    // The condition codes are not affected by this instruction.
    svalue_type value1 = ea1.get(c);
    svalue_type value2
      = long_word_size::svalue(long_word_size::get(c.regs.a[reg2]));
    svalue_type value
      = long_word_size::svalue(long_word_size::get(value2 + value1));
    long_word_size::put(c.regs.a[reg2], value);
    ea1.finish(c);

    c.regs.pc += 2 + ea1.extension_size();
  }

  /* Handles an ADDI instruction.  */
  template <class Size, class Destination> void
  m68k_addi(uint_type op, context &c, unsigned long data)
  {
    typedef typename Size::uvalue_type uvalue_type;
    typedef typename Size::svalue_type svalue_type;

    svalue_type value2 = Size::svalue(c.fetch(Size(), 2));
    Destination ea1(op & 0x7, 2 + Size::aligned_value_size());
#ifdef TRACE_INSTRUCTIONS
    L(" addi%s #%#lx", Size::suffix(), (unsigned long) value2);
    L(",%s\n", ea1.text(c));
#endif

    svalue_type value1 = ea1.get(c);
    svalue_type value = Size::svalue(Size::get(value1 + value2));
    ea1.put(c, value);
    c.regs.ccr.set_cc_as_add(value, value1, value2);
    ea1.finish(c);

    c.regs.pc += 2 + Size::aligned_value_size() + ea1.extension_size();
  }

  /* Handles an ADDQ instruction.  */
  template <class Size, class Destination> void
  m68k_addq(uint_type op, context &c, unsigned long data)
  {
    typedef typename Size::uvalue_type uvalue_type;
    typedef typename Size::svalue_type svalue_type;

    Destination ea1(op & 0x7, 2);
    int value2 = op >> 9 & 0x7;
    if (value2 == 0)
      value2 = 8;
#ifdef TRACE_INSTRUCTIONS
    L(" addq%s #%d", Size::suffix(), value2);
    L(",%s\n", ea1.text(c));
#endif

    svalue_type value1 = ea1.get(c);
    svalue_type value = Size::svalue(Size::get(value1 + value2));
    ea1.put(c, value);
    c.regs.ccr.set_cc_as_add(value, value1, value2);
    ea1.finish(c);

    c.regs.pc += 2 + ea1.extension_size();
  }

  /* Handles an ADDQ instruction (address register).  */
  template <class Size> void
  m68k_addq_a(uint_type op, context &c, unsigned long data)
  {
    typedef long_word_size::uvalue_type uvalue_type;
    typedef long_word_size::svalue_type svalue_type;

    unsigned int reg1 = op & 0x7;
    int value2 = op >> 9 & 0x7;
    if (value2 == 0)
      value2 = 8;
#ifdef TRACE_INSTRUCTIONS
    L(" addq%s #%d", Size::suffix(), value2);
    L(",%%a%u\n", reg1);
#endif

    // The condition codes are not affected by this instruction.
    svalue_type value1
      = long_word_size::svalue(long_word_size::get(c.regs.a[reg1]));
    svalue_type value
      = long_word_size::svalue(long_word_size::get(value1 + value2));
    long_word_size::put(c.regs.a[reg1], value);

    c.regs.pc += 2;
  }

  /* Handles an ADDX instruction (register).  */
  template <class Size> void
  m68k_addx_r(uint_type op, context &c, unsigned long data)
  {
    typedef typename Size::uvalue_type uvalue_type;
    typedef typename Size::svalue_type svalue_type;

    unsigned int reg1 = op & 0x7;
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef TRACE_INSTRUCTIONS
    L(" addx%s %%d%u,", Size::suffix(), reg1);
    L("%%d%u\n", reg2);
#endif

    svalue_type value1 = Size::svalue(Size::get(c.regs.d[reg1]));
    svalue_type value2 = Size::svalue(Size::get(c.regs.d[reg2]));
    svalue_type value
      = Size::svalue(Size::get(value2 + value1 + c.regs.ccr.x()));
    Size::put(c.regs.d[reg2], value);
    c.regs.ccr.set_cc_as_add(value, value2, value1);

    c.regs.pc += 2;
  }

  /* Handles an AND instruction.  */
  template <class Size, class Source> void
  m68k_and(uint_type op, context &c, unsigned long data)
  {
    typedef typename Size::uvalue_type uvalue_type;
    typedef typename Size::svalue_type svalue_type;

    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef TRACE_INSTRUCTIONS
    L(" and%s %s", Size::suffix(), ea1.text(c));
    L(",%%d%u\n", reg2);
#endif

    svalue_type value1 = ea1.get(c);
    svalue_type value2 = Size::svalue(Size::get(c.regs.d[reg2]));
    svalue_type value
      = Size::svalue(Size::get(uvalue_type(value2) & uvalue_type(value1)));
    Size::put(c.regs.d[reg2], value);
    c.regs.ccr.set_cc(value);
    ea1.finish(c);

    c.regs.pc += 2 + ea1.extension_size();
  }

  /* Handles an AND instruction (memory destination).  */
  template <class Size, class Destination> void
  m68k_and_m(uint_type op, context &c, unsigned long data)
  {
    typedef typename Size::uvalue_type uvalue_type;
    typedef typename Size::svalue_type svalue_type;

    Destination ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef TRACE_INSTRUCTIONS
    L("\tand%s\t", Size::suffix());
    L("%%d%u,", reg2);
    L("%s\n", ea1.text(c));
#endif

    svalue_type value2 = Size::svalue(Size::get(c.regs.d[reg2]));
    svalue_type value1 = ea1.get(c);
    svalue_type value = Size::svalue(Size::get(value1) & Size::get(value2));
    ea1.put(c, value);
    c.regs.ccr.set_cc(value);

    ea1.finish(c);
    c.regs.pc += 2 + ea1.extension_size();
  }

  /* Handles an ANDI instruction.  */
  template <class Size, class Destination> void
  m68k_andi(uint_type op, context &c, unsigned long data)
  {
    typedef typename Size::uvalue_type uvalue_type;
    typedef typename Size::svalue_type svalue_type;

    svalue_type value2 = Size::svalue(c.fetch(Size(), 2));
    Destination ea1(op & 0x7, 2 + Size::aligned_value_size());
#ifdef TRACE_INSTRUCTIONS
    L("\tandi%s\t", Size::suffix());
    L("#%#lx,", (unsigned long) Size::get(value2));
    L("%s\n", ea1.text(c));
#endif

    svalue_type value1 = ea1.get(c);
    svalue_type value = Size::svalue(Size::get(value1) & Size::get(value2));
    ea1.put(c, value);
    c.regs.ccr.set_cc(value);
    ea1.finish(c);

    c.regs.pc += 2 + Size::aligned_value_size() + ea1.extension_size();
  }

#if 0
  /* Handles ANDI instruction for byte.  */
  template <class Destination> void
  andib(uint_type op, context &c, unsigned long data)
  {
    sint_type value2 = extsb(c.fetch(word_size(), 2));
    Destination ea1(op & 0x7, 2 + 2);
#ifdef TRACE_INSTRUCTIONS
    L(" andib #0x%x", uint_type(value2) & 0xffu);
    L(",%s\n", ea1.textb(c));
#endif

    sint_type value1 = ea1.getb(c);
    sint_type value = extsb(uint_type(value1) & uint_type(value2));
    ea1.putb(c, value);
    c.regs.ccr.set_cc(value);
    ea1.finishb(c);

    c.regs.pc += 2 + 2 + ea1.isize(2);
  }

  template <class Destination> void
  andiw(uint_type op, context &ec, unsigned long data)
  {
    sint_type value2 = extsw(ec.fetch(word_size(), 2));
    Destination ea1(op & 0x7, 2 + 2);
#ifdef TRACE_INSTRUCTIONS
    L(" andiw #0x%x", uint_type(value2));
    L(",%s\n", ea1.textw(ec));
#endif

    sint_type value1 = ea1.getw(ec);
    sint_type value = extsw(uint_type(value1) & uint_type(value2));
    ea1.putw(ec, value);
    ec.regs.ccr.set_cc(value);
    ea1.finishw(ec);

    ec.regs.pc += 2 + 2 + ea1.isize(2);
  }

  template <class Destination> void
  andil(uint_type op, context &ec, unsigned long data)
  {
    sint32_type value2 = extsl(ec.fetch(long_word_size(), 2));
    Destination ea1(op & 0x7, 2 + 4);
#ifdef TRACE_INSTRUCTIONS
    L(" andil #0x%lx", (unsigned long) value2);
    L(",%s\n", ea1.textl(ec));
#endif

    sint32_type value1 = ea1.getl(ec);
    sint32_type value = extsl(uint32_type(value1) & uint32_type(value2));
    ea1.putl(ec, value);
    ec.regs.ccr.set_cc(value);
    ea1.finishl(ec);

    ec.regs.pc += 2 + 4 + ea1.isize(4);
  }
#endif

  /* Handles an ANDI-to-CCR instruction.  */
  void
  m68k_andi_to_ccr(uint_type op, context &c, unsigned long data)
  {
    typedef byte_size::uvalue_type uvalue_type;
    typedef byte_size::svalue_type svalue_type;

    uvalue_type value2 = c.fetch(byte_size(), 2);
#ifdef TRACE_INSTRUCTIONS
    L("\tandi%s\t", byte_size::suffix());
    L("#%#x,", value2);
    L("%%ccr\n");
#endif

    uvalue_type value1 = c.regs.ccr & 0xffu;
    uvalue_type value = value1 & value2;
    c.regs.ccr = c.regs.ccr & ~0xffu | value & 0xffu;

    c.regs.pc += 2 + byte_size::aligned_value_size();
  }

  /* Handles an ASL instruction with an immediate count.  */
  template <class Size> void
  m68k_asl_i(uint_type op, context &c, unsigned long data)
  {
    typedef typename Size::uvalue_type uvalue_type;
    typedef typename Size::svalue_type svalue_type;

    unsigned int reg1 = op & 0x7;
    unsigned int value2 = op >> 9 & 0x7;
    if (value2 == 0)
      value2 = 8;
#ifdef TRACE_INSTRUCTIONS
    L(" asl%s #%u", Size::suffix(), value2);
    L(",%%d%u\n", reg1);
#endif

    svalue_type value1 = Size::svalue(Size::get(c.regs.d[reg1]));
    svalue_type value = Size::svalue(Size::get(value1 << value2));
    Size::put(c.regs.d[reg1], value);
    c.regs.ccr.set_cc_lsl(value, value1, value2 + (32 - Size::value_bit())); // FIXME?

    c.regs.pc += 2;
  }

#if 0
  void
  asll_i(uint_type op, context &ec, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int count = op >> 9 & 0x7;
    if (count == 0)
      count = 8;
#ifdef TRACE_INSTRUCTIONS
    L(" asll #%u", count);
    L(",%%d%u\n", reg1);
#endif

    sint32_type value1 = extsl(ec.regs.d[reg1]);
    sint32_type value = extsl(value1 << count);
    ec.regs.d[reg1] = value;
    ec.regs.ccr.set_cc(value);	// FIXME.

    ec.regs.pc += 2;
  }
#endif

  /* Handles an ASL instruction with a register count.  */
  template <class Size> void
  m68k_asl_r(uint_type op, context &c, unsigned long data)
  {
    typedef typename Size::uvalue_type uvalue_type;
    typedef typename Size::svalue_type svalue_type;

    unsigned int reg1 = op & 0x7;
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef TRACE_INSTRUCTIONS
    L("\tasl%s\t", Size::suffix());
    L("%%d%u,", reg2);
    L("%%d%u\n", reg1);
#endif

    uvalue_type value2 = c.regs.d[reg2] % Size::value_bit();
    svalue_type value1 = Size::svalue(Size::get(c.regs.d[reg1]));
    svalue_type value = Size::svalue(Size::get(value1 << value2));
    Size::put(c.regs.d[reg1], value);
    c.regs.ccr.set_cc_lsl(value, value1, value2 + (32 - Size::value_bit())); // FIXME?

    c.regs.pc += 2;
  }

#if 0
  void
  asll_r(uint_type op, context &ec, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef TRACE_INSTRUCTIONS
    L(" asll %%d%u", reg2);
    L(",%%d%u\n", reg1);
#endif

    uint_type count = ec.regs.d[reg2];
    sint32_type value1 = extsl(ec.regs.d[reg1]);
    sint32_type value = extsl(value1 << count);
    ec.regs.d[reg1] = value;
    ec.regs.ccr.set_cc(value);	// FIXME.

    ec.regs.pc += 2;
  }
#endif

  /* Handles an ASR instruction with an immediate count.  */
  template <class Size> void
  m68k_asr_i(uint_type op, context &c, unsigned long data)
  {
    typedef typename Size::uvalue_type uvalue_type;
    typedef typename Size::svalue_type svalue_type;

    unsigned int reg1 = op & 0x7;
    unsigned int value2 = op >> 9 & 0x7;
    if (value2 == 0)
      value2 = 8;
#ifdef TRACE_INSTRUCTIONS
    L("\tasr%s\t", Size::suffix());
    L("#%u,", value2);
    L("%%d%u\n", reg1);
#endif

    svalue_type value1 = Size::svalue(Size::get(c.regs.d[reg1]));
    svalue_type value = Size::svalue(Size::get(value1 >> value2));
    Size::put(c.regs.d[reg1], value);
    c.regs.ccr.set_cc_asr(value, value1, value2);

    c.regs.pc += 2;
  }

#if 0
  void
  asrl_i(uint_type op, context &c, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int count = op >> 9 & 0x7;
    if (count == 0)
      count = 8;
#ifdef TRACE_INSTRUCTIONS
    L(" asrl #%u", count);
    L(",%%d%u\n", reg1);
#endif

    sint32_type value1 = extsl(c.regs.d[reg1]);
    sint32_type value = value1 >> count;
    c.regs.d[reg1] = value;
    c.regs.ccr.set_cc_asr(value, value1, count);

    c.regs.pc += 2;
  }
#endif

  void
  asrl_r(uint_type op, context &ec, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef TRACE_INSTRUCTIONS
    L(" asrl %%d%u", reg2);
    L(",%%d%u\n", reg1);
#endif

    uint_type count = ec.regs.d[reg2];
    sint32_type value1 = extsl(ec.regs.d[reg1]);
    sint32_type value = value1 >> count;
    ec.regs.d[reg1] = value;
    ec.regs.ccr.set_cc_asr(value, value1, count);

    ec.regs.pc += 2;
  }

  /* Handles a Bcc instruction.  */
  template <class Condition> void 
  m68k_b(uint_type op, context &c, unsigned long data)
  {
    sint_type disp = op & 0xff;
    size_t extsize;
    if (disp == 0)
      {
	disp = word_size::svalue(c.fetch(word_size(), 2));
	extsize = 2;
      }
    else
      {
	disp = byte_size::svalue(disp);
	extsize = 0;
      }
#ifdef TRACE_INSTRUCTIONS
    L("\tb%s\t", Condition::text());
    L("%#lx\n", (unsigned long) (c.regs.pc + 2 + disp));
#endif

    // This instruction does not affect the condition codes.
    Condition cond;
    c.regs.pc += 2 + (cond(c) ? disp : extsize);
  }

#if 0
  void bcc(uint_type op, context &ec, unsigned long data)
    {
      int len = 2;
      int disp = op & 0xff;
      if (disp == 0)
	{
	  len = 4;
	  disp = extsw(ec.fetch(word_size(), 2));
	}
      else
	disp = extsb(disp);
      VL((" bcc 0x%lx\n", (unsigned long) (ec.regs.pc + 2 + disp)));

      // XXX: The condition codes are not affected.
      ec.regs.pc += ec.regs.sr.cc() ? 2 + disp : len;
    }

  void beq(uint_type op, context &ec, unsigned long data)
    {
      int len = 2;
      int disp = op & 0xff;
      if (disp == 0)
	{
	  len = 4;
	  disp = extsw(ec.fetch(word_size(), 2));
	}
      else
	disp = extsb(disp);
#ifdef TRACE_INSTRUCTIONS
      VL((" beq 0x%lx\n", (unsigned long) (ec.regs.pc + 2 + disp)));
#endif

      // XXX: The condition codes are not affected.
      ec.regs.pc += ec.regs.sr.eq() ? 2 + disp : len;
    }

  void bge(uint_type op, context &ec, unsigned long data)
    {
      int len = 2;
      int disp = op & 0xff;
      if (disp == 0)
	{
	  len = 4;
	  disp = extsw(ec.fetch(word_size(), 2));
	}
      else
	disp = extsb(disp);
#ifdef TRACE_INSTRUCTIONS
      VL((" bge 0x%lx\n", (unsigned long) (ec.regs.pc + 2 + disp)));
#endif

      // XXX: The condition codes are not affected.
      ec.regs.pc += ec.regs.sr.ge() ? 2 + disp : len;
    }

  void bmi(uint_type op, context &ec, unsigned long data)
    {
      int len = 2;
      int disp = op & 0xff;
      if (disp == 0)
	{
	  len = 4;
	  disp = extsw(ec.fetch(word_size(), 2));
	}
      else
	disp = extsb(disp);
#ifdef TRACE_INSTRUCTIONS
      L(" bmi 0x%lx\n", (unsigned long) (ec.regs.pc + 2 + disp));
#endif

      // XXX: The condition codes are not affected.
      ec.regs.pc += ec.regs.sr.mi() ? 2 + disp : len;
    }

  void bne(uint_type op, context &ec, unsigned long data)
    {
      int len = 2;
      int disp = op & 0xff;
      if (disp == 0)
	{
	  len = 4;
	  disp = extsw(ec.fetch(word_size(), 2));
	}
      else
	disp = extsb(disp);
#ifdef TRACE_INSTRUCTIONS
      VL((" bne 0x%lx\n", (unsigned long) (ec.regs.pc + 2 + disp)));
#endif

      // XXX: The condition codes are not affected.
      ec.regs.pc += ec.regs.sr.ne() ? 2 + disp : len;
    }
#endif

  /* Handles a BCLR instruction (register).  */
  template <class Size, class Destination> void
  m68k_bclr_r(uint_type op, context &c, unsigned long data)
  {
    typedef typename Size::uvalue_type uvalue_type;
    typedef typename Size::svalue_type svalue_type;

    Destination ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef TRACE_INSTRUCTIONS
    L("\tbclr%s\t", Size::suffix());
    L("%%d%u,", reg2);
    L("%s\n", ea1.text(c));
#endif

    // This instruction affects only the Z bit of the condition codes.
    unsigned int value2 = c.regs.d[reg2] % Size::value_bit();
    uvalue_type mask = uvalue_type(1) << value2;
    uvalue_type value1 = ea1.get(c);
    bool value = value1 & mask;
    ea1.put(c, value1 & ~mask);
    c.regs.ccr.set_cc(value);	// FIXME.
    ea1.finish(c);

    c.regs.pc += 2 + ea1.extension_size();
  }

  /* Handles a BCLR instruction (immediate).  */
  template <class Size, class Destination> void
  m68k_bclr_i(uint_type op, context &c, unsigned long data)
  {
    typedef typename Size::uvalue_type uvalue_type;
    typedef typename Size::svalue_type svalue_type;

    Destination ea1(op & 0x7, 2 + 2);
    unsigned int value2 = c.fetch(word_size(), 2) % Size::value_bit();
#ifdef TRACE_INSTRUCTIONS
    L("\tbclr%s\t", Size::suffix());
    L("#%u,", value2);
    L("%s\n", ea1.text(c));
#endif

    // This instruction affects only the Z bit of the condition codes.
    uvalue_type mask = uvalue_type(1) << value2;
    uvalue_type value1 = ea1.get(c);
    bool value = value1 & mask;
    ea1.put(c, value1 & ~mask);
    c.regs.ccr.set_cc(value);	// FIXME.

    ea1.finish(c);
    c.regs.pc += 2 + 2 + ea1.extension_size();
  }

#if 0
  void
  bclrl_i(uint_type op, context &ec, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int bit = ec.fetch(word_size(), 2) & 0x1f;
#ifdef TRACE_INSTRUCTIONS
    L(" bclrl #%u", bit);
    L(",%%d%u\n", reg1);
#endif

    // This instruction affects only the Z bit of condition codes.
    uint32_type mask = uint32_type(1) << bit;
    uint32_type value1 = ec.regs.d[reg1];
    bool value = value1 & mask;
    ec.regs.d[reg1] = value1 & ~mask;
    ec.regs.ccr.set_cc(value);	// FIXME.

    ec.regs.pc += 2 + 2;
  }
#endif

  void
  bra(uint_type op, context &ec, unsigned long data)
    {
      int len = 2;
      int disp = op & 0xff;
      if (disp == 0)
	{
	  len = 4;
	  disp = extsw(ec.fetch(word_size(), 2));
	}
      else
	disp = extsb(disp);
#ifdef TRACE_INSTRUCTIONS
      VL((" bra 0x%lx\n", (unsigned long) (ec.regs.pc + 2 + disp)));
#endif

      // XXX: The condition codes are not affected.
      ec.regs.pc += 2 + disp;
    }

  void
  bsetl_i(uint_type op, context &ec, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int bit = ec.fetch(word_size(), 2) & 0x1f;
#ifdef TRACE_INSTRUCTIONS
    L(" bsetl #%u", bit);
    L(",%%d%u\n", reg1);
#endif

    // This instruction affects only the Z bit of condition codes.
    uint32_type mask = uint32_type(1) << bit;
    uint32_type value1 = ec.regs.d[reg1];
    bool value = value1 & mask;
    ec.regs.d[reg1] = value1 | mask;
    ec.regs.ccr.set_cc(value);	// FIXME.

    ec.regs.pc += 2 + 2;
  }

  /* Handles a BSET instruction (register).  */
  template <class Size, class Destination> void
  m68k_bset_r(uint_type op, context &c, unsigned long data)
  {
    typedef typename Size::uvalue_type uvalue_type;
    typedef typename Size::svalue_type svalue_type;

    Destination ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef TRACE_INSTRUCTIONS
    L(" bset%s %%d%u,", Size::suffix(), reg2);
    L("%s\n", ea1.text(c));
#endif

    // This instruction affects only the Z bit of the condition codes.
    uvalue_type mask = uvalue_type(1) << c.regs.d[reg2] % Size::value_bit();
    uvalue_type value1 = ea1.get(c);
    bool value = value1 & mask;
    ea1.put(c, value1 | mask);
    c.regs.ccr.set_cc(value);	// FIXME.

    c.regs.pc += 2 + ea1.extension_size();
  }

  void
  bsr(uint_type op, context &ec, unsigned long data)
    {
      int len = 2;
      int disp = op & 0xff;
      if (disp == 0)
	{
	  len = 4;
	  disp = extsw(ec.fetch(word_size(), 2));
	}
      else
	disp = extsb(disp);
#ifdef TRACE_INSTRUCTIONS
      VL((" bsr 0x%lx\n", (unsigned long) (ec.regs.pc + 2 + disp)));
#endif

      // XXX: The condition codes are not affected.
      int fc = ec.data_fc();
      ec.mem->putl(fc, ec.regs.a[7] - 4, ec.regs.pc + len);
      ec.regs.a[7] -= 4;
      ec.regs.pc += 2 + disp;
    }

  /* Handles a BTST instruction (register).  */
  template <class Size, class Destination> void
  m68k_btst_r(uint_type op, context &c, unsigned long data)
  {
    typedef typename Size::uvalue_type uvalue_type;
    typedef typename Size::svalue_type svalue_type;

    Destination ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef TRACE_INSTRUCTIONS
    L("\tbtst%s\t", Size::suffix());
    L("%%d%u,", reg2);
    L("%s\n", ea1.text(c));
#endif

    // This instruction affects only the Z bit of the condition codes.
    unsigned int value2 = c.regs.d[reg2] % Size::value_bit();
    uvalue_type mask = uvalue_type(1) << value2;
    uvalue_type value1 = ea1.get(c);
    bool value = value1 & mask;
    c.regs.ccr.set_cc(value);	// FIXME.
    ea1.finish(c);

    c.regs.pc += 2 + ea1.extension_size();
  }

  template <class Destination> void
  btstb_i(uint_type op, context &c, unsigned long data)
  {
    unsigned int bit = c.fetch(word_size(), 2) & 0x7;
    Destination ea1(op & 0x7, 2 + 2);
#ifdef TRACE_INSTRUCTIONS
    L(" btstb #%u", bit);
    L(",%s\n", ea1.textb(c));
#endif

    // This instruction affects only the Z bit of condition codes.
    bool value = uint_type(ea1.getb(c)) & 1u << bit;
    c.regs.ccr.set_cc(value);	// FIXME.

    c.regs.pc += 2 + 2 + ea1.isize(2);
  }

  void
  btstl_i(uint_type op, context &ec, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int bit = ec.fetch(word_size(), 2) & 0x1f;
#ifdef TRACE_INSTRUCTIONS
    L(" btstl #%u", bit);
    L(",%%d%u\n", reg1);
#endif

    // This instruction affects only the Z bit of condition codes.
    bool value = ec.regs.d[reg1] & uint32_type(1) << bit;
    ec.regs.ccr.set_cc(value);	// FIXME.

    ec.regs.pc += 2 + 2;
  }

  template <class Destination> void
  clrb(uint_type op, context &ec, unsigned long data)
    {
      Destination ea1(op & 0x7, 2);
#ifdef TRACE_INSTRUCTIONS
      VL((" clrb %s\n", ea1.textb(ec)));
#endif

      ea1.putb(ec, 0);
      ea1.finishb(ec);
      ec.regs.ccr.set_cc(0);

      ec.regs.pc += 2 + ea1.isize(2);
    }

  template <class Destination> void
  clrw(uint_type op, context &ec, unsigned long data)
    {
      Destination ea1(op & 0x7, 2);
#ifdef TRACE_INSTRUCTIONS
      VL((" clrw %s\n", ea1.textw(ec)));
#endif

      ea1.putw(ec, 0);
      ea1.finishw(ec);
      ec.regs.ccr.set_cc(0);

      ec.regs.pc += 2 + ea1.isize(2);
    }

#if 0
  template <> void clrw<address_register>(unsigned int, context &);
  // XXX: Address register cannot be the destination.
#endif

  template <class Destination> void
  clrl(uint_type op, context &ec, unsigned long data)
    {
      Destination ea1(op & 0x7, 2);
#ifdef TRACE_INSTRUCTIONS
      VL((" clrl %s\n", ea1.textl(ec)));
#endif

      ea1.putl(ec, 0);
      ea1.finishl(ec);
      ec.regs.ccr.set_cc(0);

      ec.regs.pc += 2 + ea1.isize(2);
    }

  /* Handles a CMP instruction.  */
  template <class Size, class Source> void
  m68k_cmp(uint_type op, context &c, unsigned long data)
  {
    typedef typename Size::uvalue_type uvalue_type;
    typedef typename Size::svalue_type svalue_type;

    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef TRACE_INSTRUCTIONS
    L("\tcmp%s\t", Size::suffix());
    L("%s,", ea1.text(c));
    L("%%d%u\n", reg2);
#endif

    svalue_type value1 = ea1.get(c);
    svalue_type value2 = Size::svalue(Size::get(c.regs.d[reg2]));
    svalue_type value = Size::svalue(Size::get(value2 - value1));
    c.regs.ccr.set_cc_cmp(value, value2, value1);
    ea1.finish(c);

    c.regs.pc += 2 + ea1.extension_size();
  }

#if 0
  template <class Source> void
  cmpb(uint_type op, context &ec, unsigned long data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef TRACE_INSTRUCTIONS
    L(" cmpb %s", ea1.textb(ec));
    L(",%%d%u\n", reg2);
#endif

    sint_type value1 = ea1.getb(ec);
    sint_type value2 = extsb(ec.regs.d[reg2]);
    sint_type value = extsb(value2 - value1);
    ec.regs.ccr.set_cc_cmp(value, value2, value1);
    ea1.finishb(ec);

    ec.regs.pc += 2 + ea1.isize(2);
  }

  template <class Source> void
  cmpw(uint_type op, context &ec, unsigned long data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef TRACE_INSTRUCTIONS
    L(" cmpw %s", ea1.textw(ec));
    L(",%%d%u\n", reg2);
#endif

    sint_type value1 = ea1.getw(ec);
    sint_type value2 = extsw(ec.regs.d[reg2]);
    sint_type value = extsw(value2 - value1);
    ec.regs.ccr.set_cc_cmp(value, value2, value1);
    ea1.finishw(ec);

    ec.regs.pc += 2 + ea1.isize(2);
  }

  template <class Source> void
  cmpl(uint_type op, context &ec, unsigned long data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef TRACE_INSTRUCTIONS
    L(" cmpl %s", ea1.textl(ec));
    L(",%%d%u\n", reg2);
#endif

    sint32_type value1 = ea1.getl(ec);
    sint32_type value2 = extsl(ec.regs.d[reg2]);
    sint32_type value = extsl(value2 - value1);
    ec.regs.ccr.set_cc_cmp(value, value2, value1);
    ea1.finishl(ec);

    ec.regs.pc += 2 + ea1.isize(4);
  }
#endif

  template <class Source> void
  cmpaw(uint_type op, context &ec, unsigned long data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef TRACE_INSTRUCTIONS
    L(" cmpaw %s", ea1.textw(ec));
    L(",%%a%u\n", reg2);
#endif

    sint32_type value1 = ea1.getw(ec);
    sint32_type value2 = extsl(ec.regs.a[reg2]);
    sint32_type value = extsl(value2 - value1);
    ec.regs.ccr.set_cc_cmp(value, value2, value1);
    ea1.finishw(ec);

    ec.regs.pc += 2 + ea1.isize(2);
  }

  template <class Source> void
  cmpal(uint_type op, context &ec, unsigned long data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef TRACE_INSTRUCTIONS
    L(" cmpal %s", ea1.textl(ec));
    L(",%%a%u\n", reg2);
#endif

    sint32_type value1 = ea1.getl(ec);
    sint32_type value2 = extsl(ec.regs.a[reg2]);
    sint32_type value = extsl(value2 - value1);
    ec.regs.ccr.set_cc_cmp(value, value2, value1);
    ea1.finishl(ec);

    ec.regs.pc += 2 + ea1.isize(4);
  }

  template <class Destination> void
  cmpib(uint_type op, context &ec, unsigned long data)
  {
    sint_type value2 = extsb(ec.fetch(word_size(), 2));
    Destination ea1(op & 0x7, 2 + 2);
#ifdef TRACE_INSTRUCTIONS
    L(" cmpib #0x%x", uint_type(value2));
    L(",%s\n", ea1.textb(ec));
#endif

    sint_type value1 = ea1.getb(ec);
    sint_type value = extsb(value1 - value2);
    ec.regs.ccr.set_cc_cmp(value, value1, value2);
    ea1.finishb(ec);

    ec.regs.pc += 2 + 2 + ea1.isize(2);
  }

  template <class Destination> void
  cmpiw(uint_type op, context &ec, unsigned long data)
  {
    sint_type value2 = extsw(ec.fetch(word_size(), 2));
    Destination ea1(op & 0x7, 2 + 2);
#ifdef TRACE_INSTRUCTIONS
    L(" cmpiw #0x%x", uint_type(value2));
    L(",%s\n", ea1.textw(ec));
#endif

    sint_type value1 = ea1.getw(ec);
    sint_type value = extsw(value1 - value2);
    ec.regs.ccr.set_cc_cmp(value, value1, value2);
    ea1.finishw(ec);

    ec.regs.pc += 2 + 2 + ea1.isize(2);
  }

  template <class Destination> void
  cmpil(uint_type op, context &c, unsigned long data)
  {
    sint32_type value2 = extsl(c.fetch(long_word_size(), 2));
    Destination ea1(op & 0x7, 2 + 4);
#ifdef TRACE_INSTRUCTIONS
    L(" cmpil #%#lx", (unsigned long) value2);
    L(",%s\n", ea1.textl(c));
#endif

    sint32_type value1 = ea1.getl(c);
    sint32_type value = extsl(value1 - value2);
    c.regs.ccr.set_cc_cmp(value, value1, value2);
    ea1.finishl(c);

    c.regs.pc += 2 + 4 + ea1.isize(4);
  }

  void
  cmpmb(uint_type op, context &c, unsigned long data)
  {
    postinc_indirect ea1(op & 0x7, 2);
    postinc_indirect ea2(op >> 9 & 0x7, 2 + ea1.isize(2));
#ifdef TRACE_INSTRUCTIONS
    L(" cmpb %s", ea1.textb(c));
    L(",%s\n", ea2.textb(c));
#endif

    sint_type value1 = ea1.getb(c);
    sint_type value2 = ea2.getb(c);
    sint_type value = extsb(value2 - value1);
    c.regs.ccr.set_cc_cmp(value, value2, value1);
    ea1.finishb(c);
    ea2.finishb(c);

    c.regs.pc += 2 + ea1.isize(2) + ea2.isize(2);
  }

  template <class Source> void
  divuw(uint_type op, context &ec, unsigned long data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef TRACE_INSTRUCTIONS
    L(" divuw %s", ea1.textw(ec));
    L(",%%d%u\n", reg2);
#endif

    sint_type value1 = ea1.getw(ec);
    sint32_type value2 = extsw(ec.regs.d[reg2]);
    sint32_type value = uint32_type(value2) / (uint32_type(value1) & 0xffffu);
    sint32_type rem = uint32_type(value2) % (uint32_type(value1) & 0xffffu);
    ec.regs.d[reg2] = uint32_type(rem) << 16 | uint32_type(value) & 0xffffu;
    ec.regs.ccr.set_cc(value); // FIXME.
    ea1.finishw(ec);

    ec.regs.pc += 2 + ea1.isize(2);
  }

  template <class Destination> void
  eorb_r(uint_type op, context &ec, unsigned long data)
  {
    Destination ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef TRACE_INSTRUCTIONS
    L(" eorb %%d%u", reg2);
    L(",%s\n", ea1.textb(ec));
#endif

    sint_type value1 = ea1.getb(ec);
    sint_type value2 = extsb(ec.regs.d[reg2]);
    sint_type value = extsb(uint_type(value1) ^ uint_type(value2));
    ea1.putb(ec, value);
    ec.regs.ccr.set_cc(value);
    ea1.finishb(ec);

    ec.regs.pc += 2 + ea1.isize(2);
  }

  template <class Destination> void
  eorw_r(uint_type op, context &ec, unsigned long data)
  {
    Destination ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef TRACE_INSTRUCTIONS
    L(" eorw %%d%u", reg2);
    L(",%s\n", ea1.textw(ec));
#endif

    sint_type value1 = ea1.getw(ec);
    sint_type value2 = extsw(ec.regs.d[reg2]);
    sint_type value = extsw(uint_type(value1) ^ uint_type(value2));
    ea1.putw(ec, value);
    ec.regs.ccr.set_cc(value);
    ea1.finishw(ec);

    ec.regs.pc += 2 + ea1.isize(2);
  }

  template <class Destination> void
  eorl_r(uint_type op, context &c, unsigned long data)
  {
    data_register ea2(op >> 9 & 0x7, 2);
    Destination ea1(op & 0x7, 2 + ea2.isize(4));
#ifdef TRACE_INSTRUCTIONS
    L(" eorl %s", ea2.textl(c));
    L(",%s\n", ea1.textl(c));
#endif

    sint32_type value2 = ea2.getl(c);
    sint32_type value1 = ea1.getw(c);
    sint32_type value = extsw(uint_type(value1) ^ uint_type(value2));
    ea1.putl(c, value);
    c.regs.ccr.set_cc(value);
    ea2.finishl(c);
    ea1.finishl(c);

    c.regs.pc += 2 + ea2.isize(4) + ea1.isize(4);
  }

  /* Handles an EORI instruction.  */
  template <class Size, class Destination> void
  m68k_eori(uint_type op, context &c, unsigned long data)
  {
    typedef typename Size::uvalue_type uvalue_type;
    typedef typename Size::svalue_type svalue_type;

    svalue_type value2 = Size::svalue(c.fetch(Size(), 2));
    Destination ea1(op & 0x7, 2 + Size::aligned_value_size());
#ifdef TRACE_INSTRUCTIONS
    L("\teori%s\t", Size::suffix());
    L("#%#lx,", (unsigned long) Size::get(value2));
    L("%s\n", ea1.text(c));
#endif

    svalue_type value1 = ea1.get(c);
    svalue_type value = Size::svalue(Size::get(value1) ^ Size::get(value2));
    ea1.put(c, value);
    c.regs.ccr.set_cc(value);

    ea1.finish(c);
    c.regs.pc += 2 + Size::aligned_value_size() + ea1.extension_size();
  }

#if 0
  template <class Destination> void
  eoriw(uint_type op, context &ec, unsigned long data)
  {
    sint_type value2 = extsw(ec.fetch(word_size(), 2));
    Destination ea1(op & 0x7, 2 + 2);
#ifdef TRACE_INSTRUCTIONS
    L(" eoriw #0x%x", uint_type(value2));
    L(",%s\n", ea1.textw(ec));
#endif

    sint_type value1 = ea1.getw(ec);
    sint_type value = extsw(uint_type(value1) ^ uint_type(value2));
    ea1.putw(ec, value);
    ec.regs.ccr.set_cc(value);
    ea1.finishw(ec);

    ec.regs.pc += 2 + 2 + ea1.isize(2);
  }
#endif

  /* Handles a DBcc instruction.  */
  template <class Condition> void
  m68k_db(uint_type op, context &c, unsigned long data)
  {
    Condition cond;
    unsigned int reg1 = op & 0x7;
    sint_type disp = word_size::svalue(c.fetch(word_size(), 2));
#ifdef TRACE_INSTRUCTIONS
    L(" db%s %%d%u,", cond.text(), reg1);
    L("%#lx\n", (unsigned long) (c.regs.pc + 2 + disp));
#endif

    // This instruction does not affect the condition codes.
    if (cond(c))
      c.regs.pc += 2 + 2;
    else
      {
	sint_type value = word_size::svalue(word_size::get(c.regs.d[reg1]));
	value = word_size::svalue(word_size::get(value - 1));
	word_size::put(c.regs.d[reg1], value);
	c.regs.pc += 2 + (value != -1 ? disp : 2);
      }
  }

#if 0
  void
  dbf(uint_type op, context &ec, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    int disp = extsw(ec.fetch(word_size(), 2));
#ifdef TRACE_INSTRUCTIONS
    L(" dbf %%d%d", reg1);
    L(",0x%lx\n", (unsigned long) (ec.regs.pc + 2 + disp));
#endif

    // The condition codes are not affected by this instruction.
    sint_type value = extsw(ec.regs.d[reg1]) - 1;
    const uint32_type MASK = 0xffffu;
    ec.regs.d[reg1] = ec.regs.d[reg1] & ~MASK | uint32_type(value) & MASK;
    ec.regs.pc += 2 + (value != -1 ? disp : 2);
  }
#endif

  template <class Register2, class Register1> void
  exgl(uint_type op, context &c, unsigned long data)
  {
    Register1 ea1(op & 0x7, 2);
    Register2 ea2(op >> 9 & 0x7, 2 + ea1.isize(4));
#ifdef TRACE_INSTRUCTIONS
    L(" exgl %s", ea2.textl(c));
    L(",%s\n", ea1.textl(c));
#endif

    // The condition codes are not affected by this instruction.
    sint32_type value = ea1.getl(c);
    sint32_type xvalue = ea2.getl(c);
    ea1.putl(c, xvalue);
    ea2.putl(c, value);
    ea1.finishl(c);
    ea2.finishl(c);

    c.regs.pc += 2 + ea1.isize(4) + ea2.isize(4);
  }

  void
  extw(uint_type op, context &c, unsigned long data)
  {
    data_register ea1(op & 0x7, 2);
#ifdef TRACE_INSTRUCTIONS
    L(" extw %s\n", ea1.textw(c));
#endif

    sint_type value = ea1.getb(c);
    ea1.putw(c, value);
    c.regs.ccr.set_cc(value);
    ea1.finishw(c);

    c.regs.pc += 2 + ea1.isize(2);
  }

  void
  extl(uint_type op, context &ec, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
#ifdef TRACE_INSTRUCTIONS
    L(" extl %%d%u\n", reg1);
#endif

    sint32_type value = extsw(ec.regs.d[reg1]);
    ec.regs.d[reg1] = value;
    ec.regs.ccr.set_cc(value);

    ec.regs.pc += 2;
  }

  template <class Destination> void
  jmp(uint_type op, context &c, unsigned long data)
  {
    Destination ea1(op & 0x7, 2);
#ifdef TRACE_INSTRUCTIONS
    L(" jmp %s\n", ea1.textw(c));
#endif

    // The condition codes are not affected by this instruction.
    uint32_type address = ea1.address(c);

    c.regs.pc = address;
  }

  template <class Destination> void
  jsr(uint_type op, context &ec, unsigned long data)
    {
      Destination ea1(op & 0x7, 2);
#ifdef TRACE_INSTRUCTIONS
      L(" jsr %s\n", ea1.textw(ec));
#endif

      // XXX: The condition codes are not affected.
      uint32_type address = ea1.address(ec);
      int fc = ec.data_fc();
      ec.mem->putl(fc, ec.regs.a[7] - 4, ec.regs.pc + 2 + ea1.isize(0));
      ec.regs.a[7] -= 4;
      ec.regs.pc = address;
    }

  template <class Destination> void
  lea(uint_type op, context &ec, unsigned long data)
    {
      Destination ea1(op & 0x7, 2);
      int reg2 = op >> 9 & 0x7;
#ifdef TRACE_INSTRUCTIONS
      L(" lea %s", ea1.textw(ec));
      L(",%%a%d\n", reg2);
#endif

      // XXX: The condition codes are not affected.
      uint32_type address = ea1.address(ec);
      ec.regs.a[reg2] = address;

      ec.regs.pc += 2 + ea1.isize(0);
    }

  void
  link_a(uint_type op, context &ec, unsigned long data)
    {
      int reg = op & 0x0007;
      int disp = extsw(ec.fetch(word_size(), 2));
#ifdef TRACE_INSTRUCTIONS
      VL((" link %%a%d,#%d\n", reg, disp));
#endif

      // XXX: The condition codes are not affected.
      int fc = ec.data_fc();
      ec.mem->putl(fc, ec.regs.a[7] - 4, ec.regs.a[reg]);
      ec.regs.a[7] -= 4;
      ec.regs.a[reg] = ec.regs.a[7];
      ec.regs.a[7] += disp;

      ec.regs.pc += 4;
    }

  void
  lslb_i(uint_type op, context &ec, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int count = op >> 9 & 0x7;
    if (count == 0)
      count = 8;
#ifdef TRACE_INSTRUCTIONS
    L(" lslb #%u", count);
    L(",%%d%u\n", reg1);
#endif

    sint_type value1 = extsb(ec.regs.d[reg1]);
    sint_type value = extsb(uint_type(value1) << count);
    const uint32_type MASK = ((uint32_type) 1u << 8) - 1;
    ec.regs.d[reg1] = ec.regs.d[reg1] & ~MASK | uint32_type(value) & MASK;
    ec.regs.ccr.set_cc_lsl(value, value1, count + (32 - 8));

    ec.regs.pc += 2;
  }

  void
  lslw_i(uint_type op, context &ec, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int count = op >> 9 & 0x7;
    if (count == 0)
      count = 8;
#ifdef TRACE_INSTRUCTIONS
    L(" lslw #%u", count);
    L(",%%d%u\n", reg1);
#endif

    sint_type value1 = extsw(ec.regs.d[reg1]);
    sint_type value = extsw(uint_type(value1) << count);
    const uint32_type MASK = ((uint32_type) 1u << 16) - 1;
    ec.regs.d[reg1] = ec.regs.d[reg1] & ~MASK | uint32_type(value) & MASK;
    ec.regs.ccr.set_cc_lsl(value, value1, count + (32 - 16));

    ec.regs.pc += 2;
  }

  void
  lslw_r(uint_type op, context &ec, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef TRACE_INSTRUCTIONS
    L(" lslw %%d%u", reg2);
    L(",%%d%u\n", reg1);
#endif

    uint_type count = ec.regs.d[reg2];
    sint_type value1 = extsw(ec.regs.d[reg1]);
    sint_type value = extsw(uint_type(value1) << count);
    const uint32_type MASK = ((uint32_type) 1u << 16) - 1;
    ec.regs.d[reg1] = ec.regs.d[reg1] & ~MASK | uint32_type(value) & MASK;
    ec.regs.ccr.set_cc_lsl(value, value1, count + (32 - 16));

    ec.regs.pc += 2;
  }

  void
  lsll_i(uint_type op, context &ec, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int count = op >> 9 & 0x7;
    if (count == 0)
      count = 8;
#ifdef TRACE_INSTRUCTIONS
    L(" lsll #%u", count);
    L(",%%d%u\n", reg1);
#endif

    sint32_type value1 = extsl(ec.regs.d[reg1]);
    sint32_type value = extsl(uint32_type(value1) << count);
    ec.regs.d[reg1] = value;
    ec.regs.ccr.set_cc_lsl(value, value1, count);

    ec.regs.pc += 2;
  }

  void
  lsll_r(uint_type op, context &ec, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef TRACE_INSTRUCTIONS
    L(" lsll %%d%u", reg2);
    L(",%%d%u\n", reg1);
#endif

    uint_type count = ec.regs.d[reg2];
    sint32_type value1 = extsl(ec.regs.d[reg1]);
    sint32_type value = extsl(uint_type(value1) << count);
    ec.regs.d[reg1] = value;
    ec.regs.ccr.set_cc_lsl(value, value1, count);

    ec.regs.pc += 2;
  }

  void
  lsrb_i(uint_type op, context &c, unsigned long data)
  {
    unsigned int count = op >> 9 & 0x7;
    if (count == 0)
      count = 8;
    data_register ea1(op & 0x7, 2);
#ifdef TRACE_INSTRUCTIONS
    L(" lsrb #%u", count);
    L(",%s\n", ea1.textb(c));
#endif

    sint_type value1 = ea1.getb(c);
    sint_type value = extsb((uint_type(value1) & 0xffu) >> count);
    ea1.putb(c, value);
    c.regs.ccr.set_cc_lsr(value, value1, count);

    c.regs.pc += 2 + ea1.isize(2);
  }

  void
  lsrw_i(uint_type op, context &ec, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    uint_type count = op >> 9 & 0x7;
    if (count == 0)
      count = 8;
#ifdef TRACE_INSTRUCTIONS
    L(" lsrw #%u", count);
    L(",%%d%u\n", reg1);
#endif

    sint_type value1 = extsw(ec.regs.d[reg1]);
    sint_type value = extsw((uint_type(value1) & 0xffffu) >> count);
    const uint32_type MASK = (uint32_type(1) << 16) - 1;
    ec.regs.d[reg1] = ec.regs.d[reg1] & ~MASK | uint32_type(value) & MASK;
    ec.regs.ccr.set_cc_lsr(value, value1, count);

    ec.regs.pc += 2;
  }

  void
  lsrw_r(uint_type op, context &ec, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef TRACE_INSTRUCTIONS
    L(" lsrw %%d%u", reg2);
    L(",%%d%u\n", reg1);
#endif

    uint_type count = ec.regs.d[reg2];
    sint_type value1 = extsw(ec.regs.d[reg1]);
    sint_type value = extsw((uint_type(value1) & 0xffffu) >> count);
    const uint32_type MASK = (uint32_type(1) << 16) - 1;
    ec.regs.d[reg1] = ec.regs.d[reg1] & ~MASK | uint32_type(value) & MASK;
    ec.regs.ccr.set_cc_lsr(value, value1, count);

    ec.regs.pc += 2;
  }

  void
  lsrl_i(uint_type op, context &ec, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    uint_type count = op >> 9 & 0x7;
    if (count == 0)
      count = 8;
#ifdef TRACE_INSTRUCTIONS
    L(" lsrl #%u", count);
    L(",%%d%u\n", reg1);
#endif

    sint32_type value1 = extsl(ec.regs.d[reg1]);
    sint32_type value = extsl((uint32_type(value1) & 0xffffffffu) >> count);
    ec.regs.d[reg1] = value;
    ec.regs.ccr.set_cc_lsr(value, value1, count);

    ec.regs.pc += 2;
  }

  void
  lsrl_r(uint_type op, context &ec, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef TRACE_INSTRUCTIONS
    L(" lsrl %%d%u", reg2);
    L(",%%d%u\n", reg1);
#endif

    uint_type count = ec.regs.d[reg2];
    sint32_type value1 = extsl(ec.regs.d[reg1]);
    sint32_type value = extsl((uint32_type(value1) & 0xffffffffu) >> count);
    ec.regs.d[reg1] = value;
    ec.regs.ccr.set_cc_lsr(value, value1, count);

    ec.regs.pc += 2;
  }

  template <class Size, class Source, class Destination> void
  m68k_move(uint_type op, context &c, unsigned long data)
  {
    typedef typename Size::uvalue_type uvalue_type;
    typedef typename Size::svalue_type svalue_type;

    Source ea1(op & 0x7, 2);
    Destination ea2(op >> 9 & 0x7, 2 + ea1.extension_size());
#ifdef TRACE_INSTRUCTIONS
    L("\tmove%s\t", Size::suffix());
    L("%s,", ea1.text(c));
    L("%s\n", ea2.text(c));
#endif

    svalue_type value = ea1.get(c);
    ea2.put(c, value);
    c.regs.ccr.set_cc(value);

    ea1.finish(c);
    ea2.finish(c);
    c.regs.pc += 2 + ea1.extension_size() + ea2.extension_size();
  }

#if 0
  template <class Source, class Destination> void
  moveb(uint_type op, context &ec, unsigned long data)
  {
    Source ea1(op & 0x7, 2);
    Destination ea2(op >> 9 & 0x7, 2 + ea1.isize(2));
#ifdef TRACE_INSTRUCTIONS
    L(" moveb %s", ea1.textb(ec));
    L(",%s\n", ea2.textb(ec));
#endif

    sint_type value = ea1.getb(ec);
    ea2.putb(ec, value);
    ec.regs.ccr.set_cc(value);
    ea1.finishb(ec);
    ea2.finishb(ec);

    ec.regs.pc += 2 + ea1.isize(2) + ea2.isize(2);
  }

  template <class Source, class Destination> void
  movew(uint_type op, context &ec, unsigned long data)
  {
    Source ea1(op & 0x7, 2);
    Destination ea2(op >> 9 & 0x7, 2 + ea1.isize(2));
#ifdef TRACE_INSTRUCTIONS
    L(" movew %s", ea1.textw(ec));
    L(",%s\n", ea2.textw(ec));
#endif

    sint_type value = ea1.getw(ec);
    ea2.putw(ec, value);
    ec.regs.ccr.set_cc(value);
    ea1.finishw(ec);
    ea2.finishw(ec);

    ec.regs.pc += 2 + ea1.isize(2) + ea2.isize(2);
  }

  template <class Source, class Destination> void
  movel(uint_type op, context &ec, unsigned long data)
    {
      Source ea1(op & 0x7, 2);
      Destination ea2(op >> 9 & 0x7, 2 + ea1.isize(4));
#ifdef TRACE_INSTRUCTIONS
      VL((" movel %s", ea1.textl(ec)));
      VL((",%s\n", ea2.textl(ec)));
#endif

      sint32_type value = ea1.getl(ec);
      ea2.putl(ec, value);
      ea1.finishl(ec);
      ea2.finishl(ec);
      ec.regs.ccr.set_cc(value);

      ec.regs.pc += 2 + ea1.isize(4) + ea2.isize(4);
    }
#endif /* 0 */

  /* Handles a MOVE-from-SR instruction.  */
  template <class Destination> void
  m68k_move_from_sr(uint_type op, context &c, unsigned long data)
  {
    typedef word_size::uvalue_type uvalue_type;
    typedef word_size::svalue_type svalue_type;

    Destination ea1(op & 0x7, 2);
#ifdef TRACE_INSTRUCTIONS
    L("\tmove%s\t", word_size::suffix());
    L("%%sr,");
    L("%s\n", ea1.text(c));
#endif

    // This instruction is not privileged on MC68000.
    // This instruction does not affect the condition codes.
    uvalue_type value = c.sr();
    ea1.put(c, value);
    ea1.finish(c);

    c.regs.pc += 2 + ea1.extension_size();
  }

  /* Handles a MOVE-to-SR instruction.  */
  template <class Source> void
  m68k_move_to_sr(uint_type op, context &c, unsigned long data)
  {
    typedef word_size::uvalue_type uvalue_type;
    typedef word_size::svalue_type svalue_type;

    Source ea1(op & 0x7, 2);
#ifdef TRACE_INSTRUCTIONS
    L("\tmove%s\t", word_size::suffix());
    L("%s,", ea1.text(c));
    L("%%sr\n");
#endif

    // This instruction is privileged.
    if (!c.supervisor_state())
      throw privilege_violation_exception();

    // This instruction sets the condition codes.
    uvalue_type value = ea1.get(c);
    c.set_sr(value);
    ea1.finish(c);

    c.regs.pc += 2 + ea1.extension_size();
  }

  /* Handles a MOVE-from-USP instruction.  */
  void
  m68k_move_from_usp(uint_type op, context &c, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
#ifdef TRACE_INSTRUCTIONS
    L(" movel %%usp,");
    L("%%a%u\n", reg1);
#endif

    // This instruction is privileged.
    if (!c.supervisor_state())
      throw privilege_violation_exception();

    // The condition codes are not affected by this instruction.
    c.regs.a[reg1] = c.regs.usp;

    c.regs.pc += 2;
  }

  /* Handles a MOVE-to-USP instruction.  */
  void
  m68k_move_to_usp(uint_type op, context &c, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
#ifdef TRACE_INSTRUCTIONS
    L("\tmove%s\t", long_word_size::suffix());
    L("%%a%u,", reg1);
    L("%%usp\n");
#endif

    // This instruction is privileged.
    if (!c.supervisor_state())
      throw privilege_violation_exception();

    // This instruction does not affect the condition codes.
    long_word_size::put(c.regs.usp, long_word_size::get(c.regs.a[reg1]));

    c.regs.pc += 2;
  }

  /* Handles a MOVEA instruction.  */
  template <class Size, class Source> void
  m68k_movea(uint_type op, context &c, unsigned long data)
  {
    typedef long_word_size::uvalue_type uvalue_type;
    typedef long_word_size::svalue_type svalue_type;

    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef TRACE_INSTRUCTIONS
    L(" movea%s %s", Size::suffix(), ea1.text(c));
    L(",%%a%u\n", reg2);
#endif

    // The condition codes are not affected by this instruction.
    svalue_type value = ea1.get(c);
    long_word_size::put(c.regs.a[reg2], value);
    ea1.finish(c);

    c.regs.pc += 2 + ea1.extension_size();
  }

#if 0
  template <class Source> void
  moveaw(uint_type op, context &c, unsigned long data)
  {
    Source ea1(op & 0x7, 2);
    address_register ea2(op >> 9 & 0x7, 2 + ea1.isize(2));
#ifdef TRACE_INSTRUCTIONS
    L(" moveaw %s", ea1.textw(c));
    L(",%s\n", ea2.textw(c));
#endif

    // The condition codes are not affected by this instruction.
    sint32_type value = ea1.getw(c);
    ea2.putl(c, value);
    ea1.finishw(c);
    ea2.finishw(c);

    c.regs.pc += 2 + ea1.isize(2) + ea2.isize(2);
  }

  template <class Source> void
  moveal(uint_type op, context &ec, unsigned long data)
    {
      Source ea1(op & 0x7, 2);
      address_register ea2(op >> 9 & 0x7, 2 + ea1.isize(4));
#ifdef TRACE_INSTRUCTIONS
      VL((" moveal %s", ea1.textl(ec)));
      VL((",%s\n", ea2.textl(ec)));
#endif

      // XXX: The condition codes are not affected by this
      // instruction.
      sint32_type value = ea1.getl(ec);
      ea2.putl(ec, value);
      ea1.finishl(ec);
      ea2.finishl(ec);

      ec.regs.pc += 2 + ea1.isize(4) + ea2.isize(4);
    }
#endif

  /* Handles a MOVEM instruction (register to memory) */
  template <class Size, class Destination> void
  m68k_movem_r_m(uint_type op, context &c, unsigned long data)
  {
    uint_type mask = c.fetch(word_size(), 2);
    Destination ea1(op & 0x7, 2 + 2);
#ifdef TRACE_INSTRUCTIONS
    L(" movem%s %#06x,", Size::suffix(), mask);
    L("%s\n", ea1.text(c));
#endif

    // This instruction does not affect the condition codes.
    uint_type m = 1;
    int fc = c.data_fc();
    uint32_type address = ea1.address(c);
    for (uint32_type *i = c.regs.d + 0; i != c.regs.d + 8; ++i)
      {
	if (mask & m)
	  {
	    Size::put(*c.mem, fc, address, Size::get(*i));
	    address += Size::value_size();
	  }
	m <<= 1;
      }
    for (uint32_type *i = c.regs.a + 0; i != c.regs.a + 8; ++i)
      {
	if (mask & m)
	  {
	    Size::put(*c.mem, fc, address, Size::get(*i));
	    address += Size::value_size();
	  }
	m <<= 1;
      }

    c.regs.pc += 2 + 2 + ea1.extension_size();
  }

  /* movem regs to EA (postdec).  */
  void
  moveml_r_predec(uint_type op, context &ec, unsigned long data)
    {
      int reg = op & 0x0007;
      unsigned int bitmap = ec.fetch(word_size(), 2);
#ifdef TRACE_INSTRUCTIONS
      VL((" moveml #0x%x,%%a%d@-\n", bitmap, reg));
#endif

      // XXX: The condition codes are not affected.
      uint32_type address = ec.regs.a[reg];
      int fc = ec.data_fc();
      for (uint32_type *i = ec.regs.a + 8; i != ec.regs.a + 0; --i)
	{
	  if (bitmap & 1 != 0)
	    {
	      ec.mem->putl(fc, address - 4, *(i - 1));
	      address -= 4;
	    }
	  bitmap >>= 1;
	}
      for (uint32_type *i = ec.regs.d + 8; i != ec.regs.d + 0; --i)
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
  moveml_mr(uint_type op, context &ec, unsigned long data)
    {
      Source ea1(op & 0x7, 4);
      unsigned int bitmap = ec.fetch(word_size(), 2);
#ifdef TRACE_INSTRUCTIONS
      L(" moveml %s", ea1.textl(ec));
      L(",#0x%04x\n", bitmap);
#endif

      // XXX: The condition codes are not affected.
      uint32_type address = ea1.address(ec);
      int fc = ec.data_fc();
      for (uint32_type *i = ec.regs.d + 0; i != ec.regs.d + 8; ++i)
	{
	  if (bitmap & 1 != 0)
	    {
	      *i = ec.mem->getl(fc, address);
	      address += 4;
	    }
	  bitmap >>= 1;
	}
      for (uint32_type *i = ec.regs.a + 0; i != ec.regs.a + 8; ++i)
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
  moveml_mr<postinc_indirect>(uint_type op, context &ec, unsigned long data)
    {
      int reg1 = op & 0x7;
      unsigned int bitmap = ec.fetch(word_size(), 2);
#ifdef TRACE_INSTRUCTIONS
      L(" moveml %%a%d@+", reg1);
      L(",#0x%04x\n", bitmap);
#endif

      // XXX: The condition codes are not affected.
      uint32_type address = ec.regs.a[reg1];
      int fc = ec.data_fc();
      for (uint32_type *i = ec.regs.d + 0; i != ec.regs.d + 8; ++i)
	{
	  if (bitmap & 1 != 0)
	    {
	      *i = ec.mem->getl(fc, address);
	      address += 4;
	    }
	  bitmap >>= 1;
	}
      for (uint32_type *i = ec.regs.a + 0; i != ec.regs.a + 8; ++i)
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
  moveql_d(uint_type op, context &ec, unsigned long data)
    {
      int value = extsb(op & 0xff);
      int reg = op >> 9 & 0x7;
#ifdef TRACE_INSTRUCTIONS
      VL((" moveql #%d,%%d%d\n", value, reg));
#endif
      
      ec.regs.d[reg] = value;
      ec.regs.ccr.set_cc(value);

      ec.regs.pc += 2;
    }

  template <class Source> void
  mulsw(uint_type op, context &ec, unsigned long data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef TRACE_INSTRUCTIONS
    L(" mulsw %s", ea1.textw(ec));
    L(",%%d%u\n", reg2);
#endif

    sint_type value1 = ea1.getw(ec);
    sint_type value2 = extsw(ec.regs.d[reg2]);
    sint32_type value = sint32_type(value2) * sint32_type(value1);
    ec.regs.d[reg2] = value;
    ec.regs.ccr.set_cc(value); // FIXME.
    ea1.finishw(ec);

    ec.regs.pc += 2 + ea1.isize(2);
  }

  template <class Source> void
  muluw(uint_type op, context &ec, unsigned long data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef TRACE_INSTRUCTIONS
    L(" muluw %s", ea1.textw(ec));
    L(",%%d%u\n", reg2);
#endif

    sint_type value1 = ea1.getw(ec);
    sint_type value2 = extsw(ec.regs.d[reg2]);
    sint32_type value
      = (uint32_type(value2) & 0xffffu) * (uint32_type(value1) & 0xffffu);
    ec.regs.d[reg2] = value;
    ec.regs.ccr.set_cc(value); // FIXME.
    ea1.finishw(ec);

    ec.regs.pc += 2 + ea1.isize(2);
  }

  template <class Destination> void
  negb(uint_type op, context &c, unsigned long data)
  {
    Destination ea1(op & 0x7, 2);
#ifdef TRACE_INSTRUCTIONS
    L(" negb %s\n", ea1.textb(c));
#endif

    sint_type value1 = ea1.getb(c);
    sint_type value = extsb(-value1);
    ea1.putb(c, value);
    c.regs.ccr.set_cc_sub(value, 0, value1);
    ea1.finishb(c);

    c.regs.pc += 2 + ea1.isize(2);
  }

  template <class Destination> void
  negw(uint_type op, context &ec, unsigned long data)
  {
    Destination ea1(op & 0x7, 2);
#ifdef TRACE_INSTRUCTIONS
    L(" negw %s\n", ea1.textw(ec));
#endif

    sint_type value1 = ea1.getw(ec);
    sint_type value = extsw(-value1);
    ea1.putw(ec, value);
    ec.regs.ccr.set_cc_sub(value, 0, value1);
    ea1.finishw(ec);

    ec.regs.pc += 2 + ea1.isize(2);
  }

  template <class Destination> void
  negl(uint_type op, context &ec, unsigned long data)
  {
    Destination ea1(op & 0x7, 2);
#ifdef TRACE_INSTRUCTIONS
    L(" negl %s\n", ea1.textl(ec));
#endif

    sint32_type value1 = ea1.getl(ec);
    sint32_type value = extsl(-value1);
    ea1.putl(ec, value);
    ec.regs.ccr.set_cc_sub(value, 0, value1);
    ea1.finishl(ec);

    ec.regs.pc += 2 + ea1.isize(4);
  }

  /* Handles a NOT instruction.  */
  template <class Size, class Destination> void
  m68k_not(uint_type op, context &c, unsigned long data)
  {
    typedef typename Size::uvalue_type uvalue_type;
    typedef typename Size::svalue_type svalue_type;

    Destination ea1(op & 0x7, 2);
#ifdef TRACE_INSTRUCTIONS
    L("\tnot%s\t", Size::suffix());
    L("%s\n", ea1.text(c));
#endif

    svalue_type value1 = ea1.get(c);
    svalue_type value = Size::svalue(Size::get(~Size::get(value1)));
    ea1.put(c, value);
    c.regs.ccr.set_cc(value);

    ea1.finish(c);
    c.regs.pc += 2 + ea1.extension_size();
  }

  /* Handles a NOP instruction.  */
  void
  m68k_nop(uint_type op, context &c, unsigned long data)
  {
#ifdef TRACE_INSTRUCTIONS
    L("\tnop\n");
#endif

    c.regs.pc += 2;
  }

  template <class Source> void
  orb(uint_type op, context &ec, unsigned long data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef TRACE_INSTRUCTIONS
    L(" orb %s", ea1.textb(ec));
    L(",%%d%u\n", reg2);
#endif

    sint_type value1 = ea1.getb(ec);
    sint_type value2 = extsb(ec.regs.d[reg2]);
    sint_type value = extsb(uint_type(value2) | uint_type(value1));
    ec.regs.d[reg2]
      = ec.regs.d[reg2] & ~0xff | uint32_type(value) & 0xff;
    ec.regs.ccr.set_cc(value);
    ea1.finishb(ec);

    ec.regs.pc += 2 + ea1.isize(2);
  }

  template <class Source> void
  orw(uint_type op, context &ec, unsigned long data)
  {
    Source ea1(op & 0x7, 2);
    int reg2 = op >> 9 & 0x7;
#ifdef TRACE_INSTRUCTIONS
    L(" orw %s", ea1.textw(ec));
    L(",%%d%d\n", reg2);
#endif

    uint_type value1 = ea1.getw(ec);
    uint_type value2 = ec.regs.d[reg2];
    uint_type value = value2 | value1;
    ec.regs.d[reg2]
      = ec.regs.d[reg2] & ~0xffff | uint32_type(value) & 0xffff;
    ec.regs.ccr.set_cc(value);
    ea1.finishw(ec);

    ec.regs.pc += 2 + ea1.isize(2);
  }

  template <class Source> void
  orl(uint_type op, context &ec, unsigned long data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef TRACE_INSTRUCTIONS
    L(" orl %s", ea1.textl(ec));
    L(",%%d%u\n", reg2);
#endif

    sint32_type value1 = ea1.getl(ec);
    sint32_type value2 = extsl(ec.regs.d[reg2]);
    sint32_type value = extsl(uint32_type(value2) | uint32_type(value1));
    ec.regs.d[reg2] = value;
    ec.regs.ccr.set_cc(value);
    ea1.finishl(ec);

    ec.regs.pc += 2 + ea1.isize(4);
  }

  /* Handles an OR instruction (memory destination).  */
  template <class Size, class Destination> void
  m68k_or_m(uint_type op, context &c, unsigned long data)
  {
    typedef typename Size::uvalue_type uvalue_type;
    typedef typename Size::svalue_type svalue_type;

    Destination ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef TRACE_INSTRUCTIONS
    L("\tor%s\t", Size::suffix());
    L("%%d%u,", reg2);
    L("%s\n", ea1.text(c));
#endif

    svalue_type value2 = Size::svalue(Size::get(c.regs.d[reg2]));
    svalue_type value1 = ea1.get(c);
    svalue_type value = Size::svalue(Size::get(value1) | Size::get(value2));
    ea1.put(c, value);
    c.regs.ccr.set_cc(value);

    ea1.finish(c);
    c.regs.pc += 2 + ea1.extension_size();
  }

  /* Handles an ORI-to-CCR instruction.  */
  void
  m68k_ori_to_ccr(uint_type op, context &c, unsigned long data)
  {
    typedef byte_size::uvalue_type uvalue_type;
    typedef byte_size::svalue_type svalue_type;

    uvalue_type value2 = c.fetch(byte_size(), 2);
#ifdef TRACE_INSTRUCTIONS
    L("\tori%s\t", byte_size::suffix());
    L("#%#x,", value2);
    L("%%ccr\n");
#endif

    uvalue_type value1 = c.regs.ccr & 0xffu;
    uvalue_type value = value1 | value2;
    c.regs.ccr = c.regs.ccr & ~0xffu | value & 0xffu;

    c.regs.pc += 2 + byte_size::aligned_value_size();
  }

  /* Handles an ORI-to-SR instruction.  */
  void
  m68k_ori_to_sr(uint_type op, context &c, unsigned long data)
  {
    typedef word_size::uvalue_type uvalue_type;
    typedef word_size::svalue_type svalue_type;

    uvalue_type value2 = c.fetch(word_size(), 2);
#ifdef TRACE_INSTRUCTIONS
    L("\tori%s\t", word_size::suffix());
    L("#%#x,", value2);
    L("%%sr\n");
#endif

    // This instruction is privileged.
    if (!c.supervisor_state())
      throw privilege_violation_exception();

    uvalue_type value1 = c.sr();
    uvalue_type value = value1 | value2;
    c.set_sr(value);

    c.regs.pc += 2 + word_size::aligned_value_size();
  }

  template <class Destination> void
  orib(uint_type op, context &ec, unsigned long data)
  {
    sint_type value2 = extsb(ec.fetch(word_size(), 2));
    Destination ea1(op & 0x7, 2 + 2);
#ifdef TRACE_INSTRUCTIONS
    L(" orib #0x%x", uint_type(value2) & 0xff);
    L(",%s\n", ea1.textb(ec));
#endif

    sint_type value1 = ea1.getb(ec);
    sint_type value = extsb(uint_type(value1) | uint_type(value2));
    ea1.putb(ec, value);
    ec.regs.ccr.set_cc(value);
    ea1.finishb(ec);

    ec.regs.pc += 2 + 2 + ea1.isize(2);
  }

  template <class Destination> void
  oriw(uint_type op, context &c, unsigned long data)
  {
    sint_type value2 = extsw(c.fetch(word_size(), 2));
    Destination ea1(op & 0x7, 2 + 2);
#ifdef TRACE_INSTRUCTIONS
    L(" oriw #0x%x", uint_type(value2) & 0xffffu);
    L(",%s\n", ea1.textw(c));
#endif

    sint_type value1 = ea1.getw(c);
    sint_type value = extsw(uint_type(value1) | uint_type(value2));
    ea1.putw(c, value);
    c.regs.ccr.set_cc(value);
    ea1.finishw(c);

    c.regs.pc += 2 + 2 + ea1.isize(2);
  }

  template <class Destination> void
  oril(uint_type op, context &c, unsigned long data)
  {
    sint32_type value2 = extsl(c.fetch(long_word_size(), 2));
    Destination ea1(op & 0x7, 2 + 4);
#ifdef TRACE_INSTRUCTIONS
    L(" oril #0x%lx", (unsigned long) value2);
    L(",%s\n", ea1.textl(c));
#endif

    sint32_type value1 = ea1.getl(c);
    sint32_type value = extsl(uint32_type(value1) | uint32_type(value2));
    ea1.putl(c, value);
    c.regs.ccr.set_cc(value);
    ea1.finishl(c);

    c.regs.pc += 2 + 4 + ea1.isize(4);
  }

  template <class Destination> void
  pea(uint_type op, context &ec, unsigned long data)
    {
      Destination ea1(op & 0x7, 2);
#ifdef TRACE_INSTRUCTIONS
      L(" pea %s\n", ea1.textw(ec));
#endif

      // XXX: The condition codes are not affected.
      uint32_type address = ea1.address(ec);
      int fc = ec.data_fc();
      ec.mem->putl(fc, ec.regs.a[7] - 4, address);
      ec.regs.a[7] -= 4;

      ec.regs.pc += 2 + ea1.isize(0);
    }

  /* Handles a ROL instruction (register).  */
  template <class Size> void
  m68k_rol_r(uint_type op, context &c, unsigned long data)
  {
    typedef typename Size::uvalue_type uvalue_type;
    typedef typename Size::svalue_type svalue_type;

    unsigned int reg1 = op & 0x7;
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef TRACE_INSTRUCTIONS
    L("\trol%s\t", Size::suffix());
    L("%%d%u,", reg2);
    L("%%d%u\n", reg1);
#endif

    uvalue_type count = c.regs.d[reg2] % Size::value_bit();
    svalue_type value1 = Size::svalue(Size::get(c.regs.d[reg1]));
    svalue_type value
      = Size::svalue(Size::get(uvalue_type(value1) << count
			       | ((uvalue_type(value1) & Size::value_mask())
				  >> Size::value_bit() - count)));
    Size::put(c.regs.d[reg1], value);
    c.regs.ccr.set_cc(value);	// FIXME.

    c.regs.pc += 2;
  }

#if 0
  void
  rolb_r(uint_type op, context &ec, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef TRACE_INSTRUCTIONS
    L(" rolb %%d%u", reg2);
    L(",%%d%u\n", reg1);
#endif

    uint_type count = ec.regs.d[reg2] & 0x7;
    sint_type value1 = extsb(ec.regs.d[reg1]);
    sint_type value = extsb(uint_type(value1) << count
			    | (uint_type(value1) & 0xff) >> 8 - count);
    const uint32_type MASK = 0xff;
    ec.regs.d[reg1] = ec.regs.d[reg1] & ~MASK | uint32_type(value) & MASK;
    ec.regs.ccr.set_cc(value);	// FIXME.

    ec.regs.pc += 2;
  }
#endif

  void
  rolw_i(uint_type op, context &ec, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int count = op >> 9 & 0x7;
    if (count == 0)
      count = 8;
#ifdef TRACE_INSTRUCTIONS
    L(" rolw #%u", count);
    L(",%%d%u\n", reg1);
#endif

    sint_type value1 = extsw(ec.regs.d[reg1]);
    sint_type value = extsw(uint_type(value1) << count
			    | (uint_type(value1) & 0xffffu) >> 16 - count);
    const uint32_type MASK = 0xffffu;
    ec.regs.d[reg1] = ec.regs.d[reg1] & ~MASK | uint32_type(value) & MASK;
    ec.regs.ccr.set_cc(value);	// FIXME.

    ec.regs.pc += 2;
  }

  void
  roll_i(uint_type op, context &c, unsigned long data)
  {
    unsigned int count = op >> 9 & 0x7;
    if (count == 0)
      count = 8;
    data_register ea1(op & 0x7, 2);
#ifdef TRACE_INSTRUCTIONS
    L(" roll #%u", count);
    L(",%s\n", ea1.textl(c));
#endif

    sint32_type value1 = ea1.getl(c);
    sint32_type value
      = extsl(uint32_type(value1) << count
	      | (uint32_type(value1) & 0xffffffffu) >> 32 - count);
    ea1.putl(c, value);
    c.regs.ccr.set_cc(value);	// FIXME.
    ea1.finishl(c);

    c.regs.pc += 2 + ea1.isize(4);
  }

  /* Handles a ROR instruction (immediate).  */
  template <class Size> void
  m68k_ror_i(uint_type op, context &c, unsigned long data)
  {
    typedef typename Size::uvalue_type uvalue_type;
    typedef typename Size::svalue_type svalue_type;

    unsigned int reg1 = op & 0x7;
    unsigned int count = op >> 9 & 0x7;
    if (count == 0)
      count = 8;
#ifdef TRACE_INSTRUCTIONS
    L(" ror%s #%u,", Size::suffix(), count);
    L("%%d%u\n", reg1);
#endif

    svalue_type value1 = Size::svalue(Size::get(c.regs.d[reg1]));
    svalue_type value
      = Size::svalue(Size::get(((uvalue_type(value1) & Size::value_mask())
				>> count)
			       | (uvalue_type(value1)
				  << Size::value_bit() - count)));
    Size::put(c.regs.d[reg1], value);
    c.regs.ccr.set_cc(value);	// FIXME.

    c.regs.pc += 2;
  }

#if 0
  void
  rorw_i(uint_type op, context &c, unsigned long data)
  {
    unsigned int count = op >> 9 & 0x7;
    if (count == 0)
      count = 8;
    data_register ea1(op & 0x7, 2);
#ifdef TRACE_INSTRUCTIONS
    L(" rorw #%u", count);
    L(",%s\n", ea1.textw(c));
#endif

    sint_type value1 = ea1.getw(c);
    sint_type value = extsw((uint_type(value1) & 0xffffu) >> count
			    | uint_type(value1) << 16 - count);
    ea1.putw(c, value);
    c.regs.ccr.set_cc(value);	// FIXME.
    ea1.finishw(c);

    c.regs.pc += 2 + ea1.isize(2);
  }
#endif

  void
  roxrw_i(uint_type op, context &c, unsigned long data)
  {
    unsigned int count = op >> 9 & 0x7;
    if (count == 0)
      count = 8;
    data_register ea1(op & 0x7, 2);
#ifdef TRACE_INSTRUCTIONS
    L(" roxrw #%u", count);
    L(",%s\n", ea1.textw(c));
#endif

    sint_type value1 = ea1.getw(c);
    sint_type value = extsw((uint_type(value1) & 0xffffu) >> count
			    | c.regs.ccr.x() << 16 - count
			    | uint_type(value1) << 17 - count);
    ea1.putw(c, value);
    c.regs.ccr.set_cc(value);	// FIXME.
    ea1.finishw(c);

    c.regs.pc += 2 + ea1.isize(2);
  }

  void
  roxrl_i(uint_type op, context &c, unsigned long data)
  {
    unsigned int count = op >> 9 & 0x7;
    if (count == 0)
      count = 8;
    data_register ea1(op & 0x7, 2);
#ifdef TRACE_INSTRUCTIONS
    L(" roxrl #%u", count);
    L(",%s\n", ea1.textl(c));
#endif

    sint32_type value1 = ea1.getl(c);
    sint32_type value = extsl((uint32_type(value1) & 0xffffffffu) >> count
			      | uint32_type(c.regs.ccr.x()) << 32 - count
			      | uint32_type(value1) << 33 - count);
    ea1.putl(c, value);
    c.regs.ccr.set_cc(value);	// FIXME.
    ea1.finishl(c);

    c.regs.pc += 2 + ea1.isize(4);
  }

  /* Handles a RTE instruction.  */
  void
  m68k_rte(uint_type op, context &c, unsigned long data)
  {
#ifdef TRACE_INSTRUCTIONS
    L(" rte\n");
#endif

    // This instruction is privileged.
    if (!c.supervisor_state())
      throw privilege_violation_exception();

    uint_type status = c.mem->getw(SUPER_DATA, c.regs.a[7] + 0);
    uint32_type value = c.mem->getl(SUPER_DATA, c.regs.a[7] + 2);
    c.regs.a[7] += 6;
    c.set_sr(status);
    c.regs.pc = value;
  }

  void
  rts(uint_type op, context &ec, unsigned long data)
    {
#ifdef TRACE_INSTRUCTIONS
      VL((" rts\n"));
#endif

      // XXX: The condition codes are not affected.
      int fc = ec.data_fc();
      uint32_type value = ec.mem->getl(fc, ec.regs.a[7]);
      ec.regs.a[7] += 4;
      ec.regs.pc = value;
    }

  template <class Condition, class Destination> void
  s_b(uint_type op, context &ec, unsigned long data)
  {
    Condition cond;
    Destination ea1(op & 0x7, 2);
#ifdef TRACE_INSTRUCTIONS
    L(" s%sb %s\n", cond.text(), ea1.textb(ec));
#endif

    // The condition codes are not affected by this instruction.
    ea1.putb(ec, cond(ec) ? -1 : 0);
    ea1.finishb(ec);

    ec.regs.pc += 2 + ea1.isize(2);
  }

  /* Handles a SUB instruction (memory destination).  */
  template <class Size, class Destination> void
  m68k_sub_m(uint_type op, context &c, unsigned long data)
  {
    typedef typename Size::uvalue_type uvalue_type;
    typedef typename Size::svalue_type svalue_type;

    Destination ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef TRACE_INSTRUCTIONS
    L(" sub%s %%d%u,", Size::suffix(), reg2);
    L("%s\n", ea1.text(c));
#endif

    svalue_type value2 = Size::svalue(Size::get(c.regs.d[reg2]));
    svalue_type value1 = ea1.get(c);
    svalue_type value = Size::svalue(Size::get(value1 - value2));
    ea1.put(c, value);
    c.regs.ccr.set_cc_sub(value, value1, value2);
    ea1.finish(c);

    c.regs.pc += 2 + ea1.extension_size();
  }

  template <class Source> void
  subb(uint_type op, context &ec, unsigned long data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef TRACE_INSTRUCTIONS
    L(" subb %s", ea1.textb(ec));
    L(",%%d%u\n", reg2);
#endif

    sint_type value1 = ea1.getb(ec);
    sint_type value2 = extsb(ec.regs.d[reg2]);
    sint_type value = extsb(value2 - value1);
    const uint32_type MASK = 0xff;
    ec.regs.d[reg2] = ec.regs.d[reg2] & ~MASK | uint32_type(value) & MASK;
    ec.regs.ccr.set_cc_sub(value, value2, value1);
    ea1.finishb(ec);

    ec.regs.pc += 2 + ea1.isize(2);
  }

  template <class Source> void
  subw(uint_type op, context &ec, unsigned long data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef TRACE_INSTRUCTIONS
    L(" subw %s", ea1.textw(ec));
    L(",%%d%u\n", reg2);
#endif

    sint_type value1 = ea1.getw(ec);
    sint_type value2 = extsw(ec.regs.d[reg2]);
    sint_type value = extsw(value2 - value1);
    const uint32_type MASK = 0xffffu;
    ec.regs.d[reg2] = ec.regs.d[reg2] & ~MASK | uint32_type(value) & MASK;
    ec.regs.ccr.set_cc_sub(value, value2, value1);
    ea1.finishw(ec);

    ec.regs.pc += 2 + ea1.isize(2);
  }

  template <class Destination> void
  subl(uint_type op, context &ec, unsigned long data)
    {
      Destination ea1(op & 0x7, 2);
      int reg2 = op >> 9 & 0x7;
#ifdef TRACE_INSTRUCTIONS
      VL((" subl %s", ea1.textl(ec)));
      VL((",%%d%d\n", reg2));
#endif

      sint32_type value1 = ea1.getl(ec);
      sint32_type value2 = extsl(ec.regs.d[reg2]);
      sint32_type value = extsl(value2 - value1);
      ec.regs.d[reg2] = value;
      ea1.finishl(ec);
      ec.regs.ccr.set_cc_sub(value, value2, value1);

      ec.regs.pc += 2 + ea1.isize(4);
    }

#if 0
  template <class Destination> void
  subl_r(uint_type op, context &ec, unsigned long data)
  {
    Destination ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef TRACE_INSTRUCTIONS
    L(" subl %%d%u", reg2);
    L(",%s\n", ea1.textl(ec));
#endif

    sint32_type value2 = extsl(ec.regs.d[reg2]);
    sint32_type value1 = ea1.getl(ec);
    sint32_type value = extsl(value1 - value2);
    ea1.putl(ec, value);
    ec.regs.ccr.set_cc_sub(value, value1, value2);
    ea1.finishl(ec);

    ec.regs.pc += 2 + ea1.isize(4);
  }
#endif

  /* Handles an SUBA instruction.  */
  template <class Size, class Source> void
  m68k_suba(uint_type op, context &c, unsigned long data)
  {
    typedef long_word_size::uvalue_type uvalue_type;
    typedef long_word_size::svalue_type svalue_type;

    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef TRACE_INSTRUCTIONS
    L("\tsuba%s\t", Size::suffix());
    L("%s,", ea1.text(c));
    L("%%a%u\n", reg2);
#endif

    // This instruction does not affect the condition codes.
    svalue_type value1 = ea1.get(c);
    svalue_type value2
      = long_word_size::svalue(long_word_size::get(c.regs.a[reg2]));
    svalue_type value
      = long_word_size::svalue(long_word_size::get(value2 - value1));
    long_word_size::put(c.regs.a[reg2], value);

    ea1.finish(c);
    c.regs.pc += 2 + ea1.extension_size();
  }

#if 0
  template <class Source> void
  subal(uint_type op, context &ec, unsigned long data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef TRACE_INSTRUCTIONS
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
#endif

  template <class Destination> void
  subib(uint_type op, context &ec, unsigned long data)
    {
      int value2 = extsb(ec.fetch(word_size(), 2));
      Destination ea1(op & 0x7, 2 + 2);
#ifdef TRACE_INSTRUCTIONS
      L(" subib #%d", value2);
      L(",%s\n", ea1.textb(ec));
#endif

      int value1 = ea1.getb(ec);
      int value = extsb(value1 - value2);
      ea1.putb(ec, value);
      ea1.finishb(ec);
      ec.regs.ccr.set_cc_sub(value, value1, value2);

      ec.regs.pc += 2 + 2 + ea1.isize(2);
    }

  template <class Destination> void
  subil(uint_type op, context &ec, unsigned long data)
  {
    sint32_type value2 = extsl(ec.fetch(long_word_size(), 2));
    Destination ea1(op & 0x7, 2 + 4);
#ifdef TRACE_INSTRUCTIONS
    L(" subil #%ld", (long) value2);
    L(",%s\n", ea1.textl(ec));
#endif

    sint32_type value1 = ea1.getl(ec);
    sint32_type value = extsl(value1 - value2);
    ea1.putl(ec, value);
    ec.regs.ccr.set_cc_sub(value, value1, value2);
    ea1.finishl(ec);

    ec.regs.pc += 2 + 4 + ea1.isize(4);
  }

  /* Handles a SUBQ instruction.  */
  template <class Size, class Destination> void
  m68k_subq(uint_type op, context &c, unsigned long data)
  {
    typedef typename Size::uvalue_type uvalue_type;
    typedef typename Size::svalue_type svalue_type;

    Destination ea1(op & 0x7, 2);
    int value2 = op >> 9 & 0x7;
    if (value2 == 0)
      value2 = 8;
#ifdef TRACE_INSTRUCTIONS
    L(" subq%s #%d,", Size::suffix(), value2);
    L("%s\n", ea1.text(c));
#endif

    svalue_type value1 = ea1.get(c);
    svalue_type value = Size::svalue(Size::get(value1 - value2));
    ea1.put(c, value);
    c.regs.ccr.set_cc_sub(value, value1, value2);
    ea1.finish(c);

    c.regs.pc += 2 + ea1.extension_size();
  }

  /* Handles a SUBQ instruction (address register).  */
  template <class Size> void
  m68k_subq_a(uint_type op, context &c, unsigned long data)
  {
    typedef long_word_size::uvalue_type uvalue_type;
    typedef long_word_size::svalue_type svalue_type;

    unsigned int reg1 = op & 0x7;
    int value2 = op >> 9 & 0x7;
    if (value2 == 0)
      value2 = 8;
#ifdef TRACE_INSTRUCTIONS
    L(" subq%s #%d,", Size::suffix(), value2);
    L("%%a%u\n", reg1);
#endif

    // This instruction does not affect the condition codes.
    svalue_type value1
      = long_word_size::svalue(long_word_size::get(c.regs.a[reg1]));
    svalue_type value
      = long_word_size::svalue(long_word_size::get(value1 - value2));
    long_word_size::put(c.regs.a[reg1], value);

    c.regs.pc += 2;
  }

#if 0
  template <class Destination> void
  subqw(uint_type op, context &ec, unsigned long data)
  {
    Destination ea1(op & 0x7, 2);
    int value2 = op >> 9 & 0x7;
    if (value2 == 0)
      value2 = 8;
#ifdef TRACE_INSTRUCTIONS
    L(" subqw #%d", value2);
    L(",%s\n", ea1.textw(ec));
#endif

    sint_type value1 = ea1.getw(ec);
    sint_type value = extsw(value1 - value2);
    ea1.putw(ec, value);
    ec.regs.ccr.set_cc_sub(value, value1, value2);
    ea1.finishw(ec);

    ec.regs.pc += 2 + ea1.isize(2);
  }

  template <> void
  subqw<address_register>(uint_type op, context &ec, unsigned long data)
  {
    address_register ea1(op & 0x7, 2);
    int value2 = op >> 9 & 0x7;
    if (value2 == 0)
      value2 = 8;
#ifdef TRACE_INSTRUCTIONS
    L(" subqw #%d", value2);
    L(",%s\n", ea1.textw(ec));
#endif

    // Condition codes are not affected by this instruction.
    sint_type value1 = ea1.getl(ec);
    sint_type value = extsl(value1 - value2);
    ea1.putl(ec, value);
    ea1.finishl(ec);

    ec.regs.pc += 2 + ea1.isize(2);
  }

  template <class Destination> void
  subql(uint_type op, context &ec, unsigned long data)
  {
    Destination ea1(op & 0x7, 2);
    int value2 = op >> 9 & 0x7;
    if (value2 == 0)
      value2 = 8;
#ifdef TRACE_INSTRUCTIONS
    L(" subql #%d", value2);
    L(",%s\n", ea1.textl(ec));
#endif

    sint32_type value1 = ea1.getl(ec);
    sint32_type value = extsl(value1 - value2);
    ea1.putl(ec, value);
    ec.regs.ccr.set_cc_sub(value, value1, value2);
    ea1.finishl(ec);

    ec.regs.pc += 2 + ea1.isize(4);
  }

  template <> void
  subql<address_register>(uint_type op, context &ec, unsigned long data)
  {
    address_register ea1(op & 0x7, 2);
    int value2 = op >> 9 & 0x7;
    if (value2 == 0)
      value2 = 8;
#ifdef TRACE_INSTRUCTIONS
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
#endif

  void
  swapw(uint_type op, context &ec, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
#ifdef TRACE_INSTRUCTIONS
    L(" swapw %%d%u\n", reg1);
#endif

    sint32_type value1 = extsl(ec.regs.d[reg1]);
    sint32_type value
      = extsl(uint32_type(value1) << 16 | uint32_type(value1) >> 16 & 0xffffu);
    ec.regs.d[reg1] = value;
    ec.regs.ccr.set_cc(value);

    ec.regs.pc += 2;
  }

  template <class Destination> void
  tstb(uint_type op, context &ec, unsigned long data)
    {
      Destination ea1(op & 0x7, 2);
#ifdef TRACE_INSTRUCTIONS
      VL((" tstb %s\n", ea1.textb(ec)));
#endif

      int value = ea1.getb(ec);
      ec.regs.ccr.set_cc(value);
      ea1.finishb(ec);

      ec.regs.pc += 2 + ea1.isize(2);
    }

  template <class Destination> void
  tstw(uint_type op, context &ec, unsigned long data)
  {
    Destination ea1(op & 0x7, 2);
#ifdef TRACE_INSTRUCTIONS
    L(" tstw %s\n", ea1.textw(ec));
#endif

    sint_type value = ea1.getw(ec);
    ec.regs.ccr.set_cc(value);
    ea1.finishw(ec);

    ec.regs.pc += 2 + ea1.isize(2);
  }

  template <class Destination> void
  tstl(uint_type op, context &ec, unsigned long data)
    {
      Destination ea1(op & 0x7, 2);
#ifdef TRACE_INSTRUCTIONS
      VL((" tstl %s\n", ea1.textl(ec)));
#endif

      sint32_type value = ea1.getl(ec);
      ec.regs.ccr.set_cc(value);
      ea1.finishl(ec);

      ec.regs.pc += 2 + ea1.isize(4);
    }

  void
  unlk_a(uint_type op, context &ec, unsigned long data)
    {
      int reg = op & 0x0007;
#ifdef TRACE_INSTRUCTIONS
      VL((" unlk %%a%d\n", reg));
#endif

      // XXX: The condition codes are not affected.
      int fc = ec.data_fc();
      uint32_type address = ec.mem->getl(fc, ec.regs.a[reg]);
      ec.regs.a[7] = ec.regs.a[reg] + 4;
      ec.regs.a[reg] = address;

      ec.regs.pc += 2;
    }

  /* Initializes the machine instructions of execution unit EU.  */
  void
  initialize_instructions(exec_unit &eu)
  {
    eu.set_instruction(0x0000, 0x0007, &orib<data_register>);
    eu.set_instruction(0x0010, 0x0007, &orib<indirect>);
    eu.set_instruction(0x0018, 0x0007, &orib<postinc_indirect>);
    eu.set_instruction(0x0020, 0x0007, &orib<predec_indirect>);
    eu.set_instruction(0x0028, 0x0007, &orib<disp_indirect>);
    eu.set_instruction(0x0030, 0x0007, &orib<indexed_indirect>);
    eu.set_instruction(0x0038, 0x0000, &orib<absolute_short>);
    eu.set_instruction(0x0039, 0x0000, &orib<absolute_long>);
    eu.set_instruction(0x003c, 0x0000, &m68k_ori_to_ccr);
    eu.set_instruction(0x0040, 0x0007, &oriw<data_register>);
    eu.set_instruction(0x0050, 0x0007, &oriw<indirect>);
    eu.set_instruction(0x0058, 0x0007, &oriw<postinc_indirect>);
    eu.set_instruction(0x0060, 0x0007, &oriw<predec_indirect>);
    eu.set_instruction(0x0068, 0x0007, &oriw<disp_indirect>);
    eu.set_instruction(0x0070, 0x0007, &oriw<indexed_indirect>);
    eu.set_instruction(0x0078, 0x0000, &oriw<absolute_short>);
    eu.set_instruction(0x0079, 0x0000, &oriw<absolute_long>);
    eu.set_instruction(0x007c, 0x0000, &m68k_ori_to_sr);
    eu.set_instruction(0x0080, 0x0007, &oril<data_register>);
    eu.set_instruction(0x0090, 0x0007, &oril<indirect>);
    eu.set_instruction(0x0098, 0x0007, &oril<postinc_indirect>);
    eu.set_instruction(0x00a0, 0x0007, &oril<predec_indirect>);
    eu.set_instruction(0x00a8, 0x0007, &oril<disp_indirect>);
    eu.set_instruction(0x00b0, 0x0007, &oril<indexed_indirect>);
    eu.set_instruction(0x00b8, 0x0000, &oril<absolute_short>);
    eu.set_instruction(0x00b9, 0x0000, &oril<absolute_long>);
    eu.set_instruction(0x0100, 0x0e07,
		       &m68k_btst_r<long_word_size, long_word_d_register>);
    eu.set_instruction(0x0110, 0x0e07,
		       &m68k_btst_r<byte_size, byte_indirect>);
    eu.set_instruction(0x0118, 0x0e07,
		       &m68k_btst_r<byte_size, byte_postinc_indirect>);
    eu.set_instruction(0x0120, 0x0e07,
		       &m68k_btst_r<byte_size, byte_predec_indirect>);
    eu.set_instruction(0x0128, 0x0e07,
		       &m68k_btst_r<byte_size, byte_disp_indirect>);
    eu.set_instruction(0x0130, 0x0e07,
		       &m68k_btst_r<byte_size, byte_index_indirect>);
    eu.set_instruction(0x0138, 0x0e00,
		       &m68k_btst_r<byte_size, byte_abs_short>);
    eu.set_instruction(0x0139, 0x0e00,
		       &m68k_btst_r<byte_size, byte_abs_long>);
    eu.set_instruction(0x0180, 0x0e07,
		       &m68k_bclr_r<long_word_size, long_word_d_register>);
    eu.set_instruction(0x0190, 0x0e07,
		       &m68k_bclr_r<byte_size, byte_indirect>);
    eu.set_instruction(0x0198, 0x0e07,
		       &m68k_bclr_r<byte_size, byte_postinc_indirect>);
    eu.set_instruction(0x01a0, 0x0e07,
		       &m68k_bclr_r<byte_size, byte_predec_indirect>);
    eu.set_instruction(0x01a8, 0x0e07,
		       &m68k_bclr_r<byte_size, byte_disp_indirect>);
    eu.set_instruction(0x01b0, 0x0e07,
		       &m68k_bclr_r<byte_size, byte_index_indirect>);
    eu.set_instruction(0x01b8, 0x0e00,
		       &m68k_bclr_r<byte_size, byte_abs_short>);
    eu.set_instruction(0x01b9, 0x0e00,
		       &m68k_bclr_r<byte_size, byte_abs_long>);
    eu.set_instruction(0x01c0, 0x0e07,
		       &m68k_bset_r<long_word_size, long_word_d_register>);
    eu.set_instruction(0x01d0, 0x0e07, &m68k_bset_r<byte_size, byte_indirect>);
    eu.set_instruction(0x01d8, 0x0e07,
		       &m68k_bset_r<byte_size, byte_postinc_indirect>);
    eu.set_instruction(0x01e0, 0x0e07,
		       &m68k_bset_r<byte_size, byte_predec_indirect>);
    eu.set_instruction(0x01e8, 0x0e07,
		       &m68k_bset_r<byte_size, byte_disp_indirect>);
    eu.set_instruction(0x01f0, 0x0e07,
		       &m68k_bset_r<byte_size, byte_index_indirect>);
    eu.set_instruction(0x01f8, 0x0e00,
		       &m68k_bset_r<byte_size, byte_abs_short>);
    eu.set_instruction(0x01f9, 0x0e00,
		       &m68k_bset_r<byte_size, byte_abs_long>);
    eu.set_instruction(0x0200, 0x0007, &m68k_andi<byte_size, byte_d_register>);
    eu.set_instruction(0x0210, 0x0007, &m68k_andi<byte_size, byte_indirect>);
    eu.set_instruction(0x0218, 0x0007,
		       &m68k_andi<byte_size, byte_postinc_indirect>);
    eu.set_instruction(0x0220, 0x0007,
		       &m68k_andi<byte_size, byte_predec_indirect>);
    eu.set_instruction(0x0228, 0x0007,
		       &m68k_andi<byte_size, byte_disp_indirect>);
    eu.set_instruction(0x0230, 0x0007,
		       &m68k_andi<byte_size, byte_index_indirect>);
    eu.set_instruction(0x0238, 0x0000,
		       &m68k_andi<byte_size, byte_abs_short>);
    eu.set_instruction(0x0239, 0x0000,
		       &m68k_andi<byte_size, byte_abs_long>);
    eu.set_instruction(0x023c, 0x0000,
		       &m68k_andi_to_ccr);
    eu.set_instruction(0x0240, 0x0007, &m68k_andi<word_size, word_d_register>);
    eu.set_instruction(0x0250, 0x0007, &m68k_andi<word_size, word_indirect>);
    eu.set_instruction(0x0258, 0x0007,
		       &m68k_andi<word_size, word_postinc_indirect>);
    eu.set_instruction(0x0260, 0x0007,
		       &m68k_andi<word_size, word_predec_indirect>);
    eu.set_instruction(0x0268, 0x0007,
		       &m68k_andi<word_size, word_disp_indirect>);
    eu.set_instruction(0x0270, 0x0007,
		       &m68k_andi<word_size, word_index_indirect>);
    eu.set_instruction(0x0278, 0x0000,
		       &m68k_andi<word_size, word_abs_short>);
    eu.set_instruction(0x0279, 0x0000,
		       &m68k_andi<word_size, word_abs_long>);
    eu.set_instruction(0x0280, 0x0007,
		       &m68k_andi<long_word_size, long_word_d_register>);
    eu.set_instruction(0x0290, 0x0007,
		       &m68k_andi<long_word_size, long_word_indirect>);
    eu.set_instruction(0x0298, 0x0007,
		       &m68k_andi<long_word_size, long_word_postinc_indirect>);
    eu.set_instruction(0x02a0, 0x0007,
		       &m68k_andi<long_word_size, long_word_predec_indirect>);
    eu.set_instruction(0x02a8, 0x0007,
		       &m68k_andi<long_word_size, long_word_disp_indirect>);
    eu.set_instruction(0x02b0, 0x0007,
		       &m68k_andi<long_word_size, long_word_index_indirect>);
    eu.set_instruction(0x02b8, 0x0000,
		       &m68k_andi<long_word_size, long_word_abs_short>);
    eu.set_instruction(0x02b9, 0x0000,
		       &m68k_andi<long_word_size, long_word_abs_long>);
    eu.set_instruction(0x0400, 0x0007, &subib<data_register>);
    eu.set_instruction(0x0410, 0x0007, &subib<indirect>);
    eu.set_instruction(0x0418, 0x0007, &subib<postinc_indirect>);
    eu.set_instruction(0x0420, 0x0007, &subib<predec_indirect>);
    eu.set_instruction(0x0428, 0x0007, &subib<disp_indirect>);
    eu.set_instruction(0x0430, 0x0007, &subib<indexed_indirect>);
    eu.set_instruction(0x0438, 0x0000, &subib<absolute_short>);
    eu.set_instruction(0x0439, 0x0000, &subib<absolute_long>);
    eu.set_instruction(0x0480, 0x0007, &subil<data_register>);
    eu.set_instruction(0x0490, 0x0007, &subil<indirect>);
    eu.set_instruction(0x0498, 0x0007, &subil<postinc_indirect>);
    eu.set_instruction(0x04a0, 0x0007, &subil<predec_indirect>);
    eu.set_instruction(0x04a8, 0x0007, &subil<disp_indirect>);
    eu.set_instruction(0x04b0, 0x0007, &subil<indexed_indirect>);
    eu.set_instruction(0x04b8, 0x0000, &subil<absolute_short>);
    eu.set_instruction(0x04b9, 0x0000, &subil<absolute_long>);
    eu.set_instruction(0x0600, 0x0007, &m68k_addi<byte_size, byte_d_register>);
    eu.set_instruction(0x0610, 0x0007, &m68k_addi<byte_size, byte_indirect>);
    eu.set_instruction(0x0618, 0x0007,
		       &m68k_addi<byte_size, byte_postinc_indirect>);
    eu.set_instruction(0x0620, 0x0007,
		       &m68k_addi<byte_size, byte_predec_indirect>);
    eu.set_instruction(0x0628, 0x0007,
		       &m68k_addi<byte_size, byte_disp_indirect>);
    eu.set_instruction(0x0630, 0x0007,
		       &m68k_addi<byte_size, byte_index_indirect>);
    eu.set_instruction(0x0638, 0x0000, &m68k_addi<byte_size, byte_abs_short>);
    eu.set_instruction(0x0639, 0x0000, &m68k_addi<byte_size, byte_abs_long>);
    eu.set_instruction(0x0640, 0x0007, &m68k_addi<word_size, word_d_register>);
    eu.set_instruction(0x0650, 0x0007, &m68k_addi<word_size, word_indirect>);
    eu.set_instruction(0x0658, 0x0007,
		       &m68k_addi<word_size, word_postinc_indirect>);
    eu.set_instruction(0x0660, 0x0007,
		       &m68k_addi<word_size, word_predec_indirect>);
    eu.set_instruction(0x0668, 0x0007,
		       &m68k_addi<word_size, word_disp_indirect>);
    eu.set_instruction(0x0670, 0x0007,
		       &m68k_addi<word_size, word_index_indirect>);
    eu.set_instruction(0x0678, 0x0000, &m68k_addi<word_size, word_abs_short>);
    eu.set_instruction(0x0679, 0x0000, &m68k_addi<word_size, word_abs_long>);
    eu.set_instruction(0x0680, 0x0007,
		       &m68k_addi<long_word_size, long_word_d_register>);
    eu.set_instruction(0x0690, 0x0007,
		       &m68k_addi<long_word_size, long_word_indirect>);
    eu.set_instruction(0x0698, 0x0007,
		       &m68k_addi<long_word_size, long_word_postinc_indirect>);
    eu.set_instruction(0x06a0, 0x0007,
		       &m68k_addi<long_word_size, long_word_predec_indirect>);
    eu.set_instruction(0x06a8, 0x0007,
		       &m68k_addi<long_word_size, long_word_disp_indirect>);
    eu.set_instruction(0x06b0, 0x0007,
		       &m68k_addi<long_word_size, long_word_index_indirect>);
    eu.set_instruction(0x06b8, 0x0000,
		       &m68k_addi<long_word_size, long_word_abs_short>);
    eu.set_instruction(0x06b9, 0x0000,
		       &m68k_addi<long_word_size, long_word_abs_long>);
    eu.set_instruction(0x0800, 0x0007, &btstl_i);
    eu.set_instruction(0x0810, 0x0007, &btstb_i<indirect>);
    eu.set_instruction(0x0818, 0x0007, &btstb_i<postinc_indirect>);
    eu.set_instruction(0x0820, 0x0007, &btstb_i<predec_indirect>);
    eu.set_instruction(0x0828, 0x0007, &btstb_i<disp_indirect>);
    eu.set_instruction(0x0830, 0x0007, &btstb_i<indexed_indirect>);
    eu.set_instruction(0x0838, 0x0000, &btstb_i<absolute_short>);
    eu.set_instruction(0x0839, 0x0000, &btstb_i<absolute_long>);
    eu.set_instruction(0x0880, 0x0007,
		       &m68k_bclr_i<long_word_size, long_word_d_register>);
    eu.set_instruction(0x0890, 0x0007,
		       &m68k_bclr_i<byte_size, byte_indirect>);
    eu.set_instruction(0x0898, 0x0007,
		       &m68k_bclr_i<byte_size, byte_postinc_indirect>);
    eu.set_instruction(0x08a0, 0x0007,
		       &m68k_bclr_i<byte_size, byte_predec_indirect>);
    eu.set_instruction(0x08a8, 0x0007,
		       &m68k_bclr_i<byte_size, byte_disp_indirect>);
    eu.set_instruction(0x08b0, 0x0007,
		       &m68k_bclr_i<byte_size, byte_index_indirect>);
    eu.set_instruction(0x08b8, 0x0000,
		       &m68k_bclr_i<byte_size, byte_abs_short>);
    eu.set_instruction(0x08b9, 0x0000,
		       &m68k_bclr_i<byte_size, byte_abs_long>);
    eu.set_instruction(0x08c0, 0x0007, &bsetl_i);
    eu.set_instruction(0x0a00, 0x0007,
		       &m68k_eori<byte_size, byte_d_register>);
    eu.set_instruction(0x0a10, 0x0007,
		       &m68k_eori<byte_size, byte_indirect>);
    eu.set_instruction(0x0a18, 0x0007,
		       &m68k_eori<byte_size, byte_postinc_indirect>);
    eu.set_instruction(0x0a20, 0x0007,
		       &m68k_eori<byte_size, byte_predec_indirect>);
    eu.set_instruction(0x0a28, 0x0007,
		       &m68k_eori<byte_size, byte_disp_indirect>);
    eu.set_instruction(0x0a30, 0x0007,
		       &m68k_eori<byte_size, byte_index_indirect>);
    eu.set_instruction(0x0a38, 0x0000,
		       &m68k_eori<byte_size, byte_abs_short>);
    eu.set_instruction(0x0a39, 0x0000,
		       &m68k_eori<byte_size, byte_abs_long>);
    eu.set_instruction(0x0a40, 0x0007,
		       &m68k_eori<word_size, word_d_register>);
    eu.set_instruction(0x0a50, 0x0007,
		       &m68k_eori<word_size, word_indirect>);
    eu.set_instruction(0x0a58, 0x0007,
		       &m68k_eori<word_size, word_postinc_indirect>);
    eu.set_instruction(0x0a60, 0x0007,
		       &m68k_eori<word_size, word_predec_indirect>);
    eu.set_instruction(0x0a68, 0x0007,
		       &m68k_eori<word_size, word_disp_indirect>);
    eu.set_instruction(0x0a70, 0x0007,
		       &m68k_eori<word_size, word_index_indirect>);
    eu.set_instruction(0x0a78, 0x0000,
		       &m68k_eori<word_size, word_abs_short>);
    eu.set_instruction(0x0a79, 0x0000,
		       &m68k_eori<word_size, word_abs_long>);
    eu.set_instruction(0x0a80, 0x0007,
		       &m68k_eori<long_word_size, long_word_d_register>);
    eu.set_instruction(0x0a90, 0x0007,
		       &m68k_eori<long_word_size, long_word_indirect>);
    eu.set_instruction(0x0a98, 0x0007,
		       &m68k_eori<long_word_size, long_word_postinc_indirect>);
    eu.set_instruction(0x0aa0, 0x0007,
		       &m68k_eori<long_word_size, long_word_predec_indirect>);
    eu.set_instruction(0x0aa8, 0x0007,
		       &m68k_eori<long_word_size, long_word_disp_indirect>);
    eu.set_instruction(0x0ab0, 0x0007,
		       &m68k_eori<long_word_size, long_word_index_indirect>);
    eu.set_instruction(0x0ab8, 0x0000,
		       &m68k_eori<long_word_size, long_word_abs_short>);
    eu.set_instruction(0x0ab9, 0x0000,
		       &m68k_eori<long_word_size, long_word_abs_long>);
    eu.set_instruction(0x0c00, 0x0007, &cmpib<data_register>);
    eu.set_instruction(0x0c10, 0x0007, &cmpib<indirect>);
    eu.set_instruction(0x0c18, 0x0007, &cmpib<postinc_indirect>);
    eu.set_instruction(0x0c20, 0x0007, &cmpib<predec_indirect>);
    eu.set_instruction(0x0c28, 0x0007, &cmpib<disp_indirect>);
    eu.set_instruction(0x0c30, 0x0007, &cmpib<indexed_indirect>);
    eu.set_instruction(0x0c38, 0x0000, &cmpib<absolute_short>);
    eu.set_instruction(0x0c39, 0x0000, &cmpib<absolute_long>);
    eu.set_instruction(0x0c40, 0x0007, &cmpiw<data_register>);
    eu.set_instruction(0x0c50, 0x0007, &cmpiw<indirect>);
    eu.set_instruction(0x0c58, 0x0007, &cmpiw<postinc_indirect>);
    eu.set_instruction(0x0c60, 0x0007, &cmpiw<predec_indirect>);
    eu.set_instruction(0x0c68, 0x0007, &cmpiw<disp_indirect>);
    eu.set_instruction(0x0c70, 0x0007, &cmpiw<indexed_indirect>);
    eu.set_instruction(0x0c78, 0x0000, &cmpiw<absolute_short>);
    eu.set_instruction(0x0c79, 0x0000, &cmpiw<absolute_long>);
    eu.set_instruction(0x0c80, 0x0007, &cmpil<data_register>);
    eu.set_instruction(0x0c90, 0x0007, &cmpil<indirect>);
    eu.set_instruction(0x0c98, 0x0007, &cmpil<postinc_indirect>);
    eu.set_instruction(0x0ca0, 0x0007, &cmpil<predec_indirect>);
    eu.set_instruction(0x0ca8, 0x0007, &cmpil<disp_indirect>);
    eu.set_instruction(0x0cb0, 0x0007, &cmpil<indexed_indirect>);
    eu.set_instruction(0x0cb8, 0x0000, &cmpil<absolute_short>);
    eu.set_instruction(0x0cb9, 0x0000, &cmpil<absolute_long>);
    eu.set_instruction(0x1000, 0x0e07,
		       &m68k_move<byte_size, byte_d_register, byte_d_register>);
    eu.set_instruction(0x1010, 0x0e07,
		       &m68k_move<byte_size, byte_indirect, byte_d_register>);
    eu.set_instruction(0x1018, 0x0e07,
		       &m68k_move<byte_size, byte_postinc_indirect, byte_d_register>);
    eu.set_instruction(0x1020, 0x0e07,
		       &m68k_move<byte_size, byte_predec_indirect, byte_d_register>);
    eu.set_instruction(0x1028, 0x0e07,
		       &m68k_move<byte_size, byte_disp_indirect, byte_d_register>);
    eu.set_instruction(0x1030, 0x0e07,
		       &m68k_move<byte_size, byte_index_indirect, byte_d_register>);
    eu.set_instruction(0x1038, 0x0e00,
		       &m68k_move<byte_size, byte_abs_short, byte_d_register>);
    eu.set_instruction(0x1039, 0x0e00,
		       &m68k_move<byte_size, byte_abs_long, byte_d_register>);
    eu.set_instruction(0x103a, 0x0e00,
		       &m68k_move<byte_size, byte_disp_pc_indirect, byte_d_register>);
    eu.set_instruction(0x103b, 0x0e00,
		       &m68k_move<byte_size, byte_index_pc_indirect, byte_d_register>);
    eu.set_instruction(0x103c, 0x0e00,
		       &m68k_move<byte_size, byte_immediate, byte_d_register>);
    eu.set_instruction(0x1080, 0x0e07,
		       &m68k_move<byte_size, byte_d_register, byte_indirect>);
    eu.set_instruction(0x1090, 0x0e07,
		       &m68k_move<byte_size, byte_indirect, byte_indirect>);
    eu.set_instruction(0x1098, 0x0e07,
		       &m68k_move<byte_size, byte_postinc_indirect, byte_indirect>);
    eu.set_instruction(0x10a0, 0x0e07,
		       &m68k_move<byte_size, byte_predec_indirect, byte_indirect>);
    eu.set_instruction(0x10a8, 0x0e07,
		       &m68k_move<byte_size, byte_disp_indirect, byte_indirect>);
    eu.set_instruction(0x10b0, 0x0e07,
		       &m68k_move<byte_size, byte_index_indirect, byte_indirect>);
    eu.set_instruction(0x10b8, 0x0e00,
		       &m68k_move<byte_size, byte_abs_short, byte_indirect>);
    eu.set_instruction(0x10b9, 0x0e00,
		       &m68k_move<byte_size, byte_abs_long, byte_indirect>);
    eu.set_instruction(0x10ba, 0x0e00,
		       &m68k_move<byte_size, byte_disp_pc_indirect, byte_indirect>);
    eu.set_instruction(0x10bb, 0x0e00,
		       &m68k_move<byte_size, byte_index_pc_indirect, byte_indirect>);
    eu.set_instruction(0x10bc, 0x0e00,
		       &m68k_move<byte_size, byte_immediate, byte_indirect>);
    eu.set_instruction(0x10c0, 0x0e07,
		       &m68k_move<byte_size, byte_d_register, byte_postinc_indirect>);
    eu.set_instruction(0x10d0, 0x0e07,
		       &m68k_move<byte_size, byte_indirect, byte_postinc_indirect>);
    eu.set_instruction(0x10d8, 0x0e07,
		       &m68k_move<byte_size, byte_postinc_indirect, byte_postinc_indirect>);
    eu.set_instruction(0x10e0, 0x0e07,
		       &m68k_move<byte_size, byte_predec_indirect, byte_postinc_indirect>);
    eu.set_instruction(0x10e8, 0x0e07,
		       &m68k_move<byte_size, byte_disp_indirect, byte_postinc_indirect>);
    eu.set_instruction(0x10f0, 0x0e07,
		       &m68k_move<byte_size, byte_index_indirect, byte_postinc_indirect>);
    eu.set_instruction(0x10f8, 0x0e00,
		       &m68k_move<byte_size, byte_abs_short, byte_postinc_indirect>);
    eu.set_instruction(0x10f9, 0x0e00,
		       &m68k_move<byte_size, byte_abs_long, byte_postinc_indirect>);
    eu.set_instruction(0x10fa, 0x0e00,
		       &m68k_move<byte_size, byte_disp_pc_indirect, byte_postinc_indirect>);
    eu.set_instruction(0x10fb, 0x0e00,
		       &m68k_move<byte_size, byte_index_pc_indirect, byte_postinc_indirect>);
    eu.set_instruction(0x10fc, 0x0e00,
		       &m68k_move<byte_size, byte_immediate, byte_postinc_indirect>);
    eu.set_instruction(0x1100, 0x0e07,
		       &m68k_move<byte_size, byte_d_register, byte_predec_indirect>);
    eu.set_instruction(0x1110, 0x0e07,
		       &m68k_move<byte_size, byte_indirect, byte_predec_indirect>);
    eu.set_instruction(0x1118, 0x0e07,
		       &m68k_move<byte_size, byte_postinc_indirect, byte_predec_indirect>);
    eu.set_instruction(0x1120, 0x0e07,
		       &m68k_move<byte_size, byte_predec_indirect, byte_predec_indirect>);
    eu.set_instruction(0x1128, 0x0e07,
		       &m68k_move<byte_size, byte_disp_indirect, byte_predec_indirect>);
    eu.set_instruction(0x1130, 0x0e07,
		       &m68k_move<byte_size, byte_index_indirect, byte_predec_indirect>);
    eu.set_instruction(0x1138, 0x0e00,
		       &m68k_move<byte_size, byte_abs_short, byte_predec_indirect>);
    eu.set_instruction(0x1139, 0x0e00,
		       &m68k_move<byte_size, byte_abs_long, byte_predec_indirect>);
    eu.set_instruction(0x113a, 0x0e00,
		       &m68k_move<byte_size, byte_disp_pc_indirect, byte_predec_indirect>);
    eu.set_instruction(0x113b, 0x0e00,
		       &m68k_move<byte_size, byte_index_pc_indirect, byte_predec_indirect>);
    eu.set_instruction(0x113c, 0x0e00,
		       &m68k_move<byte_size, byte_immediate, byte_predec_indirect>);
    eu.set_instruction(0x1140, 0x0e07,
		       &m68k_move<byte_size, byte_d_register, byte_disp_indirect>);
    eu.set_instruction(0x1150, 0x0e07,
		       &m68k_move<byte_size, byte_indirect, byte_disp_indirect>);
    eu.set_instruction(0x1158, 0x0e07,
		       &m68k_move<byte_size, byte_postinc_indirect, byte_disp_indirect>);
    eu.set_instruction(0x1160, 0x0e07,
		       &m68k_move<byte_size, byte_predec_indirect, byte_disp_indirect>);
    eu.set_instruction(0x1168, 0x0e07,
		       &m68k_move<byte_size, byte_disp_indirect, byte_disp_indirect>);
    eu.set_instruction(0x1170, 0x0e07,
		       &m68k_move<byte_size, byte_index_indirect, byte_disp_indirect>);
    eu.set_instruction(0x1178, 0x0e00,
		       &m68k_move<byte_size, byte_abs_short, byte_disp_indirect>);
    eu.set_instruction(0x1179, 0x0e00,
		       &m68k_move<byte_size, byte_abs_long, byte_disp_indirect>);
    eu.set_instruction(0x117a, 0x0e00,
		       &m68k_move<byte_size, byte_disp_pc_indirect, byte_disp_indirect>);
    eu.set_instruction(0x117b, 0x0e00,
		       &m68k_move<byte_size, byte_index_pc_indirect, byte_disp_indirect>);
    eu.set_instruction(0x117c, 0x0e00,
		       &m68k_move<byte_size, byte_immediate, byte_disp_indirect>);
    eu.set_instruction(0x1180, 0x0e07,
		       &m68k_move<byte_size, byte_d_register, byte_index_indirect>);
    eu.set_instruction(0x1190, 0x0e07,
		       &m68k_move<byte_size, byte_indirect, byte_index_indirect>);
    eu.set_instruction(0x1198, 0x0e07,
		       &m68k_move<byte_size, byte_postinc_indirect, byte_index_indirect>);
    eu.set_instruction(0x11a0, 0x0e07,
		       &m68k_move<byte_size, byte_predec_indirect, byte_index_indirect>);
    eu.set_instruction(0x11a8, 0x0e07,
		       &m68k_move<byte_size, byte_disp_indirect, byte_index_indirect>);
    eu.set_instruction(0x11b0, 0x0e07,
		       &m68k_move<byte_size, byte_index_indirect, byte_index_indirect>);
    eu.set_instruction(0x11b8, 0x0e00,
		       &m68k_move<byte_size, byte_abs_short, byte_index_indirect>);
    eu.set_instruction(0x11b9, 0x0e00,
		       &m68k_move<byte_size, byte_abs_long, byte_index_indirect>);
    eu.set_instruction(0x11ba, 0x0e00,
		       &m68k_move<byte_size, byte_disp_pc_indirect, byte_index_indirect>);
    eu.set_instruction(0x11bb, 0x0e00,
		       &m68k_move<byte_size, byte_index_pc_indirect, byte_index_indirect>);
    eu.set_instruction(0x11bc, 0x0e00,
		       &m68k_move<byte_size, byte_immediate, byte_index_indirect>);
    eu.set_instruction(0x11c0, 0x0007,
		       &m68k_move<byte_size, byte_d_register, byte_abs_short>);
    eu.set_instruction(0x11d0, 0x0007,
		       &m68k_move<byte_size, byte_indirect, byte_abs_short>);
    eu.set_instruction(0x11d8, 0x0007,
		       &m68k_move<byte_size, byte_postinc_indirect, byte_abs_short>);
    eu.set_instruction(0x11e0, 0x0007,
		       &m68k_move<byte_size, byte_predec_indirect, byte_abs_short>);
    eu.set_instruction(0x11e8, 0x0007,
		       &m68k_move<byte_size, byte_disp_indirect, byte_abs_short>);
    eu.set_instruction(0x11f0, 0x0007,
		       &m68k_move<byte_size, byte_index_indirect, byte_abs_short>);
    eu.set_instruction(0x11f8, 0x0000,
		       &m68k_move<byte_size, byte_abs_short, byte_abs_short>);
    eu.set_instruction(0x11f9, 0x0000,
		       &m68k_move<byte_size, byte_abs_long, byte_abs_short>);
    eu.set_instruction(0x11fa, 0x0000,
		       &m68k_move<byte_size, byte_disp_pc_indirect, byte_abs_short>);
    eu.set_instruction(0x11fb, 0x0000,
		       &m68k_move<byte_size, byte_index_pc_indirect, byte_abs_short>);
    eu.set_instruction(0x11fc, 0x0000,
		       &m68k_move<byte_size, byte_immediate, byte_abs_short>);
    eu.set_instruction(0x13c0, 0x0007,
		       &m68k_move<byte_size, byte_d_register, byte_abs_long>);
    eu.set_instruction(0x13d0, 0x0007,
		       &m68k_move<byte_size, byte_indirect, byte_abs_long>);
    eu.set_instruction(0x13d8, 0x0007,
		       &m68k_move<byte_size, byte_postinc_indirect, byte_abs_long>);
    eu.set_instruction(0x13e0, 0x0007,
		       &m68k_move<byte_size, byte_predec_indirect, byte_abs_long>);
    eu.set_instruction(0x13e8, 0x0007,
		       &m68k_move<byte_size, byte_disp_indirect, byte_abs_long>);
    eu.set_instruction(0x13f0, 0x0007,
		       &m68k_move<byte_size, byte_index_indirect, byte_abs_long>);
    eu.set_instruction(0x13f8, 0x0000,
		       &m68k_move<byte_size, byte_abs_short, byte_abs_long>);
    eu.set_instruction(0x13f9, 0x0000,
		       &m68k_move<byte_size, byte_abs_long, byte_abs_long>);
    eu.set_instruction(0x13fa, 0x0000,
		       &m68k_move<byte_size, byte_disp_pc_indirect, byte_abs_long>);
    eu.set_instruction(0x13fb, 0x0000,
		       &m68k_move<byte_size, byte_index_pc_indirect, byte_abs_long>);
    eu.set_instruction(0x13fc, 0x0000,
		       &m68k_move<byte_size, byte_immediate, byte_abs_long>);
    eu.set_instruction(0x2000, 0x0e07,
		       &m68k_move<long_word_size, long_word_d_register, long_word_d_register>);
    eu.set_instruction(0x2008, 0x0e07,
		       &m68k_move<long_word_size, long_word_a_register, long_word_d_register>);
    eu.set_instruction(0x2010, 0x0e07,
		       &m68k_move<long_word_size, long_word_indirect, long_word_d_register>);
    eu.set_instruction(0x2018, 0x0e07,
		       &m68k_move<long_word_size, long_word_postinc_indirect, long_word_d_register>);
    eu.set_instruction(0x2020, 0x0e07,
		       &m68k_move<long_word_size, long_word_predec_indirect, long_word_d_register>);
    eu.set_instruction(0x2028, 0x0e07,
		       &m68k_move<long_word_size, long_word_disp_indirect, long_word_d_register>);
    eu.set_instruction(0x2030, 0x0e07,
		       &m68k_move<long_word_size, long_word_index_indirect, long_word_d_register>);
    eu.set_instruction(0x2038, 0x0e00,
		       &m68k_move<long_word_size, long_word_abs_short, long_word_d_register>);
    eu.set_instruction(0x2039, 0x0e00,
		       &m68k_move<long_word_size, long_word_abs_long, long_word_d_register>);
    eu.set_instruction(0x203a, 0x0e00,
		       &m68k_move<long_word_size, long_word_disp_pc_indirect, long_word_d_register>);
    eu.set_instruction(0x203b, 0x0e00,
		       &m68k_move<long_word_size, long_word_index_pc_indirect, long_word_d_register>);
    eu.set_instruction(0x203c, 0x0e00,
		       &m68k_move<long_word_size, long_word_immediate, long_word_d_register>);
    eu.set_instruction(0x2040, 0x0e07,
		       &m68k_movea<long_word_size, long_word_d_register>);
    eu.set_instruction(0x2048, 0x0e07,
		       &m68k_movea<long_word_size, long_word_a_register>);
    eu.set_instruction(0x2050, 0x0e07,
		       &m68k_movea<long_word_size, long_word_indirect>);
    eu.set_instruction(0x2058, 0x0e07,
		       &m68k_movea<long_word_size,
		                   long_word_postinc_indirect>);
    eu.set_instruction(0x2060, 0x0e07,
		       &m68k_movea<long_word_size, long_word_predec_indirect>);
    eu.set_instruction(0x2068, 0x0e07,
		       &m68k_movea<long_word_size, long_word_disp_indirect>);
    eu.set_instruction(0x2070, 0x0e07,
		       &m68k_movea<long_word_size, long_word_index_indirect>);
    eu.set_instruction(0x2078, 0x0e00,
		       &m68k_movea<long_word_size, long_word_abs_short>);
    eu.set_instruction(0x2079, 0x0e00,
		       &m68k_movea<long_word_size, long_word_abs_long>);
    eu.set_instruction(0x207a, 0x0e00,
		       &m68k_movea<long_word_size,
		                   long_word_disp_pc_indirect>);
    eu.set_instruction(0x207b, 0x0e00,
		       &m68k_movea<long_word_size,
		                   long_word_index_pc_indirect>);
    eu.set_instruction(0x207c, 0x0e00,
		       &m68k_movea<long_word_size, long_word_immediate>);
    eu.set_instruction(0x2080, 0x0e07,
		       &m68k_move<long_word_size, long_word_d_register, long_word_indirect>);
    eu.set_instruction(0x2088, 0x0e07,
		       &m68k_move<long_word_size, long_word_a_register, long_word_indirect>);
    eu.set_instruction(0x2090, 0x0e07,
		       &m68k_move<long_word_size, long_word_indirect, long_word_indirect>);
    eu.set_instruction(0x2098, 0x0e07,
		       &m68k_move<long_word_size, long_word_postinc_indirect, long_word_indirect>);
    eu.set_instruction(0x20a0, 0x0e07,
		       &m68k_move<long_word_size, long_word_predec_indirect, long_word_indirect>);
    eu.set_instruction(0x20a8, 0x0e07,
		       &m68k_move<long_word_size, long_word_disp_indirect, long_word_indirect>);
    eu.set_instruction(0x20b0, 0x0e07,
		       &m68k_move<long_word_size, long_word_index_indirect, long_word_indirect>);
    eu.set_instruction(0x20b8, 0x0e00,
		       &m68k_move<long_word_size, long_word_abs_short, long_word_indirect>);
    eu.set_instruction(0x20b9, 0x0e00,
		       &m68k_move<long_word_size, long_word_abs_long, long_word_indirect>);
    eu.set_instruction(0x20ba, 0x0e00,
		       &m68k_move<long_word_size, long_word_disp_pc_indirect, long_word_indirect>);
    eu.set_instruction(0x20bb, 0x0e00,
		       &m68k_move<long_word_size, long_word_index_pc_indirect, long_word_indirect>);
    eu.set_instruction(0x20bc, 0x0e00,
		       &m68k_move<long_word_size, long_word_immediate, long_word_indirect>);
    eu.set_instruction(0x20c0, 0x0e07,
		       &m68k_move<long_word_size, long_word_d_register, long_word_postinc_indirect>);
    eu.set_instruction(0x20c8, 0x0e07,
		       &m68k_move<long_word_size, long_word_a_register, long_word_postinc_indirect>);
    eu.set_instruction(0x20d0, 0x0e07,
		       &m68k_move<long_word_size, long_word_indirect, long_word_postinc_indirect>);
    eu.set_instruction(0x20d8, 0x0e07,
		       &m68k_move<long_word_size, long_word_postinc_indirect, long_word_postinc_indirect>);
    eu.set_instruction(0x20e0, 0x0e07,
		       &m68k_move<long_word_size, long_word_predec_indirect, long_word_postinc_indirect>);
    eu.set_instruction(0x20e8, 0x0e07,
		       &m68k_move<long_word_size, long_word_disp_indirect, long_word_postinc_indirect>);
    eu.set_instruction(0x20f0, 0x0e07,
		       &m68k_move<long_word_size, long_word_index_indirect, long_word_postinc_indirect>);
    eu.set_instruction(0x20f8, 0x0e00,
		       &m68k_move<long_word_size, long_word_abs_short, long_word_postinc_indirect>);
    eu.set_instruction(0x20f9, 0x0e00,
		       &m68k_move<long_word_size, long_word_abs_long, long_word_postinc_indirect>);
    eu.set_instruction(0x20fa, 0x0e00,
		       &m68k_move<long_word_size, long_word_disp_pc_indirect, long_word_postinc_indirect>);
    eu.set_instruction(0x20fb, 0x0e00,
		       &m68k_move<long_word_size, long_word_index_pc_indirect, long_word_postinc_indirect>);
    eu.set_instruction(0x20fc, 0x0e00,
		       &m68k_move<long_word_size, long_word_immediate, long_word_postinc_indirect>);
    eu.set_instruction(0x2100, 0x0e07,
		       &m68k_move<long_word_size, long_word_d_register, long_word_predec_indirect>);
    eu.set_instruction(0x2108, 0x0e07,
		       &m68k_move<long_word_size, long_word_a_register, long_word_predec_indirect>);
    eu.set_instruction(0x2110, 0x0e07,
		       &m68k_move<long_word_size, long_word_indirect, long_word_predec_indirect>);
    eu.set_instruction(0x2118, 0x0e07,
		       &m68k_move<long_word_size, long_word_postinc_indirect, long_word_predec_indirect>);
    eu.set_instruction(0x2120, 0x0e07,
		       &m68k_move<long_word_size, long_word_predec_indirect, long_word_predec_indirect>);
    eu.set_instruction(0x2128, 0x0e07,
		       &m68k_move<long_word_size, long_word_disp_indirect, long_word_predec_indirect>);
    eu.set_instruction(0x2130, 0x0e07,
		       &m68k_move<long_word_size, long_word_index_indirect, long_word_predec_indirect>);
    eu.set_instruction(0x2138, 0x0e00,
		       &m68k_move<long_word_size, long_word_abs_short, long_word_predec_indirect>);
    eu.set_instruction(0x2139, 0x0e00,
		       &m68k_move<long_word_size, long_word_abs_long, long_word_predec_indirect>);
    eu.set_instruction(0x213a, 0x0e00,
		       &m68k_move<long_word_size, long_word_disp_pc_indirect, long_word_predec_indirect>);
    eu.set_instruction(0x213b, 0x0e00,
		       &m68k_move<long_word_size, long_word_index_pc_indirect, long_word_predec_indirect>);
    eu.set_instruction(0x213c, 0x0e00,
		       &m68k_move<long_word_size, long_word_immediate, long_word_predec_indirect>);
    eu.set_instruction(0x2140, 0x0e07,
		       &m68k_move<long_word_size, long_word_d_register, long_word_disp_indirect>);
    eu.set_instruction(0x2148, 0x0e07,
		       &m68k_move<long_word_size, long_word_a_register, long_word_disp_indirect>);
    eu.set_instruction(0x2150, 0x0e07,
		       &m68k_move<long_word_size, long_word_indirect, long_word_disp_indirect>);
    eu.set_instruction(0x2158, 0x0e07,
		       &m68k_move<long_word_size, long_word_postinc_indirect, long_word_disp_indirect>);
    eu.set_instruction(0x2160, 0x0e07,
		       &m68k_move<long_word_size, long_word_predec_indirect, long_word_disp_indirect>);
    eu.set_instruction(0x2168, 0x0e07,
		       &m68k_move<long_word_size, long_word_disp_indirect, long_word_disp_indirect>);
    eu.set_instruction(0x2170, 0x0e07,
		       &m68k_move<long_word_size, long_word_index_indirect, long_word_disp_indirect>);
    eu.set_instruction(0x2178, 0x0e00,
		       &m68k_move<long_word_size, long_word_abs_short, long_word_disp_indirect>);
    eu.set_instruction(0x2179, 0x0e00,
		       &m68k_move<long_word_size, long_word_abs_long, long_word_disp_indirect>);
    eu.set_instruction(0x217a, 0x0e00,
		       &m68k_move<long_word_size, long_word_disp_pc_indirect, long_word_disp_indirect>);
    eu.set_instruction(0x217b, 0x0e00,
		       &m68k_move<long_word_size, long_word_index_pc_indirect, long_word_disp_indirect>);
    eu.set_instruction(0x217c, 0x0e00,
		       &m68k_move<long_word_size, long_word_immediate, long_word_disp_indirect>);
    eu.set_instruction(0x2180, 0x0e07,
		       &m68k_move<long_word_size, long_word_d_register, long_word_index_indirect>);
    eu.set_instruction(0x2188, 0x0e07,
		       &m68k_move<long_word_size, long_word_a_register, long_word_index_indirect>);
    eu.set_instruction(0x2190, 0x0e07,
		       &m68k_move<long_word_size, long_word_indirect, long_word_index_indirect>);
    eu.set_instruction(0x2198, 0x0e07,
		       &m68k_move<long_word_size, long_word_postinc_indirect, long_word_index_indirect>);
    eu.set_instruction(0x21a0, 0x0e07,
		       &m68k_move<long_word_size, long_word_predec_indirect, long_word_index_indirect>);
    eu.set_instruction(0x21a8, 0x0e07,
		       &m68k_move<long_word_size, long_word_disp_indirect, long_word_index_indirect>);
    eu.set_instruction(0x21b0, 0x0e07,
		       &m68k_move<long_word_size, long_word_index_indirect, long_word_index_indirect>);
    eu.set_instruction(0x21b8, 0x0e00,
		       &m68k_move<long_word_size, long_word_abs_short, long_word_index_indirect>);
    eu.set_instruction(0x21b9, 0x0e00,
		       &m68k_move<long_word_size, long_word_abs_long, long_word_index_indirect>);
    eu.set_instruction(0x21ba, 0x0e00,
		       &m68k_move<long_word_size, long_word_disp_pc_indirect, long_word_index_indirect>);
    eu.set_instruction(0x21bb, 0x0e00,
		       &m68k_move<long_word_size, long_word_index_pc_indirect, long_word_index_indirect>);
    eu.set_instruction(0x21bc, 0x0e00,
		       &m68k_move<long_word_size, long_word_immediate, long_word_index_indirect>);
    eu.set_instruction(0x21c0, 0x0007,
		       &m68k_move<long_word_size, long_word_d_register, long_word_abs_short>);
    eu.set_instruction(0x21c8, 0x0007,
		       &m68k_move<long_word_size, long_word_a_register, long_word_abs_short>);
    eu.set_instruction(0x21d0, 0x0007,
		       &m68k_move<long_word_size, long_word_indirect, long_word_abs_short>);
    eu.set_instruction(0x21d8, 0x0007,
		       &m68k_move<long_word_size, long_word_postinc_indirect, long_word_abs_short>);
    eu.set_instruction(0x21e0, 0x0007,
		       &m68k_move<long_word_size, long_word_predec_indirect, long_word_abs_short>);
    eu.set_instruction(0x21e8, 0x0007,
		       &m68k_move<long_word_size, long_word_disp_indirect, long_word_abs_short>);
    eu.set_instruction(0x21f0, 0x0007,
		       &m68k_move<long_word_size, long_word_index_indirect, long_word_abs_short>);
    eu.set_instruction(0x21f8, 0x0000,
		       &m68k_move<long_word_size, long_word_abs_short, long_word_abs_short>);
    eu.set_instruction(0x21f9, 0x0000,
		       &m68k_move<long_word_size, long_word_abs_long, long_word_abs_short>);
    eu.set_instruction(0x21fa, 0x0000,
		       &m68k_move<long_word_size, long_word_disp_pc_indirect, long_word_abs_short>);
    eu.set_instruction(0x21fb, 0x0000,
		       &m68k_move<long_word_size, long_word_index_pc_indirect, long_word_abs_short>);
    eu.set_instruction(0x21fc, 0x0000,
		       &m68k_move<long_word_size, long_word_immediate, long_word_abs_short>);
    eu.set_instruction(0x23c0, 0x0007,
		       &m68k_move<long_word_size, long_word_d_register, long_word_abs_long>);
    eu.set_instruction(0x23c8, 0x0007,
		       &m68k_move<long_word_size, long_word_a_register, long_word_abs_long>);
    eu.set_instruction(0x23d0, 0x0007,
		       &m68k_move<long_word_size, long_word_indirect, long_word_abs_long>);
    eu.set_instruction(0x23d8, 0x0007,
		       &m68k_move<long_word_size, long_word_postinc_indirect, long_word_abs_long>);
    eu.set_instruction(0x23e0, 0x0007,
		       &m68k_move<long_word_size, long_word_predec_indirect, long_word_abs_long>);
    eu.set_instruction(0x23e8, 0x0007,
		       &m68k_move<long_word_size, long_word_disp_indirect, long_word_abs_long>);
    eu.set_instruction(0x23f0, 0x0007,
		       &m68k_move<long_word_size, long_word_index_indirect, long_word_abs_long>);
    eu.set_instruction(0x23f8, 0x0000,
		       &m68k_move<long_word_size, long_word_abs_short, long_word_abs_long>);
    eu.set_instruction(0x23f9, 0x0000,
		       &m68k_move<long_word_size, long_word_abs_long, long_word_abs_long>);
    eu.set_instruction(0x23fa, 0x0000,
		       &m68k_move<long_word_size, long_word_disp_pc_indirect, long_word_abs_long>);
    eu.set_instruction(0x23fb, 0x0000,
		       &m68k_move<long_word_size, long_word_index_pc_indirect, long_word_abs_long>);
    eu.set_instruction(0x23fc, 0x0000,
		       &m68k_move<long_word_size, long_word_immediate, long_word_abs_long>);
    eu.set_instruction(0x3000, 0x0e07,
		       &m68k_move<word_size, word_d_register, word_d_register>);
    eu.set_instruction(0x3008, 0x0e07,
		       &m68k_move<word_size, word_a_register, word_d_register>);
    eu.set_instruction(0x3010, 0x0e07,
		       &m68k_move<word_size, word_indirect, word_d_register>);
    eu.set_instruction(0x3018, 0x0e07,
		       &m68k_move<word_size, word_postinc_indirect, word_d_register>);
    eu.set_instruction(0x3020, 0x0e07,
		       &m68k_move<word_size, word_predec_indirect, word_d_register>);
    eu.set_instruction(0x3028, 0x0e07,
		       &m68k_move<word_size, word_disp_indirect, word_d_register>);
    eu.set_instruction(0x3030, 0x0e07,
		       &m68k_move<word_size, word_index_indirect, word_d_register>);
    eu.set_instruction(0x3038, 0x0e00,
		       &m68k_move<word_size, word_abs_short, word_d_register>);
    eu.set_instruction(0x3039, 0x0e00,
		       &m68k_move<word_size, word_abs_long, word_d_register>);
    eu.set_instruction(0x303a, 0x0e00,
		       &m68k_move<word_size, word_disp_pc_indirect, word_d_register>);
    eu.set_instruction(0x303b, 0x0e00,
		       &m68k_move<word_size, word_index_pc_indirect, word_d_register>);
    eu.set_instruction(0x303c, 0x0e00,
		       &m68k_move<word_size, word_immediate, word_d_register>);
    eu.set_instruction(0x3040, 0x0e07,
		       &m68k_movea<word_size, word_d_register>);
    eu.set_instruction(0x3048, 0x0e07,
		       &m68k_movea<word_size, word_a_register>);
    eu.set_instruction(0x3050, 0x0e07, &m68k_movea<word_size, word_indirect>);
    eu.set_instruction(0x3058, 0x0e07,
		       &m68k_movea<word_size, word_postinc_indirect>);
    eu.set_instruction(0x3060, 0x0e07,
		       &m68k_movea<word_size, word_predec_indirect>);
    eu.set_instruction(0x3068, 0x0e07,
		       &m68k_movea<word_size, word_disp_indirect>);
    eu.set_instruction(0x3070, 0x0e07,
		       &m68k_movea<word_size, word_index_indirect>);
    eu.set_instruction(0x3078, 0x0e00, &m68k_movea<word_size, word_abs_short>);
    eu.set_instruction(0x3079, 0x0e00, &m68k_movea<word_size, word_abs_long>);
    eu.set_instruction(0x307a, 0x0e00,
		       &m68k_movea<word_size, word_disp_pc_indirect>);
    eu.set_instruction(0x307b, 0x0e00,
		       &m68k_movea<word_size, word_index_pc_indirect>);
    eu.set_instruction(0x307c, 0x0e00, &m68k_movea<word_size, word_immediate>);
    eu.set_instruction(0x3080, 0x0e07,
		       &m68k_move<word_size, word_d_register, word_indirect>);
    eu.set_instruction(0x3088, 0x0e07,
		       &m68k_move<word_size, word_a_register, word_indirect>);
    eu.set_instruction(0x3090, 0x0e07,
		       &m68k_move<word_size, word_indirect, word_indirect>);
    eu.set_instruction(0x3098, 0x0e07,
		       &m68k_move<word_size, word_postinc_indirect, word_indirect>);
    eu.set_instruction(0x30a0, 0x0e07,
		       &m68k_move<word_size, word_predec_indirect, word_indirect>);
    eu.set_instruction(0x30a8, 0x0e07,
		       &m68k_move<word_size, word_disp_indirect, word_indirect>);
    eu.set_instruction(0x30b0, 0x0e07,
		       &m68k_move<word_size, word_index_indirect, word_indirect>);
    eu.set_instruction(0x30b8, 0x0e00,
		       &m68k_move<word_size, word_abs_short, word_indirect>);
    eu.set_instruction(0x30b9, 0x0e00,
		       &m68k_move<word_size, word_abs_long, word_indirect>);
    eu.set_instruction(0x30ba, 0x0e00,
		       &m68k_move<word_size, word_disp_pc_indirect, word_indirect>);
    eu.set_instruction(0x30bb, 0x0e00,
		       &m68k_move<word_size, word_index_pc_indirect, word_indirect>);
    eu.set_instruction(0x30bc, 0x0e00,
		       &m68k_move<word_size, word_immediate, word_indirect>);
    eu.set_instruction(0x30c0, 0x0e07,
		       &m68k_move<word_size, word_d_register, word_postinc_indirect>);
    eu.set_instruction(0x30c8, 0x0e07,
		       &m68k_move<word_size, word_a_register, word_postinc_indirect>);
    eu.set_instruction(0x30d0, 0x0e07,
		       &m68k_move<word_size, word_indirect, word_postinc_indirect>);
    eu.set_instruction(0x30d8, 0x0e07,
		       &m68k_move<word_size, word_postinc_indirect, word_postinc_indirect>);
    eu.set_instruction(0x30e0, 0x0e07,
		       &m68k_move<word_size, word_predec_indirect, word_postinc_indirect>);
    eu.set_instruction(0x30e8, 0x0e07,
		       &m68k_move<word_size, word_disp_indirect, word_postinc_indirect>);
    eu.set_instruction(0x30f0, 0x0e07,
		       &m68k_move<word_size, word_index_indirect, word_postinc_indirect>);
    eu.set_instruction(0x30f8, 0x0e00,
		       &m68k_move<word_size, word_abs_short, word_postinc_indirect>);
    eu.set_instruction(0x30f9, 0x0e00,
		       &m68k_move<word_size, word_abs_long, word_postinc_indirect>);
    eu.set_instruction(0x30fa, 0x0e00,
		       &m68k_move<word_size, word_disp_pc_indirect, word_postinc_indirect>);
    eu.set_instruction(0x30fb, 0x0e00,
		       &m68k_move<word_size, word_index_pc_indirect, word_postinc_indirect>);
    eu.set_instruction(0x30fc, 0x0e00,
		       &m68k_move<word_size, word_immediate, word_postinc_indirect>);
    eu.set_instruction(0x3100, 0x0e07,
		       &m68k_move<word_size, word_d_register, word_predec_indirect>);
    eu.set_instruction(0x3108, 0x0e07,
		       &m68k_move<word_size, word_a_register, word_predec_indirect>);
    eu.set_instruction(0x3110, 0x0e07,
		       &m68k_move<word_size, word_indirect, word_predec_indirect>);
    eu.set_instruction(0x3118, 0x0e07,
		       &m68k_move<word_size, word_postinc_indirect, word_predec_indirect>);
    eu.set_instruction(0x3120, 0x0e07,
		       &m68k_move<word_size, word_predec_indirect, word_predec_indirect>);
    eu.set_instruction(0x3128, 0x0e07,
		       &m68k_move<word_size, word_disp_indirect, word_predec_indirect>);
    eu.set_instruction(0x3130, 0x0e07,
		       &m68k_move<word_size, word_index_indirect, word_predec_indirect>);
    eu.set_instruction(0x3138, 0x0e00,
		       &m68k_move<word_size, word_abs_short, word_predec_indirect>);
    eu.set_instruction(0x3139, 0x0e00,
		       &m68k_move<word_size, word_abs_long, word_predec_indirect>);
    eu.set_instruction(0x313a, 0x0e00,
		       &m68k_move<word_size, word_disp_pc_indirect, word_predec_indirect>);
    eu.set_instruction(0x313b, 0x0e00,
		       &m68k_move<word_size, word_index_pc_indirect, word_predec_indirect>);
    eu.set_instruction(0x313c, 0x0e00,
		       &m68k_move<word_size, word_immediate, word_predec_indirect>);
    eu.set_instruction(0x3140, 0x0e07,
		       &m68k_move<word_size, word_d_register, word_disp_indirect>);
    eu.set_instruction(0x3148, 0x0e07,
		       &m68k_move<word_size, word_a_register, word_disp_indirect>);
    eu.set_instruction(0x3150, 0x0e07,
		       &m68k_move<word_size, word_indirect, word_disp_indirect>);
    eu.set_instruction(0x3158, 0x0e07,
		       &m68k_move<word_size, word_postinc_indirect, word_disp_indirect>);
    eu.set_instruction(0x3160, 0x0e07,
		       &m68k_move<word_size, word_predec_indirect, word_disp_indirect>);
    eu.set_instruction(0x3168, 0x0e07,
		       &m68k_move<word_size, word_disp_indirect, word_disp_indirect>);
    eu.set_instruction(0x3170, 0x0e07,
		       &m68k_move<word_size, word_index_indirect, word_disp_indirect>);
    eu.set_instruction(0x3178, 0x0e00,
		       &m68k_move<word_size, word_abs_short, word_disp_indirect>);
    eu.set_instruction(0x3179, 0x0e00,
		       &m68k_move<word_size, word_abs_long, word_disp_indirect>);
    eu.set_instruction(0x317a, 0x0e00,
		       &m68k_move<word_size, word_disp_pc_indirect, word_disp_indirect>);
    eu.set_instruction(0x317b, 0x0e00,
		       &m68k_move<word_size, word_index_pc_indirect, word_disp_indirect>);
    eu.set_instruction(0x317c, 0x0e00,
		       &m68k_move<word_size, word_immediate, word_disp_indirect>);
    eu.set_instruction(0x3180, 0x0e07,
		       &m68k_move<word_size, word_d_register, word_index_indirect>);
    eu.set_instruction(0x3188, 0x0e07,
		       &m68k_move<word_size, word_a_register, word_index_indirect>);
    eu.set_instruction(0x3190, 0x0e07,
		       &m68k_move<word_size, word_indirect, word_index_indirect>);
    eu.set_instruction(0x3198, 0x0e07,
		       &m68k_move<word_size, word_postinc_indirect, word_index_indirect>);
    eu.set_instruction(0x31a0, 0x0e07,
		       &m68k_move<word_size, word_predec_indirect, word_index_indirect>);
    eu.set_instruction(0x31a8, 0x0e07,
		       &m68k_move<word_size, word_disp_indirect, word_index_indirect>);
    eu.set_instruction(0x31b0, 0x0e07,
		       &m68k_move<word_size, word_index_indirect, word_index_indirect>);
    eu.set_instruction(0x31b8, 0x0e00,
		       &m68k_move<word_size, word_abs_short, word_index_indirect>);
    eu.set_instruction(0x31b9, 0x0e00,
		       &m68k_move<word_size, word_abs_long, word_index_indirect>);
    eu.set_instruction(0x31ba, 0x0e00,
		       &m68k_move<word_size, word_disp_pc_indirect, word_index_indirect>);
    eu.set_instruction(0x31bb, 0x0e00,
		       &m68k_move<word_size, word_index_pc_indirect, word_index_indirect>);
    eu.set_instruction(0x31bc, 0x0e00,
		       &m68k_move<word_size, word_immediate, word_index_indirect>);
    eu.set_instruction(0x31c0, 0x0007,
		       &m68k_move<word_size, word_d_register, word_abs_short>);
    eu.set_instruction(0x31c8, 0x0007,
		       &m68k_move<word_size, word_a_register, word_abs_short>);
    eu.set_instruction(0x31d0, 0x0007,
		       &m68k_move<word_size, word_indirect, word_abs_short>);
    eu.set_instruction(0x31d8, 0x0007,
		       &m68k_move<word_size, word_postinc_indirect, word_abs_short>);
    eu.set_instruction(0x31e0, 0x0007,
		       &m68k_move<word_size, word_predec_indirect, word_abs_short>);
    eu.set_instruction(0x31e8, 0x0007,
		       &m68k_move<word_size, word_disp_indirect, word_abs_short>);
    eu.set_instruction(0x31f0, 0x0007,
		       &m68k_move<word_size, word_index_indirect, word_abs_short>);
    eu.set_instruction(0x31f8, 0x0000,
		       &m68k_move<word_size, word_abs_short, word_abs_short>);
    eu.set_instruction(0x31f9, 0x0000,
		       &m68k_move<word_size, word_abs_long, word_abs_short>);
    eu.set_instruction(0x31fa, 0x0000,
		       &m68k_move<word_size, word_disp_pc_indirect, word_abs_short>);
    eu.set_instruction(0x31fb, 0x0000,
		       &m68k_move<word_size, word_index_pc_indirect, word_abs_short>);
    eu.set_instruction(0x31fc, 0x0000,
		       &m68k_move<word_size, word_immediate, word_abs_short>);
    eu.set_instruction(0x33c0, 0x0007,
		       &m68k_move<word_size, word_d_register, word_abs_long>);
    eu.set_instruction(0x33c8, 0x0007,
		       &m68k_move<word_size, word_a_register, word_abs_long>);
    eu.set_instruction(0x33d0, 0x0007,
		       &m68k_move<word_size, word_indirect, word_abs_long>);
    eu.set_instruction(0x33d8, 0x0007,
		       &m68k_move<word_size, word_postinc_indirect, word_abs_long>);
    eu.set_instruction(0x33e0, 0x0007,
		       &m68k_move<word_size, word_predec_indirect, word_abs_long>);
    eu.set_instruction(0x33e8, 0x0007,
		       &m68k_move<word_size, word_disp_indirect, word_abs_long>);
    eu.set_instruction(0x33f0, 0x0007,
		       &m68k_move<word_size, word_index_indirect, word_abs_long>);
    eu.set_instruction(0x33f8, 0x0000,
		       &m68k_move<word_size, word_abs_short, word_abs_long>);
    eu.set_instruction(0x33f9, 0x0000,
		       &m68k_move<word_size, word_abs_long, word_abs_long>);
    eu.set_instruction(0x33fa, 0x0000,
		       &m68k_move<word_size, word_disp_pc_indirect, word_abs_long>);
    eu.set_instruction(0x33fb, 0x0000,
		       &m68k_move<word_size, word_index_pc_indirect, word_abs_long>);
    eu.set_instruction(0x33fc, 0x0000,
		       &m68k_move<word_size, word_immediate, word_abs_long>);
    eu.set_instruction(0x40c0, 0x0007, &m68k_move_from_sr<word_d_register>);
    eu.set_instruction(0x40d0, 0x0007,
		       &m68k_move_from_sr<word_indirect>);
    eu.set_instruction(0x40d8, 0x0007,
		       &m68k_move_from_sr<word_postinc_indirect>);
    eu.set_instruction(0x40e0, 0x0007,
		       &m68k_move_from_sr<word_predec_indirect>);
    eu.set_instruction(0x40e8, 0x0007,
		       &m68k_move_from_sr<word_disp_indirect>);
    eu.set_instruction(0x40f0, 0x0007,
		       &m68k_move_from_sr<word_index_indirect>);
    eu.set_instruction(0x40f8, 0x0000,
		       &m68k_move_from_sr<word_abs_short>);
    eu.set_instruction(0x40f9, 0x0000,
		       &m68k_move_from_sr<word_abs_long>);
    eu.set_instruction(0x41d0, 0x0e07, &lea<indirect>);
    eu.set_instruction(0x41e8, 0x0e07, &lea<disp_indirect>);
    eu.set_instruction(0x41f0, 0x0e07, &lea<indexed_indirect>);
    eu.set_instruction(0x41f8, 0x0e00, &lea<absolute_short>);
    eu.set_instruction(0x41f9, 0x0e00, &lea<absolute_long>);
    eu.set_instruction(0x41fa, 0x0e00, &lea<disp_pc>);
    eu.set_instruction(0x41fb, 0x0e00, &lea<indexed_pc_indirect>);
    eu.set_instruction(0x4200, 0x0007, &clrb<data_register>);
    eu.set_instruction(0x4210, 0x0007, &clrb<indirect>);
    eu.set_instruction(0x4218, 0x0007, &clrb<postinc_indirect>);
    eu.set_instruction(0x4220, 0x0007, &clrb<predec_indirect>);
    eu.set_instruction(0x4228, 0x0007, &clrb<disp_indirect>);
    eu.set_instruction(0x4230, 0x0007, &clrb<indexed_indirect>);
    eu.set_instruction(0x4238, 0x0000, &clrb<absolute_short>);
    eu.set_instruction(0x4239, 0x0000, &clrb<absolute_long>);
    eu.set_instruction(0x4240, 0x0007, &clrw<data_register>);
    eu.set_instruction(0x4250, 0x0007, &clrw<indirect>);
    eu.set_instruction(0x4258, 0x0007, &clrw<postinc_indirect>);
    eu.set_instruction(0x4260, 0x0007, &clrw<predec_indirect>);
    eu.set_instruction(0x4268, 0x0007, &clrw<disp_indirect>);
    eu.set_instruction(0x4270, 0x0007, &clrw<indexed_indirect>);
    eu.set_instruction(0x4278, 0x0000, &clrw<absolute_short>);
    eu.set_instruction(0x4279, 0x0000, &clrw<absolute_long>);
    eu.set_instruction(0x4280, 0x0007, &clrl<data_register>);
    eu.set_instruction(0x4290, 0x0007, &clrl<indirect>);
    eu.set_instruction(0x4298, 0x0007, &clrl<postinc_indirect>);
    eu.set_instruction(0x42a0, 0x0007, &clrl<predec_indirect>);
    eu.set_instruction(0x42a8, 0x0007, &clrl<disp_indirect>);
    eu.set_instruction(0x42b0, 0x0007, &clrl<indexed_indirect>);
    eu.set_instruction(0x42b8, 0x0000, &clrl<absolute_short>);
    eu.set_instruction(0x42b9, 0x0000, &clrl<absolute_long>);
    eu.set_instruction(0x4400, 0x0007, &negb<data_register>);
    eu.set_instruction(0x4410, 0x0007, &negb<indirect>);
    eu.set_instruction(0x4418, 0x0007, &negb<postinc_indirect>);
    eu.set_instruction(0x4420, 0x0007, &negb<predec_indirect>);
    eu.set_instruction(0x4428, 0x0007, &negb<disp_indirect>);
    eu.set_instruction(0x4430, 0x0007, &negb<indexed_indirect>);
    eu.set_instruction(0x4438, 0x0000, &negb<absolute_short>);
    eu.set_instruction(0x4439, 0x0000, &negb<absolute_long>);
    eu.set_instruction(0x4440, 0x0007, &negw<data_register>);
    eu.set_instruction(0x4450, 0x0007, &negw<indirect>);
    eu.set_instruction(0x4458, 0x0007, &negw<postinc_indirect>);
    eu.set_instruction(0x4460, 0x0007, &negw<predec_indirect>);
    eu.set_instruction(0x4468, 0x0007, &negw<disp_indirect>);
    eu.set_instruction(0x4470, 0x0007, &negw<indexed_indirect>);
    eu.set_instruction(0x4478, 0x0000, &negw<absolute_short>);
    eu.set_instruction(0x4479, 0x0000, &negw<absolute_long>);
    eu.set_instruction(0x4480, 0x0007, &negl<data_register>);
    eu.set_instruction(0x4490, 0x0007, &negl<indirect>);
    eu.set_instruction(0x4498, 0x0007, &negl<postinc_indirect>);
    eu.set_instruction(0x44a0, 0x0007, &negl<predec_indirect>);
    eu.set_instruction(0x44a8, 0x0007, &negl<disp_indirect>);
    eu.set_instruction(0x44b0, 0x0007, &negl<indexed_indirect>);
    eu.set_instruction(0x44b8, 0x0000, &negl<absolute_short>);
    eu.set_instruction(0x44b9, 0x0000, &negl<absolute_long>);
    eu.set_instruction(0x4600, 0x0007,
		       &m68k_not<byte_size, byte_d_register>);
    eu.set_instruction(0x4610, 0x0007,
		       &m68k_not<byte_size, byte_indirect>);
    eu.set_instruction(0x4618, 0x0007,
		       &m68k_not<byte_size, byte_postinc_indirect>);
    eu.set_instruction(0x4620, 0x0007,
		       &m68k_not<byte_size, byte_predec_indirect>);
    eu.set_instruction(0x4628, 0x0007,
		       &m68k_not<byte_size, byte_disp_indirect>);
    eu.set_instruction(0x4630, 0x0007,
		       &m68k_not<byte_size, byte_index_indirect>);
    eu.set_instruction(0x4638, 0x0000,
		       &m68k_not<byte_size, byte_abs_short>);
    eu.set_instruction(0x4639, 0x0000,
		       &m68k_not<byte_size, byte_abs_long>);
    eu.set_instruction(0x4640, 0x0007,
		       &m68k_not<word_size, word_d_register>);
    eu.set_instruction(0x4650, 0x0007,
		       &m68k_not<word_size, word_indirect>);
    eu.set_instruction(0x4658, 0x0007,
		       &m68k_not<word_size, word_postinc_indirect>);
    eu.set_instruction(0x4660, 0x0007,
		       &m68k_not<word_size, word_predec_indirect>);
    eu.set_instruction(0x4668, 0x0007,
		       &m68k_not<word_size, word_disp_indirect>);
    eu.set_instruction(0x4670, 0x0007,
		       &m68k_not<word_size, word_index_indirect>);
    eu.set_instruction(0x4678, 0x0000,
		       &m68k_not<word_size, word_abs_short>);
    eu.set_instruction(0x4679, 0x0000,
		       &m68k_not<word_size, word_abs_long>);
    eu.set_instruction(0x4680, 0x0007,
		       &m68k_not<long_word_size, long_word_d_register>);
    eu.set_instruction(0x4690, 0x0007,
		       &m68k_not<long_word_size, long_word_indirect>);
    eu.set_instruction(0x4698, 0x0007,
		       &m68k_not<long_word_size, long_word_postinc_indirect>);
    eu.set_instruction(0x46a0, 0x0007,
		       &m68k_not<long_word_size, long_word_predec_indirect>);
    eu.set_instruction(0x46a8, 0x0007,
		       &m68k_not<long_word_size, long_word_disp_indirect>);
    eu.set_instruction(0x46b0, 0x0007,
		       &m68k_not<long_word_size, long_word_index_indirect>);
    eu.set_instruction(0x46b8, 0x0000,
		       &m68k_not<long_word_size, long_word_abs_short>);
    eu.set_instruction(0x46b9, 0x0000,
		       &m68k_not<long_word_size, long_word_abs_long>);
    eu.set_instruction(0x46c0, 0x0007, &m68k_move_to_sr<word_d_register>);
    eu.set_instruction(0x46d0, 0x0007,
		       &m68k_move_to_sr<word_indirect>);
    eu.set_instruction(0x46d8, 0x0007,
		       &m68k_move_to_sr<word_postinc_indirect>);
    eu.set_instruction(0x46e0, 0x0007,
		       &m68k_move_to_sr<word_predec_indirect>);
    eu.set_instruction(0x46e8, 0x0007,
		       &m68k_move_to_sr<word_disp_indirect>);
    eu.set_instruction(0x46f0, 0x0007,
		       &m68k_move_to_sr<word_index_indirect>);
    eu.set_instruction(0x46f8, 0x0000,
		       &m68k_move_to_sr<word_abs_short>);
    eu.set_instruction(0x46f9, 0x0000,
		       &m68k_move_to_sr<word_abs_long>);
    eu.set_instruction(0x46fa, 0x0000,
		       &m68k_move_to_sr<word_disp_pc_indirect>);
    eu.set_instruction(0x46fb, 0x0000,
		       &m68k_move_to_sr<word_index_pc_indirect>);
    eu.set_instruction(0x46fc, 0x0000,
		       &m68k_move_to_sr<word_immediate>);
    eu.set_instruction(0x4840, 0x0007, &swapw);
    eu.set_instruction(0x4850, 0x0007, &pea<indirect>);
    eu.set_instruction(0x4868, 0x0007, &pea<disp_indirect>);
    eu.set_instruction(0x4870, 0x0007, &pea<indexed_indirect>);
    eu.set_instruction(0x4878, 0x0000, &pea<absolute_short>);
    eu.set_instruction(0x4879, 0x0000, &pea<absolute_long>);
    eu.set_instruction(0x487a, 0x0000, &pea<disp_pc>);
    eu.set_instruction(0x4880, 0x0007, &extw);
    eu.set_instruction(0x4890, 0x0007,
		       &m68k_movem_r_m<word_size, word_indirect>);
    eu.set_instruction(0x48a8, 0x0007,
		       &m68k_movem_r_m<word_size, word_disp_indirect>);
    eu.set_instruction(0x48b0, 0x0007,
		       &m68k_movem_r_m<word_size, word_index_indirect>);
    eu.set_instruction(0x48b8, 0x0000,
		       &m68k_movem_r_m<word_size, word_abs_short>);
    eu.set_instruction(0x48b9, 0x0000,
		       &m68k_movem_r_m<word_size, word_abs_long>);
    eu.set_instruction(0x48c0, 0x0007, &extl);
    eu.set_instruction(0x48d0, 0x0007,
		       &m68k_movem_r_m<long_word_size, long_word_indirect>);
    eu.set_instruction(0x48e0, 0x0007, &moveml_r_predec);
    eu.set_instruction(0x48e8, 0x0007,
		       &m68k_movem_r_m<long_word_size,
		                       long_word_disp_indirect>);
    eu.set_instruction(0x48f0, 0x0007,
		       &m68k_movem_r_m<long_word_size,
		                       long_word_index_indirect>);
    eu.set_instruction(0x48f8, 0x0000,
		       &m68k_movem_r_m<long_word_size, long_word_abs_short>);
    eu.set_instruction(0x48f9, 0x0000,
		       &m68k_movem_r_m<long_word_size, long_word_abs_long>);
    eu.set_instruction(0x4a00, 0x0007, &tstb<data_register>);
    eu.set_instruction(0x4a10, 0x0007, &tstb<indirect>);
    eu.set_instruction(0x4a18, 0x0007, &tstb<postinc_indirect>);
    eu.set_instruction(0x4a20, 0x0007, &tstb<predec_indirect>);
    eu.set_instruction(0x4a28, 0x0007, &tstb<disp_indirect>);
    eu.set_instruction(0x4a30, 0x0007, &tstb<indexed_indirect>);
    eu.set_instruction(0x4a38, 0x0000, &tstb<absolute_short>);
    eu.set_instruction(0x4a39, 0x0000, &tstb<absolute_long>);
    eu.set_instruction(0x4a40, 0x0007, &tstw<data_register>);
    eu.set_instruction(0x4a50, 0x0007, &tstw<indirect>);
    eu.set_instruction(0x4a58, 0x0007, &tstw<postinc_indirect>);
    eu.set_instruction(0x4a60, 0x0007, &tstw<predec_indirect>);
    eu.set_instruction(0x4a68, 0x0007, &tstw<disp_indirect>);
    eu.set_instruction(0x4a70, 0x0007, &tstw<indexed_indirect>);
    eu.set_instruction(0x4a78, 0x0000, &tstw<absolute_short>);
    eu.set_instruction(0x4a79, 0x0000, &tstw<absolute_long>);
    eu.set_instruction(0x4a80, 0x0007, &tstl<data_register>);
    eu.set_instruction(0x4a90, 0x0007, &tstl<indirect>);
    eu.set_instruction(0x4a98, 0x0007, &tstl<postinc_indirect>);
    eu.set_instruction(0x4aa0, 0x0007, &tstl<predec_indirect>);
    eu.set_instruction(0x4aa8, 0x0007, &tstl<disp_indirect>);
    eu.set_instruction(0x4ab0, 0x0007, &tstl<indexed_indirect>);
    eu.set_instruction(0x4ab8, 0x0000, &tstl<absolute_short>);
    eu.set_instruction(0x4ab9, 0x0000, &tstl<absolute_long>);
    eu.set_instruction(0x4cd0, 0x0007, &moveml_mr<indirect>);
    eu.set_instruction(0x4cd8, 0x0007, &moveml_mr<postinc_indirect>);
    eu.set_instruction(0x4ce8, 0x0007, &moveml_mr<disp_indirect>);
    eu.set_instruction(0x4cf0, 0x0007, &moveml_mr<indexed_indirect>);
    eu.set_instruction(0x4cf8, 0x0000, &moveml_mr<absolute_short>);
    eu.set_instruction(0x4cf9, 0x0000, &moveml_mr<absolute_long>);
    eu.set_instruction(0x4cfa, 0x0000, &moveml_mr<disp_pc>);
    eu.set_instruction(0x4e50, 0x0007, &link_a);
    eu.set_instruction(0x4e58, 0x0007, &unlk_a);
    eu.set_instruction(0x4e60, 0x0007, &m68k_move_to_usp);
    eu.set_instruction(0x4e68, 0x0007, &m68k_move_from_usp);
    eu.set_instruction(0x4e71, 0x0000, &m68k_nop);
    eu.set_instruction(0x4e73, 0x0000, &m68k_rte);
    eu.set_instruction(0x4e75, 0x0000, &rts);
    eu.set_instruction(0x4e90, 0x0007, &jsr<indirect>);
    eu.set_instruction(0x4ea8, 0x0007, &jsr<disp_indirect>);
    eu.set_instruction(0x4eb0, 0x0007, &jsr<indexed_indirect>);
    eu.set_instruction(0x4eb8, 0x0000, &jsr<absolute_short>);
    eu.set_instruction(0x4eb9, 0x0000, &jsr<absolute_long>);
    eu.set_instruction(0x4eba, 0x0000, &jsr<disp_pc>);
    eu.set_instruction(0x4ebb, 0x0000, &jsr<indexed_pc_indirect>);
    eu.set_instruction(0x4ed0, 0x0007, &jmp<indirect>);
    eu.set_instruction(0x4ee8, 0x0007, &jmp<disp_indirect>);
    eu.set_instruction(0x4ef0, 0x0007, &jmp<indexed_indirect>);
    eu.set_instruction(0x4ef8, 0x0000, &jmp<absolute_short>);
    eu.set_instruction(0x4ef9, 0x0000, &jmp<absolute_long>);
    eu.set_instruction(0x4efa, 0x0000, &jmp<disp_pc>);
    eu.set_instruction(0x4efb, 0x0000, &jmp<indexed_pc_indirect>);
    eu.set_instruction(0x5000, 0x0e07, &m68k_addq<byte_size, byte_d_register>);
    eu.set_instruction(0x5010, 0x0e07, &m68k_addq<byte_size, byte_indirect>);
    eu.set_instruction(0x5018, 0x0e07,
		       &m68k_addq<byte_size, byte_postinc_indirect>);
    eu.set_instruction(0x5020, 0x0e07,
		       &m68k_addq<byte_size, byte_predec_indirect>);
    eu.set_instruction(0x5028, 0x0e07,
		       &m68k_addq<byte_size, byte_disp_indirect>);
    eu.set_instruction(0x5030, 0x0e07,
		       &m68k_addq<byte_size, byte_index_indirect>);
    eu.set_instruction(0x5038, 0x0e00, &m68k_addq<byte_size, byte_abs_short>);
    eu.set_instruction(0x5039, 0x0e00, &m68k_addq<byte_size, byte_abs_long>);
    eu.set_instruction(0x5040, 0x0e07, &m68k_addq<word_size, word_d_register>);
    eu.set_instruction(0x5048, 0x0e07, &m68k_addq_a<word_size>);
    eu.set_instruction(0x5050, 0x0e07, &m68k_addq<word_size, word_indirect>);
    eu.set_instruction(0x5058, 0x0e07,
		       &m68k_addq<word_size, word_postinc_indirect>);
    eu.set_instruction(0x5060, 0x0e07,
		       &m68k_addq<word_size, word_predec_indirect>);
    eu.set_instruction(0x5068, 0x0e07,
		       &m68k_addq<word_size, word_disp_indirect>);
    eu.set_instruction(0x5070, 0x0e07,
		       &m68k_addq<word_size, word_index_indirect>);
    eu.set_instruction(0x5078, 0x0e00, &m68k_addq<word_size, word_abs_short>);
    eu.set_instruction(0x5079, 0x0e00, &m68k_addq<word_size, word_abs_long>);
    eu.set_instruction(0x5080, 0x0e07,
		       &m68k_addq<long_word_size, long_word_d_register>);
    eu.set_instruction(0x5088, 0x0e07, &m68k_addq_a<long_word_size>);
    eu.set_instruction(0x5090, 0x0e07,
		       &m68k_addq<long_word_size, long_word_indirect>);
    eu.set_instruction(0x5098, 0x0e07,
		       &m68k_addq<long_word_size, long_word_postinc_indirect>);
    eu.set_instruction(0x50a0, 0x0e07,
		       &m68k_addq<long_word_size, long_word_predec_indirect>);
    eu.set_instruction(0x50a8, 0x0e07,
		       &m68k_addq<long_word_size, long_word_disp_indirect>);
    eu.set_instruction(0x50b0, 0x0e07,
		       &m68k_addq<long_word_size, long_word_index_indirect>);
    eu.set_instruction(0x50b8, 0x0e00,
		       &m68k_addq<long_word_size, long_word_abs_short>);
    eu.set_instruction(0x50b9, 0x0e00,
		       &m68k_addq<long_word_size, long_word_abs_long>);
    eu.set_instruction(0x50c0, 0x0007, &s_b<t, data_register>);
    eu.set_instruction(0x50c8, 0x0007, &m68k_db<t>);
    eu.set_instruction(0x50d0, 0x0007, &s_b<t, indirect>);
    eu.set_instruction(0x50d8, 0x0007, &s_b<t, postinc_indirect>);
    eu.set_instruction(0x50e0, 0x0007, &s_b<t, predec_indirect>);
    eu.set_instruction(0x50e8, 0x0007, &s_b<t, disp_indirect>);
    eu.set_instruction(0x50f0, 0x0007, &s_b<t, indexed_indirect>);
    eu.set_instruction(0x50f8, 0x0000, &s_b<t, absolute_short>);
    eu.set_instruction(0x50f9, 0x0000, &s_b<t, absolute_long>);
    eu.set_instruction(0x5100, 0x0e07, &m68k_subq<byte_size, byte_d_register>);
    eu.set_instruction(0x5110, 0x0e07, &m68k_subq<byte_size, byte_indirect>);
    eu.set_instruction(0x5118, 0x0e07,
		       &m68k_subq<byte_size, byte_postinc_indirect>);
    eu.set_instruction(0x5120, 0x0e07,
		       &m68k_subq<byte_size, byte_predec_indirect>);
    eu.set_instruction(0x5128, 0x0e07,
		       &m68k_subq<byte_size, byte_disp_indirect>);
    eu.set_instruction(0x5130, 0x0e07,
		       &m68k_subq<byte_size, byte_index_indirect>);
    eu.set_instruction(0x5138, 0x0e00, &m68k_subq<byte_size, byte_abs_short>);
    eu.set_instruction(0x5139, 0x0e00, &m68k_subq<byte_size, byte_abs_long>);
    eu.set_instruction(0x5140, 0x0e07, &m68k_subq<word_size, word_d_register>);
    eu.set_instruction(0x5148, 0x0e07, &m68k_subq_a<word_size>);
    eu.set_instruction(0x5150, 0x0e07, &m68k_subq<word_size, word_indirect>);
    eu.set_instruction(0x5158, 0x0e07,
		       &m68k_subq<word_size, word_postinc_indirect>);
    eu.set_instruction(0x5160, 0x0e07,
		       &m68k_subq<word_size, word_predec_indirect>);
    eu.set_instruction(0x5168, 0x0e07,
		       &m68k_subq<word_size, word_disp_indirect>);
    eu.set_instruction(0x5170, 0x0e07,
		       &m68k_subq<word_size, word_index_indirect>);
    eu.set_instruction(0x5178, 0x0e00, &m68k_subq<word_size, word_abs_short>);
    eu.set_instruction(0x5179, 0x0e00, &m68k_subq<word_size, word_abs_long>);
    eu.set_instruction(0x5180, 0x0e07,
		       &m68k_subq<long_word_size, long_word_d_register>);
    eu.set_instruction(0x5188, 0x0e07, &m68k_subq_a<long_word_size>);
    eu.set_instruction(0x5190, 0x0e07,
		       &m68k_subq<long_word_size, long_word_indirect>);
    eu.set_instruction(0x5198, 0x0e07,
		       &m68k_subq<long_word_size, long_word_postinc_indirect>);
    eu.set_instruction(0x51a0, 0x0e07,
		       &m68k_subq<long_word_size, long_word_predec_indirect>);
    eu.set_instruction(0x51a8, 0x0e07,
		       &m68k_subq<long_word_size, long_word_disp_indirect>);
    eu.set_instruction(0x51b0, 0x0e07,
		       &m68k_subq<long_word_size, long_word_index_indirect>);
    eu.set_instruction(0x51b8, 0x0e00,
		       &m68k_subq<long_word_size, long_word_abs_short>);
    eu.set_instruction(0x51b9, 0x0e00,
		       &m68k_subq<long_word_size, long_word_abs_long>);
    eu.set_instruction(0x51c0, 0x0007, &s_b<f, data_register>);
    eu.set_instruction(0x51c8, 0x0007, &m68k_db<f>);
    eu.set_instruction(0x51d0, 0x0007, &s_b<f, indirect>);
    eu.set_instruction(0x51d8, 0x0007, &s_b<f, postinc_indirect>);
    eu.set_instruction(0x51e0, 0x0007, &s_b<f, predec_indirect>);
    eu.set_instruction(0x51e8, 0x0007, &s_b<f, disp_indirect>);
    eu.set_instruction(0x51f0, 0x0007, &s_b<f, indexed_indirect>);
    eu.set_instruction(0x51f8, 0x0000, &s_b<f, absolute_short>);
    eu.set_instruction(0x51f9, 0x0000, &s_b<f, absolute_long>);
    eu.set_instruction(0x52c0, 0x0007, &s_b<hi, data_register>);
    eu.set_instruction(0x52c8, 0x0007, &m68k_db<hi>);
    eu.set_instruction(0x52d0, 0x0007, &s_b<hi, indirect>);
    eu.set_instruction(0x52d8, 0x0007, &s_b<hi, postinc_indirect>);
    eu.set_instruction(0x52e0, 0x0007, &s_b<hi, predec_indirect>);
    eu.set_instruction(0x52e8, 0x0007, &s_b<hi, disp_indirect>);
    eu.set_instruction(0x52f0, 0x0007, &s_b<hi, indexed_indirect>);
    eu.set_instruction(0x52f8, 0x0000, &s_b<hi, absolute_short>);
    eu.set_instruction(0x52f9, 0x0000, &s_b<hi, absolute_long>);
    eu.set_instruction(0x53c0, 0x0007, &s_b<ls, data_register>);
    eu.set_instruction(0x53c8, 0x0007, &m68k_db<ls>);
    eu.set_instruction(0x53d0, 0x0007, &s_b<ls, indirect>);
    eu.set_instruction(0x53d8, 0x0007, &s_b<ls, postinc_indirect>);
    eu.set_instruction(0x53e0, 0x0007, &s_b<ls, predec_indirect>);
    eu.set_instruction(0x53e8, 0x0007, &s_b<ls, disp_indirect>);
    eu.set_instruction(0x53f0, 0x0007, &s_b<ls, indexed_indirect>);
    eu.set_instruction(0x53f8, 0x0000, &s_b<ls, absolute_short>);
    eu.set_instruction(0x53f9, 0x0000, &s_b<ls, absolute_long>);
    eu.set_instruction(0x54c0, 0x0007, &s_b<cc, data_register>);
    eu.set_instruction(0x54c8, 0x0007, &m68k_db<cc>);
    eu.set_instruction(0x54d0, 0x0007, &s_b<cc, indirect>);
    eu.set_instruction(0x54d8, 0x0007, &s_b<cc, postinc_indirect>);
    eu.set_instruction(0x54e0, 0x0007, &s_b<cc, predec_indirect>);
    eu.set_instruction(0x54e8, 0x0007, &s_b<cc, disp_indirect>);
    eu.set_instruction(0x54f0, 0x0007, &s_b<cc, indexed_indirect>);
    eu.set_instruction(0x54f8, 0x0000, &s_b<cc, absolute_short>);
    eu.set_instruction(0x54f9, 0x0000, &s_b<cc, absolute_long>);
    eu.set_instruction(0x55c0, 0x0007, &s_b<cs, data_register>);
    eu.set_instruction(0x55c8, 0x0007, &m68k_db<cs>);
    eu.set_instruction(0x55d0, 0x0007, &s_b<cs, indirect>);
    eu.set_instruction(0x55d8, 0x0007, &s_b<cs, postinc_indirect>);
    eu.set_instruction(0x55e0, 0x0007, &s_b<cs, predec_indirect>);
    eu.set_instruction(0x55e8, 0x0007, &s_b<cs, disp_indirect>);
    eu.set_instruction(0x55f0, 0x0007, &s_b<cs, indexed_indirect>);
    eu.set_instruction(0x55f8, 0x0000, &s_b<cs, absolute_short>);
    eu.set_instruction(0x55f9, 0x0000, &s_b<cs, absolute_long>);
    eu.set_instruction(0x56c0, 0x0007, &s_b<ne, data_register>);
    eu.set_instruction(0x56c8, 0x0007, &m68k_db<ne>);
    eu.set_instruction(0x56d0, 0x0007, &s_b<ne, indirect>);
    eu.set_instruction(0x56d8, 0x0007, &s_b<ne, postinc_indirect>);
    eu.set_instruction(0x56e0, 0x0007, &s_b<ne, predec_indirect>);
    eu.set_instruction(0x56e8, 0x0007, &s_b<ne, disp_indirect>);
    eu.set_instruction(0x56f0, 0x0007, &s_b<ne, indexed_indirect>);
    eu.set_instruction(0x56f8, 0x0000, &s_b<ne, absolute_short>);
    eu.set_instruction(0x56f9, 0x0000, &s_b<ne, absolute_long>);
    eu.set_instruction(0x57c0, 0x0007, &s_b<eq, data_register>);
    eu.set_instruction(0x57c8, 0x0007, &m68k_db<eq>);
    eu.set_instruction(0x57d0, 0x0007, &s_b<eq, indirect>);
    eu.set_instruction(0x57d8, 0x0007, &s_b<eq, postinc_indirect>);
    eu.set_instruction(0x57e0, 0x0007, &s_b<eq, predec_indirect>);
    eu.set_instruction(0x57e8, 0x0007, &s_b<eq, disp_indirect>);
    eu.set_instruction(0x57f0, 0x0007, &s_b<eq, indexed_indirect>);
    eu.set_instruction(0x57f8, 0x0000, &s_b<eq, absolute_short>);
    eu.set_instruction(0x57f9, 0x0000, &s_b<eq, absolute_long>);
    eu.set_instruction(0x5ac0, 0x0007, &s_b<pl, data_register>);
    eu.set_instruction(0x5ac8, 0x0007, &m68k_db<pl>);
    eu.set_instruction(0x5ad0, 0x0007, &s_b<pl, indirect>);
    eu.set_instruction(0x5ad8, 0x0007, &s_b<pl, postinc_indirect>);
    eu.set_instruction(0x5ae0, 0x0007, &s_b<pl, predec_indirect>);
    eu.set_instruction(0x5ae8, 0x0007, &s_b<pl, disp_indirect>);
    eu.set_instruction(0x5af0, 0x0007, &s_b<pl, indexed_indirect>);
    eu.set_instruction(0x5af8, 0x0000, &s_b<pl, absolute_short>);
    eu.set_instruction(0x5af9, 0x0000, &s_b<pl, absolute_long>);
    eu.set_instruction(0x5bc0, 0x0007, &s_b<mi, data_register>);
    eu.set_instruction(0x5bc8, 0x0007, &m68k_db<mi>);
    eu.set_instruction(0x5bd0, 0x0007, &s_b<mi, indirect>);
    eu.set_instruction(0x5bd8, 0x0007, &s_b<mi, postinc_indirect>);
    eu.set_instruction(0x5be0, 0x0007, &s_b<mi, predec_indirect>);
    eu.set_instruction(0x5be8, 0x0007, &s_b<mi, disp_indirect>);
    eu.set_instruction(0x5bf0, 0x0007, &s_b<mi, indexed_indirect>);
    eu.set_instruction(0x5bf8, 0x0000, &s_b<mi, absolute_short>);
    eu.set_instruction(0x5bf9, 0x0000, &s_b<mi, absolute_long>);
    eu.set_instruction(0x5cc0, 0x0007, &s_b<ge, data_register>);
    eu.set_instruction(0x5cc8, 0x0007, &m68k_db<ge>);
    eu.set_instruction(0x5cd0, 0x0007, &s_b<ge, indirect>);
    eu.set_instruction(0x5cd8, 0x0007, &s_b<ge, postinc_indirect>);
    eu.set_instruction(0x5ce0, 0x0007, &s_b<ge, predec_indirect>);
    eu.set_instruction(0x5ce8, 0x0007, &s_b<ge, disp_indirect>);
    eu.set_instruction(0x5cf0, 0x0007, &s_b<ge, indexed_indirect>);
    eu.set_instruction(0x5cf8, 0x0000, &s_b<ge, absolute_short>);
    eu.set_instruction(0x5cf9, 0x0000, &s_b<ge, absolute_long>);
    eu.set_instruction(0x5dc0, 0x0007, &s_b<lt, data_register>);
    eu.set_instruction(0x5dc8, 0x0007, &m68k_db<lt>);
    eu.set_instruction(0x5dd0, 0x0007, &s_b<lt, indirect>);
    eu.set_instruction(0x5dd8, 0x0007, &s_b<lt, postinc_indirect>);
    eu.set_instruction(0x5de0, 0x0007, &s_b<lt, predec_indirect>);
    eu.set_instruction(0x5de8, 0x0007, &s_b<lt, disp_indirect>);
    eu.set_instruction(0x5df0, 0x0007, &s_b<lt, indexed_indirect>);
    eu.set_instruction(0x5df8, 0x0000, &s_b<lt, absolute_short>);
    eu.set_instruction(0x5df9, 0x0000, &s_b<lt, absolute_long>);
    eu.set_instruction(0x5ec0, 0x0007, &s_b<gt, data_register>);
    eu.set_instruction(0x5ec8, 0x0007, &m68k_db<gt>);
    eu.set_instruction(0x5ed0, 0x0007, &s_b<gt, indirect>);
    eu.set_instruction(0x5ed8, 0x0007, &s_b<gt, postinc_indirect>);
    eu.set_instruction(0x5ee0, 0x0007, &s_b<gt, predec_indirect>);
    eu.set_instruction(0x5ee8, 0x0007, &s_b<gt, disp_indirect>);
    eu.set_instruction(0x5ef0, 0x0007, &s_b<gt, indexed_indirect>);
    eu.set_instruction(0x5ef8, 0x0000, &s_b<gt, absolute_short>);
    eu.set_instruction(0x5ef9, 0x0000, &s_b<gt, absolute_long>);
    eu.set_instruction(0x5fc0, 0x0007, &s_b<le, data_register>);
    eu.set_instruction(0x5fc8, 0x0007, &m68k_db<le>);
    eu.set_instruction(0x5fd0, 0x0007, &s_b<le, indirect>);
    eu.set_instruction(0x5fd8, 0x0007, &s_b<le, postinc_indirect>);
    eu.set_instruction(0x5fe0, 0x0007, &s_b<le, predec_indirect>);
    eu.set_instruction(0x5fe8, 0x0007, &s_b<le, disp_indirect>);
    eu.set_instruction(0x5ff0, 0x0007, &s_b<le, indexed_indirect>);
    eu.set_instruction(0x5ff8, 0x0000, &s_b<le, absolute_short>);
    eu.set_instruction(0x5ff9, 0x0000, &s_b<le, absolute_long>);
    eu.set_instruction(0x6000, 0x00ff, &bra);
    eu.set_instruction(0x6100, 0x00ff, &bsr);
    eu.set_instruction(0x6200, 0x00ff, &m68k_b<hi>);
    eu.set_instruction(0x6300, 0x00ff, &m68k_b<ls>);
    eu.set_instruction(0x6400, 0x00ff, &m68k_b<cc>);
    eu.set_instruction(0x6500, 0x00ff, &m68k_b<cs>);
    eu.set_instruction(0x6600, 0x00ff, &m68k_b<ne>);
    eu.set_instruction(0x6700, 0x00ff, &m68k_b<eq>);
    eu.set_instruction(0x6a00, 0x00ff, &m68k_b<pl>);
    eu.set_instruction(0x6b00, 0x00ff, &m68k_b<mi>);
    eu.set_instruction(0x6c00, 0x00ff, &m68k_b<ge>);
    eu.set_instruction(0x6d00, 0x00ff, &m68k_b<lt>);
    eu.set_instruction(0x6e00, 0x00ff, &m68k_b<gt>);
    eu.set_instruction(0x6f00, 0x00ff, &m68k_b<le>);
    eu.set_instruction(0x7000, 0x0eff, &moveql_d);
    eu.set_instruction(0x8000, 0x0e07, &orb<data_register>);
    eu.set_instruction(0x8010, 0x0e07, &orb<indirect>);
    eu.set_instruction(0x8018, 0x0e07, &orb<postinc_indirect>);
    eu.set_instruction(0x8020, 0x0e07, &orb<predec_indirect>);
    eu.set_instruction(0x8028, 0x0e07, &orb<disp_indirect>);
    eu.set_instruction(0x8030, 0x0e07, &orb<indexed_indirect>);
    eu.set_instruction(0x8038, 0x0e00, &orb<absolute_short>);
    eu.set_instruction(0x8039, 0x0e00, &orb<absolute_long>);
    eu.set_instruction(0x803c, 0x0e00, &orb<immediate>);
    eu.set_instruction(0x8040, 0x0e07, &orw<data_register>);
    eu.set_instruction(0x8050, 0x0e07, &orw<indirect>);
    eu.set_instruction(0x8058, 0x0e07, &orw<postinc_indirect>);
    eu.set_instruction(0x8060, 0x0e07, &orw<predec_indirect>);
    eu.set_instruction(0x8068, 0x0e07, &orw<disp_indirect>);
    eu.set_instruction(0x8070, 0x0e07, &orw<indexed_indirect>);
    eu.set_instruction(0x8078, 0x0e00, &orw<absolute_short>);
    eu.set_instruction(0x8079, 0x0e00, &orw<absolute_long>);
    eu.set_instruction(0x807a, 0x0e00, &orw<disp_pc>);
    eu.set_instruction(0x807c, 0x0e00, &orw<immediate>);
    eu.set_instruction(0x8080, 0x0e07, &orl<data_register>);
    eu.set_instruction(0x8090, 0x0e07, &orl<indirect>);
    eu.set_instruction(0x8098, 0x0e07, &orl<postinc_indirect>);
    eu.set_instruction(0x80a0, 0x0e07, &orl<predec_indirect>);
    eu.set_instruction(0x80a8, 0x0e07, &orl<disp_indirect>);
    eu.set_instruction(0x80b0, 0x0e07, &orl<indexed_indirect>);
    eu.set_instruction(0x80b8, 0x0e00, &orl<absolute_short>);
    eu.set_instruction(0x80b9, 0x0e00, &orl<absolute_long>);
    eu.set_instruction(0x80ba, 0x0e00, &orl<disp_pc>);
    eu.set_instruction(0x80bc, 0x0e00, &orl<immediate>);
    eu.set_instruction(0x80c0, 0x0e07, &divuw<data_register>);
    eu.set_instruction(0x80d0, 0x0e07, &divuw<indirect>);
    eu.set_instruction(0x80d8, 0x0e07, &divuw<postinc_indirect>);
    eu.set_instruction(0x80e0, 0x0e07, &divuw<predec_indirect>);
    eu.set_instruction(0x80e8, 0x0e07, &divuw<disp_indirect>);
    eu.set_instruction(0x80f0, 0x0e07, &divuw<indexed_indirect>);
    eu.set_instruction(0x80f8, 0x0e00, &divuw<absolute_short>);
    eu.set_instruction(0x80f9, 0x0e00, &divuw<absolute_long>);
    eu.set_instruction(0x80fa, 0x0e00, &divuw<disp_pc>);
    eu.set_instruction(0x80fc, 0x0e00, &divuw<immediate>);
    eu.set_instruction(0x8110, 0x0e07,
		       &m68k_or_m<byte_size, byte_indirect>);
    eu.set_instruction(0x8118, 0x0e07,
		       &m68k_or_m<byte_size, byte_postinc_indirect>);
    eu.set_instruction(0x8120, 0x0e07,
		       &m68k_or_m<byte_size, byte_predec_indirect>);
    eu.set_instruction(0x8128, 0x0e07,
		       &m68k_or_m<byte_size, byte_disp_indirect>);
    eu.set_instruction(0x8130, 0x0e07,
		       &m68k_or_m<byte_size, byte_index_indirect>);
    eu.set_instruction(0x8138, 0x0e00,
		       &m68k_or_m<byte_size, byte_abs_short>);
    eu.set_instruction(0x8139, 0x0e00,
		       &m68k_or_m<byte_size, byte_abs_long>);
    eu.set_instruction(0x8150, 0x0e07,
		       &m68k_or_m<word_size, word_indirect>);
    eu.set_instruction(0x8158, 0x0e07,
		       &m68k_or_m<word_size, word_postinc_indirect>);
    eu.set_instruction(0x8160, 0x0e07,
		       &m68k_or_m<word_size, word_predec_indirect>);
    eu.set_instruction(0x8168, 0x0e07,
		       &m68k_or_m<word_size, word_disp_indirect>);
    eu.set_instruction(0x8170, 0x0e07,
		       &m68k_or_m<word_size, word_index_indirect>);
    eu.set_instruction(0x8178, 0x0e00,
		       &m68k_or_m<word_size, word_abs_short>);
    eu.set_instruction(0x8179, 0x0e00,
		       &m68k_or_m<word_size, word_abs_long>);
    eu.set_instruction(0x8190, 0x0e07,
		       &m68k_or_m<long_word_size, long_word_indirect>);
    eu.set_instruction(0x8198, 0x0e07,
		       &m68k_or_m<long_word_size, long_word_postinc_indirect>);
    eu.set_instruction(0x81a0, 0x0e07,
		       &m68k_or_m<long_word_size, long_word_predec_indirect>);
    eu.set_instruction(0x81a8, 0x0e07,
		       &m68k_or_m<long_word_size, long_word_disp_indirect>);
    eu.set_instruction(0x81b0, 0x0e07,
		       &m68k_or_m<long_word_size, long_word_index_indirect>);
    eu.set_instruction(0x81b8, 0x0e00,
		       &m68k_or_m<long_word_size, long_word_abs_short>);
    eu.set_instruction(0x81b9, 0x0e00,
		       &m68k_or_m<long_word_size, long_word_abs_long>);
    eu.set_instruction(0x9000, 0x0e07, &subb<data_register>);
    eu.set_instruction(0x9010, 0x0e07, &subb<indirect>);
    eu.set_instruction(0x9018, 0x0e07, &subb<postinc_indirect>);
    eu.set_instruction(0x9020, 0x0e07, &subb<predec_indirect>);
    eu.set_instruction(0x9028, 0x0e07, &subb<disp_indirect>);
    eu.set_instruction(0x9030, 0x0e07, &subb<indexed_indirect>);
    eu.set_instruction(0x9038, 0x0e00, &subb<absolute_short>);
    eu.set_instruction(0x9039, 0x0e00, &subb<absolute_long>);
    eu.set_instruction(0x903a, 0x0e00, &subb<disp_pc>);
    eu.set_instruction(0x903c, 0x0e00, &subb<immediate>);
    eu.set_instruction(0x9040, 0x0e07, &subw<data_register>);
    eu.set_instruction(0x9048, 0x0e07, &subw<address_register>);
    eu.set_instruction(0x9050, 0x0e07, &subw<indirect>);
    eu.set_instruction(0x9058, 0x0e07, &subw<postinc_indirect>);
    eu.set_instruction(0x9060, 0x0e07, &subw<predec_indirect>);
    eu.set_instruction(0x9068, 0x0e07, &subw<disp_indirect>);
    eu.set_instruction(0x9070, 0x0e07, &subw<indexed_indirect>);
    eu.set_instruction(0x9078, 0x0e00, &subw<absolute_short>);
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
    eu.set_instruction(0x90b8, 0x0e00, &subl<absolute_short>);
    eu.set_instruction(0x90b9, 0x0e00, &subl<absolute_long>);
    eu.set_instruction(0x90bc, 0x0e00, &subl<immediate>);
    eu.set_instruction(0x90c0, 0x0e07,
		       &m68k_suba<word_size, word_d_register>);
    eu.set_instruction(0x90c8, 0x0e07,
		       &m68k_suba<word_size, word_a_register>);
    eu.set_instruction(0x90d0, 0x0e07,
		       &m68k_suba<word_size, word_indirect>);
    eu.set_instruction(0x90d8, 0x0e07,
		       &m68k_suba<word_size, word_postinc_indirect>);
    eu.set_instruction(0x90e0, 0x0e07,
		       &m68k_suba<word_size, word_predec_indirect>);
    eu.set_instruction(0x90e8, 0x0e07,
		       &m68k_suba<word_size, word_disp_indirect>);
    eu.set_instruction(0x90f0, 0x0e07,
		       &m68k_suba<word_size, word_index_indirect>);
    eu.set_instruction(0x90f8, 0x0e00,
		       &m68k_suba<word_size, word_abs_short>);
    eu.set_instruction(0x90f9, 0x0e00,
		       &m68k_suba<word_size, word_abs_long>);
    eu.set_instruction(0x90fa, 0x0e00,
		       &m68k_suba<word_size, word_disp_pc_indirect>);
    eu.set_instruction(0x90fb, 0x0e00,
		       &m68k_suba<word_size, word_index_pc_indirect>);
    eu.set_instruction(0x90fc, 0x0e00,
		       &m68k_suba<word_size, word_immediate>);
    eu.set_instruction(0x9110, 0x0e07,
		       &m68k_sub_m<byte_size, byte_indirect>);
    eu.set_instruction(0x9118, 0x0e07,
		       &m68k_sub_m<byte_size, byte_postinc_indirect>);
    eu.set_instruction(0x9120, 0x0e07,
		       &m68k_sub_m<byte_size, byte_predec_indirect>);
    eu.set_instruction(0x9128, 0x0e07,
		       &m68k_sub_m<byte_size, byte_disp_indirect>);
    eu.set_instruction(0x9130, 0x0e07,
		       &m68k_sub_m<byte_size, byte_index_indirect>);
    eu.set_instruction(0x9138, 0x0e00, &m68k_sub_m<byte_size, byte_abs_short>);
    eu.set_instruction(0x9139, 0x0e00, &m68k_sub_m<byte_size, byte_abs_long>);
    eu.set_instruction(0x9150, 0x0e07,
		       &m68k_sub_m<word_size, word_indirect>);
    eu.set_instruction(0x9158, 0x0e07,
		       &m68k_sub_m<word_size, word_postinc_indirect>);
    eu.set_instruction(0x9160, 0x0e07,
		       &m68k_sub_m<word_size, word_predec_indirect>);
    eu.set_instruction(0x9168, 0x0e07,
		       &m68k_sub_m<word_size, word_disp_indirect>);
    eu.set_instruction(0x9170, 0x0e07,
		       &m68k_sub_m<word_size, word_index_indirect>);
    eu.set_instruction(0x9178, 0x0e00, &m68k_sub_m<word_size, word_abs_short>);
    eu.set_instruction(0x9179, 0x0e00, &m68k_sub_m<word_size, word_abs_long>);
    eu.set_instruction(0x9190, 0x0e07,
		       &m68k_sub_m<long_word_size, long_word_indirect>);
    eu.set_instruction(0x9198, 0x0e07,
		       &m68k_sub_m<long_word_size,
		                   long_word_postinc_indirect>);
    eu.set_instruction(0x91a0, 0x0e07,
		       &m68k_sub_m<long_word_size, long_word_predec_indirect>);
    eu.set_instruction(0x91a8, 0x0e07,
		       &m68k_sub_m<long_word_size, long_word_disp_indirect>);
    eu.set_instruction(0x91b0, 0x0e07,
		       &m68k_sub_m<long_word_size, long_word_index_indirect>);
    eu.set_instruction(0x91b8, 0x0e00,
		       &m68k_sub_m<long_word_size, long_word_abs_short>);
    eu.set_instruction(0x91b9, 0x0e00,
		       &m68k_sub_m<long_word_size, long_word_abs_long>);
    eu.set_instruction(0x91c0, 0x0e07,
		       &m68k_suba<long_word_size, long_word_d_register>);
    eu.set_instruction(0x91c8, 0x0e07,
		       &m68k_suba<long_word_size, long_word_a_register>);
    eu.set_instruction(0x91d0, 0x0e07,
		       &m68k_suba<long_word_size, long_word_indirect>);
    eu.set_instruction(0x91d8, 0x0e07,
		       &m68k_suba<long_word_size, long_word_postinc_indirect>);
    eu.set_instruction(0x91e0, 0x0e07,
		       &m68k_suba<long_word_size, long_word_predec_indirect>);
    eu.set_instruction(0x91e8, 0x0e07,
		       &m68k_suba<long_word_size, long_word_disp_indirect>);
    eu.set_instruction(0x91f0, 0x0e07,
		       &m68k_suba<long_word_size, long_word_index_indirect>);
    eu.set_instruction(0x91f8, 0x0e00,
		       &m68k_suba<long_word_size, long_word_abs_short>);
    eu.set_instruction(0x91f9, 0x0e00,
		       &m68k_suba<long_word_size, long_word_abs_long>);
    eu.set_instruction(0x91fa, 0x0e00,
		       &m68k_suba<long_word_size, long_word_disp_pc_indirect>);
    eu.set_instruction(0x91fb, 0x0e00,
		       &m68k_suba<long_word_size, long_word_index_pc_indirect>);
    eu.set_instruction(0x91fc, 0x0e00,
		       &m68k_suba<long_word_size, long_word_immediate>);
    eu.set_instruction(0xb000, 0x0e07,
		       &m68k_cmp<byte_size, byte_d_register>);
    eu.set_instruction(0xb010, 0x0e07,
		       &m68k_cmp<byte_size, byte_indirect>);
    eu.set_instruction(0xb018, 0x0e07,
		       &m68k_cmp<byte_size, byte_postinc_indirect>);
    eu.set_instruction(0xb020, 0x0e07,
		       &m68k_cmp<byte_size, byte_predec_indirect>);
    eu.set_instruction(0xb028, 0x0e07,
		       &m68k_cmp<byte_size, byte_disp_indirect>);
    eu.set_instruction(0xb030, 0x0e07,
		       &m68k_cmp<byte_size, byte_index_indirect>);
    eu.set_instruction(0xb038, 0x0e00,
		       &m68k_cmp<byte_size, byte_abs_short>);
    eu.set_instruction(0xb039, 0x0e00,
		       &m68k_cmp<byte_size, byte_abs_long>);
    eu.set_instruction(0xb03a, 0x0e00,
		       &m68k_cmp<byte_size, byte_disp_pc_indirect>);
    eu.set_instruction(0xb03b, 0x0e00,
		       &m68k_cmp<byte_size, byte_index_pc_indirect>);
    eu.set_instruction(0xb03c, 0x0e00,
		       &m68k_cmp<byte_size, byte_immediate>);
    eu.set_instruction(0xb040, 0x0e07,
		       &m68k_cmp<word_size, word_d_register>);
    eu.set_instruction(0xb048, 0x0e07,
		       &m68k_cmp<word_size, word_a_register>);
    eu.set_instruction(0xb050, 0x0e07,
		       &m68k_cmp<word_size, word_indirect>);
    eu.set_instruction(0xb058, 0x0e07,
		       &m68k_cmp<word_size, word_postinc_indirect>);
    eu.set_instruction(0xb060, 0x0e07,
		       &m68k_cmp<word_size, word_predec_indirect>);
    eu.set_instruction(0xb068, 0x0e07,
		       &m68k_cmp<word_size, word_disp_indirect>);
    eu.set_instruction(0xb070, 0x0e07,
		       &m68k_cmp<word_size, word_index_indirect>);
    eu.set_instruction(0xb078, 0x0e00,
		       &m68k_cmp<word_size, word_abs_short>);
    eu.set_instruction(0xb079, 0x0e00,
		       &m68k_cmp<word_size, word_abs_long>);
    eu.set_instruction(0xb07a, 0x0e00,
		       &m68k_cmp<word_size, word_disp_pc_indirect>);
    eu.set_instruction(0xb07b, 0x0e00,
		       &m68k_cmp<word_size, word_index_pc_indirect>);
    eu.set_instruction(0xb07c, 0x0e00,
		       &m68k_cmp<word_size, word_immediate>);
    eu.set_instruction(0xb080, 0x0e07,
		       &m68k_cmp<long_word_size, long_word_d_register>);
    eu.set_instruction(0xb088, 0x0e07,
		       &m68k_cmp<long_word_size, long_word_a_register>);
    eu.set_instruction(0xb090, 0x0e07,
		       &m68k_cmp<long_word_size, long_word_indirect>);
    eu.set_instruction(0xb098, 0x0e07,
		       &m68k_cmp<long_word_size, long_word_postinc_indirect>);
    eu.set_instruction(0xb0a0, 0x0e07,
		       &m68k_cmp<long_word_size, long_word_predec_indirect>);
    eu.set_instruction(0xb0a8, 0x0e07,
		       &m68k_cmp<long_word_size, long_word_disp_indirect>);
    eu.set_instruction(0xb0b0, 0x0e07,
		       &m68k_cmp<long_word_size, long_word_index_indirect>);
    eu.set_instruction(0xb0b8, 0x0e00,
		       &m68k_cmp<long_word_size, long_word_abs_short>);
    eu.set_instruction(0xb0b9, 0x0e00,
		       &m68k_cmp<long_word_size, long_word_abs_long>);
    eu.set_instruction(0xb0ba, 0x0e00,
		       &m68k_cmp<long_word_size, long_word_disp_pc_indirect>);
    eu.set_instruction(0xb0bb, 0x0e00,
		       &m68k_cmp<long_word_size, long_word_index_pc_indirect>);
    eu.set_instruction(0xb0bc, 0x0e00,
		       &m68k_cmp<long_word_size, long_word_immediate>);
    eu.set_instruction(0xb0c0, 0x0e07, &cmpaw<data_register>);
    eu.set_instruction(0xb0c8, 0x0e07, &cmpaw<address_register>);
    eu.set_instruction(0xb0d0, 0x0e07, &cmpaw<indirect>);
    eu.set_instruction(0xb0d8, 0x0e07, &cmpaw<postinc_indirect>);
    eu.set_instruction(0xb0e0, 0x0e07, &cmpaw<predec_indirect>);
    eu.set_instruction(0xb0e8, 0x0e07, &cmpaw<disp_indirect>);
    eu.set_instruction(0xb0f0, 0x0e07, &cmpaw<indexed_indirect>);  
    eu.set_instruction(0xb0f8, 0x0e00, &cmpaw<absolute_short>);
    eu.set_instruction(0xb0f9, 0x0e00, &cmpaw<absolute_long>);
    eu.set_instruction(0xb0fa, 0x0e00, &cmpaw<disp_pc>);
    eu.set_instruction(0xb0fc, 0x0e00, &cmpaw<immediate>);
    eu.set_instruction(0xb100, 0x0e07, &eorb_r<data_register>);
    eu.set_instruction(0xb108, 0x0e07, &cmpmb);
    eu.set_instruction(0xb110, 0x0e07, &eorb_r<indirect>);
    eu.set_instruction(0xb118, 0x0e07, &eorb_r<postinc_indirect>);
    eu.set_instruction(0xb120, 0x0e07, &eorb_r<predec_indirect>);
    eu.set_instruction(0xb128, 0x0e07, &eorb_r<disp_indirect>);
    eu.set_instruction(0xb130, 0x0e07, &eorb_r<indexed_indirect>);
    eu.set_instruction(0xb138, 0x0e00, &eorb_r<absolute_short>);
    eu.set_instruction(0xb139, 0x0e00, &eorb_r<absolute_long>);
    eu.set_instruction(0xb140, 0x0e07, &eorw_r<data_register>);
    eu.set_instruction(0xb150, 0x0e07, &eorw_r<indirect>);
    eu.set_instruction(0xb158, 0x0e07, &eorw_r<postinc_indirect>);
    eu.set_instruction(0xb160, 0x0e07, &eorw_r<predec_indirect>);
    eu.set_instruction(0xb168, 0x0e07, &eorw_r<disp_indirect>);
    eu.set_instruction(0xb170, 0x0e07, &eorw_r<indexed_indirect>);
    eu.set_instruction(0xb178, 0x0e00, &eorw_r<absolute_short>);
    eu.set_instruction(0xb179, 0x0e00, &eorw_r<absolute_long>);
    eu.set_instruction(0xb180, 0x0e07, &eorl_r<data_register>);
    eu.set_instruction(0xb190, 0x0e07, &eorl_r<indirect>);
    eu.set_instruction(0xb198, 0x0e07, &eorl_r<postinc_indirect>);
    eu.set_instruction(0xb1a0, 0x0e07, &eorl_r<predec_indirect>);
    eu.set_instruction(0xb1a8, 0x0e07, &eorl_r<disp_indirect>);
    eu.set_instruction(0xb1b0, 0x0e07, &eorl_r<indexed_indirect>);
    eu.set_instruction(0xb1b8, 0x0e00, &eorl_r<absolute_short>);
    eu.set_instruction(0xb1b9, 0x0e00, &eorl_r<absolute_long>);
    eu.set_instruction(0xb1c0, 0x0e07, &cmpal<data_register>);
    eu.set_instruction(0xb1c8, 0x0e07, &cmpal<address_register>);
    eu.set_instruction(0xb1d0, 0x0e07, &cmpal<indirect>);
    eu.set_instruction(0xb1d8, 0x0e07, &cmpal<postinc_indirect>);
    eu.set_instruction(0xb1e0, 0x0e07, &cmpal<predec_indirect>);
    eu.set_instruction(0xb1e8, 0x0e07, &cmpal<disp_indirect>);
    eu.set_instruction(0xb1f0, 0x0e07, &cmpal<indexed_indirect>);
    eu.set_instruction(0xb1f8, 0x0e00, &cmpal<absolute_short>);
    eu.set_instruction(0xb1f9, 0x0e00, &cmpal<absolute_long>);
    eu.set_instruction(0xb1fa, 0x0e00, &cmpal<disp_pc>);
    eu.set_instruction(0xb1fc, 0x0e00, &cmpal<immediate>);
    eu.set_instruction(0xc000, 0x0e07, &m68k_and<byte_size, byte_d_register>);
    eu.set_instruction(0xc010, 0x0e07, &m68k_and<byte_size, byte_indirect>);
    eu.set_instruction(0xc018, 0x0e07,
		       &m68k_and<byte_size, byte_postinc_indirect>);
    eu.set_instruction(0xc020, 0x0e07,
		       &m68k_and<byte_size, byte_predec_indirect>);
    eu.set_instruction(0xc028, 0x0e07,
		       &m68k_and<byte_size, byte_disp_indirect>);
    eu.set_instruction(0xc030, 0x0e07,
		       &m68k_and<byte_size, byte_index_indirect>);
    eu.set_instruction(0xc038, 0x0e00, &m68k_and<byte_size, byte_abs_short>);
    eu.set_instruction(0xc039, 0x0e00, &m68k_and<byte_size, byte_abs_long>);
    eu.set_instruction(0xc03a, 0x0e00,
		       &m68k_and<byte_size, byte_disp_pc_indirect>);
    eu.set_instruction(0xc03b, 0x0e00,
		       &m68k_and<byte_size, byte_index_pc_indirect>);
    eu.set_instruction(0xc03c, 0x0e00, &m68k_and<byte_size, byte_immediate>);
    eu.set_instruction(0xc040, 0x0e07, &m68k_and<word_size, word_d_register>);
    eu.set_instruction(0xc050, 0x0e07, &m68k_and<word_size, word_indirect>);
    eu.set_instruction(0xc058, 0x0e07,
		       &m68k_and<word_size, word_postinc_indirect>);
    eu.set_instruction(0xc060, 0x0e07,
		       &m68k_and<word_size, word_predec_indirect>);
    eu.set_instruction(0xc068, 0x0e07,
		       &m68k_and<word_size, word_disp_indirect>);
    eu.set_instruction(0xc070, 0x0e07,
		       &m68k_and<word_size, word_index_indirect>);
    eu.set_instruction(0xc078, 0x0e00, &m68k_and<word_size, word_abs_short>);
    eu.set_instruction(0xc079, 0x0e00, &m68k_and<word_size, word_abs_long>);
    eu.set_instruction(0xc07a, 0x0e00,
		       &m68k_and<word_size, word_disp_pc_indirect>);
    eu.set_instruction(0xc07b, 0x0e00,
		       &m68k_and<word_size, word_index_pc_indirect>);
    eu.set_instruction(0xc07c, 0x0e00, &m68k_and<word_size, word_immediate>);
    eu.set_instruction(0xc080, 0x0e07,
		       &m68k_and<long_word_size, long_word_d_register>);
    eu.set_instruction(0xc090, 0x0e07,
		       &m68k_and<long_word_size, long_word_indirect>);
    eu.set_instruction(0xc098, 0x0e07,
		       &m68k_and<long_word_size, long_word_postinc_indirect>);
    eu.set_instruction(0xc0a0, 0x0e07,
		       &m68k_and<long_word_size, long_word_predec_indirect>);
    eu.set_instruction(0xc0a8, 0x0e07,
		       &m68k_and<long_word_size, long_word_disp_indirect>);
    eu.set_instruction(0xc0b0, 0x0e07,
		       &m68k_and<long_word_size, long_word_index_indirect>);
    eu.set_instruction(0xc0b8, 0x0e00,
		       &m68k_and<long_word_size, long_word_abs_short>);
    eu.set_instruction(0xc0b9, 0x0e00,
		       &m68k_and<long_word_size, long_word_abs_long>);
    eu.set_instruction(0xc0ba, 0x0e00,
		       &m68k_and<long_word_size, long_word_disp_pc_indirect>);
    eu.set_instruction(0xc0bb, 0x0e00,
		       &m68k_and<long_word_size, long_word_index_pc_indirect>);
    eu.set_instruction(0xc0bc, 0x0e00,
		       &m68k_and<long_word_size, long_word_immediate>);
    eu.set_instruction(0xc0c0, 0x0e07, &muluw<data_register>);
    eu.set_instruction(0xc0d0, 0x0e07, &muluw<indirect>);
    eu.set_instruction(0xc0d8, 0x0e07, &muluw<postinc_indirect>);
    eu.set_instruction(0xc0e0, 0x0e07, &muluw<predec_indirect>);
    eu.set_instruction(0xc0e8, 0x0e07, &muluw<disp_indirect>);
    eu.set_instruction(0xc0f0, 0x0e07, &muluw<indexed_indirect>);
    eu.set_instruction(0xc0f8, 0x0e00, &muluw<absolute_short>);
    eu.set_instruction(0xc0f9, 0x0e00, &muluw<absolute_long>);
    eu.set_instruction(0xc0fc, 0x0e00, &muluw<immediate>);
    eu.set_instruction(0xc110, 0x0e07,
		       &m68k_and_m<byte_size, byte_indirect>);
    eu.set_instruction(0xc118, 0x0e07,
		       &m68k_and_m<byte_size, byte_postinc_indirect>);
    eu.set_instruction(0xc120, 0x0e07,
		       &m68k_and_m<byte_size, byte_predec_indirect>);
    eu.set_instruction(0xc128, 0x0e07,
		       &m68k_and_m<byte_size, byte_disp_indirect>);
    eu.set_instruction(0xc130, 0x0e07,
		       &m68k_and_m<byte_size, byte_index_indirect>);
    eu.set_instruction(0xc138, 0x0e00,
		       &m68k_and_m<byte_size, byte_abs_short>);
    eu.set_instruction(0xc139, 0x0e00,
		       &m68k_and_m<byte_size, byte_abs_long>);
    eu.set_instruction(0xc140, 0x0e07, &exgl<data_register, data_register>);
    eu.set_instruction(0xc148, 0x0e07, &exgl<address_register, address_register>);
    eu.set_instruction(0xc150, 0x0e07,
		       &m68k_and_m<word_size, word_indirect>);
    eu.set_instruction(0xc158, 0x0e07,
		       &m68k_and_m<word_size, word_postinc_indirect>);
    eu.set_instruction(0xc160, 0x0e07,
		       &m68k_and_m<word_size, word_predec_indirect>);
    eu.set_instruction(0xc168, 0x0e07,
		       &m68k_and_m<word_size, word_disp_indirect>);
    eu.set_instruction(0xc170, 0x0e07,
		       &m68k_and_m<word_size, word_index_indirect>);
    eu.set_instruction(0xc178, 0x0e00,
		       &m68k_and_m<word_size, word_abs_short>);
    eu.set_instruction(0xc179, 0x0e00,
		       &m68k_and_m<word_size, word_abs_long>);
    eu.set_instruction(0xc188, 0x0e07, &exgl<data_register, address_register>);
    eu.set_instruction(0xc190, 0x0e07,
		       &m68k_and_m<long_word_size, long_word_indirect>);
    eu.set_instruction(0xc198, 0x0e07,
		       &m68k_and_m<long_word_size, long_word_postinc_indirect>);
    eu.set_instruction(0xc1a0, 0x0e07,
		       &m68k_and_m<long_word_size, long_word_predec_indirect>);
    eu.set_instruction(0xc1a8, 0x0e07,
		       &m68k_and_m<long_word_size, long_word_disp_indirect>);
    eu.set_instruction(0xc1b0, 0x0e07,
		       &m68k_and_m<long_word_size, long_word_index_indirect>);
    eu.set_instruction(0xc1b8, 0x0e00,
		       &m68k_and_m<long_word_size, long_word_abs_short>);
    eu.set_instruction(0xc1b9, 0x0e00,
		       &m68k_and_m<long_word_size, long_word_abs_long>);
    eu.set_instruction(0xc1c0, 0x0e07, &mulsw<data_register>);
    eu.set_instruction(0xc1d0, 0x0e07, &mulsw<indirect>);
    eu.set_instruction(0xc1d8, 0x0e07, &mulsw<postinc_indirect>);
    eu.set_instruction(0xc1e0, 0x0e07, &mulsw<predec_indirect>);
    eu.set_instruction(0xc1e8, 0x0e07, &mulsw<disp_indirect>);
    eu.set_instruction(0xc1f0, 0x0e07, &mulsw<indexed_indirect>);
    eu.set_instruction(0xc1f8, 0x0e00, &mulsw<absolute_short>);
    eu.set_instruction(0xc1f9, 0x0e00, &mulsw<absolute_long>);
    eu.set_instruction(0xc1fc, 0x0e00, &mulsw<immediate>);
    eu.set_instruction(0xd000, 0x0e07, &m68k_add<byte_size, byte_d_register>);
    eu.set_instruction(0xd010, 0x0e07, &m68k_add<byte_size, byte_indirect>);
    eu.set_instruction(0xd018, 0x0e07,
		       &m68k_add<byte_size, byte_postinc_indirect>);
    eu.set_instruction(0xd020, 0x0e07,
		       &m68k_add<byte_size, byte_predec_indirect>);
    eu.set_instruction(0xd028, 0x0e07,
		       &m68k_add<byte_size, byte_disp_indirect>);
    eu.set_instruction(0xd030, 0x0e07,
		       &m68k_add<byte_size, byte_index_indirect>);
    eu.set_instruction(0xd038, 0x0e00, &m68k_add<byte_size, byte_abs_short>);
    eu.set_instruction(0xd039, 0x0e00, &m68k_add<byte_size, byte_abs_long>);
    eu.set_instruction(0xd03a, 0x0e00,
		       &m68k_add<byte_size, byte_disp_pc_indirect>);
    eu.set_instruction(0xd03b, 0x0e00,
		       &m68k_add<byte_size, byte_index_pc_indirect>);
    eu.set_instruction(0xd03c, 0x0e00, &m68k_add<byte_size, byte_immediate>);
    eu.set_instruction(0xd040, 0x0e07, &m68k_add<word_size, word_d_register>);
    eu.set_instruction(0xd048, 0x0e07, &m68k_add<word_size, word_a_register>);
    eu.set_instruction(0xd050, 0x0e07, &m68k_add<word_size, word_indirect>);
    eu.set_instruction(0xd058, 0x0e07,
		       &m68k_add<word_size, word_postinc_indirect>);
    eu.set_instruction(0xd060, 0x0e07,
		       &m68k_add<word_size, word_predec_indirect>);
    eu.set_instruction(0xd068, 0x0e07,
		       &m68k_add<word_size, word_disp_indirect>);
    eu.set_instruction(0xd070, 0x0e07,
		       &m68k_add<word_size, word_index_indirect>);
    eu.set_instruction(0xd078, 0x0e00, &m68k_add<word_size, word_abs_short>);
    eu.set_instruction(0xd079, 0x0e00, &m68k_add<word_size, word_abs_long>);
    eu.set_instruction(0xd07a, 0x0e00,
		       &m68k_add<word_size, word_disp_pc_indirect>);
    eu.set_instruction(0xd07b, 0x0e00,
		       &m68k_add<word_size, word_index_pc_indirect>);
    eu.set_instruction(0xd07c, 0x0e00, &m68k_add<word_size, word_immediate>);
    eu.set_instruction(0xd080, 0x0e07,
		       &m68k_add<long_word_size, long_word_d_register>);
    eu.set_instruction(0xd088, 0x0e07,
		       &m68k_add<long_word_size, long_word_a_register>);
    eu.set_instruction(0xd090, 0x0e07,
		       &m68k_add<long_word_size, long_word_indirect>);
    eu.set_instruction(0xd098, 0x0e07,
		       &m68k_add<long_word_size, long_word_postinc_indirect>);
    eu.set_instruction(0xd0a0, 0x0e07,
		       &m68k_add<long_word_size, long_word_predec_indirect>);
    eu.set_instruction(0xd0a8, 0x0e07,
		       &m68k_add<long_word_size, long_word_disp_indirect>);
    eu.set_instruction(0xd0b0, 0x0e07,
		       &m68k_add<long_word_size, long_word_index_indirect>);
    eu.set_instruction(0xd0b8, 0x0e00,
		       &m68k_add<long_word_size, long_word_abs_short>);
    eu.set_instruction(0xd0b9, 0x0e00,
		       &m68k_add<long_word_size, long_word_abs_long>);
    eu.set_instruction(0xd0ba, 0x0e00,
		       &m68k_add<long_word_size, long_word_disp_pc_indirect>);
    eu.set_instruction(0xd0bb, 0x0e00,
		       &m68k_add<long_word_size, long_word_index_pc_indirect>);
    eu.set_instruction(0xd0bc, 0x0e00,
		       &m68k_add<long_word_size, long_word_immediate>);
    eu.set_instruction(0xd0c0, 0x0e07, &m68k_adda<word_size, word_d_register>);
    eu.set_instruction(0xd0c8, 0x0e07, &m68k_adda<word_size, word_a_register>);
    eu.set_instruction(0xd0d0, 0x0e07, &m68k_adda<word_size, word_indirect>);
    eu.set_instruction(0xd0d8, 0x0e07,
		       &m68k_adda<word_size, word_postinc_indirect>);
    eu.set_instruction(0xd0e0, 0x0e07,
		       &m68k_adda<word_size, word_predec_indirect>);
    eu.set_instruction(0xd0e8, 0x0e07,
		       &m68k_adda<word_size, word_disp_indirect>);
    eu.set_instruction(0xd0f0, 0x0e07,
		       &m68k_adda<word_size, word_index_indirect>);
    eu.set_instruction(0xd0f8, 0x0e00, &m68k_adda<word_size, word_abs_short>);
    eu.set_instruction(0xd0f9, 0x0e00, &m68k_adda<word_size, word_abs_long>);
    eu.set_instruction(0xd0fa, 0x0e00,
		       &m68k_adda<word_size, word_disp_pc_indirect>);
    eu.set_instruction(0xd0fb, 0x0e00,
		       &m68k_adda<word_size, word_index_pc_indirect>);
    eu.set_instruction(0xd0fc, 0x0e00, &m68k_adda<word_size, word_immediate>);
    eu.set_instruction(0xd100, 0x0e07, &m68k_addx_r<byte_size>);
    eu.set_instruction(0xd110, 0x0e07,
		       &m68k_add_m<byte_size, byte_indirect>);
    eu.set_instruction(0xd118, 0x0e07,
		       &m68k_add_m<byte_size, byte_postinc_indirect>);
    eu.set_instruction(0xd120, 0x0e07,
		       &m68k_add_m<byte_size, byte_predec_indirect>);
    eu.set_instruction(0xd128, 0x0e07,
		       &m68k_add_m<byte_size, byte_disp_indirect>);
    eu.set_instruction(0xd130, 0x0e07,
		       &m68k_add_m<byte_size, byte_index_indirect>);
    eu.set_instruction(0xd138, 0x0e00, &m68k_add_m<byte_size, byte_abs_short>);
    eu.set_instruction(0xd139, 0x0e00, &m68k_add_m<byte_size, byte_abs_long>);
    eu.set_instruction(0xd140, 0x0e07, &m68k_addx_r<word_size>);
    eu.set_instruction(0xd150, 0x0e07,
		       &m68k_add_m<word_size, word_indirect>);
    eu.set_instruction(0xd158, 0x0e07,
		       &m68k_add_m<word_size, word_postinc_indirect>);
    eu.set_instruction(0xd160, 0x0e07,
		       &m68k_add_m<word_size, word_predec_indirect>);
    eu.set_instruction(0xd168, 0x0e07,
		       &m68k_add_m<word_size, word_disp_indirect>);
    eu.set_instruction(0xd170, 0x0e07,
		       &m68k_add_m<word_size, word_index_indirect>);
    eu.set_instruction(0xd178, 0x0e00, &m68k_add_m<word_size, word_abs_short>);
    eu.set_instruction(0xd179, 0x0e00, &m68k_add_m<word_size, word_abs_long>);
    eu.set_instruction(0xd180, 0x0e07, &m68k_addx_r<long_word_size>);
    eu.set_instruction(0xd190, 0x0e07,
		       &m68k_add_m<long_word_size, long_word_indirect>);
    eu.set_instruction(0xd198, 0x0e07,
		       &m68k_add_m<long_word_size,
		                   long_word_postinc_indirect>);
    eu.set_instruction(0xd1a0, 0x0e07,
		       &m68k_add_m<long_word_size, long_word_predec_indirect>);
    eu.set_instruction(0xd1a8, 0x0e07,
		       &m68k_add_m<long_word_size, long_word_disp_indirect>);
    eu.set_instruction(0xd1b0, 0x0e07,
		       &m68k_add_m<long_word_size, long_word_index_indirect>);
    eu.set_instruction(0xd1b8, 0x0e00,
		       &m68k_add_m<long_word_size, long_word_abs_short>);
    eu.set_instruction(0xd1b9, 0x0e00,
		       &m68k_add_m<long_word_size, long_word_abs_long>);
    eu.set_instruction(0xd1c0, 0x0e07,
		       &m68k_adda<long_word_size, long_word_d_register>);
    eu.set_instruction(0xd1c8, 0x0e07,
		       &m68k_adda<long_word_size, long_word_a_register>);
    eu.set_instruction(0xd1d0, 0x0e07,
		       &m68k_adda<long_word_size, long_word_indirect>);
    eu.set_instruction(0xd1d8, 0x0e07,
		       &m68k_adda<long_word_size, long_word_postinc_indirect>);
    eu.set_instruction(0xd1e0, 0x0e07,
		       &m68k_adda<long_word_size, long_word_predec_indirect>);
    eu.set_instruction(0xd1e8, 0x0e07,
		       &m68k_adda<long_word_size, long_word_disp_indirect>);
    eu.set_instruction(0xd1f0, 0x0e07,
		       &m68k_adda<long_word_size, long_word_index_indirect>);
    eu.set_instruction(0xd1f8, 0x0e00,
		       &m68k_adda<long_word_size, long_word_abs_short>);
    eu.set_instruction(0xd1f9, 0x0e00,
		       &m68k_adda<long_word_size, long_word_abs_long>);
    eu.set_instruction(0xd1fa, 0x0e00,
		       &m68k_adda<long_word_size, long_word_disp_pc_indirect>);
    eu.set_instruction(0xd1fb, 0x0e00,
		       &m68k_adda<long_word_size,
		                  long_word_index_pc_indirect>);
    eu.set_instruction(0xd1fc, 0x0e00,
		       &m68k_adda<long_word_size, long_word_immediate>);
    eu.set_instruction(0xe000, 0x0e07,
		       &m68k_asr_i<byte_size>);
    eu.set_instruction(0xe008, 0x0e07, &lsrb_i);
    eu.set_instruction(0xe018, 0x0e07, &m68k_ror_i<byte_size>);
    eu.set_instruction(0xe040, 0x0e07,
		       &m68k_asr_i<word_size>);
    eu.set_instruction(0xe048, 0x0e07, &lsrw_i);
    eu.set_instruction(0xe050, 0x0e07, &roxrw_i);
    eu.set_instruction(0xe058, 0x0e07, &m68k_ror_i<word_size>);
    eu.set_instruction(0xe068, 0x0e07, &lsrw_r);
    eu.set_instruction(0xe080, 0x0e07,
		       &m68k_asr_i<long_word_size>);
    eu.set_instruction(0xe088, 0x0e07, &lsrl_i);
    eu.set_instruction(0xe090, 0x0e07, &roxrl_i);
    eu.set_instruction(0xe098, 0x0e07, &m68k_ror_i<long_word_size>);
    eu.set_instruction(0xe0a0, 0x0e07, &asrl_r);
    eu.set_instruction(0xe0a8, 0x0e07, &lsrl_r);
    eu.set_instruction(0xe100, 0x0e07, &m68k_asl_i<byte_size>);
    eu.set_instruction(0xe108, 0x0e07, &lslb_i);
    eu.set_instruction(0xe120, 0x0e07, &m68k_asl_r<byte_size>);
    eu.set_instruction(0xe138, 0x0e07, &m68k_rol_r<byte_size>);
    eu.set_instruction(0xe140, 0x0e07, &m68k_asl_i<word_size>);
    eu.set_instruction(0xe148, 0x0e07, &lslw_i);
    eu.set_instruction(0xe158, 0x0e07, &rolw_i);
    eu.set_instruction(0xe160, 0x0e07, &m68k_asl_r<word_size>);
    eu.set_instruction(0xe168, 0x0e07, &lslw_r);
    eu.set_instruction(0xe178, 0x0e07, &m68k_rol_r<word_size>);
    eu.set_instruction(0xe180, 0x0e07, &m68k_asl_i<long_word_size>);
    eu.set_instruction(0xe188, 0x0e07, &lsll_i);
    eu.set_instruction(0xe198, 0x0e07, &roll_i);
    eu.set_instruction(0xe1a0, 0x0e07, &m68k_asl_r<long_word_size>);
    eu.set_instruction(0xe1a8, 0x0e07, &lsll_r);
    eu.set_instruction(0xe1b8, 0x0e07, &m68k_rol_r<long_word_size>);
  }
} // (unnamed namespace)

exec_unit::exec_unit()
  : instructions(0x10000, instruction_type(&illegal, 0))
{
  initialize_instructions(*this);
}

/* Executes an illegal instruction.  */
void
exec_unit::illegal(uint_type op, context &c, unsigned long data)
{
  throw illegal_instruction_exception();
}
