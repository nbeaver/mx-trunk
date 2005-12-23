/*
 * Name:    mx_scan_list.c
 *
 * Purpose: Driver for MX_LIST_SCAN scan records.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003-2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_array.h"
#include "mx_scan.h"
#include "mx_scan_list.h"
#include "sl_file.h"

MX_RECORD_FUNCTION_LIST mxs_list_scan_record_function_list = {
	mxs_list_scan_initialize_type,
	mxs_list_scan_create_record_structures,
	mxs_list_scan_finish_record_initialization,
	mxs_list_scan_delete_record,
	mxs_list_scan_print_scan_structure
};

MX_SCAN_FUNCTION_LIST mxs_list_scan_scan_function_list = {
	mxs_list_scan_prepare_for_scan_start,
	mxs_list_scan_execute_scan_body,
	mxs_list_scan_cleanup_after_scan_end
};

MX_EXPORT mx_status_type
mxs_list_scan_initialize_type( long record_type )
{
	static const char fname[] = "mxs_list_scan_initialize_type()";

	MX_DRIVER *driver;
	MX_RECORD_FIELD_DEFAULTS *record_field_defaults;
	MX_RECORD_FIELD_DEFAULTS **record_field_defaults_ptr;
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

	mx_status = mx_scan_fixup_varargs_record_field_defaults(
			record_field_defaults, num_record_fields,
			&num_independent_variables_varargs_cookie,
			&num_motors_varargs_cookie,
			&num_input_devices_varargs_cookie );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/** Fix up the record fields specific to MX_LIST_SCAN records **/

	/* Currently there are none. */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_list_scan_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxs_list_scan_create_record_structures()";

	MX_SCAN *scan;
	MX_LIST_SCAN *list_scan;
	MX_LIST_SCAN_FUNCTION_LIST *flist;
	MX_DRIVER *driver;
	mx_status_type (*fptr)( MX_RECORD *, MX_SCAN *, MX_LIST_SCAN * );
	mx_status_type mx_status;

	/* Allocate memory for the necessary structures. */

	scan = (MX_SCAN *) malloc( sizeof(MX_SCAN) );

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SCAN structure." );
	}

	list_scan
		= (MX_LIST_SCAN *) malloc( sizeof(MX_LIST_SCAN) );

	if ( list_scan == (MX_LIST_SCAN *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_LIST_SCAN structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_superclass_struct = scan;
	record->record_class_struct = list_scan;
	record->record_type_struct = NULL;	/* Not used by this class. */

	scan->record = record;

	scan->measurement.scan = scan;
	scan->datafile.scan = scan;
	scan->plot.scan = scan;

	scan->datafile.x_motor_array = NULL;
	scan->plot.x_motor_array = NULL;

	record->superclass_specific_function_list
				= &mxs_list_scan_scan_function_list;

	scan->num_missing_records = 0;
	scan->missing_record_array = NULL;

	/* Find the MX_LIST_SCAN_FUNCTION_LIST pointer. */

	driver = mx_get_driver_by_type( record->mx_type );

	if ( driver == (MX_DRIVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_DRIVER for record type %ld is NULL.", record->mx_type );
	}

	flist = (MX_LIST_SCAN_FUNCTION_LIST *)
			(driver->class_specific_function_list);

	if ( flist == (MX_LIST_SCAN_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_LIST_SCAN_FUNCTION_LIST for record type %ld is NULL.",
			record->mx_type );
	}

	record->class_specific_function_list = flist;

	/* Call the type specific 'create_record_structures' function. */

	fptr = flist->create_record_structures;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"List scan type specific 'create_record_structures' pointer is NULL.");
	}

	mx_status = (*fptr)( record, scan, list_scan );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_list_scan_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[]="mxs_list_scan_finish_record_initialization()";

	MX_SCAN *scan;
	MX_LIST_SCAN_FUNCTION_LIST *flist;
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

	flist = (MX_LIST_SCAN_FUNCTION_LIST *)
				(record->class_specific_function_list);

	if ( flist == (MX_LIST_SCAN_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_LIST_SCAN_FUNCTION_LIST pointer is NULL.");
	}

	fptr = flist->finish_record_initialization;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"List scan type specific 'finish_record_initialization' pointer is NULL.");
	}

	mx_status = (*fptr)( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxs_list_scan_delete_record( MX_RECORD *record )
{
	static const char fname[] = "mxs_list_scan_delete_record()";

	MX_SCAN *scan;
	MX_LIST_SCAN *list_scan;
	MX_LIST_SCAN_FUNCTION_LIST *flist;
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

		list_scan = (MX_LIST_SCAN *) scan->record->record_class_struct;

		if ( list_scan != NULL ) {
			flist = (MX_LIST_SCAN_FUNCTION_LIST *)
				(record->class_specific_function_list);

			if ( flist == (MX_LIST_SCAN_FUNCTION_LIST *) NULL ) {
				return mx_error(
					MXE_CORRUPT_DATA_STRUCTURE, fname,
				"MX_LIST_SCAN_FUNCTION_LIST pointer is NULL.");
			}

			fptr = flist->delete_record;

			if ( fptr == NULL ) {
				return mx_error(
					MXE_CORRUPT_DATA_STRUCTURE, fname,
	"List scan type specific 'delete_record' pointer is NULL.");
			}

			mx_status = (*fptr)( record );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			mx_free( list_scan );

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
mxs_list_scan_print_scan_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxs_list_scan_print_scan_structure()";

	MX_FILE_LIST_SCAN *file_list_scan;
	mx_status_type mx_status;

	if ( record != NULL ) {
		fprintf( file, "SCAN parameters for list scan '%s':\n",
				record->name );
	}

	mx_status = mx_scan_print_scan_structure( file, record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	file_list_scan = (MX_FILE_LIST_SCAN *) record->record_type_struct;

	if ( file_list_scan == (MX_FILE_LIST_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_FILE_LIST_SCAN pointer for scan '%s' is NULL.",
			record->name );
	}

	fprintf( file, "  Position filename = '%s'\n\n",
				file_list_scan->position_filename );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_list_scan_prepare_for_scan_start( MX_SCAN *scan )
{
	static const char fname[] = "mxs_list_scan_prepare_for_scan_start()";

	MX_LIST_SCAN *list_scan;
	MX_LIST_SCAN_FUNCTION_LIST *flist;
	mx_status_type (*fptr)( MX_LIST_SCAN * );
	int i;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.",fname));

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_SCAN pointer passed was NULL." );
	}

	list_scan = (MX_LIST_SCAN *)(scan->record->record_class_struct);

	if ( list_scan == (MX_LIST_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_LIST_SCAN pointer for scan record '%s' is NULL.",
			scan->record->name );
	}

	flist = (MX_LIST_SCAN_FUNCTION_LIST *)
				(scan->record->class_specific_function_list);

	if ( flist == (MX_LIST_SCAN_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_LIST_SCAN_FUNCTION_LIST pointer is NULL.");
	}

	fptr = flist->open_list;

	if ( fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"List scan type specific 'open_list' pointer is NULL.");
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

	/* If we are displaying a plot, tell the plotting package that this
	 * is a new section of the scan.
	 */

	if ( mx_plotting_is_enabled( scan->record ) ) {

		mx_status = mx_plot_start_plot_section( &(scan->plot) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Open the position list. */

	mx_status = (*fptr)( list_scan );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

/*===*/

#define CLOSE_POSITION_LIST \
	do { \
		(void) (*close_list_fptr) (list_scan); \
	} while(0)

MX_EXPORT mx_status_type
mxs_list_scan_execute_scan_body( MX_SCAN *scan )
{
	static const char fname[] = "mxs_list_scan_execute_scan_body()";

	MX_LIST_SCAN *list_scan;
	MX_LIST_SCAN_FUNCTION_LIST *flist;
	MX_RECORD *x_motor_record;
	double x_motor_position;
	long j, k;
	int get_position;
	mx_status_type (*close_list_fptr) ( MX_LIST_SCAN * );
	mx_status_type (*get_next_measurement_parameters_fptr)
						( MX_SCAN *, MX_LIST_SCAN * );
	mx_status_type mx_status, mx_status2;

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_SCAN pointer passed was NULL." );
	}

	list_scan = (MX_LIST_SCAN *)(scan->record->record_class_struct);

	if ( list_scan == (MX_LIST_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_LIST_SCAN pointer for scan record '%s' is NULL.",
			scan->record->name );
	}

	flist = (MX_LIST_SCAN_FUNCTION_LIST *)
			(scan->record->class_specific_function_list);

	if ( flist == (MX_LIST_SCAN_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_LIST_SCAN_FUNCTION_LIST ptr for scan record '%s' is NULL.",
			scan->record->name );
	}

	get_next_measurement_parameters_fptr =
		flist->get_next_measurement_parameters;

	if ( get_next_measurement_parameters_fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"'get_next_measurement_parameters' function pointer "
		"for scan record '%s' is NULL.",
			scan->record->name );
	}

	close_list_fptr = flist->close_list;

	if ( close_list_fptr == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"'close_list' function pointer for scan record '%s' is NULL.",
			scan->record->name );
	}

	/* Now step through all the positions in the position list. */

	mx_info("Moving to start position.");

	while(1) {
		/* Get the next set of motor positions. */

		mx_status =
		  (*get_next_measurement_parameters_fptr)( scan, list_scan );

		if ( mx_status.code == MXE_END_OF_DATA ) {
			/* We have reached the end of the position list
			 * so we can exit the while(1) loop.
			 */
			break;

		} else if ( mx_status.code != MXE_SUCCESS ) {
			CLOSE_POSITION_LIST;
			return mx_status;
		} 

		do {		/** Pause/abort retry loop. **/

			/* Move to the computed motor positions. */

			mx_status = mx_motor_array_move_absolute_with_report(
				scan->num_motors,
				scan->motor_record_array,
				scan->motor_position,
				NULL,
				MXF_MTR_SCAN_IN_PROGRESS );

			if ( ( mx_status.code != MXE_SUCCESS )
			  && ( mx_status.code != MXE_PAUSE_REQUESTED ) )
			{
				CLOSE_POSITION_LIST;
				return mx_status;
			}

			/* Perform the measurement. */

			if ( mx_status.code != MXE_PAUSE_REQUESTED ) {
				mx_status =
				    mx_scan_handle_data_measurement( scan );

				if ( mx_status.code != MXE_SUCCESS ) {
					CLOSE_POSITION_LIST;
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

			mx_status = mx_scan_handle_pause_request( scan );

			switch( mx_status.code ) {
			case MXE_SUCCESS:
				mx_info("Retrying the last scan step.");

				mx_status = mx_wait_for_motor_array_stop(
					scan->num_motors,
					scan->motor_record_array,
					( MXF_MTR_SCAN_IN_PROGRESS
						    | MXF_MTR_IGNORE_PAUSE ) );

				if ( mx_status.code != MXE_SUCCESS ) {
					CLOSE_POSITION_LIST;
					return mx_status;
				}

				/* Back to the top of the do...while() loop.*/
				break;
			case MXE_STOP_REQUESTED:
				CLOSE_POSITION_LIST;

				mx_info(
				"Waiting for the motors to stop moving.");

				mx_status2 = mx_wait_for_motor_array_stop(
					scan->num_motors,
					scan->motor_record_array,
					( MXF_MTR_SCAN_IN_PROGRESS
						    | MXF_MTR_IGNORE_PAUSE ) );

				if ( mx_status2.code != MXE_SUCCESS )
					return mx_status2;

				return mx_status;
				break;
			case MXE_INTERRUPTED:
				mx_info( "Aborting current motor moves." );

				for (j = 0; j < scan->num_motors; j++) {
					(void) mx_motor_soft_abort(
						   scan->motor_record_array[j]);
				}
				return mx_status;
				break;
			case MXE_PAUSE_REQUESTED:
				/* Ignore additional pause requests. */

				break;
			default:
				CLOSE_POSITION_LIST;
				return mx_status;
				break;
			}

		} while (1);	/** End of pause/abort retry loop. **/


		/* If alternate X axis motors have been specified,
		 * get and save their current positions for use
		 * by the datafile handling code.
		 *
		 * Note that x_position_array is an N by 1 array,
		 * where N is the number of motors, since we only
		 * handle one measurement at a time in step scans.
		 */

		for ( j = 0; j < scan->datafile.num_x_motors; j++ ) {

			x_motor_record = scan->datafile.x_motor_array[j];

			mx_status = mx_motor_get_position( x_motor_record,
							&x_motor_position );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			scan->datafile.x_position_array[j][0]
				= x_motor_position;
		}

		/* Do the same thing for the plot data.
		 *
		 * The list of alternate X motors for the plot will
		 * not necessarily be the same as the list for the
		 * datafile, but we do not want to unnecessarily
		 * read a motor position twice.
		 */

		for ( j = 0; j < scan->plot.num_x_motors; j++ ) {

			x_motor_record = scan->plot.x_motor_array[j];

			/* See if this motor's position was already
			 * read by the datafile loop above.
			 */

			get_position = TRUE;

			for ( k = 0; k < scan->datafile.num_x_motors; k++ ) {

				if ( x_motor_record ==
					    scan->datafile.x_motor_array[j] )
				{
				    scan->plot.x_position_array[j][0] =
					  scan->datafile.x_position_array[k][0];

				    get_position = FALSE;
				    break;	/* Exit the inner for() loop. */
				}
			}

			/* If not, then we must read the position now. */

			if ( get_position ) {
				mx_status = mx_motor_get_position( x_motor_record,
							&x_motor_position );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

				scan->plot.x_position_array[j][0]
					= x_motor_position;
			}
		}

		/**** Write the result out to the datafile. ****/

		mx_status = mx_add_measurement_to_datafile( &(scan->datafile) );

		if ( mx_status.code != MXE_SUCCESS ) {
			CLOSE_POSITION_LIST;
			return mx_status;
		}

		/* Report the progress of the scan back to the user. */

		mx_status = mx_scan_display_scan_progress( scan );

		if ( mx_status.code != MXE_SUCCESS ) {
			CLOSE_POSITION_LIST;
			return mx_status;
		}

		/* Finally, if enabled, add the measurement to the
		 * running scan plot.
		 */

		if ( mx_plotting_is_enabled( scan->record ) ) {

			mx_status = mx_add_measurement_to_plot_buffer(
						&(scan->plot) );

			if ( mx_status.code != MXE_SUCCESS ) {
				CLOSE_POSITION_LIST;
				return mx_status;
			}

			mx_status = mx_display_plot( &(scan->plot) );

			if ( mx_status.code != MXE_SUCCESS ) {
				CLOSE_POSITION_LIST;
				return mx_status;
			}
		}

		mx_status = mx_scan_increment_measurement_number( scan );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Close the position list without ignoring the returned status. */

	mx_status = (*close_list_fptr) ( list_scan );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxs_list_scan_cleanup_after_scan_end( MX_SCAN *scan )
{
	return mx_standard_cleanup_after_scan_end( scan );
}


