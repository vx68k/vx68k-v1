/* vx68k - Virtual X68000
   Copyright (C) 1998, 1999 Hypercore Software Design, Ltd.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307, USA.  */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#undef const
#undef inline

#include <vx68k/human.h>

using namespace vx68k::human;
using namespace vm68k;
using namespace std;

sint_type
memory_allocator::free(uint32_type memptr)
{
  return 0;
}

sint32_type
memory_allocator::alloc(uint32_type len, uint32_type parent)
{
  return 0;
}

memory_allocator::memory_allocator(address_space *as,
				   uint32_type address, uint32_type size)
  : _as(as),
    first_block(0)
{
  _as->putl(SUPER_DATA, address + 0, 0);
  _as->putl(SUPER_DATA, address + 4, 0);
  _as->putl(SUPER_DATA, address + 8, address + size);
  _as->putl(SUPER_DATA, address + 12, 0);
  first_block = address;
}

