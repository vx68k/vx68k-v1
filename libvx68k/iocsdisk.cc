/* Virtual X68000 - Sharp X68000 emulator
   Copyright (C) 1998, 2000 Hypercore Software Design, Ltd.

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

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif
#undef const
#undef inline

#include <vx68k/iocs.h>
#include <vm68k/cpu.h>

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#include <stdexcept>

#ifdef HAVE_NANA_H
# include <nana.h>
# include <cstdio>
#else
# include <cassert>
# define I assert
#endif

using namespace vx68k::iocs;
using namespace vm68k;
using namespace std;

namespace
{
  /* Disk error.  */
  class disk_error: public runtime_error
  {
  private:
    sint32_type d0;

  public:
    disk_error(sint32_type id0)
      : d0(id0) {}

    sint32_type value() const
    {return d0;}
  };
} // namespace (unnamed)

off_t
image_file_floppy_disk::record_offset(uint32_type pos)
{
  unsigned int n = pos >> 24 & 0xff;
  unsigned int c = pos >> 16 & 0xff;
  unsigned int h = pos >> 8 & 0xff;
  unsigned int r = pos & 0xff;

  if (n == 3)
    {
      if (c > 76 || h > 1
	  || r < 1 || r > 8)
	throw disk_error(long_word_size::svalue(0x40040000));
    }
  else
    throw disk_error(0x40040000);

  off_t off = ((off_t(c) * 2 + h) * 8 + (r - 1)) * 1024;
  return off;
}

sint32_type
image_file_floppy_disk::read(uint_type mode, uint32_type pos,
			     memory_map &a,
			     uint32_type buf, uint32_type nbytes)
{
  I(image_fildes >= 0);

  try
    {
      off_t res = lseek(image_fildes, record_offset(pos), SEEK_SET);
      I(res != -1);
    }
  catch (const disk_error &e)
    {
      return e.value() | (mode >> 8 & 0x3) << 24;
    }

  nbytes = (nbytes + 1023u) / 1024u * 1024u;
  while (nbytes >= 1024)
    {
      unsigned char data[1024];

      int res = ::read(image_fildes, data, 1024);
      if (res == -1)
	return long_word_size::svalue(0x40200000);
      if (res != 1024)
	return long_word_size::svalue(0x40202000);

      a.write(buf, data, 1024, memory::SUPER_DATA);

      buf += 1024;
      nbytes -= 1024;
    }

  return 0;
}

sint32_type
image_file_floppy_disk::write(uint_type mode, uint32_type pos,
			      const memory_map &a,
			      uint32_type buf, uint32_type nbytes)
{
  I(image_fildes >= 0);

  try
    {
      off_t res = lseek(image_fildes, record_offset(pos), SEEK_SET);
      I(res != -1);
    }
  catch (const disk_error &e)
    {
      return e.value() | (mode >> 8 & 0x3) << 24;
    }
#ifdef HAVE_NANA_H
  L("image_file_floppy_disk: `write' not implemented.\n");
#endif
  return 0;
}

sint32_type
image_file_floppy_disk::verify(uint_type mode, uint32_type pos,
			       const memory_map &a,
			       uint32_type buf, uint32_type nbytes)
{
  I(image_fildes >= 0);

  try
    {
      off_t res = lseek(image_fildes, record_offset(pos), SEEK_SET);
      I(res != -1);
    }
  catch (const disk_error &e)
    {
      return e.value() | (mode >> 8 & 0x3) << 24;
    }
#ifdef HAVE_NANA_H
  L("image_file_floppy_disk: `verify' not implemented.\n");
#endif
  return 0;
}

image_file_floppy_disk::~image_file_floppy_disk()
{
  I(image_fildes >= 0);

  if (close(image_fildes) == -1)
    {
    }
}

image_file_floppy_disk::image_file_floppy_disk(int fildes)
  : image_fildes(fildes)
{
  I(image_fildes >= 0);
  if (image_fildes < 0)
    throw invalid_argument("image_file_floppy_disk");
}
