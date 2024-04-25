/* Software floating-point emulation.
   (*c) = (long double)(a)
   Copyright (C) 1997-2024 Free Software Foundation, Inc.
   This file is part of the GNU C Library.

   The GNU C Library is free software; you can redistribute it and/or
   modify it under the terms of the GNU Lesser General Public
   License as published by the Free Software Foundation; either
   version 2.1 of the License, or (at your option) any later version.

   The GNU C Library is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
   Lesser General Public License for more details.

   You should have received a copy of the GNU Lesser General Public
   License along with the GNU C Library; if not, see
   <https://www.gnu.org/licenses/>.  */

#include "soft-fp.h"
#include "quad.h"

void _Qp_uxtoq(long double *c, const unsigned long a)
{
  FP_DECL_EX;
  FP_DECL_Q(C);
  unsigned long b = a;

  FP_FROM_INT_Q(C, b, 64, unsigned long);
  FP_PACK_RAW_QP(c, C);
  QP_NO_EXCEPTIONS;
}
