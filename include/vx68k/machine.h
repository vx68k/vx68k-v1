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

#ifndef _VX68K_MACHINE_H
#define _VX68K_MACHINE_H 1

#include <vm68k/cpu.h>

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

  /* Interface to console.  */
  struct console
  {
    virtual void get_b16_image(unsigned int, unsigned char *, size_t) const = 0;
    virtual void get_k16_image(unsigned int, unsigned char *, size_t) const = 0;
  };

  /* Graphics VRAM.  */
  class graphics_vram
    : public memory
  {
  private:
    uint16 *base;
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

  /* Text VRAM.  */
  class text_vram
    : public memory
  {
  private:
    uint16 *buf;
    console *connected_console;
    unsigned int curx, cury;

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
    void draw_char(unsigned int);
    void scroll();

  public:
    void connect(console *);
  };

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

  public:
    explicit machine(size_t);

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
  };
} // vx68k

#endif /* not _VX68K_MACHINE_H */

