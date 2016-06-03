/*
 * Name:    mx_scan_area_detector.c
 *
 * Purpose: Driver for MX_AREA_DETECTOR_SCAN records.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2008, 2010, 2013, 2015-2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MX_AREA_DETECTOR_DEBUG_SCAN	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_array.h"
#include "mx_image.h"
#include "mx_area_detector.h"
#include "mx_scan.h"
#include "mx_scan_area_detector.h"

MX_RECORD_FUNCTION_LIST mxs_area_detector_scan_record_function_list = {
	mxs_area_detector_scan_initialize_driver,
	mxs_area_detector_scan_create_record_structures,
	mxs_area_detector_scan_finish_record_initialization,
};

MX_SCAN_FUNCTION_LIST mxs_area_detector_scan_scan_function_list = {
	mxs_area_detector_scan_prepare_for_scan_start,
	mxs_area_detector_scan_execute_scan_body,
	mxs_area_detector_scan_cleanup_after_scan_end
};

MX_EXPORT mx_status_type
mxs_area_detector_scan_initialize_driver( MX_DRIVER *driver )
{
	static const char fname[] =
		"mxs_area_detector_scan_initialize_driver()";

	MX_RECORD_FIELD_DEFAULTS *field;
	long i;
	long referenced_field_index;
	long num_independent_variables_varargs_cookie;
	long num_motors_varargs_cookie;
	long num_input_devices_varargs_cookie;
	long num_energies_varargs_cookie;
	long num_static_motors_varargs_cookie;
	mx_status_type mx_status;

	static const char static_motor_field[][MXU_FIELD_NAME_LENGTH+1] = {
		"static_motor_array",
		"static_motor_positions"
	};

	long num_static_motor_fields = sizeof(static_motor_field)
					/ sizeof(static_motor_field[0]);

	static const char scan_position_field[][MXU_FIELD_NAME_LENGTH+1] = {
		"start_position",
		"step_size",
		"num_frames"
	};

	long num_scan_position_fields = sizeof(scan_position_field)
					/ sizeof(scan_position_field[0]);

#if 0 && MX_AREA_DETECTOR_DEBUG_SCAN
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	if ( driver == (MX_DRIVER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_DRIVER pointer passed was NULL." );
	}

	/**** Fix up the record fields common to all scan types. ****/

	mx_status = mx_scan_fixup_varargs_record_field_defaults( driver,
				&num_independent_variables_varargs_cookie,
				&num_motors_varargs_cookie,
				&num_input_devices_varargs_cookie );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Fixup the energy array field. */

	mx_status = mx_find_record_field_defaults_index( driver,
						"num_energies",
						&referenced_field_index );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_construct_varargs_cookie(
		referenced_field_index, 0, &num_energies_varargs_cookie );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_find_record_field_defaults( driver,
						"energy_array", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = num_energies_varargs_cookie;

	/* Fixup the static motor fields. */

	mx_status = mx_find_record_field_defaults_index( driver,
						"num_static_motors",
						&referenced_field_index );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_construct_varargs_cookie(
		referenced_field_index, 0, &num_static_motors_varargs_cookie );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	for ( i = 0; i < num_static_motor_fields; i++ ) {
		mx_status = mx_find_record_field_defaults( driver,
					static_motor_field[i], &field );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		field->dimension[0] = num_static_motors_varargs_cookie;
	}

	/* Fixup the scan position fields. */

	for ( i = 0; i < num_scan_position_fields; i++ ) {
		mx_status = mx_find_record_field_defaults( driver,
					scan_position_field[i], &field );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		field->dimension[0] = num_motors_varargs_cookie;
	}

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
				scan->record->class_specific_function_list;

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

#if MX_AREA_DETECTOR_DEBUG_SCAN
	MX_DEBUG(-2,("%s: flist->prepare_for_scan_start = %p", fname, fptr));
#endif

	/* It is not an error for flist->prepare_for_scan_start to be NULL,
	 * this just means that this particular scan type does not need
	 * to do anything special when a scan is starting.
	 */

	if ( fptr != NULL ) {
		mx_status = (*fptr)( scan );

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

	MX_AREA_DETECTOR_SCAN *ad_scan;
	MX_AREA_DETECTOR_SCAN_FUNCTION_LIST *flist;
	mx_status_type (*fptr) (MX_SCAN *);
	mx_status_type mx_status;

#if MX_AREA_DETECTOR_DEBUG_SCAN
	MX_DEBUG(-2,("%s invoked for scan '%s'.", fname, scan->record->name));
#endif

	ad_scan = (MX_AREA_DETECTOR_SCAN *) scan->record->record_class_struct;

	if ( ad_scan == (MX_AREA_DETECTOR_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_AREA_DETECTOR_SCAN pointer for scan '%s' is NULL.",
			scan->record->name );
	}

	flist = (MX_AREA_DETECTOR_SCAN_FUNCTION_LIST *)
				scan->record->class_specific_function_list;

	if ( flist == (MX_AREA_DETECTOR_SCAN_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_AREA_DETECTOR_SCAN_FUNCTION_LIST pointer for scan '%s' is NULL.",
			scan->record->name );
	}

	fptr = flist->execute_scan_body;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The execute_scan_body function for scan '%s' is NULL.",
			scan->record->name );
	}

	/* Move all of the motors to the start position. */

#if MX_AREA_DETECTOR_DEBUG_SCAN
	MX_DEBUG(-2,("%s: moving motors to the start positions.", fname));
#endif

	mx_status = mx_motor_array_move_absolute( scan->num_motors,
						scan->motor_record_array,
						ad_scan->start_position, 0 );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_move_absolute( ad_scan->energy_record,
						ad_scan->energy_array[0], 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_array_move_absolute( ad_scan->num_static_motors,
						ad_scan->static_motor_array,
						ad_scan->static_motor_positions,
						0 );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait for the motors to get to their destinations. */

#if MX_AREA_DETECTOR_DEBUG_SCAN
	MX_DEBUG(-2,
	("%s: waiting for motors to get to the start positions", fname));
#endif

	mx_status = mx_wait_for_motor_array_stop( scan->num_motors,
						scan->motor_record_array, 0 );
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_wait_for_motor_stop( ad_scan->energy_record, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_wait_for_motor_array_stop( ad_scan->num_static_motors,
						ad_scan->static_motor_array, 0);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Now execute the body of the scan. */

#if MX_AREA_DETECTOR_DEBUG_SCAN
	MX_DEBUG(-2,("%s: ready to execute the scan body.", fname));
#endif

	mx_status = (*fptr)( scan );
	
	return mx_status;
}

MX_EXPORT mx_status_type
mxs_area_detector_scan_cleanup_after_scan_end( MX_SCAN *scan )
{
	static const char fname[] =
		"mxs_area_detector_scan_cleanup_after_scan_end()";

	MX_AREA_DETECTOR_SCAN_FUNCTION_LIST *flist;
	mx_status_type (*fptr) (MX_SCAN *);
	mx_status_type mx_status;

#if MX_AREA_DETECTOR_DEBUG_SCAN
	MX_DEBUG(-2,("%s invoked for scan '%s'.", fname, scan->record->name));
#endif

	flist = (MX_AREA_DETECTOR_SCAN_FUNCTION_LIST *)
				scan->record->class_specific_function_list;

	if ( flist == (MX_AREA_DETECTOR_SCAN_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_AREA_DETECTOR_SCAN_FUNCTION_LIST pointer for scan '%s' is NULL.",
			scan->record->name );
	}

	fptr = flist->cleanup_after_scan_end;

	if ( fptr != NULL ) {
		mx_status = (*fptr)( scan );
	}
	
	mx_status =  mx_standard_cleanup_after_scan_end( scan );

	return mx_status;
}

/*------*/

MX_EXPORT mx_status_type
mxs_area_detector_scan_default_initialize_datafile_naming( MX_SCAN * scan )
{
	static const char fname[] =
		"mxs_area_detector_scan_default_initialize_datafile_naming()";

	MX_RECORD *scan_record, *ad_record;
	MX_AREA_DETECTOR *ad;
	char *scan_datafile_name;
	char *ptr, *datafile_name_ptr;
	mx_status_type mx_status;

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SCAN pointer passed was NULL." );
	}

	scan_record = scan->record;

	if ( scan_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for MX_SCAN %p is NULL.", scan );
	}

	MX_DEBUG(-2,("%s invoked for scan '%s'.", fname, scan_record->name ));

	ad_record = scan->input_device_array[0];

	if ( ad_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The first input device record for scan '%s' is NULL.",
			scan_record->name );
	}
	if ( ad_record->mx_class != MXC_AREA_DETECTOR ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Input device '%s' for scan '%s' is not an area detector.",
			ad_record->name, scan_record->name );
	}

	ad = ad_record->record_class_struct;

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_AREA_DETECTOR pointer for record '%s' is NULL.",
			ad_record->name );
	}

	/* The area detector image filenames are derived from the scan
	 * datafile name.  We must split this into the datafile directory
	 * part and the datafile name part.
	 */

	scan_datafile_name = strdup( scan->datafile_name );

	if ( scan_datafile_name == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a copy of the "
		"scan datafile name for scan '%s'.", scan_record->name );
	}

	ptr = strrchr( scan_datafile_name, '/' );

	if ( ptr == NULL ) {
		/* Use the current directory as the directory name. */

		mx_get_current_directory_name( ad->datafile_directory,
					sizeof(ad->datafile_directory) );

		datafile_name_ptr = scan_datafile_name;
	} else {
		/* The datafile name starts after the directory separator. */

		datafile_name_ptr = ptr + 1;

		/* Null terminate the directory name and then copy it. */

		*ptr = '\0';

		strlcpy( ad->datafile_directory, scan_datafile_name,
				sizeof(ad->datafile_directory) );
	}

	/* Now generate a datafile name pattern using the filename pointed
	 * to by datafile_name_ptr.
	 */

	snprintf( ad->datafile_pattern, sizeof(ad->datafile_pattern),
		"%s####.smv", datafile_name_ptr );

	mx_free( scan_datafile_name );

	MX_DEBUG(-2,("%s: datafile_directory = '%s', datafile_pattern = '%s'",
		fname, ad->datafile_directory, ad->datafile_pattern));

	mx_status = mx_area_detector_initialize_datafile_number( ad_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxs_area_detector_scan_default_construct_next_datafile_name( MX_SCAN * scan )
{
	static const char fname[] =
		"mxs_area_detector_scan_default_construct_next_datafile_name()";

	MX_RECORD *scan_record, *ad_record;
	MX_AREA_DETECTOR *ad;
	mx_status_type mx_status;

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SCAN pointer passed was NULL." );
	}

	scan_record = scan->record;

	if ( scan_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for MX_SCAN %p is NULL.", scan );
	}

	MX_DEBUG(-2,("%s invoked for scan '%s'.", fname, scan_record->name ));

	ad_record = scan->input_device_array[0];

	if ( ad_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The first input device record for scan '%s' is NULL.",
			scan_record->name );
	}
	if ( ad_record->mx_class != MXC_AREA_DETECTOR ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Input device '%s' for scan '%s' is not an area detector.",
			ad_record->name, scan_record->name );
	}

	ad = ad_record->record_class_struct;

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_AREA_DETECTOR pointer for record '%s' is NULL.",
			ad_record->name );
	}

	mx_status = mx_area_detector_construct_next_datafile_name(
						ad_record, TRUE, NULL, 0 );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_area_detector_scan_setup_datafile_naming( MX_RECORD *scan_record,
				mx_status_type (*initialize_fn)(MX_SCAN *),
				mx_status_type (*construct_fn)(MX_SCAN *) )
{
	static const char fname[] =
		"mx_area_detector_scan_setup_datafile_naming()";

	MX_AREA_DETECTOR_SCAN_FUNCTION_LIST *flist;

	MX_DEBUG(-2,("%s invoked.", fname));

	if ( scan_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The scan record pointer passed was NULL." );
	}
	if ( scan_record->mx_class != MXS_AREA_DETECTOR_SCAN ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Record '%s' is not an area detector scan record.",
			scan_record->name );
	}

	flist = scan_record->class_specific_function_list;

	if ( flist == (MX_AREA_DETECTOR_SCAN_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_AREA_DETECTOR_SCAN_FUNCTION_LIST pointer "
		"for scan '%s' is NULL.", scan_record->name );
	}

	flist->initialize_datafile_naming = initialize_fn;

	flist->construct_next_datafile_name = construct_fn;

	return MX_SUCCESSFUL_RESULT;
}

