/* Virtual X68000 - X68000 virtual machine
   Copyright (C) 1998-2002 Hypercore Software Design, Ltd.

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
# include <config.h>
#endif
#undef const
#undef inline

#include <vx68k/dos>

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
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

using namespace std;

namespace vx68k
{
  main_memory::main_memory(uint32_type size)
    : _size((size + 1) / 2 * 2),
      _protected_size(0),
      content(0)
  {
    content = static_cast<unsigned short *>(malloc(_size));
#ifndef NDEBUG
    // These ILLEGAL instructions makes debugging easy.
    fill(content + 0, content + _size / 2, 0x4afc);
#endif
  }

  void
  main_memory::set_protected_size(uint32_type protected_size)
  {
    _protected_size = protected_size;
  }
}

namespace vx68k
{
  int
  main_memory::get_8(uint32_type address, function_code fc) const
    throw (bus_error)
  {
    uint32_type i = address & 0xffffff;
    if (i >= _size)
      throw bus_error(address, READ | fc);

    int value;
    switch (i & 1)
      {
      case 0:
	value = content[i / 2] >> 8;
	I(value <= 0xff);
	break;

      case 1:
	value = content[i / 2] & 0xff;
	break;

      default:
	abort();
      }

    return value;
  }

  uint16_type
  main_memory::get_16(uint32_type address, function_code fc) const
    throw (bus_error)
  {
    I((address & 1) == 0);
    uint32_type i = address & 0xffffff;
    if (i >= _size)
      throw bus_error(address, READ | fc);

    uint16_type value = content[i / 2];
    I(value <= 0xffff);
    return value;
  }

  uint32_type
  main_memory::get_32(uint32_type address, function_code fc) const
    throw (bus_error)
  {
    assert((address & 3) == 0);
    if (address % 0x1000000U >= _size)
      throw bus_error(address, READ | fc);

    uint32_type i = address % 0x1000000U / 2;
    uint16_type value0 = content[i];
    uint16_type value1 = content[i + 1];
    assert(value0 <= 0xffff);
    assert(value1 <= 0xffff);

    uint32_type value = uint32_type(value0) << 16 | value1;
    return value;
  }

  void
  main_memory::put_8(uint32_type address, int value, function_code fc)
    throw (bus_error)
  {
    value &= 0xff;

    if (address % 0x1000000U >= _size
	|| fc != SUPER_DATA && address % 0x1000000U < _protected_size)
      throw bus_error(address, WRITE | fc);

    uint32_type i = address % 0x1000000U / 2;
    if (address % 2 != 0)
      {
	content[i] = content[i] & 0xff00 | value;
      }
    else
      {
	content[i] = content[i] & 0xff | value << 8;
      }
  }

  void
  main_memory::put_16(uint32_type address, uint16_type value, function_code fc)
    throw (bus_error)
  {
    assert((address & 1) == 0);
    value &= 0xffff;

    if (address % 0x1000000U >= _size
	|| fc != SUPER_DATA && address % 0x1000000U < _protected_size)
      throw bus_error(address, WRITE | fc);

    uint32_type i = address % 0x1000000 / 2;
    content[i] = value;
  }

  void
  main_memory::put_32(uint32_type address, uint32_type value, function_code fc)
    throw (bus_error)
  {
    assert((address & 3) == 0);
    value &= 0xffffffffU;

    if (address % 0x1000000U >= _size
	|| fc != SUPER_DATA && address % 0x1000000U < _protected_size)
      throw bus_error(address, WRITE | fc);

    uint32_type i = address % 0x1000000U / 2;
    content[i] = value >> 16;
    content[i + 1] = value & 0xffffu;
  }

  main_memory::~main_memory()
  {
#ifdef DUMP_MEMORY
    int fd = open("dump", O_WRONLY | O_CREAT | O_TRUNC, 0666);
    write(fd, content, _size);
    close(fd);
#endif
    free(content);
  }
}
