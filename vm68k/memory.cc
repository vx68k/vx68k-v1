#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#undef const
#undef inline

#include "vm68k/memory.h"

#include <algorithm>

uint16
memory::read16 (int, uint32) const
  throw (bus_error)
{
  abort ();
  return 0;
}

memory::memory ()
{
  memory_page *null = NULL;
  fill (page + 0, page + (1 << 32 - PAGE_SIZE_SHIFT), null);
}

