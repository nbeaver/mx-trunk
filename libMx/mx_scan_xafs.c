/*
 * Name:    mx_scan_xafs.c
 *
 * Purpose: Driver for MX_XAFS_SCAN scan records.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_variable.h"
#include "mx_driver.h"
#include "mx_scan.h"
#include "mx_scan_xafs.h"
#include "d_xafs_wavenumber.h"

MX_RECORD_FUNCTION_LIST mxs_xafs_scan_record_function_list = {
	mxs_xafs_scan_initialize_type,
	mxs_xafs_scan_create_record_structures,
	mxs_xafs_scan_finish_record_initialization,
	mxs_xafs_scan_delete_record,
	mxs_xafs_scan_print_scan_structure
};

MX_SCAN_FUNCTION_LIST mxs_xafs_scan_scan_function_list = {
	mxs_xafs_scan_prepare_for_scan_start,
	mxs_xafs_scan_execute_scan_body,
	mxs_xafs_scan_cleanup_after_scan_end
};

MX_EXPORT mx_status_type
mxs_xafs_scan_initialize_type( long record_type )
{
	static const char fname[] = "mxs_xafs_scan_initialize_type()";

	MX_DRIVER *driver;
	MX_RECORD_FIELD_DEFAULTS *record_field_defaults;
	MX_RECORD_FIELD_DEFAULTS **record_field_defaults_ptr;
	MX_RECORD_FIELD_DEFAULTS *field;
	mx_length_type num_record_fields;
	mx_length_type num_independent_variables_varargs_cookie;
	mx_length_type num_motors_varargs_cookie;
	mx_length_type num_input_devices_varargs_cookie;

	mx_length_type num_boundaries_field_index;
	mx_length_type num_boundaries_varargs_cookie;
	mx_length_type num_regions_field_index;
	mx_length_type num_regions_varargs_cookie;
	mx_status_type status;

	driver = mx_get_driver_by_type( record_type );

	if ( driver == (MX_DRIVER *) NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Record type %ld not found.",
			record_type );
	}

	record_field_defaults_ptr = driver->record_field_defaults_ptr;

	if (record_field_defaults_ptr == (MX_RECORD_FIELD_DEFAULTS **) NULL) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"'record_field_defaults_ptr' for record type '%s' is NULL.",
			driver->name );
	}

	record_field_defaults = *record_field_defaults_ptr;

	if ( record_field_defaults == (MX_RECORD_FIELD_DEFAULTS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"'record_field_defaults_ptr' for record type '%s' is NULL.",
			driver->name );
	}

	if ( driver->num_record_fields == (long *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"'num_record_fields' pointer for record type '%s' is NULL.",
			driver->name );
	}

	/**** Fix up the record fields common to all scan types. ****/

	num_record_fields = *(driver->num_record_fields);

	status = mx_scan_fixup_varargs_record_field_defaults(
			record_field_defaults, num_record_fields,
			&num_independent_variables_varargs_cookie,
			&num_motors_varargs_cookie,
			&num_input_devices_varargs_cookie );

	if ( status.code != MXE_SUCCESS )
		return status;

	/** Fix up the record fields specific to MX_XAFS_SCAN records **/

	status = mx_find_record_field_defaults_index(
			record_field_defaults, num_record_fields,
			"num_boundaries", &num_boundaries_field_index );

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mx_construct_varargs_cookie(
		num_boundaries_field_index, 0, &num_boundaries_varargs_cookie);

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mx_find_record_field_defaults(
			record_field_defaults, num_record_fields,
			"region_boundary", &field );

	if ( status.code != MXE_SUCCESS )
		return status;

	field->dimension[0] = num_boundaries_varargs_cookie;

	/*--*/

	status = mx_find_record_field_defaults_index(
			record_field_defaults, num_record_fields,
			"num_regions", &num_regions_field_index );

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mx_construct_varargs_cookie(
		num_regions_field_index, 0, &num_regions_varargs_cookie);

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mx_find_record_field_defaults(
			record_field_defaults, num_record_fields,
			"region_step_size", &field );

	if ( status.code != MXE_SUCCESS )
		return status;

	field->dimension[0] = num_regions_varargs_cookie;

	status = mx_find_record_field_defaults(
			record_field_defaults, num_record_fields,
			"region_measurement_time", &field );

	if ( status.code != MXE_SUCCESS )
		return status;

	field->dimension[0] = num_regions_varargs_cookie;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_xafs_scan_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxs_xafs_scan_create_record_structures()";

	MX_SCAN *scan;
	MX_XAFS_SCAN *xafs_scan;

	/* Allocate memory for the necessary structures. */

	scan = (MX_SCAN *) malloc( sizeof(MX_SCAN) );

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SCAN structure." );
	}

	xafs_scan = (MX_XAFS_SCAN *) malloc( sizeof(MX_XAFS_SCAN) );

	if ( xafs_scan == (MX_XAFS_SCAN *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_XAFS_SCAN structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_superclass_struct = scan;
	record->record_class_struct = xafs_scan;
	record->record_type_struct = NULL;	/* Not used by this class. */

	scan->record = record;

	scan->measurement.scan = scan;
	scan->datafile.scan = scan;
	scan->plot.scan = scan;

	scan->datafile.x_motor_array = NULL;
	scan->plot.x_motor_array = NULL;

	record->superclass_specific_function_list
				= &mxs_xafs_scan_scan_function_list;

	scan->num_missing_records = 0;
	scan->missing_record_array = NULL;

	/* Find the MX_XAFS_SCAN_FUNCTION_LIST pointer. */

	record->class_specific_function_list = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_xafs_scan_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] = "mxs_xafs_scan_finish_record_initialization()";

	MX_XAFS_SCAN *xafs_scan;
	mx_status_type status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL." );
	}

	xafs_scan = (MX_XAFS_SCAN *)(record->record_class_struct);

	if ( xafs_scan == (MX_XAFS_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_XAFS_SCAN pointer for record '%s' is NULL.",
			record->name );
	}

	status = mx_scan_finish_record_initialization( record );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Perform a consistency check on some of the XAFS scan parameters. */

	if ( xafs_scan->num_regions !=
		(xafs_scan->num_energy_regions+xafs_scan->num_k_regions) ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"Number of regions does not equal the sum of the number of energy regions "
"and k regions.  num_regions = %ld, num_energy_regions = %ld, "
"num_k_regions = %ld",
			xafs_scan->num_regions,
			xafs_scan->num_energy_regions,
			xafs_scan->num_k_regions );
	}
	if ( xafs_scan->num_boundaries != (1 + xafs_scan->num_regions) ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"Number of region boundaries does not equal the number of regions plus 1.  "
"num_boundaries = %ld, num_regions = %ld",
			xafs_scan->num_boundaries,
			xafs_scan->num_regions );
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_xafs_scan_delete_record( MX_RECORD *record )
{
	static const char fname[] = "mxs_xafs_scan_delete_record()";

	MX_SCAN *scan;
	MX_XAFS_SCAN *xafs_scan;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( record == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed was NULL." );
	}

	scan = (MX_SCAN *)(record->record_superclass_struct);

	if ( scan != NULL ) {
		if ( scan->missing_record_array != NULL ) {
			int i;

			for ( i = 0; i < scan->num_missing_records; i++ ) {
				mx_delete_record(
				    scan->missing_record_array[i] );
			}

			mx_free( scan->missing_record_array );
		}

		xafs_scan = (MX_XAFS_SCAN *) scan->record->record_class_struct;

		if ( xafs_scan != NULL ) {

			record->record_type_struct = NULL;

			mx_free( xafs_scan );

			record->record_class_struct = NULL;
		}

		if ( scan->datafile.x_motor_array != NULL ) {
			mx_free( scan->datafile.x_motor_array );
		}

		if ( scan->plot.x_motor_array != NULL ) {
			mx_free( scan->plot.x_motor_array );
		}

		mx_free( scan );

		scan = NULL;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_xafs_scan_print_scan_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxs_xafs_scan_print_scan_structure()";

	MX_SCAN *scan;
	MX_XAFS_SCAN *xafs_scan;
	long i, j;
	double start, start_squared, last_energy_boundary;
	mx_status_type status;

	if ( record != NULL ) {
		fprintf( file, "SCAN parameters for xafs scan '%s':\n",
				record->name );
	}

	status = mx_scan_print_scan_structure( file, record );

	if ( status.code != MXE_SUCCESS )
		return status;

	scan = (MX_SCAN *) record->record_superclass_struct;

	xafs_scan = (MX_XAFS_SCAN *) record->record_class_struct;

	if ( xafs_scan == (MX_XAFS_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_XAFS_SCAN pointer for scan '%s' is NULL.",
			record->name );
	}

	fprintf( file, "  %ld energy regions,  %ld k regions\n\n",
			xafs_scan->num_energy_regions,
			xafs_scan->num_k_regions );

	/* Display the energy regions. */

	for ( i = 0; i < xafs_scan->num_energy_regions; i++ ) {
		fprintf( file,
"  Region %ld:  %8g eV  to %8g eV,  %8g eV  step size, %g sec\n",
			i, xafs_scan->region_boundary[i],
			xafs_scan->region_boundary[i+1],
			xafs_scan->region_step_size[i],
			xafs_scan->region_measurement_time[i] );
	}

	/* Display the k regions. */

	if ( xafs_scan->num_energy_regions <= 0 ) {
		start = 0.0;
	} else {
		last_energy_boundary =
		    xafs_scan->region_boundary[ xafs_scan->num_energy_regions ];

		start_squared = last_energy_boundary
					/ MX_HBAR_SQUARED_OVER_2M_ELECTRON;

		if ( start_squared <= 0.0 ) {
			start = 0.0;
		} else {
			start = sqrt( start_squared );
		}
	}

	for ( i = 0; i < xafs_scan->num_k_regions; i++ ) {
		j = i + xafs_scan->num_energy_regions;

		fprintf( file,
"  Region %ld:  %8g A-1 to %8g A-1, %8g A-1 step size, %g sec\n",
			j, start, xafs_scan->region_boundary[j+1],
			xafs_scan->region_step_size[j],
			xafs_scan->region_measurement_time[j] );

		start = xafs_scan->region_boundary[j+1];
	}

	fprintf( file, "\n" );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_xafs_scan_prepare_for_scan_start( MX_SCAN *scan )
{
	static const char fname[] = "mxs_xafs_scan_prepare_for_scan_start()";

	MX_RECORD *e_minus_e0_record;
	MX_RECORD *k_record;
	mx_status_type status;

	/* Some motor drivers use cached values of variables when 
	 * scan 'fast mode' is turned on rather than querying a
	 * remote server for the values on each step.  For example,
	 * the 'energy' pseudomotor does not reread the value of
	 * 'd_spacing' on each step when 'fast mode' is turned on.
	 *
	 * We can make sure that the cached values are up to date
	 * by reading the position of 'e_minus_e0' and 'k'
	 * before 'fast mode' is turned on.
	 */

	e_minus_e0_record = mx_get_record( scan->record, "e_minus_e0" );

	k_record = mx_get_record( scan->record, "k" );

	if ( ( e_minus_e0_record == (MX_RECORD *) NULL )
	  || ( k_record == (MX_RECORD *) NULL ) )
	{
		return mx_error( MXE_SOFTWARE_CONFIGURATION_ERROR, fname,
	"The attempt to run XAFS scan '%s' failed since the pseudomotors "
	"'e_minus_e0' and 'k' are not both defined in the MX database.  "
	"'e_minus_e0' should be the X-ray energy minus the edge energy "
	"in eV, while 'k' should be the electron wavenumber in angstroms.",
			scan->record->name );
	}

	/* Read the positions. */

	status = mx_motor_get_position( e_minus_e0_record, NULL );

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mx_motor_get_position( k_record, NULL );

	if ( status.code != MXE_SUCCESS )
		return status;

	/* Perform standard scan startup operations.
	 *
	 * Among other things, this turns on 'fast mode'.
	 */

	status = mx_standard_prepare_for_scan_start( scan );

	return status;
}

MX_EXPORT mx_status_type
mxs_xafs_scan_execute_scan_body( MX_SCAN *scan )
{
	static const char fname[] = "mxs_xafs_scan_execute_scan_body()";

	MX_XAFS_SCAN *xafs_scan;
	MX_RECORD *scan_region_record, *record_list;
	char record_description[ MXU_RECORD_DESCRIPTION_LENGTH + 1 ];
	char timer_name[ MXU_RECORD_NAME_LENGTH + 1 ];
	char format_buffer[20];
	double start_position, end_position, step_size, end_of_region;
	double e_minus_e0_start, k_start_squared;
	double energy_diff, nominal_measurement_time;
	char *ptr;
	size_t string_length, buffer_left;
	long i, j, num_steps;
	int num_items;
	mx_status_type status;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_SCAN pointer passed was NULL." );
	}

	xafs_scan = (MX_XAFS_SCAN *)(scan->record->record_class_struct);

	if ( xafs_scan == (MX_XAFS_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_XAFS_SCAN pointer for scan record '%s' is NULL.",
			scan->record->name );
	}

	record_list = scan->record->list_head;

	for ( i = 0; i < xafs_scan->num_regions; i++ ) {

		sprintf( record_description,
	"mx_xafstmp%ld scan linear_scan motor_scan \"\" \"\" 1 1 1 ", i );

		string_length = strlen( record_description );
		buffer_left = sizeof( record_description ) - string_length - 1;
		ptr = record_description + string_length;

		if ( i < xafs_scan->num_energy_regions ) {
			strncat( ptr, "e_minus_e0 ", buffer_left );
		} else {
			strncat( ptr, "k ", buffer_left );
		}

		string_length = strlen( record_description );
		ptr = record_description + string_length;

		sprintf( ptr, "%ld ", scan->num_input_devices );

		for ( j = 0; j < scan->num_input_devices; j++ ) {

			string_length = strlen( record_description );
			ptr = record_description + string_length;

			sprintf( ptr, "%s ",
					scan->input_device_array[j]->name );
		}

		end_of_region = xafs_scan->region_boundary[i+1];
		step_size = xafs_scan->region_step_size[i];

		if ( i != xafs_scan->num_energy_regions ) {
			start_position = xafs_scan->region_boundary[i];
		} else {
			e_minus_e0_start = xafs_scan->region_boundary[i];

			k_start_squared = e_minus_e0_start
					/ MX_HBAR_SQUARED_OVER_2M_ELECTRON;

			if ( k_start_squared < 0.0 ) {
				start_position = 0.0;
			} else {
				start_position = sqrt( k_start_squared );
			}
		}

		num_steps = 1L + (long) ( (end_of_region - start_position)
							/ step_size );

		/* Suppress duplicate points at the end of XAFS scan regions.*/

		end_position = start_position
				+ step_size * (double) ( num_steps - 1L );

		if ( (xafs_scan->num_regions - 1) > i ) {
			if ( i < xafs_scan->num_energy_regions ) {
				energy_diff = end_of_region - end_position;
			} else {
				energy_diff = MX_HBAR_SQUARED_OVER_2M_ELECTRON
				  * ( end_of_region * end_of_region
					- end_position * end_position );
			}
			if ( fabs( energy_diff ) < 0.01 ) {
				num_steps--;
			}
		}

		/* Get the timer name. */

		sprintf( format_buffer, "%%lg %%%ds", MXU_RECORD_NAME_LENGTH );

		num_items = sscanf( scan->measurement_arguments, format_buffer,
					&nominal_measurement_time, timer_name );

		if ( num_items != 2 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
			"The timer name cannot be found in the "
			"measurement_arguments field '%s' for scan '%s'.",
			scan->measurement_arguments,
			scan->record->name );
		}

		/*--*/

		string_length = strlen( record_description );
		ptr = record_description + string_length;

#if 0
		sprintf( ptr,
    "0 %.*g preset_time \"%.*g %s\" child %s child \"%s\" %.*g %.*g %ld",
			scan->record->precision,
			scan->settling_time,
			scan->record->precision,
			xafs_scan->region_measurement_time[i],
			timer_name,
			scan->record->name,
			scan->record->name,
			scan->record->precision,
			start_position,
			scan->record->precision,
			step_size,
			num_steps );
#else
		sprintf( ptr,
    "0 %.*g preset_time \"%.*g %s\" child:%s %s child:%s \"%s\" %.*g %.*g %ld",
			scan->record->precision,
			scan->settling_time,
			scan->record->precision,
			xafs_scan->region_measurement_time[i],
			timer_name,
			scan->datafile.options,
			scan->record->name,
			scan->plot.options,
			scan->record->name,
			scan->record->precision,
			start_position,
			scan->record->precision,
			step_size,
			num_steps );
#endif

		/* Now create the scan record from the description that we
		 * have just constructed.
		 */

		MX_DEBUG( 2,("%s: description = '%s'",
					fname, record_description));

		status = mx_create_record_from_description( record_list,
				record_description, &scan_region_record, 0 );

		if ( status.code != MXE_SUCCESS )
			return status;

		status = mx_finish_record_initialization( scan_region_record );

		if ( status.code != MXE_SUCCESS )
			return status;

		/* Run the scan. */

		status = mx_perform_scan( scan_region_record );

		/* Make sure the temporary scan region record is deleted. */

		(void) mx_delete_record( scan_region_record );

		if ( status.code != MXE_SUCCESS )
			return status;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_xafs_scan_cleanup_after_scan_end( MX_SCAN *scan )
{
	return mx_standard_cleanup_after_scan_end( scan );
}

