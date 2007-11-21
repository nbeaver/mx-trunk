/*
 * Name:    mx_math.c
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

#include "mx_util.h"
#include "mx_math.h"

/* The routine mx_solve_tridiagonal_matrix() solves matrix equations
 * of the form y = Tx where T is a known tridiagonal matrix, y is a
 * known column vector, and x is the unknown column vector that we
 * are trying to solve for.  In MX, this function is used to solve
 * cubic spline problems.  The routine uses Gaussian elimination
 * to compute the X vector and has been cross checked with the
 * equivalent Fortran algorithm from "Numerical Methods" by
 * Robert W. Hornbeck.
 */

MX_EXPORT void
mx_solve_tridiagonal_matrix( double *a,		/* Below the diagonal. */
				double *b,	/* The diagonal. */
				double *c,	/* Above the diagonal. */
				double *y,	/* Column vector. */
				double *x,	/* Solution column vector. */
				int n )		/* Dimension. */
{
	double multiplier;
	int i;

	c[0] = mx_divide_safely( c[0], b[0] );
	y[0] = mx_divide_safely( y[0], b[0] );

	for ( i = 0; i < n; i++ ) {
		multiplier = mx_divide_safely( 1.0, b[i] - c[i-1] * a[i] );
		c[i] = c[i] * multiplier;
		y[i] = ( y[i] - a[i] * y[i-1] ) * multiplier;
	}

	x[n-1] = y[n-1];

	for ( i = n-2; i > -1; i-- ) {
		x[i] = y[i] - c[i] * x[i+1];
	}

	return;
}

