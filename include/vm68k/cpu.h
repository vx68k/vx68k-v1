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
      const unsigned int M = 1u << 7;
      value &= M + M - 1;
      return value >= M ? -(int) (M + M - value) : (int) value;
    }

  inline int extsw(unsigned int value)
    {
      const unsigned int M = 1u << 15;
      value &= M + M - 1;
      return value >= M ? -(int) (M + M - value) : (int) value;
    }

  inline int32 extsl(uint32 value)
    {
      const uint32 M = (uint32) 1u << 31;
      value &= M + M - 1;
      return value >= M ? -(int32) (M + M - value) : (int32) value;
    }

  class status_register
  {
  private:
    int32 result;
  public:
    bool eq() const;
    bool ne() const {return !eq();}
    bool lt() const;
    bool ge() const {return !lt();}
    void set_cc(int32);
    bool supervisor_state() const;
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

  class execution_context;	// Forward declaration.
 
  /* Execution unit.  */
  class exec_unit
  {
  public:
    typedef void (*insn_handler)(int, execution_context *);
  public:
    static void illegal(int, execution_context *);
  protected:
    static void install_instructions(exec_unit *);
  private:
    insn_handler instruction[0x10000];
  public:
    exec_unit();
  public:
    void set_instruction(int op, int mask, insn_handler);
    void dispatch(unsigned int op, execution_context *) const;
  };

  class execution_context
  {
  public:
    registers regs;
    address_space *mem;
    exception_listener *exception;
  private:
    exec_unit *eu;
  public:
    execution_context(address_space *, exec_unit *);
  public:
    int program_fc() const
      {return regs.sr.supervisor_state() ? SUPER_PROGRAM : USER_PROGRAM;}
    int data_fc() const
      {return regs.sr.supervisor_state() ? SUPER_DATA : USER_DATA;}
    unsigned int fetchw(int disp) const
      {return mem->getw(program_fc(), regs.pc + disp);}
    uint32 fetchl(int disp) const
      {return mem->getl(program_fc(), regs.pc + disp);}
    void run();
  };
} // namespace vm68k

#endif

