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

#ifndef VX68K_MEMORY_H
#define VX68K_MEMORY_H 1

#include "vm68k/memory.h"

namespace vx68k
{

using namespace vm68k::types;
using vm68k::bus_error;

class main_memory_page
  : public vm68k::memory_page
{
public:
  explicit main_memory_page (size_t);
  ~main_memory_page ();
  void read (int, uint32, void *, size_t) const throw (bus_error);
  void write (int, uint32, const void *, size_t) throw (bus_error);
  uint8 getb (int, uint32) const throw (bus_error);
  uint16 getw (int, uint32) const throw (bus_error);
  void putb (int, uint32, uint8) throw (bus_error);
  void putw (int, uint32, uint16) throw (bus_error);
};

class address_space
  : public vm68k::address_space
{
public:
  explicit address_space (size_t);
private:
  main_memory_page main_memory;
};

};				// namespace vx68k

#endif
