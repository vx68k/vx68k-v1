/* -*-C++-*- */
/* vx68k - Virtual X68000
   Copyright (C) 1998, 1999 Hypercore Software Design, Ltd.

   This program is free software; you can redistribute it and/or
   modify it under the terms of the GNU General Public License as
   published by the Free Software Foundation; either version 2 of the
   License, or (at your option) any later version.

   This program is distributed in the hope that it will be useful, but
   WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
   02111-1307, USA.  */

#ifdef HAVE_NANA_H

#include <nana.h>

#else /* not HAVE_NANA_H */

#include <cstdio>
#include <cstdarg>
#include <cassert>

inline int debug_printf(const char *fmt,...)
{
  std::va_list va;
  std::va_start(va, fmt);
  int r = std::vfprintf(std::stderr, fmt, va);
  std::va_end(va, fmt);
  return r;
}

#define I(EXPR) (std::assert(EXPR))
#define VL(L) (debug_printf L)

#endif /* not HAVE_NANA_H */
