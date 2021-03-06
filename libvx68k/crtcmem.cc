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

#include <cstdio>

#ifdef HAVE_NANA_H
# include <nana.h>
#else
# include <cassert>
# define I assert
#endif

using vx68k::crtc_memory;
using vm68k::mutex_lock;
using namespace vm68k::types;
using namespace std;

void
crtc_memory::reset(console::time_type t)
{
  vdisp_start_time = t;
}

void
crtc_memory::check_timeouts(console::time_type t, context &c)
{
  mutex_lock lock(&mutex);

  if (t - vdisp_start_time >= vdisp_interval)
    {
      vdisp_start_time += vdisp_interval;

      if (vdisp_interrupt_enabled())
	{
	  I(vdisp_counter_value > 0);
	  if (--vdisp_counter_value == 0)
	    {
	      vdisp_counter_value = vdisp_counter_data;
	      c.interrupt(6, 0x4d);
	    }
	}
    }
}

void
crtc_memory::set_vdisp_counter_data(unsigned int value)
{
  mutex_lock lock(&mutex);

  vdisp_counter_data = value;
  if (vdisp_interrupt_enabled())
    vdisp_counter_value = vdisp_counter_data;
}

uint16_type
crtc_memory::get_16(uint32_type address, function_code fc) const
  throw (memory_exception)
{
#ifdef DL
  DL("class crtc_memory: get_16: fc=%d address=0x%08lx\n", fc, address + 0UL);
#endif

  address &= 0x1fff;
  switch (address) {
  case 0x28:
    return 0x417;
  default:
    return 0;
  }
}

int
crtc_memory::get_8(uint32_type address, function_code fc) const
  throw (memory_exception)
{
#ifdef DL
  DL("class crtc_memory: get_8: fc=%d address=0x%08lx\n", fc, address + 0UL);
#endif

  uint16_type w = get_16(address & ~1u, fc);
  if (address & 1u)
    return w >> 8 & 0xff;
  else
    return w & 0xff;
}

void
crtc_memory::put_16(uint32_type address, uint16_type value, function_code fc)
  throw (memory_exception)
{
#ifdef DL
  DL("class crtc_memory: put_16: fc=%d address=0x%08lx value=0x%04x\n",
     fc, address + 0UL, value);
#endif

  static bool once;
  if (!once++)
    fprintf(stderr, "class crtc_memory: FIXME: `put_16' not implemented\n");
}

void
crtc_memory::put_8(uint32_type address, int value, function_code fc)
  throw (memory_exception)
{
#ifdef DL
  DL("class crtc_memory: put_8: fc=%d address=0x%08lx value=0x%02x\n",
     fc, address + 0UL, value);
#endif

  static bool once;
  if (!once++)
    fprintf(stderr, "class crtc_memory: FIXME: `put_8' not implemented\n");
}

crtc_memory::~crtc_memory()
{
  pthread_mutex_destroy(&mutex);
}

crtc_memory::crtc_memory()
  : vdisp_interval(1000 / 55),
    vdisp_counter_data(0)
{
  pthread_mutex_init(&mutex, NULL);
}
