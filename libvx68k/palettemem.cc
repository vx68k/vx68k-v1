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
#include <vm68k/mutex.h>

#ifdef HAVE_NANA_H
# include <nana.h>
# include <cstdio>
#else
# include <cassert>
# define I assert
#endif

using vx68k::palettes_memory;
using vm68k::bus_error_exception;
using vm68k::mutex_lock;
using namespace vm68k::types;
using namespace std;

bool
palettes_memory::check_text_colors_modified()
{
  mutex_lock lock(&mutex);

  bool tmp = text_colors_modified;
  text_colors_modified = false;
  return tmp;
}

void
palettes_memory::get_text_colors(unsigned int first, unsigned int last,
				 unsigned short *out)
{
  mutex_lock lock(&mutex);

  while (first != last)
    {
      *out++ = _tpalette[first++];
    }
}

void
palettes_memory::get_text_colors(unsigned int first, unsigned int last,
				 unsigned char *out)
{
  mutex_lock lock(&mutex);

  while (first != last)
    {
      uint_type value = _tpalette[first++];
      if (value == 0)
	{
	  *out++ = 0;
	  *out++ = 0;
	  *out++ = 0;
	  *out++ = 0;
	}
      else
	{
	  unsigned int x = value & 0x1;
	  unsigned int r = value >> 5 & 0x3e | x;
	  unsigned int g = value >> 10 & 0x3e | x;
	  unsigned int b = value & 0x3f;
	  *out++ = r * 0xff / 0x3f;
	  *out++ = g * 0xff / 0x3f;
	  *out++ = b * 0xff / 0x3f;
	  *out++ = 0xff;
	}
    }
}

uint_type
palettes_memory::get_16(function_code fc, uint32_type address) const
{
  address &= 0xfffffffeu;
#ifdef HAVE_NANA_H
  DL("class palettes_memory: get_16: fc=%d address=0x%08lx\n",
     fc, address + 0UL);
#endif

  if (fc != SUPER_DATA)
    throw bus_error_exception(true, fc, address);

  uint32_type off = address & 0x1fff;
  if (off >= 2 * 256 * 2)
    {
      switch (off)
	{
	case 0x400:
	  return 0;

	case 0x500:
	  return 0;

	case 0x600:
	  return 0;

	default:
	  return 0;
	}
    }
  else if (off >= 256 * 2)
    {
      unsigned int i = (off - 256 * 2) / 2;
      uint_type value = _tpalette[i];
      return value;
    }
  else
    {
      return 0;
    }
}

unsigned int
palettes_memory::get_8(function_code fc, uint32_type address) const
{
  address &= 0xffffffffU;
#ifdef HAVE_NANA_H
  DL("class palettes_memory: get_8: fc=%d address=0x%08lx\n",
     fc, address + 0UL);
#endif

  uint_type w = get_16(fc, address);
  if (address % 2 != 0)
    return w >> 8 & 0xff;
  else
    return w & 0xff;
}

void
palettes_memory::put_16(function_code fc, uint32_type address, uint_type value)
{
  address &= 0xfffffffeu;
  value &= 0xffffu;
#ifdef HAVE_NANA_H
  DL("class palettes_memory: put_16: fc=%d address=0x%08lx value=0x%04x\n",
     fc, address + 0UL, value);
#endif

  if (fc != SUPER_DATA)
    throw bus_error_exception(false, fc, address);

  uint32_type off = address & 0x1fff;
  if (off >= 2 * 256 * 2)
    {
      switch (off)
	{
	case 0x400:
	  break;

	case 0x500:
	  break;

	case 0x600:
	  break;

	default:
	  break;
	}
    }
  else if (off >= 256 * 2)
    {
      mutex_lock lock(&mutex);

      unsigned int i = (off - 256 * 2) / 2;
      _tpalette[i] = value;
      text_colors_modified = true;
    }
  else
    {
    }
}

void
palettes_memory::put_8(function_code fc, uint32_type address, unsigned int value)
{
  address &= 0xffffffffU;
  value &= 0xff;
#ifdef HAVE_NANA_H
  DL("class palettes_memory: put_16: fc=%d address=0x%08lx value=0x%02x\n",
     fc, address + 0UL, value);
#endif

  uint_type w = this->get_16(fc, address);
  if (address % 2 != 0)
    this->put_16(fc, address, w & 0xff | value << 8);
  else
    this->put_16(fc, address, w & ~0xff | value);
}

palettes_memory::~palettes_memory()
{
  pthread_mutex_destroy(&mutex);
}

palettes_memory::palettes_memory()
  : _tpalette(256, 0),
    text_colors_modified(false)
{
  _tpalette[0] = 0;
  _tpalette[1] = 0xf83e;
  _tpalette[2] = 0xffc0;
  _tpalette[3] = 0xfffe;
  fill(_tpalette.begin() + 4, _tpalette.begin() + 8, 0xde6c);
  fill(_tpalette.begin() + 8, _tpalette.begin() + 16, 0x4022);

  pthread_mutex_init(&mutex, NULL);
}
