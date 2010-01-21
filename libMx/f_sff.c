/*
 * Name:    f_sff.c
 *
 * Purpose: Datafile driver for simple file format data file with no header.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003, 2005-2006, 2009-2010
 *    Illinois Institute of Technology
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
#include "mx_time.h"
#include "mx_stdint.h"
#include "mx_array.h"
#include "mx_driver.h"
#include "mx_scan.h"
#include "mx_variable.h"
#include "mx_datafile.h"
#include "f_sff.h"

MX_DATAFILE_FUNCTION_LIST mxdf_sff_datafile_function_list = {
	mxdf_sff_open,
	mxdf_sff_close,
	mxdf_sff_write_main_header,
	mxdf_sff_write_segment_header,
	mxdf_sff_write_trailer,
	mxdf_sff_add_measurement_to_datafile,
	mxdf_sff_add_array_to_datafile
};

#define MX_SFF_LINE_TERMINATOR  '|'
#define MX_SFF_TOKEN_SEPARATORS " \t"

static mx_status_type mxdf_sff_write_token_value(
				MX_DATAFILE *, FILE *, char *, MX_RECORD *);
static mx_status_type mxdf_sff_handle_special_token(
				MX_DATAFILE *, FILE *, char *, MX_RECORD *);

#define FAILURE			0

#define CHECK_FPRINTF_STATUS \
	if ( status == EOF ) { \
		saved_errno = errno; \
		return mx_error( MXE_FILE_IO_ERROR, fname, \
		"Error writing data to datafile '%s'.  Reason = '%s'", \
			datafile->filename, strerror( saved_errno ) ); \
	}

MX_EXPORT mx_status_type
mxdf_sff_open( MX_DATAFILE *datafile )
{
	static const char fname[] = "mxdf_sff_open()";

	MX_DATAFILE_SFF *sff_file_struct;
	int saved_errno;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( datafile == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DATAFILE pointer passed was NULL.");
	}

	sff_file_struct
		= (MX_DATAFILE_SFF *) malloc( sizeof(MX_DATAFILE_SFF) );

	if ( sff_file_struct == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate MX_DATAFILE_SFF structure for datafile '%s'",
			datafile->filename );
	}

	datafile->datafile_type_struct = sff_file_struct;

	sff_file_struct->file = fopen(datafile->filename, "w");

	saved_errno = errno;

	if ( sff_file_struct->file == NULL ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
			"Cannot open datafile '%s'.  Reason = '%s'",
			datafile->filename, strerror( saved_errno ) );
	}

	if (setvbuf(sff_file_struct->file, (char *)NULL, _IOLBF, BUFSIZ) != 0)
	{
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Cannot set line buffering on datafile '%s'.  Reason = '%s'",
			datafile->filename, strerror( saved_errno ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxdf_sff_close( MX_DATAFILE *datafile )
{
	static const char fname[] = "mxdf_sff_close()";

	MX_DATAFILE_SFF *sff_file_struct;
	int status, saved_errno;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( datafile == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DATAFILE pointer passed was NULL.");
	}

	sff_file_struct
		= (MX_DATAFILE_SFF *)(datafile->datafile_type_struct);

	if ( sff_file_struct == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_DATAFILE_SFF pointer for datafile '%s' is NULL.",
			datafile->filename );
	}

	if ( sff_file_struct->file == NULL ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Datafile '%s' was not open.", datafile->filename );
	}

	status = fclose( sff_file_struct->file );

	saved_errno = errno;

	sff_file_struct->file = NULL;

	free( sff_file_struct );

	datafile->datafile_type_struct = NULL;

	if ( status == EOF ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Attempt to close datafile '%s' failed.  Reason = '%s'",
			datafile->filename, strerror( saved_errno ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxdf_sff_write_main_header( MX_DATAFILE *datafile )
{
	return mxdf_sff_write_header( datafile, NULL, MX_SFF_MAIN_HEADER );
}

MX_EXPORT mx_status_type
mxdf_sff_write_segment_header( MX_DATAFILE *datafile )
{
	return mxdf_sff_write_header( datafile, NULL, MX_SFF_SEGMENT_HEADER );
}

MX_EXPORT mx_status_type
mxdf_sff_write_trailer( MX_DATAFILE *datafile )
{
	return mxdf_sff_write_header( datafile, NULL, MX_SFF_TRAILER );
}

MX_EXPORT mx_status_type
mxdf_sff_add_measurement_to_datafile( MX_DATAFILE *datafile )
{
	static const char fname[] = "mxdf_sff_add_measurement_to_datafile()";

	MX_DATAFILE_SFF *sff_file_struct;
	MX_RECORD **motor_record_array;
	MX_RECORD *motor_record;
	MX_MOTOR *motor;
	MX_RECORD **input_device_array;
	MX_RECORD *input_device;
	MX_RECORD *x_motor_record;
	MX_SCAN *scan;
	FILE *output_file;
	char buffer[80];
	long i, num_mcas;
	double normalization;
	int status, saved_errno;
	mx_bool_type early_move_flag;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( datafile == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DATAFILE pointer passed was NULL.");
	}

	sff_file_struct
		= (MX_DATAFILE_SFF *)(datafile->datafile_type_struct);

	if ( sff_file_struct == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_DATAFILE_SFF pointer for datafile '%s' is NULL.",
			datafile->filename );
	}

	output_file = sff_file_struct->file;

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

	mx_status = mx_scan_get_early_move_flag( scan, &early_move_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

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

			    motor_record = motor_record_array[i];

			    if ( early_move_flag ) {
			        motor = (MX_MOTOR *)
					motor_record->record_class_struct;

				status = fprintf( output_file, " %-10.*g",
					motor_record->precision,
					motor->old_destination );
			    } else {
				status = fprintf( output_file, " %-10.*g",
					motor_record->precision,
					(scan->motor_position)[i] );
			    }
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

		switch( input_device->mx_class ) {
		case MXC_MULTICHANNEL_ANALYZER:
			num_mcas++;

			buffer[0] = '\0';
			break;

		case MXC_AREA_DETECTOR:
			mx_status = mx_scan_save_area_detector_image(
						scan, input_device );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
			break;

		default:
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
mxdf_sff_add_array_to_datafile( MX_DATAFILE *datafile,
		long position_type, long num_positions, void *position_array,
		long data_type, long num_data_points, void *data_array )
{
	static const char fname[] = "mxdf_sff_add_array_to_datafile()";

	MX_DATAFILE_SFF *sff_file_struct;
	MX_SCAN *scan;
	FILE *output_file;
	long *long_position_array, *long_data_array;
	double *double_position_array, *double_data_array;
	long i;
	int status, saved_errno;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( datafile == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DATAFILE pointer passed was NULL.");
	}

	sff_file_struct
		= (MX_DATAFILE_SFF *)(datafile->datafile_type_struct);

	if ( sff_file_struct == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_DATAFILE_SFF pointer for datafile '%s' is NULL.",
			datafile->filename );
	}

	output_file = sff_file_struct->file;

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

	long_position_array = long_data_array = NULL;
	double_position_array = double_data_array = NULL;

	/* Construct data type specific array pointers. */

	switch( position_type ) {
	case MXFT_LONG:
		long_position_array = (void *) position_array;
		break;
	case MXFT_DOUBLE:
		double_position_array = (void *) position_array;
		break;
	default:
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"Only MXFT_LONG or MXFT_DOUBLE position arrays are supported." );
	}
	
	switch( data_type ) {
	case MXFT_LONG:
		long_data_array = (void *) data_array;
		break;
	case MXFT_DOUBLE:
		double_data_array = (void *) data_array;
		break;
	default:
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"Only MXFT_LONG or MXFT_DOUBLE data arrays are supported." );
	}
	
	/* Print out the current motor positions (if any). */

	switch( position_type ) {
	case MXFT_LONG:
		for ( i = 0; i < num_positions; i++ ) {
			status = fprintf( output_file, " %-10ld",
				long_position_array[i] );

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
	case MXFT_LONG:
		for ( i = 0; i < num_data_points; i++ ) {
			status = fprintf( output_file, " %-10ld",
					long_data_array[i] );

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

MX_EXPORT mx_status_type
mxdf_sff_write_header( MX_DATAFILE *datafile, FILE *file, int header_type )
{
	static const char fname[] = "mxdf_sff_write_header()";

	MX_RECORD *record_list, *record;
	MX_DATAFILE_SFF *sff_file_struct;
	MX_SCAN *scan;
	FILE *output_file;
	char header_fmt[500];
	char header_fmt_name[20];
	char *ptr, *token_ptr, *token_end;
	long num_lines, num_matched, num_not_matched;
	int status, saved_errno;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked; header_type = %d", fname, header_type));

	if ( datafile == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_DATAFILE pointer passed was NULL.");
	}

	if ( header_type == MX_SFF_MCA_HEADER ) {
		if ( file == (FILE *) NULL ) {
			return mx_error( MXE_NULL_ARGUMENT, fname,
			"The file pointer for an SFF MCA file was NULL." );
		}

		output_file = file;
	} else {
		sff_file_struct
			= (MX_DATAFILE_SFF *)(datafile->datafile_type_struct);

		if ( sff_file_struct == NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_DATAFILE_SFF pointer for datafile '%s' is NULL.",
				datafile->filename );
		}

		output_file = sff_file_struct->file;

		if ( output_file == NULL ) {
			return mx_error( MXE_FILE_IO_ERROR, fname,
			"Datafile '%s' is not currently open.",
				datafile->filename );
		}
	}

	scan = (MX_SCAN *) (datafile->scan);

	if ( scan == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The datafile '%s' is not attached to any scan.  scan ptr = NULL.",
			datafile->filename );
	}

	switch( header_type ) {
	case MX_SFF_MAIN_HEADER:
		strcpy( header_fmt_name, "sff_header_fmt" );
		break;
	case MX_SFF_SEGMENT_HEADER:
		strcpy( header_fmt_name, "sff_segment_fmt" );
		break;
	case MX_SFF_TRAILER:
		strcpy( header_fmt_name, "sff_trailer_fmt" );
		break;
	case MX_SFF_MCA_HEADER:
		strcpy( header_fmt_name, "sff_mca_fmt" );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unknown header type %d", header_type );
	}

	/* See if the header format record exists.  If it does not exist,
	 * then we just skip this header.
	 */

	record_list = scan->record->list_head;

	record = mx_get_record( record_list, header_fmt_name );

	if ( record == (MX_RECORD *) NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* Get the SFF header format string for this header. */

	mx_status = mx_get_string_variable_by_name(
			record_list, header_fmt_name,
			&header_fmt[0], sizeof(header_fmt)-1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* How many lines are there in the header, not counting the
	 * line with the SFF magic token in it?
	 */

	ptr = &header_fmt[0];

	if ( strspn(ptr, MX_SFF_TOKEN_SEPARATORS) < strlen(ptr)  ) {

		/* If the token doesn't consist entirely of token
		 * separator characters, then there must be at least
		 * one line in the main part of the header.
		 */

		num_lines = 1;
	} else {
		num_lines = 0;
	}

	while ( ptr != NULL && *ptr != '\0' ) {
		if ( *ptr == MX_SFF_LINE_TERMINATOR ) {
			num_lines++;
		}
		ptr++;
	}

	/* Write out the first line of the header. */

	switch( header_type ) {
	case MX_SFF_MAIN_HEADER:
		status = fprintf( output_file,
				"# SFF main_header %ld\n", num_lines );
		break;
	case MX_SFF_SEGMENT_HEADER:
		status = fprintf( output_file,
				"# SFF segment_header %ld\n", num_lines );
		break;
	case MX_SFF_TRAILER:
		status = fprintf( output_file,
				"# SFF trailer %ld\n", num_lines );
		break;
	case MX_SFF_MCA_HEADER:
		status = fprintf( output_file,
				"# SFF mca_header %ld\n", num_lines );
		break;
	default:
		status = FAILURE;
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unknown header type %d", header_type );
	}
	CHECK_FPRINTF_STATUS;

	MX_DEBUG( 2,("%s: num_lines = %ld", fname, num_lines));

	if ( num_lines == 0 ) {
		/* If the number of lines remaining to be written is zero,
		 * then we are done.
		 */

		return MX_SUCCESSFUL_RESULT;
	}

	/* Now write out the main part of the header. */

	status = fprintf( output_file, "# " );

	ptr = &header_fmt[0];

	while ( ptr != NULL ) {

		/* Skip leading white space. */

		num_matched = (long) strspn( ptr, MX_SFF_TOKEN_SEPARATORS );

		token_ptr = ptr + num_matched;

		MX_DEBUG( 2,("%s: num_matched = %ld, token_ptr = %p",
			fname, num_matched, token_ptr));

		MX_DEBUG( 2,("%s: token_ptr = '%s'", fname, token_ptr));

		/* Is this a line break token? */

		if ( *token_ptr == MX_SFF_LINE_TERMINATOR ) {
			MX_DEBUG( 2,("%s: End of line.", fname));

			status = fprintf( output_file, "\n# " );

			CHECK_FPRINTF_STATUS;

			ptr = token_ptr + 1;

			continue;   /* Cycle back to the while(1) statement. */

		}
		/* Otherwise, find the end of the token. */

		num_not_matched =
			(long) strcspn(token_ptr, MX_SFF_TOKEN_SEPARATORS);

		MX_DEBUG( 2,("%s: num_not_matched = %ld",
					fname, num_not_matched));

		if ( num_not_matched == 0 ) {
			/* We've reached the end of the tokens. */

			MX_DEBUG( 2,("%s: End of tokens #1.",fname));

			break;		/* Exit the while(1) loop. */
		}

		token_end = token_ptr + num_not_matched;

		if ( *token_end == '\0' ) {
			ptr = NULL;
		} else {
			ptr = token_end + 1;

			/* Null terminate the token. */

			*token_end = '\0';
		}

		/* Write out the name of the token. */

		MX_DEBUG( 2,("%s: Token = '%s'", fname, token_ptr));

		status = fprintf( output_file, "%s = ", token_ptr );

		CHECK_FPRINTF_STATUS;

		if ( *token_ptr == '%' ) {
			mx_status = mxdf_sff_handle_special_token( datafile,
				output_file, token_ptr, record_list );
		} else {
			mx_status = mxdf_sff_write_token_value( datafile,
				output_file, token_ptr, record_list );
		}

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( ptr == NULL ) {
			MX_DEBUG( 2,("%s: End of tokens #2.", fname));

			status = fprintf( output_file, ";\n" );
		} else {
			status = fprintf( output_file, "; " );
		}
		CHECK_FPRINTF_STATUS;
	}
	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxdf_sff_write_token_value( MX_DATAFILE *datafile,
		FILE *output_file, char *token, MX_RECORD *record_list )
{
	static const char fname[] = "mxdf_sff_write_token_value()";

	mx_status_type mx_status;
	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	char token_buffer[500];
	void *data_ptr, *value_ptr;
	long field_type;
	int status, saved_errno;
	char *record_name, *field_name, *dot_ptr, *ptr;
	static char value_field_name[] = "value";

	mx_status_type ( *token_constructor ) ( void *, char *, size_t,
					MX_RECORD *, MX_RECORD_FIELD * );

	MX_DEBUG( 2,("%s invoked for token '%s'.", fname, token));

	dot_ptr = strchr( token, '.' );

	record_name = token;

	if ( dot_ptr == NULL ) {
		field_name = value_field_name;
	} else {
		*dot_ptr = '\0';	/* Null terminate the record name. */

		field_name = dot_ptr + 1;
	}

	record = mx_get_record( record_list, record_name );

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NOT_FOUND, fname,
			"Record '%s' does not exist.", record_name );
	}

	mx_status = mx_find_record_field( record, field_name, &record_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field_type = record_field->datatype;

	data_ptr = record_field->data_pointer;

	if ( record_field->flags & MXFF_VARARGS ) {
		value_ptr =
			mx_read_void_pointer_from_memory_location( data_ptr );
	} else {
		value_ptr = data_ptr;
	}

	mx_status = mx_get_token_constructor( field_type, &token_constructor );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Construct the string description of this field. */

	strcpy( token_buffer, "" );

	if ( (record_field->num_dimensions == 0)
	  || ((field_type == MXFT_STRING)
	     && (record_field->num_dimensions == 1)) ) {

		mx_status = (*token_constructor)( value_ptr,
				token_buffer, sizeof( token_buffer ) - 1,
				record, record_field );

	} else {
		mx_status = mx_create_array_description( value_ptr, 0,
				token_buffer, sizeof( token_buffer ) - 1,
				record, record_field, token_constructor );
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* There may be an extra space in front if we invoked
	 * mx_create_array_description().
	 */

	ptr = &token_buffer[0];

	if ( *ptr == ' ' ) {
		ptr++;
	}

	MX_DEBUG( 2,("%s: Constructed token buffer = '%s'",
						fname, ptr));

	status = fprintf( output_file, "%s", ptr );

	CHECK_FPRINTF_STATUS;

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxdf_sff_handle_special_token( MX_DATAFILE *datafile,
		FILE *output_file, char *token, MX_RECORD *record_list )
{
	static const char fname[] = "mxdf_sff_handle_special_token()";

	MX_SCAN *scan;
	MX_RECORD_FIELD *record_field;
	char rfname[ MXU_RECORD_FIELD_NAME_LENGTH + 1 ];
	char *token_ptr, *ptr;
	int first_motor;
	long i;
	mx_status_type mx_status;

	scan = (MX_SCAN *) (datafile->scan);

	/* The first character of the token indicated that this is
	 * a special token, so we skip over the first character.
	 */

	token_ptr = token + 1;

	/* Now identify which special token this is. */

	if ( strcmp( token_ptr, "devices" ) == 0 ) {

		/* This is a request to list the names of all the devices
		 * whose values are contained in the body of the data file.
		 */

		first_motor = TRUE;

		for ( i = 0; i < scan->num_motors; i++ ) {
			if ( scan->motor_is_independent_variable[i] ) {
				if ( first_motor ) {
					first_motor = FALSE;
				} else {
					fprintf( output_file, " " );
				}
				fprintf( output_file, "%s",
					scan->motor_record_array[i]->name );
			}
		}
		for ( i = 0; i < scan->num_input_devices; i++ ) {
			fprintf( output_file, " %s",
					scan->input_device_array[i]->name );
		}

	} else if ( strcmp( token_ptr, "motors" ) == 0 ) {

		/* This is a request to list the names of only the motors
		 * used in this scan.
		 */

		first_motor = TRUE;

		for ( i = 0; i < scan->num_motors; i++ ) {
			if ( scan->motor_is_independent_variable[i] ) {
				if ( first_motor ) {
					first_motor = FALSE;
				} else {
					fprintf( output_file, " " );
				}
				fprintf( output_file, "%s",
					scan->motor_record_array[i]->name );
			}
		}

	} else if ( strcmp( token_ptr, "x_motors" ) == 0 ) {

		/* This is a request to list the names of any alternative
		 * X axis motors specified for this scan via an 'x='
		 * directive in the scan datafile type string.
		 */

		first_motor = TRUE;

		for ( i = 0; i < scan->datafile.num_x_motors; i++ ) {
			if ( first_motor ) {
				first_motor = FALSE;
			} else {
				fprintf( output_file, " " );
			}
			fprintf( output_file, "%s",
				scan->datafile.x_motor_array[i]->name );
		}

	} else if ( strcmp( token_ptr, "inputs" ) == 0 ) {

		/* This is a request to list the names of only the 
		 * input devices.
		 */

		for ( i = 0; i < scan->num_input_devices; i++ ) {
			if ( i > 0 ) {
				fprintf( output_file, " " );
			}
			fprintf( output_file, "%s",
					scan->input_device_array[i]->name );
		}

	} else if ( strncmp( token_ptr, "scan.", 5 ) == 0 ) {

		/* This token allows a field in the current scan to be
		 * referred to via the the pseudo record name of %scan.
		 * Thus, one could put '%scan.step_size' in the SFF
		 * format string and expect it to work regardless of
		 * the name of the scan.
		 *
		 * Any field names that do not exist in the current
		 * scan record are silently ignored.
		 */

		ptr = token_ptr + 5;

		record_field = mx_get_record_field( scan->record, ptr );

		if ( record_field != (MX_RECORD_FIELD *) NULL ) {

			sprintf( rfname, "%s.%s", scan->record->name, ptr );

			mx_status = mxdf_sff_write_token_value( datafile,
				output_file, rfname, record_list );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

	} else if ( strcmp( token_ptr, "date" ) == 0 ) {

		char local_buffer[80];

		mx_current_time_string( local_buffer, sizeof(local_buffer) );

		fprintf( output_file, "\"%s\"", local_buffer );

	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The special token '%s' in the SFF format string is not "
		"a valid special token.", token_ptr );
	}

	return MX_SUCCESSFUL_RESULT;
}

