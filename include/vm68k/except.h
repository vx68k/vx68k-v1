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

#ifndef _VM68K_EXCEPT_H
#define _VM68K_EXCEPT_H 1

#include <vm68k/types.h>

#include <stdexcept>

namespace vm68k
{
  using namespace std;

  /* Ordinary exception other than bus error and address error.  */
  struct ordinary_exception: runtime_error
  {
  };

  /* Illegal instruction exception.  */
  struct illegal_instruction_exception: ordinary_exception
  {
  };

  /* Zero divide exception.  */
  struct zero_divide_exception: ordinary_exception
  {
  };

  /* Privilege violation exception.  */
  struct privilege_violation_exception: ordinary_exception
  {
  };
} // vm68k

#endif /* not _VM68K_EXCEPT_H */
