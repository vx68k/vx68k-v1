/* -*-C++-*- */
/* vx68k - Virtual X68000
   Copyright (C) 1998 Hypercore Software Design, Ltd.

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

#ifndef _VX68K_MEMORY_H
#define _VX68K_MEMORY_H 1

#include <vm68k/memory.h>

namespace vx68k
{
  using namespace vm68k;
  using namespace std;

  class main_memory
    : public memory
  {
  private:
    uint32_type end;
    uint16 *array;

  public:
    explicit main_memory(size_t);
    ~main_memory();

  public:
    size_t read(int, uint32_type, void *, size_t) const;
    uint_type getb(int, uint32_type) const;
    uint_type getw(int, uint32_type) const;
    uint32_type getl(int fc, uint32_type address) const;

  public:
    size_t write(int, uint32_type, const void *, size_t);
    void putb(int, uint32_type, uint_type);
    void putw(int, uint32_type, uint_type);
  };

  const size_t GRAPHICS_VRAM_SIZE = 2 * 1024 * 1024;
  const size_t TEXT_VRAM_PLANE_SIZE = 128 * 1024;
  const size_t TEXT_VRAM_SIZE = 4 * TEXT_VRAM_PLANE_SIZE;

  /* Graphics VRAM.  */
  class graphics_vram
    : public memory
  {
  private:
    uint16 *base;

  public:
    graphics_vram();
    ~graphics_vram();

  public:
    size_t read(int, uint32_type, void *, size_t) const;
    uint_type getb(int, uint32_type) const;
    uint_type getw(int, uint32_type) const;

  public:
    size_t write(int, uint32_type, const void *, size_t);
    void putb(int, uint32_type, uint_type);
    void putw(int, uint32_type, uint_type);
  };

  /* Text VRAM.  */
  class text_vram
    : public memory
  {
  private:
    uint16 *base;

  public:
    text_vram();
    ~text_vram();

  public:
    size_t read(int, uint32_type, void *, size_t) const;
    uint_type getb(int, uint32_type) const;
    uint_type getw(int, uint32_type) const;

  public:
    size_t write(int, uint32_type, const void *, size_t);
    void putb(int, uint32_type, uint_type);
    void putw(int, uint32_type, uint_type);
  };
} // vx68k

#endif /* not _VX68K_MEMORY_H */

