#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#undef const
#undef inline

#include "vm68k/cpu.h"

#include <algorithm>
#include <cassert>

/* Set PC.  */
void
cpu::set_pc (uint32 addr)
{
  if (addr & 1 != 0)
    {
      if (interrupt != NULL)
	interrupt->address_error (&regs, mem);
      /* Fail to trap address error.  */
      abort ();
    }

  regs.pc = addr;
}

void
cpu::set_interrupt_listener (interrupt_listener *l)
{
  interrupt = l;
}

#define SUPER_PROGRAM 6

void
cpu::run ()
{
  for (;;)
    {
      int w = mem->read16 (SUPER_PROGRAM, regs.pc);
      assert (w >= 0 && w < 0x10000);
      insn[w] (w, &regs, mem);
    }
}

void
cpu::set_handlers (int begin, int end, insn_handler h)
{
  assert (begin >= 0);
  assert (begin <= end);
  assert (end <= 0x10000);
  fill (insn + begin, insn + end, h);
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

