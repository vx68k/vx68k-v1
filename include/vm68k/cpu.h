/* -*-C++-*- */
/* vx68k - Virtual X68000
   Copyright (C) 1998 Hypercore Software Design, Ltd.

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

/* User view of CPU registers.  */
struct user_cpu_regs
{
  uint32 gr[16];		// d0-d7/a0-a7
  uint32 pc;
  uint16 ccr;
};

/* CPU registers (mc68000).  */
struct cpu_regs
  : user_cpu_regs
{
  uint32 usp;
  uint32 ssp;
  uint16 sr;
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
    static void illegal(int, execution_context *);
  protected:
    static void install_instructions(exec_unit *);
  private:
    typedef void (*insn_handler)(int, execution_context *);
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
public:
  cpu ();
  void run (execution_context *);
  void set_exception_listener (exception_listener *);
  typedef void (*insn_handler) (int, execution_context *);
  void set_handlers (int begin, int end, insn_handler);
  static void illegal_insn (int, execution_context *);
private:
  insn_handler insn[0x10000];
};

};				// namespace vm68k

#endif

