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

#ifndef _VM68K_TYPES_H
#define _VM68K_TYPES_H 1

#include <climits>

namespace vm68k
{
  using namespace std;

  namespace types
  {
    typedef unsigned int uint_type;

#if UINT_MAX >= 0xffffffff
    typedef unsigned int uint32_type;
#else
    typedef unsigned long uint32_type;
#endif

#if INT_MIN >= -0x7fff
    typedef long sint_type;
#else
    typedef int sint_type;
#endif

#if LONG_MIN >= -0x7fffffff
# ifdef __GNUC__
    typedef long long sint32_type;
# else
#  error No type that can hold m68k signed 32-bit number.
# endif
#elsif INT_MIN >= -0x7fffffff
    typedef long sint32_type;
#else
    typedef int sint32_type;
#endif
  } // types

  using namespace types;
} // vm68k

#endif /* not _VM68K_TYPES_H */

