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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#undef const
#undef inline

#include <vx68k/machine.h>

using namespace vx68k;
using namespace std;

machine::machine(size_t memory_size)
  : _memory_size(memory_size),
    main_mem(memory_size)
{
  using vm68k::PAGE_SHIFT;

  as.set_pages(0 >> PAGE_SHIFT, _memory_size >> PAGE_SHIFT, &main_mem);
#if 0
  as.set_pages(0xc00000 >> PAGE_SHIFT, 0xe00000 >> PAGE_SHIFT, &graphic_vram);
  as.set_pages(0xe00000 >> PAGE_SHIFT, 0xe80000 >> PAGE_SHIFT, &text_vram);
#endif
}
