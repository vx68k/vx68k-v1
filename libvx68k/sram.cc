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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#undef const
#undef inline

#include <vx68k/machine.h>

#include <sys/mman.h>
#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <stdexcept>

#ifdef HAVE_NANA_H
# include <nana.h>
# include <cstdio>
#else
# include <cassert>
# define I assert
#endif

using vx68k::sram;
using vm68k::getw;
using vm68k::getl;
using vm68k::putl;
using namespace vm68k::types;
using namespace std;

uint_type
sram::get_16(int fc, uint32_type address) const
{
#ifdef HAVE_NANA_H
  L("class sram: get_16 fc=%d address=%#010x\n", fc, address);
#endif
  address &= 0x3fff;
  uint_type value = ::getw(buf + address);
  return value;
}

uint_type
sram::get_8(int fc, uint32_type address) const
{
#ifdef HAVE_NANA_H
  L("class sram: get_8 fc=%d address=%#010x\n", fc, address);
#endif
  address &= 0x3fff;
  uint_type value = *(buf + address) & 0xffu;
  return value;
}

void
sram::put_16(int, uint32_type, uint_type)
{
#ifdef HAVE_NANA_H
  L("class sram: FIXME: `put_16' not implemented\n");
#endif
}

void
sram::put_8(int, uint32_type, uint_type)
{
#ifdef HAVE_NANA_H
  L("class sram: FIXME: `put_8' not implemented\n");
#endif
}

sram::~sram()
{
  munmap(buf, 16 * 1024);
}

sram::sram()
  : buf(NULL)
{
  int fildes = open("sram", O_RDWR | O_CREAT, 0666);
  off_t off = lseek(fildes, 0, SEEK_END);
  if (off < 16 * 1024)
    {
      lseek(fildes, 16 * 1024 - 1, SEEK_SET);
      ::write(fildes, "", 1);
    }

  buf = (unsigned char *) mmap(0, 16 * 1024, PROT_READ | PROT_WRITE,
			       MAP_SHARED, fildes, 0);

  if (::getl(buf + 8) == 0)
    ::putl(buf + 8, 4 * 1024 * 1024);

  if (*(buf + 0x1d) == 0)
    *(buf + 0x1d) = 16;
}
