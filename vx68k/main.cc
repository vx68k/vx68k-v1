#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include "vm68k/cpu.h"
#include "vx68k/memory.h"

int
main (int argc, char **argv)
{
  x68k_memory mem;
  cpu main_cpu (&mem);
  abort ();			// FIXME
  main_cpu.run ();
  return 1;
}

