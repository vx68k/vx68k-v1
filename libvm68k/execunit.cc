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
#endif

using vm68k::exec_unit;
using vm68k::context;
using vm68k::byte_size;
using vm68k::word_size;
using vm68k::long_word_size;
using vm68k::privilege_violation_exception;
using namespace vm68k::types;
using namespace std;

#ifdef HAVE_NANA_H
bool nana_instruction_trace = false;
#endif

void
exec_unit::run(context &c) const
{
  for (;;)
    {
      if (c.interrupted())
	c.handle_interrupts();

#ifdef HAVE_NANA_H
# ifdef DUMP_REGISTERS
      for (unsigned int i = 0; i != 8; ++i)
	{
	  LG(nana_instruction_trace,
	     "| %%d%u = 0x%08lx, %%a%u = 0x%08lx\n",
	     i, (unsigned long) c.regs.d[i],
	     i, (unsigned long) c.regs.a[i]);
	}
# endif
      LG(nana_instruction_trace, "| 0x%08lx (0x%04x)\n",
	 long_word_size::uvalue(c.regs.pc) + 0UL,
	 word_size::uvalue(c.fetch(word_size(), 0)));
#endif
      step(c);
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
  using vm68k::memory;
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

#ifdef HAVE_NANA_H
# undef L_DEFAULT_GUARD
# define L_DEFAULT_GUARD nana_instruction_trace
#endif

  /* Handles an ADD instruction.  */
  template <class Size, class Source> void
  m68k_add(uint_type op, context &c, unsigned long data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\tadd%s %s,%%d%u\n", Size::suffix(), ea1.text(c).c_str(), reg2);
#endif

    typename Size::svalue_type value1 = ea1.get(c);
    typename Size::svalue_type value2 = Size::get(c.regs.d[reg2]);
    typename Size::svalue_type value = Size::svalue(value2 + value1);
    Size::put(c.regs.d[reg2], value);
    c.regs.ccr.set_cc_as_add(value, value2, value1);

    ea1.finish(c);
    c.regs.pc += 2 + Source::extension_size();
  }

  /* Handles an ADD instruction (memory destination).  */
  template <class Size, class Destination> void
  m68k_add_m(uint_type op, context &c, unsigned long data)
  {
    Destination ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\tadd%s %%d%u,%s\n", Size::suffix(), reg2, ea1.text(c).c_str());
#endif

    typename Size::svalue_type value2 = Size::get(c.regs.d[reg2]);
    typename Size::svalue_type value1 = ea1.get(c);
    typename Size::svalue_type value = Size::svalue(value1 + value2);
    ea1.put(c, value);
    c.regs.ccr.set_cc_as_add(value, value1, value2);

    ea1.finish(c);
    c.regs.pc += 2 + Destination::extension_size();
  }

  /* Handles an ADDA instruction.  */
  template <class Size, class Source> void
  m68k_adda(uint_type op, context &c, unsigned long data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\tadda%s %s,%%a%u\n", Size::suffix(), ea1.text(c).c_str(), reg2);
#endif

    // The condition codes are not affected by this instruction.
    long_word_size::svalue_type value1 = ea1.get(c);
    long_word_size::svalue_type value2 = long_word_size::get(c.regs.a[reg2]);
    long_word_size::svalue_type value
      = long_word_size::svalue(value2 + value1);
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
#ifdef HAVE_NANA_H
    L("\taddi%s #%#lx,%s\n", Size::suffix(), Size::uvalue(value2) + 0UL,
      ea1.text(c).c_str());
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
#ifdef HAVE_NANA_H
    L("\taddq%s #%d,%s\n", Size::suffix(), value2, ea1.text(c).c_str());
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
#ifdef HAVE_NANA_H
    L("\taddq%s #%d,%%a%u\n", Size::suffix(), value2, reg1);
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
#ifdef HAVE_NANA_H
    L("\taddx%s %%d%u,%%d%u\n", Size::suffix(), reg1, reg2);
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
#ifdef HAVE_NANA_H
    L("\tand%s %s,%%d%u\n", Size::suffix(), ea1.text(c).c_str(), reg2);
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
#ifdef HAVE_NANA_H
    L("\tand%s %%d%u,%s\n", Size::suffix(), reg2, ea1.text(c).c_str());
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
#ifdef HAVE_NANA_H
    L("\tandi%s #%#lx,%s\n", Size::suffix(), Size::uvalue(value2) + 0UL,
      ea1.text(c).c_str());
#endif

    svalue_type value1 = ea1.get(c);
    svalue_type value = Size::svalue(Size::get(value1) & Size::get(value2));
    ea1.put(c, value);
    c.regs.ccr.set_cc(value);
    ea1.finish(c);

    c.regs.pc += 2 + Size::aligned_value_size() + ea1.extension_size();
  }

  /* Handles an ANDI-to-CCR instruction.  */
  void
  m68k_andi_to_ccr(uint_type op, context &c, unsigned long data)
  {
    typedef byte_size::uvalue_type uvalue_type;
    typedef byte_size::svalue_type svalue_type;

    uvalue_type value2 = c.fetch(byte_size(), 2);
#ifdef HAVE_NANA_H
    L("\tandi%s #%#x,%%ccr\n", byte_size::suffix(), value2);
#endif

    uvalue_type value1 = c.regs.ccr & 0xffu;
    uvalue_type value = value1 & value2;
    c.regs.ccr = c.regs.ccr & ~0xffu | value & 0xffu;

    c.regs.pc += 2 + byte_size::aligned_value_size();
  }

  /* Handles an ANDI-to-SR instruction.  */
  void
  m68k_andi_to_sr(uint_type op, context &c, unsigned long data)
  {
    const word_size::uvalue_type value2 = c.fetch(word_size(), 2);
#ifdef HAVE_NANA_H
    L("\tandi%s #%#x,%%sr\n", word_size::suffix(), value2);
#endif

    // This instruction is privileged.
    if (!c.supervisor_state())
      throw privilege_violation_exception();

    const word_size::uvalue_type value1 = c.sr();
    const word_size::uvalue_type value = value1 & value2;
    c.set_sr(value);

    c.regs.advance_pc(2 + word_size::aligned_value_size());
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
#ifdef HAVE_NANA_H
    L("\tasl%s #%u,%%d%u\n", Size::suffix(), value2, reg1);
#endif

    svalue_type value1 = Size::svalue(Size::get(c.regs.d[reg1]));
    svalue_type value = Size::svalue(Size::get(value1 << value2));
    Size::put(c.regs.d[reg1], value);
    c.regs.ccr.set_cc_lsl(value, value1, value2 + (32 - Size::value_bit())); // FIXME?

    c.regs.pc += 2;
  }

  /* Handles an ASL instruction with a register count.  */
  template <class Size> void
  m68k_asl_r(uint_type op, context &c, unsigned long data)
  {
    typedef typename Size::uvalue_type uvalue_type;
    typedef typename Size::svalue_type svalue_type;

    unsigned int reg1 = op & 0x7;
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\tasl%s ", Size::suffix());
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
#ifdef HAVE_NANA_H
    L("\tasr%s ", Size::suffix());
    L("#%u,", value2);
    L("%%d%u\n", reg1);
#endif

    svalue_type value1 = Size::svalue(Size::get(c.regs.d[reg1]));
    svalue_type value = Size::svalue(Size::get(value1 >> value2));
    Size::put(c.regs.d[reg1], value);
    c.regs.ccr.set_cc_asr(value, value1, value2);

    c.regs.pc += 2;
  }

  /* Handles an ASR instruction (register).  */
  template <class Size> void
  m68k_asr_r(uint_type op, context &c, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\tasr%s %%d%u,%%d%u\n", Size::suffix(), reg2, reg1);
#endif

    typename Size::uvalue_type value2 = c.regs.d[reg2] % Size::value_bit();
    typename Size::svalue_type value1 = Size::get(c.regs.d[reg1]);
    typename Size::svalue_type value = Size::svalue(value1 >> value2);
    Size::put(c.regs.d[reg1], value);
    c.regs.ccr.set_cc_asr(value, value1, value2);

    c.regs.pc += 2;
  }

#if 0
  void
  asrl_r(uint_type op, context &ec, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
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
#endif

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
#ifdef HAVE_NANA_H
    L("\tb%s %#lx\n", Condition::text(),
      long_word_size::uvalue(c.regs.pc + 2 + disp) + 0UL);
#endif

    // This instruction does not affect the condition codes.
    Condition cond;
    c.regs.pc += 2 + (cond(c) ? disp : extsize);
  }

  /* Handles a BCLR instruction (register).  */
  template <class Size, class Destination> void
  m68k_bclr_r(uint_type op, context &c, unsigned long data)
  {
    typedef typename Size::uvalue_type uvalue_type;
    typedef typename Size::svalue_type svalue_type;

    Destination ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\tbclr%s %%d%u,%s\n", Size::suffix(), reg2, ea1.text(c).c_str());
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
#ifdef HAVE_NANA_H
    L("\tbclr%s #%u,%s\n", Size::suffix(), value2, ea1.text(c).c_str());
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

  void
  m68k_bra(uint_type op, context &c, unsigned long data)
  {
    word_size::svalue_type disp = op & 0xff;
    size_t len = 0;
    if (disp == 0)
      {
	disp = c.fetch(word_size(), 2);
	len = word_size::aligned_value_size();
      }
    else
      disp = byte_size::svalue(disp);
#ifdef HAVE_NANA_H
    L("\tbra %#lx\n", long_word_size::uvalue(c.regs.pc + 2 + disp) + 0UL);
#endif

    // XXX: The condition codes are not affected.
    c.regs.pc += 2 + disp;
  }

  /* Handles a BSET instruction (register).  */
  template <class Size, class Destination> void
  m68k_bset_r(uint_type op, context &c, unsigned long data)
  {
    typedef typename Size::uvalue_type uvalue_type;
    typedef typename Size::svalue_type svalue_type;

    Destination ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\tbset%s %%d%u,%s\n", Size::suffix(), reg2, ea1.text(c).c_str());
#endif

    // This instruction affects only the Z bit of the condition codes.
    uvalue_type mask = uvalue_type(1) << c.regs.d[reg2] % Size::value_bit();
    uvalue_type value1 = ea1.get(c);
    bool value = value1 & mask;
    ea1.put(c, value1 | mask);
    c.regs.ccr.set_cc(value);	// FIXME.

    c.regs.pc += 2 + ea1.extension_size();
  }

  /* Handles a BSET instruction (immediate).  */
  template <class Size, class Destination> void
  m68k_bset_i(uint_type op, context &c, unsigned long data)
  {
    typedef typename Size::uvalue_type uvalue_type;
    typedef typename Size::svalue_type svalue_type;

    unsigned int value2 = c.fetch(word_size(), 2) % Size::value_bit();
    Destination ea1(op & 0x7, 2 + word_size::aligned_value_size());
#ifdef HAVE_NANA_H
    L("\tbset%s #%u,%s\n", Size::suffix(), value2, ea1.text(c).c_str());
#endif

    // This instruction affects only the Z bit of the condition codes.
    uvalue_type mask = uvalue_type(1) << value2;
    uvalue_type value1 = ea1.get(c);
    bool value = value1 & mask;
    ea1.put(c, value1 | mask);
    c.regs.ccr.set_cc(value);	// FIXME.

    ea1.finish(c);
    c.regs.pc += (2 + word_size::aligned_value_size()
		  + Destination::extension_size());
  }

  void
  m68k_bsr(uint_type op, context &c, unsigned long data)
  {
    word_size::svalue_type disp = op & 0xff;
    size_t len = 0;
    if (disp == 0)
      {
	disp = c.fetch(word_size(), 2);
	len = word_size::aligned_value_size();
      }
    else
      disp = byte_size::svalue(disp);
#ifdef HAVE_NANA_H
    L("\tbsr %#lx\n", long_word_size::uvalue(c.regs.pc + 2 + disp) + 0UL);
#endif

    // XXX: The condition codes are not affected.
    memory::function_code fc = c.data_fc();
    c.mem->put_32(fc, c.regs.a[7] - long_word_size::aligned_value_size(),
		  c.regs.pc + 2 + len);
    c.regs.a[7] -= long_word_size::aligned_value_size();

    c.regs.pc += 2 + disp;
  }

  /* Handles a BTST instruction (register).  */
  template <class Size, class Destination> void
  m68k_btst_r(uint_type op, context &c, unsigned long data)
  {
    typedef typename Size::uvalue_type uvalue_type;
    typedef typename Size::svalue_type svalue_type;

    Destination ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\tbtst%s %%d%u,%s\n", Size::suffix(), reg2, ea1.text(c).c_str());
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

  /* Handles a BTST instruction (immediate).  */
  template <class Size, class Destination> void
  m68k_btst_i(uint_type op, context &c, unsigned long data)
  {
    unsigned int value2 = c.fetch(word_size(), 2) % Size::value_bit();
    Destination ea1(op & 0x7, 2 + word_size::aligned_value_size());
#ifdef HAVE_NANA_H
    L("\tbtst%s #%u,%s\n", Size::suffix(), value2, ea1.text(c).c_str());
#endif

    // This instruction affects only the Z bit of the condition codes.
    typename Size::uvalue_type mask = typename Size::uvalue_type(1) << value2;
    typename Size::svalue_type value1 = ea1.get(c);
    bool value = Size::uvalue(value1) & mask;
    c.regs.ccr.set_cc(value);	// FIXME.

    ea1.finish(c);
    long_word_size::put(c.regs.pc,
			c.regs.pc + 2 + word_size::aligned_value_size()
			+ Destination::extension_size());
  }

#if 0
  template <class Destination> void
  btstb_i(uint_type op, context &c, unsigned long data)
  {
    unsigned int bit = c.fetch(word_size(), 2) & 0x7;
    Destination ea1(op & 0x7, 2 + 2);
#ifdef HAVE_NANA_H
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
#ifdef HAVE_NANA_H
    L(" btstl #%u", bit);
    L(",%%d%u\n", reg1);
#endif

    // This instruction affects only the Z bit of condition codes.
    bool value = ec.regs.d[reg1] & uint32_type(1) << bit;
    ec.regs.ccr.set_cc(value);	// FIXME.

    ec.regs.pc += 2 + 2;
  }
#endif

  /* Handles a CLR instruction.  */
  template <class Size, class Destination> void
  m68k_clr(uint_type op, context &c, unsigned long data)
  {
    Destination ea1(op & 0x7, 2);
#ifdef HAVE_NANA_H
    L("\tclr%s %s\n", Size::suffix(), ea1.text(c).c_str());
#endif

    ea1.put(c, 0);
    c.regs.ccr.set_cc(0);

    ea1.finish(c);
    long_word_size::put(c.regs.pc,
			c.regs.pc + 2 + Destination::extension_size());
  }

#if 0
  template <class Destination> void
  clrb(uint_type op, context &ec, unsigned long data)
    {
      Destination ea1(op & 0x7, 2);
#ifdef HAVE_NANA_H
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
#ifdef HAVE_NANA_H
      VL((" clrw %s\n", ea1.textw(ec)));
#endif

      ea1.putw(ec, 0);
      ea1.finishw(ec);
      ec.regs.ccr.set_cc(0);

      ec.regs.pc += 2 + ea1.isize(2);
    }

  template <> void clrw<address_register>(unsigned int, context &);
  // XXX: Address register cannot be the destination.

  template <class Destination> void
  clrl(uint_type op, context &ec, unsigned long data)
    {
      Destination ea1(op & 0x7, 2);
#ifdef HAVE_NANA_H
      VL((" clrl %s\n", ea1.textl(ec)));
#endif

      ea1.putl(ec, 0);
      ea1.finishl(ec);
      ec.regs.ccr.set_cc(0);

      ec.regs.pc += 2 + ea1.isize(2);
    }
#endif

  /* Handles a CMP instruction.  */
  template <class Size, class Source> void
  m68k_cmp(uint_type op, context &c, unsigned long data)
  {
    typedef typename Size::uvalue_type uvalue_type;
    typedef typename Size::svalue_type svalue_type;

    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\tcmp%s %s,%%d%u\n", Size::suffix(), ea1.text(c).c_str(), reg2);
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
#ifdef HAVE_NANA_H
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
#ifdef HAVE_NANA_H
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
#ifdef HAVE_NANA_H
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

  /* Handles a CMPA instruction.  */
  template <class Size, class Source> void
  m68k_cmpa(uint_type op, context &c, unsigned long data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\tcmpa%s %s,%%a%u\n", Size::suffix(), ea1.text(c).c_str(), reg2);
#endif

    long_word_size::svalue_type value1 = ea1.get(c);
    long_word_size::svalue_type value2 = long_word_size::get(c.regs.a[reg2]);
    long_word_size::svalue_type value
      = long_word_size::svalue(value2 - value1);
    c.regs.ccr.set_cc_cmp(value, value2, value1);

    ea1.finish(c);
    long_word_size::put(c.regs.pc, c.regs.pc + 2 + Source::extension_size());
  }

#if 0
  template <class Source> void
  cmpaw(uint_type op, context &ec, unsigned long data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
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
#ifdef HAVE_NANA_H
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
#endif

  template <class Size, class Destination> void
  m68k_cmpi(uint_type op, context &c, unsigned long data)
  {
    typename Size::svalue_type value2 = c.fetch(Size(), 2);
    Destination ea1(op & 0x7, 2 + Size::aligned_value_size());
#ifdef HAVE_NANA_H
    L("\tcmpi%s #%#lx,%s\n", Size::suffix(), Size::uvalue(value2) + 0UL,
      ea1.text(c).c_str());
#endif

    typename Size::svalue_type value1 = ea1.get(c);
    typename Size::svalue_type value = Size::svalue(value1 - value2);
    c.regs.ccr.set_cc_cmp(value, value1, value2);

    ea1.finish(c);
    c.regs.advance_pc(2 + Size::aligned_value_size()
		      + Destination::extension_size());
  }

#if 0
  template <class Destination> void
  cmpib(uint_type op, context &ec, unsigned long data)
  {
    sint_type value2 = extsb(ec.fetch(word_size(), 2));
    Destination ea1(op & 0x7, 2 + 2);
#ifdef HAVE_NANA_H
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
#ifdef HAVE_NANA_H
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
#ifdef HAVE_NANA_H
    L(" cmpil #%#lx", (unsigned long) value2);
    L(",%s\n", ea1.textl(c));
#endif

    sint32_type value1 = ea1.getl(c);
    sint32_type value = extsl(value1 - value2);
    c.regs.ccr.set_cc_cmp(value, value1, value2);
    ea1.finishl(c);

    c.regs.pc += 2 + 4 + ea1.isize(4);
  }
#endif

  /* Handles a CMPM instruction.  */
  template <class Size> void
  m68k_cmpm(uint_type op, context &c, unsigned long data)
  {
    typedef typename Size::uvalue_type uvalue_type;
    typedef typename Size::svalue_type svalue_type;

    basic_postinc_indirect<Size> ea1(op & 0x7, 2);
    basic_postinc_indirect<Size> ea2(op >> 9 & 0x7, 2 + ea1.extension_size());
#ifdef HAVE_NANA_H
    L("\tcmpm%s %s,%s\n", Size::suffix(), ea1.text(c).c_str(),
      ea2.text(c).c_str());
#endif

    svalue_type value1 = ea1.get(c);
    svalue_type value2 = ea2.get(c);
    svalue_type value = Size::svalue(Size::get(value2 - value1));
    c.regs.ccr.set_cc_cmp(value, value2, value1);

    ea1.finish(c);
    ea2.finish(c);
    c.regs.pc += 2 + ea1.extension_size() + ea2.extension_size();
  }

  /* Handles a DIVU instruction.  */
  template <class Source> void
  m68k_divu(uint_type op, context &c, unsigned long data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\tdivu%s %s,%%d%u\n", word_size::suffix(), ea1.text(c).c_str(), reg2);
#endif

    word_size::svalue_type value1 = ea1.get(c);
    long_word_size::svalue_type value2 = long_word_size::get(c.regs.d[reg2]);
    long_word_size::svalue_type value
      = long_word_size::uvalue(value2) / word_size::uvalue(value1);
    long_word_size::svalue_type rem
      = long_word_size::uvalue(value2) % word_size::uvalue(value1);
    long_word_size::put(c.regs.d[reg2],
			long_word_size::uvalue(rem) << 16
			| long_word_size::uvalue(value) & 0xffff);
    c.regs.ccr.set_cc(value);	// FIXME.

    ea1.finish(c);
    c.regs.pc += 2 + Source::extension_size();
  }

  /* Handles an EOR instruction.  */
  template <class Size, class Destination> void
  m68k_eor_m(uint_type op, context &c, unsigned long data)
  {
    Destination ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\teor%s %%d%u,%s\n", Size::suffix(), reg2, ea1.text(c).c_str());
#endif

    typename Size::svalue_type value1 = ea1.get(c);
    typename Size::svalue_type value2 = Size::get(c.regs.d[reg2]);
    typename Size::svalue_type value
      = Size::svalue(Size::uvalue(value1) ^ Size::uvalue(value2));
    ea1.put(c, value);
    c.regs.ccr.set_cc(value);

    ea1.finish(c);
    long_word_size::put(c.regs.pc,
			c.regs.pc + 2 + Destination::extension_size());
  }

#if 0
  template <class Destination> void
  eorb_r(uint_type op, context &ec, unsigned long data)
  {
    Destination ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
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
#ifdef HAVE_NANA_H
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
#ifdef HAVE_NANA_H
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
#endif

  /* Handles an EORI instruction.  */
  template <class Size, class Destination> void
  m68k_eori(uint_type op, context &c, unsigned long data)
  {
    typedef typename Size::uvalue_type uvalue_type;
    typedef typename Size::svalue_type svalue_type;

    svalue_type value2 = Size::svalue(c.fetch(Size(), 2));
    Destination ea1(op & 0x7, 2 + Size::aligned_value_size());
#ifdef HAVE_NANA_H
    L("\teori%s #%#lx,%s\n", Size::suffix(), Size::uvalue(value2) + 0UL,
      ea1.text(c).c_str());
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
#ifdef HAVE_NANA_H
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
#ifdef HAVE_NANA_H
    L("\tdb%s %%d%u,%#lx\n", Condition::text(), reg1,
      long_word_size::uvalue(c.regs.pc + 2 + disp) + 0UL);
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
#ifdef HAVE_NANA_H
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

  /* Handles an EXG instruction (data registers).  */
  void
  m68k_exg_d_d(uint_type op, context &c, unsigned long data)
  {
    const unsigned int reg1 = op & 0x7;
    const unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\texg%s %%d%u,%%d%u\n", long_word_size::suffix(), reg2, reg1);
#endif

    // The condition codes are not affected by this instruction.
    const long_word_size::svalue_type value
      = long_word_size::get(c.regs.d[reg1]);
    long_word_size::put(c.regs.d[reg1], long_word_size::get(c.regs.d[reg2]));
    long_word_size::put(c.regs.d[reg2], value);

    c.regs.advance_pc(2);
  }

  /* Handles an EXG instruction (address registers).  */
  void
  m68k_exg_a_a(uint_type op, context &c, unsigned long data)
  {
    const unsigned int reg1 = op & 0x7;
    const unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\texg%s %%a%u,%%a%u\n", long_word_size::suffix(), reg2, reg1);
#endif

    // The condition codes are not affected by this instruction.
    const long_word_size::svalue_type value
      = long_word_size::get(c.regs.a[reg1]);
    long_word_size::put(c.regs.a[reg1], long_word_size::get(c.regs.a[reg2]));
    long_word_size::put(c.regs.a[reg2], value);

    c.regs.advance_pc(2);
  }

  /* Handles an EXG instruction (data register and address register).  */
  void
  m68k_exg_d_a(uint_type op, context &c, unsigned long data)
  {
    const unsigned int reg1 = op & 0x7;
    const unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\texg%s %%d%u,%%a%u\n", long_word_size::suffix(), reg2, reg1);
#endif

    // The condition codes are not affected by this instruction.
    const long_word_size::svalue_type value
      = long_word_size::get(c.regs.a[reg1]);
    long_word_size::put(c.regs.a[reg1], long_word_size::get(c.regs.d[reg2]));
    long_word_size::put(c.regs.d[reg2], value);

    c.regs.advance_pc(2);
  }

#if 0
  template <class Register2, class Register1> void
  exgl(uint_type op, context &c, unsigned long data)
  {
    Register1 ea1(op & 0x7, 2);
    Register2 ea2(op >> 9 & 0x7, 2 + ea1.isize(4));
#ifdef HAVE_NANA_H
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
#endif

  /* Handles an EXT instruction.  */
  template <class Size1, class Size2> void
  m68k_ext(uint_type op, context &c, unsigned long data)
  {
    const unsigned int reg1 = op & 0x7;
#ifdef HAVE_NANA_H
    L("\text%s %%d%u\n", Size2::suffix(), reg1);
#endif

    typename Size2::svalue_type value = Size1::get(c.regs.d[reg1]);
    Size2::put(c.regs.d[reg1], value);
    c.regs.ccr.set_cc(value);

    c.regs.advance_pc(2);
  }

#if 0
  void
  extw(uint_type op, context &c, unsigned long data)
  {
    data_register ea1(op & 0x7, 2);
#ifdef HAVE_NANA_H
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
#ifdef HAVE_NANA_H
    L(" extl %%d%u\n", reg1);
#endif

    sint32_type value = extsw(ec.regs.d[reg1]);
    ec.regs.d[reg1] = value;
    ec.regs.ccr.set_cc(value);

    ec.regs.pc += 2;
  }
#endif

  template <class Destination> void
  m68k_jmp(uint_type op, context &c, unsigned long data)
  {
    Destination ea1(op & 0x7, 2);
#ifdef HAVE_NANA_H
    L("\tjmp %s\n", ea1.text(c).c_str());
#endif

    // The condition codes are not affected by this instruction.
    uint32_type address = ea1.address(c);

    long_word_size::put(c.regs.pc, address);
  }

  template <class Destination> void
  m68k_jsr(uint_type op, context &c, unsigned long data)
  {
    Destination ea1(op & 0x7, 2);
#ifdef HAVE_NANA_H
    L("\tjsr %s\n", ea1.text(c).c_str());
#endif

    // XXX: The condition codes are not affected.
    uint32_type address = ea1.address(c);
    memory::function_code fc = c.data_fc();
    uint32_type sp = c.regs.a[7] - long_word_size::aligned_value_size();
    long_word_size::put(*c.mem, fc, sp,
			c.regs.pc + 2 + Destination::extension_size());
    long_word_size::put(c.regs.a[7], sp);

    long_word_size::put(c.regs.pc, address);
  }

  template <class Destination> void
  m68k_lea(uint_type op, context &c, unsigned long data)
  {
    Destination ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\tlea%s %s,%%a%u\n", long_word_size::suffix(), ea1.text(c).c_str(),
      reg2);
#endif

    // XXX: The condition codes are not affected.
    uint32_type address = ea1.address(c);
    long_word_size::put(c.regs.a[reg2], address);

    long_word_size::put(c.regs.pc,
			c.regs.pc + 2 + Destination::extension_size());
  }

  void
  m68k_link(uint_type op, context &c, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    word_size::svalue_type disp = c.fetch(word_size(), 2);
#ifdef HAVE_NANA_H
    L("\tlink %%a%u,#%#x\n", reg1, word_size::uvalue(disp));
#endif

    // XXX: The condition codes are not affected.
    memory::function_code fc = c.data_fc();
    uint32_type fp = c.regs.a[7] - long_word_size::aligned_value_size();
    long_word_size::put(*c.mem, fc, fp, c.regs.a[reg1]);
    long_word_size::put(c.regs.a[7], fp + disp);
    long_word_size::put(c.regs.a[reg1], fp);

    long_word_size::put(c.regs.pc,
			c.regs.pc + 2 + word_size::aligned_value_size());
  }

  /* Handles a LSL instruction (immediate).  */
  template <class Size> void
  m68k_lsl_i(uint_type op, context &c, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int value2 = op >> 9 & 0x7;
    if (value2 == 0)
      value2 = 8;
#ifdef HAVE_NANA_H
    L("\tlsl%s #%u,%%d%u\n", Size::suffix(), value2, reg1);
#endif

    typename Size::svalue_type value1 = Size::get(c.regs.d[reg1]);
    typename Size::svalue_type value
      = Size::svalue(Size::uvalue(value1) << value2);
    Size::put(c.regs.d[reg1], value);
    c.regs.ccr.set_cc_lsl(value, value1, value2 + (32 - Size::value_bit())); // FIXME?

    c.regs.advance_pc(2);
  }

#if 0
  void
  lslb_i(uint_type op, context &ec, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int count = op >> 9 & 0x7;
    if (count == 0)
      count = 8;
#ifdef HAVE_NANA_H
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
#ifdef HAVE_NANA_H
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
#endif

  /* Handles a LSL instruction (register).  */
  template <class Size> void
  m68k_lsl_r(uint_type op, context &c, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\tlsl%s %%d%u,%%d%u\n", Size::suffix(), reg2, reg1);
#endif

    unsigned int value2 = c.regs.d[reg2] % Size::value_bit();
    typename Size::svalue_type value1 = Size::get(c.regs.d[reg1]);
    typename Size::svalue_type value
      = Size::svalue(Size::uvalue(value1) << value2);
    Size::put(c.regs.d[reg1], value);
    c.regs.ccr.set_cc_lsl(value, value1, value2 + (32 - Size::value_bit())); // FIXME?

    c.regs.advance_pc(2);
  }

#if 0
  void
  lslw_r(uint_type op, context &ec, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
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
#ifdef HAVE_NANA_H
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
#ifdef HAVE_NANA_H
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
#endif

  /* Handles a LSR instruction (immediate).  */
  template <class Size> void
  m68k_lsr_i(uint_type op, context &c, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int value2 = op >> 9 & 0x7;
    if (value2 == 0)
      value2 = 8;
#ifdef HAVE_NANA_H
    L("\tlsr%s #%u,%%d%u\n", Size::suffix(), value2, reg1);
#endif

    typename Size::svalue_type value1 = Size::get(c.regs.d[reg1]);
    typename Size::svalue_type value
      = Size::svalue(Size::uvalue(value1) >> value2);
    Size::put(c.regs.d[reg1], value);
    c.regs.ccr.set_cc_lsr(value, value1, value2);

    c.regs.advance_pc(2);
  }

#if 0
  void
  lsrb_i(uint_type op, context &c, unsigned long data)
  {
    unsigned int count = op >> 9 & 0x7;
    if (count == 0)
      count = 8;
    data_register ea1(op & 0x7, 2);
#ifdef HAVE_NANA_H
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
#ifdef HAVE_NANA_H
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
#endif

  /* Handles a LSR instruction (register).  */
  template <class Size> void
  m68k_lsr_r(uint_type op, context &c, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\tlsr%s %%d%u,%%d%u\n", Size::suffix(), reg2, reg1);
#endif

    unsigned int value2 = c.regs.d[reg2] % Size::value_bit();
    typename Size::svalue_type value1 = Size::get(c.regs.d[reg1]);
    typename Size::svalue_type value =
      Size::svalue(Size::uvalue(value1) >> value2);
    Size::put(c.regs.d[reg1], value);
    c.regs.ccr.set_cc_lsr(value, value1, value2);

    c.regs.advance_pc(2);
  }

#if 0
  void
  lsrw_r(uint_type op, context &ec, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
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
#ifdef HAVE_NANA_H
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
#ifdef HAVE_NANA_H
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
#endif

  /* Handles a LSR instruction (memory).  */
  template <class Destination> void
  m68k_lsr_m(uint_type op, context &c, unsigned long data)
  {
    const Destination ea1(op & 7, 2);
#ifdef HAVE_NANA_H
    L("\tlsr%s %s\n", word_size::suffix(), ea1.text(c).c_str());
#endif

    word_size::svalue_type value1 = ea1.get(c);
    word_size::svalue_type value
      = word_size::svalue(word_size::uvalue(value1) >> 1);
    ea1.put(c, value);
    c.regs.ccr.set_cc_lsr(value, value1, 1);

    ea1.finish(c);
    c.regs.advance_pc(2 + Destination::extension_size());
  }

  template <class Size, class Source, class Destination> void
  m68k_move(uint_type op, context &c, unsigned long data)
  {
    typedef typename Size::uvalue_type uvalue_type;
    typedef typename Size::svalue_type svalue_type;

    Source ea1(op & 0x7, 2);
    Destination ea2(op >> 9 & 0x7, 2 + ea1.extension_size());
#ifdef HAVE_NANA_H
    L("\tmove%s %s,%s\n", Size::suffix(), ea1.text(c).c_str(),
      ea2.text(c).c_str());
#endif

    svalue_type value = ea1.get(c);
    ea2.put(c, value);
    c.regs.ccr.set_cc(value);

    ea1.finish(c);
    ea2.finish(c);
    c.regs.pc += 2 + ea1.extension_size() + ea2.extension_size();
  }

  /* Handles a MOVE-from-SR instruction.  */
  template <class Destination> void
  m68k_move_from_sr(uint_type op, context &c, unsigned long data)
  {
    typedef word_size::uvalue_type uvalue_type;
    typedef word_size::svalue_type svalue_type;

    Destination ea1(op & 0x7, 2);
#ifdef HAVE_NANA_H
    L("\tmove%s %%sr,%s\n", word_size::suffix(), ea1.text(c).c_str());
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
#ifdef HAVE_NANA_H
    L("\tmove%s %s,%%sr\n", word_size::suffix(), ea1.text(c).c_str());
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
#ifdef HAVE_NANA_H
    L("\tmove%s %%usp,%%a%u\n", long_word_size::suffix(), reg1);
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
#ifdef HAVE_NANA_H
    L("\tmove%s ", long_word_size::suffix());
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
#ifdef HAVE_NANA_H
    L("\tmovea%s %s,%%a%u\n", Size::suffix(), ea1.text(c).c_str(), reg2);
#endif

    // The condition codes are not affected by this instruction.
    svalue_type value = ea1.get(c);
    long_word_size::put(c.regs.a[reg2], value);
    ea1.finish(c);

    c.regs.pc += 2 + ea1.extension_size();
  }

  /* Handles a MOVEM instruction (register to memory) */
  template <class Size, class Destination> void
  m68k_movem_r_m(uint_type op, context &c, unsigned long data)
  {
    uint_type mask = c.fetch(word_size(), 2);
    Destination ea1(op & 0x7, 2 + 2);
#ifdef HAVE_NANA_H
    L("\tmovem%s #%#x,%s\n", Size::suffix(), mask, ea1.text(c).c_str());
#endif

    // This instruction does not affect the condition codes.
    uint_type m = 1;
    memory::function_code fc = c.data_fc();
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

  /* Handles a MOVEM instruction (register to predec memory).  */
  template <class Size> void
  m68k_movem_r_predec(uint_type op, context &c, unsigned long data)
  {
    typedef typename Size::uvalue_type uvalue_type;
    typedef typename Size::svalue_type svalue_type;

    unsigned int reg1 = op & 0x7;
    uint_type mask = c.fetch(word_size(), 2);
#ifdef HAVE_NANA_H
    L("\tmovem%s #%#x,%%a%u@-\n", Size::suffix(), mask, reg1);
#endif

    // This instruction does not affect the condition codes.
    uint_type m = 1;
    memory::function_code fc = c.data_fc();
    sint32_type address
      = long_word_size::svalue(long_word_size::get(c.regs.a[reg1]));
    // This instruction iterates registers in reverse.
    for (uint32_type *i = c.regs.a + 8; i != c.regs.a + 0; --i)
      {
	if (mask & m)
	  {
	    address -= Size::value_size();
	    Size::put(*c.mem, fc, address,
		      long_word_size::svalue(long_word_size::get(*(i - 1))));
	  }
	m <<= 1;
      }
    for (uint32_type *i = c.regs.d + 8; i != c.regs.d + 0; --i)
      {
	if (mask & m)
	  {
	    address -= Size::value_size();
	    Size::put(*c.mem, fc, address,
		      long_word_size::svalue(long_word_size::get(*(i - 1))));
	  }
	m <<= 1;
      }

    long_word_size::put(c.regs.a[reg1], address);
    c.regs.pc += 2 + 2;
  }

  /* Handles a MOVEM instruction (memory to register).  */
  template <class Size, class Source> void
  m68k_movem_m_r(uint_type op, context &c, unsigned long data)
  {
    word_size::uvalue_type mask = c.fetch(word_size(), 2);
    Source ea1(op & 0x7, 2 + word_size::aligned_value_size());
#ifdef HAVE_NANA_H
    L("\tmovem%s %s,#%#x\n", Size::suffix(), ea1.text(c).c_str(), mask);
#endif

    // XXX: The condition codes are not affected.
    uint_type m = 1;
    memory::function_code fc = c.data_fc();
    uint32_type address = ea1.address(c);
    for (uint32_type *i = c.regs.d + 0; i != c.regs.d + 8; ++i)
      {
	if (mask & m)
	  {
	    long_word_size::put(*i, Size::get(*c.mem, fc, address));
	    address += Size::value_size();
	  }
	m <<= 1;
      }
    for (uint32_type *i = c.regs.a + 0; i != c.regs.a + 8; ++i)
      {
	if (mask & m)
	  {
	    long_word_size::put(*i, Size::get(*c.mem, fc, address));
	    address += Size::value_size();
	  }
	m <<= 1;
      }

    c.regs.advance_pc(2 + word_size::aligned_value_size()
		      + Source::extension_size());
  }

#if 0
  /* moveml instruction (memory to register) */
  template <class Source> void
  moveml_mr(uint_type op, context &ec, unsigned long data)
  {
    Source ea1(op & 0x7, 4);
    unsigned int bitmap = ec.fetch(word_size(), 2);
#ifdef HAVE_NANA_H
    L(" moveml %s", ea1.textl(ec));
    L(",#0x%04x\n", bitmap);
#endif

    // XXX: The condition codes are not affected.
    uint32_type address = ea1.address(ec);
    memory::function_code fc = ec.data_fc();
    for (uint32_type *i = ec.regs.d + 0; i != ec.regs.d + 8; ++i)
      {
	if ((bitmap & 1) != 0)
	  {
	    *i = ec.mem->get_32(fc, address);
	    address += 4;
	  }
	bitmap >>= 1;
      }
    for (uint32_type *i = ec.regs.a + 0; i != ec.regs.a + 8; ++i)
      {
	if ((bitmap & 1) != 0)
	  {
	    *i = ec.mem->get_32(fc, address);
	    address += 4;
	  }
	bitmap >>= 1;
      }

    ec.regs.pc += 4 + ea1.isize(4);
  }
#endif

  /* Handles a MOVEM instruction (postinc memory to register).  */
  template <class Size> void
  m68k_movem_postinc_r(uint_type op, context &c, unsigned long data)
  {
    typedef typename Size::uvalue_type uvalue_type;
    typedef typename Size::svalue_type svalue_type;

    unsigned int reg1 = op & 0x7;
    uint_type mask = c.fetch(word_size(), 2);
#ifdef HAVE_NANA_H
    L("\tmovem%s %%a%u@+,#%#x\n", Size::suffix(), reg1, mask);
#endif

    // This instruction does not affect the condition codes.
    uint_type m = 1;
    memory::function_code fc = c.data_fc();
    sint32_type address
      = long_word_size::svalue(long_word_size::get(c.regs.a[reg1]));
    // This instruction sign-extends words to long words.
    for (uint32_type *i = c.regs.d + 0; i != c.regs.d + 8; ++i)
      {
	if (mask & m)
	  {
	    long_word_size::put(*i,
				Size::svalue(Size::get(*c.mem, fc, address)));
	    address += Size::value_size();
	  }
	m <<= 1;
      }
    for (uint32_type *i = c.regs.a + 0; i != c.regs.a + 8; ++i)
      {
	if (mask & m)
	  {
	    long_word_size::put(*i,
				Size::svalue(Size::get(*c.mem, fc, address)));
	    address += Size::value_size();
	  }
	m <<= 1;
      }

    c.regs.a[reg1] = address;
    c.regs.pc += 2 + 2;
  }

  /* Handles a MOVEQ instruction.  */
  void
  m68k_moveq(uint_type op, context &c, unsigned long data)
  {
    long_word_size::svalue_type value = byte_size::svalue(op);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\tmoveq%s #%#x,%%d%u\n", long_word_size::suffix(),
      byte_size::uvalue(value), reg2);
#endif

    long_word_size::put(c.regs.d[reg2], value);
    c.regs.ccr.set_cc(value);

    c.regs.advance_pc(2);
  }

  /* Handles a MULS instruction.  */
  template <class Source> void
  m68k_muls(uint_type op, context &c, unsigned long data)
  {
    const Source ea1(op & 0x7, 2);
    const unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\tmuls%s %s,%%d%u\n", word_size::suffix(), ea1.text(c).c_str(), reg2);
#endif

    word_size::svalue_type value1 = ea1.get(c);
    word_size::svalue_type value2 = word_size::get(c.regs.d[reg2]);
    long_word_size::svalue_type value
      = (long_word_size::svalue_type(value2)
	 * long_word_size::svalue_type(value1));
    long_word_size::put(c.regs.d[reg2], value);
    c.regs.ccr.set_cc(value);	// FIXME.

    ea1.finish(c);
    c.regs.advance_pc(2 + Source::extension_size());
  }

  /* Handles a MULU instruction.  */
  template <class Source> void
  m68k_mulu(uint_type op, context &c, unsigned long data)
  {
    const Source ea1(op & 0x7, 2);
    const unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\tmulu%s %s,%%d%u\n", word_size::suffix(), ea1.text(c).c_str(), reg2);
#endif

    word_size::svalue_type value1 = ea1.get(c);
    word_size::svalue_type value2 = word_size::get(c.regs.d[reg2]);
    long_word_size::svalue_type value
      = (long_word_size::svalue
	 (long_word_size::uvalue_type(word_size::uvalue(value2))
	  * long_word_size::uvalue_type(word_size::uvalue(value1))));
    long_word_size::put(c.regs.d[reg2], value);
    c.regs.ccr.set_cc(value); // FIXME.

    ea1.finish(c);
    c.regs.advance_pc(2 + Source::extension_size());
  }

  /* Handles a NEG instruction.  */
  template <class Size, class Destination> void
  m68k_neg(uint_type op, context &c, unsigned long data)
  {
    const Destination ea1(op & 0x7, 2);
#ifdef HAVE_NANA_H
    L("\tneg%s %s\n", Size::suffix(), ea1.text(c).c_str());
#endif

    typename Size::svalue_type value1 = ea1.get(c);
    typename Size::svalue_type value = Size::svalue(-value1);
    ea1.put(c, value);
    c.regs.ccr.set_cc_sub(value, 0, value1);

    ea1.finish(c);
    c.regs.advance_pc(2 + Destination::extension_size());
  }

#if 0
  template <class Destination> void
  negb(uint_type op, context &c, unsigned long data)
  {
    Destination ea1(op & 0x7, 2);
#ifdef HAVE_NANA_H
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
#ifdef HAVE_NANA_H
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
#ifdef HAVE_NANA_H
    L(" negl %s\n", ea1.textl(ec));
#endif

    sint32_type value1 = ea1.getl(ec);
    sint32_type value = extsl(-value1);
    ea1.putl(ec, value);
    ec.regs.ccr.set_cc_sub(value, 0, value1);
    ea1.finishl(ec);

    ec.regs.pc += 2 + ea1.isize(4);
  }
#endif

  /* Handles a NOT instruction.  */
  template <class Size, class Destination> void
  m68k_not(uint_type op, context &c, unsigned long data)
  {
    typedef typename Size::uvalue_type uvalue_type;
    typedef typename Size::svalue_type svalue_type;

    Destination ea1(op & 0x7, 2);
#ifdef HAVE_NANA_H
    L("\tnot%s %s\n", Size::suffix(), ea1.text(c).c_str());
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
#ifdef HAVE_NANA_H
    L("\tnop\n");
#endif

    c.regs.pc += 2;
  }

  /* Handles an OR instruction.  */
  template <class Size, class Source> void
  m68k_or(uint_type op, context &c, unsigned long data)
  {
    const Source ea1(op & 0x7, 2);
    const unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\tor%s %s,%%d%u\n", Size::suffix(), ea1.text(c).c_str(), reg2);
#endif

    typename Size::svalue_type value1 = ea1.get(c);
    typename Size::svalue_type value2 = Size::get(c.regs.d[reg2]);
    typename Size::svalue_type value
      = Size::svalue(Size::uvalue(value2) | Size::uvalue(value1));
    Size::put(c.regs.d[reg2], value);
    c.regs.ccr.set_cc(value);

    ea1.finish(c);
    c.regs.advance_pc(2 + Source::extension_size());
  }

#if 0
  template <class Source> void
  orb(uint_type op, context &ec, unsigned long data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
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
#ifdef HAVE_NANA_H
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
#ifdef HAVE_NANA_H
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
#endif

  /* Handles an OR instruction (memory destination).  */
  template <class Size, class Destination> void
  m68k_or_m(uint_type op, context &c, unsigned long data)
  {
    typedef typename Size::uvalue_type uvalue_type;
    typedef typename Size::svalue_type svalue_type;

    Destination ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\tor%s %%d%u,%s\n", Size::suffix(), reg2, ea1.text(c).c_str());
#endif

    svalue_type value2 = Size::svalue(Size::get(c.regs.d[reg2]));
    svalue_type value1 = ea1.get(c);
    svalue_type value = Size::svalue(Size::get(value1) | Size::get(value2));
    ea1.put(c, value);
    c.regs.ccr.set_cc(value);

    ea1.finish(c);
    c.regs.pc += 2 + ea1.extension_size();
  }

  /* Handles an ORI instruction.  */
  template <class Size, class Destination> void
  m68k_ori(uint_type op, context &c, unsigned long data)
  {
    const typename Size::svalue_type value2 = c.fetch(Size(), 2);
    const Destination ea1(op & 0x7, 2 + Size::aligned_value_size());
#ifdef HAVE_NANA_H
    L("\tori%s #%#lx,%s", Size::suffix(), Size::uvalue(value2) + 0UL,
      ea1.text(c).c_str());
#endif

    const typename Size::svalue_type value1 = ea1.get(c);
    const typename Size::svalue_type value
      = Size::svalue(Size::uvalue(value1) | Size::uvalue(value2));
    ea1.put(c, value);
    c.regs.ccr.set_cc(value);

    ea1.finish(c);
    c.regs.advance_pc(2 + Size::aligned_value_size()
		      + Destination::extension_size());
  }

  /* Handles an ORI-to-CCR instruction.  */
  void
  m68k_ori_to_ccr(uint_type op, context &c, unsigned long data)
  {
    typedef byte_size::uvalue_type uvalue_type;
    typedef byte_size::svalue_type svalue_type;

    uvalue_type value2 = c.fetch(byte_size(), 2);
#ifdef HAVE_NANA_H
    L("\tori%s #%#x,%%ccr\n", byte_size::suffix(), value2);
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
#ifdef HAVE_NANA_H
    L("\tori%s #%#x,%%sr\n", word_size::suffix(), value2);
#endif

    // This instruction is privileged.
    if (!c.supervisor_state())
      throw privilege_violation_exception();

    uvalue_type value1 = c.sr();
    uvalue_type value = value1 | value2;
    c.set_sr(value);

    c.regs.pc += 2 + word_size::aligned_value_size();
  }

#if 0
  template <class Destination> void
  orib(uint_type op, context &ec, unsigned long data)
  {
    sint_type value2 = extsb(ec.fetch(word_size(), 2));
    Destination ea1(op & 0x7, 2 + 2);
#ifdef HAVE_NANA_H
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
#ifdef HAVE_NANA_H
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
#ifdef HAVE_NANA_H
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
#endif

  /* Handles a PEA instruction.  */
  template <class Destination> void
  m68k_pea(uint_type op, context &c, unsigned long data)
  {
    const Destination ea1(op & 0x7, 2);
#ifdef HAVE_NANA_H
    L("\tpea%s %s\n", long_word_size::suffix(), ea1.text(c).c_str());
#endif

    // XXX: The condition codes are not affected.
    uint32_type address = ea1.address(c);
    memory::function_code fc = c.data_fc();
    uint32_type sp = c.regs.a[7] - long_word_size::aligned_value_size();
    long_word_size::put(*c.mem, fc, sp, address);
    long_word_size::put(c.regs.a[7], sp);

    c.regs.advance_pc(2 + Destination::extension_size());
  }

  /* Handles a ROL instruction (register).  */
  template <class Size> void
  m68k_rol_r(uint_type op, context &c, unsigned long data)
  {
    typedef typename Size::uvalue_type uvalue_type;
    typedef typename Size::svalue_type svalue_type;

    unsigned int reg1 = op & 0x7;
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\trol%s ", Size::suffix());
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
#ifdef HAVE_NANA_H
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

  /* Handles a ROL instruction (immediate).  */
  template <class Size> void
  m68k_rol_i(uint_type op, context &c, unsigned long data)
  {
    const unsigned int reg1 = op & 0x7;
    const unsigned int value2 = ((op >> 9) - 1 & 0x7) + 1;
#ifdef HAVE_NANA_H
    L("\trol%s #%u,%%d%u\n", Size::suffix(), value2, reg1);
#endif

    const typename Size::svalue_type value1 = Size::get(c.regs.d[reg1]);
    const typename Size::svalue_type value
      = Size::svalue(Size::uvalue(value1) << value2
		     | Size::uvalue(value1) >> Size::value_bit() - value2);
    Size::put(c.regs.d[reg1], value);
    c.regs.ccr.set_cc(value);	// FIXME.

    c.regs.advance_pc(2);
  }

#if 0
  void
  rolw_i(uint_type op, context &ec, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
    unsigned int count = op >> 9 & 0x7;
    if (count == 0)
      count = 8;
#ifdef HAVE_NANA_H
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
#ifdef HAVE_NANA_H
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
#endif

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
#ifdef HAVE_NANA_H
    L("\tror%s #%u,%%d%u\n", Size::suffix(), count, reg1);
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

  /* Handles a ROXR instruction (immediate).  */
  template <class Size> void
  m68k_roxr_i(uint_type op, context &c, unsigned long data)
  {
    const unsigned int reg1 = op & 0x7;
    const unsigned int value2 = ((op >> 9) - 1 & 0x7) + 1;
#ifdef HAVE_NANA_H
    L("\troxr%s #%u,%%d%u", Size::suffix(), value2, reg1);
#endif

    const typename Size::svalue_type value1 = Size::get(c.regs.d[reg1]);
    const typename Size::svalue_type value
      = Size::svalue(Size::uvalue(value1) >> value2
		     | c.regs.ccr.x() << Size::value_bit() - value2
		     | Size::uvalue(value1) << Size::value_bit() + 1 - value2);
    Size::put(c.regs.d[reg1], value);
    c.regs.ccr.set_cc(value);	// FIXME.

    c.regs.advance_pc(2);
  }

#if 0
  void
  roxrw_i(uint_type op, context &c, unsigned long data)
  {
    unsigned int count = op >> 9 & 0x7;
    if (count == 0)
      count = 8;
    data_register ea1(op & 0x7, 2);
#ifdef HAVE_NANA_H
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
#ifdef HAVE_NANA_H
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
#endif

  /* Handles a RTE instruction.  */
  void
  m68k_rte(uint_type op, context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    L("\trte\n");
#endif

    // This instruction is privileged.
    if (!c.supervisor_state())
      throw privilege_violation_exception();

    uint_type status = c.mem->get_16(memory::SUPER_DATA, c.regs.a[7] + 0);
    uint32_type value = c.mem->get_32(memory::SUPER_DATA, c.regs.a[7] + 2);
    c.regs.a[7] += 6;
    c.set_sr(status);
    c.regs.pc = value;
  }

  void
  m68k_rts(uint_type op, context &c, unsigned long data)
  {
#ifdef HAVE_NANA_H
    L("\trts\n");
#endif

    // XXX: The condition codes are not affected.
    memory::function_code fc = c.data_fc();
    sint32_type value = long_word_size::get(*c.mem, fc, c.regs.a[7]);
    long_word_size::put(c.regs.a[7], c.regs.a[7] + 4);

    long_word_size::put(c.regs.pc, value);
  }

  template <class Condition, class Destination> void
  m68k_s(uint_type op, context &c, unsigned long data)
  {
    const Destination ea1(op & 0x7, 2);
#ifdef HAVE_NANA_H
    L("\ts%s%s %s\n", Condition::text(), byte_size::suffix(),
      ea1.text(c).c_str());
#endif

    // The condition codes are not affected by this instruction.
    Condition cond;
    ea1.put(c, cond(c) ? ~0 : 0);

    ea1.finish(c);
    c.regs.advance_pc(2 + Destination::extension_size());
  }

  /* Handles a SUB instruction.  */
  template <class Size, class Source> void
  m68k_sub(uint_type op, context &c, unsigned long data)
  {
    const Source ea1(op & 0x7, 2);
    const unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\tsub%s %s,%%d%u\n", Size::suffix(), ea1.text(c).c_str(), reg2);
#endif

    const typename Size::svalue_type value1 = ea1.get(c);
    const typename Size::svalue_type value2 = Size::get(c.regs.d[reg2]);
    const typename Size::svalue_type value = Size::svalue(value2 - value1);
    Size::put(c.regs.d[reg2], value);
    c.regs.ccr.set_cc_sub(value, value2, value1);

    ea1.finish(c);
    c.regs.advance_pc(2 + Source::extension_size());
  }

  /* Handles a SUB instruction (memory destination).  */
  template <class Size, class Destination> void
  m68k_sub_m(uint_type op, context &c, unsigned long data)
  {
    typedef typename Size::uvalue_type uvalue_type;
    typedef typename Size::svalue_type svalue_type;

    Destination ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\tsub%s %%d%u,%s\n", Size::suffix(), reg2, ea1.text(c).c_str());
#endif

    svalue_type value2 = Size::svalue(Size::get(c.regs.d[reg2]));
    svalue_type value1 = ea1.get(c);
    svalue_type value = Size::svalue(Size::get(value1 - value2));
    ea1.put(c, value);
    c.regs.ccr.set_cc_sub(value, value1, value2);
    ea1.finish(c);

    c.regs.pc += 2 + ea1.extension_size();
  }

#if 0
  template <class Source> void
  subb(uint_type op, context &ec, unsigned long data)
  {
    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
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
#ifdef HAVE_NANA_H
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
#ifdef HAVE_NANA_H
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
#endif

  /* Handles a SUBA instruction.  */
  template <class Size, class Source> void
  m68k_suba(uint_type op, context &c, unsigned long data)
  {
    typedef long_word_size::uvalue_type uvalue_type;
    typedef long_word_size::svalue_type svalue_type;

    Source ea1(op & 0x7, 2);
    unsigned int reg2 = op >> 9 & 0x7;
#ifdef HAVE_NANA_H
    L("\tsuba%s %s,%%a%u\n", Size::suffix(), ea1.text(c).c_str(), reg2);
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

  /* Handles a SUBI instruction.  */
  template <class Size, class Destination> void
  m68k_subi(uint_type op, context &c, unsigned long data)
  {
    typename Size::svalue_type value2 = c.fetch(word_size(), 2);
    Destination ea1(op & 0x7, 2 + word_size::aligned_value_size());
#ifdef HAVE_NANA_H
    L("\tsubi%s #%#lx,%s\n", Size::suffix(), Size::uvalue(value2) + 0UL,
      ea1.text(c).c_str());
#endif

    typename Size::svalue_type value1 = ea1.get(c);
    typename Size::svalue_type value = Size::svalue(value1 - value2);
    ea1.put(c, value);
    c.regs.ccr.set_cc_sub(value, value1, value2);

    ea1.finish(c);
    long_word_size::put(c.regs.pc,
			c.regs.pc + 2 + word_size::aligned_value_size()
			+ Destination::extension_size());
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
#ifdef HAVE_NANA_H
    L("\tsubq%s #%d,%s\n", Size::suffix(), value2, ea1.text(c).c_str());
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
#ifdef HAVE_NANA_H
    L("\tsubq%s #%d,%%a%u\n", Size::suffix(), value2, reg1);
#endif

    // This instruction does not affect the condition codes.
    svalue_type value1
      = long_word_size::svalue(long_word_size::get(c.regs.a[reg1]));
    svalue_type value
      = long_word_size::svalue(long_word_size::get(value1 - value2));
    long_word_size::put(c.regs.a[reg1], value);

    c.regs.pc += 2;
  }

  /* Handles a SWAP instruction.  */
  void
  m68k_swap(uint_type op, context &c, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
#ifdef HAVE_NANA_H
    L("\tswap%s %%d%u\n", word_size::suffix(), reg1);
#endif

    long_word_size::svalue_type value1 = long_word_size::get(c.regs.d[reg1]);
    long_word_size::svalue_type value
      = long_word_size::svalue(long_word_size::uvalue(value1) << 16
			       | (long_word_size::uvalue(value1) >> 16
				  & 0xffff));
    long_word_size::put(c.regs.d[reg1], value);
    c.regs.ccr.set_cc(value);

    long_word_size::put(c.regs.pc, c.regs.pc + 2);
  }

  /* Handles a TST instruction.  */
  template <class Size, class Destination> void
  m68k_tst(uint_type op, context &c, unsigned long data)
  {
    Destination ea1(op & 0x7, 2);
#ifdef HAVE_NANA_H
    L("\ttst%s %s\n", Size::suffix(), ea1.text(c).c_str());
#endif

    typename Size::svalue_type value = ea1.get(c);
    c.regs.ccr.set_cc(value);

    ea1.finish(c);
    long_word_size::put(c.regs.pc,
			c.regs.pc + 2 + Destination::extension_size());
  }

#if 0
  template <class Destination> void
  tstb(uint_type op, context &ec, unsigned long data)
    {
      Destination ea1(op & 0x7, 2);
#ifdef HAVE_NANA_H
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
#ifdef HAVE_NANA_H
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
#ifdef HAVE_NANA_H
      VL((" tstl %s\n", ea1.textl(ec)));
#endif

      sint32_type value = ea1.getl(ec);
      ec.regs.ccr.set_cc(value);
      ea1.finishl(ec);

      ec.regs.pc += 2 + ea1.isize(4);
    }
#endif

  void
  m68k_unlk(uint_type op, context &c, unsigned long data)
  {
    unsigned int reg1 = op & 0x7;
#ifdef HAVE_NANA_H
    L("\tunlk %%a%u\n", reg1);
#endif

    // XXX: The condition codes are not affected.
    memory::function_code fc = c.data_fc();
    uint32_type fp = c.regs.a[reg1];
    long_word_size::put(c.regs.a[reg1], long_word_size::get(*c.mem, fc, fp));
    long_word_size::put(c.regs.a[7],
			fp + long_word_size::aligned_value_size());

    long_word_size::put(c.regs.pc, c.regs.pc + 2);
  }

#ifdef HAVE_NANA_H
# undef L_DEFAULT_GUARD
# define L_DEFAULT_GUARD true
#endif

  /* Initializes the machine instructions of execution unit EU.  */
  void
  initialize_instructions(exec_unit &eu)
  {
    eu.set_instruction(0x0000, 0x0007,
		       &m68k_ori<byte_size, byte_d_register>);
    eu.set_instruction(0x0010, 0x0007,
		       &m68k_ori<byte_size, byte_indirect>);
    eu.set_instruction(0x0018, 0x0007,
		       &m68k_ori<byte_size, byte_postinc_indirect>);
    eu.set_instruction(0x0020, 0x0007,
		       &m68k_ori<byte_size, byte_predec_indirect>);
    eu.set_instruction(0x0028, 0x0007,
		       &m68k_ori<byte_size, byte_disp_indirect>);
    eu.set_instruction(0x0030, 0x0007,
		       &m68k_ori<byte_size, byte_index_indirect>);
    eu.set_instruction(0x0038, 0x0000,
		       &m68k_ori<byte_size, byte_abs_short>);
    eu.set_instruction(0x0039, 0x0000,
		       &m68k_ori<byte_size, byte_abs_long>);
    eu.set_instruction(0x003c, 0x0000,
		       &m68k_ori_to_ccr);
    eu.set_instruction(0x0040, 0x0007,
		       &m68k_ori<word_size, word_d_register>);
    eu.set_instruction(0x0050, 0x0007,
		       &m68k_ori<word_size, word_indirect>);
    eu.set_instruction(0x0058, 0x0007,
		       &m68k_ori<word_size, word_postinc_indirect>);
    eu.set_instruction(0x0060, 0x0007,
		       &m68k_ori<word_size, word_predec_indirect>);
    eu.set_instruction(0x0068, 0x0007,
		       &m68k_ori<word_size, word_disp_indirect>);
    eu.set_instruction(0x0070, 0x0007,
		       &m68k_ori<word_size, word_index_indirect>);
    eu.set_instruction(0x0078, 0x0000,
		       &m68k_ori<word_size, word_abs_short>);
    eu.set_instruction(0x0079, 0x0000,
		       &m68k_ori<word_size, word_abs_long>);
    eu.set_instruction(0x007c, 0x0000,
		       &m68k_ori_to_sr);
    eu.set_instruction(0x0080, 0x0007,
		       &m68k_ori<long_word_size, long_word_d_register>);
    eu.set_instruction(0x0090, 0x0007,
		       &m68k_ori<long_word_size, long_word_indirect>);
    eu.set_instruction(0x0098, 0x0007,
		       &m68k_ori<long_word_size, long_word_postinc_indirect>);
    eu.set_instruction(0x00a0, 0x0007,
		       &m68k_ori<long_word_size, long_word_predec_indirect>);
    eu.set_instruction(0x00a8, 0x0007,
		       &m68k_ori<long_word_size, long_word_disp_indirect>);
    eu.set_instruction(0x00b0, 0x0007,
		       &m68k_ori<long_word_size, long_word_index_indirect>);
    eu.set_instruction(0x00b8, 0x0000,
		       &m68k_ori<long_word_size, long_word_abs_short>);
    eu.set_instruction(0x00b9, 0x0000,
		       &m68k_ori<long_word_size, long_word_abs_long>);
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
    eu.set_instruction(0x027c, 0x0000,
		       &m68k_andi_to_sr);
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
    eu.set_instruction(0x0400, 0x0007,
		       &m68k_subi<byte_size, byte_d_register>);
    eu.set_instruction(0x0410, 0x0007,
		       &m68k_subi<byte_size, byte_indirect>);
    eu.set_instruction(0x0418, 0x0007,
		       &m68k_subi<byte_size, byte_postinc_indirect>);
    eu.set_instruction(0x0420, 0x0007,
		       &m68k_subi<byte_size, byte_predec_indirect>);
    eu.set_instruction(0x0428, 0x0007,
		       &m68k_subi<byte_size, byte_disp_indirect>);
    eu.set_instruction(0x0430, 0x0007,
		       &m68k_subi<byte_size, byte_index_indirect>);
    eu.set_instruction(0x0438, 0x0000,
		       &m68k_subi<byte_size, byte_abs_short>);
    eu.set_instruction(0x0439, 0x0000,
		       &m68k_subi<byte_size, byte_abs_long>);
    eu.set_instruction(0x0440, 0x0007,
		       &m68k_subi<word_size, word_d_register>);
    eu.set_instruction(0x0450, 0x0007,
		       &m68k_subi<word_size, word_indirect>);
    eu.set_instruction(0x0458, 0x0007,
		       &m68k_subi<word_size, word_postinc_indirect>);
    eu.set_instruction(0x0460, 0x0007,
		       &m68k_subi<word_size, word_predec_indirect>);
    eu.set_instruction(0x0468, 0x0007,
		       &m68k_subi<word_size, word_disp_indirect>);
    eu.set_instruction(0x0470, 0x0007,
		       &m68k_subi<word_size, word_index_indirect>);
    eu.set_instruction(0x0478, 0x0000,
		       &m68k_subi<word_size, word_abs_short>);
    eu.set_instruction(0x0479, 0x0000,
		       &m68k_subi<word_size, word_abs_long>);
    eu.set_instruction(0x0480, 0x0007,
		       &m68k_subi<long_word_size, long_word_d_register>);
    eu.set_instruction(0x0490, 0x0007,
		       &m68k_subi<long_word_size, long_word_indirect>);
    eu.set_instruction(0x0498, 0x0007,
		       &m68k_subi<long_word_size, long_word_postinc_indirect>);
    eu.set_instruction(0x04a0, 0x0007,
		       &m68k_subi<long_word_size, long_word_predec_indirect>);
    eu.set_instruction(0x04a8, 0x0007,
		       &m68k_subi<long_word_size, long_word_disp_indirect>);
    eu.set_instruction(0x04b0, 0x0007,
		       &m68k_subi<long_word_size, long_word_index_indirect>);
    eu.set_instruction(0x04b8, 0x0000,
		       &m68k_subi<long_word_size, long_word_abs_short>);
    eu.set_instruction(0x04b9, 0x0000,
		       &m68k_subi<long_word_size, long_word_abs_long>);
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
    eu.set_instruction(0x0800, 0x0007,
		       &m68k_btst_i<long_word_size, long_word_d_register>);
    eu.set_instruction(0x0810, 0x0007,
		       &m68k_btst_i<byte_size, byte_indirect>);
    eu.set_instruction(0x0818, 0x0007,
		       &m68k_btst_i<byte_size, byte_postinc_indirect>);
    eu.set_instruction(0x0820, 0x0007,
		       &m68k_btst_i<byte_size, byte_predec_indirect>);
    eu.set_instruction(0x0828, 0x0007,
		       &m68k_btst_i<byte_size, byte_disp_indirect>);
    eu.set_instruction(0x0830, 0x0007,
		       &m68k_btst_i<byte_size, byte_index_indirect>);
    eu.set_instruction(0x0838, 0x0000,
		       &m68k_btst_i<byte_size, byte_abs_short>);
    eu.set_instruction(0x0839, 0x0000,
		       &m68k_btst_i<byte_size, byte_abs_long>);
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
    eu.set_instruction(0x08c0, 0x0007,
		       &m68k_bset_i<long_word_size, long_word_d_register>);
    eu.set_instruction(0x08d0, 0x0007,
		       &m68k_bset_i<byte_size, byte_indirect>);
    eu.set_instruction(0x08d8, 0x0007,
		       &m68k_bset_i<byte_size, byte_postinc_indirect>);
    eu.set_instruction(0x08e0, 0x0007,
		       &m68k_bset_i<byte_size, byte_predec_indirect>);
    eu.set_instruction(0x08e8, 0x0007,
		       &m68k_bset_i<byte_size, byte_disp_indirect>);
    eu.set_instruction(0x08f0, 0x0007,
		       &m68k_bset_i<byte_size, byte_index_indirect>);
    eu.set_instruction(0x08f8, 0x0000,
		       &m68k_bset_i<byte_size, byte_abs_short>);
    eu.set_instruction(0x08f9, 0x0000,
		       &m68k_bset_i<byte_size, byte_abs_long>);
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
    eu.set_instruction(0x0c00, 0x0007,
		       &m68k_cmpi<byte_size, byte_d_register>);
    eu.set_instruction(0x0c10, 0x0007,
		       &m68k_cmpi<byte_size, byte_indirect>);
    eu.set_instruction(0x0c18, 0x0007,
		       &m68k_cmpi<byte_size, byte_postinc_indirect>);
    eu.set_instruction(0x0c20, 0x0007,
		       &m68k_cmpi<byte_size, byte_predec_indirect>);
    eu.set_instruction(0x0c28, 0x0007,
		       &m68k_cmpi<byte_size, byte_disp_indirect>);
    eu.set_instruction(0x0c30, 0x0007,
		       &m68k_cmpi<byte_size, byte_index_indirect>);
    eu.set_instruction(0x0c38, 0x0000,
		       &m68k_cmpi<byte_size, byte_abs_short>);
    eu.set_instruction(0x0c39, 0x0000,
		       &m68k_cmpi<byte_size, byte_abs_long>);
    eu.set_instruction(0x0c40, 0x0007,
		       &m68k_cmpi<word_size, word_d_register>);
    eu.set_instruction(0x0c50, 0x0007,
		       &m68k_cmpi<word_size, word_indirect>);
    eu.set_instruction(0x0c58, 0x0007,
		       &m68k_cmpi<word_size, word_postinc_indirect>);
    eu.set_instruction(0x0c60, 0x0007,
		       &m68k_cmpi<word_size, word_predec_indirect>);
    eu.set_instruction(0x0c68, 0x0007,
		       &m68k_cmpi<word_size, word_disp_indirect>);
    eu.set_instruction(0x0c70, 0x0007,
		       &m68k_cmpi<word_size, word_index_indirect>);
    eu.set_instruction(0x0c78, 0x0000,
		       &m68k_cmpi<word_size, word_abs_short>);
    eu.set_instruction(0x0c79, 0x0000,
		       &m68k_cmpi<word_size, word_abs_long>);
    eu.set_instruction(0x0c80, 0x0007,
		       &m68k_cmpi<long_word_size, long_word_d_register>);
    eu.set_instruction(0x0c90, 0x0007,
		       &m68k_cmpi<long_word_size, long_word_indirect>);
    eu.set_instruction(0x0c98, 0x0007,
		       &m68k_cmpi<long_word_size, long_word_postinc_indirect>);
    eu.set_instruction(0x0ca0, 0x0007,
		       &m68k_cmpi<long_word_size, long_word_predec_indirect>);
    eu.set_instruction(0x0ca8, 0x0007,
		       &m68k_cmpi<long_word_size, long_word_disp_indirect>);
    eu.set_instruction(0x0cb0, 0x0007,
		       &m68k_cmpi<long_word_size, long_word_index_indirect>);
    eu.set_instruction(0x0cb8, 0x0000,
		       &m68k_cmpi<long_word_size, long_word_abs_short>);
    eu.set_instruction(0x0cb9, 0x0000,
		       &m68k_cmpi<long_word_size, long_word_abs_long>);
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
    eu.set_instruction(0x41d0, 0x0e07,
		       &m68k_lea<word_indirect>);
    eu.set_instruction(0x41e8, 0x0e07,
		       &m68k_lea<word_disp_indirect>);
    eu.set_instruction(0x41f0, 0x0e07,
		       &m68k_lea<word_index_indirect>);
    eu.set_instruction(0x41f8, 0x0e00,
		       &m68k_lea<word_abs_short>);
    eu.set_instruction(0x41f9, 0x0e00,
		       &m68k_lea<word_abs_long>);
    eu.set_instruction(0x41fa, 0x0e00,
		       &m68k_lea<word_disp_pc_indirect>);
    eu.set_instruction(0x41fb, 0x0e00,
		       &m68k_lea<word_index_pc_indirect>);
    eu.set_instruction(0x4200, 0x0007,
		       &m68k_clr<byte_size, byte_d_register>);
    eu.set_instruction(0x4210, 0x0007,
		       &m68k_clr<byte_size, byte_indirect>);
    eu.set_instruction(0x4218, 0x0007,
		       &m68k_clr<byte_size, byte_postinc_indirect>);
    eu.set_instruction(0x4220, 0x0007,
		       &m68k_clr<byte_size, byte_predec_indirect>);
    eu.set_instruction(0x4228, 0x0007,
		       &m68k_clr<byte_size, byte_disp_indirect>);
    eu.set_instruction(0x4230, 0x0007,
		       &m68k_clr<byte_size, byte_index_indirect>);
    eu.set_instruction(0x4238, 0x0000,
		       &m68k_clr<byte_size, byte_abs_short>);
    eu.set_instruction(0x4239, 0x0000,
		       &m68k_clr<byte_size, byte_abs_long>);
    eu.set_instruction(0x4240, 0x0007,
		       &m68k_clr<word_size, word_d_register>);
    eu.set_instruction(0x4250, 0x0007,
		       &m68k_clr<word_size, word_indirect>);
    eu.set_instruction(0x4258, 0x0007,
		       &m68k_clr<word_size, word_postinc_indirect>);
    eu.set_instruction(0x4260, 0x0007,
		       &m68k_clr<word_size, word_predec_indirect>);
    eu.set_instruction(0x4268, 0x0007,
		       &m68k_clr<word_size, word_disp_indirect>);
    eu.set_instruction(0x4270, 0x0007,
		       &m68k_clr<word_size, word_index_indirect>);
    eu.set_instruction(0x4278, 0x0000,
		       &m68k_clr<word_size, word_abs_short>);
    eu.set_instruction(0x4279, 0x0000,
		       &m68k_clr<word_size, word_abs_long>);
    eu.set_instruction(0x4280, 0x0007,
		       &m68k_clr<long_word_size, long_word_d_register>);
    eu.set_instruction(0x4290, 0x0007,
		       &m68k_clr<long_word_size, long_word_indirect>);
    eu.set_instruction(0x4298, 0x0007,
		       &m68k_clr<long_word_size, long_word_postinc_indirect>);
    eu.set_instruction(0x42a0, 0x0007,
		       &m68k_clr<long_word_size, long_word_predec_indirect>);
    eu.set_instruction(0x42a8, 0x0007,
		       &m68k_clr<long_word_size, long_word_disp_indirect>);
    eu.set_instruction(0x42b0, 0x0007,
		       &m68k_clr<long_word_size, long_word_index_indirect>);
    eu.set_instruction(0x42b8, 0x0000,
		       &m68k_clr<long_word_size, long_word_abs_short>);
    eu.set_instruction(0x42b9, 0x0000,
		       &m68k_clr<long_word_size, long_word_abs_long>);
    eu.set_instruction(0x4400, 0x0007,
		       &m68k_neg<byte_size, byte_d_register>);
    eu.set_instruction(0x4410, 0x0007,
		       &m68k_neg<byte_size, byte_indirect>);
    eu.set_instruction(0x4418, 0x0007,
		       &m68k_neg<byte_size, byte_postinc_indirect>);
    eu.set_instruction(0x4420, 0x0007,
		       &m68k_neg<byte_size, byte_predec_indirect>);
    eu.set_instruction(0x4428, 0x0007,
		       &m68k_neg<byte_size, byte_disp_indirect>);
    eu.set_instruction(0x4430, 0x0007,
		       &m68k_neg<byte_size, byte_index_indirect>);
    eu.set_instruction(0x4438, 0x0000,
		       &m68k_neg<byte_size, byte_abs_short>);
    eu.set_instruction(0x4439, 0x0000,
		       &m68k_neg<byte_size, byte_abs_long>);
    eu.set_instruction(0x4440, 0x0007,
		       &m68k_neg<word_size, word_d_register>);
    eu.set_instruction(0x4450, 0x0007,
		       &m68k_neg<word_size, word_indirect>);
    eu.set_instruction(0x4458, 0x0007,
		       &m68k_neg<word_size, word_postinc_indirect>);
    eu.set_instruction(0x4460, 0x0007,
		       &m68k_neg<word_size, word_predec_indirect>);
    eu.set_instruction(0x4468, 0x0007,
		       &m68k_neg<word_size, word_disp_indirect>);
    eu.set_instruction(0x4470, 0x0007,
		       &m68k_neg<word_size, word_index_indirect>);
    eu.set_instruction(0x4478, 0x0000,
		       &m68k_neg<word_size, word_abs_short>);
    eu.set_instruction(0x4479, 0x0000,
		       &m68k_neg<word_size, word_abs_long>);
    eu.set_instruction(0x4480, 0x0007,
		       &m68k_neg<long_word_size, long_word_d_register>);
    eu.set_instruction(0x4490, 0x0007,
		       &m68k_neg<long_word_size, long_word_indirect>);
    eu.set_instruction(0x4498, 0x0007,
		       &m68k_neg<long_word_size, long_word_postinc_indirect>);
    eu.set_instruction(0x44a0, 0x0007,
		       &m68k_neg<long_word_size, long_word_predec_indirect>);
    eu.set_instruction(0x44a8, 0x0007,
		       &m68k_neg<long_word_size, long_word_disp_indirect>);
    eu.set_instruction(0x44b0, 0x0007,
		       &m68k_neg<long_word_size, long_word_index_indirect>);
    eu.set_instruction(0x44b8, 0x0000,
		       &m68k_neg<long_word_size, long_word_abs_short>);
    eu.set_instruction(0x44b9, 0x0000,
		       &m68k_neg<long_word_size, long_word_abs_long>);
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
    eu.set_instruction(0x4840, 0x0007,
		       &m68k_swap);
    eu.set_instruction(0x4850, 0x0007,
		       &m68k_pea<word_indirect>);
    eu.set_instruction(0x4868, 0x0007,
		       &m68k_pea<word_disp_indirect>);
    eu.set_instruction(0x4870, 0x0007,
		       &m68k_pea<word_index_indirect>);
    eu.set_instruction(0x4878, 0x0000,
		       &m68k_pea<word_abs_short>);
    eu.set_instruction(0x4879, 0x0000,
		       &m68k_pea<word_abs_long>);
    eu.set_instruction(0x487a, 0x0000,
		       &m68k_pea<word_disp_pc_indirect>);
    eu.set_instruction(0x487b, 0x0000,
		       &m68k_pea<word_index_pc_indirect>);
    eu.set_instruction(0x4880, 0x0007,
		       &m68k_ext<byte_size, word_size>);
    eu.set_instruction(0x4890, 0x0007,
		       &m68k_movem_r_m<word_size, word_indirect>);
    eu.set_instruction(0x48a0, 0x0007,
		       &m68k_movem_r_predec<word_size>);
    eu.set_instruction(0x48a8, 0x0007,
		       &m68k_movem_r_m<word_size, word_disp_indirect>);
    eu.set_instruction(0x48b0, 0x0007,
		       &m68k_movem_r_m<word_size, word_index_indirect>);
    eu.set_instruction(0x48b8, 0x0000,
		       &m68k_movem_r_m<word_size, word_abs_short>);
    eu.set_instruction(0x48b9, 0x0000,
		       &m68k_movem_r_m<word_size, word_abs_long>);
    eu.set_instruction(0x48c0, 0x0007,
		       &m68k_ext<word_size, long_word_size>);
    eu.set_instruction(0x48d0, 0x0007,
		       &m68k_movem_r_m<long_word_size, long_word_indirect>);
    eu.set_instruction(0x48e0, 0x0007,
		       &m68k_movem_r_predec<long_word_size>);
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
    eu.set_instruction(0x4a00, 0x0007,
		       &m68k_tst<byte_size, byte_d_register>);
    eu.set_instruction(0x4a10, 0x0007,
		       &m68k_tst<byte_size, byte_indirect>);
    eu.set_instruction(0x4a18, 0x0007,
		       &m68k_tst<byte_size, byte_postinc_indirect>);
    eu.set_instruction(0x4a20, 0x0007,
		       &m68k_tst<byte_size, byte_predec_indirect>);
    eu.set_instruction(0x4a28, 0x0007,
		       &m68k_tst<byte_size, byte_disp_indirect>);
    eu.set_instruction(0x4a30, 0x0007,
		       &m68k_tst<byte_size, byte_index_indirect>);
    eu.set_instruction(0x4a38, 0x0000,
		       &m68k_tst<byte_size, byte_abs_short>);
    eu.set_instruction(0x4a39, 0x0000,
		       &m68k_tst<byte_size, byte_abs_long>);
    eu.set_instruction(0x4a40, 0x0007,
		       &m68k_tst<word_size, word_d_register>);
    eu.set_instruction(0x4a50, 0x0007,
		       &m68k_tst<word_size, word_indirect>);
    eu.set_instruction(0x4a58, 0x0007,
		       &m68k_tst<word_size, word_postinc_indirect>);
    eu.set_instruction(0x4a60, 0x0007,
		       &m68k_tst<word_size, word_predec_indirect>);
    eu.set_instruction(0x4a68, 0x0007,
		       &m68k_tst<word_size, word_disp_indirect>);
    eu.set_instruction(0x4a70, 0x0007,
		       &m68k_tst<word_size, word_index_indirect>);
    eu.set_instruction(0x4a78, 0x0000,
		       &m68k_tst<word_size, word_abs_short>);
    eu.set_instruction(0x4a79, 0x0000,
		       &m68k_tst<word_size, word_abs_long>);
    eu.set_instruction(0x4a80, 0x0007,
		       &m68k_tst<long_word_size, long_word_d_register>);
    eu.set_instruction(0x4a90, 0x0007,
		       &m68k_tst<long_word_size, long_word_indirect>);
    eu.set_instruction(0x4a98, 0x0007,
		       &m68k_tst<long_word_size, long_word_postinc_indirect>);
    eu.set_instruction(0x4aa0, 0x0007,
		       &m68k_tst<long_word_size, long_word_predec_indirect>);
    eu.set_instruction(0x4aa8, 0x0007,
		       &m68k_tst<long_word_size, long_word_disp_indirect>);
    eu.set_instruction(0x4ab0, 0x0007,
		       &m68k_tst<long_word_size, long_word_index_indirect>);
    eu.set_instruction(0x4ab8, 0x0000,
		       &m68k_tst<long_word_size, long_word_abs_short>);
    eu.set_instruction(0x4ab9, 0x0000,
		       &m68k_tst<long_word_size, long_word_abs_long>);
    eu.set_instruction(0x4c90, 0x0007,
		       &m68k_movem_m_r<word_size, word_indirect>);
    eu.set_instruction(0x4c98, 0x0007,
		       &m68k_movem_postinc_r<word_size>);
    eu.set_instruction(0x4ca8, 0x0007,
		       &m68k_movem_m_r<word_size, word_disp_indirect>);
    eu.set_instruction(0x4cb0, 0x0007,
		       &m68k_movem_m_r<word_size, word_index_indirect>);
    eu.set_instruction(0x4cb8, 0x0000,
		       &m68k_movem_m_r<word_size, word_abs_short>);
    eu.set_instruction(0x4cb9, 0x0000,
		       &m68k_movem_m_r<word_size, word_abs_long>);
    eu.set_instruction(0x4cba, 0x0000,
		       &m68k_movem_m_r<word_size, word_disp_pc_indirect>);
    eu.set_instruction(0x4cbb, 0x0000,
		       &m68k_movem_m_r<word_size, word_index_pc_indirect>);
    eu.set_instruction(0x4cd0, 0x0007,
		       &m68k_movem_m_r<long_word_size, long_word_indirect>);
    eu.set_instruction(0x4cd8, 0x0007,
		       &m68k_movem_postinc_r<long_word_size>);
    eu.set_instruction(0x4ce8, 0x0007,
		       &m68k_movem_m_r<long_word_size, long_word_disp_indirect>);
    eu.set_instruction(0x4cf0, 0x0007,
		       &m68k_movem_m_r<long_word_size, long_word_index_indirect>);
    eu.set_instruction(0x4cf8, 0x0000,
		       &m68k_movem_m_r<long_word_size, long_word_abs_short>);
    eu.set_instruction(0x4cf9, 0x0000,
		       &m68k_movem_m_r<long_word_size, long_word_abs_long>);
    eu.set_instruction(0x4cfa, 0x0000,
		       &m68k_movem_m_r<long_word_size, long_word_disp_pc_indirect>);
    eu.set_instruction(0x4cfb, 0x0000,
		       &m68k_movem_m_r<long_word_size, long_word_index_pc_indirect>);
    eu.set_instruction(0x4e50, 0x0007,
		       &m68k_link);
    eu.set_instruction(0x4e58, 0x0007,
		       &m68k_unlk);
    eu.set_instruction(0x4e60, 0x0007, &m68k_move_to_usp);
    eu.set_instruction(0x4e68, 0x0007, &m68k_move_from_usp);
    eu.set_instruction(0x4e71, 0x0000, &m68k_nop);
    eu.set_instruction(0x4e73, 0x0000, &m68k_rte);
    eu.set_instruction(0x4e75, 0x0000,
		       &m68k_rts);
    eu.set_instruction(0x4e90, 0x0007,
		       &m68k_jsr<word_indirect>);
    eu.set_instruction(0x4ea8, 0x0007,
		       &m68k_jsr<word_disp_indirect>);
    eu.set_instruction(0x4eb0, 0x0007,
		       &m68k_jsr<word_index_indirect>);
    eu.set_instruction(0x4eb8, 0x0000,
		       &m68k_jsr<word_abs_short>);
    eu.set_instruction(0x4eb9, 0x0000,
		       &m68k_jsr<word_abs_long>);
    eu.set_instruction(0x4eba, 0x0000,
		       &m68k_jsr<word_disp_pc_indirect>);
    eu.set_instruction(0x4ebb, 0x0000,
		       &m68k_jsr<word_index_pc_indirect>);
    eu.set_instruction(0x4ed0, 0x0007,
		       &m68k_jmp<word_indirect>);
    eu.set_instruction(0x4ee8, 0x0007,
		       &m68k_jmp<word_disp_indirect>);
    eu.set_instruction(0x4ef0, 0x0007,
		       &m68k_jmp<word_index_indirect>);
    eu.set_instruction(0x4ef8, 0x0000,
		       &m68k_jmp<word_abs_short>);
    eu.set_instruction(0x4ef9, 0x0000,
		       &m68k_jmp<word_abs_long>);
    eu.set_instruction(0x4efa, 0x0000,
		       &m68k_jmp<word_disp_pc_indirect>);
    eu.set_instruction(0x4efb, 0x0000,
		       &m68k_jmp<word_index_pc_indirect>);
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
    eu.set_instruction(0x50c0, 0x0007,
		       &m68k_s<t, byte_d_register>);
    eu.set_instruction(0x50c8, 0x0007,
		       &m68k_db<t>);
    eu.set_instruction(0x50d0, 0x0007,
		       &m68k_s<t, byte_indirect>);
    eu.set_instruction(0x50d8, 0x0007,
		       &m68k_s<t, byte_postinc_indirect>);
    eu.set_instruction(0x50e0, 0x0007,
		       &m68k_s<t, byte_predec_indirect>);
    eu.set_instruction(0x50e8, 0x0007,
		       &m68k_s<t, byte_disp_indirect>);
    eu.set_instruction(0x50f0, 0x0007,
		       &m68k_s<t, byte_index_indirect>);
    eu.set_instruction(0x50f8, 0x0000,
		       &m68k_s<t, byte_abs_short>);
    eu.set_instruction(0x50f9, 0x0000,
		       &m68k_s<t, byte_abs_long>);
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
    eu.set_instruction(0x51c0, 0x0007,
		       &m68k_s<f, byte_d_register>);
    eu.set_instruction(0x51c8, 0x0007,
		       &m68k_db<f>);
    eu.set_instruction(0x51d0, 0x0007,
		       &m68k_s<f, byte_indirect>);
    eu.set_instruction(0x51d8, 0x0007,
		       &m68k_s<f, byte_postinc_indirect>);
    eu.set_instruction(0x51e0, 0x0007,
		       &m68k_s<f, byte_predec_indirect>);
    eu.set_instruction(0x51e8, 0x0007,
		       &m68k_s<f, byte_disp_indirect>);
    eu.set_instruction(0x51f0, 0x0007,
		       &m68k_s<f, byte_index_indirect>);
    eu.set_instruction(0x51f8, 0x0000,
		       &m68k_s<f, byte_abs_short>);
    eu.set_instruction(0x51f9, 0x0000,
		       &m68k_s<f, byte_abs_long>);
    eu.set_instruction(0x52c0, 0x0007,
		       &m68k_s<hi, byte_d_register>);
    eu.set_instruction(0x52c8, 0x0007,
		       &m68k_db<hi>);
    eu.set_instruction(0x52d0, 0x0007,
		       &m68k_s<hi, byte_indirect>);
    eu.set_instruction(0x52d8, 0x0007,
		       &m68k_s<hi, byte_postinc_indirect>);
    eu.set_instruction(0x52e0, 0x0007,
		       &m68k_s<hi, byte_predec_indirect>);
    eu.set_instruction(0x52e8, 0x0007,
		       &m68k_s<hi, byte_disp_indirect>);
    eu.set_instruction(0x52f0, 0x0007,
		       &m68k_s<hi, byte_index_indirect>);
    eu.set_instruction(0x52f8, 0x0000,
		       &m68k_s<hi, byte_abs_short>);
    eu.set_instruction(0x52f9, 0x0000,
		       &m68k_s<hi, byte_abs_long>);
    eu.set_instruction(0x53c0, 0x0007,
		       &m68k_s<ls, byte_d_register>);
    eu.set_instruction(0x53c8, 0x0007,
		       &m68k_db<ls>);
    eu.set_instruction(0x53d0, 0x0007,
		       &m68k_s<ls, byte_indirect>);
    eu.set_instruction(0x53d8, 0x0007,
		       &m68k_s<ls, byte_postinc_indirect>);
    eu.set_instruction(0x53e0, 0x0007,
		       &m68k_s<ls, byte_predec_indirect>);
    eu.set_instruction(0x53e8, 0x0007,
		       &m68k_s<ls, byte_disp_indirect>);
    eu.set_instruction(0x53f0, 0x0007,
		       &m68k_s<ls, byte_index_indirect>);
    eu.set_instruction(0x53f8, 0x0000,
		       &m68k_s<ls, byte_abs_short>);
    eu.set_instruction(0x53f9, 0x0000,
		       &m68k_s<ls, byte_abs_long>);
    eu.set_instruction(0x54c0, 0x0007,
		       &m68k_s<cc, byte_d_register>);
    eu.set_instruction(0x54c8, 0x0007,
		       &m68k_db<cc>);
    eu.set_instruction(0x54d0, 0x0007,
		       &m68k_s<cc, byte_indirect>);
    eu.set_instruction(0x54d8, 0x0007,
		       &m68k_s<cc, byte_postinc_indirect>);
    eu.set_instruction(0x54e0, 0x0007,
		       &m68k_s<cc, byte_predec_indirect>);
    eu.set_instruction(0x54e8, 0x0007,
		       &m68k_s<cc, byte_disp_indirect>);
    eu.set_instruction(0x54f0, 0x0007,
		       &m68k_s<cc, byte_index_indirect>);
    eu.set_instruction(0x54f8, 0x0000,
		       &m68k_s<cc, byte_abs_short>);
    eu.set_instruction(0x54f9, 0x0000,
		       &m68k_s<cc, byte_abs_long>);
    eu.set_instruction(0x55c0, 0x0007,
		       &m68k_s<cs, byte_d_register>);
    eu.set_instruction(0x55c8, 0x0007,
		       &m68k_db<cs>);
    eu.set_instruction(0x55d0, 0x0007,
		       &m68k_s<cs, byte_indirect>);
    eu.set_instruction(0x55d8, 0x0007,
		       &m68k_s<cs, byte_postinc_indirect>);
    eu.set_instruction(0x55e0, 0x0007,
		       &m68k_s<cs, byte_predec_indirect>);
    eu.set_instruction(0x55e8, 0x0007,
		       &m68k_s<cs, byte_disp_indirect>);
    eu.set_instruction(0x55f0, 0x0007,
		       &m68k_s<cs, byte_index_indirect>);
    eu.set_instruction(0x55f8, 0x0000,
		       &m68k_s<cs, byte_abs_short>);
    eu.set_instruction(0x55f9, 0x0000,
		       &m68k_s<cs, byte_abs_long>);
    eu.set_instruction(0x56c0, 0x0007,
		       &m68k_s<ne, byte_d_register>);
    eu.set_instruction(0x56c8, 0x0007,
		       &m68k_db<ne>);
    eu.set_instruction(0x56d0, 0x0007,
		       &m68k_s<ne, byte_indirect>);
    eu.set_instruction(0x56d8, 0x0007,
		       &m68k_s<ne, byte_postinc_indirect>);
    eu.set_instruction(0x56e0, 0x0007,
		       &m68k_s<ne, byte_predec_indirect>);
    eu.set_instruction(0x56e8, 0x0007,
		       &m68k_s<ne, byte_disp_indirect>);
    eu.set_instruction(0x56f0, 0x0007,
		       &m68k_s<ne, byte_index_indirect>);
    eu.set_instruction(0x56f8, 0x0000,
		       &m68k_s<ne, byte_abs_short>);
    eu.set_instruction(0x56f9, 0x0000,
		       &m68k_s<ne, byte_abs_long>);
    eu.set_instruction(0x57c0, 0x0007,
		       &m68k_s<eq, byte_d_register>);
    eu.set_instruction(0x57c8, 0x0007,
		       &m68k_db<eq>);
    eu.set_instruction(0x57d0, 0x0007,
		       &m68k_s<eq, byte_indirect>);
    eu.set_instruction(0x57d8, 0x0007,
		       &m68k_s<eq, byte_postinc_indirect>);
    eu.set_instruction(0x57e0, 0x0007,
		       &m68k_s<eq, byte_predec_indirect>);
    eu.set_instruction(0x57e8, 0x0007,
		       &m68k_s<eq, byte_disp_indirect>);
    eu.set_instruction(0x57f0, 0x0007,
		       &m68k_s<eq, byte_index_indirect>);
    eu.set_instruction(0x57f8, 0x0000,
		       &m68k_s<eq, byte_abs_short>);
    eu.set_instruction(0x57f9, 0x0000,
		       &m68k_s<eq, byte_abs_long>);
    eu.set_instruction(0x5ac0, 0x0007,
		       &m68k_s<pl, byte_d_register>);
    eu.set_instruction(0x5ac8, 0x0007,
		       &m68k_db<pl>);
    eu.set_instruction(0x5ad0, 0x0007,
		       &m68k_s<pl, byte_indirect>);
    eu.set_instruction(0x5ad8, 0x0007,
		       &m68k_s<pl, byte_postinc_indirect>);
    eu.set_instruction(0x5ae0, 0x0007,
		       &m68k_s<pl, byte_predec_indirect>);
    eu.set_instruction(0x5ae8, 0x0007,
		       &m68k_s<pl, byte_disp_indirect>);
    eu.set_instruction(0x5af0, 0x0007,
		       &m68k_s<pl, byte_index_indirect>);
    eu.set_instruction(0x5af8, 0x0000,
		       &m68k_s<pl, byte_abs_short>);
    eu.set_instruction(0x5af9, 0x0000,
		       &m68k_s<pl, byte_abs_long>);
    eu.set_instruction(0x5bc0, 0x0007,
		       &m68k_s<mi, byte_d_register>);
    eu.set_instruction(0x5bc8, 0x0007,
		       &m68k_db<mi>);
    eu.set_instruction(0x5bd0, 0x0007,
		       &m68k_s<mi, byte_indirect>);
    eu.set_instruction(0x5bd8, 0x0007,
		       &m68k_s<mi, byte_postinc_indirect>);
    eu.set_instruction(0x5be0, 0x0007,
		       &m68k_s<mi, byte_predec_indirect>);
    eu.set_instruction(0x5be8, 0x0007,
		       &m68k_s<mi, byte_disp_indirect>);
    eu.set_instruction(0x5bf0, 0x0007,
		       &m68k_s<mi, byte_index_indirect>);
    eu.set_instruction(0x5bf8, 0x0000,
		       &m68k_s<mi, byte_abs_short>);
    eu.set_instruction(0x5bf9, 0x0000,
		       &m68k_s<mi, byte_abs_long>);
    eu.set_instruction(0x5cc0, 0x0007,
		       &m68k_s<ge, byte_d_register>);
    eu.set_instruction(0x5cc8, 0x0007,
		       &m68k_db<ge>);
    eu.set_instruction(0x5cd0, 0x0007,
		       &m68k_s<ge, byte_indirect>);
    eu.set_instruction(0x5cd8, 0x0007,
		       &m68k_s<ge, byte_postinc_indirect>);
    eu.set_instruction(0x5ce0, 0x0007,
		       &m68k_s<ge, byte_predec_indirect>);
    eu.set_instruction(0x5ce8, 0x0007,
		       &m68k_s<ge, byte_disp_indirect>);
    eu.set_instruction(0x5cf0, 0x0007,
		       &m68k_s<ge, byte_index_indirect>);
    eu.set_instruction(0x5cf8, 0x0000,
		       &m68k_s<ge, byte_abs_short>);
    eu.set_instruction(0x5cf9, 0x0000,
		       &m68k_s<ge, byte_abs_long>);
    eu.set_instruction(0x5dc0, 0x0007,
		       &m68k_s<lt, byte_d_register>);
    eu.set_instruction(0x5dc8, 0x0007,
		       &m68k_db<lt>);
    eu.set_instruction(0x5dd0, 0x0007,
		       &m68k_s<lt, byte_indirect>);
    eu.set_instruction(0x5dd8, 0x0007,
		       &m68k_s<lt, byte_postinc_indirect>);
    eu.set_instruction(0x5de0, 0x0007,
		       &m68k_s<lt, byte_predec_indirect>);
    eu.set_instruction(0x5de8, 0x0007,
		       &m68k_s<lt, byte_disp_indirect>);
    eu.set_instruction(0x5df0, 0x0007,
		       &m68k_s<lt, byte_index_indirect>);
    eu.set_instruction(0x5df8, 0x0000,
		       &m68k_s<lt, byte_abs_short>);
    eu.set_instruction(0x5df9, 0x0000,
		       &m68k_s<lt, byte_abs_long>);
    eu.set_instruction(0x5ec0, 0x0007,
		       &m68k_s<gt, byte_d_register>);
    eu.set_instruction(0x5ec8, 0x0007,
		       &m68k_db<gt>);
    eu.set_instruction(0x5ed0, 0x0007,
		       &m68k_s<gt, byte_indirect>);
    eu.set_instruction(0x5ed8, 0x0007,
		       &m68k_s<gt, byte_postinc_indirect>);
    eu.set_instruction(0x5ee0, 0x0007,
		       &m68k_s<gt, byte_predec_indirect>);
    eu.set_instruction(0x5ee8, 0x0007,
		       &m68k_s<gt, byte_disp_indirect>);
    eu.set_instruction(0x5ef0, 0x0007,
		       &m68k_s<gt, byte_index_indirect>);
    eu.set_instruction(0x5ef8, 0x0000,
		       &m68k_s<gt, byte_abs_short>);
    eu.set_instruction(0x5ef9, 0x0000,
		       &m68k_s<gt, byte_abs_long>);
    eu.set_instruction(0x5fc0, 0x0007,
		       &m68k_s<le, byte_d_register>);
    eu.set_instruction(0x5fc8, 0x0007,
		       &m68k_db<le>);
    eu.set_instruction(0x5fd0, 0x0007,
		       &m68k_s<le, byte_indirect>);
    eu.set_instruction(0x5fd8, 0x0007,
		       &m68k_s<le, byte_postinc_indirect>);
    eu.set_instruction(0x5fe0, 0x0007,
		       &m68k_s<le, byte_predec_indirect>);
    eu.set_instruction(0x5fe8, 0x0007,
		       &m68k_s<le, byte_disp_indirect>);
    eu.set_instruction(0x5ff0, 0x0007,
		       &m68k_s<le, byte_index_indirect>);
    eu.set_instruction(0x5ff8, 0x0000,
		       &m68k_s<le, byte_abs_short>);
    eu.set_instruction(0x5ff9, 0x0000,
		       &m68k_s<le, byte_abs_long>);
    eu.set_instruction(0x6000, 0x00ff,
		       &m68k_bra);
    eu.set_instruction(0x6100, 0x00ff,
		       &m68k_bsr);
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
    eu.set_instruction(0x7000, 0x0eff,
		       &m68k_moveq);
    eu.set_instruction(0x8000, 0x0e07,
		       &m68k_or<byte_size, byte_d_register>);
    eu.set_instruction(0x8010, 0x0e07,
		       &m68k_or<byte_size, byte_indirect>);
    eu.set_instruction(0x8018, 0x0e07,
		       &m68k_or<byte_size, byte_postinc_indirect>);
    eu.set_instruction(0x8020, 0x0e07,
		       &m68k_or<byte_size, byte_predec_indirect>);
    eu.set_instruction(0x8028, 0x0e07,
		       &m68k_or<byte_size, byte_disp_indirect>);
    eu.set_instruction(0x8030, 0x0e07,
		       &m68k_or<byte_size, byte_index_indirect>);
    eu.set_instruction(0x8038, 0x0e00,
		       &m68k_or<byte_size, byte_abs_short>);
    eu.set_instruction(0x8039, 0x0e00,
		       &m68k_or<byte_size, byte_abs_long>);
    eu.set_instruction(0x803a, 0x0e00,
		       &m68k_or<byte_size, byte_disp_pc_indirect>);
    eu.set_instruction(0x803b, 0x0e00,
		       &m68k_or<byte_size, byte_index_pc_indirect>);
    eu.set_instruction(0x803c, 0x0e00,
		       &m68k_or<byte_size, byte_immediate>);
    eu.set_instruction(0x8040, 0x0e07,
		       &m68k_or<word_size, word_d_register>);
    eu.set_instruction(0x8050, 0x0e07,
		       &m68k_or<word_size, word_indirect>);
    eu.set_instruction(0x8058, 0x0e07,
		       &m68k_or<word_size, word_postinc_indirect>);
    eu.set_instruction(0x8060, 0x0e07,
		       &m68k_or<word_size, word_predec_indirect>);
    eu.set_instruction(0x8068, 0x0e07,
		       &m68k_or<word_size, word_disp_indirect>);
    eu.set_instruction(0x8070, 0x0e07,
		       &m68k_or<word_size, word_index_indirect>);
    eu.set_instruction(0x8078, 0x0e00,
		       &m68k_or<word_size, word_abs_short>);
    eu.set_instruction(0x8079, 0x0e00,
		       &m68k_or<word_size, word_abs_long>);
    eu.set_instruction(0x807a, 0x0e00,
		       &m68k_or<word_size, word_disp_pc_indirect>);
    eu.set_instruction(0x807b, 0x0e00,
		       &m68k_or<word_size, word_index_pc_indirect>);
    eu.set_instruction(0x807c, 0x0e00,
		       &m68k_or<word_size, word_immediate>);
    eu.set_instruction(0x8080, 0x0e07,
		       &m68k_or<long_word_size, long_word_d_register>);
    eu.set_instruction(0x8090, 0x0e07,
		       &m68k_or<long_word_size, long_word_indirect>);
    eu.set_instruction(0x8098, 0x0e07,
		       &m68k_or<long_word_size, long_word_postinc_indirect>);
    eu.set_instruction(0x80a0, 0x0e07,
		       &m68k_or<long_word_size, long_word_predec_indirect>);
    eu.set_instruction(0x80a8, 0x0e07,
		       &m68k_or<long_word_size, long_word_disp_indirect>);
    eu.set_instruction(0x80b0, 0x0e07,
		       &m68k_or<long_word_size, long_word_index_indirect>);
    eu.set_instruction(0x80b8, 0x0e00,
		       &m68k_or<long_word_size, long_word_abs_short>);
    eu.set_instruction(0x80b9, 0x0e00,
		       &m68k_or<long_word_size, long_word_abs_long>);
    eu.set_instruction(0x80ba, 0x0e00,
		       &m68k_or<long_word_size, long_word_disp_pc_indirect>);
    eu.set_instruction(0x80bb, 0x0e00,
		       &m68k_or<long_word_size, long_word_index_pc_indirect>);
    eu.set_instruction(0x80bc, 0x0e00,
		       &m68k_or<long_word_size, long_word_immediate>);
    eu.set_instruction(0x80c0, 0x0e07,
		       &m68k_divu<word_d_register>);
    eu.set_instruction(0x80d0, 0x0e07,
		       &m68k_divu<word_indirect>);
    eu.set_instruction(0x80d8, 0x0e07,
		       &m68k_divu<word_postinc_indirect>);
    eu.set_instruction(0x80e0, 0x0e07,
		       &m68k_divu<word_predec_indirect>);
    eu.set_instruction(0x80e8, 0x0e07,
		       &m68k_divu<word_disp_indirect>);
    eu.set_instruction(0x80f0, 0x0e07,
		       &m68k_divu<word_index_indirect>);
    eu.set_instruction(0x80f8, 0x0e00,
		       &m68k_divu<word_abs_short>);
    eu.set_instruction(0x80f9, 0x0e00,
		       &m68k_divu<word_abs_long>);
    eu.set_instruction(0x80fa, 0x0e00,
		       &m68k_divu<word_disp_pc_indirect>);
    eu.set_instruction(0x80fb, 0x0e00,
		       &m68k_divu<word_index_pc_indirect>);
    eu.set_instruction(0x80fc, 0x0e00,
		       &m68k_divu<word_immediate>);
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
    eu.set_instruction(0x9000, 0x0e07,
		       &m68k_sub<byte_size, byte_d_register>);
    eu.set_instruction(0x9010, 0x0e07,
		       &m68k_sub<byte_size, byte_indirect>);
    eu.set_instruction(0x9018, 0x0e07,
		       &m68k_sub<byte_size, byte_postinc_indirect>);
    eu.set_instruction(0x9020, 0x0e07,
		       &m68k_sub<byte_size, byte_predec_indirect>);
    eu.set_instruction(0x9028, 0x0e07,
		       &m68k_sub<byte_size, byte_disp_indirect>);
    eu.set_instruction(0x9030, 0x0e07,
		       &m68k_sub<byte_size, byte_index_indirect>);
    eu.set_instruction(0x9038, 0x0e00,
		       &m68k_sub<byte_size, byte_abs_short>);
    eu.set_instruction(0x9039, 0x0e00,
		       &m68k_sub<byte_size, byte_abs_long>);
    eu.set_instruction(0x903a, 0x0e00,
		       &m68k_sub<byte_size, byte_disp_pc_indirect>);
    eu.set_instruction(0x903b, 0x0e00,
		       &m68k_sub<byte_size, byte_index_pc_indirect>);
    eu.set_instruction(0x903c, 0x0e00,
		       &m68k_sub<byte_size, byte_immediate>);
    eu.set_instruction(0x9040, 0x0e07,
		       &m68k_sub<word_size, word_d_register>);
    eu.set_instruction(0x9048, 0x0e07,
		       &m68k_sub<word_size, word_a_register>);
    eu.set_instruction(0x9050, 0x0e07,
		       &m68k_sub<word_size, word_indirect>);
    eu.set_instruction(0x9058, 0x0e07,
		       &m68k_sub<word_size, word_postinc_indirect>);
    eu.set_instruction(0x9060, 0x0e07,
		       &m68k_sub<word_size, word_predec_indirect>);
    eu.set_instruction(0x9068, 0x0e07,
		       &m68k_sub<word_size, word_disp_indirect>);
    eu.set_instruction(0x9070, 0x0e07,
		       &m68k_sub<word_size, word_index_indirect>);
    eu.set_instruction(0x9078, 0x0e00,
		       &m68k_sub<word_size, word_abs_short>);
    eu.set_instruction(0x9079, 0x0e00,
		       &m68k_sub<word_size, word_abs_long>);
    eu.set_instruction(0x907a, 0x0e00,
		       &m68k_sub<word_size, word_disp_pc_indirect>);
    eu.set_instruction(0x907b, 0x0e00,
		       &m68k_sub<word_size, word_index_pc_indirect>);
    eu.set_instruction(0x907c, 0x0e00,
		       &m68k_sub<word_size, word_immediate>);
    eu.set_instruction(0x9080, 0x0e07,
		       &m68k_sub<long_word_size, long_word_d_register>);
    eu.set_instruction(0x9088, 0x0e07,
		       &m68k_sub<long_word_size, long_word_a_register>);
    eu.set_instruction(0x9090, 0x0e07,
		       &m68k_sub<long_word_size, long_word_indirect>);
    eu.set_instruction(0x9098, 0x0e07,
		       &m68k_sub<long_word_size, long_word_postinc_indirect>);
    eu.set_instruction(0x90a0, 0x0e07,
		       &m68k_sub<long_word_size, long_word_predec_indirect>);
    eu.set_instruction(0x90a8, 0x0e07,
		       &m68k_sub<long_word_size, long_word_disp_indirect>);
    eu.set_instruction(0x90b0, 0x0e07,
		       &m68k_sub<long_word_size, long_word_index_indirect>);
    eu.set_instruction(0x90b8, 0x0e00,
		       &m68k_sub<long_word_size, long_word_abs_short>);
    eu.set_instruction(0x90b9, 0x0e00,
		       &m68k_sub<long_word_size, long_word_abs_long>);
    eu.set_instruction(0x90ba, 0x0e00,
		       &m68k_sub<long_word_size, long_word_disp_pc_indirect>);
    eu.set_instruction(0x90bb, 0x0e00,
		       &m68k_sub<long_word_size, long_word_index_pc_indirect>);
    eu.set_instruction(0x90bc, 0x0e00,
		       &m68k_sub<long_word_size, long_word_immediate>);
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
    eu.set_instruction(0xb0c0, 0x0e07,
		       &m68k_cmpa<word_size, word_d_register>);
    eu.set_instruction(0xb0c8, 0x0e07,
		       &m68k_cmpa<word_size, word_a_register>);
    eu.set_instruction(0xb0d0, 0x0e07,
		       &m68k_cmpa<word_size, word_indirect>);
    eu.set_instruction(0xb0d8, 0x0e07,
		       &m68k_cmpa<word_size, word_postinc_indirect>);
    eu.set_instruction(0xb0e0, 0x0e07,
		       &m68k_cmpa<word_size, word_predec_indirect>);
    eu.set_instruction(0xb0e8, 0x0e07,
		       &m68k_cmpa<word_size, word_disp_indirect>);
    eu.set_instruction(0xb0f0, 0x0e07,
		       &m68k_cmpa<word_size, word_index_indirect>);  
    eu.set_instruction(0xb0f8, 0x0e00,
		       &m68k_cmpa<word_size, word_abs_short>);
    eu.set_instruction(0xb0f9, 0x0e00,
		       &m68k_cmpa<word_size, word_abs_long>);
    eu.set_instruction(0xb0fa, 0x0e00,
		       &m68k_cmpa<word_size, word_disp_pc_indirect>);
    eu.set_instruction(0xb0fb, 0x0e00,
		       &m68k_cmpa<word_size, word_index_pc_indirect>);
    eu.set_instruction(0xb0fc, 0x0e00,
		       &m68k_cmpa<word_size, word_immediate>);
    eu.set_instruction(0xb100, 0x0e07,
		       &m68k_eor_m<byte_size, byte_d_register>);
    eu.set_instruction(0xb108, 0x0e07,
		       &m68k_cmpm<byte_size>);
    eu.set_instruction(0xb110, 0x0e07,
		       &m68k_eor_m<byte_size, byte_indirect>);
    eu.set_instruction(0xb118, 0x0e07,
		       &m68k_eor_m<byte_size, byte_postinc_indirect>);
    eu.set_instruction(0xb120, 0x0e07,
		       &m68k_eor_m<byte_size, byte_predec_indirect>);
    eu.set_instruction(0xb128, 0x0e07,
		       &m68k_eor_m<byte_size, byte_disp_indirect>);
    eu.set_instruction(0xb130, 0x0e07,
		       &m68k_eor_m<byte_size, byte_index_indirect>);
    eu.set_instruction(0xb138, 0x0e00,
		       &m68k_eor_m<byte_size, byte_abs_short>);
    eu.set_instruction(0xb139, 0x0e00,
		       &m68k_eor_m<byte_size, byte_abs_long>);
    eu.set_instruction(0xb140, 0x0e07,
		       &m68k_eor_m<word_size, word_d_register>);
    eu.set_instruction(0xb148, 0x0e07,
		       &m68k_cmpm<word_size>);
    eu.set_instruction(0xb150, 0x0e07,
		       &m68k_eor_m<word_size, word_indirect>);
    eu.set_instruction(0xb158, 0x0e07,
		       &m68k_eor_m<word_size, word_postinc_indirect>);
    eu.set_instruction(0xb160, 0x0e07,
		       &m68k_eor_m<word_size, word_predec_indirect>);
    eu.set_instruction(0xb168, 0x0e07,
		       &m68k_eor_m<word_size, word_disp_indirect>);
    eu.set_instruction(0xb170, 0x0e07,
		       &m68k_eor_m<word_size, word_index_indirect>);
    eu.set_instruction(0xb178, 0x0e00,
		       &m68k_eor_m<word_size, word_abs_short>);
    eu.set_instruction(0xb179, 0x0e00,
		       &m68k_eor_m<word_size, word_abs_long>);
    eu.set_instruction(0xb180, 0x0e07,
		       &m68k_eor_m<long_word_size, long_word_d_register>);
    eu.set_instruction(0xb188, 0x0e07,
		       &m68k_cmpm<long_word_size>);
    eu.set_instruction(0xb190, 0x0e07,
		       &m68k_eor_m<long_word_size, long_word_indirect>);
    eu.set_instruction(0xb198, 0x0e07,
		       &m68k_eor_m<long_word_size, long_word_postinc_indirect>);
    eu.set_instruction(0xb1a0, 0x0e07,
		       &m68k_eor_m<long_word_size, long_word_predec_indirect>);
    eu.set_instruction(0xb1a8, 0x0e07,
		       &m68k_eor_m<long_word_size, long_word_disp_indirect>);
    eu.set_instruction(0xb1b0, 0x0e07,
		       &m68k_eor_m<long_word_size, long_word_index_indirect>);
    eu.set_instruction(0xb1b8, 0x0e00,
		       &m68k_eor_m<long_word_size, long_word_abs_short>);
    eu.set_instruction(0xb1b9, 0x0e00,
		       &m68k_eor_m<long_word_size, long_word_abs_long>);
    eu.set_instruction(0xb1c0, 0x0e07,
		       &m68k_cmpa<long_word_size, long_word_d_register>);
    eu.set_instruction(0xb1c8, 0x0e07,
		       &m68k_cmpa<long_word_size, long_word_a_register>);
    eu.set_instruction(0xb1d0, 0x0e07,
		       &m68k_cmpa<long_word_size, long_word_indirect>);
    eu.set_instruction(0xb1d8, 0x0e07,
		       &m68k_cmpa<long_word_size, long_word_postinc_indirect>);
    eu.set_instruction(0xb1e0, 0x0e07,
		       &m68k_cmpa<long_word_size, long_word_predec_indirect>);
    eu.set_instruction(0xb1e8, 0x0e07,
		       &m68k_cmpa<long_word_size, long_word_disp_indirect>);
    eu.set_instruction(0xb1f0, 0x0e07,
		       &m68k_cmpa<long_word_size, long_word_index_indirect>);
    eu.set_instruction(0xb1f8, 0x0e00,
		       &m68k_cmpa<long_word_size, long_word_abs_short>);
    eu.set_instruction(0xb1f9, 0x0e00,
		       &m68k_cmpa<long_word_size, long_word_abs_long>);
    eu.set_instruction(0xb1fa, 0x0e00,
		       &m68k_cmpa<long_word_size, long_word_disp_pc_indirect>);
    eu.set_instruction(0xb1fb, 0x0e00,
		       &m68k_cmpa<long_word_size, long_word_index_pc_indirect>);
    eu.set_instruction(0xb1fc, 0x0e00,
		       &m68k_cmpa<long_word_size, long_word_immediate>);
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
    eu.set_instruction(0xc0c0, 0x0e07,
		       &m68k_mulu<word_d_register>);
    eu.set_instruction(0xc0d0, 0x0e07,
		       &m68k_mulu<word_indirect>);
    eu.set_instruction(0xc0d8, 0x0e07,
		       &m68k_mulu<word_postinc_indirect>);
    eu.set_instruction(0xc0e0, 0x0e07,
		       &m68k_mulu<word_predec_indirect>);
    eu.set_instruction(0xc0e8, 0x0e07,
		       &m68k_mulu<word_disp_indirect>);
    eu.set_instruction(0xc0f0, 0x0e07,
		       &m68k_mulu<word_index_indirect>);
    eu.set_instruction(0xc0f8, 0x0e00,
		       &m68k_mulu<word_abs_short>);
    eu.set_instruction(0xc0f9, 0x0e00,
		       &m68k_mulu<word_abs_long>);
    eu.set_instruction(0xc0fa, 0x0e00,
		       &m68k_mulu<word_disp_pc_indirect>);
    eu.set_instruction(0xc0fb, 0x0e00,
		       &m68k_mulu<word_index_pc_indirect>);
    eu.set_instruction(0xc0fc, 0x0e00,
		       &m68k_mulu<word_immediate>);
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
    eu.set_instruction(0xc140, 0x0e07,
		       &m68k_exg_d_d);
    eu.set_instruction(0xc148, 0x0e07,
		       &m68k_exg_a_a);
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
    eu.set_instruction(0xc188, 0x0e07,
		       &m68k_exg_d_a);
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
    eu.set_instruction(0xc1c0, 0x0e07,
		       &m68k_muls<word_d_register>);
    eu.set_instruction(0xc1d0, 0x0e07,
		       &m68k_muls<word_indirect>);
    eu.set_instruction(0xc1d8, 0x0e07,
		       &m68k_muls<word_postinc_indirect>);
    eu.set_instruction(0xc1e0, 0x0e07,
		       &m68k_muls<word_predec_indirect>);
    eu.set_instruction(0xc1e8, 0x0e07,
		       &m68k_muls<word_disp_indirect>);
    eu.set_instruction(0xc1f0, 0x0e07,
		       &m68k_muls<word_index_indirect>);
    eu.set_instruction(0xc1f8, 0x0e00,
		       &m68k_muls<word_abs_short>);
    eu.set_instruction(0xc1f9, 0x0e00,
		       &m68k_muls<word_abs_long>);
    eu.set_instruction(0xc1fa, 0x0e00,
		       &m68k_muls<word_disp_pc_indirect>);
    eu.set_instruction(0xc1fb, 0x0e00,
		       &m68k_muls<word_index_pc_indirect>);
    eu.set_instruction(0xc1fc, 0x0e00,
		       &m68k_muls<word_immediate>);
    eu.set_instruction(0xd000, 0x0e07,
		       &m68k_add<byte_size, byte_d_register>);
    eu.set_instruction(0xd010, 0x0e07,
		       &m68k_add<byte_size, byte_indirect>);
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
		       &m68k_adda<long_word_size, long_word_index_pc_indirect>);
    eu.set_instruction(0xd1fc, 0x0e00,
		       &m68k_adda<long_word_size, long_word_immediate>);
    eu.set_instruction(0xe000, 0x0e07,
		       &m68k_asr_i<byte_size>);
    eu.set_instruction(0xe008, 0x0e07,
		       &m68k_lsr_i<byte_size>);
    eu.set_instruction(0xe010, 0x0e07,
		       &m68k_roxr_i<byte_size>);
    eu.set_instruction(0xe018, 0x0e07,
		       &m68k_ror_i<byte_size>);
    eu.set_instruction(0xe020, 0x0e07,
		       &m68k_asr_r<byte_size>);
    eu.set_instruction(0xe028, 0x0e07,
		       &m68k_lsr_r<byte_size>);
    eu.set_instruction(0xe040, 0x0e07,
		       &m68k_asr_i<word_size>);
    eu.set_instruction(0xe048, 0x0e07,
		       &m68k_lsr_i<word_size>);
    eu.set_instruction(0xe050, 0x0e07,
		       &m68k_roxr_i<word_size>);
    eu.set_instruction(0xe058, 0x0e07,
		       &m68k_ror_i<word_size>);
    eu.set_instruction(0xe060, 0x0e07,
		       &m68k_asr_r<word_size>);
    eu.set_instruction(0xe068, 0x0e07,
		       &m68k_lsr_r<word_size>);
    eu.set_instruction(0xe080, 0x0e07,
		       &m68k_asr_i<long_word_size>);
    eu.set_instruction(0xe088, 0x0e07,
		       &m68k_lsr_i<long_word_size>);
    eu.set_instruction(0xe090, 0x0e07,
		       &m68k_roxr_i<long_word_size>);
    eu.set_instruction(0xe098, 0x0e07,
		       &m68k_ror_i<long_word_size>);
    eu.set_instruction(0xe0a0, 0x0e07,
		       &m68k_asr_r<long_word_size>);
    eu.set_instruction(0xe0a8, 0x0e07,
		       &m68k_lsr_r<long_word_size>);
    eu.set_instruction(0xe100, 0x0e07,
		       &m68k_asl_i<byte_size>);
    eu.set_instruction(0xe108, 0x0e07,
		       &m68k_lsl_i<byte_size>);
    eu.set_instruction(0xe118, 0x0e07,
		       &m68k_rol_i<byte_size>);
    eu.set_instruction(0xe120, 0x0e07,
		       &m68k_asl_r<byte_size>);
    eu.set_instruction(0xe128, 0x0e07,
		       &m68k_lsl_r<byte_size>);
    eu.set_instruction(0xe138, 0x0e07,
		       &m68k_rol_r<byte_size>);
    eu.set_instruction(0xe140, 0x0e07,
		       &m68k_asl_i<word_size>);
    eu.set_instruction(0xe148, 0x0e07,
		       &m68k_lsl_i<word_size>);
    eu.set_instruction(0xe158, 0x0e07,
		       &m68k_rol_i<word_size>);
    eu.set_instruction(0xe160, 0x0e07,
		       &m68k_asl_r<word_size>);
    eu.set_instruction(0xe168, 0x0e07,
		       &m68k_lsl_r<word_size>);
    eu.set_instruction(0xe178, 0x0e07,
		       &m68k_rol_r<word_size>);
    eu.set_instruction(0xe180, 0x0e07,
		       &m68k_asl_i<long_word_size>);
    eu.set_instruction(0xe188, 0x0e07,
		       &m68k_lsl_i<long_word_size>);
    eu.set_instruction(0xe198, 0x0e07,
		       &m68k_rol_i<long_word_size>);
    eu.set_instruction(0xe1a0, 0x0e07,
		       &m68k_asl_r<long_word_size>);
    eu.set_instruction(0xe1a8, 0x0e07,
		       &m68k_lsl_r<long_word_size>);
    eu.set_instruction(0xe1b8, 0x0e07,
		       &m68k_rol_r<long_word_size>);
    eu.set_instruction(0xe2d0,      7,
		       &m68k_lsr_m<word_indirect>);
    eu.set_instruction(0xe2d8,      7,
		       &m68k_lsr_m<word_postinc_indirect>);
    eu.set_instruction(0xe2e0,      7,
		       &m68k_lsr_m<word_predec_indirect>);
    eu.set_instruction(0xe2e8,      7,
		       &m68k_lsr_m<word_disp_indirect>);
    eu.set_instruction(0xe2f0,      7,
		       &m68k_lsr_m<word_index_indirect>);
    eu.set_instruction(0xe2f8,      0,
		       &m68k_lsr_m<word_abs_short>);
    eu.set_instruction(0xe2f9,      0,
		       &m68k_lsr_m<word_abs_long>);
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
