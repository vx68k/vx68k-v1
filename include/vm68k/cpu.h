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

  struct status_register
  {
    bool eq() const;
    bool ne() const {return eq();}
    bool supervisor_state() const;
  };

  /* CPU registers (mc68000).  */
  struct cpu_regs
  {
    uint32 d0, d1, d2, d3, d4, d5, d6, d7;
    uint32 a0, a1, a2, a3, a4, a5, a6, a7;
    status_register sr;
    uint32 pc;
    uint32 usp;
    uint32 ssp;
  };

struct exception_listener
{
  virtual void bus_error (cpu_regs *, address_space *) = 0;
  virtual void address_error (cpu_regs *, address_space *) = 0;
  virtual void trap (int, cpu_regs *, address_space *) = 0;
  virtual void interrupt (int, cpu_regs *, address_space *) = 0;
  virtual void illegal (int, cpu_regs *, address_space *) = 0;
};

struct execution_context
{
  cpu_regs regs;
  address_space *mem;
  exception_listener *exception;
  explicit execution_context (address_space *);
};

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
    void execute(execution_context *) const;
  };

  /* A CPU.

     This CPU will emulate non-exceptional operations of a m68k
     processor.  Exceptional operations must be handled by callbacks.  */
  class cpu
  {
  private:
    exec_unit eu;
  public:
    cpu ();
    void run (execution_context *);
    void set_exception_listener (exception_listener *);
    void set_handlers (int begin, int end, exec_unit::insn_handler);
  };

} // namespace vm68k

#endif

