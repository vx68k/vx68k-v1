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

void
memory_allocator::remove_block(uint32_type block)
{
  uint32_type prev = _as->getl(SUPER_DATA, block + 0);
  uint32_type next = _as->getl(SUPER_DATA, block + 12);

  _as->putl(SUPER_DATA, prev + 12, next);
  if (next != 0)
    _as->putl(SUPER_DATA, next + 0, prev);
  else
    last_block = prev;
}

void
memory_allocator::make_block(uint32_type block, uint32_type len,
			      uint32_type prev, uint32_type parent)
{
  uint32_type next = _as->getl(SUPER_DATA, block + 12);

  _as->putl(SUPER_DATA, block + 0, prev);
  _as->putl(SUPER_DATA, block + 4, parent);
  _as->putl(SUPER_DATA, block + 8, block + len);
  _as->putl(SUPER_DATA, block + 12, next);

  _as->putl(SUPER_DATA, prev + 12, block);
  if (next != 0)
    _as->putl(SUPER_DATA, next + 0, block);
  else
    last_block = block;
}

void
memory_allocator::free_by_parent(uint32_type parent)
{
  uint32_type block = last_block;
  while (block != 0)
    {
      if (_as->getl(SUPER_DATA, block + 4) == parent)
	{
	  free_by_parent(block);
	  remove_block(block);
	}

      block = _as->getl(SUPER_DATA, block + 0);
    }
}

sint_type
memory_allocator::free(uint32_type memptr)
{
  memptr -= 0x10;

  uint32_type block = last_block;
  while (block != 0)
    {
      if (block == memptr)
	{
	  free_by_parent(block);
	  remove_block(block);
	  return 0;
	}

      block = _as->getl(SUPER_DATA, block + 0);
    }

  // No matching block.
  return -9;
}

sint32_type
memory_allocator::alloc(uint32_type len, uint32_type parent)
{
  len += 0x10;
  uint32_type max_free_size = 0;

  uint32_type end = limit;
  uint32_type block = last_block;
  while (block != 0)
    {
      uint32_type candidate = (_as->getl(SUPER_DATA, block + 8) + 0xf) & ~0xf;
      uint32_type free_size = end - candidate;
      if (free_size >= len)
	{
	  make_block(candidate, len, block, parent);
	  return extsl(candidate + 0x10);
	}

      if (free_size > max_free_size)
	max_free_size = free_size;

      end = block;
      block = _as->getl(SUPER_DATA, block + 0);
    }

  if (max_free_size > 0x10)
    return extsl(0x81000000 + (max_free_size - 0x10));

  return extsl(0x82000000);
}

sint32_type
memory_allocator::alloc_largest(uint32_type parent)
{
  uint32_type prev = 0;
  uint32_type largest = 0;
  uint32_type max_free_size = 0;

  uint32_type end = limit;
  uint32_type block = last_block;
  while (block != 0)
    {
      uint32_type candidate = (_as->getl(SUPER_DATA, block + 8) + 0xf) & ~0xf;
      uint32_type free_size = end - candidate;
      if (free_size > max_free_size)
	{
	  prev = block;
	  largest = candidate;
	  max_free_size = free_size;
	}

      end = block;
      block = _as->getl(SUPER_DATA, block + 0);
    }

  if (max_free_size > 0x10)
    {
      make_block(largest, max_free_size, prev, parent);
      return extsl(largest + 0x10);
    }

  return extsl(0x82000000);
}

memory_allocator::memory_allocator(address_space *as,
				   uint32_type address, uint32_type lim)
  : _as(as),
    limit(lim & ~0xf),
    last_block(0)
{
  address = (address + 0xf) | ~0xf;
  _as->putl(SUPER_DATA, address + 0, 0);
  _as->putl(SUPER_DATA, address + 4, 0);
  _as->putl(SUPER_DATA, address + 8, address + 0x10);
  _as->putl(SUPER_DATA, address + 12, 0);
  last_block = address;
}

