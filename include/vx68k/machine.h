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

#ifndef _VX68K_MACHINE_H
#define _VX68K_MACHINE_H 1

#include <vx68k/memory.h>
#include <vm68k/cpu.h>

namespace vx68k
{
  using namespace vm68k;
  using namespace std;

  class machine
  {
  private:
    size_t _memory_size;
    main_memory_page main_memory;
    class address_space as;
    class exec_unit eu;
  public:
    explicit machine(size_t);
  public:
    size_t memory_size() const
      {return _memory_size;}
    class address_space *address_space()
      {return &as;}
    class exec_unit *exec_unit()
      {return &eu;}
  };
} // vx68k

#endif /* not _VX68K_MACHINE_H */

