/* -*- C++ -*- */
/* Virtual X68000 - X68000 virtual machine
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

#include <vm68k/cpu.h>

namespace vm68k
{
  struct instruction_map
  {
    uint_type base, mask;
    exec_unit::instruction_handler handler;
  };

  void install_instructions_0(exec_unit &, unsigned long data);
  void install_instructions_1(exec_unit &, unsigned long data);
  void install_instructions_2(exec_unit &, unsigned long data);
  void install_instructions_3(exec_unit &, unsigned long data);
  void install_instructions_4(exec_unit &, unsigned long data);
  void install_instructions_5(exec_unit &, unsigned long data);
  void install_instructions_6(exec_unit &, unsigned long data);
  void install_instructions_7(exec_unit &, unsigned long data);
  void install_instructions_8(exec_unit &, unsigned long data);
  void install_instructions_9(exec_unit &, unsigned long data);
  void install_instructions_11(exec_unit &, unsigned long data);
  void install_instructions_12(exec_unit &, unsigned long data);
  void install_instructions_13(exec_unit &, unsigned long data);
  void install_instructions_14(exec_unit &, unsigned long data);
}
