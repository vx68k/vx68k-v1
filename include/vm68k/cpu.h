/* -*-C++-*- */
/* vx68k - Virtual X68000
   Copyright (C) 1998-2000 Hypercore Software Design, Ltd.

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

#ifndef _VM68K_CPU_H
#define _VM68K_CPU_H 1

#include <vm68k/memory.h>

#include <vector>
#include <utility>
#include <cassert>

namespace vm68k
{
  using namespace std;

  /* Access methods for byte data.  */
  struct byte_size
  {
    typedef unsigned int uvalue_type;
    typedef int svalue_type;

    static size_t value_bit() {return 8;}
    static unsigned int value_mask() {return (1u << value_bit()) - 1;}
    static size_t value_size() {return 1;}
    static size_t aligned_value_size() {return 2;}
    static int svalue(unsigned int value)
      {
	assert(value <= value_mask());
	const unsigned int N = 1u << value_bit() - 1;
	if (value >= N)
	  return -int(value_mask() - value) - 1;
	else
	  return value;
      }

    static unsigned int get(uint32_type value) {return value & value_mask();}
    static unsigned int get(const address_space &a, int fc,
			    uint32_type address)
      {return a.getb(fc, address);}
    static void put(uint32_type &dest, unsigned int value)
      {dest = dest & ~uint32_type(value_mask()) | value & value_mask();}
    static void put(address_space &a, int fc,
		    uint32_type address, unsigned int value)
      {a.putb(fc, address, value & value_mask());}

    static const char *suffix() {return "b";}
  };

  /* Access methods for word data.  */
  struct word_size
  {
    typedef uint_type uvalue_type;
    typedef sint_type svalue_type;

    static size_t value_bit() {return 16;}
    static uint_type value_mask() {return (uint_type(1) << value_bit()) - 1;}
    static size_t value_size() {return 2;}
    static size_t aligned_value_size() {return value_size();}
    static sint_type svalue(uint_type value)
      {
	assert(value <= value_mask());
	const uint_type N = uint_type(1) << value_bit() - 1;
	if (value >= N)
	  return -sint_type(value_mask() - value) - 1;
	else
	  return value;
      }

    static uint_type get(uint32_type value) {return value & value_mask();}
    static uint_type get(const address_space &a, int fc, uint32_type address)
      {return a.getw(fc, address);}
    static void put(uint32_type &dest, uint_type value)
      {dest = dest & ~uint32_type(value_mask()) | value & value_mask();}
    static void put(address_space &a, int fc,
		    uint32_type address, uint_type value)
      {a.putw(fc, address, value & value_mask());}

    static const char *suffix() {return "w";}
  };

  /* Access methods for long word data.  */
  struct long_word_size
  {
    typedef uint32_type uvalue_type;
    typedef sint32_type svalue_type;

    static size_t value_bit() {return 32;}
    static uint32_type value_mask()
      //{return (uint32_type(1) << value_bit()) - 1;}
      {return 0xffffffffu;}
    static size_t value_size() {return 4;}
    static size_t aligned_value_size() {return value_size();}
    static sint32_type svalue(uint32_type value)
      {
	assert(value <= value_mask());
	const uint32_type N = uint32_type(1) << value_bit() - 1;
	if (value >= N)
	  return -sint32_type(value_mask() - value) - 1;
	else
	  return value;
      }

    static uint32_type get(uint32_type value) {return value;}
    static uint32_type get(const address_space &a, int fc,
			   uint32_type address)
      {return a.getl(fc, address);}
    static void put(uint32_type &dest, uint32_type value)
      {dest = value & value_mask();}
    static void put(address_space &a, int fc,
		    uint32_type address, uint32_type value)
      {a.putl(fc, address, value & value_mask());}

    static const char *suffix() {return "l";}
  };

#ifdef VM68K_ENABLE_DEPRECATED

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

#endif /* VM68K_ENABLE_DEPRECATED */

  /* Abstruct base class for condition testers.  */
  class condition_tester
  {
  public:
    virtual bool ls(const sint32_type *v) const
    {return cs(v) || eq(v);}
    virtual bool cs(const sint32_type *) const = 0;
    virtual bool eq(const sint32_type *) const = 0;
    virtual bool mi(const sint32_type *) const = 0;
    virtual bool lt(const sint32_type *) const = 0;
    virtual bool le(const sint32_type *v) const
    {return eq(v) || lt(v);}
  };

  class bitset_condition_tester: public condition_tester
  {
  public:
    bool cs(const sint32_type *) const;
    bool eq(const sint32_type *) const;
    bool mi(const sint32_type *) const;
    bool lt(const sint32_type *) const;
  };

  /* Status register.  */
  class status_register
  {
  protected:
    enum
    {S = 1 << 13};

  private:			// Condition testers
    static const bitset_condition_tester bitset_tester;
    static const condition_tester *const general_condition_tester;
    static const condition_tester *const add_condition_tester;

  private:
    const condition_tester *cc_eval;
    sint32_type cc_values[3];
    const condition_tester *x_eval;
    sint32_type x_values[3];
    uint_type value;

  public:
    status_register();

  public:
    operator uint_type() const;
    status_register &operator=(uint_type v)
    {
      x_eval = cc_eval = &bitset_tester;
      x_values[0] = cc_values[0] = v;
      return *this;
    }

  public:
    bool hi() const
      {return !ls();}
    bool ls() const
      {return cc_eval->ls(cc_values);}
    bool cc() const
      {return !cs();}
    bool cs() const
      {return cc_eval->cs(cc_values);}
    bool ne() const
      {return !eq();}
    bool eq() const
      {return cc_eval->eq(cc_values);}
    bool pl() const
      {return !mi();}
    bool mi() const
      {return cc_eval->mi(cc_values);}
    bool ge() const
      {return !lt();}
    bool lt() const
      {return cc_eval->lt(cc_values);}
    bool gt() const
      {return !le();}
    bool le() const
      {return cc_eval->le(cc_values);}

  public:
    uint_type x() const
      {return x_eval->cs(x_values) ? 1 : 0;}

  public:
    /* Sets the condition codes by a result.  */
    void set_cc(sint32_type r)
    {
      cc_eval = general_condition_tester;
      cc_values[0] = r;
    }

    /* Sets the condition codes as ADD.  */
    void set_cc_as_add(sint32_type r, sint32_type d, sint32_type s)
    {
      x_eval = cc_eval = add_condition_tester;
      x_values[0] = cc_values[0] = r;
      x_values[1] = cc_values[1] = d;
      x_values[2] = cc_values[2] = s;
    }

    void set_cc_cmp(sint32_type, sint32_type, sint32_type);
    void set_cc_sub(sint32_type, sint32_type, sint32_type);
    void set_cc_asr(sint32_type, sint32_type, uint_type);
    void set_cc_lsr(sint32_type r, sint32_type d, uint_type s)
      {set_cc_asr(r, d, s);}
    void set_cc_lsl(sint32_type, sint32_type, uint_type);

  public:
    /* Returns whether supervisor state.  */
    bool supervisor_state() const
      {return (value & S) != 0;}

    /* Sets the S bit.  */
    void set_s_bit(bool s)
      {if (s) value |= S; else value &= ~S;}
  };

  /* CPU registers (mc68000).  */
  struct registers
  {
    uint32_type d[8];		/* %d0-%d7 */
    uint32_type a[8];		/* %a0-%a6/%sp */
    uint32_type pc;
    status_register sr;
    uint32_type usp;
    uint32_type ssp;
  };

#if 0
  struct exception_listener
  {
    virtual void bus_error (registers *, address_space *) = 0;
    virtual void address_error (registers *, address_space *) = 0;
    virtual void trap (int, registers *, address_space *) = 0;
    virtual void interrupt (int, registers *, address_space *) = 0;
    virtual void illegal (int, registers *, address_space *) = 0;
  };
#endif

  /* Context of execution.  A context represents all the state of
     execution.  See also `class exec_unit'.  */
  class context
  {
  public:
    registers regs;
    address_space *mem;
    //exception_listener *exception;

  private:
    /* Cache values for program and data FC's.  */
    int pfc_cache, dfc_cache;

  private:			// interrupt
    /* True if the thread in this context is interrupted.  */
    bool a_interrupted;

  public:
    explicit context(address_space *);

  public:
    /* Returns true if supervisor state.  */
    bool supervisor_state() const
    {return regs.sr.supervisor_state();}

    /* Sets the supervisor state to STATE.  */
    void set_supervisor_state(bool state);

    /* Returns the value of the status register.  */
    uint_type sr() const;

    /* Sets the status register.  */
    void set_sr(uint_type value);

  public:
    /* Returns the FC for program in the current state.  */
    int program_fc() const
    {return pfc_cache;}

    /* Returns the FC for data in the current state.  */
    int data_fc() const
    {return dfc_cache;}

  public:
    template <class Size>
    typename Size::uvalue_type fetch(Size, size_t offset) const
    {return Size::get(*mem, program_fc(), regs.pc + offset);}

  public:			// interrupt
    /* Returns true if the thread in this context is interrupted.  */
    bool interrupted() const
    {return a_interrupted;}

    /* Handles interrupts.  */
    void handle_interrupts();
  };

  template <> inline byte_size::uvalue_type
  context::fetch(byte_size, size_t offset) const
  {
    return byte_size::get(word_size::get(*mem, program_fc(),
					 regs.pc + offset));
  }

  /* Execution unit.  */
  class exec_unit
  {
  public:
    typedef void (*instruction_handler)(uint_type, context &, unsigned long);

    /* Type of an instruction.  */
    typedef pair<instruction_handler, unsigned long> instruction_type;

  public:
    static void illegal(uint_type, context &, unsigned long);

  private:
    vector<instruction_type> instructions;

  public:
    exec_unit();

  public:
    /* Sets an instruction for an operation word.  The old value is
       returned.  */
    instruction_type set_instruction(uint_type op, const instruction_type &i)
    {
      op &= 0xffffu;
      instruction_type old_value = instructions[op];
      instructions[op] = i;
      return old_value;
    }

    /* Sets an instruction to operation codes.  */
    void set_instruction(int op, int mask, const instruction_type &i);

    void set_instruction(int op, int mask, instruction_handler h)
      {set_instruction(op, mask, instruction_type(h, 0));}

  protected:
    /* Dispatches for instruction handlers.  */
    void dispatch(uint_type op, context &ec) const
      {
	op &= 0xffffu;
	instructions[op].first(op, ec, instructions[op].second);
      }

  public:
    /* Steps one instruction.  */
    void step(context &c) const
      {dispatch(c.fetch(word_size(), 0), c);}

    /* Starts the program.  */
    void run(context &c) const;
  };
} // vm68k

#endif /* not _VM68K_CPU_H */

