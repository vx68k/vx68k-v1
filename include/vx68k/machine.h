/* -*-C++-*- */
/* vx68k - Virtual X68000
   Copyright (C) 1998, 1999 Hypercore Software Design, Ltd.

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

#ifndef _VX68K_MACHINE_H
#define _VX68K_MACHINE_H 1

#include <vm68k/cpu.h>
#include <pthread.h>
#include <queue>

namespace vx68k
{
  using namespace vm68k;
  using namespace std;

  struct iocs_function_data
  {
  };

  class main_memory
    : public memory
  {
  private:
    uint32_type end;
    unsigned short *array;

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

  /* Interface to console.  */
  struct console
  {
    virtual void update_area(int x, int y, int width, int height) = 0;
    virtual void get_b16_image(unsigned int, unsigned char *, size_t) const = 0;
    virtual void get_k16_image(unsigned int, unsigned char *, size_t) const = 0;
  };

  /* Graphics VRAM.  */
  class graphics_vram
    : public memory
  {
  private:
    unsigned short *base;
    console *connected_console;

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

  public:
    void connect(console *);
  };

  /* Raster iterator for the text VRAM.  This class is used by the
     video system to scan pixels on the text VRAM, and must be
     efficient for a forward sequential access.

     FIXME: this should be a random access iterator.  */
  class text_vram_raster_iterator: public forward_iterator<uint_type, int>
  {
  private:
    unsigned short *buf;
    unsigned int pos;

  public:
    text_vram_raster_iterator()
      : buf(NULL), pos(0) {}

    text_vram_raster_iterator(unsigned short *b, unsigned int p)
      : buf(b), pos(p) {}

  public:
    bool operator==(const text_vram_raster_iterator &another) const
    {
      /* BUF is not tested for speed.  */
      return pos == another.pos;
    }

    bool operator!=(const text_vram_raster_iterator &another) const
    {
      return !(*this == another);
    }

  public:
    uint_type operator*() const;

  public:
    text_vram_raster_iterator &operator++();

    text_vram_raster_iterator operator++(int)
    {
      text_vram_raster_iterator tmp = *this;
      ++(*this);
      return tmp;
    }
  };

  /* Text VRAM.  */
  class text_vram
    : public memory
  {
  public:
    typedef text_vram_raster_iterator raster_iterator;

  private:
    unsigned short *buf;
    console *connected_console;

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

  /* Virtual machine of X68000.  */
  class machine
    : public instruction_data
  {
  public:
    typedef void (*iocs_function_handler)(context &, machine &,
					  iocs_function_data *);

  protected:
    static void invalid_iocs_function(context &, machine &,
				      iocs_function_data *);
    static void iocs(uint_type, context &, instruction_data *);

  private:
    size_t _memory_size;
    main_memory mem;
    text_vram tvram;
    class address_space as;
    class exec_unit eu;
    pair<iocs_function_handler, iocs_function_data *> iocs_functions[0x100];

    /* Cursor position.  */
    unsigned int curx, cury;

    /* Saved byte 1 of double-byte character.  */
    unsigned char saved_byte1;

    /* Key input queue.  */
    queue<uint_type> key_queue;

    /* Ready condition for key_queue.  */
    pthread_cond_t key_queue_not_empty;

    /* Mutex for key_queue.  */
    pthread_mutex_t key_queue_mutex;

  public:
    explicit machine(size_t);
    ~machine();

  public:
    size_t memory_size() const
      {return _memory_size;}
    class address_space *address_space()
      {return &as;}
    class exec_unit *exec_unit()
      {return &eu;}

  public:
    void connect(console *con)
      {tvram.connect(con);}
    void get_image(int x, int y, int width, int height,
		   unsigned char *rgb_buf, size_t row_size);

  public:
    void set_iocs_function(unsigned int, iocs_function_handler,
			   iocs_function_data *);

  public:
    void b_putc(uint_type);
    void b_print(uint32_type);

  public:			// Keyboard Input
    /* Queues a key input.  */
    void queue_key(uint_type key);

    /* Gets a key input from the queue.  */
    uint_type get_key();
  };
} // vx68k

#endif /* not _VX68K_MACHINE_H */
