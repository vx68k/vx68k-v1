/* vx68k - Virtual X68000
   Copyright (C) 1998, 1999 Hypercore Software Design, Ltd.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307, USA.  */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#undef const
#undef inline

#include <vx68k/human.h>

#include <fstream>
#include <algorithm>
#include <stdexcept>
#include <cstdlib>
#include <cstdio>

#ifdef HAVE_NANA_H
# include <nana.h>
# include <cstdio>
#else
# include <cassert>
# define I assert
# define VL(EXPR)
#endif

using namespace vx68k::human;
using namespace vm68k;
using namespace std;

sint32_type
dos_exec_context::read(uint_type fd, uint32_type dataptr, uint32_type size)
{
  if (fd >= NFILES || files[fd] == NULL)
    return -6;

  return files[fd]->read(mem, dataptr, size);
}

sint32_type
dos_exec_context::write(uint_type fd, uint32_type dataptr, uint32_type size)
{
  if (fd >= NFILES || files[fd] == NULL)
    return -6;

  return files[fd]->write(mem, dataptr, size);
}

sint_type
dos_exec_context::fgetc(uint_type fd)
{
  if (fd >= NFILES || files[fd] == NULL)
    return -6;

  return files[fd]->fgetc();
}

sint_type
dos_exec_context::fputc(sint_type code, uint_type filno)
{
  if (filno >= NFILES || files[filno] == NULL)
    return -6;

  return files[filno]->fputc(code);
}

sint32_type
dos_exec_context::fputs(uint32_type mesptr, uint_type filno)
{
  if (filno < 0 || filno >= NFILES || files[filno] == NULL)
    return -6;

  return files[filno]->fputs(mem, mesptr);
}

sint32_type
dos_exec_context::seek(uint_type fd, sint32_type offset, uint_type mode)
{
  if (fd >= NFILES || files[fd] == NULL)
    return -6;

  return files[fd]->seek(offset, mode);
}

/* Closes a DOS file descriptor.  */
sint_type
dos_exec_context::close(uint_type fd)
{
  if (fd >= NFILES || files[fd] == NULL)
    return -6;

  _fs->unref(files[fd]);
  files[fd] = NULL;
  return 0;
}

sint_type
dos_exec_context::dup(uint_type filno)
{
  if (filno >= NFILES || files[filno] == NULL)
    return -6;

  file **found = find(files + 0, files + NFILES, (file *) 0);
  if (found == files + NFILES)
    return -4;

  *found = _fs->ref(files[filno]);
  return found - (files + 0);
}

/* Opens a file.  */
sint_type
dos_exec_context::open(uint32_type nameptr, uint_type mode)
{
  file **found = find(files + 0, files + NFILES, (file *) 0);
  if (found == files + NFILES)
    return -4;

  sint_type err = _fs->open(*found, mem, nameptr, mode);
  return err < 0 ? err : found - (files + 0);
}

/* Creates a file.  */
sint_type
dos_exec_context::create(uint32_type nameptr, uint_type atr)
{
  file **found = find(files + 0, files + NFILES, (file *) 0);
  if (found == files + NFILES)
    return -4;

  sint_type err = _fs->create(*found, mem, nameptr, atr);
  return err < 0 ? err : found - (files + 0);
}

sint_type
dos_exec_context::mfree(uint32_type memptr)
{
  if (memptr == 0)
    {
      _allocator->free_by_parent(current_pdb);
      return 0;
    }

  return _allocator->free(memptr);
}

namespace
{
  struct quit_loop
  {
    uint16 status;
    quit_loop(uint16 s): status(s) {};
  };
}

void
dos_exec_context::exit(unsigned int status)
{
  throw quit_loop(status);
}

uint16
dos_exec_context::start(uint32 address, const char *const *argv)
{
  regs.pc = address;
  uint16 status = 0;
  try
    {
      run();
      abort ();
    }
  catch (quit_loop &x)
    {
      status = x.status;
    }
  catch (illegal_instruction &e)
    {
      uint16 op = mem->getw(SUPER_DATA, regs.pc);
      fprintf(stderr, "vm68k illegal instruction (op = 0x%x)\n", op);
      status = 0xff;
    }
  catch (bus_error &x)
    {
      uint_type op = mem->getw(SUPER_DATA, regs.pc);
      fprintf(stderr, "vm68k bus error (fc = %#x, address = %#lx, op = %#x)\n",
	      x.status, (unsigned long) x.address, op);
      status = 0xff;
    }

  return status;
}

uint32
dos_exec_context::load_executable(const char *name, uint32_type address)
{
  ifstream is (name, ios::in | ios::binary);
  if (!is)
    throw runtime_error("open error");
  char head[64];
  is.read (head, 64);
  if (!is)
    throw runtime_error("read error");
  if (head[0] != 'H' || head[1] != 'U')
    throw runtime_error("exec format error");

  uint32 base = getl (head + 4);
  uint32 start_offset = getl (head + 8);
  size_t text_size = getl (head + 12);
  size_t data_size = getl (head + 16);
  size_t bss_size = getl (head + 20);
  size_t reloc_size = getl (head + 24);
  if (debug_level >= 1)
    {
      fprintf(stderr, "Base : 0x%lx\n", (unsigned long) base);
      fprintf(stderr, "Start: 0x%lx\n", (unsigned long) start_offset);
      fprintf(stderr, "Text : %lu\n", (unsigned long) text_size);
      fprintf(stderr, "Data : %lu\n", (unsigned long) data_size);
      fprintf(stderr, "BSS  : %lu\n", (unsigned long) bss_size);
      fprintf(stderr, "Fixup: %lu\n", (unsigned long) reloc_size);
    }

  uint32 load_address = address + 0xf0;

  char *buf = static_cast<char *>(::malloc(text_size + data_size));
  try
    {
      is.read (buf, text_size + data_size);
      if (!is)
	throw runtime_error("read error");
      mem->write(SUPER_DATA, load_address,
		 buf, text_size + data_size);
    }
  catch (...)
    {
      free (buf);
      throw;
    }
  free (buf);

  /* Fix-up.  */
  char *fixup_buf = static_cast<char *>(::malloc(reloc_size));
  try
    {
      is.read(fixup_buf, reloc_size);
      if (!is)
	throw runtime_error("read error");

      const char *p = fixup_buf;
      uint32 address = load_address;
      while (p != fixup_buf + reloc_size)
	{
	  uint32 d = getw(p);
	  p += 2;
	  if (d == 1)		// Prefix for long offset.
	    {
	      d = getl(p);
	      p += 4;
	    }
	  if (d % 2 != 0)
	    {
	      fprintf(stderr, "Illegal fixup at an odd address\n");
	      throw runtime_error("exec format error");
	    }
	  address += d;
	  uint32 value = mem->getl(SUPER_DATA, address);
	  mem->putl(SUPER_DATA, address, value + load_address - base);
	  VL(("Fixup at 0x%lx (0x%lx -> 0x%lx)\n",
	      (unsigned long) address, (unsigned long) value,
	      (unsigned long) value + load_address - base));
	}
    }
  catch (...)
    {
      free(fixup_buf);
      throw;
    }
  free(fixup_buf);

  // PSP setup.
  mem->puts(SUPER_DATA, load_address - 128, name);
  regs.a[0] = load_address - 0x100;
  regs.a[1] = load_address + text_size + data_size + bss_size;

  return load_address + start_offset;
}

uint32_type
dos_exec_context::load(const char *name, uint32_type arg, uint32_type env)
{
  uint32_type pdb = _allocator->alloc_largest(current_pdb);

  uint32_type pdb_base = pdb - 0x10;
  mem->putl(SUPER_DATA, pdb_base + 0x10, env);
  mem->putl(SUPER_DATA, pdb_base + 0x20, arg);

  uint32_type start = load_executable(name, pdb);

  regs.a[2] = arg;
  regs.a[3] = env;
  regs.a[4] = start;

  return pdb;
}

sint_type
dos_exec_context::getenv(uint32_type getname, uint32_type env,
			 uint32_type getbuf)
{
  string name = mem->gets(SUPER_DATA, getname);

  // FIXME
  string s;
  const char *value = ::getenv(name.c_str());
  if (value != NULL)
    s = value;

  s.erase(255);
  mem->puts(SUPER_DATA, getbuf, s);

  return 0;
}

dos_exec_context::~dos_exec_context()
{
  for (reverse_iterator<file **> i(files + NFILES);
       i != reverse_iterator<file **>(files + 0);
       ++i)
    _fs->unref(*i);
  for (reverse_iterator<file **> i(std_files + 5);
       i != reverse_iterator<file **>(std_files + 0);
       ++i)
    _fs->unref(*i);
}

dos_exec_context::dos_exec_context(address_space *m, exec_unit *eu,
				   memory_allocator *a, file_system *fs)
  : context(m, eu),
    _allocator(a),
    _fs(fs),
    current_pdb(0),
    debug_level(0)
{
  fill(std_files + 0, std_files + 5, (file *) 0);
  fill(files + 0, files + NFILES, (file *) 0);
  current_pdb = _allocator->root();
  _fs->open(std_files[0], STDIN_FILENO);
  _fs->open(std_files[1], STDOUT_FILENO);
  _fs->open(std_files[2], STDERR_FILENO);
  std_files[3] = _fs->ref(std_files[2]); // FIXME
  std_files[4] = _fs->ref(std_files[2]); // FIXME
  files[0] = _fs->ref(std_files[0]);
  files[1] = _fs->ref(std_files[1]);
  files[2] = _fs->ref(std_files[2]);
  files[3] = _fs->ref(std_files[3]);
  files[4] = _fs->ref(std_files[4]);
}

