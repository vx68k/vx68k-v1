/* -*-C++-*- */
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

#ifndef _VX68K_IOCS_H
#define _VX68K_IOCS_H 1

#include <vm68k/types.h>
#include <cstddef>

namespace vx68k
{
  namespace iocs
  {
    using namespace vm68k::types;
    using std::size_t;

    /* Abstract disk in the view of the IOCS.  This class is not that
       of disk units but of disk media.  */
    class disk
    {
    public:
      virtual ~disk() {}

    public:
      /* Reads records.  */
      virtual sint32_type read(uint_type, uint32_type, void *, size_t) = 0;

      /* Writes records.  */
      virtual sint32_type write(uint_type, uint32_type, const void *, size_t)
	= 0;

      /* Verifies records by comparing the contents.  */
      virtual sint32_type verify(uint_type, uint32_type, const void *, size_t)
	= 0;
    };

    /* Floppy disk that is simulated by an image file.  */
    class image_file_floppy_disk: public disk
    {
    private:
      int image_fildes;

    public:
      image_file_floppy_disk(int fildes);
      ~image_file_floppy_disk();

    public:
      sint32_type read(uint_type, uint32_type, void *, size_t);
      sint32_type write(uint_type, uint32_type, const void *, size_t);
      sint32_type verify(uint_type, uint32_type, const void *, size_t);
    };
  } // namespace iocs
} // namespace vx68k

#endif /* not _VX68K_IOCS_H */
