/* Virtual X68000 - Sharp X68000 emulator
   Copyright (C) 1998, 2000 Hypercore Software Design, Ltd.

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

/* NOTE

   The IOCS implementation is being moved to class system_rom.  */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#undef const
#undef inline

#include <vx68k/machine.h>

#include <algorithm>
#include <memory>
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
machine::b_print(const address_space *as, uint32_type strptr)
{
  const string str = as->gets(SUPER_DATA, strptr);

  for (string::const_iterator i = str.begin();
       i != str.end();
       ++i)
    {
      b_putc((unsigned char) *i);
    }
}

sint32_type
machine::read_disk(address_space &as, uint_type mode, uint32_type pos,
		   uint32_type buf, uint32_type nbytes)
{
  uint_type u = mode >> 8 & 0xf;

#ifdef HAVE_NANA_H
  L("machine: reading disk %#x %#x %#x %#x\n", mode, pos, buf, nbytes);
#endif
  switch (mode >> 12)
    {
    case 0x9:
      if (u >= NFDS)
	throw range_error("read_disk");

      return fd[u]->read(mode, pos, as, buf, nbytes);

    default:
      abort();
    }
}

sint32_type
machine::write_disk(const address_space &as, uint_type mode, uint32_type pos,
		    uint32_type buf, uint32_type nbytes) const
{
  uint_type u = mode >> 8 & 0xf;

#ifdef HAVE_NANA_H
  L("machine: writing disk %#x %#x %#x %#x\n", mode, pos, buf, nbytes);
#endif
  switch (mode >> 12)
    {
    case 0x9:
      if (u >= NFDS)
	throw range_error("write_disk");

      return fd[u]->write(mode, pos, as, buf, nbytes);

    default:
      abort();
    }
}

void
machine::boot(context &c)
{
  /* c.mem must be of class x68k_address_space.  */

  sint32_type st = read_disk(*c.mem, 0x9000, 0x03000001, 0x2000, 1024);
  if (st >> 24 & 0xc0)
    {
#ifdef HAVE_NANA_H
      L("machine: boot error %#x\n", st);
#endif
      throw runtime_error("machine");
    }

  c.regs.pc = 0x2000;
  eu.run(c);
}

void
machine::boot()
{
  x68k_address_space as(this);
  context c(&as);
  boot(c);
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

void
machine::load_fd(unsigned int u, int fildes)
{
  if (u >= NFDS)
    throw range_error("machine");

  auto_ptr<iocs::image_file_floppy_disk> d
    (new iocs::image_file_floppy_disk(fildes));

  unload_fd(u);
  fd[u] = d.release();
}

void
machine::unload_fd(unsigned int u)
{
  if (u >= NFDS)
    throw range_error("machine");

  delete fd[u];
  fd[u] = NULL;
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

void
machine::configure(address_space &as)
{
  as.set_pages(0 >> PAGE_SHIFT, _memory_size >> PAGE_SHIFT, &mem);
#if 0
  as.set_pages(0xc00000 >> PAGE_SHIFT, 0xe00000 >> PAGE_SHIFT, &graphic_vram);
#endif
  as.set_pages(0xe00000 >> PAGE_SHIFT, 0xe80000 >> PAGE_SHIFT, &tvram);
  as.set_pages(0xfc0000 >> PAGE_SHIFT, 0x1000000 >> PAGE_SHIFT, &rom);

  rom.initialize(as);
}

machine::~machine()
{
  for (iocs::disk **i = fd + 0; i != fd + NFDS; ++i)
    delete *i;

  rom.detach(&eu);

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

  fill(fd + 0, fd + NFDS, (iocs::disk *) NULL);

  rom.attach(&eu);
}
