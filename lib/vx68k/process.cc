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
#include <stdexcept>

using namespace vx68k::human;
using namespace vx68k;
using namespace std;

sint32_type
process::read(sint_type fd,
	      address_space *mem, uint32_type buf, uint32_type size)
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
process::write(sint_type fd,
	       const address_space *mem, uint32_type buf, uint32_type size)
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

/* Closes a DOS file descriptor.  */
sint_type
process::close(sint_type fd)
{
  // FIXME.
  if (::close(fd) == -1)
    return -6;			// FIXME.

  return 0;
}

/* Opens a file.  */
sint_type
process::open(const char *name, sint_type flag)
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
process::create(const char *name, sint_type attr)
{
  // FIXME.
  sint_type fd = ::open(name, O_RDWR | O_CREAT | O_TRUNC, 0666);
  if (fd == -1)
    return -2;			// FIXME: errno test.

  return fd;
}

sint32_type
process::load(const char *name,
	      uint32_type first, uint32_type last) const
{
  return -1;
}

sint32_type
process::create_process(const char *name, process *&) const
{
  return -1;
}

sint32_type
process::exec(uint32_type start, uint32_type args, uint32_type env)
{
  return -1;
}

process::~process()
{
  if (block != 0)
    _allocator->free(block - 0x10);
}

process::process(memory_allocator *a, file_system *fs)
  : _allocator(a),
    _fs(fs),
    block(0)
{
  sint32_type t = _allocator->alloc_largest(0);
  if (t < 0)
    throw runtime_error("Out of memory");

  block = t - 0x10;
}

