/* -*-C++-*- */
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

#ifndef VX68K_HUMAN_H
#define VX68K_HUMAN_H 1

#include "vm68k/cpu.h"
#include "vx68k/memory.h"

namespace vx68k
{
  using namespace vm68k;

  namespace human
  {
    class dos_exec_context
      : public context
    {
    public:
      dos_exec_context(address_space *, exec_unit *);
    public:
      void exit(unsigned int);
      int open(const char *, unsigned int);
      int close(int);
      int32 read(int, uint32, uint32);
      int32 write(int, uint32, uint32);
      int32 seek(int, int32, unsigned int);
      int fgetc(int);
      uint32 load_executable(const char *);
      uint16 start(uint32, const char *const *);
    };

    class dos
    {
    private:
      address_space *mem;
      vm68k::exec_unit main_cpu;
    public:
      dos (address_space *, size_t);
    public:
      uint16 execute (const char *, const char *const *);
    };
  } // human
} // vx68k

#endif
