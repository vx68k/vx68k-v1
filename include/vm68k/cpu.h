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

  class status_register
  {
  protected:
    enum
    {S = 1 << 13};
  private:
    int32 result;
    uint16 value;
  public:
    status_register();
  public:
    bool hi() const
      {return !ls();}
    bool ls() const;
    bool cc() const
      {return !cs();}
    bool cs() const;
    bool ne() const
      {return !eq();}
    bool eq() const;
    bool pl() const
      {return !mi();}
    bool mi() const;
    bool ge() const
      {return !lt();}
    bool lt() const;
    void set_cc(int32);
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
    typedef void (*insn_handler)(unsigned int, context *);
  public:
    static void illegal(unsigned int, context *);
  protected:
    static void install_instructions(exec_unit *);
  private:
    insn_handler instruction[0x10000];
  public:
    exec_unit();
  public:
    void set_instruction(int op, int mask, insn_handler);
    void dispatch(unsigned int op, context *) const;
  };

  class context
  {
  public:
    registers regs;
    address_space *mem;
    exception_listener *exception;
  private:
    exec_unit *eu;
  public:
    context(address_space *, exec_unit *);
  public:
    int program_fc() const
      {return regs.sr.supervisor_state() ? SUPER_PROGRAM : USER_PROGRAM;}
    int data_fc() const
      {return regs.sr.supervisor_state() ? SUPER_DATA : USER_DATA;}
    unsigned int fetchw(int disp) const
      {return mem->getw(program_fc(), regs.pc + disp);}
    uint32 fetchl(int disp) const
      {return mem->getl(program_fc(), regs.pc + disp);}

    /* Steps one instruction.  */
    void step()
      {eu->dispatch(fetchw(0), this);}

    /* Starts the program.  */
    void run();
  };

  typedef context execution_context;
} // namespace vm68k

#endif

