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
#define VM68k_CPU_H

#include <iterator>
#include "vm68k/types.h"
#include "vm68k/memory.h"

/* User view of CPU registers.  */
struct user_cpu_regs
{
  uint32 r[16];
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
  virtual void bus_error (cpu_regs *, memory *) = 0;
  virtual void address_error (cpu_regs *, memory *) = 0;
  virtual void trap (int, cpu_regs *, memory *) = 0;
  virtual void interrupt (int, cpu_regs *, memory *) = 0;
  virtual void illegal (int, cpu_regs *, memory *) = 0;
};

struct execution_context
{
  cpu_regs regs;
  memory *mem;
  exception_listener *exception;
  explicit execution_context (memory *);
};

class cpu
{
public:
  explicit cpu (memory *);
  void set_pc (uint32);
  void run ();
  void set_exception_listener (exception_listener *);
  typedef void (*insn_handler) (int, execution_context *);
  void set_handlers (int begin, int end, insn_handler);
  static void illegal_insn (int, execution_context *);
private:
  execution_context context;
  insn_handler insn[0x10000];
};

#endif

