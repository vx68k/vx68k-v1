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

#ifdef HAVE_FCNTL_H
# include <fcntl.h>
#endif
#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif
#include <fstream>
#include <algorithm>
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
dos_exec_context::read(sint_type fd, uint32_type buf, uint32_type size)
{
  // FIXME.
  unsigned char *data_buf = new unsigned char [size];

  ssize_t done = ::read(fd, data_buf, size);
  if (done == -1)
    {
      delete [] data_buf;
      return -6;			// FIXME.
    }

  mem->write(SUPER_DATA, buf, data_buf, done);
  delete [] data_buf;
  return done;
}

sint32_type
dos_exec_context::write(sint_type fd, uint32_type buf, uint32_type size)
{
  // FIXME.
  unsigned char *data_buf = new unsigned char [size];
  mem->read(SUPER_DATA, buf, data_buf, size);

  ssize_t done = ::write(fd, data_buf, size);
  if (done == -1)
    {
      delete [] data_buf;
      return -6;			// FIXME.
    }

  delete [] data_buf;
  return done;
}

int
dos_exec_context::fgetc(int fd)
{
  unsigned char data_buf[1];
  ssize_t done = ::read(fd, data_buf, 1);
  if (done == -1)
    return -6;			// FIXME.
  return data_buf[0];
}

int32
dos_exec_context::seek(int fd, int32 offset, unsigned int whence)
{
  // FIXME.
  int32 pos = ::lseek(fd, offset, whence);
  if (pos == -1)
    return -6;			// FIXME.
  return pos;
}

/* Closes a DOS file descriptor.  */
sint_type
dos_exec_context::close(sint_type fd)
{
  // FIXME.
  if (::close(fd) == -1)
    return -6;			// FIXME.

  return 0;
}

/* Opens a file.  */
sint_type
dos_exec_context::open(const char *name, sint_type flag)
{
  // FIXME.
  static const int uflag[] = {O_RDONLY, O_WRONLY, O_RDWR};

  if ((flag & 0xf) > 2)
    return -12;			// FIXME.

  sint_type fd = ::open(name, uflag[flag & 0xf]);
  if (fd == -1)
    return -2;			// FIXME: errno test.

  return fd;
}

/* Creates a file.  */
sint_type
dos_exec_context::create(const char *name, sint_type attr)
{
  // FIXME.
  sint_type fd = ::open(name, O_RDWR | O_CREAT | O_TRUNC, 0666);
  if (fd == -1)
    return -2;			// FIXME: errno test.

  return fd;
}

sint32_type
dos_exec_context::fputc(sint_type code, sint_type filno)
{
  // FIXME.
  unsigned char buf[1];
  buf[0] = code;
  ::write(filno, buf, 1);

  return 1;
}

sint32_type
dos_exec_context::fputs(uint32_type mesptr, sint_type filno)
{
  // FIXME.
  uint32_type ptr = mesptr;
  unsigned char buf[1];
  do
    {
      buf[0] = mem->getb(SUPER_DATA, ptr++);
      if (buf[0] != 0)
	::write(filno, buf, 1);
    }
  while (buf[0] != 0);

  return ptr - 1 - mesptr;
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

  return status;
}

uint32
dos_exec_context::load_executable(const char *name, uint32_type address)
{
  ifstream is (name, ios::in | ios::binary);
  if (!is)
    abort ();			// FIXME
  char head[64];
  is.read (head, 64);
  if (!is)
    abort ();			// FIXME
  if (head[0] != 'H' || head[1] != 'U')
    abort ();			// FIXME

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
	abort ();		// FIXME
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
	abort();		// FIXME.

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
	      abort();		// FIXME.
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
  mem->write(SUPER_DATA, load_address - 128,
	     name, strlen(name) + 1);
  regs.a[0] = load_address - 0x100;
  regs.a[1] = load_address + text_size + data_size + bss_size;
  regs.a[2] = 0x7000;		// FIXME.
  regs.a[3] = 0x7000;		// FIXME.
  regs.a[4] = load_address + start_offset;

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
  // FIXME.
  char name[256];
  mem->read(SUPER_DATA, getname, name, 256);

  const char *value = ::getenv(name);
  if (value == NULL)
    value = "";

  // FIXME
  mem->write(SUPER_DATA, getbuf, value, strlen(value) + 1);

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
				   memory_allocator *a)
  : context(m, eu),
    _allocator(a),
    current_pdb(0),
    debug_level(0)
{
  current_pdb = _allocator->root();
  fill(std_files + 0, std_files + 5, (file *) 0);
  fill(files + 0, files + NFILES, (file *) 0);
}

