/*
 * Name:    f_text.c
 *
 * Purpose: Datafile driver for simple text data file with no header.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003, 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_scan.h"
#include "mx_datafile.h"
#include "f_text.h"

MX_DATAFILE_FUNCTION_LIST mxdf_text_datafile_function_list = {
	mxdf_text_open,
	mxdf_text_close,
	mxdf_text_write_main_header,
	mxdf_text_write_segment_header,
	mxdf_text_write_trailer,
	mxdf_text_add_measurement_to_datafile,
	mxdf_text_add_array_to_datafile
};

#define CHECK_FPRINTF_STATUS \
	if ( status == EOF ) { \
		saved_errno = errno; \
		return mx_error( MXE_FILE_IO_ERROR, fname, \
		"Error writing data to datafile '%s'.  Reason = '%s'", \
			datafile->filename, strerror( saved_errno ) ); \
	}

MX_EXPORT mx_status_type
mxdf_text_open( MX_DATAFILE *datafile )
{
	static const char fname[] = "mxdf_text_open()";

	MX_DATAFILE_TEXT *text_file_struct;
	int saved_errno;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( datafile == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DATAFILE pointer passed was NULL.");
	}

	text_file_struct
		= (MX_DATAFILE_TEXT *) malloc( sizeof(MX_DATAFILE_TEXT) );

	if ( text_file_struct == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate MX_DATAFILE_TEXT structure for datafile '%s'",
			datafile->filename );
	}

	datafile->datafile_type_struct = text_file_struct;

	text_file_struct->file = fopen(datafile->filename, "w");

	saved_errno = errno;

	if ( text_file_struct->file == NULL ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
			"Cannot open datafile '%s'.  Reason = '%s'",
			datafile->filename, strerror( saved_errno ) );
	}

	if (setvbuf(text_file_struct->file, (char *)NULL, _IOLBF, BUFSIZ) != 0)
	{
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Cannot set line buffering on datafile '%s'.  Reason = '%s'",
			datafile->filename, strerror( saved_errno ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxdf_text_close( MX_DATAFILE *datafile )
{
	static const char fname[] = "mxdf_text_close()";

	MX_DATAFILE_TEXT *text_file_struct;
	int status, saved_errno;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( datafile == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DATAFILE pointer passed was NULL.");
	}

	text_file_struct
		= (MX_DATAFILE_TEXT *)(datafile->datafile_type_struct);

	if ( text_file_struct == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_DATAFILE_TEXT pointer for datafile '%s' is NULL.",
			datafile->filename );
	}

	if ( text_file_struct->file == NULL ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Datafile '%s' was not open.", datafile->filename );
	}

	status = fclose( text_file_struct->file );

	saved_errno = errno;

	text_file_struct->file = NULL;

	free( text_file_struct );

	datafile->datafile_type_struct = NULL;

	if ( status == EOF ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Attempt to close datafile '%s' failed.  Reason = '%s'",
			datafile->filename, strerror( saved_errno ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxdf_text_write_main_header( MX_DATAFILE *datafile )
{
	/* For a text datafile, this function does nothing. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxdf_text_write_segment_header( MX_DATAFILE *datafile )
{
	/* For a text datafile, this function does nothing. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxdf_text_write_trailer( MX_DATAFILE *datafile )
{
	/* For a text datafile, this function does nothing. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxdf_text_add_measurement_to_datafile( MX_DATAFILE *datafile )
{
	static const char fname[] = "mxdf_text_add_measurement_to_datafile()";

	MX_DATAFILE_TEXT *text_file_struct;
	MX_RECORD **motor_record_array;
	MX_RECORD **input_device_array;
	MX_RECORD *input_device;
	MX_RECORD *x_motor_record;
	MX_SCAN *scan;
	FILE *output_file;
	char buffer[80];
	long i, num_mcas;
	double normalization;
	int status, saved_errno;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( datafile == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DATAFILE pointer passed was NULL.");
	}

	text_file_struct
		= (MX_DATAFILE_TEXT *)(datafile->datafile_type_struct);

	if ( text_file_struct == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_DATAFILE_TEXT pointer for datafile '%s' is NULL.",
			datafile->filename );
	}

	output_file = text_file_struct->file;

	if ( output_file == NULL ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Datafile '%s' is not currently open.",
			datafile->filename );
	}

	scan = (MX_SCAN *) (datafile->scan);

	if ( scan == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The datafile '%s' is not attached to any scan.  scan ptr = NULL.",
			datafile->filename );
	}

	if ( scan->num_motors <= 0 ) {
		motor_record_array = NULL;
	} else {
		motor_record_array = (MX_RECORD **)(scan->motor_record_array);

		if ( motor_record_array == (MX_RECORD **) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
				"The scan '%s' has a NULL motor record array.",
				scan->record->name );
		}
	}

	input_device_array = (MX_RECORD **)(scan->input_device_array);

	if ( input_device_array == (MX_RECORD **) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The scan '%s' has a NULL input device array.",
			scan->record->name );
	}

	/* Print out the current motor positions (if any). */

	if ( scan->datafile.num_x_motors == 0 ) {

		/* By default, we use the axes being scanned. */

		for ( i = 0; i < scan->num_motors; i++ ) {
			if ( (scan->motor_is_independent_variable)[i] ) {
				status = fprintf( output_file, " %-10.*g",
					motor_record_array[i]->precision,
					(scan->motor_position)[i] );

				CHECK_FPRINTF_STATUS;
			}
		}
	} else {
		/* However, if alternate X axis motors have been specified,
		 * we print them instead.
		 */

		if ( scan->datafile.x_position_array == (double **) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The alternate x_position_array pointer for scan '%s' is NULL.",
				scan->record->name );
		}

		for ( i = 0; i < scan->datafile.num_x_motors; i++ ) {
			x_motor_record = scan->datafile.x_motor_array[i];

			status = fprintf( output_file, " %-10.*g",
				x_motor_record->precision,
				scan->datafile.x_position_array[i][0] );

			CHECK_FPRINTF_STATUS;
		}
	}

	/* If we were requested to normalize the data, the 'normalization'
	 * variable is set to the scan measurement time.  Otherwise,
	 * 'normalization' is set to -1.0.
	 */

	if ( scan->datafile.normalize_data ) {
		normalization = mx_scan_get_measurement_time( scan );
	} else {
		normalization = -1.0;
	}

	/* Print out the scaler measurements. */

	num_mcas = 0;

	mx_status = MX_SUCCESSFUL_RESULT;

	for ( i = 0; i < scan->num_input_devices; i++ ) {
		input_device = input_device_array[i];

		if ( input_device->mx_class == MXC_MULTICHANNEL_ANALYZER ) {
			num_mcas++;
		} else {
			mx_status =
			    mx_convert_normalized_device_value_to_string(
				input_device, normalization,
				buffer, sizeof(buffer)-1 );
		}

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		status = fprintf( output_file, " %s", buffer );

		CHECK_FPRINTF_STATUS;
	}

	status = fprintf( output_file, "\n" );

	CHECK_FPRINTF_STATUS;

	status = fflush( output_file );

	CHECK_FPRINTF_STATUS;	/* The same logic works for fflush(). */

	if ( num_mcas == 0 ) {
		return MX_SUCCESSFUL_RESULT;
	} else {
		mx_status = mx_scan_save_mca_measurements( scan, num_mcas );

		return mx_status;
	}
}

MX_EXPORT mx_status_type
mxdf_text_add_array_to_datafile( MX_DATAFILE *datafile,
	long position_type, mx_length_type num_positions, void *position_array,
	long data_type, mx_length_type num_data_points, void *data_array )
{
	static const char fname[] = "mxdf_text_add_array_to_datafile()";

	MX_DATAFILE_TEXT *text_file_struct;
	MX_SCAN *scan;
	FILE *output_file;
	int32_t *int32_position_array, *int32_data_array;
	double *double_position_array, *double_data_array;
	mx_length_type i;
	int status, saved_errno;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( datafile == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DATAFILE pointer passed was NULL.");
	}

	text_file_struct
		= (MX_DATAFILE_TEXT *)(datafile->datafile_type_struct);

	if ( text_file_struct == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_DATAFILE_TEXT pointer for datafile '%s' is NULL.",
			datafile->filename );
	}

	output_file = text_file_struct->file;

	if ( output_file == NULL ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Datafile '%s' is not currently open.",
			datafile->filename );
	}

	scan = (MX_SCAN *) (datafile->scan);

	if ( scan == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The datafile '%s' is not attached to any scan.  scan ptr = NULL.",
			datafile->filename );
	}

	int32_position_array = int32_data_array = NULL;
	double_position_array = double_data_array = NULL;

	/* Construct data type specific array pointers. */

	switch( position_type ) {
	case MXFT_INT32:
		int32_position_array = (void *) position_array;
		break;
	case MXFT_DOUBLE:
		double_position_array = (void *) position_array;
		break;
	default:
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"Only MXFT_INT32 or MXFT_DOUBLE position arrays are supported." );
		break;
	}
	
	switch( data_type ) {
	case MXFT_INT32:
		int32_data_array = (void *) data_array;
		break;
	case MXFT_DOUBLE:
		double_data_array = (void *) data_array;
		break;
	default:
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"Only MXFT_INT32 or MXFT_DOUBLE data arrays are supported." );
		break;
	}
	
	/* Print out the current motor positions (if any). */

	switch( position_type ) {
	case MXFT_INT32:
		for ( i = 0; i < num_positions; i++ ) {
			status = fprintf( output_file, " %-10ld",
					(long)(int32_position_array[i]) );

			CHECK_FPRINTF_STATUS;
		}
		break;
	case MXFT_DOUBLE:
		for ( i = 0; i < num_positions; i++ ) {
			status = fprintf( output_file, " %-10.*g",
				scan->record->precision,
				double_position_array[i] );

			CHECK_FPRINTF_STATUS;
		}
		break;
	}

	/* Print out the scaler measurements. */

	switch( data_type ) {
	case MXFT_INT32:
		for ( i = 0; i < num_data_points; i++ ) {
			status = fprintf( output_file, " %-10ld",
					(long)(int32_data_array[i]) );

			CHECK_FPRINTF_STATUS;
		}
		break;
	case MXFT_DOUBLE:
		for ( i = 0; i < num_data_points; i++ ) {
			status = fprintf( output_file, " %-10.*g",
					scan->record->precision,
					double_data_array[i] );

			CHECK_FPRINTF_STATUS;
		}
		break;
	}

	status = fprintf( output_file, "\n" );

	CHECK_FPRINTF_STATUS;

	status = fflush( output_file );

	CHECK_FPRINTF_STATUS;	/* The same logic works for fflush(). */

	return MX_SUCCESSFUL_RESULT;
}

