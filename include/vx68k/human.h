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

#include <vx68k/machine.h>

namespace vx68k
{
  using namespace vm68k;

  namespace human
  {
    class memory_allocator
    {
    private:
      address_space *_as;
      uint32_type limit;
      uint32_type last_block;

    public:
      memory_allocator(address_space *, uint32_type, uint32_type);

    public:
      sint32_type alloc(uint32_type len, uint32_type parent);
      sint32_type alloc_largest(uint32_type parent);
      sint_type free(uint32_type memptr);
      void free_by_parent(uint32_type parent);
      sint32_type resize(uint32_type memptr, uint32_type newlen);

    protected:
      void make_block(uint32_type, uint32_type, uint32_type, uint32_type);
      void remove_block(uint32_type block);
    };

    class file;

    class file_system
    {
    };

    class file
    {
    public:
      virtual ~file() {}
    };

    class dos_exec_context
      : public context
    {
    private:
      memory_allocator *_allocator;
      uint32_type current_pdb;

    private:
      int debug_level;

    public:
      dos_exec_context(address_space *, exec_unit *, memory_allocator *);

    public:
      uint32_type getpdb() const
	{return current_pdb;}
      void setpdb(uint32_type pdb)
	{current_pdb = pdb;}

      uint32_type load(const char *name, uint32_type arg, uint32_type env);
      void exit(unsigned int);

    public:
      sint32_type malloc(uint32_type len)
	{return _allocator->alloc(len, current_pdb);}
      sint_type mfree(uint32_type);
      sint32_type setblock(uint32_type memptr, uint32_type newlen)
	{return _allocator->resize(memptr, newlen);}

    public:
      sint_type create(const char *name, sint_type attr);
      sint_type open(const char *, sint_type);
      sint_type close(sint_type);
      sint32_type read(sint_type, uint32_type, uint32_type);
      sint32_type write(sint_type, uint32_type, uint32_type);
      int32 seek(int, int32, unsigned int);

      int fgetc(int);
      sint32_type fputc(sint_type, sint_type);
      sint32_type fputs(uint32_type, sint_type);

    public:
      uint32 load_executable(const char *, uint32_type address);
      uint16 start(uint32, const char *const *);

    public:
      void set_debug_level(int lev)
	{debug_level = lev;}
    };

    /* Dummy process for interface from POSIX to DOS.  */
    class shell
    {
    private:
      dos_exec_context *_ec;
      uint32_type pdb;

    public:
      shell(dos_exec_context *);
      ~shell();

    public:
      sint_type exec(const char *name, const char *const *argv,
		     const char *const *envp);
    };

    class dos
      : public virtual instruction_data
    {
    private:
      machine *vm;
      memory_allocator allocator;
      file_system fs;

    private:
      int debug_level;

    public:
      dos(machine *);

    public:
      uint32_type load(const char *name, dos_exec_context &c);
      uint16 execute (const char *, const char *const *);

    public:
      void set_debug_level(int lev)
	{debug_level = lev;}
    };
  } // human
} // vx68k

#endif
