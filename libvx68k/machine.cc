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

#include <algorithm>
#include <stdexcept>

using namespace vx68k;
using namespace vm68k;
using namespace std;

#ifdef HAVE_NANA_H
# include <nana.h>
# include <cstdio>
#else
# include <cassert>
# define I assert
#endif

void
machine::b_putc(uint_type code)
{
  /* FIXME.  This function must handle double-byte character.  */
  if (code <= 0x1f || code == 0x7f)
    {
      saved_byte1 = 0;

      switch (code)
	{
	case 0x09:		// HT
	  curx = (curx + 8) & ~7;
	  break;
	  
	case 0x0a:		// LF
	  ++cury;
	  if (cury == 31)
	    {
	      --cury;
	      tvram.scroll();
	    }
	  break;

	case 0x0d:		// CR
	  curx = 0;
	  break;
	}
    }
  else
    {
      if (saved_byte1 != 0)
	{
	  if (code <= 0xff)
	    code |= saved_byte1 << 8;

	  saved_byte1 = 0;
	}

      if (code >= 0x100 && curx + 1 == 96)
	++curx;

      if (curx == 96)
	{
	  curx = 0;
	  ++cury;

	  if (cury == 31)
	    {
	      --cury;
	      tvram.scroll();
	    }
	}

      if ((code >= 0x80 && code <= 0x9f)
	  || (code >= 0xe0 && code <= 0xff))
	saved_byte1 = code;
      else
	{
	  tvram.draw_char(curx, cury, code);
	  ++curx;
	  if (code >= 0x100)
	    ++curx;
	}
    }
}

void
machine::b_print(uint32_type strptr)
{
  const string str = as.gets(SUPER_DATA, strptr);

  for (string::const_iterator i = str.begin();
       i != str.end();
       ++i)
    {
      b_putc((unsigned char) *i);
    }
}

void
machine::queue_key(uint_type key)
{
  pthread_mutex_lock(&key_queue_mutex);

  try
    {
      key_queue.push(key);
      pthread_cond_signal(&key_queue_not_empty);
    }
  catch (...)
    {
      pthread_mutex_unlock(&key_queue_mutex);
      throw;
    }

  pthread_mutex_unlock(&key_queue_mutex);
}

uint_type
machine::get_key()
{
  uint_type key;

  pthread_mutex_lock(&key_queue_mutex);

  try
    {
      while (key_queue.empty())
	pthread_cond_wait(&key_queue_not_empty, &key_queue_mutex);

      key = key_queue.front();
      key_queue.pop();
    }
  catch (...)
    {
      pthread_mutex_unlock(&key_queue_mutex);
      throw;
    }

  pthread_mutex_unlock(&key_queue_mutex);

  return key;
}

/* IOCS functions.  */

namespace
{
  void
  iocs_b_lpeek(context &c, machine &m, iocs_function_data *data)
  {
    uint32_type address = c.regs.a[1];
#ifdef L
    L("| address = %#10x\n", address);
#endif

    c.regs.d[0] = m.address_space()->getl(SUPER_DATA, address);
    c.regs.a[1] = address + 4;
  }

  void
  set_iocs_functions(machine &m)
  {
    m.set_iocs_function(0x84, &iocs_b_lpeek, NULL);
  }
} // (unnamed namespace)

void
machine::invalid_iocs_function(context &c, machine &m,
			       iocs_function_data *data)
{
  throw runtime_error("invalid iocs function");	// FIXME
}

void
machine::iocs(uint_type op, context &c, instruction_data *data)
{
  unsigned int funcno = c.regs.d[0] & 0xffu;
#ifdef L
  L(" trap #15\t| IOCS %#4x\n", funcno);
#endif

  machine *m = static_cast<machine *>(data);
  I(m != NULL);
  (*m->iocs_functions[funcno].first)(c, *m, m->iocs_functions[funcno].second);

  c.regs.pc += 2;
}

void
machine::set_iocs_function(unsigned int i, iocs_function_handler handler,
			   iocs_function_data *data)
{
  if (i > 0xffu)
    throw range_error("iocs function must be between 0 and 0xff");

  iocs_functions[i] = make_pair(handler, data);
}

void
machine::get_image(int x, int y, int width, int height,
		   unsigned char *rgb_buf, size_t row_size)
{
  for (int i = 0; i != height; ++i)
    for (int j = 0; j != width; ++j)
      {
	unsigned char *p = rgb_buf + i * row_size + j * 3;
	p[0] = 0;
	p[1] = 0;
	p[2] = 0;
      }

  tvram.get_image(x, y, width, height, rgb_buf, row_size);
}

machine::~machine()
{
  for (iocs::disk **i = fd + 0; i != fd + NFDS; ++i)
    delete *i;

  pthread_mutex_destroy(&key_queue_mutex);
  pthread_cond_destroy(&key_queue_not_empty);
}

machine::machine(size_t memory_size)
  : _memory_size(memory_size),
    mem(memory_size),
    curx(0), cury(0),
    saved_byte1(0)
{
  pthread_cond_init(&key_queue_not_empty, NULL);
  pthread_mutex_init(&key_queue_mutex, NULL);

  fill(iocs_functions + 0, iocs_functions + 0x100,
       make_pair(&invalid_iocs_function, (iocs_function_data *) 0));

  fill(fd + 0, fd + NFDS, (iocs::disk *) NULL);

  as.set_pages(0 >> PAGE_SHIFT, _memory_size >> PAGE_SHIFT, &mem);
#if 0
  as.set_pages(0xc00000 >> PAGE_SHIFT, 0xe00000 >> PAGE_SHIFT, &graphic_vram);
#endif
  as.set_pages(0xe00000 >> PAGE_SHIFT, 0xe80000 >> PAGE_SHIFT, &tvram);

  eu.set_instruction(0x4e4f, 0, &iocs, this);

  set_iocs_functions(*this);
}
