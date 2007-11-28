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

typedef struct {
	unsigned long num_points;
	double *x_array;
	double *y_array;
	double *gpp_array;	/* Array of second derivatives. */
	double gp_start;	/* First derivative at the first point. */
	double gp_end;		/* First derivative at the last point. */
} MX_CUBIC_SPLINE;

MX_API mx_status_type
mx_create_natural_cubic_spline( unsigned long num_points,
				double *x_array,
				double *y_array,
				MX_CUBIC_SPLINE ** spline );

MX_API void
mx_delete_cubic_spline( MX_CUBIC_SPLINE *spline );

MX_API double
mx_get_cubic_spline_value( MX_CUBIC_SPLINE *spline, double x );

#ifdef __cplusplus
}
#endif

#endif /* __MX_MATH_H__ */

