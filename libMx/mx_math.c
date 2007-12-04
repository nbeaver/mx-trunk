/*
 * Name:    mx_math.c
 *
 * Purpose: A collection of mathematical algorithms used by MX.
 *
 * Author:  William Lavender
 *
 * Note:    This is by no means a complete algorithm library.
 *          If you want a complete library, try BLAS or LAPACK.
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_math.h"

/* The routine mx_solve_tridiagonal_matrix() solves matrix equations
 * of the form y = Tx where T is a known tridiagonal matrix, y is a
 * known column vector, and x is the unknown column vector that we
 * are trying to solve for.  In MX, this function is used to solve
 * cubic spline problems.  The routine uses Gaussian elimination
 * to compute the X vector and has been cross checked with the
 * equivalent Fortran algorithm from "Numerical Methods" by
 * Robert W. Hornbeck, Quantum, 1975.
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

	for ( i = 1; i < n; i++ ) {
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

/*--------------------------------------------------------------------------*/

/* The following cubic spline code comes from pages 47 to 49 of
 * "Numerical Methods" by Robert W. Hornbeck, Quantum, 1975.
 */

#define FREE_INTERNAL_VECTORS \
		do {			\
			mx_free(A);	\
			mx_free(B);	\
			mx_free(C);	\
			mx_free(R);	\
		} while (0)

MX_EXPORT mx_status_type
mx_create_natural_cubic_spline( unsigned long num_points,
				double *x_array,
				double *y_array,
				MX_CUBIC_SPLINE **spline )
{
	static const char fname[] = "mx_create_natural_cubic_spline()";

	double *X, *Y;
	double *A, *B, *C;
	double *G, *R;
	unsigned long i, j;
	double delta_x, delta_y, gpp;

	X = Y = A = B = C = G = R = NULL;

	if ( num_points == 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Splines containing zero points are not supported.  "
		"You must provide at least two points." );
	} else 
	if ( num_points == 1 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Splines containing only one point are not supported.  "
		"You must provide at least two points." );
	}

	if ( x_array == (double *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The x_array argument passed was NULL." );
	}
	if ( y_array == (double *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The y_array argument passed was NULL." );
	}
	if ( spline == (MX_CUBIC_SPLINE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_CUBIC_SPLINE argument passed was NULL." );
	}

	*spline = calloc( 1, sizeof(MX_CUBIC_SPLINE) );

	if ( (*spline) == (MX_CUBIC_SPLINE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	  "Ran out of memory trying to allocate an MX_CUBIC_SPLINE structure.");
	}

	(*spline)->num_points = num_points;

	X = malloc( num_points * sizeof(double) );

	if ( X == (double *) NULL ) {
		mx_delete_cubic_spline( *spline );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate the x array." );
	}

	memcpy( X, x_array, num_points * sizeof(double) );

	(*spline)->x_array = X;

	Y = malloc( num_points * sizeof(double) );

	if ( Y == (double *) NULL ) {
		mx_delete_cubic_spline( *spline );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate the y array." );
	}

	memcpy( Y, y_array, num_points * sizeof(double) );

	(*spline)->y_array = Y;

	/* As a special case, we use a straight line if only two points
	 * are provided.  If so, then the rest of this function must be
	 * skipped since it is not able to handle a spline with only
	 * two points.
	 */

	if ( num_points == 2 ) {
		return MX_SUCCESSFUL_RESULT;
	}

	(*spline)->gpp_array = calloc( num_points, sizeof(double) );

	if ( (*spline)->gpp_array == (double *) NULL ) {
		mx_delete_cubic_spline( *spline );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate the gpp array." );
	}

	/* Initialize the values at the start and the end of
	 * the second derivative array.
	 */

	(*spline)->gpp_array[0] = 0.0;

	(*spline)->gpp_array[num_points - 1] = 0.0;

	/* Allocated memory for the contents of a tridiagonal matrix equation
	 * TG = R that will be solved to find all the other second derivative
	 * values.  The three diagonals in the matrix T are referred to as
	 * A, B, and C where A is below the diagonal, B _is_ the diagonal,
	 * and C is above the diagonal.
	 */

	A = malloc( (num_points - 2) * sizeof(double) );

	if ( A == (double *) NULL ) {
		mx_delete_cubic_spline( *spline );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate the A array." );
	}

	B = malloc( (num_points - 2) * sizeof(double) );

	if ( B == (double *) NULL ) {
		FREE_INTERNAL_VECTORS;
		mx_delete_cubic_spline( *spline );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate the B array." );
	}

	C = malloc( (num_points - 2) * sizeof(double) );

	if ( C == (double *) NULL ) {
		FREE_INTERNAL_VECTORS;
		mx_delete_cubic_spline( *spline );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate the C array." );
	}

	/* Allocate the column vector for the right hand side R. */

	R = malloc( (num_points - 2) * sizeof(double) );

	if ( R == (double *) NULL ) {
		FREE_INTERNAL_VECTORS;
		mx_delete_cubic_spline( *spline );

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate the R array." );
	}

	/* Get a pointer to the second element of the second derivative
	 * array which will be used as the column vector G in the matrix
	 * equation.
	 */

	G = &((*spline)->gpp_array[1]);

	/* Fill in the tridiagonal matrix. */

	A[0] = 0.0;

	for ( i = 1; i < num_points - 2; i++ ) {
		j = i + 1;

		A[i] = mx_divide_safely( X[j] - X[j-1],  X[j+1] - X[j] );
	}

	for ( i = 0; i < num_points - 2; i++ ) {
		j = i + 1;

		B[i] = mx_divide_safely( 2.0 * (X[j+1] - X[j-1]),
							X[j+1] - X[j] );
	}

	for ( i = 0; i < num_points - 3; i++ ) {
		C[i] = 1.0;
	}

	C[num_points - 3] = 0.0;

	/* Fill in the right hand side vector. */

	for ( i = 0; i < num_points - 2; i++ ) {
		j = i + 1;

		R[i] = 6.0 * ( 
			mx_divide_safely( Y[j+1] - Y[j],
				( X[j+1] - X[j] ) * ( X[j+1] - X[j] ) )

			- mx_divide_safely( Y[j] - Y[j-1],
				( X[j+1] - X[j] ) * ( X[j] - X[j-1] ) ) );
	}

#if 0
	/* Print out the arrays. */

	fprintf( stderr, "A = " );

	for ( i = 0; i < num_points - 2; i++ ) {
		fprintf( stderr, "%g  ", A[i] );
	}

	fprintf( stderr, "\nB = " );

	for ( i = 0; i < num_points - 2; i++ ) {
		fprintf( stderr, "%g  ", B[i] );
	}

	fprintf( stderr, "\nC = " );

	for ( i = 0; i < num_points - 2; i++ ) {
		fprintf( stderr, "%g  ", C[i] );
	}

	fprintf( stderr, "\nR = " );

	for ( i = 0; i < num_points - 2; i++ ) {
		fprintf( stderr, "%g  ", R[i] );
	}

	fprintf( stderr, "\n" );
#endif

	/* Solve the tridiagonal matrix. */

	mx_solve_tridiagonal_matrix( A, B, C, R, G, num_points - 2 );

#if 0
	/* Print out the solution. */

	fprintf( stderr, "gpp_array = " );

	for ( i = 0; i < num_points; i++ ) {
		fprintf( stderr, "%g  ", (*spline)->gpp_array[i] );
	}

	fprintf( stderr, "\n" );
#endif

	/* Compute the slope at the start of the cubic spline. */

	delta_y = Y[1] - Y[0];
	delta_x = X[1] - X[0];

	gpp = (*spline)->gpp_array[1];

	(*spline)->gp_start = mx_divide_safely( delta_y, delta_x )
			- ( delta_x / 6.0 ) * gpp;
		
	/* Compute the slope at the end of the cubic spline. */

	delta_y = Y[num_points - 1] - Y[num_points - 2];
	delta_x = X[num_points - 1] - X[num_points - 2];

	gpp = (*spline)->gpp_array[num_points-2];

	(*spline)->gp_end = mx_divide_safely( delta_y, delta_x )
			+ ( delta_x / 6.0 ) * gpp;
		
#if 0
	fprintf( stderr, "gp_start = %g,  gp_end = %g\n",
		(*spline)->gp_start, (*spline)->gp_end );
#endif

	FREE_INTERNAL_VECTORS;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT void
mx_delete_cubic_spline( MX_CUBIC_SPLINE *spline )
{
	if ( spline == (MX_CUBIC_SPLINE *) NULL )
		return;

	if ( spline->x_array != (double *) NULL ) {
		free( spline->x_array );
	}

	if ( spline->y_array != (double *) NULL ) {
		free( spline->y_array );
	}

	if ( spline->gpp_array != (double *) NULL ) {
		free( spline->gpp_array );
	}

	free( spline );

	return;
}

MX_EXPORT double
mx_get_cubic_spline_value( MX_CUBIC_SPLINE *spline, double x )
{
	static const char fname[] = "mx_get_cubic_spline_value()";

	double *x_array, *y_array;
	double slope, x0, x1, y0, y1, gpp0, gpp1, delta_x;
	double value;
	unsigned long i, num_points;

	if ( spline == (MX_CUBIC_SPLINE *) NULL )
		return 0.0;

	num_points = spline->num_points;

	x_array = spline->x_array;
	y_array = spline->y_array;
	
	/* As a special case, we use a straight line if only two points
	 * are provided.  If so, then the rest of this function must be
	 * skipped since it is not able to handle a spline with only
	 * two points.
	 */

	if ( num_points == 2 ) {
		slope = mx_divide_safely( y_array[1] - y_array[0],
					x_array[1] - x_array[0] );

		value = y_array[0] + slope * ( x - x_array[0] );

		return value;
	}

	/* Are we outside the range of the X array?  If so, then extrapolate
	 * using a constant slope.
	 */

	if ( x <= x_array[0] ) {
		slope = spline->gp_start;

		value = y_array[0] + slope * ( x - x_array[0] );

		return value;
	}

	if ( x >= x_array[num_points - 1] ) {
		slope = spline->gp_end;

		value = y_array[num_points - 1]
				+ slope * ( x - x_array[num_points - 1] );

		return value;
	}

	/* Search for x in the x array. */

	for ( i = 0; i < num_points - 2; i++ ) {
		if ( ( x >= x_array[i] ) && ( x < x_array[i+1] ) )
		{
			break;		/* Exit the while loop. */
		}
	}

	if ( i >= num_points ) {
		(void) mx_error( MXE_UNKNOWN_ERROR, fname,
			"Somehow we got to i = %lu for num_points = %lu.  "
			"This should not be able to happen!", i, num_points );

		return 0.0;
	}

	x0 = x_array[i];
	x1 = x_array[i+1];

	y0 = y_array[i];
	y1 = y_array[i+1];

	gpp0 = spline->gpp_array[i];
	gpp1 = spline->gpp_array[i+1];

	delta_x = x1 - x0;

#if 0
	fprintf(stderr, "x = %g, x0 = %g, x1 = %g\n", x, x0, x1);
	fprintf(stderr, "        f0 = %g, f1 = %g\n", y0, y1);
	fprintf(stderr, "        gpp0 = %g, gpp1 = %g\n", gpp0, gpp1);
	fprintf(stderr, "delta_x = %g\n", delta_x);
#endif

	/* Compute the value using equation 4.26 in Hornbeck. */

	value = ( gpp0 / 6.0 ) * ( ( (x1-x)*(x1-x)*(x1-x) / delta_x )
					- delta_x * (x1-x) )

			+ ( gpp1 / 6.0 )
				* ( ( (x-x0)*(x-x0)*(x-x0) / delta_x )
					- delta_x * (x-x0) )

			+ ( y0 * (x1-x) / delta_x )

			+ ( y1 * (x-x0) / delta_x );

#if 0
	fprintf(stderr, "value = %g\n", value);
#endif

	return value;
}

