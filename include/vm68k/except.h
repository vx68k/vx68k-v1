/* -*-C++-*- */
/* vx68k - Virtual X68000
   Copyright (C) 1998 Hypercore Software Design, Ltd.

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

#ifndef _VM68K_EXCEPT_H
#define _VM68K_EXCEPT_H 1

#include <vm68k/types.h>
#include <exception>

namespace vm68k
{
  using namespace std;

struct bus_error
  : exception
{
  enum {WRITE = 0, READ = 0x10};
  int status;
  uint32 address;
  bus_error (int, uint32);
};

struct address_error
  : exception
{
};

struct illegal_instruction
  : exception
{
};

struct zero_divide
  : exception
{
};

struct privilege_violation
  : exception
{
};

} // vm68k

#endif /* not _VM68K_EXCEPT_H */

