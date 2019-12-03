#include <stdio.h>
#include <stdlib.h>
#include <float.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_stdint.h"
#include "mx_array.h"

int
main( int argc, char *argv[] )
{
	static const char fname[] = "scaled_vector";

	char *char_src = NULL;
	unsigned char *uchar_src = NULL;
	short *short_src = NULL;
	unsigned short *ushort_src = NULL;
	mx_bool_type *bool_src = NULL;
	long *long_src = NULL;
	unsigned long *ulong_src = NULL;
	float *float_src = NULL;
	double *double_src = NULL;

	char *char_dest = NULL;
	unsigned char *uchar_dest = NULL;
	short *short_dest = NULL;
	unsigned short *ushort_dest = NULL;
	mx_bool_type *bool_dest = NULL;
	long *long_dest = NULL;
	unsigned long *ulong_dest = NULL;
	float *float_dest = NULL;
	double *double_dest = NULL;

	void *source_vector = NULL;
	long source_vector_length = 0;
	long source_mx_datatype = 0;

	double scale, offset, minimum, maximum, rescaled_value;

	long i, first_arg;

	mx_status_type mx_status;

	/******************************************************************/

	if ( argc < 7 ) {
		fprintf( stderr, "Usage: scaled_vector scale offset "
		    "minimum maximum source_datatype "
		    "first_arg [ second_arg ... ]\n" );
		exit(1);
	}

	scale = atof( argv[1] );
	offset = atof( argv[2] );
	minimum = atof( argv[3] );
	maximum = atof( argv[4] );

	if ( mx_strcasecmp( argv[5], "char" ) == 0 ) {
		source_mx_datatype = MXFT_CHAR;
	} else
	if ( mx_strcasecmp( argv[5], "uchar" ) == 0 ) {
		source_mx_datatype = MXFT_UCHAR;
	} else
	if ( mx_strcasecmp( argv[5], "short" ) == 0 ) {
		source_mx_datatype = MXFT_SHORT;
	} else
	if ( mx_strcasecmp( argv[5], "ushort" ) == 0 ) {
		source_mx_datatype = MXFT_USHORT;
	} else
	if ( mx_strcasecmp( argv[5], "bool" ) == 0 ) {
		source_mx_datatype = MXFT_BOOL;
	} else
	if ( mx_strcasecmp( argv[5], "long" ) == 0 ) {
		source_mx_datatype = MXFT_LONG;
	} else
	if ( mx_strcasecmp( argv[5], "ulong" ) == 0 ) {
		source_mx_datatype = MXFT_ULONG;
	} else
	if ( mx_strcasecmp( argv[5], "float" ) == 0 ) {
		source_mx_datatype = MXFT_FLOAT;
	} else
	if ( mx_strcasecmp( argv[5], "double" ) == 0 ) {
		source_mx_datatype = MXFT_DOUBLE;
	} else
	if ( mx_strcasecmp( argv[5], "string" ) == 0 ) {
		fprintf( stderr,
	  "The MX 'string' datatype is not supported by this test program.\n" );
		exit(1);
	} else {
		fprintf( stderr, "Unrecognized MX datatype '%s'.\n", argv[5] );
		exit(1);
	}

	first_arg = 6;

	source_vector_length = argc - first_arg;

	fprintf( stderr, "\n" );

	/******************************************************************/

	/* Allocate and populate the one, particular source array needed
	 * of the requested MX type.
	 */

	switch( source_mx_datatype ) {
	case MXFT_CHAR:
		char_src = calloc( source_vector_length, sizeof(char) );

		if ( char_src == (char *) NULL )
		{
			mx_error( MXE_OUT_OF_MEMORY, fname,
	"Ran out of memory trying to allocate a %ld element array of chars.\n",
				source_vector_length );
		}

		fprintf( stderr, "%s: original vector (as char) = { ", fname );

		for( i = 0; i < source_vector_length; i++ ) {
			char_src[i] = argv[first_arg + i][0];

			fprintf( stderr, "'%c' %#x, ",
				char_src[i], char_src[i] );
		}

		fprintf( stderr, "}\n" );

		source_vector = char_src;
		break;

	case MXFT_UCHAR:
		uchar_src = calloc( source_vector_length,
					sizeof(unsigned char) );

		if ( uchar_src == (unsigned char *) NULL )
		{
			mx_error( MXE_OUT_OF_MEMORY, fname,
"Ran out of memory trying to allocate a %ld element array of unsigned chars.\n",
				source_vector_length );
		}

		fprintf( stderr, "%s: original vector (as uchar) = { ", fname );

		for( i = 0; i < source_vector_length; i++ ) {
			uchar_src[i] = argv[first_arg + i][0];

			fprintf( stderr, "'%c' %#x, ",
				uchar_src[i], uchar_src[i] );
		}

		fprintf( stderr, "}\n" );

		source_vector = uchar_src;
		break;

	case MXFT_SHORT:
		short_src = calloc( source_vector_length, sizeof(short) );

		if ( short_src == (short *) NULL )
		{
			mx_error( MXE_OUT_OF_MEMORY, fname,
	"Ran out of memory trying to allocate a %ld element array of shorts.\n",
				source_vector_length );
		}

		fprintf( stderr, "%s: original vector (as short) = { ", fname );

		for( i = 0; i < source_vector_length; i++ ) {
			short_src[i] = atof( argv[first_arg + i] );

			fprintf( stderr, "%hd ", short_src[i] );
		}

		fprintf( stderr, "}\n" );

		source_vector = short_src;
		break;

	case MXFT_USHORT:
		ushort_src = calloc( source_vector_length,
					sizeof(unsigned short) );

		if ( ushort_src == (unsigned short *) NULL )
		{
			mx_error( MXE_OUT_OF_MEMORY, fname,
"Ran out of memory trying to allocate a %ld element array of unsigned shorts.\n",
				source_vector_length );
		}

		fprintf( stderr, "%s: original vector (as ushort) = { ", fname);

		for( i = 0; i < source_vector_length; i++ ) {
			ushort_src[i] = atof( argv[first_arg + i] );

			fprintf( stderr, "%hu ", ushort_src[i] );
		}

		fprintf( stderr, "}\n" );

		source_vector = ushort_src;
		break;

	case MXFT_BOOL:
		bool_src = calloc( source_vector_length, sizeof(mx_bool_type) );

		if ( bool_src == (mx_bool_type *) NULL )
		{
			mx_error( MXE_OUT_OF_MEMORY, fname,
	"Ran out of memory trying to allocate a %ld element array of bools.\n",
				source_vector_length );
		}

		fprintf( stderr, "%s: original vector (as bool) = { ", fname );

		for( i = 0; i < source_vector_length; i++ ) {
			bool_src[i] = atof( argv[first_arg + i] );

			fprintf( stderr, "%d ", (int) bool_src[i] );
		}

		fprintf( stderr, "}\n" );

		source_vector = bool_src;
		break;

	case MXFT_LONG:
		long_src = calloc( source_vector_length, sizeof(long) );

		if ( long_src == (long *) NULL )
		{
			mx_error( MXE_OUT_OF_MEMORY, fname,
	"Ran out of memory trying to allocate a %ld element array of longs.\n",
				source_vector_length );
		}

		fprintf( stderr, "%s: original vector (as long) = { ", fname );

		for( i = 0; i < source_vector_length; i++ ) {
			long_src[i] = atof( argv[first_arg + i] );

			fprintf( stderr, "%ld ", long_src[i] );
		}

		fprintf( stderr, "}\n" );

		source_vector = long_src;
		break;

	case MXFT_ULONG:
		ulong_src = calloc( source_vector_length,
					sizeof(unsigned long) );

		if ( ulong_src == (unsigned long *) NULL )
		{
			mx_error( MXE_OUT_OF_MEMORY, fname,
"Ran out of memory trying to allocate a %ld element array of unsigned longs.\n",
				source_vector_length );
		}

		fprintf( stderr, "%s: original vector (as ulong) = { ", fname);

		for( i = 0; i < source_vector_length; i++ ) {
			ulong_src[i] = atof( argv[first_arg + i] );

			fprintf( stderr, "%lu ", ulong_src[i] );
		}

		fprintf( stderr, "}\n" );

		source_vector = ulong_src;
		break;

	case MXFT_FLOAT:
		float_src = calloc( source_vector_length, sizeof(float) );

		if ( float_src == (float *) NULL )
		{
			mx_error( MXE_OUT_OF_MEMORY, fname,
	"Ran out of memory trying to allocate a %ld element array of floats.\n",
				source_vector_length );
		}

		fprintf( stderr, "%s: original vector (as float) = { ", fname );

		for( i = 0; i < source_vector_length; i++ ) {
			float_src[i] = atof( argv[first_arg + i] );

			fprintf( stderr, "%d ", (int) float_src[i] );
		}

		fprintf( stderr, "}\n" );

		source_vector = float_src;
		break;

	case MXFT_DOUBLE:
		double_src = calloc( source_vector_length, sizeof(double) );

		if ( double_src == (double *) NULL )
		{
			mx_error( MXE_OUT_OF_MEMORY, fname,
    "Ran out of memory trying to allocate a %ld element array of doubles.\n",
				source_vector_length );
		}

		fprintf( stderr, "%s: original vector (as double) = { ", fname);

		for( i = 0; i < source_vector_length; i++ ) {
			double_src[i] = atof( argv[first_arg + i] );

			fprintf( stderr, "%d ", (int) double_src[i] );
		}

		fprintf( stderr, "}\n" );

		source_vector = double_src;
		break;

	}

	/******************************************************************/

	/* Next allocate all of the destination arrays. */

	char_dest = calloc( source_vector_length, sizeof(char) );

	if ( char_dest == (char *) NULL ) {
		mx_error( MXE_OUT_OF_MEMORY, fname,
    "Ran out of memory trying to allocate a %ld element char array.",
    			source_vector_length );
		exit(1);
	}

	uchar_dest = calloc( source_vector_length, sizeof(unsigned char) );

	if ( uchar_dest == (unsigned char *) NULL ) {
		mx_error( MXE_OUT_OF_MEMORY, fname,
    "Ran out of memory trying to allocate a %ld element unsigned char array.",
    			source_vector_length );
		exit(1);
	}

	short_dest = calloc( source_vector_length, sizeof(short) );

	if ( short_dest == (short *) NULL ) {
		mx_error( MXE_OUT_OF_MEMORY, fname,
    "Ran out of memory trying to allocate a %ld element short array.",
    			source_vector_length );
		exit(1);
	}

	ushort_dest = calloc( source_vector_length, sizeof(unsigned short) );

	if ( ushort_dest == (unsigned short *) NULL ) {
		mx_error( MXE_OUT_OF_MEMORY, fname,
    "Ran out of memory trying to allocate a %ld element unsigned short array.",
    			source_vector_length );
		exit(1);
	}

	bool_dest = calloc( source_vector_length, sizeof(mx_bool_type) );

	if ( bool_dest == (mx_bool_type *) NULL ) {
		mx_error( MXE_OUT_OF_MEMORY, fname,
    "Ran out of memory trying to allocate a %ld element bool array.",
    			source_vector_length );
		exit(1);
	}

	long_dest = calloc( source_vector_length, sizeof(long) );

	if ( long_dest == (long *) NULL ) {
		mx_error( MXE_OUT_OF_MEMORY, fname,
    "Ran out of memory trying to allocate a %ld element long array.",
    			source_vector_length );
		exit(1);
	}

	ulong_dest = calloc( source_vector_length, sizeof(unsigned long) );

	if ( ulong_dest == (unsigned long *) NULL ) {
		mx_error( MXE_OUT_OF_MEMORY, fname,
    "Ran out of memory trying to allocate a %ld element unsigned long array.",
    			source_vector_length );
		exit(1);
	}

	float_dest = calloc( source_vector_length, sizeof(unsigned long) );

	if ( float_dest == (float *) NULL ) {
		mx_error( MXE_OUT_OF_MEMORY, fname,
    "Ran out of memory trying to allocate a %ld element float array.",
    			source_vector_length );
		exit(1);
	}

	double_dest = calloc( source_vector_length, sizeof(unsigned long) );

	if ( double_dest == (double *) NULL ) {
		mx_error( MXE_OUT_OF_MEMORY, fname,
    "Ran out of memory trying to allocate a %ld element double array.",
    			source_vector_length );
		exit(1);
	}

	/******************************************************************/

	fprintf( stderr, "\nRescaled values = { " );

	for ( i = 0; i < source_vector_length; i++ ) {
		switch( source_mx_datatype ) {
		case MXFT_CHAR:
			rescaled_value = scale * char_src[i] + offset;
			break;
		case MXFT_UCHAR:
			rescaled_value = scale * uchar_src[i] + offset;
			break;
		case MXFT_SHORT:
			rescaled_value = scale * short_src[i] + offset;
			break;
		case MXFT_USHORT:
			rescaled_value = scale * ushort_src[i] + offset;
			break;
		case MXFT_BOOL:
			rescaled_value = scale * bool_src[i] + offset;
			break;
		case MXFT_LONG:
			rescaled_value = scale * long_src[i] + offset;
			break;
		case MXFT_ULONG:
			rescaled_value = scale * ulong_src[i] + offset;
			break;
		case MXFT_FLOAT:
			rescaled_value = scale * float_src[i] + offset;
			break;
		case MXFT_DOUBLE:
			rescaled_value = scale * double_src[i] + offset;
			break;
		}

		fprintf( stderr, "%g ", rescaled_value );
	}

	fprintf( stderr, "}\n\n" );

	/********* Source to char *********/

	mx_status = mx_convert_and_copy_scaled_vector(
			source_vector, source_mx_datatype, source_vector_length,
			char_dest, MXFT_CHAR, source_vector_length,
			scale, offset, minimum, maximum );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	fprintf( stderr, "MXFT_CHAR = { " );

	for (i = 0; i < source_vector_length; i++ ) {
		fprintf( stderr, "'%c' %d, ",
			char_dest[i], (int) char_dest[i] );
	}

	fprintf( stderr, "}\n\n" );

	/********* Source to uchar *********/

	mx_status = mx_convert_and_copy_scaled_vector(
			source_vector, source_mx_datatype, source_vector_length,
			uchar_dest, MXFT_UCHAR, source_vector_length,
			scale, offset, minimum, maximum );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	fprintf( stderr, "MXFT_UCHAR = { " );

	for (i = 0; i < source_vector_length; i++ ) {
		fprintf( stderr, "'%c' %d, ",
			uchar_dest[i], (int) uchar_dest[i] );
	}

	fprintf( stderr, "}\n\n" );

	/********* Source to short *********/

	mx_status = mx_convert_and_copy_scaled_vector(
			source_vector, source_mx_datatype, source_vector_length,
			short_dest, MXFT_SHORT, source_vector_length,
			scale, offset, minimum, maximum );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	fprintf( stderr, "MXFT_SHORT = { " );

	for (i = 0; i < source_vector_length; i++ ) {
		fprintf( stderr, "%hd, ", short_dest[i] );
	}

	fprintf( stderr, "}\n\n" );

	/********* Source to ushort *********/

	mx_status = mx_convert_and_copy_scaled_vector(
			source_vector, source_mx_datatype, source_vector_length,
			ushort_dest, MXFT_USHORT, source_vector_length,
			scale, offset, minimum, maximum );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	fprintf( stderr, "MXFT_USHORT = { " );

	for (i = 0; i < source_vector_length; i++ ) {
		fprintf( stderr, "%hu, ", ushort_dest[i] );
	} 

	fprintf( stderr, "}\n\n" );

	/********* Source to bool *********/

	mx_status = mx_convert_and_copy_scaled_vector(
			source_vector, source_mx_datatype, source_vector_length,
			bool_dest, MXFT_BOOL, source_vector_length,
			scale, offset, minimum, maximum );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	fprintf( stderr, "MXFT_BOOL = { " );

	for (i = 0; i < source_vector_length; i++ ) {
		fprintf( stderr, "%ld, ", (long) bool_dest[i] );
	}

	fprintf( stderr, "}\n\n" );

	/********* Source to long *********/

	mx_status = mx_convert_and_copy_scaled_vector(
			source_vector, source_mx_datatype, source_vector_length,
			long_dest, MXFT_LONG, source_vector_length,
			scale, offset, minimum, maximum );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	fprintf( stderr, "MXFT_LONG = { " );

	for (i = 0; i < source_vector_length; i++ ) {
		fprintf( stderr, "%ld, ", long_dest[i] );
	}

	fprintf( stderr, "}\n\n" );

	/********* Source to ulong *********/

	mx_status = mx_convert_and_copy_scaled_vector(
			source_vector, source_mx_datatype, source_vector_length,
			ulong_dest, MXFT_ULONG, source_vector_length,
			scale, offset, minimum, maximum );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	fprintf( stderr, "MXFT_ULONG = { " );

	for (i = 0; i < source_vector_length; i++ ) {
		fprintf( stderr, "%lu, ", ulong_dest[i] );
	}

	fprintf( stderr, "}\n\n" );

	/********* Source to float *********/

	mx_status = mx_convert_and_copy_scaled_vector(
			source_vector, source_mx_datatype, source_vector_length,
			float_dest, MXFT_FLOAT, source_vector_length,
			scale, offset, minimum, maximum );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	fprintf( stderr, "MXFT_FLOAT = { " );

	for (i = 0; i < source_vector_length; i++ ) {
		fprintf( stderr, "%g, ", float_dest[i] );
	}

	fprintf( stderr, "}\n\n" );

	/********* Source to double *********/

	mx_status = mx_convert_and_copy_scaled_vector(
			source_vector, source_mx_datatype, source_vector_length,
			double_dest, MXFT_DOUBLE, source_vector_length,
			scale, offset, minimum, maximum );

	if ( mx_status.code != MXE_SUCCESS )
		exit( mx_status.code );

	fprintf( stderr, "MXFT_DOUBLE = { " );

	for (i = 0; i < source_vector_length; i++ ) {
		fprintf( stderr, "%g, ", double_dest[i] );
	}

	fprintf( stderr, "}\n\n" );

	exit(0);
}

