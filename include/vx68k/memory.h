/* vx68k - Virtual X68000 (-*- C++ -*-)
   Copyright (C) 1998-2000 Hypercore Software Design, Ltd.

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

/* Memory components for Virtual X68000.  */

#ifndef _VX68K_MEMORY_H
#define _VX68K_MEMORY_H 1

#include <vm68k/memory.h>

namespace vx68k
{
  using vm68k::memory;
  using namespace vm68k::types;

  class main_memory;

  /* Area set interface.  This object is mapped from 0xe86000 to
     0xe88000.  This object has only the area set register, which is
     at 0xe86001(b).  */
  class area_set: public memory
  {
  private:
    /* Reference to the main memory.  */
    main_memory *_mm;

  public:
    explicit area_set(main_memory *);

  public:
    /* Reads data from this object.  */
    uint_type getw(int, uint32_type) const;
    uint_type getb(int, uint32_type) const;
    size_t read(int, uint32_type, void *, size_t) const;

    /* Writes data to this object.  */
    void putw(int, uint32_type, uint_type);
    void putb(int, uint32_type, uint_type);
    size_t write(int, uint32_type, const void *, size_t);
  };
} // namespace vx68k

#endif /* not _VX68K_MEMORY_H */
