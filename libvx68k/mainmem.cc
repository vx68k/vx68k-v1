/* Virtual X68000 - X68000 virtual machine
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

#include <vx68k/memory.h>

#ifdef DUMP_MEMORY
# ifdef HAVE_FCNTL_H
#  include <fcntl.h>
# endif
# ifdef HAVE_UNISTD_H
#  include <unistd.h>
# endif
#endif

#include <cstdlib>
#include <cstdio>

#ifdef HAVE_NANA_H
# include <nana.h>
# undef assert
# define assert I
#else
# include <cassert>
#endif

using vx68k::main_memory;
using vm68k::bus_error_exception;
using namespace vm68k::types;
using namespace std;

void
main_memory::set_super_area(size_t n)
{
  super_area = n;
}

int
main_memory::get_8(uint32_type address, function_code fc) const
{
  if (address % 0x1000000U >= end)
    throw bus_error_exception(true, fc, address);

  uint32_type i = address % 0x1000000U / 2;
  if (address % 2 != 0)
    {
      unsigned int value = data[i] & 0xff;
      return value;
    }
  else
    {
      unsigned int value = data[i] >> 8;
      assert(value <= 0xff);
      return value;
    }
}

uint16_type
main_memory::get_16(uint32_type address, function_code fc) const
{
  assert((address & 1) == 0);
  if (address % 0x1000000U >= end)
    throw bus_error_exception(true, fc, address);

  uint32_type i = address % 0x1000000U / 2;
  uint_type value = data[i];
  assert(value <= 0xffff);

  return value;
}

uint32_type
main_memory::get_32(uint32_type address, function_code fc) const
{
  assert((address & 3) == 0);
  if (address % 0x1000000U >= end)
    throw bus_error_exception(true, fc, address);

  uint32_type i = address % 0x1000000U / 2;
  uint_type value0 = data[i];
  uint_type value1 = data[i + 1];
  assert(value0 <= 0xffff);
  assert(value1 <= 0xffff);

  uint32_type value = uint32_type(value0) << 16 | value1;
  return value;
}

void
main_memory::put_8(uint32_type address, int value, function_code fc)
{
  value &= 0xff;

  if (address % 0x1000000U >= end
      || fc != memory::SUPER_DATA && address % 0x1000000U < super_area)
    throw bus_error_exception(false, fc, address);

  uint32_type i = address % 0x1000000U / 2;
  if (address % 2 != 0)
    {
      data[i] = data[i] & 0xff00 | value;
    }
  else
    {
      data[i] = data[i] & 0xff | value << 8;
    }
}

void
main_memory::put_16(uint32_type address, uint16_type value, function_code fc)
{
  assert((address & 1) == 0);
  value &= 0xffff;

  if (address % 0x1000000U >= end
      || fc != memory::SUPER_DATA && address % 0x1000000U < super_area)
    throw bus_error_exception(false, fc, address);

  uint32_type i = address % 0x1000000 / 2;
  data[i] = value;
}

void
main_memory::put_32(uint32_type address, uint32_type value, function_code fc)
{
  assert((address & 3) == 0);
  value &= 0xffffffffU;

  if (address % 0x1000000U >= end
      || fc != memory::SUPER_DATA && address % 0x1000000U < super_area)
    throw bus_error_exception(false, fc, address);

  uint32_type i = address % 0x1000000U / 2;
  data[i] = value >> 16;
  data[i + 1] = value & 0xffffu;
}

main_memory::~main_memory()
{
#ifdef DUMP_MEMORY
  int fd = open("dump", O_WRONLY | O_CREAT | O_TRUNC, 0666);
  ::write(fd, data, end);
  close(fd);
#endif
  free(data);
}

main_memory::main_memory(size_t n)
  : end((n + 1) / 2 * 2),
    super_area(0),
    data(NULL)
{
  data = static_cast<unsigned short *>(calloc(end / 2,
					      sizeof (unsigned short)));
#ifndef NDEBUG
  // These ILLEGAL instructions makes debugging easy.
  fill(data + 0, data + (end >> 1), 0x4afc);
#endif
}

