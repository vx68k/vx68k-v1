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

#ifdef HAVE_UNISTD_H
# include <unistd.h>
#endif

#ifdef HAVE_NANA_H
# include <nana.h>
# include <cstdio>
#else
# include <cassert>
# define I assert
#endif

using namespace vx68k::iocs;
using namespace std;

sint32_type
image_file_floppy_disk::read(uint_type mode, uint32_type pos,
			     void *buf, size_t nbytes)
{
#ifdef HAVE_NANA_H
  L("image_file_floppy_disk: `read' not implemented.\n");
#endif
  return 0;
}

sint32_type
image_file_floppy_disk::write(uint_type mode, uint32_type pos,
			      const void *buf, size_t nbytes)
{
#ifdef HAVE_NANA_H
  L("image_file_floppy_disk: `write' not implemented.\n");
#endif
  return 0;
}

sint32_type
image_file_floppy_disk::verify(uint_type mode, uint32_type pos,
			       const void *buf, size_t nbytes)
{
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
}
