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

#ifndef VM68K_CPU_H
#define VM68K_CPU_H 1

#include <iterator>
#include "vm68k/types.h"
#include "vm68k/memory.h"

namespace vm68k
{

  inline int extsb(unsigned int value)
    {
      const unsigned int N = 1u << 7;
      const unsigned int M = (N << 1) - 1;
      value &= M;
      return value >= N ? -(int) (M - value) - 1 : (int) value;
    }

  inline int extsw(unsigned int value)
    {
      const unsigned int N = 1u << 15;
      const unsigned int M = (N << 1) - 1;
      value &= M;
      return value >= N ? -(int) (M - value) - 1 : (int) value;
    }

  inline int32 extsl(uint32 value)
    {
      const uint32 N = (uint32) 1u << 31;
      const uint32 M = (N << 1) - 1;
      value &= M;
      return value >= N ? -(int32) (M - value) - 1 : (int32) value;
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
    const cc_evaluator *cc_eval;
    sint32_type cc_values[3];
    uint16 value;
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
    void set_cc(int32);
    void set_cc_cmp(sint32_type, sint32_type, sint32_type);
    void set_cc_sub(sint32_type, sint32_type, sint32_type);
    void set_cc_asr(sint32_type, sint32_type, uint_type);
    void set_cc_lsr(sint32_type r, sint32_type d, uint_type s)
      {set_cc_asr(r, d, s);}
    void set_cc_lsl(sint32_type, sint32_type, uint_type);
  public:
    bool supervisor_state() const
      {return (value & S) != 0;}
  };

  /* CPU registers (mc68000).  */
  struct registers
  {
    uint32 d[8];		/* %d0-%d7 */
    uint32 a[8];		/* %a0-%a6/%sp */
    status_register sr;
    uint32 pc;
    uint32 usp;
    uint32 ssp;
  };

struct exception_listener
{
  virtual void bus_error (registers *, address_space *) = 0;
  virtual void address_error (registers *, address_space *) = 0;
  virtual void trap (int, registers *, address_space *) = 0;
  virtual void interrupt (int, registers *, address_space *) = 0;
  virtual void illegal (int, registers *, address_space *) = 0;
};

  class context;	// Forward declaration.
 
  /* Execution unit.  */
  class exec_unit
  {
  public:
    typedef void (*instruction_handler)(unsigned int, context &);
  public:
    static void illegal(unsigned int, context &);
  protected:
    static void install_instructions(exec_unit &);
  private:
    instruction_handler instruction[0x10000];
  public:
    exec_unit();
  public:
    void set_instruction(int op, int mask, instruction_handler);
    void dispatch(unsigned int op, context &) const;
  };

  class context
  {
  public:
    registers regs;
    address_space *mem;
    exception_listener *exception;
  private:
    const exec_unit *eu;
  public:
    context(address_space *, const exec_unit *);
  public:
    int program_fc() const
      {return regs.sr.supervisor_state() ? SUPER_PROGRAM : USER_PROGRAM;}
    int data_fc() const
      {return regs.sr.supervisor_state() ? SUPER_DATA : USER_DATA;}

  public:
    uint_type fetchw(int disp) const
      {return mem->getw_aligned(program_fc(), regs.pc + disp);}
    uint32 fetchl(int disp) const
      {return mem->getl(program_fc(), regs.pc + disp);}

    /* Steps one instruction.  */
    void step()
      {eu->dispatch(fetchw(0), *this);}

    /* Starts the program.  */
    void run();
  };
} // namespace vm68k

#endif

