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

#ifndef _VX68K_HUMAN_H
#define _VX68K_HUMAN_H 1

#include <vx68k/machine.h>
#include <map>

namespace vx68k
{
  using namespace vm68k;
  using namespace std;

  namespace human
  {
    class memory_allocator
    {
    private:
      address_space *_as;
      uint32_type limit;
      uint32_type root_block;
      uint32_type last_block;

    public:
      memory_allocator(address_space *, uint32_type, uint32_type);

    public:
      uint32_type root() const
	{return root_block + 0x10;}

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

    /* File system.  */
    class file_system
    {
    private:
      machine *_m;
      map<file *, int> files;

    public:
      explicit file_system(machine *);

    public:
      string export_file_name(const string &);
      sint_type chmod(const address_space *, uint32_type, sint_type);

    public:
      sint_type create(file *&, const address_space *, uint32_type, sint_type);
      void open(file *&, int);
      sint_type open(file *&, const string &, uint_type);
      sint_type open(file *&, const address_space *, uint32_type, sint_type);
      file *ref(file *);
      void unref(file *);
    };

    /* Abstract file.  */
    class file
    {
      friend void file_system::unref(file *);
    protected:
      virtual ~file() {}
    public:
      virtual sint32_type seek(sint32_type, uint_type) {return -1;}
      virtual sint32_type read(address_space *, uint32_type, uint32_type) = 0;
      virtual sint32_type write(const address_space *,
				uint32_type, uint32_type) = 0;
      virtual sint_type fgetc() = 0;
      virtual sint_type fputc(sint_type) = 0;
      virtual sint32_type fputs(const address_space *, uint32_type) = 0;
    };

    /* Regular file that maps onto a POSIX file.  */
    class regular_file
      : public file
    {
    private:
      int fd;
    public:
      regular_file(int f);
    protected:
      ~regular_file();
    public:
      sint32_type seek(sint32_type, uint_type);
      sint32_type read(address_space *, uint32_type, uint32_type);
      sint32_type write(const address_space *, uint32_type, uint32_type);
      sint_type fgetc();
      sint_type fputc(sint_type);
      sint32_type fputs(const address_space *, uint32_type);
    };

    const size_t NFILES = 96;

    class dos_exec_context
      : public context
    {
    private:
      exec_unit *_eu;		// FIXME
      memory_allocator *_allocator;
      file_system *_fs;
      uint32_type current_pdb;
      file *std_files[5];
      file *files[NFILES];

    private:
      int debug_level;

    public:
      dos_exec_context(address_space *, exec_unit *,
		       memory_allocator *, file_system *);
      ~dos_exec_context();

    public:
      uint32_type getpdb() const
	{return current_pdb;}
      void setpdb(uint32_type pdb)
	{current_pdb = pdb;}
      sint_type getenv(uint32_type, uint32_type, uint32_type);

      uint32_type load(const char *name, uint32_type arg, uint32_type env);
      void exit(unsigned int);

    public:
      sint32_type malloc(uint32_type len)
	{return _allocator->alloc(len, current_pdb);}
      sint_type mfree(uint32_type);
      sint32_type setblock(uint32_type memptr, uint32_type newlen)
	{return _allocator->resize(memptr, newlen);}

    public:
      sint_type create(uint32_type nameptr, uint_type attr);
      sint_type open(uint32_type nameptr, uint_type);
      sint_type dup(uint_type);
      sint_type close(uint_type);
      sint32_type read(uint_type, uint32_type, uint32_type);
      sint32_type write(uint_type, uint32_type, uint32_type);
      sint32_type seek(uint_type, sint32_type, uint_type);

      sint_type fgetc(uint_type);
      sint_type fputc(sint_type, uint_type);
      sint32_type fputs(uint32_type, uint_type);

    public:
      uint32_type load_executable(const char *, uint32_type address);
      uint_type start(uint32_type, const char *const *);

    public:
      void set_debug_level(int lev)
	{debug_level = lev;}
    };

    /* Pseudo process for interface from POSIX to DOS.  */
    class shell
    {
    private:
      dos_exec_context *_context;
      uint32_type pdb;

    public:
      shell(dos_exec_context *);
      ~shell();

    protected:
      uint32_type create_env(const char *const *envp);

    public:
      int exec(const char *name, const char *const *argv,
	       const char *const *envp);
    };

    class dos
      : public instruction_data
    {
    private:
      class machine *vm;
      memory_allocator allocator;
      file_system _fs;

    private:
      int debug_level;

    public:
      dos(class machine *);

    public:
      class machine *machine()
        {return vm;}
      file_system *fs()
	{return &_fs;}
      dos_exec_context *create_context();

    public:
      void set_debug_level(int lev)
	{debug_level = lev;}
    };
  } // human
} // vx68k

#endif /* not _VX68K_HUMAN_H */

