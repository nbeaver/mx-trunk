#include <stdio.h>
#include <stdlib.h>
#include <float.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_stdint.h"
#include "mx_array.h"

#define VECTOR_LENGTH	6

int
main( int argc, char *argv[] )
{
	static const char fname[] = "scaled_vector";

	long test_vector[VECTOR_LENGTH] = {1, 2, 3, 5, 8, 13};
	size_t test_vector_length =
		sizeof(test_vector) / sizeof(test_vector[0]);
	long test_datatype = MXFT_LONG;

	double scale = 6.7;
	double offset = 30;

	size_t i;

	char char_vector[VECTOR_LENGTH];
	unsigned char uchar_vector[VECTOR_LENGTH];
	short short_vector[VECTOR_LENGTH];
	unsigned short ushort_vector[VECTOR_LENGTH];
	mx_bool_type bool_vector[VECTOR_LENGTH];
	long long_vector[VECTOR_LENGTH];
	unsigned long ulong_vector[VECTOR_LENGTH];
	float float_vector[VECTOR_LENGTH];
	double double_vector[VECTOR_LENGTH];

	mx_status_type mx_status;

	fprintf( stderr, "%s: Original vector (as long) = { ", fname );

	for ( i = 0; i < test_vector_length; i++ ) {
		fprintf( stderr, "%ld ", test_vector[i] );
	}

	fprintf( stderr, "}\n\n" );

	fprintf( stderr, "Rescaled values = { " );

	for ( i = 0; i < test_vector_length; i++ ) {
		fprintf( stderr, "%g ", scale * test_vector[i] + offset );
	}

	fprintf( stderr, "}\n\n" );

	/********* Long to char *********/

	mx_status = mx_convert_and_copy_scaled_vector(
			test_vector, test_datatype, test_vector_length,
			char_vector, MXFT_CHAR, test_vector_length,
			scale, offset, -DBL_MAX, DBL_MAX );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	fprintf( stderr, "MXFT_CHAR = { " );

	for (i = 0; i < test_vector_length; i++ ) {
		fprintf( stderr, "'%c' %ld, ",
			char_vector[i], (long) char_vector[i] );
	}

	fprintf( stderr, "}\n\n" );

	/********* Long to uchar *********/

	mx_status = mx_convert_and_copy_scaled_vector(
			test_vector, test_datatype, test_vector_length,
			uchar_vector, MXFT_UCHAR, test_vector_length,
			scale, offset, -DBL_MAX, DBL_MAX );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	fprintf( stderr, "MXFT_UCHAR = { " );

	for (i = 0; i < test_vector_length; i++ ) {
		fprintf( stderr, "'%c' %ld, ",
			char_vector[i], (long) char_vector[i] );
	}

	fprintf( stderr, "}\n\n" );

	/********* Long to short *********/

	mx_status = mx_convert_and_copy_scaled_vector(
			test_vector, test_datatype, test_vector_length,
			short_vector, MXFT_SHORT, test_vector_length,
			scale, offset, -DBL_MAX, DBL_MAX );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	fprintf( stderr, "MXFT_SHORT = { " );

	for (i = 0; i < test_vector_length; i++ ) {
		fprintf( stderr, "%hd, ", short_vector[i] );
	}

	fprintf( stderr, "}\n\n" );

	/********* Long to ushort *********/

	mx_status = mx_convert_and_copy_scaled_vector(
			test_vector, test_datatype, test_vector_length,
			ushort_vector, MXFT_USHORT, test_vector_length,
			scale, offset, -DBL_MAX, DBL_MAX );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	fprintf( stderr, "MXFT_USHORT = { " );

	for (i = 0; i < test_vector_length; i++ ) {
		fprintf( stderr, "%hu, ",
			ushort_vector[i] );
	}

	fprintf( stderr, "}\n\n" );

	/********* Long to bool *********/

	mx_status = mx_convert_and_copy_scaled_vector(
			test_vector, test_datatype, test_vector_length,
			bool_vector, MXFT_BOOL, test_vector_length,
			scale, offset, -DBL_MAX, DBL_MAX );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	fprintf( stderr, "MXFT_BOOL = { " );

	for (i = 0; i < test_vector_length; i++ ) {
		fprintf( stderr, "%ld, ", (long) bool_vector[i] );
	}

	fprintf( stderr, "}\n\n" );

	/********* Long to long *********/

	mx_status = mx_convert_and_copy_scaled_vector(
			test_vector, test_datatype, test_vector_length,
			long_vector, MXFT_LONG, test_vector_length,
			scale, offset, -DBL_MAX, DBL_MAX );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	fprintf( stderr, "MXFT_LONG = { " );

	for (i = 0; i < test_vector_length; i++ ) {
		fprintf( stderr, "%ld, ", long_vector[i] );
	}

	fprintf( stderr, "}\n\n" );

	/********* Long to ulong *********/

	mx_status = mx_convert_and_copy_scaled_vector(
			test_vector, test_datatype, test_vector_length,
			ulong_vector, MXFT_ULONG, test_vector_length,
			scale, offset, -DBL_MAX, DBL_MAX );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	fprintf( stderr, "MXFT_ULONG = { " );

	for (i = 0; i < test_vector_length; i++ ) {
		fprintf( stderr, "%lu, ",
			ulong_vector[i] );
	}

	fprintf( stderr, "}\n\n" );

	/********* Long to float *********/

	mx_status = mx_convert_and_copy_scaled_vector(
			test_vector, test_datatype, test_vector_length,
			float_vector, MXFT_FLOAT, test_vector_length,
			scale, offset, -DBL_MAX, DBL_MAX );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	fprintf( stderr, "MXFT_FLOAT = { " );

	for (i = 0; i < test_vector_length; i++ ) {
		fprintf( stderr, "%g, ", float_vector[i] );
	}

	fprintf( stderr, "}\n\n" );

	/********* Long to double *********/

	mx_status = mx_convert_and_copy_scaled_vector(
			test_vector, test_datatype, test_vector_length,
			double_vector, MXFT_DOUBLE, test_vector_length,
			scale, offset, -DBL_MAX, DBL_MAX );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	fprintf( stderr, "MXFT_DOUBLE = { " );

	for (i = 0; i < test_vector_length; i++ ) {
		fprintf( stderr, "%g, ", double_vector[i] );
	}

	fprintf( stderr, "}\n\n" );

	exit(0);
}

