/* -*-C++-*- */
/* vx68k - Virtual X68000
   Copyright (C) 1998, 1999 Hypercore Software Design, Ltd.

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
#include <utility>

namespace vm68k
{
  using namespace std;

  struct byte_size_traits
  {
    typedef unsigned int uvalue_type;
    typedef int svalue_type;
    static size_t value_size() {return 1;}
    static unsigned int value_mask() {return (1u << value_size() * 8) - 1;}
    static size_t immediate_operand_size() {return 2;}

    static unsigned int get(uint32_type value) {return value & 0xffu;}
    static void set(uint32_type &dest, unsigned int value)
      {dest = dest & ~value_mask() | value & value_mask();}
  };

  struct word_size_traits
  {
    typedef uint_type uvalue_type;
    typedef sint_type svalue_type;
    static size_t value_size() {return 2;}
    static uint_type value_mask()
      {return (uint_type(1) << value_size() * 8) - 1;}
    static size_t immediate_operand_size() {return value_size();}

    static uint_type get(uint32_type value) {return value & 0xffffu;}
    static void set(uint32_type &dest, uint_type value)
      {dest = dest & ~value_mask() | value & value_mask();}
  };

  struct long_word_size_traits
  {
    typedef uint32_type uvalue_type;
    typedef sint32_type svalue_type;
    static size_t value_size() {return 4;}
    static uint32_type value_mask()
      {return (uint32_type(1) << value_size() * 8) - 1;}
    static size_t immediate_operand_size() {return value_size();}

    static uint32_type get(uint32_type value) {return value;}
    static void set(uint32_type &dest, uint32_type value)
      {dest = value & value_mask();}
  };

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

  /* Condition code evaluator (abstract base class).  */
  struct cc_evaluator
  {
    virtual bool ls(const sint32_type *) const = 0;
    virtual bool cs(const sint32_type *) const = 0;
    virtual bool eq(const sint32_type *) const = 0;
    virtual bool mi(const sint32_type *) const = 0;
    virtual bool lt(const sint32_type *) const = 0;
    virtual bool le(const sint32_type *) const = 0;
  };

  /* Status register.  */
  class status_register
  {
  protected:
    enum
    {S = 1 << 13};
  private:
    static const cc_evaluator *const common_cc_eval;
  private:
    const cc_evaluator *cc_eval;
    sint32_type cc_values[3];
    const cc_evaluator *x_eval;
    sint32_type x_values[3];
    uint_type value;
  public:
    status_register();
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
	cc_eval = common_cc_eval;
	cc_values[0] = r;
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

  /* Base object that can be passed to instructions.  */
  struct instruction_data
  {
  }; 

  class exec_unit;		// Forward declaration.

  /* Context of execution.  */
  class context
  {
  public:
    registers regs;
    address_space *mem;
    //exception_listener *exception;

  private:
    int pfc_cache, dfc_cache;

  public:
    explicit context(address_space *);

  public:
    /* Returns true if supervisor state.  */
    bool supervisor_state() const
      {return regs.sr.supervisor_state();}

    /* Sets the supervisor state to STATE.  */
    void set_supervisor_state(bool state);

    /* Returns the FC for program in the current state.  */
    int program_fc() const
      {return pfc_cache;}

    /* Returns the FC for data in the current state.  */
    int data_fc() const
      {return dfc_cache;}

  public:
    uint_type fetchw(int disp) const
      {return mem->getw_aligned(program_fc(), regs.pc + disp);}
    uint32_type fetchl(int disp) const
      {return mem->getl(program_fc(), regs.pc + disp);}
  };

  /* Execution unit.  */
  class exec_unit
  {
  public:
    typedef void (*instruction_handler)(unsigned int, context &,
					instruction_data *);
  public:
    static void illegal(unsigned int, context &, instruction_data *);
  protected:
    static void install_instructions(exec_unit &);
  private:
    pair<instruction_handler, instruction_data *> instructions[0x10000];
  public:
    exec_unit();
  public:
    void set_instruction(int op, int mask, instruction_handler,
			 instruction_data *);
    void set_instruction(int op, int mask, instruction_handler h)
      {set_instruction(op, mask, h, NULL);}

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
      {dispatch(c.fetchw(0), c);}

    /* Starts the program.  */
    void run(context &c) const;
  };
} // vm68k

#endif /* not _VM68K_CPU_H */

