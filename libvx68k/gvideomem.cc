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

#include <cstdio>

#ifdef HAVE_NANA_H
# include <nana.h>
#else
# include <cassert>
# define I assert
#endif

using vx68k::graphics_video_memory;
using namespace vm68k::types;
using namespace std;

uint16_type
graphics_video_memory::get_16(uint32_type address, function_code fc) const
  throw (memory_exception)
{
#ifdef DL
  DL("class graphics_video_memory: get_16: fc=%d address=0x%08lx\n",
     fc, (unsigned long) address);
#endif
  static bool once;
  if (!once++)
    fprintf(stderr,
	    "class graphics_video_memory: FIXME: `get_16' not implemented\n");
  return 0;
}

int
graphics_video_memory::get_8(uint32_type address, function_code fc) const
  throw (memory_exception)
{
#ifdef DL
  DL("class graphics_video_memory: get_8: fc=%d address=0x%08lx\n",
     fc, (unsigned long) address);
#endif
  static bool once;
  if (!once++)
    fprintf(stderr,
	    "class graphics_video_memory: FIXME: `get_8' not implemented\n");
  return 0;
}

void
graphics_video_memory::put_16(uint32_type address, uint16_type value,
			      function_code fc)
  throw (memory_exception)
{
#ifdef DL
  DL("class graphics_video_memory: put_16: fc=%d address=0x%08lx value=0x%04x\n",
     fc, (unsigned long) address, value);
#endif
  static bool once;
  if (!once++)
    fprintf(stderr,
	    "class graphics_video_memory: FIXME: `put_16' not implemented\n");
}

void
graphics_video_memory::put_8(uint32_type address, int value, function_code fc)
  throw (memory_exception)
{
#ifdef DL
  DL("class graphics_video_memory: put_8: fc=%d address=0x%08lx value=0x%02x\n",
     fc, (unsigned long) address, value);
#endif
  static bool once;
  if (!once++)
    fprintf(stderr,
	    "class graphics_video_memory: FIXME: `put_8' not implemented\n");
}
