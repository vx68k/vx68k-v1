/* -*-C++-*- */

#ifndef VM68K_TYPES_H
#define VM68k_TYPES_H

#include <climits>

#if INT_MAX >= 0x7fffffff
typedef int int32;
typedef unsigned int uint32;
#else
typedef long int32;
typedef unsigned long uint32;
#endif

typedef unsigned short uint16;
typedef unsigned char uint8;

#endif

