/*
 * Name:    mx_scan_linear.c
 *
 * Purpose: Driver for MX_LINEAR_SCAN step scan records.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_array.h"
#include "mx_scan.h"
#include "mx_scan_linear.h"

#include "mx_poison.h"

MX_RECORD_FUNCTION_LIST mxs_linear_scan_record_function_list = {
	mxs_linear_scan_initialize_type,
	mxs_linear_scan_create_record_structures,
	mxs_linear_scan_finish_record_initialization,
	mxs_linear_scan_delete_record,
	mxs_linear_scan_print_scan_structure
};

MX_SCAN_FUNCTION_LIST mxs_linear_scan_scan_function_list = {
	mxs_linear_scan_prepare_for_scan_start,
	mxs_linear_scan_execute_scan_body,
	mxs_linear_scan_cleanup_after_scan_end
};

/* Prototype an internal use only function. */

static mx_status_type
mxs_linear_scan_execute_scan_level( MX_SCAN *scan,
		MX_LINEAR_SCAN *linear_scan,
		long dimension_level,
		mx_status_type (*compute_motor_positions_fptr)
					(MX_SCAN *, MX_LINEAR_SCAN *),
		mx_status_type (*move_special_fptr)
					(MX_SCAN *, MX_LINEAR_SCAN *,
					long, MX_RECORD **, double *,
					MX_MOTOR_MOVE_REPORT_FUNCTION,
					unsigned long)
		);

static mx_status_type
mxs_linear_scan_do_normal_scan( MX_SCAN *scan,
				MX_LINEAR_SCAN *linear_scan,
				long array_index,
				long num_dimension_steps,
				mx_status_type (*compute_motor_positions_fptr)
					(MX_SCAN *, MX_LINEAR_SCAN *),
				mx_status_type (*move_special_fptr)
					( MX_SCAN *, MX_LINEAR_SCAN *,
					long, MX_RECORD **, double *,
					MX_MOTOR_MOVE_REPORT_FUNCTION,
					unsigned long ),
				mx_bool_type start_fast_mode );

static mx_status_type
mxs_linear_scan_do_early_move_scan( MX_SCAN *scan,
				MX_LINEAR_SCAN *linear_scan,
				long array_index,
				long num_dimension_steps,
				mx_status_type (*compute_motor_positions_fptr)
					(MX_SCAN *, MX_LINEAR_SCAN *),
				mx_status_type (*move_special_fptr)
					( MX_SCAN *, MX_LINEAR_SCAN *,
					long, MX_RECORD **, double *,
					MX_MOTOR_MOVE_REPORT_FUNCTION,
					unsigned long ),
				mx_bool_type start_fast_mode );

/*=========*/

#define NUM_LINEAR_SCAN_FIELDS	4

MX_EXPORT mx_status_type
mxs_linear_scan_initialize_type( long record_type )
{
	static const char fname[] = "mxs_linear_scan_initialize_type()";

	static const char
		field_name[NUM_LINEAR_SCAN_FIELDS][MXU_FIELD_NAME_LENGTH+1]
	    = {
		"start_position",
		"step_size",
		"num_measurements",
		"step_number"};

	MX_DRIVER *driver;
	MX_RECORD_FIELD_DEFAULTS *record_field_defaults;
	MX_RECORD_FIELD_DEFAULTS **record_field_defaults_ptr;
	MX_RECORD_FIELD_DEFAULTS *field;
	int i;
	long num_record_fields;
	long num_independent_variables_varargs_cookie;
	long num_motors_varargs_cookie;
	long num_input_devices_varargs_cookie;
	mx_status_type mx_status;

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

	/**** Fix up the record fields specific to MX_LINEAR_SCAN records ****/

	for ( i = 0; i < NUM_LINEAR_SCAN_FIELDS; i++ ) {
		mx_status = mx_find_record_field_defaults(
			record_field_defaults, num_record_fields,
			field_name[i], &field );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		field->dimension[0] = num_independent_variables_varargs_cookie;
	}

	return MX_SUCCESSFUL_RESULT;
}

#undef NUM_LINEAR_SCAN_FIELDS		/* Just for safety's sake. */

MX_EXPORT mx_status_type
mxs_linear_scan_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxs_linear_scan_create_record_structures()";

	MX_SCAN *scan;
	MX_LINEAR_SCAN *linear_scan;
	MX_LINEAR_SCAN_FUNCTION_LIST *flist;
	MX_DRIVER *driver;
	mx_status_type (*fptr)( MX_RECORD *, MX_SCAN *, MX_LINEAR_SCAN * );
	mx_status_type mx_status;

	/* Allocate memory for the necessary structures. */

	scan = (MX_SCAN *) malloc( sizeof(MX_SCAN) );

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SCAN structure." );
	}

	linear_scan = (MX_LINEAR_SCAN *) malloc( sizeof(MX_LINEAR_SCAN) );

	if ( linear_scan == (MX_LINEAR_SCAN *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_LINEAR_SCAN structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_superclass_struct = scan;
	record->record_class_struct = linear_scan;
	record->record_type_struct = NULL;

	scan->record = record;

	scan->measurement.scan = scan;
	scan->datafile.scan = scan;
	scan->plot.scan = scan;

	scan->datafile.x_motor_array = NULL;
	scan->plot.x_motor_array = NULL;

	record->superclass_specific_function_list
				= &mxs_linear_scan_scan_function_list;

	scan->num_missing_records = 0;
	scan->missing_record_array = NULL;

	/* Find the MX_LINEAR_SCAN_FUNCTION_LIST pointer. */

	driver = mx_get_driver_by_type( record->mx_type );

	if ( driver == (MX_DRIVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_DRIVER for record type %ld is NULL.",
			record->mx_type );
	}

	flist = (MX_LINEAR_SCAN_FUNCTION_LIST *)
			(driver->class_specific_function_list);

	if ( flist == (MX_LINEAR_SCAN_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_LINEAR_SCAN_FUNCTION_LIST for record type %ld is NULL.",
			record->mx_type );
	}

	record->class_specific_function_list = flist;

	/* Call the type specific 'create_record_structures' function. */

	fptr = flist->create_record_structures;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"Linear scan type specific 'create_record_structures' pointer is NULL.");
	}

	mx_status = (*fptr)( record, scan, linear_scan );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_linear_scan_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxs_linear_scan_finish_record_initialization()";

	MX_SCAN *scan;
	MX_LINEAR_SCAN_FUNCTION_LIST *flist;
	mx_status_type (*fptr)( MX_RECORD * );
	long dimension[2];
	size_t element_size[2];
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s'.", fname, record->name));

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

	flist = (MX_LINEAR_SCAN_FUNCTION_LIST *)
				(record->class_specific_function_list);

	if ( flist == (MX_LINEAR_SCAN_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_LINEAR_SCAN_FUNCTION_LIST pointer is NULL.");
	}

	fptr = flist->finish_record_initialization;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"Linear scan type specific 'finish_record_initialization' pointer is NULL.");
	}

	mx_status = (*fptr)( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxs_linear_scan_delete_record( MX_RECORD *record )
{
	static const char fname[] = "mxs_linear_scan_delete_record()";

	MX_SCAN *scan;
	MX_LINEAR_SCAN *linear_scan;
	MX_LINEAR_SCAN_FUNCTION_LIST *flist;
	mx_status_type (*fptr)( MX_RECORD * );
	long dimension[2];
	size_t element_size[2];
	mx_status_type mx_status;

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

		linear_scan = (MX_LINEAR_SCAN *)
					(scan->record->record_class_struct);

		if ( linear_scan != NULL ) {
			flist = (MX_LINEAR_SCAN_FUNCTION_LIST *)
				(record->class_specific_function_list);

			if ( flist == (MX_LINEAR_SCAN_FUNCTION_LIST *) NULL ) {
				return mx_error(
					MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_LINEAR_SCAN_FUNCTION_LIST pointer is NULL.");
			}

			fptr = flist->delete_record;

			/* If fptr is NULL, this record type has no type
			 * specific structures to delete.  Otherwise,
			 * we invoke fptr to delete the type specific
			 * structures.
			 */

			if ( fptr != NULL ) {
				mx_status = (*fptr)( record );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
			}

			record->record_type_struct = NULL;
			linear_scan->record_type_struct = NULL;

			mx_free( linear_scan );

			record->record_class_struct = NULL;
		}

		if ( scan->datafile.x_motor_array != NULL ) {
			mx_free( scan->datafile.x_motor_array );
		}

		if ( scan->datafile.x_position_array != NULL ) {
			dimension[0] = scan->datafile.num_x_motors;
			dimension[1] = 1;

			element_size[0] = sizeof(double);
			element_size[1] = sizeof(double *);

			(void) mx_free_array( scan->datafile.x_position_array,
					      2, dimension, element_size );
		}

		if ( scan->plot.x_motor_array != NULL ) {
			mx_free( scan->plot.x_motor_array );
		}

		if ( scan->plot.x_position_array != NULL ) {
			dimension[0] = scan->plot.num_x_motors;
			dimension[1] = 1;

			element_size[0] = sizeof(double);
			element_size[1] = sizeof(double *);

			(void) mx_free_array( scan->plot.x_position_array,
					      2, dimension, element_size );
		}

		mx_free( scan );

		record->record_superclass_struct = NULL;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_linear_scan_print_scan_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxs_linear_scan_print_scan_structure()";

	static const char fake_motor_units[] = "???";
	MX_SCAN *scan;
	MX_LINEAR_SCAN *linear_scan;
	MX_RECORD *motor_record;
	MX_MOTOR *motor;
	long i, j;
	double end_position;
	const char *motor_units;
	mx_status_type mx_status;

	if ( record != NULL ) {
		fprintf( file, "SCAN parameters for linear scan '%s':\n",
				record->name );
	}

	mx_status = mx_scan_print_scan_structure( file, record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	scan = (MX_SCAN *) record->record_superclass_struct;

	linear_scan = (MX_LINEAR_SCAN *) record->record_class_struct;

	if ( linear_scan == (MX_LINEAR_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_LINEAR_SCAN pointer for scan '%s' is NULL.",
			record->name );
	}

	fprintf( file, "  Scan range:\n" );

	j = 0;

	if ( scan->num_motors == 0 ) {
		if ( scan->num_independent_variables > 0 ) {
			fprintf( file, "    %ld measurements\n",
				linear_scan->num_measurements[j] );
		}
	} else {
		for ( i = 0; i < scan->num_motors; i++ ) {
			if ( scan->motor_is_independent_variable[i] == TRUE ) {
				motor_record = scan->motor_record_array[i];

				motor = (MX_MOTOR *)
				    motor_record->record_class_struct;
	
				if ( motor_record->mx_superclass
					== MXR_PLACEHOLDER )
				{
					motor_units = fake_motor_units;
				} else {
					motor_units = motor->units;
				}

				end_position = linear_scan->start_position[j]
					+ linear_scan->step_size[j] * (double)
				    ( linear_scan->num_measurements[j] - 1 );
	
				fprintf( file,
		"    %s: %g %s to %g %s, %g %s step size, %ld measurements\n",
					scan->motor_record_array[i]->name,
					linear_scan->start_position[j],
					motor_units,
					end_position,
					motor_units,
					linear_scan->step_size[j],
					motor_units,
					linear_scan->num_measurements[j] );
	
				j++;
			}
		}
	}
	fprintf( file, "\n" );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_linear_scan_prepare_for_scan_start( MX_SCAN *scan )
{
	static const char fname[] = "mxs_linear_scan_prepare_for_scan_start()";

	MX_LINEAR_SCAN_FUNCTION_LIST *flist;
	mx_status_type (*fptr) (MX_SCAN *);
	int i;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked", fname));

	flist = (MX_LINEAR_SCAN_FUNCTION_LIST *)
				(scan->record->class_specific_function_list);

	if ( flist == (MX_LINEAR_SCAN_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_LINEAR_SCAN_FUNCTION_LIST pointer for scan '%s' is NULL.",
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
mxs_linear_scan_execute_scan_body( MX_SCAN *scan )
{
	static const char fname[] = "mxs_linear_scan_execute_scan_body()";

	MX_LINEAR_SCAN *linear_scan;
	MX_LINEAR_SCAN_FUNCTION_LIST *flist;
	mx_status_type (*compute_motor_positions_fptr)
					( MX_SCAN *, MX_LINEAR_SCAN * );
	mx_status_type (*move_special_fptr)
					( MX_SCAN *, MX_LINEAR_SCAN *,
					long, MX_RECORD **, double *,
					MX_MOTOR_MOVE_REPORT_FUNCTION,
					unsigned long );
	mx_status_type mx_status;

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_SCAN pointer passed was NULL." );
	}

	linear_scan = (MX_LINEAR_SCAN *)(scan->record->record_class_struct);

	if ( linear_scan == (MX_LINEAR_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_LINEAR_SCAN pointer for scan record '%s' is NULL.",
			scan->record->name );
	}

	flist = (MX_LINEAR_SCAN_FUNCTION_LIST *)
			(scan->record->class_specific_function_list);

	if ( flist == (MX_LINEAR_SCAN_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_LINEAR_SCAN_FUNCTION_LIST ptr for scan record '%s' is NULL.",
			scan->record->name );
	}

	compute_motor_positions_fptr = flist->compute_motor_positions;

	if ( compute_motor_positions_fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"'compute_motor_position' function pointer for scan record '%s' is NULL.",
			scan->record->name );
	}

	/* It is not an error for flist->motor_record_array_move_special
	 * to be NULL.  If this function ptr is NULL, this just means that
	 * mx_motor_record_array_move_absolute will be called directly
	 * rather than being preprocessed by motor_record_array_move_special.
	 */

	move_special_fptr = flist->motor_record_array_move_special;

	mx_status = mxs_linear_scan_execute_scan_level( scan, linear_scan,
			scan->num_independent_variables - 1,
			compute_motor_positions_fptr,
			move_special_fptr );

	return mx_status;
}

MX_EXPORT mx_status_type
mxs_linear_scan_cleanup_after_scan_end( MX_SCAN *scan )
{
	return mx_standard_cleanup_after_scan_end( scan );
}

static mx_status_type
mxs_linear_scan_move_absolute(
	MX_SCAN *scan,
	MX_LINEAR_SCAN *linear_scan,
	mx_status_type (*move_special_fptr)
				( MX_SCAN *, MX_LINEAR_SCAN *,
				long, MX_RECORD **, double *,
				MX_MOTOR_MOVE_REPORT_FUNCTION,
				unsigned long )
	)
{
	unsigned long flags;
	mx_status_type mx_status;

	/* If move_special_fptr is NULL, we just invoke
	 * mx_motor_array_move_absolute directly.
	 * Otherwise, the computed move must be
	 * preprocessed by move_special_fptr before
	 * the motors are moved.  See how slit scans
	 * are implemented in 'libMx/s_slit.c' for
	 * an example of this.
	 */

	flags = MXF_MTR_NOWAIT | MXF_MTR_SCAN_IN_PROGRESS;

	if ( move_special_fptr == NULL ) {
		if ( scan->num_motors > 0 ) {
			mx_status = mx_motor_array_move_absolute_with_report(
						scan->num_motors,
						scan->motor_record_array,
						scan->motor_position,
						NULL,
						flags );
		} else {
			mx_status = MX_SUCCESSFUL_RESULT;
		}
	} else {
		mx_status = (*move_special_fptr) ( scan,
						linear_scan,
						scan->num_motors,
						scan->motor_record_array,
						scan->motor_position,
						NULL,
						flags );
	}

	return mx_status;
}

static mx_status_type
mxs_linear_scan_execute_scan_level( MX_SCAN *scan,
		MX_LINEAR_SCAN *linear_scan,
		long dimension_level,
		mx_status_type (*compute_motor_positions_fptr)
					(MX_SCAN *, MX_LINEAR_SCAN *),
		mx_status_type (*move_special_fptr)
					( MX_SCAN *, MX_LINEAR_SCAN *,
					long, MX_RECORD **, double *,
					MX_MOTOR_MOVE_REPORT_FUNCTION,
					unsigned long )
		)
{
	static const char fname[] = "mxs_linear_scan_execute_scan_level()";

	long i, array_index, num_dimension_steps, new_dimension_level;
	mx_bool_type fast_mode, start_fast_mode, early_move_flag;
	mx_status_type mx_status;

	array_index = scan->num_independent_variables - dimension_level - 1;

	num_dimension_steps = linear_scan->num_measurements[ array_index ];

	new_dimension_level = dimension_level - 1;

	MX_DEBUG( 2,
("%s: dimension_level = %ld, array_index = %ld, num_dimension_steps = %ld",
		fname, dimension_level, array_index, num_dimension_steps ));

	if ( dimension_level > 0 ) {
		/** More levels in the array to descend through. **/

		MX_DEBUG( 2,("%s: -> Go to next dimension level = %ld",
					fname, new_dimension_level));

		for ( i = 0; i < num_dimension_steps; i++ ) {

			linear_scan->step_number[ array_index ] = i;

			mx_status = mxs_linear_scan_execute_scan_level(scan,
					linear_scan,
					new_dimension_level,
					compute_motor_positions_fptr,
					move_special_fptr);

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	} else {
		/** At bottom level of independent variables. **/

		/* Is fast mode already on?  If not, then arrange for it
		 * to be turned on after the first measurement.
		 */

		mx_status = mx_get_fast_mode( scan->record, &fast_mode );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( fast_mode ) {
			start_fast_mode = FALSE;
		} else {
			start_fast_mode = TRUE;
		}

		/* Have we been asked to start moving to the next measurement
		 * position before the previous measurement is read out?
		 */

		mx_status = mx_scan_get_early_move_flag(scan, &early_move_flag);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		MX_DEBUG( 2,
		("%s: At bottom level of independent variables.", fname ));

		if ( early_move_flag == FALSE ) {
			mx_status = mxs_linear_scan_do_normal_scan(
						scan,
						linear_scan,
						array_index,
						num_dimension_steps,
						compute_motor_positions_fptr,
						move_special_fptr,
						start_fast_mode );
		} else {
			mx_status = mxs_linear_scan_do_early_move_scan(
						scan,
						linear_scan,
						array_index,
						num_dimension_steps,
						compute_motor_positions_fptr,
						move_special_fptr,
						start_fast_mode );
		}

		(scan->plot.section_number)++;
	}
	return MX_SUCCESSFUL_RESULT;
}

/*---*/

static mx_status_type
mxs_linear_scan_do_normal_scan( MX_SCAN *scan,
				MX_LINEAR_SCAN *linear_scan,
				long array_index,
				long num_dimension_steps,
				mx_status_type (*compute_motor_positions_fptr)
					(MX_SCAN *, MX_LINEAR_SCAN *),
				mx_status_type (*move_special_fptr)
					( MX_SCAN *, MX_LINEAR_SCAN *,
					long, MX_RECORD **, double *,
					MX_MOTOR_MOVE_REPORT_FUNCTION,
					unsigned long ),
				mx_bool_type start_fast_mode )
{
	long i;
	mx_status_type mx_status;

	if ( scan->num_motors > 0 ) {
		mx_scanlog_info(
		"Moving to start position for scan section.");
	}

	mx_status = MX_SUCCESSFUL_RESULT;

	for ( i = 0; i < num_dimension_steps; i++ ) {

		/* Compute the next set of motor positions. */

		linear_scan->step_number[ array_index ] = i;

		mx_status = (*compute_motor_positions_fptr)
					( scan, linear_scan );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		do {		/** Pause/abort retry loop. **/

			/* Move to the computed motor positions. */

			mx_status = mxs_linear_scan_move_absolute(
				scan, linear_scan, move_special_fptr );

			if ( ( mx_status.code != MXE_SUCCESS )
			  && ( mx_status.code != MXE_PAUSE_REQUESTED ) )
			{
				return mx_status;
			}

			/* Wait for the move to complete. */

			mx_status = mx_wait_for_motor_array_stop(
				scan->num_motors,
				scan->motor_record_array,
				MXF_MTR_SCAN_IN_PROGRESS );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* Perform the measurement. */

			if ( mx_status.code != MXE_PAUSE_REQUESTED ) {

			    mx_status =
			       mx_scan_acquire_and_readout_data( scan );

				if ( ( mx_status.code != MXE_SUCCESS )
				  && ( mx_status.code !=
						MXE_PAUSE_REQUESTED ) )
				{
					return mx_status;
				}
			}

			/* If a pause has not been requested by the
			 * user, exit the pause/abort retry loop.
			 */

			if ( mx_status.code == MXE_SUCCESS ) {
				break;   /* Exit the do..while loop */
			}

			/* If we get here, the most recent status
			 * code was MXE_PAUSE_REQUESTED, so invoke
			 * a function to handle the pause request.
			 */

			mx_status = mx_scan_handle_pause_request(scan);

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

		} while (1);	/** End of pause/abort retry loop. **/

		/* If alternate X axis motors have been specified,
		 * get and save their current positions for use
		 * by the datafile handling code.
		 */

		mx_status = mx_scan_handle_alternate_x_motors( scan );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/**** Write the result out to the datafile. ****/

		mx_status = mx_add_measurement_to_datafile(
						&(scan->datafile) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Report the progress of the scan back to the user. */

		mx_status = mx_scan_display_scan_progress( scan );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Finally, if enabled, add the measurement to the
		 * running scan plot.
		 */

		if ( mx_plotting_is_enabled( scan->record ) ) {

			if ( i == 0 ) {
				/* Tell the plotting package that
				 * this is a new section of the scan.
				 */

				mx_status = mx_plot_start_plot_section(
						&(scan->plot) );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
			}

			mx_status = mx_add_measurement_to_plot_buffer(
						&(scan->plot) );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_status = mx_display_plot( &(scan->plot) );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

		mx_status = mx_scan_increment_measurement_number(scan);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* If needed, start fast mode. */

		if ( start_fast_mode ) {
			mx_status = 
				mx_set_fast_mode( scan->record, TRUE);

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			start_fast_mode = FALSE;
		}
	}

	return mx_status;
}

/*---*/

static mx_status_type
mxs_linear_scan_do_early_move_scan( MX_SCAN *scan,
				MX_LINEAR_SCAN *linear_scan,
				long array_index,
				long num_dimension_steps,
				mx_status_type (*compute_motor_positions_fptr)
					(MX_SCAN *, MX_LINEAR_SCAN *),
				mx_status_type (*move_special_fptr)
					( MX_SCAN *, MX_LINEAR_SCAN *,
					long, MX_RECORD **, double *,
					MX_MOTOR_MOVE_REPORT_FUNCTION,
					unsigned long ),
				mx_bool_type start_fast_mode )
{
	long i;
	mx_status_type mx_status;

	if ( scan->num_motors > 0 ) {
		mx_scanlog_info(
		"Moving to start position for scan section.");
	}

	/* Get ready to move to the start position. */

	linear_scan->step_number[ array_index ] = 0;

	mx_status = (*compute_motor_positions_fptr)( scan, linear_scan );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Move to the start position. */

	mx_status = mxs_linear_scan_move_absolute(
			scan, linear_scan, move_special_fptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*** Now loop through the measurements. ***/

	for ( i = 0; i < num_dimension_steps; i++ ) {

		/* Wait for the motors to get to the next position. */

		mx_status = mx_wait_for_motor_array_stop( scan->num_motors,
						scan->motor_record_array,
						MXF_MTR_SCAN_IN_PROGRESS );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* If alternate X axis motors have been specified,
		 * get and save their current positions for use
		 * by the datafile handling code.
		 */

		mx_status = mx_scan_handle_alternate_x_motors( scan );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Acquire data for the current measurement. */

		mx_status = mx_scan_acquire_data( scan );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* If this is not the last step of the scan level, then
		 * start the move to the next step position.  We do not
		 * wait for the move to complete at this point.
		 */

		if ( i < (num_dimension_steps - 1) ) {

			/* Compute the next set of motor positions. */

			linear_scan->step_number[ array_index ] = i+1;

			mx_status = (*compute_motor_positions_fptr)
						( scan, linear_scan );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			/* Start the move to the computed motor positions. */

			mx_status = mxs_linear_scan_move_absolute(
					scan, linear_scan, move_special_fptr );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		} else {
			/* If this _is_ the last step of the scan level, tell
			 * the scan to update all of the 'old_destination"
			 * fields of the independent motor records.
			 */

			mx_status = mx_scan_update_old_destinations( scan );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

		/* Read out the acquired data while the motors are
		 * moving to the next step position.
		 */

		mx_status = mx_readout_data( &(scan->measurement) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/**** Write the result out to the datafile. ****/

		mx_status = mx_add_measurement_to_datafile(
						&(scan->datafile) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Report the progress of the scan back to the user. */

		mx_status = mx_scan_display_scan_progress( scan );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Finally, if enabled, add the measurement to the
		 * running scan plot.
		 */

		if ( mx_plotting_is_enabled( scan->record ) ) {

			if ( i == 0 ) {
				/* Tell the plotting package that
				 * this is a new section of the scan.
				 */

				mx_status = mx_plot_start_plot_section(
						&(scan->plot) );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
			}

			mx_status = mx_add_measurement_to_plot_buffer(
						&(scan->plot) );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_status = mx_display_plot( &(scan->plot) );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

		mx_status = mx_scan_increment_measurement_number(scan);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* If needed, start fast mode. */

		if ( start_fast_mode ) {
			mx_status = 
				mx_set_fast_mode( scan->record, TRUE);

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			start_fast_mode = FALSE;
		}
	}

	return mx_status;
}

