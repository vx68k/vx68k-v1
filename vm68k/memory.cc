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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#undef const
#undef inline

#include "vm68k/memory.h"

#include <algorithm>

namespace vm68k
{

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

};				// namespace vm68k

