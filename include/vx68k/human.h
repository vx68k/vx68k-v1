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
    class file
    {
    public:
      virtual ~file() {}
    };

    class file_system
    {
    };

    class process
    {
    public:
      void exit(sint_type);
      sint32_type malloc(uint32_type);
      sint_type mfree(sint32_type);
      sint32_type setblock(sint32_type, uint32_type);
    public:
      sint_type create(const char *name, sint_type attr);
      sint_type open(const char *name, sint_type mode);
      sint_type close(sint_type);
      sint32_type read(sint_type, uint32_type, uint32_type);
      sint32_type write(sint_type, uint32_type, uint32_type);
      sint32_type seek(sint_type, sint32_type, uint_type);
      sint_type dup(sint_type);
      sint_type dup2(sint_type, sint_type);
    };

    class dos_exec_context
      : public context
    {
    private:
      process *_process;

    public:
      dos_exec_context(exec_unit *, address_space *, process *);

    public:
      process *current_process() const
	{return _process;}

      void exit(unsigned int);
      int open(const char *, unsigned int);
      int close(int);
      int32 read(int, uint32, uint32);
      sint32_type write(int, uint32_type, uint32_type);
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
      file_system fs;
    public:
      dos (address_space *, size_t);
    public:
      uint16 execute (const char *, const char *const *);
    };
  } // human
} // vx68k

#endif
