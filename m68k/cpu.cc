#include "cpu.h"

#include <algorithm>

/* Set PC.  */
void
cpu::set_pc (uint32 addr)
{
  regs.pc = addr;
}

void
cpu::set_interrupt_listener (interrupt_listener *l)
{
  interrupt = l;
}

void
cpu::run ()
{
  for (;;)
    {
      int w = mem->read16 (regs.pc);
      insn[w] (w, &regs, mem);
    }
}

void
cpu::undefined_insn (int, cpu_regs *, memory *)
{
  abort ();			// FIXME
}

cpu::cpu (memory *m)
  : mem (m),
    interrupt (NULL)
{
  std::fill (insn + 0, insn + 0x10000, &undefined_insn);
}

