/*
 * Name:    mx_scan_area_detector.c
 *
 * Purpose: Driver for MX_AREA_DETECTOR_SCAN records.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_AREA_DETECTOR_DEBUG_SCAN	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_array.h"
#include "mx_scan.h"
#include "mx_scan_area_detector.h"

#include "mx_poison.h"

MX_RECORD_FUNCTION_LIST mxs_area_detector_scan_record_function_list = {
	mxs_area_detector_scan_initialize_type,
	mxs_area_detector_scan_create_record_structures,
	mxs_area_detector_scan_finish_record_initialization,
};

MX_SCAN_FUNCTION_LIST mxs_area_detector_scan_scan_function_list = {
	mxs_area_detector_scan_prepare_for_scan_start,
	mxs_area_detector_scan_execute_scan_body,
	mxs_area_detector_scan_cleanup_after_scan_end
};

MX_EXPORT mx_status_type
mxs_area_detector_scan_initialize_type( long record_type )
{
	static const char fname[] = "mxs_area_detector_scan_initialize_type()";

	MX_DRIVER *driver;
	MX_RECORD_FIELD_DEFAULTS *record_field_defaults;
	MX_RECORD_FIELD_DEFAULTS **record_field_defaults_ptr;
	long num_record_fields;
	long num_independent_variables_varargs_cookie;
	long num_motors_varargs_cookie;
	long num_input_devices_varargs_cookie;
	mx_status_type mx_status;

#if MX_AREA_DETECTOR_DEBUG_SCAN
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	driver = mx_get_driver_by_type( record_type );

	if ( driver == (MX_DRIVER *) NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Record type %ld not found.",
			record_type );
	}

	record_field_defaults_ptr
			= driver->record_field_defaults_ptr;

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

	mx_status = mx_scan_fixup_varargs_record_field_defaults(
			record_field_defaults, num_record_fields,
			&num_independent_variables_varargs_cookie,
			&num_motors_varargs_cookie,
			&num_input_devices_varargs_cookie );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* FIXME: Varying length fields should be initialized here. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_area_detector_scan_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxs_area_detector_scan_create_record_structures()";

	MX_SCAN *scan;
	MX_AREA_DETECTOR_SCAN *area_detector_scan;
	MX_AREA_DETECTOR_SCAN_FUNCTION_LIST *flist;
	MX_DRIVER *driver;
	mx_status_type (*fptr)( MX_RECORD *, MX_SCAN *,
				MX_AREA_DETECTOR_SCAN * );
	mx_status_type mx_status;

#if MX_AREA_DETECTOR_DEBUG_SCAN
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name));
#endif

	/* Allocate memory for the necessary structures. */

	scan = (MX_SCAN *) malloc( sizeof(MX_SCAN) );

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_SCAN structure." );
	}

	area_detector_scan = (MX_AREA_DETECTOR_SCAN *)
				malloc( sizeof(MX_AREA_DETECTOR_SCAN) );

	if ( area_detector_scan == (MX_AREA_DETECTOR_SCAN *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_AREA_DETECTOR_SCAN structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_superclass_struct = scan;
	record->record_class_struct = area_detector_scan;
	record->record_type_struct = NULL;

	scan->record = record;

	scan->measurement.scan = scan;
	scan->datafile.scan = scan;
	scan->plot.scan = scan;

	scan->datafile.x_motor_array = NULL;
	scan->plot.x_motor_array = NULL;

	record->superclass_specific_function_list
				= &mxs_area_detector_scan_scan_function_list;

	scan->num_missing_records = 0;
	scan->missing_record_array = NULL;

	/* Find the MX_AREA_DETECTOR_SCAN_FUNCTION_LIST pointer. */

	driver = mx_get_driver_by_type( record->mx_type );

	if ( driver == (MX_DRIVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_DRIVER for record type %ld is NULL.",
			record->mx_type );
	}

	flist = (MX_AREA_DETECTOR_SCAN_FUNCTION_LIST *)
			driver->class_specific_function_list;

	if ( flist == (MX_AREA_DETECTOR_SCAN_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	    "MX_AREA_DETECTOR_SCAN_FUNCTION_LIST for record type %ld is NULL.",
			record->mx_type );
	}

	record->class_specific_function_list = flist;

	/* Call the type specific 'create_record_structures' function. */

	fptr = flist->create_record_structures;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Area detector scan type specific 'create_record_structures' "
		"pointer is NULL.");
	}

	mx_status = (*fptr)( record, scan, area_detector_scan );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_area_detector_scan_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxs_area_detector_scan_finish_record_initialization()";

	MX_SCAN *scan;
	MX_AREA_DETECTOR_SCAN_FUNCTION_LIST *flist;
	mx_status_type (*fptr)( MX_RECORD * );
	long dimension[2];
	size_t element_size[2];
	mx_status_type mx_status;

#if MX_AREA_DETECTOR_DEBUG_SCAN
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name));
#endif

	mx_status = mx_scan_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	scan = (MX_SCAN *) record->record_superclass_struct;

	/* If needed allocate memory for the alternate motor position arrays. */

	if ( scan->datafile.num_x_motors > 0 ) {
		dimension[0] = scan->datafile.num_x_motors;
		dimension[1] = 1;

		element_size[0] = sizeof(double);
		element_size[1] = sizeof(double *);

		scan->datafile.x_position_array = (double **)
			mx_allocate_array( 2, dimension, element_size );

		if ( scan->datafile.x_position_array == (double **) NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %ld by %ld array of "
		"alternate position values for scan '%s'.",
				dimension[0], dimension[1], scan->record->name);
		}
	}

	if ( scan->plot.num_x_motors > 0 ) {
		dimension[0] = scan->plot.num_x_motors;
		dimension[1] = 1;

		element_size[0] = sizeof(double);
		element_size[1] = sizeof(double *);

		scan->plot.x_position_array = (double **)
			mx_allocate_array( 2, dimension, element_size );

		if ( scan->plot.x_position_array == (double **) NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %ld by %ld array of "
		"alternate position values for scan '%s'.",
				dimension[0], dimension[1], scan->record->name);
		}
	}

	/* Call the record type specific 'finish_record_initialization'
	 * function.
	 */

	flist = (MX_AREA_DETECTOR_SCAN_FUNCTION_LIST *)
				(record->class_specific_function_list);

	if ( flist == (MX_AREA_DETECTOR_SCAN_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_AREA_DETECTOR_SCAN_FUNCTION_LIST pointer is NULL.");
	}

	fptr = flist->finish_record_initialization;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Area detector scan type specific "
		"'finish_record_initialization' pointer is NULL.");
	}

	mx_status = (*fptr)( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxs_area_detector_scan_prepare_for_scan_start( MX_SCAN *scan )
{
	static const char fname[] =
		"mxs_area_detector_scan_prepare_for_scan_start()";

	MX_AREA_DETECTOR_SCAN_FUNCTION_LIST *flist;
	mx_status_type (*fptr) (MX_SCAN *);
	int i;
	mx_status_type mx_status;

#if MX_AREA_DETECTOR_DEBUG_SCAN
	MX_DEBUG(-2,("%s invoked for scan '%s'.", fname, scan->record->name));
#endif

	flist = (MX_AREA_DETECTOR_SCAN_FUNCTION_LIST *)
				(scan->record->class_specific_function_list);

	if ( flist == (MX_AREA_DETECTOR_SCAN_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_AREA_DETECTOR_SCAN_FUNCTION_LIST pointer for scan '%s' is NULL.",
			scan->record->name );
	}
	
	/* Some motor drivers use cached values of variables when 
	 * scan 'fast mode' is turned on rather than querying a
	 * remote server for the values on each step.  For example,
	 * the 'energy' pseudomotor does not reread the value of
	 * 'd_spacing' on each step when 'fast mode' is turned on.
	 *
	 * We can make sure that the cached values are up to date
	 * by reading the position of each motor used by the scan
	 * before 'fast mode' is turned on.
	 */

	for ( i = 0; i < scan->num_motors; i++ ) {
		mx_status = mx_motor_get_position( scan->motor_record_array[i],
						NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Perform standard scan startup operations.
	 *
	 * Among other things, this turns on 'fast mode'.
	 */

	mx_status = mx_standard_prepare_for_scan_start( scan );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Clear all the input devices that need it. */

	mx_status = mx_clear_scan_input_devices( scan );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Perform scan type specific initialization. */

	fptr = flist->prepare_for_scan_start;

	/* It is not an error for flist->prepare_for_scan_start to be NULL,
	 * this just means that this particular scan type does not need
	 * to do anything special when a scan is starting.
	 */

	if ( fptr != NULL ) {
		mx_status = (*fptr) (scan);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_area_detector_scan_execute_scan_body( MX_SCAN *scan )
{
	static const char fname[] =
		"mxs_area_detector_scan_execute_scan_body()";

#if MX_AREA_DETECTOR_DEBUG_SCAN
	MX_DEBUG(-2,("%s invoked for scan '%s'.", fname, scan->record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_area_detector_scan_cleanup_after_scan_end( MX_SCAN *scan )
{
	return mx_standard_cleanup_after_scan_end( scan );
}

