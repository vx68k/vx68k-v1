/* -*- C++ -*- */
/* Virtual X68000 - X68000 virtual machine
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

#include <queue>
#include <vector>
#include <utility>
#include <cassert>

namespace vm68k
{
  using namespace std;

  /* Access methods for byte data.  */
  struct byte_size
  {
    typedef int uvalue_type;
    typedef int value_type;
    typedef int svalue_type;

    static size_t value_bit() {return 8;}
    static int value_mask() {return (1 << value_bit()) - 1;}
    static size_t value_size() {return 1;}
    static size_t aligned_value_size() {return 2;}

    static int uvalue(int value);
    static int svalue(int value);

    static int uget(const uint32_type &reg);
    static int uget(const memory_map &,
		    memory::function_code, uint32_type address);
    static int get(const uint32_type &reg);
    static int get(const memory_map &,
		   memory::function_code, uint32_type address);

    static void put(uint32_type &reg, int value);
    static void put(memory_map &, memory::function_code,
		    uint32_type address, int value);

    static const char *suffix() {return "b";}
  };

  inline int
  byte_size::uvalue(int value)
  {
    return value & value_mask();
  }

  inline int
  byte_size::svalue(int value)
  {
    value = uvalue(value);
    const int N = 1 << value_bit() - 1;
    if (value >= N)
      return -int(value_mask() - value) - 1;
    else
      return value;
  }

  inline int
  byte_size::uget(const uint32_type &reg)
  {
    return uvalue(reg);
  }

  inline int
  byte_size::uget(const memory_map &m,
		  memory::function_code fc, uint32_type address)
  {
    return m.get_8(address, fc);
  }

  inline int
  byte_size::get(const uint32_type &reg)
  {
    return svalue(uget(reg));
  }

  inline int
  byte_size::get(const memory_map &m, memory::function_code fc,
		 uint32_type address)
  {
    return svalue(uget(m, fc, address));
  }

  inline void
  byte_size::put(uint32_type &reg, int value)
  {
    reg = reg & ~uint32_type(value_mask()) | uvalue(value);
  }

  inline void
  byte_size::put(memory_map &m, memory::function_code fc,
		 uint32_type address, int value)
  {
    m.put_8(address, value, fc);
  }

  /* Access methods for word data.  */
  struct word_size
  {
    typedef uint16_type uvalue_type;
    typedef sint16_type value_type;
    typedef sint16_type svalue_type;

    static size_t value_bit() {return 16;}
    static uint16_type value_mask()
    {return (uint16_type(1) << value_bit()) - 1;}
    static size_t value_size() {return 2;}
    static size_t aligned_value_size() {return value_size();}

    static uint16_type uvalue(uint16_type value);
    static sint16_type svalue(uint16_type value);

    static uint16_type uget(const uint32_type &reg);
    static uint16_type uget_unchecked(const memory_map &,
				      memory::function_code,
				      uint32_type address);
    static uint16_type uget(const memory_map &,
			    memory::function_code, uint32_type address);
    static sint16_type get(const uint32_type &reg);
    static sint16_type get_unchecked(const memory_map &,
				     memory::function_code,
				     uint32_type address);
    static sint16_type get(const memory_map &,
			   memory::function_code, uint32_type address);

    static void put(uint32_type &reg, uint16_type value);
    static void put(memory_map &, memory::function_code,
		    uint32_type address, uint16_type value);

    static const char *suffix() {return "w";}
  };

  inline uint16_type
  word_size::uvalue(uint16_type value)
  {
    return value & value_mask();
  }

  inline sint16_type
  word_size::svalue(uint16_type value)
  {
    value = uvalue(value);
    const uint16_type N = uint16_type(1) << value_bit() - 1;
    if (value >= N)
      return -sint16_type(value_mask() - value) - 1;
    else
      return value;
  }

  inline uint16_type
  word_size::uget(const uint32_type &reg)
  {
    return uvalue(reg);
  }

  inline uint16_type
  word_size::uget_unchecked(const memory_map &m,
			    memory::function_code fc, uint32_type address)
  {
    return m.get_16_unchecked(address, fc);
  }

  inline uint16_type
  word_size::uget(const memory_map &m,
		  memory::function_code fc, uint32_type address)
  {
    return m.get_16(address, fc);
  }

  inline sint16_type
  word_size::get(const uint32_type &reg)
  {
    return svalue(uget(reg));
  }

  inline sint16_type
  word_size::get_unchecked(const memory_map &m,
			   memory::function_code fc, uint32_type address)
  {
    return svalue(uget_unchecked(m, fc, address));
  }

  inline sint16_type
  word_size::get(const memory_map &m,
		 memory::function_code fc, uint32_type address)
  {
    return svalue(uget(m, fc, address));
  }

  inline void
  word_size::put(uint32_type &reg, uint16_type value)
  {
    reg = reg & ~uint32_type(value_mask()) | uvalue(value);
  }

  inline void
  word_size::put(memory_map &m, memory::function_code fc,
		 uint32_type address, uint16_type value)
  {
    m.put_16(address, value, fc);
  }

  /* Access methods for long word data.  */
  struct long_word_size
  {
    typedef uint32_type uvalue_type;
    typedef sint32_type value_type;
    typedef sint32_type svalue_type;

    static size_t value_bit() {return 32;}
    static uint32_type value_mask()
      //{return (uint32_type(1) << value_bit()) - 1;}
      {return 0xffffffffu;}
    static size_t value_size() {return 4;}
    static size_t aligned_value_size() {return value_size();}

    static uint32_type uvalue(uint32_type value);
    static sint32_type svalue(uint32_type value);

    static uint32_type uget(const uint32_type &reg);
    static uint32_type uget_unchecked(const memory_map &,
				      memory::function_code,
				      uint32_type address);
    static uint32_type uget(const memory_map &,
			    memory::function_code, uint32_type address);
    static sint32_type get(const uint32_type &reg);
    static sint32_type get_unchecked(const memory_map &,
				     memory::function_code,
				     uint32_type address);
    static sint32_type get(const memory_map &,
			   memory::function_code, uint32_type address);

    static void put(uint32_type &reg, uint32_type value);
    static void put(memory_map &, memory::function_code,
		    uint32_type address, uint32_type value);

    static const char *suffix() {return "l";}
  };

  inline uint32_type
  long_word_size::uvalue(uint32_type value)
  {
    return value & value_mask();
  }

  inline sint32_type
  long_word_size::svalue(uint32_type value)
  {
    value &= value_mask();
    const uint32_type N = uint32_type(1) << value_bit() - 1;
    if (value >= N)
      return -sint32_type(value_mask() - value) - 1;
    else
      return value;
  }

  inline uint32_type
  long_word_size::uget(const uint32_type &reg)
  {
    return uvalue(reg);
  }

  inline uint32_type
  long_word_size::uget_unchecked(const memory_map &m,
				 memory::function_code fc, uint32_type address)
  {
    return m.get_32(address, fc);
  }

  inline uint32_type
  long_word_size::uget(const memory_map &m,
		       memory::function_code fc, uint32_type address)
  {
    return m.get_32(address, fc);
  }

  inline sint32_type
  long_word_size::get(const uint32_type &reg)
  {
    return svalue(uget(reg));
  }

  inline sint32_type
  long_word_size::get_unchecked(const memory_map &m,
				memory::function_code fc, uint32_type address)
  {
    return svalue(uget_unchecked(m, fc, address));
  }

  inline sint32_type
  long_word_size::get(const memory_map &m,
		      memory::function_code fc, uint32_type address)
  {
    return svalue(uget(m, fc, address));
  }

  inline void
  long_word_size::put(uint32_type &reg, uint32_type value)
  {
    reg = reg & ~uint32_type(value_mask()) | uvalue(value);
  }

  inline void
  long_word_size::put(memory_map &m, memory::function_code fc,
		      uint32_type address, uint32_type value)
  {
    m.put_32(address, value, fc);
  }

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
  inline sint16_type
  extsw(uint16_type value)
  {
    const uint16_type N = uint16_type(1) << 15;
    const uint16_type M = (N << 1) - 1;
    value &= M;
    return value >= N ? -sint16_type(M - value) - 1 : sint16_type(value);
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
    virtual bool ls(const sint32_type *) const;
    virtual bool cs(const sint32_type *) const = 0;
    virtual bool eq(const sint32_type *) const = 0;
    virtual bool mi(const sint32_type *) const = 0;
    virtual bool lt(const sint32_type *) const = 0;
    virtual bool le(const sint32_type *) const;
    virtual unsigned int x(const sint32_type *) const;
  };

  inline bool
  condition_tester::ls(const sint32_type *v) const
  {
    return this->cs(v) || this->eq(v);
  }

  inline bool
  condition_tester::le(const sint32_type *v) const
  {
    return this->eq(v) || this->lt(v);
  }

  inline unsigned int
  condition_tester::x(const sint32_type *v) const
  {
    return this->cs(v);
  }

  class bitset_condition_tester: public condition_tester
  {
  public:
    bool cs(const sint32_type *) const;
    bool eq(const sint32_type *) const;
    bool mi(const sint32_type *) const;
    bool lt(const sint32_type *) const;
  };

  /* Status register.  */
  class condition_code
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
    uint16_type value;

  public:
    condition_code();

  public:
    operator uint16_type() const;
    condition_code &operator=(uint16_type v)
    {
      value = v & 0xff00;
      x_eval = cc_eval = &bitset_tester;
      x_values[0] = cc_values[0] = v;
      return *this;
    }

  public:
    bool hi() const {return !cc_eval->ls(cc_values);}
    bool ls() const {return  cc_eval->ls(cc_values);}
    bool cc() const {return !cc_eval->cs(cc_values);}
    bool cs() const {return  cc_eval->cs(cc_values);}
    bool ne() const {return !cc_eval->eq(cc_values);}
    bool eq() const {return  cc_eval->eq(cc_values);}
    bool pl() const {return !cc_eval->mi(cc_values);}
    bool mi() const {return  cc_eval->mi(cc_values);}
    bool ge() const {return !cc_eval->lt(cc_values);}
    bool lt() const {return  cc_eval->lt(cc_values);}
    bool gt() const {return !cc_eval->le(cc_values);}
    bool le() const {return  cc_eval->le(cc_values);}

  public:
    int x() const
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
    void set_cc_asr(sint32_type, sint32_type, unsigned int);
    void set_cc_lsr(sint32_type r, sint32_type d, unsigned int s)
      {set_cc_asr(r, d, s);}
    void set_cc_lsl(sint32_type, sint32_type, unsigned int);

  public:
    /* Returns whether supervisor state.  */
    bool supervisor_state() const
      {return (value & S) != 0;}

    /* Sets the S bit.  */
    void set_s_bit(bool s)
      {if (s) value |= S; else value &= ~S;}
  };

  typedef condition_code status_register;

  /* CPU registers (mc68000).  */
  struct registers
  {
    uint32_type d[8];		/* %d0-%d7 */
    uint32_type a[8];		/* %a0-%a6/%sp */
    uint32_type pc;
    condition_code ccr;
    uint32_type usp;
    uint32_type ssp;

    template <class Size>
    typename Size::svalue_type data_register(unsigned int regno, Size)
    {return Size::get(d[regno]);}
    template <class Size>
    void set_data_register(unsigned int regno, Size,
			   typename Size::uvalue_type value)
    {Size::put(d[regno], value);}

    void set_pc(uint32_type value) {long_word_size::put(pc, value);}
    void advance_pc(uint32_type value) {long_word_size::put(pc, pc + value);}
  };

  /* Context of execution.  A context represents all the state of
     execution.  See also `class exec_unit'.  */
  class context
  {
  public:
    registers regs;
    memory_map *mem;

  private:
    /* Cache values for program and data FC's.  */
    memory::function_code pfc_cache, dfc_cache;

  private:			// interrupt
    /* True if the thread in this context is interrupted.  */
    bool a_interrupted;

    vector<queue<unsigned int> > interrupt_queues;

  public:
    explicit context(memory_map *);

  public:
    /* Returns true if supervisor state.  */
    bool supervisor_state() const
    {return regs.ccr.supervisor_state();}

    /* Sets the supervisor state to STATE.  */
    void set_supervisor_state(bool state);

    /* Returns the value of the status register.  */
    uint16_type sr() const;

    /* Sets the status register.  */
    void set_sr(uint16_type value);

  public:
    /* Returns the FC for program in the current state.  */
    memory::function_code program_fc() const {return pfc_cache;}

    /* Returns the FC for data in the current state.  */
    memory::function_code data_fc() const {return dfc_cache;}

  public:
    template <class Size>
    typename Size::uvalue_type ufetch(Size, size_t offset) const;
    template <class Size>
    typename Size::svalue_type fetch(Size, size_t offset) const;

  public:			// interrupt
    /* Returns true if the thread in this context is interrupted.  */
    bool interrupted() const
    {return a_interrupted;}

    /* Interrupts.  */
    void interrupt(int prio, unsigned int vecno);

    /* Handles interrupts.  */
    void handle_interrupts();
  };

  template <class Size> inline typename Size::uvalue_type
  context::ufetch(Size, size_t offset) const
  {
    return Size::uget_unchecked(*mem, program_fc(), regs.pc + offset);
  }

  template <> inline byte_size::uvalue_type
  context::ufetch(byte_size, size_t offset) const
  {
    return byte_size::uvalue(word_size::uget_unchecked(*mem, program_fc(),
						       regs.pc + offset));
  }

  template <class Size> inline typename Size::svalue_type
  context::fetch(Size, size_t offset) const
  {
    return Size::get_unchecked(*mem, program_fc(), regs.pc + offset);
  }

  template <> inline byte_size::svalue_type
  context::fetch(byte_size, size_t offset) const
  {
    return byte_size::svalue(word_size::uget_unchecked(*mem, program_fc(),
						       regs.pc + offset));
  }

  /* Execution unit.  */
  class exec_unit
  {
  public:
    typedef void (*instruction_handler)(uint16_type, context &, unsigned long);

    /* Type of an instruction.  */
    typedef pair<instruction_handler, unsigned long> instruction_type;

  public:
    static void illegal(uint16_type, context &, unsigned long);

  private:
    vector<instruction_type> instructions;

  public:
    exec_unit();

  public:
    /* Sets an instruction for an operation word.  The old value is
       returned.  */
    instruction_type set_instruction(uint16_type op, const instruction_type &i)
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
    void dispatch(uint16_type op, context &ec) const
      {
	op &= 0xffffu;
	instructions[op].first(op, ec, instructions[op].second);
      }

  public:
    /* Steps one instruction.  */
    void step(context &c) const;

    /* Starts the program.  */
    void run(context &c) const;
  };

  inline void
  exec_unit::step(context &c) const
  {
    dispatch(c.ufetch(word_size(), 0), c);
  }
} // vm68k

#endif /* not _VM68K_CPU_H */

