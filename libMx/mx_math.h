/*
 * Name:    mx_math.h
 *
 * Purpose: A collection of mathematical algorithms used by MX.
 *
 * Author:  William Lavender
 *
 * Note:    This is by no means a complete algorithm library.
 *          If you want a complete library, try the Atlas system
 *          which is based on BLAS and LAPACK.
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_MATH_H__
#define __MX_MATH_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

MX_API void mx_solve_tridiagonal_matrix( double *a,
					double *b,
					double *c,
					double *y,
					double *x,
					int n );

#ifdef __cplusplus
}
#endif

#endif /* __MX_MATH_H__ */

