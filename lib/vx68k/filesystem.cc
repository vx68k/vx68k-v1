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
#include <cstring>

using namespace vx68k::human;
using namespace vx68k;
using namespace std;

namespace
{
} // (unnamed namespace)

sint32_type
regular_file::read(address_space *as, uint32_type dataptr, uint32_type size)
{
  // FIXME.
  unsigned char *data = new unsigned char [size];

  ssize_t result = ::read(fd, data, size);
  if (result == -1)
    {
      delete [] data;
      return -6;			// FIXME.
    }

  as->write(SUPER_DATA, dataptr, data, result);
  delete [] data;
  return result;
}

sint32_type
regular_file::write(const address_space *as, uint32_type dataptr,
		    uint32_type size)
{
  // FIXME.
  unsigned char *data = new unsigned char [size];
  as->read(SUPER_DATA, dataptr, data, size);

  ssize_t result = ::write(fd, data, size);
  if (result == -1)
    {
      delete [] data;
      return -6;			// FIXME.
    }

  delete [] data;
  return result;
}

regular_file::~regular_file()
{
  close(fd);
}

regular_file::regular_file(int f)
  : fd(f)
{
}

void
file_system::unref(file *f)
{
  // FIXME
  delete f;
}

sint_type
file_system::open(file *&ret, const char *name, sint_type mode)
{
  // FIXME.
  static const int uflag[] = {O_RDONLY, O_WRONLY, O_RDWR};


  if ((mode & 0xf) > 2)
    return -12;			// FIXME.

  int fd = ::open(name, uflag[mode & 0xf]);
  if (fd == -1)
    return -2;			// FIXME: errno test.

  ret = new regular_file(fd);
  return 0;
}

sint_type
file_system::create(file *&ret, const char *name, sint_type atr)
{
  return -1;
}

sint_type
file_system::chmod(const address_space *as, uint32_type nameptr, sint_type atr)
{
  // FIXME.
  char name[256];
  as->read(SUPER_DATA, nameptr, name, 256);
  char *c = strpbrk(name, " "); // ???
  if (c != NULL)
    *c = '\0';

  return -1;
}

