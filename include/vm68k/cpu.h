/* -*-C++-*- */

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
struct cpu_regs: user_cpu_regs
{
  uint32 usp;
  uint32 ssp;
  uint16 sr;
};

struct interrupt_listener
{
  virtual ~interrupt_listener () {}
  virtual void bus_error (cpu_regs *, memory *) = 0;
  virtual void address_error (cpu_regs *, memory *) = 0;
  virtual void trap (int, cpu_regs *, memory *) = 0;
  virtual void interrupt (int, cpu_regs *, memory *) = 0;
};

class cpu
{
public:
  cpu (memory *);
  void set_pc (uint32);
  void run ();
  void set_interrupt_listener (interrupt_listener *);
protected:
  typedef void (*insn_handler) (int, cpu_regs *, memory *);
  void set_handlers (int begin, int end, insn_handler);
  static void undefined_insn (int, cpu_regs *, memory *);
private:
  cpu_regs regs;
  memory *mem;
  interrupt_listener *interrupt;
  insn_handler insn[0x10000];
};

#endif

