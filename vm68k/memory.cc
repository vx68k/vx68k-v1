#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#undef const
#undef inline

#include "vm68k/cpu.h"

#include <algorithm>

memory::memory ()
{
  memory_page *null = NULL;
  fill (page + 0, page + (1 << 32 - PAGE_SIZE_SHIFT), null);
}

