#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_math.h"

#define N 5

int
main( int argc, char *argv[] ) {

	/* Test solving the equation Ax = y, where A is the following
	 * 5 by 5 tridiagonal matrix:
	 *
	 *   3.5   4    0    0    0
	 *   1.7   3.2 -1.4  0    0
	 *   0    -5.1  1.5  2.2  0
	 *   0     0   -3.4  0.3  1.1
	 *   0     0    0    0.7  6.3
	 *
	 * y is the following column vector
	 *
	 *  13.15
	 *   8.33
	 * -13.25
	 *   3.37
	 *  14.77
	 *
	 * and the expected solution is the column vector x:
	 *
	 *   2.5
	 *   1.1
	 *  -0.4
	 *  -3.2
	 *   2.7
	 *
	 */

	int i;

	double a[N] = { 0.0,  1.7, -5.1, -3.4, 0.7 };
	double b[N] = { 3.5,  3.2,  1.5,  0.3, 6.3 };
	double c[N] = { 4.0, -1.4,  2.2,  1.1, 0.0 };

	double y[N] = { 13.15, 8.33, -13.25, 3.37, 14.77 };

	double x[N] = { 0, 0, 0, 0, 0 };

	mx_solve_tridiagonal_matrix( a, b, c, y, x, N );

	fprintf( stderr, "x = " );

	for ( i = 0; i < N; i++ ) {
		fprintf( stderr, "%g  ", x[i] );
	}

	fprintf( stderr, "\n" );

	exit(0);
}

