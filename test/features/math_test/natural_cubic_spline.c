#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_math.h"

#define N  7

int
main( int argc, char *argv[] )
{
	FILE *input_file;
	MX_CUBIC_SPLINE *spline;
	unsigned long i, num_points;
	int num_items;
	double *x_array, *y_array;
	char buffer[80];
	char *filename;
	double start, end, stepsize;
	double x, y;
	mx_status_type mx_status;

	if ( argc != 5 ) {
		fprintf( stderr,
		"Usage: natural_cubic_spline filename start end stepsize\n" );

		exit(1);
	}

	filename = argv[1];
	start = atof( argv[2] );
	end = atof( argv[3] );
	stepsize = atof( argv[4] );

	/* Open a file containing a set of X-Y values. */

	input_file = fopen( filename, "r" );

	if ( input_file == NULL ) {
		fprintf(stderr, "Cannot open '%s'.\n", filename);
		exit(1);
	}

	/* Figure out how many lines there are in the file. */

	num_points = 0;

	fgets( buffer, sizeof(buffer), input_file );

	while ( feof(input_file) == 0 ) {
		num_points++;

		fgets( buffer, sizeof(buffer), input_file );
	}

	/* Allocate arrays for the X and Y values. */

	x_array = malloc( num_points * sizeof(double) );

	if ( x_array == NULL ) {
		fprintf( stderr,
		"malloc() failed for an X array of %lu points.\n",
			num_points );
		exit(1);
	}

	y_array = malloc( num_points * sizeof(double) );

	if ( y_array == NULL ) {
		fprintf( stderr,
		"malloc() failed for a Y array of %lu points.\n",
			num_points );
		exit(1);
	}

	/* Go back to the start of the file and read in the X-Y values. */

	rewind( input_file );

	for ( i = 0; i < num_points; i++ ) {
		fgets( buffer, sizeof(buffer), input_file );

		num_items = sscanf( buffer, "%lg %lg",
					&x_array[i], &y_array[i] );

		if ( num_items != 2 ) {
			fprintf( stderr,
		"Did not see an X-Y data pair for line %lu containing '%s'.\n",
				i, buffer );
			exit(1);
		}
	}

	fclose( input_file );

	/* Fit a natural cubic spline through the set of points. */

	mx_status = mx_create_natural_cubic_spline( num_points,
						x_array, y_array, &spline );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	/* Loop from start to end and print cubic spline interpolated values. */

	for ( x = start; x <= end; x += stepsize ) {
		y = mx_get_cubic_spline_value( spline, x );

		printf( "%-8g %-8g\n", x, y );
	}

	exit(0);
}

