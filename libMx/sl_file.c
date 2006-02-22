/*
 * Name:    sl_file.c
 *
 * Purpose: Scan description file for list scans that read positions
 *          from a file.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003-2004 Illinois Institute of Technology
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
#include "mx_scan.h"
#include "mx_scan_list.h"
#include "mx_measurement.h"
#include "m_time.h"
#include "m_count.h"
#include "m_pulse_period.h"
#include "sl_file.h"

/* Initialize the list scan driver jump table. */

MX_LIST_SCAN_FUNCTION_LIST mxs_file_list_scan_function_list = {
	mxs_file_list_scan_create_record_structures,
	mxs_file_list_scan_finish_record_initialization,
	mxs_file_list_scan_delete_record,
	mxs_file_list_scan_open_list,
	mxs_file_list_scan_close_list,
	mxs_file_list_scan_get_next_measurement_parameters,
};

MX_RECORD_FIELD_DEFAULTS mxs_file_list_scan_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SCAN_STANDARD_FIELDS,
	MX_FILE_LIST_SCAN_STANDARD_FIELDS
};

long mxs_file_list_scan_num_record_fields
			= sizeof( mxs_file_list_scan_defaults )
			/ sizeof( mxs_file_list_scan_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxs_file_list_scan_def_ptr
			= &mxs_file_list_scan_defaults[0];

MX_EXPORT mx_status_type
mxs_file_list_scan_create_record_structures( MX_RECORD *record,
		MX_SCAN *scan, MX_LIST_SCAN *list_scan )
{
	static const char fname[] =
		"mxs_file_list_scan_create_record_structures()";

	MX_FILE_LIST_SCAN *file_list_scan;

	file_list_scan = (MX_FILE_LIST_SCAN *)
				malloc( sizeof(MX_FILE_LIST_SCAN) );

	if ( file_list_scan == (MX_FILE_LIST_SCAN *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Out of memory allocating MX_FILE_LIST_SCAN structure for record '%s'",
			record->name );
	}

	file_list_scan->position_file = NULL;

	strcpy( file_list_scan->position_filename, "" );

	record->record_type_struct = file_list_scan;
	list_scan->record_type_struct = file_list_scan;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_file_list_scan_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxs_file_list_scan_finish_record_initialization()";

	MX_SCAN *scan;
	long i;

	record->class_specific_function_list
					= &mxs_file_list_scan_function_list;

	scan = (MX_SCAN *)(record->record_superclass_struct);

	record->class_specific_function_list
					= &mxs_file_list_scan_function_list;

	if ( scan->num_independent_variables != scan->num_motors ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"num_independent_variables = %ld is not the same as num_motors = %ld.  "
"This is not allowed for a list scan.",
			scan->num_independent_variables,
			scan->num_motors );
	}

	for ( i = 0; i < scan->num_motors; i++ ) {
		scan->motor_is_independent_variable[i] = TRUE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_file_list_scan_delete_record( MX_RECORD *record )
{
	MX_SCAN *scan;
	MX_LIST_SCAN *list_scan;
	MX_FILE_LIST_SCAN *file_list_scan;

	scan = (MX_SCAN *)(record->record_superclass_struct);
	list_scan = (MX_LIST_SCAN *)(record->record_class_struct);
	file_list_scan = (MX_FILE_LIST_SCAN *)(record->record_type_struct);

	if ( file_list_scan != (MX_FILE_LIST_SCAN *) NULL ) {
		free( file_list_scan );
	}

	record->record_type_struct = NULL;
	list_scan->record_type_struct = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_file_list_scan_open_list( MX_LIST_SCAN *list_scan )
{
	static const char fname[] = "mxs_file_list_scan_open_list()";

	MX_FILE_LIST_SCAN *file_list_scan;
	int saved_errno;

	MX_DEBUG( 2,("%s invoked.",fname));

	file_list_scan = (MX_FILE_LIST_SCAN *)(list_scan->record_type_struct);

	if ( file_list_scan == (MX_FILE_LIST_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_FILE_LIST_SCAN pointer is NULL." );
	}

	if ( strlen( file_list_scan->position_filename ) == 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"No motor position filename specified." );
	}

	file_list_scan->position_file
			= fopen( file_list_scan->position_filename, "r" );

	saved_errno = errno;

	if ( file_list_scan->position_file == NULL ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
		"Cannot open motor position file '%s'.  Reason = '%s'",
			file_list_scan->position_filename,
			strerror( saved_errno ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_file_list_scan_close_list( MX_LIST_SCAN *list_scan )
{
	static const char fname[] = "mxs_file_list_scan_close_list()";

	MX_FILE_LIST_SCAN *file_list_scan;
	int status, saved_errno;

	MX_DEBUG( 2,("%s invoked.",fname));

	file_list_scan = (MX_FILE_LIST_SCAN *)(list_scan->record_type_struct);

	if ( file_list_scan == (MX_FILE_LIST_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_FILE_LIST_SCAN pointer is NULL." );
	}

	if ( file_list_scan->position_file == NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The motor position file '%s' was not open.",
			file_list_scan->position_filename );
	}

	status = fclose( file_list_scan->position_file );

	saved_errno = errno;

	if ( status == EOF ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
	"Attempt to close motor position file '%s' failed.  Reason = '%s'.",
			file_list_scan->position_filename,
			strerror( saved_errno ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_file_list_scan_get_next_measurement_parameters( MX_SCAN *scan,
						MX_LIST_SCAN *list_scan )
{
	static const char fname[] =
		"mxs_file_list_scan_get_next_measurement_parameters()";

	MX_MEASUREMENT_PRESET_TIME *preset_time_struct;
	MX_MEASUREMENT_PRESET_COUNT *preset_count_struct;
	MX_MEASUREMENT_PRESET_PULSE_PERIOD *preset_pulse_period_struct;
	MX_FILE_LIST_SCAN *file_list_scan;
	FILE *file;
	char buffer[250];
	char *ptr, *token_ptr, *token_end;
	double *motor_position;
	double double_value;
	int num_items, saved_errno;
	long i;
	size_t num_matched, num_not_matched;

	MX_DEBUG( 2,("%s invoked.",fname));

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SCAN pointer passed was NULL." );
	}

	if ( list_scan == (MX_LIST_SCAN *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_LIST_SCAN_POINTER passed was NULL." );
	}

	file_list_scan = (MX_FILE_LIST_SCAN *) list_scan->record_type_struct;

	if ( file_list_scan == (MX_FILE_LIST_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_FILE_LIST_SCAN pointer is NULL." );
	}

	file = file_list_scan->position_file;

	if ( file == NULL ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
		"The motor position file '%s' is not open.",
			file_list_scan->position_filename );
	}

	motor_position = scan->motor_position;

	if ( motor_position == (double *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"motor_position array pointer is NULL." );
	}

	fgets( buffer, sizeof buffer, file );

	saved_errno = errno;

	if ( ferror( file ) ) {
		return mx_error( MXE_FILE_IO_ERROR, fname,
	"Error while reading from motor position file '%s'.  Reason = '%s'.",
			file_list_scan->position_filename,
			strerror( saved_errno ) );
	}

	/* Getting an end-of-file is not a true error.  It just means
	 * what it says, namely, that we have reached the end of the
	 * motor position file.
	 */

	if ( feof( file ) ) {
		return mx_error_quiet( MXE_END_OF_DATA, fname,
		"End of file on motor position file '%s'.",
			file_list_scan->position_filename );
	}

	/* Loop through the measurement time and motor positions.
	 * The case 'i == -1' is used for the measurement time.
	 */

	ptr = &buffer[0];

	for ( i = -1; i < scan->num_motors; i++ ) {

		/* Skip leading white space. */

		num_matched = strspn( ptr, MX_RECORD_FIELD_SEPARATORS );

		token_ptr = ptr + num_matched;

		/* Find the end of the token. */

		num_not_matched
			= strcspn( token_ptr, MX_RECORD_FIELD_SEPARATORS );

		if ( num_not_matched == 0 ) {
			if ( i == -1 ) {
				return mx_error(
					MXE_UNEXPECTED_END_OF_DATA, fname,
					"No values were seen on line." );
			} else {
				return mx_error(
					MXE_UNEXPECTED_END_OF_DATA, fname,
		"Only %ld motor positions on line when %ld were expected.",
				i, scan->num_motors );
			}
		}

		token_end = token_ptr + num_not_matched;

		ptr = token_end + 1;

		/* Null terminate the token. */

		*token_end = '\0';

		/* Now parse the token. */

		num_items = sscanf( token_ptr, "%lg", &double_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
			"The token '%s' is not a number.", token_ptr );
		}

		if ( i == -1 ) {
			switch( scan->measurement.type ) {
			case MXM_PRESET_TIME:
				preset_time_struct =
				    (MX_MEASUREMENT_PRESET_TIME *)
				scan->measurement.measurement_type_struct;

				preset_time_struct->integration_time
							= double_value;
				break;

			case MXM_PRESET_COUNT:
				preset_count_struct =
				    (MX_MEASUREMENT_PRESET_COUNT *)
				scan->measurement.measurement_type_struct;

				preset_count_struct->preset_count
						= mx_round( double_value );
				break;

			case MXM_PRESET_PULSE_PERIOD:
				preset_pulse_period_struct =
				    (MX_MEASUREMENT_PRESET_PULSE_PERIOD *)
				scan->measurement.measurement_type_struct;

				preset_pulse_period_struct->pulse_period
							= double_value;
				break;
			default:
				return mx_error( MXE_UNSUPPORTED, fname,
			"Unsupported measurement type %ld for scan '%s'.",
					scan->measurement.type,
					scan->record->name );
				break;
			}
		} else {
			motor_position[i] = double_value;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

