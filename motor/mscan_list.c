/*
 * Name:    mscan_list.c
 *
 * Purpose: Motor list scan setup and modify function.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

/* If old_scan == NULL, we are doing a new scan setup.  Otherwise, we are
 * modifying a previous scan.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "motor.h"
#include "mdialog.h"
#include "mx_array.h"
#include "mx_scan_list.h"
#include "sl_file.h"

int
motor_setup_list_scan_parameters(
	char *scan_name,
	MX_SCAN *old_scan,
	char *record_description_buffer,
	size_t record_description_buffer_length,
	char *input_devices_string,
	char *measurement_parameters_string,
	char *datafile_and_plot_parameters_string )
{
	const char fname[] = "motor_setup_list_scan_parameters()";

	MX_RECORD *record;
	MX_RECORD **motor_record_array;
	MX_FILE_LIST_SCAN *file_list_scan;
	mx_status_type mx_status;
	static char buffer[ MXU_RECORD_DESCRIPTION_LENGTH + 1 ];
	char prompt[100];
	char scan_position_filename[MXU_FILENAME_LENGTH + 1];
	int i, status;
	int string_length;
	long scan_class, scan_type;
	long scan_num_scans;
	long scan_num_independent_variables;
	long scan_num_motors;
	long old_scan_num_motors;
	unsigned long scan_flags;
	int *motor_is_independent_variable;

	char **motor_name_array;
	long motor_name_dimension_array[2];

	static size_t name_element_size_array[2] = {
		sizeof(char), sizeof(char *) };

	long default_long;
	char *default_string_ptr;

	scan_class = MXS_LIST_SCAN;

	if ( old_scan == (MX_SCAN *) NULL ) {
		/* Only file list scans are currently supported. */

		scan_type = MXS_LST_FILE;
	} else {
		/* 'modify scan' is not allowed to change the scan type.
		 * Use 'setup scan' if you want a different scan type.
		 */

		scan_type = old_scan->record->mx_type;
	}

	/* Set the number of times to scan. */

	if ( old_scan == NULL ) {
		default_long = 1;
	} else {
		default_long = old_scan->num_scans;
	}

	status = motor_get_long( output, "Enter number of times to scan -> ",
			TRUE, default_long,
			&scan_num_scans, 1, 1000000000 );

	if ( status != SUCCESS ) {
		return status;
	}

	/* Set the number of motors used by the scan. */

	switch( scan_type ) {
	case MXS_LST_FILE:
		if ( old_scan == NULL ) {
			default_long = 1;
		} else {
			default_long = old_scan->num_motors;
		}
		status = motor_get_long( output, "Enter number of motors -> ",
			TRUE, default_long,
			&scan_num_motors, 1, 1000000 );

		if ( status != SUCCESS ) {
			return status;
		}
		scan_num_independent_variables = scan_num_motors;
		break;
	default:
		fprintf( output, "%s: Unrecognized scan type %ld.\n",
			fname, scan_type );

		return FAILURE;
	}

	/* Add the record type info to the record description. */

	snprintf( record_description_buffer,
			record_description_buffer_length,
			"%s scan list_scan ", scan_name );

	switch( scan_type ) {
	case MXS_LST_FILE:
		strlcat( record_description_buffer, "file_list_scan \"\" \"\" ",
					record_description_buffer_length );
		break;
	default:
		fprintf( output, "Unknown list scan type = %ld\n",
			scan_type );
		break;
	}

	snprintf( buffer, sizeof(buffer),
			"%ld %ld %ld ", scan_num_scans,
			scan_num_independent_variables, scan_num_motors );

	strlcat( record_description_buffer, buffer,
					record_description_buffer_length );

	if ( scan_num_independent_variables <= 0 ) {
		fprintf( output,
	"%s: This scan has %ld independent variables.  That can't be right.\n",
		fname, scan_num_independent_variables );

		return FAILURE;
	}
	if ( scan_num_motors <= 0 ) {
		fprintf( output,
		"%s: This scan has %ld motors.  That can't be right.\n",
		fname, scan_num_motors );

		return FAILURE;
	}

	motor_is_independent_variable
			= (int *) malloc( scan_num_motors * sizeof(int));

	if ( motor_is_independent_variable == NULL ) {
		fprintf( output,
"%s: Out of memory allocating 'motor_is_independent_variable' array.\n",
			fname );
		return FAILURE;
	}

	switch( scan_type ) {
	default:
		for ( i = 0; i < scan_num_motors; i++ ) {
			motor_is_independent_variable[i] = TRUE;
		}
		break;
	}

	if ( old_scan == (MX_SCAN *) NULL ) {
		motor_record_array = NULL;
		old_scan_num_motors = 0;

		switch( scan_type ) {
		case MXS_LST_FILE:
			file_list_scan = NULL;
			break;
		default:
			fprintf( output, "%s: Unrecognized scan type %ld.\n",
				fname, scan_type );
			return FAILURE;
		}
	} else {
		motor_record_array = old_scan->motor_record_array;
		old_scan_num_motors = old_scan->num_motors;

		switch( scan_type ) {
		case MXS_LST_FILE:
			file_list_scan = (MX_FILE_LIST_SCAN *)
				(old_scan->record->record_type_struct);

			if ( file_list_scan == (MX_FILE_LIST_SCAN *) NULL ) {
				fprintf( output,
		"%s: MX_FILE_LIST_SCAN pointer in MX_SCAN is NULL.\n", fname );
				return FAILURE;
			}
			break;
		default:
			fprintf( output, "%s: Unrecognized scan type %ld.\n",
				fname, scan_type );
			return FAILURE;
		}
	}

	motor_name_dimension_array[0] = scan_num_motors;
	motor_name_dimension_array[1] = MXU_RECORD_NAME_LENGTH + 1;

	motor_name_array = (char **) mx_allocate_array(
		2, motor_name_dimension_array,
		name_element_size_array );

	if ( motor_name_array == (char **) NULL ) {
		fprintf( output,
	"%s: Unable to allocate memory for list of motor names.\n", fname);
			return FAILURE;
	}

	/* Prompt for the names of the motors and add them to
	 * the record description.
	 */

	status = motor_setup_scan_motor_names(
			record_description_buffer,
			record_description_buffer_length,
			old_scan_num_motors,
			scan_num_motors,
			motor_record_array,
			motor_name_array );

	(void) mx_free_array( motor_name_array,
		2, motor_name_dimension_array,
		name_element_size_array );

	if ( status != SUCCESS ) {
		return status;
	}

	/* We need to identify where the list of motor positions is to 
	 * be obtained from.  This will vary from scan type to scan type.
	 *
	 * This information is added to the record description after
	 * the common scan parameters.
	 */

	switch( scan_type ) {
	case MXS_LST_FILE:
		snprintf( prompt, sizeof(prompt),
			"Enter name of file containing motor positions -> " );

		string_length = sizeof(scan_position_filename) - 1;

		if ( file_list_scan == NULL ) {
			default_string_ptr = NULL;
		} else {
			default_string_ptr = file_list_scan->position_filename;
		}
		status = motor_get_string( output, prompt,
				default_string_ptr,
				&string_length, scan_position_filename );

		if ( status != SUCCESS ) {
			return status;
		}
		break;
	default:
		fprintf( output, "%s: Unrecognized scan type %ld.\n",
			fname, scan_type );

		return FAILURE;
	}

	/* Prompt for the common scan parameters such as input devices,
	 * data files and plot types.
	 */

	if ( input_devices_string != NULL ) {
		strlcat( record_description_buffer, input_devices_string,
					record_description_buffer_length );
	} else {
		status = motor_setup_input_devices( old_scan,
						scan_class, scan_type,
						buffer, sizeof(buffer), NULL );
		if ( status != SUCCESS )
			return status;

		strlcat( record_description_buffer, buffer,
					record_description_buffer_length );
	}

	/* Preserve the existing value of the 'scan_flags' field, if any. */

	if ( old_scan == NULL ) {
		scan_flags = 0;
	} else {
		scan_flags = old_scan->scan_flags;
	}

	snprintf( buffer, sizeof(buffer), "%#lx ", scan_flags );

	strlcat( record_description_buffer, buffer,
			record_description_buffer_length );

	/* Prompt for the measurement parameters. */

	if ( measurement_parameters_string != NULL ) {
		strlcat( record_description_buffer,
				measurement_parameters_string,
				record_description_buffer_length );
	} else {
		status = motor_setup_measurement_parameters( old_scan,
						buffer, sizeof(buffer), FALSE );
		if ( status != SUCCESS )
			return status;

		strlcat( record_description_buffer, buffer,
				record_description_buffer_length );
	}

	/* Prompt for the datafile and plot parameters. */

	if ( datafile_and_plot_parameters_string != NULL ) {
		strlcat( record_description_buffer,
				datafile_and_plot_parameters_string,
				record_description_buffer_length );
	} else {
		status = motor_setup_datafile_and_plot_parameters( old_scan,
						scan_class, scan_type,
						buffer, sizeof(buffer) );
		if ( status != SUCCESS )
			return status;

		strlcat( record_description_buffer, buffer,
				record_description_buffer_length );
	}

	/* Now add the source of the position list to the record
	 * description.
	 */

	switch( scan_type ) {
	case MXS_LST_FILE:
		strlcat( record_description_buffer,
				scan_position_filename,
				record_description_buffer_length );

		break;
	default:
		fprintf( output, "%s: Unrecognized scan type %ld.\n",
			fname, scan_type );

		return FAILURE;
	}

	/* Delete the old scan if it exists. */

	if ( old_scan != (MX_SCAN *) NULL ) {

		mx_status = mx_delete_record( old_scan->record );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;
	}

	/* Add the scan to the record list. */

	mx_status = mx_create_record_from_description( motor_record_list,
		record_description_buffer, &record, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	mx_status = mx_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	return SUCCESS;
}

