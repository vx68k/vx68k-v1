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

  /* Interface to console.  */
  struct console
  {
    virtual void update_area(int x, int y, int width, int height) = 0;
    virtual void get_b16_image(unsigned int, unsigned char *, size_t) const = 0;
    virtual void get_k16_image(unsigned int, unsigned char *, size_t) const = 0;
  };

  /* Raster iterator for the text VRAM.  This class is used by the
     video system to scan pixels on the text VRAM, and must be
     efficient for a forward sequential access.

     FIXME: this should be a random access iterator.  */
  class text_video_raster_iterator: public forward_iterator<uint_type, int>
  {
  private:
    unsigned short *buf;
    unsigned int pos;

  public:
    text_video_raster_iterator()
      : buf(NULL), pos(0) {}

    text_video_raster_iterator(unsigned short *b, unsigned int p)
      : buf(b), pos(p) {}

  public:
    bool operator==(const text_video_raster_iterator &another) const
    {
      /* BUF is not tested for speed.  */
      return pos == another.pos;
    }

    bool operator!=(const text_video_raster_iterator &another) const
    {
      return !(*this == another);
    }

  public:
    uint_type operator*() const;

  public:
    text_video_raster_iterator &operator++();

    text_video_raster_iterator operator++(int)
    {
      text_video_raster_iterator tmp = *this;
      ++(*this);
      return tmp;
    }
  };

  /* Text VRAM.  */
  class text_video_memory: public memory
  {
  public:
    typedef text_video_raster_iterator raster_iterator;

  private:
    unsigned short *buf;
    console *connected_console;

  public:
    text_video_memory();
    ~text_video_memory();

  public:
    uint_type get_16(int, uint32_type) const;
    uint_type get_8(int, uint32_type) const;

    void put_16(int, uint32_type, uint_type);
    void put_8(int, uint32_type, uint_type);

  public:
    /* Draw a character CODE at [X Y].  */
    void draw_char(int x, int y, unsigned int code);

    /* Scroll one line up.  */
    void scroll();

  public:
    void connect(console *);

    /* Get the visual image of this text VRAM.  Image is of size
       [WIDTH HEIGHT] at position [X Y].  RGB_BUF is an array of
       bytes.  ROW_SIZE is the row size of RGB_BUF.  */
    void get_image(int x, int y, int width, int height,
		   unsigned char *rgb_buf, size_t row_size);
  };

  /* CRTC registers memory.  */
  class crtc_memory: public memory
  {
  public:
    /* Reads data from this object.  */
    uint_type get_16(int, uint32_type) const;
    uint_type get_8(int, uint32_type) const;

    /* Writes data to this object.  */
    void put_16(int, uint32_type, uint_type);
    void put_8(int, uint32_type, uint_type);
  };

  /* Area set register.  This object is mapped from 0xe86000 to
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
    uint_type get_16(int, uint32_type) const;
    uint_type get_8(int, uint32_type) const;

    /* Writes data to this object.  */
    void put_16(int, uint32_type, uint_type);
    void put_8(int, uint32_type, uint_type);
  };

  /* SCC registers memory.  */
  class scc_memory: public memory
  {
  public:
    /* Reads data from this object.  */
    uint_type get_16(int, uint32_type) const;
    uint_type get_8(int, uint32_type) const;

    /* Writes data to this object.  */
    void put_16(int, uint32_type, uint_type);
    void put_8(int, uint32_type, uint_type);
  };

  /* PPI (a.k.a 8255A) registers memory.  On X68000, a PPI is used for
     joypad interface and ADPCM contorl.  */
  class ppi_memory: public memory
  {
  public:
    /* Reads data from this object.  */
    uint_type get_16(int, uint32_type) const;
    uint_type get_8(int, uint32_type) const;

    /* Writes data to this object.  */
    void put_16(int, uint32_type, uint_type);
    void put_8(int, uint32_type, uint_type);
  };
} // namespace vx68k

#endif /* not _VX68K_MEMORY_H */
