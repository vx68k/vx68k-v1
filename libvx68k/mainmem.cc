/* vx68k - Virtual X68000
   Copyright (C) 1998-2000 Hypercore Software Design, Ltd.

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

#ifdef DUMP_MEMORY
# ifdef HAVE_FCNTL_H
#  include <fcntl.h>
# endif
# ifdef HAVE_UNISTD_H
#  include <unistd.h>
# endif
#endif

using namespace vx68k;
using namespace std;

uint_type
main_memory::get_8(int fc, uint32_type address) const
{
  if (address >= end)
    {
      generate_bus_error(true, fc, address);
      abort();
    }

  uint_type w = array[address >> 1];
  return address & 0x1 != 0 ? w & 0xffu : w >> 8;
}

uint_type
main_memory::get_16(int fc, uint32_type address) const
{
  // Address error?
  if (address >= end)
    {
      generate_bus_error(true, fc, address);
      abort();
    }

  return array[address >> 1];
}

uint32_type
main_memory::get_32(int fc, uint32_type address) const
{
  // Address error?
  uint32_type address2 = address + 2;
  if (address2 >= end)
    {
      generate_bus_error(true, fc, address2);
      abort();
    }

  return uint32_type(array[address >> 1]) << 16 | array[address2 >> 1];
}

void
main_memory::put_8(int fc, uint32_type address, uint_type value)
{
  if (address >= end)
    {
      generate_bus_error(false, fc, address);
      abort();
    }

  uint_type w = array[address >> 1];
  if (address & 0x1 != 0)
    w = w & ~0xffu | value & 0xffu;
  else
    w = value << 8 | w & 0xffu;
  array[address >> 1] = w & 0xffffu;
}

void
main_memory::put_16(int fc, uint32_type address, uint_type value)
{
  // Address error?
  if (address >= end)
    {
      generate_bus_error(false, fc, address);
      abort();
    }

  array[address >> 1] = value;
}

main_memory::~main_memory()
{
#ifdef DUMP_MEMORY
  int fd = open("dump", O_WRONLY | O_CREAT | O_TRUNC, 0666);
  ::write(fd, array, end);
  close(fd);
#endif
  delete [] array;
}

main_memory::main_memory(size_t n)
  : end((n + 1u) & ~1u),
    array(NULL)
{
  array = new unsigned short [end >> 1];
#ifndef NDEBUG
  // These ILLEGAL instructions makes the debug easy.
  fill(array + 0, array + (end >> 1), 0x4afc);
#endif
}

