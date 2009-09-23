/*
 * Name:    mx_scan.c
 *
 * Purpose: Scan record support for the MX library.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define DEBUG_CLEAR_TIMING	TRUE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <ctype.h>
#include <errno.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_key.h"
#include "mx_driver.h"
#include "mx_record.h"
#include "mx_scan.h"
#include "mx_scan_linear.h"
#include "mx_relay.h"
#include "mx_analog_input.h"
#include "mx_analog_output.h"
#include "mx_digital_input.h"
#include "mx_digital_output.h"
#include "mx_scaler.h"
#include "mx_amplifier.h"
#include "mx_mca.h"
#include "mx_variable.h"
#include "mx_array.h"
#include "mx_hrt.h"
#include "mx_hrt_debug.h"

#include "f_custom.h"
#include "p_custom.h"

/* --------------- */

static mx_status_type ( *mx_scan_pause_request_handler ) ( MX_SCAN * )
				= mx_default_scan_pause_request_handler;

static mx_status_type mx_update_dark_currents( MX_SCAN * );

static mx_status_type mx_setup_scan_shutter( MX_SCAN * );

static mx_status_type mx_setup_measurement_permit_and_fault_handlers(
								MX_SCAN * );

static mx_status_type mx_scan_free_measurement_permit_and_fault_handlers(
								MX_SCAN * );

/* --------------- */

/* mx_scan_finish_record_initialization() performs data structure
 * initialization steps that must be done for all scan records.
 */

MX_EXPORT mx_status_type
mx_scan_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] = "mx_scan_finish_record_initialization()";

	MX_SCAN *scan;
	MX_MEASUREMENT_TYPE_ENTRY *measurement_type_entry;
	MX_DATAFILE_TYPE_ENTRY *datafile_type_entry;
	MX_PLOT_TYPE_ENTRY *plot_type_entry;
	char *ptr;
	ptrdiff_t length;
	mx_status_type mx_status;

	scan = ( MX_SCAN * ) record->record_superclass_struct;

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SCAN pointer for scan record '%s' is NULL.",
			record->name );
	}

	/*-------------------------------------------------------------------*/

	/* Find the driver for the specified measurement type. */

	mx_status = mx_get_measurement_type_by_name( mx_measurement_type_list,
			scan->measurement_type, &measurement_type_entry );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	scan->measurement.type              = measurement_type_entry->type;
	scan->measurement.mx_typename       = scan->measurement_type;
	scan->measurement.measurement_type_struct = NULL;
	scan->measurement.measurement_function_list
			= measurement_type_entry->measurement_function_list;
	scan->measurement.measurement_arguments = scan->measurement_arguments;

	/*-------------------------------------------------------------------*/

	/* Split the datafile type description into the type name and the
	 * type arguments using a colon ':' character as the separator.
	 * If there is no colon character, then the entire string is the
	 * type name.
	 */

	ptr = strchr( scan->datafile_description, ':' );

	if ( ptr == NULL ) {
		strlcpy( scan->datafile.mx_typename,
				scan->datafile_description,
				MXU_DATAFILE_TYPE_NAME_LENGTH );

		scan->datafile.options[0] = '\0';

	} else {
		length = ptr - scan->datafile_description + 1;

		if ( length > MXU_DATAFILE_TYPE_NAME_LENGTH ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"The datafile type name part of the datafile type description "
	"for scan '%s' is longer than the maximum length of %d.  "
	"Datafile type description = '%s'.",
				scan->record->name,
				MXU_DATAFILE_TYPE_NAME_LENGTH,
				scan->datafile_description );
		}

		strlcpy( scan->datafile.mx_typename,
				scan->datafile_description,
				length );

		/* The type arguments start after the colon character. */

		ptr++;

		strlcpy( scan->datafile.options,
				ptr, MXU_DATAFILE_OPTIONS_LENGTH );
	}

#if 0
	if ( strlen( scan->datafile.options ) > 0 ) {
		MX_DEBUG(-2,
		("Scan '%s' data, mx_typename = '%s', type_arguments = '%s'",
			scan->record->name, scan->datafile.mx_typename,
			scan->datafile.options));
	}
#endif

	/* Find the driver for the specified datafile type. */

	mx_status = mx_get_datafile_type_by_name( mx_datafile_type_list,
			scan->datafile.mx_typename, &datafile_type_entry );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	scan->datafile.type                 = datafile_type_entry->type;
	scan->datafile.datafile_type_struct = NULL;
	scan->datafile.datafile_function_list
				= datafile_type_entry->datafile_function_list;
	scan->datafile.filename             = scan->datafile_name;

	scan->datafile.num_x_motors = 0;
	scan->datafile.x_motor_array = NULL;
	scan->datafile.x_position_array = NULL;

	scan->datafile.normalize_data = FALSE;

	if ( strlen( scan->datafile.options ) > 0 ) {
		mx_status = mx_datafile_parse_options( &(scan->datafile) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/*-------------------------------------------------------------------*/

	/* Split the plot type description into the type name and the
	 * type arguments using a colon ':' character as the separator.
	 * If there is no colon character, then the entire string is the
	 * type name.
	 */

	ptr = strchr( scan->plot_description, ':' );

	if ( ptr == NULL ) {
		strlcpy( scan->plot.mx_typename,
				scan->plot_description,
				MXU_PLOT_TYPE_NAME_LENGTH );

		scan->plot.options[0] = '\0';

	} else {
		length = ptr - scan->plot_description + 1;

		if ( length > MXU_PLOT_TYPE_NAME_LENGTH ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"The plot type name part of the plot type description "
	"for scan '%s' is longer than the maximum length of %d.  "
	"Plot type description = '%s'.",
				scan->record->name,
				MXU_PLOT_TYPE_NAME_LENGTH,
				scan->plot_description );
		}

		strlcpy( scan->plot.mx_typename,
				scan->plot_description,
				length );

		/* The type arguments start after the colon character. */

		ptr++;

		strlcpy( scan->plot.options,
				ptr, MXU_PLOT_OPTIONS_LENGTH );
	}

#if 0
	if ( strlen( scan->plot.options ) > 0 ) {
		MX_DEBUG(-2,
		("Scan '%s' plot, mx_typename = '%s', type_arguments = '%s'",
			scan->record->name, scan->plot.mx_typename,
			scan->plot.options));
	}
#endif

	/* Find the driver for the specified plot type. */

	mx_status = mx_get_plot_type_by_name( mx_plot_type_list,
			scan->plot.mx_typename, &plot_type_entry );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	scan->plot.type               = plot_type_entry->type;
	scan->plot.plot_type_struct   = NULL;
	scan->plot.plot_function_list = plot_type_entry->plot_function_list;
	scan->plot.plot_arguments     = scan->plot_arguments;

	scan->plot.num_x_motors = 0;
	scan->plot.x_motor_array = NULL;
	scan->plot.x_position_array = NULL;

	scan->plot.continuous_plot = FALSE;
	scan->plot.normalize_data = FALSE;

	if ( strlen( scan->plot.options ) > 0 ) {
		mx_status = mx_plot_parse_options( &(scan->plot) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/*-------------------------------------------------------------------*/

	scan->shutter_record = NULL;

	scan->num_measurement_permit_handlers = 0;
	scan->num_measurement_fault_handlers = 0;

	scan->measurement_permit_handler_array = NULL;
	scan->measurement_fault_handler_array = NULL;

	return MX_SUCCESSFUL_RESULT;
}

/* --------------- */

MX_EXPORT mx_status_type
mx_scan_print_scan_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mx_scan_print_scan_structure()";

	MX_SCAN *scan;
	MX_DRIVER *driver;
	int num_items;
	long i, measurement_counts;
	double measurement_time, pulse_period;
	size_t length;
	char format[40];
	char device_name[MXU_RECORD_NAME_LENGTH+1];
	char scan_type[MXU_DRIVER_NAME_LENGTH+1];

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	scan = (MX_SCAN *) record->record_superclass_struct;

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SCAN pointer for record '%s' is NULL.", record->name );
	}

	driver = mx_get_driver_by_type( record->mx_type );

	if ( driver == (MX_DRIVER *) NULL ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Record type %ld not found.",
			record->mx_type );
	}

	strlcpy( scan_type, driver->name, MXU_DRIVER_NAME_LENGTH + 1 );

	length = strlen(scan_type);

	for ( i = 0; i < length; i++ ) {
		if ( islower( (int) (scan_type[i]) ) ) {
			scan_type[i] = toupper( (int)(scan_type[i]) );
		}
	}

	fprintf( file, "  Scan type             = %s\n\n", scan_type );

	fprintf( file, "  Name                  = %s\n", record->name );
	fprintf( file, "  Number of scans       = %ld\n", scan->num_scans );
	fprintf( file, "\n" );

	if ( record->mx_type != MXS_XAF_STANDARD ) {
		if ( scan->num_motors == 1 ) {
			fprintf( file, "  1 motor:         " );
		} else {
			fprintf( file, "  %ld motors:        ",
					scan->num_motors );
		}
		for ( i = 0; i < scan->num_motors; i++ ) {
			if ( i > 0 ) {
				fprintf( file, ", " );
			}
			fprintf( file, "%s",
				scan->motor_record_array[i]->name );
		}
		fprintf( file, "\n" );
	}

	if ( scan->num_input_devices == 1 ) {
		fprintf( file, "  1 input device:  " );
	} else {
		fprintf( file, "  %ld input devices: ",
			scan->num_input_devices );
	}
	for ( i = 0; i < scan->num_input_devices; i++ ) {
		if ( i > 0 ) {
			fprintf( file, ", " );
		}
		fprintf( file, "%s", scan->input_device_array[i]->name );
	}
	fprintf( file, "\n\n" );

	fprintf( file, "  Settling time         = %g seconds\n\n",
					scan->settling_time );

	if ( record->mx_type != MXS_XAF_STANDARD ) {
		fprintf( file, "  Measurement type      = %s\n",
						scan->measurement_type );
	
		switch( scan->measurement.type ) {
		case MXM_PRESET_TIME:
			sprintf(format, "%%lg %%%ds", MXU_RECORD_NAME_LENGTH);
	
			num_items = sscanf(scan->measurement_arguments, format,
					&measurement_time, device_name );
	
			if ( num_items != 2 ) {
				return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Cannot parse measurement arguments '%s' for scan '%s'.",
					scan->measurement_arguments,
					record->name );
			}
			fprintf( file,
		"  Measurement time      = %g seconds ( Timer '%s' )\n",
					measurement_time, device_name );
			break;
	
		case MXM_PRESET_COUNT:
			sprintf(format, "%%ld %%%ds", MXU_RECORD_NAME_LENGTH);
	
			num_items = sscanf(scan->measurement_arguments, format,
					&measurement_counts, device_name );
	
			if ( num_items != 2 ) {
				return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Cannot parse measurement arguments '%s' for scan '%s'.",
					scan->measurement_arguments,
					record->name );
			}
			fprintf( file,
		"  Measurement preset    = %ld counts ( Scaler '%s' )\n",
					measurement_counts, device_name );
			break;

		case MXM_PRESET_PULSE_PERIOD:
			sprintf(format, "%%lg %%%ds", MXU_RECORD_NAME_LENGTH);

			num_items = sscanf(scan->measurement_arguments, format,
					&pulse_period, device_name );
	
			if ( num_items != 2 ) {
				return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Cannot parse measurement arguments '%s' for scan '%s'.",
					scan->measurement_arguments,
					record->name );
			}
			fprintf( file,
	"  Measurement time      = %g seconds ( Pulse generator '%s' )\n",
					pulse_period, device_name );
			break;
		default:
			fprintf( file, "  Measurement arguments = '%s'\n",
				scan->measurement_arguments );
			break;
		}
		fprintf( file, "\n" );
	}

	fprintf( file, "  Datafile type         = %s\n",
					scan->datafile_description );
	fprintf( file, "  Datafile name         = %s\n", scan->datafile_name );
	fprintf( file, "  Plot type             = %s\n",
					scan->plot_description );
	fprintf( file, "  Plot arguments        = %s\n", scan->plot_arguments );
	fprintf( file, "\n" );

	return MX_SUCCESSFUL_RESULT;
}

/* --------------- */

MX_EXPORT mx_status_type
mx_perform_scan( MX_RECORD *scan_record )
{
	static const char fname[] = "mx_perform_scan()";

	MX_SCAN *scan;
	MX_SCAN_FUNCTION_LIST *scan_function_list;
	mx_status_type ( *prepare_for_scan_start_fn ) ( MX_SCAN * );
	mx_status_type ( *execute_scan_body_fn ) ( MX_SCAN * );
	mx_status_type ( *cleanup_after_scan_end_fn ) ( MX_SCAN * );
	MX_LIST_HEAD *list_head;
	MX_CLOCK_TICK start_tick, end_tick;
	MX_CLOCK_TICK scan_body_start_tick, scan_body_end_tick;
	long i;
	mx_status_type mx_status;

	MX_DEBUG( 2, ( "%s invoked.", fname ) );

	if ( mx_get_debug_level() >= 1 ) {
		start_tick = mx_current_clock_tick();
	}

	/*** Find and check a lot of pointer variables that we will need. ***/

	if ( scan_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	scan = (MX_SCAN *) scan_record->record_superclass_struct;

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_SCAN pointer for scan record '%s' is NULL.",
				scan_record->name );
	}

	scan_function_list = (MX_SCAN_FUNCTION_LIST *)
			scan_record->superclass_specific_function_list;

	if ( scan_function_list == (MX_SCAN_FUNCTION_LIST *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"Scan function list pointer for scan '%s' is NULL.",
			scan_record->name );
	}

	prepare_for_scan_start_fn = scan_function_list->prepare_for_scan_start;

	if ( prepare_for_scan_start_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"prepare_for_scan_start function pointer for scan '%s' is NULL.",
			scan_record->name );
	}

	execute_scan_body_fn = scan_function_list->execute_scan_body;

	if ( execute_scan_body_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"execute_scan_body function pointer for scan '%s' is NULL.",
			scan_record->name );
	}

	cleanup_after_scan_end_fn = scan_function_list->cleanup_after_scan_end;

	if ( cleanup_after_scan_end_fn == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"cleanup_after_scan_end function pointer for scan '%s' is NULL.",
			scan_record->name );
	}

	list_head = mx_get_record_list_head_struct( scan_record );

	if ( list_head == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_LIST_HEAD pointer for scan record '%s' is NULL.",
			scan_record->name );
	}

	/*** If the scan uses records that do not exist, abort now. */

#define MXP_SCAN_MAX_NAMES	5

	if ( scan->missing_record_array != NULL ) {
		char name_buffer[1000];
		long max_names_listed;
		size_t name_buffer_used_length;

		if ( scan->num_missing_records == 1 ) {
			return mx_error( MXE_NOT_FOUND, fname,
			"Motor '%s' used by scan '%s' does not exist.  "
			"Scan aborted.",
				scan->missing_record_array[0]->name,
				scan->record->name );
		}

		if ( scan->num_missing_records > MXP_SCAN_MAX_NAMES ) {
			max_names_listed = MXP_SCAN_MAX_NAMES;
		} else {
			max_names_listed = scan->num_missing_records;
		}

		strcpy( name_buffer, "" );

		for ( i = 0; i < max_names_listed; i++ ) {
			name_buffer_used_length = strlen( name_buffer );

			strncat( name_buffer,
				scan->missing_record_array[i]->name,
				name_buffer_used_length - 7 );

			if ( i < (max_names_listed - 1) ) {
				strncat( name_buffer,
					", ", name_buffer_used_length );

			} else {
				if ( max_names_listed <= MXP_SCAN_MAX_NAMES ) {
					strncat( name_buffer,
					    " ", name_buffer_used_length );
				} else {
					strncat( name_buffer,
					    ", ... ",
					    	name_buffer_used_length );
				}
			}
		}

		return mx_error( MXE_NOT_FOUND, fname,
			"Motors '%s' used by scan '%s' do not exist.  "
			"Scan aborted.",  name_buffer, scan->record->name );
	}

	/*** If this is a quick scan, save the motor speeds ***/

	if ( scan_record->mx_class == MXS_QUICK_SCAN ) {
		mx_status = mx_scan_save_speeds( scan );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/*** Record that the scan has started. ***/

	if ( list_head->log_handler != NULL ) {
		mx_log_scan_start( list_head, scan );
	}

	/*** Configure the measurement type. ***/

	mx_status = mx_configure_measurement_type( &(scan->measurement) );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_handle_abnormal_scan_termination(
				list_head, scan, mx_status );
		return mx_status;
	}

	/*** Loop through the iterations of the scan. ***/

	for ( i = 0; i < scan->num_scans; i++ ) {

		scan->current_scan_number = i;

		if ( scan->num_scans > 1 ) {
			mx_info( "Starting scan %ld of %ld.",
					i+1, scan->num_scans );
		}

		mx_status = prepare_for_scan_start_fn( scan );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_handle_abnormal_scan_termination(
					list_head, scan, mx_status );
			return mx_status;
		}

		/* Perform any measurement type specific processing required
		 * before the start of the scan.
		 */

		mx_status = mx_prescan_measurement_processing(
						&(scan->measurement) );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_handle_abnormal_scan_termination(
					list_head, scan, mx_status );
			return mx_status;
		}

		/*** Open the shutter if requested. ***/

		if ( scan->shutter_policy == MXF_SCAN_SHUTTER_OPEN_FOR_SCAN ) {
			mx_status = mx_relay_command( scan->shutter_record,
						MXF_OPEN_RELAY );

			if ( mx_status.code != MXE_SUCCESS ) {
				mx_handle_abnormal_scan_termination(
						list_head, scan, mx_status );
				return mx_status;
			}
		}

		/*** Perform the scan. ***/

		if ( mx_get_debug_level() >= 1 ) {
			scan_body_start_tick = mx_current_clock_tick();
		}

		scan->execute_scan_body_status = execute_scan_body_fn( scan );

		if ( mx_get_debug_level() >= 1 ) {
			scan_body_end_tick = mx_current_clock_tick();
		}

		/*** Close the shutter now if requested. ***/

		if ( scan->shutter_policy != MXF_SCAN_SHUTTER_IGNORE ) {
			(void) mx_relay_command( scan->shutter_record,
						MXF_CLOSE_RELAY );
		}

		/* Perform any measurement type specific processing required
		 * after the end of the scan.
		 */

		mx_status = mx_postscan_measurement_processing(
						&(scan->measurement) );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_handle_abnormal_scan_termination(
					list_head, scan, mx_status );
			return mx_status;
		}

		/* Check the status of the scan that just completed. */

		switch( scan->execute_scan_body_status.code ) {
		case MXE_SUCCESS:
			mx_info( "Scan '%s' complete.", scan_record->name );
			break;
		case MXE_STOP_REQUESTED:
			mx_info( "Scan '%s' stopped.", scan_record->name );
			break;
		case MXE_INTERRUPTED:
			mx_info( "Scan '%s' aborted.", scan_record->name );
			break;
		default:
			mx_info( "Scan '%s' terminated with error code %ld.",
				scan_record->name,
				scan->execute_scan_body_status.code );
			break;
		}

		/*
		 * Perform any post scan actions such as reading out MCSs, etc.
		 */

		mx_status = cleanup_after_scan_end_fn( scan );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_handle_abnormal_scan_termination(
					list_head, scan, mx_status );
			return mx_status;
		}

		/* If this is a quick scan, restore the motor speeds. */

		if ( scan_record->mx_class == MXS_QUICK_SCAN ) {
			mx_status = mx_scan_restore_speeds( scan );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	}

	/* Deconfigure the measurement type. */

	(void) mx_deconfigure_measurement_type( &(scan->measurement) );

	/* Record that the scan has ended. */

	if ( list_head->log_handler != NULL ) {
		mx_log_scan_end( list_head, scan,
					scan->execute_scan_body_status );
	}

	if ( mx_get_debug_level() >= 1 ) {

		MX_CLOCK_TICK prepare_for_scan_start_ticks;
		double prepare_for_scan_start_time;

		MX_CLOCK_TICK scan_body_ticks;
		double scan_body_time;

		MX_CLOCK_TICK cleanup_after_scan_end_ticks;
		double cleanup_after_scan_end_time;

		MX_CLOCK_TICK total_scan_ticks;
		double total_scan_time;

		end_tick = mx_current_clock_tick();

		prepare_for_scan_start_ticks = mx_subtract_clock_ticks(
						scan_body_start_tick,
						start_tick );

		prepare_for_scan_start_time = mx_convert_clock_ticks_to_seconds(
			prepare_for_scan_start_ticks );

		MX_DEBUG(-2,("%s: preparing for scan start = %g seconds",
			fname, prepare_for_scan_start_time));

		scan_body_ticks = mx_subtract_clock_ticks(
						scan_body_end_tick,
						scan_body_start_tick );

		scan_body_time = mx_convert_clock_ticks_to_seconds(
			scan_body_ticks );

		MX_DEBUG(-2,("%s: scan body                = %g seconds",
			fname, scan_body_time));

		cleanup_after_scan_end_ticks = mx_subtract_clock_ticks(
						end_tick,
						scan_body_end_tick );

		cleanup_after_scan_end_time = mx_convert_clock_ticks_to_seconds(
			cleanup_after_scan_end_ticks );

		MX_DEBUG(-2,("%s: cleanup after scan end   = %g seconds",
			fname, cleanup_after_scan_end_time));

		total_scan_ticks = mx_subtract_clock_ticks(
						end_tick, start_tick );

		total_scan_time = mx_convert_clock_ticks_to_seconds(
			total_scan_ticks );

		MX_DEBUG(-2,("%s: total scan time          = %g seconds",
			fname, total_scan_time));
	}

	return MX_SUCCESSFUL_RESULT;
}

/* --------------- */

MX_EXPORT mx_status_type
mx_scan_set_custom_datafile_handler( MX_RECORD *scan_record,
				void *custom_args,
				MX_DATAFILE_FUNCTION_LIST *custom_flist )
{
	static const char fname[] = "mx_scan_set_custom_datafile_handler()";

	MX_DATAFILE_CUSTOM *custom_datafile;
	MX_SCAN *scan;

	if ( scan_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}
	if ( scan_record->mx_superclass != MXR_SCAN ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Record '%s' is not a scan record.",
			scan_record->name );
	}

	scan = (MX_SCAN *) scan_record->record_superclass_struct;

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SCAN pointer for scan '%s' is NULL.",
			scan_record->name );
	}

	if ( scan->datafile.datafile_type_struct != NULL ) {
		mx_free( scan->datafile.datafile_type_struct );
	}

	custom_datafile = (MX_DATAFILE_CUSTOM *)
			calloc( 1, sizeof(MX_DATAFILE_CUSTOM) );

	if ( custom_datafile == (MX_DATAFILE_CUSTOM *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate "
			"an MX_DATAFILE_CUSTOM structure." );
	}

	custom_datafile->custom_args = custom_args;

	custom_datafile->custom_function_list = custom_flist;

	scan->datafile.datafile_type_struct = custom_datafile;

	return MX_SUCCESSFUL_RESULT;
}

/* --------------- */

MX_EXPORT mx_status_type
mx_scan_set_custom_plot_handler( MX_RECORD *scan_record,
				void *custom_args,
				MX_PLOT_FUNCTION_LIST *custom_flist )
{
	static const char fname[] = "mx_scan_set_custom_plot_handler()";

	MX_PLOT_CUSTOM *custom_plot;
	MX_SCAN *scan;

	if ( scan_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}
	if ( scan_record->mx_superclass != MXR_SCAN ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Record '%s' is not a scan record.",
			scan_record->name );
	}

	scan = (MX_SCAN *) scan_record->record_superclass_struct;

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SCAN pointer for scan '%s' is NULL.",
			scan_record->name );
	}

	if ( scan->plot.plot_type_struct != NULL ) {
		mx_free( scan->plot.plot_type_struct );
	}

	custom_plot = (MX_PLOT_CUSTOM *) calloc( 1, sizeof(MX_PLOT_CUSTOM) );

	if ( custom_plot == (MX_PLOT_CUSTOM *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Ran out of memory trying to allocate "
			"an MX_PLOT_CUSTOM structure." );
	}

	custom_plot->custom_args = custom_args;

	custom_plot->custom_function_list = custom_flist;

	scan->plot.plot_type_struct = custom_plot;

	return MX_SUCCESSFUL_RESULT;
}

/* --------------- */

MX_EXPORT mx_status_type
mx_standard_prepare_for_scan_start( MX_SCAN *scan )
{
	static const char fname[] = "mx_standard_prepare_for_scan_start()";

	mx_status_type mx_status;

	MX_DEBUG( 2, ( "%s invoked.", fname ) );

	scan->measurement_number = 0L;

	/* ==== Setup a scan shutter if requested. ==== */

	mx_status = mx_setup_scan_shutter( scan );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* ==== Setup scan enable/disable support if requested. ==== */

	mx_status = mx_setup_measurement_permit_and_fault_handlers( scan );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* ==== Try to open the datafile. ==== */

	mx_status = mx_datafile_open( &(scan->datafile) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* ==== Initialize graphical plotting of scan results. ==== */

	if ( mx_plotting_is_enabled( scan->record ) ) {

		mx_status = mx_plot_open( &(scan->plot) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		scan->plot.section_number = 0;
	}

	/* ==== Write out the main scan header. ==== */

	mx_status = mx_datafile_write_main_header( &(scan->datafile) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Make sure all of the dark current values are up to date
	 * before starting the scan.
	 */

	mx_status = mx_update_dark_currents( scan );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0	/* The new policy is that fast mode should be turned on between
	 * the first and second steps of a step scan, rather than before
	 * the first step.  It should not be turned on for MCS quick
	 * scans at all.
	 */

	/* ==== Put all MX servers into fast mode. ==== */

	mx_status = mx_set_fast_mode( scan->record, TRUE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;
#endif

	return MX_SUCCESSFUL_RESULT;
}

/* --------------- */

MX_EXPORT mx_status_type
mx_standard_cleanup_after_scan_end( MX_SCAN *scan )
{
	static const char fname[] = "mx_standard_cleanup_after_scan_end()";

	MX_LIST_HEAD *list_head;
	int enable_flag, prompt_for_plot_close;
	mx_status_type datafile_close_status;

	MX_DEBUG( 2, ( "%s invoked.", fname ) );

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"Scan routine was passed a NULL pointer.");
	}

	list_head = mx_get_record_list_head_struct( scan->record );

	if ( list_head == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_LIST_HEAD pointer for scan record '%s' is NULL.",
			scan->record->name );
	}

	/* Make sure the datafile is closed. */

	datafile_close_status = mx_datafile_close( &(scan->datafile) );

	/* Turn off fast mode in the MX servers in case it was set. */

	(void) mx_set_fast_mode( scan->record, FALSE );

	/* Free unneeded handlers. */

	mx_scan_free_measurement_permit_and_fault_handlers( scan );

	/* Handle the user's final interactions with the plot. */

	enable_flag = mx_plotting_is_enabled( scan->record );

	prompt_for_plot_close = TRUE;

	switch ( enable_flag ) {
	case MXPF_PLOT_OFF:
	case MXPF_PLOT_NOWAIT:
		prompt_for_plot_close = FALSE;

		break;
	default:
		if (scan->plot.type == MXP_CHILD
		 || scan->plot.type == MXP_NONE ) {

			prompt_for_plot_close = FALSE;

		} else if ( scan->execute_scan_body_status.code != MXE_SUCCESS )
		{
			prompt_for_plot_close = TRUE;

		} else if ( scan->num_scans - scan->current_scan_number > 1 ) {

			prompt_for_plot_close = FALSE;
		} else {
			prompt_for_plot_close = TRUE;
		}
		break;
	}

	if ( prompt_for_plot_close ) {

			mx_info_dialog(
	"\aHit any key to close the plot...",
	"Scan Complete.  Click the OK button to close the plot", "OK");

	} else {
		/* Give the plotting program a little time to finish
		 * plotting the last data point.
		 */

		mx_msleep(500);
	}

	if ( enable_flag != MXPF_PLOT_OFF ) {
		(void) mx_plot_close( &(scan->plot) );
	}

	if ( scan->execute_scan_body_status.code != MXE_SUCCESS ) {
		return scan->execute_scan_body_status;
	}
	if ( datafile_close_status.code != MXE_SUCCESS ) {
		return datafile_close_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

/* --------------- */

/* This function saves motor speeds and synchronous motion mode flags. */

MX_EXPORT mx_status_type
mx_scan_save_speeds( MX_SCAN *scan )
{
	static const char fname[] = "mx_scan_save_speeds()";

	MX_RECORD *motor_record;
	long i;
	mx_status_type mx_status;

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SCAN pointer passed was NULL." );
	}

	for ( i = 0; i < scan->num_motors; i++ ) {

		motor_record = (scan->motor_record_array)[i];

		mx_status = mx_motor_save_speed( motor_record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

/* This function restores motor speeds and synchronous motion mode flags. */

MX_EXPORT mx_status_type
mx_scan_restore_speeds( MX_SCAN *scan )
{
	static const char fname[] = "mx_scan_restore_speeds()";

	MX_RECORD *motor_record;
	long i;

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SCAN pointer passed was NULL." );
	}

	for ( i = 0; i < scan->num_motors; i++ ) {

		motor_record = (scan->motor_record_array)[i];

		/* Errors in restoring the motor speed are ignored. */

		(void) mx_motor_restore_speed( motor_record );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* --------------- */

static mx_status_type
mx_setup_scan_shutter( MX_SCAN *scan )
{
	static const char fname[] = "mx_setup_scan_shutter()";

	MX_RECORD *shutter_policy_record;
	char shutter_record_name[MXU_RECORD_NAME_LENGTH + 1];

	char format_buffer[20];
	char buffer[80];
	int num_items;
	mx_status_type mx_status;

	MX_DEBUG( 2, ( "%s invoked.", fname ) );

	/* ==== Setup a scan shutter if requested. ==== */

	shutter_policy_record = mx_get_record( scan->record,
					MX_SCAN_SHUTTER_POLICY_RECORD_NAME );

	if ( shutter_policy_record == NULL ) {
		scan->shutter_policy = MXF_SCAN_SHUTTER_IGNORE;
		scan->shutter_record = NULL;
	} else {
		mx_status = mx_get_string_variable( shutter_policy_record,
						buffer, sizeof(buffer) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		sprintf( format_buffer, "%%ld %%%ds", MXU_RECORD_NAME_LENGTH );

		num_items = sscanf( buffer, format_buffer,
					&(scan->shutter_policy),
					shutter_record_name );

		if ( num_items != 2 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The shutter policy control record '%s' is not correctly formatted.",
				MX_SCAN_SHUTTER_POLICY_RECORD_NAME );
		}

		/* Check that the shutter policy has a valid value. */

		switch( scan->shutter_policy ) {
		case MXF_SCAN_SHUTTER_IGNORE:
		case MXF_SCAN_SHUTTER_OPEN_FOR_SCAN:
		case MXF_SCAN_SHUTTER_OPEN_FOR_DATAPOINT:
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The '%s' record is set to an illegal value of %ld.  "
"Allowed values are 0=ignore, 1=open for scan, 2=open for datapoint",
				MX_SCAN_SHUTTER_POLICY_RECORD_NAME,
				scan->shutter_policy );
		}

		/* Find the shutter record. */

		scan->shutter_record = mx_get_record( scan->record,
						shutter_record_name );

		if ( scan->shutter_record == NULL ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The shutter '%s' specified in the shutter policy control record '%s' "
"does not exist.", shutter_record_name, MX_SCAN_SHUTTER_POLICY_RECORD_NAME );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* --------------- */

static mx_status_type
mx_update_dark_currents( MX_SCAN *scan )
{
	static const char fname[] = "mx_update_dark_currents()";

	MX_RECORD *device_record;
	MX_SCALER *scaler;
	MX_ANALOG_INPUT *ainput;
	long i, scaler_mask, ainput_mask;
	double dark_current;
	mx_status_type mx_status;

	scaler_mask = MXF_SCL_SUBTRACT_DARK_CURRENT
			| MXF_SCL_SERVER_SUBTRACTS_DARK_CURRENT;

	ainput_mask = MXF_AIN_SUBTRACT_DARK_CURRENT
			| MXF_AIN_SERVER_SUBTRACTS_DARK_CURRENT;

	/* Walk through the list of input devices and update the
	 * dark current for records that need it.
	 */

	for ( i = 0; i < scan->num_input_devices; i++ ) {
		device_record = scan->input_device_array[i];

		switch( device_record->mx_class ) {
		case MXC_SCALER:
			scaler = (MX_SCALER *)
					device_record->record_class_struct;

			if ( scaler->scaler_flags & scaler_mask ) {
				mx_status = mx_scaler_get_dark_current(
						device_record, &dark_current );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

				MX_DEBUG( 2,
				("%s: scaler '%s' dark current = %g",
				    fname, device_record->name, dark_current));
			}
			break;
		case MXC_ANALOG_INPUT:
			ainput = (MX_ANALOG_INPUT *)
					device_record->record_class_struct;

			if ( ainput->analog_input_flags & ainput_mask ) {
				mx_status = mx_analog_input_get_dark_current(
						device_record, &dark_current );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

				MX_DEBUG( 2,
				("%s: analog input '%s' dark current = %g",
				    fname, device_record->name, dark_current));
			}
			break;

		/* Other device classes are skipped. */
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* --------------- */

static mx_status_type
mx_get_scan_handler_name_array( MX_SCAN *scan, char *list_record_name,
			long *num_handlers, char ***handler_string_array )
{
	static const char fname[] = "mx_get_scan_handler_name_array()";

	MX_RECORD *list_record;
	long num_dimensions, field_type;
	long *dimension_array;
	void *pointer_to_value;
	mx_status_type mx_status;

	*num_handlers = 0;

	list_record = mx_get_record( scan->record, list_record_name );

	if ( list_record == (MX_RECORD *) NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

	mx_status = mx_get_variable_parameters( list_record,
					&num_dimensions,
					&dimension_array,
					&field_type,
					&pointer_to_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( field_type != MXFT_STRING ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
"The measurement handler list '%s' is not a 2 dimensional string variable.",
			list_record_name );
	}
	if ( num_dimensions != 2 ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
"The measurement handler list '%s' is not a 2 dimensional string variable.  "
"Instead it has %ld dimensions.", list_record_name, num_dimensions );
	}
	if ( dimension_array[0] <= 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"The measurement handler list '%s' has an illegal first dimension size of %ld."
"  This size must be greater than zero.", list_record_name, dimension_array[0]);
	}

	*num_handlers = dimension_array[0];

	*handler_string_array = (char **) pointer_to_value;

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_setup_measurement_permit_and_fault_handlers( MX_SCAN *scan )
{
	static const char fname[] = "mx_setup_measurement_permit_and_fault_handlers()";

	MX_MEASUREMENT_PERMIT *permit_handler;
	MX_MEASUREMENT_FAULT *fault_handler;
	char **handler_string_array;
	long i, num_permit_entries, num_fault_entries;
	mx_status_type mx_status;

	scan->num_measurement_permit_handlers = 0;
	scan->num_measurement_fault_handlers = 0;

	/* Setup the permit handlers. */

	mx_status = mx_get_scan_handler_name_array( scan,
				MX_SCAN_PERMIT_HANDLER_LIST,
				&num_permit_entries,
				&handler_string_array );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_permit_entries > 0 ) {

		scan->measurement_permit_handler_array =
			(MX_MEASUREMENT_PERMIT **) malloc( num_permit_entries
					* sizeof( MX_MEASUREMENT_PERMIT * ) );

		if ( scan->measurement_permit_handler_array == NULL ) {
			return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Could not allocate %ld MX_MEASUREMENT_PERMIT handlers for scan '%s'.",
			num_permit_entries, scan->record->name );
		}
	}

	for ( i = 0; i < num_permit_entries; i++ ) {

		mx_status = mx_measurement_permit_create_handler(
				&permit_handler, scan,
				handler_string_array[i] );

		if ( mx_status.code != MXE_SUCCESS ) {
		    mx_scan_free_measurement_permit_and_fault_handlers( scan );

		    return mx_status;
		}

		/* If mx_measurement_permit_create_handler() returns a NULL
		 * pointer, that means that this scan does not need to use
		 * this permit handler.  Otherwise, add the returned handler
		 * to the array.
		 */

		if ( permit_handler != (MX_MEASUREMENT_PERMIT *) NULL ) {

			scan->measurement_permit_handler_array[ 
				scan->num_measurement_permit_handlers ] 
							= permit_handler;

			(scan->num_measurement_permit_handlers)++;
		}
	}

	/* Setup the fault handlers. */

	mx_status = mx_get_scan_handler_name_array( scan,
				MX_SCAN_FAULT_HANDLER_LIST,
				&num_fault_entries,
				&handler_string_array );

	if ( mx_status.code != MXE_SUCCESS ) {
		mx_scan_free_measurement_permit_and_fault_handlers( scan );

		return mx_status;
	}

	if ( num_fault_entries > 0 ) {

		scan->measurement_fault_handler_array =
			(MX_MEASUREMENT_FAULT **) malloc( num_fault_entries
					* sizeof( MX_MEASUREMENT_FAULT * ) );

		if ( scan->measurement_fault_handler_array == NULL ) {
		    mx_scan_free_measurement_permit_and_fault_handlers( scan );

		    return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Could not allocate %ld MX_MEASUREMENT_FAULT handlers for scan '%s'.",
			num_fault_entries, scan->record->name );
		}
	}

	for ( i = 0; i < num_fault_entries; i++ ) {

		mx_status = mx_measurement_fault_create_handler(
				&fault_handler, scan,
				handler_string_array[i] );

		if ( mx_status.code != MXE_SUCCESS ) {
		    mx_scan_free_measurement_permit_and_fault_handlers( scan );

		    return mx_status;
		}

		/* If mx_measurement_fault_create_handler() returns a NULL
		 * pointer, that means that this scan does not need to use
		 * this fault handler.  Otherwise, add the returned handler
		 * to the array.
		 */

		if ( fault_handler != (MX_MEASUREMENT_FAULT *) NULL ) {

			scan->measurement_fault_handler_array[ 
				scan->num_measurement_fault_handlers ] 
							= fault_handler;

			(scan->num_measurement_fault_handlers)++;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mx_scan_free_measurement_permit_and_fault_handlers( MX_SCAN *scan )
{
	long i;

	if ( scan->measurement_permit_handler_array != NULL ) {

		for ( i = 0; i < scan->num_measurement_permit_handlers; i++ ) {

			(void) mx_measurement_permit_destroy_handler(
				scan->measurement_permit_handler_array[i] );

			scan->measurement_permit_handler_array[i] = NULL;
		}

		free( scan->measurement_permit_handler_array );

		scan->measurement_permit_handler_array = NULL;
	}

	if ( scan->measurement_fault_handler_array != NULL ) {

		for ( i = 0; i < scan->num_measurement_fault_handlers; i++ ) {

			(void) mx_measurement_fault_destroy_handler(
				scan->measurement_fault_handler_array[i] );

			scan->measurement_fault_handler_array[i] = NULL;
		}

		free( scan->measurement_fault_handler_array );

		scan->measurement_fault_handler_array = NULL;
	}

	return MX_SUCCESSFUL_RESULT;
}

/* --------------- */

MX_EXPORT mx_status_type
mx_scan_wait_for_all_permits( MX_SCAN *scan )
{
	static const char fname[] = "mx_scan_wait_for_all_permits()";

	long i;
	mx_status_type mx_status;

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SCAN pointer passed was NULL." );
	}

	for ( i = 0; i < scan->num_measurement_permit_handlers; i++ ) {

		mx_status = mx_measurement_permit_wait_for_permission(
				scan->measurement_permit_handler_array[i] );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Put in a tiny sleep between tests. */

		mx_msleep(1);
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_scan_check_for_all_faults( MX_SCAN *scan, mx_bool_type *fault_occurred )
{
	static const char fname[] = "mx_scan_check_for_all_faults()";

	long i;
	int fault_status;
	mx_status_type mx_status;

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SCAN pointer passed was NULL." );
	}
	if ( fault_occurred == (mx_bool_type *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The fault_occurred pointer passed was NULL." );
	}

	*fault_occurred = FALSE;

	for ( i = 0; i < scan->num_measurement_fault_handlers; i++ ) {

		mx_status = mx_measurement_fault_check_for_fault(
			scan->measurement_fault_handler_array[i],
			&fault_status );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( fault_status == TRUE ) {

			*fault_occurred = TRUE;

			/* If a measurement fault occurred, reset it.*/

			mx_status = mx_measurement_fault_reset(
			    scan->measurement_fault_handler_array[i],
				MXMF_NONE );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}

		/* Put in a tiny sleep between tests. */

		mx_msleep(1);
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_scan_reset_all_faults( MX_SCAN *scan )
{
	static const char fname[] = "mx_scan_reset_all_faults()";

	long i;
	mx_status_type mx_status;

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SCAN pointer passed was NULL." );
	}

	for ( i = 0; i < scan->num_measurement_fault_handlers; i++ ) {

		mx_status = mx_measurement_fault_reset(
				scan->measurement_fault_handler_array[i],
				MXMF_PREPARE_FOR_FIRST_MEASUREMENT_ATTEMPT );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

/* --------------- */

MX_EXPORT mx_status_type
mx_scan_acquire_and_readout_data( MX_SCAN *scan )
{
	static const char fname[] = "mx_scan_acquire_and_readout_data()";

	int interrupt;
	mx_bool_type fault_occurred;
	mx_status_type mx_status, acquire_data_status;

	acquire_data_status = MX_SUCCESSFUL_RESULT;

	/* Reset any faults that have occurred during the interval since
	 * the last measurement.
	 */

	mx_status = mx_scan_reset_all_faults( scan );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/*** Begin the measurement fault handling loop ***/

	fault_occurred = TRUE;

	while ( fault_occurred == TRUE ) {

		MX_DEBUG( 2,("%s: beginning of fault_occurred loop.", fname ));

		interrupt = mx_user_requested_interrupt_or_pause();

		switch ( interrupt ) {
		case MXF_USER_INT_NONE:
			break;

		case MXF_USER_INT_PAUSE:
			return mx_error( MXE_PAUSE_REQUESTED, fname,
				"Pause requested by user." );
			break;

		default:
			return mx_error( MXE_INTERRUPTED, fname,
				"The measurement was interrupted." );
		}

		/* Wait until all of the measurement permit handlers say
		 * that it is OK to take the measurement.
		 */

		mx_status = mx_scan_wait_for_all_permits( scan );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Perform the measurement. */

		acquire_data_status = mx_acquire_data( &(scan->measurement) );

		if ( acquire_data_status.code != MXE_SUCCESS )
			return acquire_data_status;

		mx_status = mx_readout_data( &(scan->measurement) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Did something bad happen during the measurement? */

		mx_status = mx_scan_check_for_all_faults(scan, &fault_occurred);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		MX_DEBUG( 2,("%s: end of fault_occurred loop.", fname ));

		if ( fault_occurred ) {
			mx_info(
"The measurement just performed was bad.  Retrying the measurement." );
		}

	}    /* End of measurement fault handling loop ***/

	return acquire_data_status;
}

/* --------------- */

MX_EXPORT mx_status_type
mx_scan_acquire_data( MX_SCAN *scan )
{
	static const char fname[] = "mx_scan_acquire_data()";

	mx_status_type mx_status;

	/* Reset any faults that have occurred during the interval since
	 * the last measurement.
	 */

	mx_status = mx_scan_reset_all_faults( scan );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* This version of the code does not and cannot have a fault loop. */

	if ( mx_user_requested_interrupt() ) {
		return mx_error( MXE_INTERRUPTED, fname,
		"%s was interrupted.", fname );
	}

	/* Wait until all of the measurement permit handlers say
	 * that it is OK to take the measurement.
	 */

	mx_status = mx_scan_wait_for_all_permits( scan );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Perform the measurement. */

	mx_status = mx_acquire_data( &(scan->measurement) );

	return mx_status;
}

/* --------------- */

static mx_status_type
mxp_scan_update_motor_destination( MX_RECORD *motor_record )
{
	static const char fname[] = "mxp_scan_update_motor_destination()";

	MX_MOTOR *motor;
	MX_RECORD *real_motor_record;
	unsigned long flags;
	mx_bool_type do_recursion;
	mx_status_type mx_status;

	motor = motor_record->record_class_struct;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_MOTOR pointer for motor '%s' is NULL.",
			motor_record->name );
	}

	motor->old_destination = motor->destination;

	MX_DEBUG( 2,("%s: Set motor '%s' old destination to %g",
		fname, motor_record->name, motor->old_destination));

	flags = motor->motor_flags;

	real_motor_record = motor->real_motor_record;

	do_recursion = FALSE;

	if ( flags & MXF_MTR_IS_PSEUDOMOTOR ) {
		do_recursion = TRUE;
	}
	if ( flags & MXF_MTR_IS_REMOTE_MOTOR ) {
		do_recursion = FALSE;
	}
	if ( flags & MXF_MTR_PSEUDOMOTOR_RECURSION_IS_NOT_NECESSARY ) {
		do_recursion = FALSE;
	}
	if ( real_motor_record == (MX_RECORD *) NULL ) {
		do_recursion = FALSE;
	}

	if ( do_recursion ) {
		mx_status =
			mxp_scan_update_motor_destination( real_motor_record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

/* After the last step of a scan with the 'early move' flag set, we must
 * update the values of the motor->old_destination fields with the value
 * from motor->destination so that the last step of the scan will have the
 * correct value written to the data file and the plot.
 */

MX_EXPORT mx_status_type
mx_scan_update_old_destinations( MX_SCAN *scan )
{
	static const char fname[] = "mx_scan_update_old_destinations()";

	MX_RECORD **motor_record_array;
	MX_RECORD *motor_record;
	long i;
	mx_bool_type early_move_flag;
	mx_status_type mx_status;

	/* It is an error to call this function if the early move flag
	 * is not set.
	 */

	mx_status = mx_scan_get_early_move_flag( scan, &early_move_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( early_move_flag == FALSE ) {
		return mx_error( MXE_NOT_VALID_FOR_CURRENT_STATE, fname,
		"Scan '%s' attempted to update the old motor destination "
		"values when the early move flag was not set.  "
		"This is not allowed.",  scan->record->name );
	}

	/* If the scan does not use motors, then there is nothing to update. */

	if ( scan->num_motors == 0 ) {
		return MX_SUCCESSFUL_RESULT;
	}

	motor_record_array = scan->motor_record_array;

	if ( motor_record_array == (MX_RECORD **) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The motor_record_array pointer for scan '%s' is NULL.",
			scan->record->name );
	}

	for ( i = 0; i < scan->num_motors; i++ ) {
		if ( scan->motor_is_independent_variable[i] ) {
			motor_record = motor_record_array[i];

			if ( motor_record == (MX_RECORD *) NULL ) {
				return mx_error(
					MXE_CORRUPT_DATA_STRUCTURE, fname,
				"The MX_RECORD pointer for motor %ld used "
				"by scan '%s' is NULL.", i, scan->record->name);
			}

			mx_status =
			    mxp_scan_update_motor_destination( motor_record );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* --------------- */

MX_EXPORT void
mx_set_scan_pause_request_handler(
	mx_status_type (*pause_request_handler)( MX_SCAN * ) )
{
	if ( pause_request_handler == NULL ) {
		mx_scan_pause_request_handler = mx_default_scan_pause_request_handler;
	} else {
		mx_scan_pause_request_handler = pause_request_handler;
	}
	return;
}

MX_EXPORT mx_status_type
mx_default_scan_pause_request_handler( MX_SCAN *scan )
{
	static const char fname[] = "mx_default_scan_pause_request_handler()";

	MX_DEBUG(-2,("%s invoked.", fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_scan_handle_pause_request( MX_SCAN *scan )
{
	static const char fname[] = "mx_scan_handle_pause_request()";

	long i;
	mx_status_type mx_status, mx_status2;

	if ( mx_scan_pause_request_handler != NULL ) {

		mx_status = ( *mx_scan_pause_request_handler )( scan );
	} else {
		return mx_error( MXE_NULL_ARGUMENT, fname,
"The mx_scan_pause_request_handler pointer is NULL.  This shouldn't happen." );
	}

	switch( mx_status.code ) {
	case MXE_SUCCESS:
		mx_info("Retrying the last scan step.");

		/* Pass control back to the caller so that the caller
		 * can recover from the pause.
		 */

		break;
	case MXE_STOP_REQUESTED:
		mx_info( "Waiting for the motors to stop moving.");

		mx_status2 = mx_wait_for_motor_array_stop(
				scan->num_motors, scan->motor_record_array,
			( MXF_MTR_SCAN_IN_PROGRESS | MXF_MTR_IGNORE_PAUSE ) );

		if ( mx_status2.code != MXE_SUCCESS )
			return mx_status2;
		break;
	case MXE_INTERRUPTED:
		mx_info( "Aborting current motor moves.");

		for ( i = 0; i < scan->num_motors; i++ ) {
			(void) mx_motor_soft_abort(scan->motor_record_array[i]);
		}
		break;
	case MXE_PAUSE_REQUESTED:
		/* Ignore additional pause requests. */

		mx_status = MX_SUCCESSFUL_RESULT;
		break;
	default:
		break;
	}

	return mx_status;
}

/* --------------- */

MX_EXPORT mx_status_type
mx_scan_display_scan_progress( MX_SCAN *scan )
{
	static const char fname[] = "mx_scan_display_scan_progress()";

	MX_LINEAR_SCAN *linear_scan;
	MX_RECORD **input_device_array;
	MX_RECORD *input_device;
	MX_RECORD **motor_record_array;
	MX_RECORD *motor_record;
	MX_MOTOR *motor;
	double *motor_position;
	double normalization;
	long *motor_is_independent_variable;
	static char truncation_marker[] = " ... truncated ...";
	char buffer[2000];
	char little_buffer[50];
	size_t buffer_left, max_length, trailing_space;
	long i, num_motors, num_input_devices;
	mx_bool_type early_move_flag;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	if ( scan->scan_flags & MXF_SCAN_SUPPRESS_PROGRESS_DISPLAY ) {
		return MX_SUCCESSFUL_RESULT;
	}

	motor_position = scan->motor_position;
	motor_is_independent_variable = scan->motor_is_independent_variable;

	num_motors = scan->num_motors;
	motor_record_array = scan->motor_record_array;

	num_input_devices = scan->num_input_devices;
	input_device_array = scan->input_device_array;

	strcpy(buffer, "");

	mx_status = mx_scan_get_early_move_flag( scan, &early_move_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Leave extra space for the truncation marker at the end. */

	trailing_space = sizeof(truncation_marker) + 5;

	max_length = sizeof(buffer) - trailing_space;

	if ( scan->datafile.num_x_motors == 0 ) {

		/* By default, we use the axes being scanned. */

		if ( num_motors <= 0 ) {
			if ( scan->record->mx_class == MXS_LINEAR_SCAN ) {
				buffer_left = max_length - strlen(buffer) - 1;

				linear_scan = (MX_LINEAR_SCAN *)
					(scan->record->record_class_struct);
				sprintf( little_buffer, " %ld",
					(linear_scan->step_number)[0] );
				strncat( buffer, little_buffer, buffer_left );
			}
		} else {
			for ( i = 0; i < num_motors; i++ ) {
				buffer_left = max_length - strlen(buffer) - 1;

				if ( motor_is_independent_variable[i] ) {

				    if ( early_move_flag ) {
				        motor_record = motor_record_array[i];

				        motor = (MX_MOTOR *)
					    motor_record->record_class_struct;

					sprintf( little_buffer, " %-.3f",
							motor->old_destination);
					strncat( buffer, little_buffer,
							buffer_left );
				    } else {
					sprintf( little_buffer, " %-.3f",
							motor_position[i] );
					strncat( buffer, little_buffer,
							buffer_left );
				    }
				}
			}
		}
	} else {
		/* However, if alternate X axis motors have been specified,
		 * we show them instead.
		 */

		if ( scan->datafile.x_position_array == (double **) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The alternate x_position_array pointer for scan '%s' is NULL.",
				scan->record->name );
		}

		for ( i = 0; i < scan->datafile.num_x_motors; i++ ) {

			buffer_left = max_length - strlen(buffer) - 1;

			sprintf( little_buffer, " %-.3f",
				scan->datafile.x_position_array[i][0] );

			strncat( buffer, little_buffer, buffer_left );
		}
	}
		
	buffer_left = max_length - strlen(buffer) - 1;

	strncat(buffer, " - ", buffer_left);

	/* If we were requested to normalize the data in the data file,
	 * the 'normalization' variable is set to the scan measurement time.
	 * Otherwise, 'normalization' is set to -1.0.
	 */

	if ( scan->datafile.normalize_data ) {
		normalization = mx_scan_get_measurement_time( scan );
	} else {
		normalization = -1.0;
	}

	for ( i = 0; i < num_input_devices; i++ ) {

		if ( buffer_left <= 1 ) {
			break;			/* Exit the for() loop. */
		}

		input_device = input_device_array[i];

		mx_status = mx_convert_normalized_device_value_to_string(
				input_device, normalization,
				little_buffer, sizeof(little_buffer) - 1);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		buffer_left = max_length - strlen(buffer) - 1;

		strncat(buffer, little_buffer, buffer_left);

		buffer_left = max_length - strlen(buffer) - 1;

		strncat(buffer, " ", buffer_left);
	} 

	if ( ( sizeof(buffer) - strlen(buffer) ) < (trailing_space + 2) ) {
		/* If the printed string was close to filling the
		 * output buffer, append a truncation marker to the
		 * buffer.
		 */

		buffer_left = sizeof(buffer) - strlen(buffer) - 1;

		strncat(buffer, truncation_marker, buffer_left);
	}

	/* Send the formatted progress string to the user. */

	mx_scanlog_info(buffer);

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_clear_scan_input_devices( MX_SCAN *scan )
{
	static const char fname[] = "mx_clear_scan_input_devices()";

	MX_RECORD *input_device;
	long i;
	mx_status_type mx_status;

#if DEBUG_CLEAR_TIMING
	MX_HRT_TIMING timing;

	MX_HRT_START( timing );
#endif

	/* ===== Clear all the input devices that need it. ===== */

	if ( scan->num_input_devices > 0 ) {
		if ( scan->input_device_array == (MX_RECORD **) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
				"Scaler array pointer is NULL.");
		}
	}

	for ( i = 0; i < scan->num_input_devices; i++ ) {

		input_device = (scan->input_device_array)[i];

		if ( input_device == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"Input device %ld pointer is NULL", i );
		}

		switch( input_device->mx_superclass ) {
		case MXR_VARIABLE:
		case MXR_SCAN:
			/* No clear operation needed. */
			break;

		case MXR_DEVICE:
			switch( input_device->mx_class ) {
			case MXC_SCALER:
				mx_status = mx_scaler_clear( input_device );

				if ( mx_status.code != MXE_SUCCESS ) {
					return mx_status;
				}
				break;

			case MXC_TIMER:
				mx_status = mx_timer_clear( input_device );

				if ( mx_status.code != MXE_SUCCESS ) {
					return mx_status;
				}
				break;

			case MXC_MULTICHANNEL_ANALYZER:
				mx_status = mx_mca_clear( input_device );

				if ( mx_status.code != MXE_SUCCESS ) {
					return mx_status;
				}
				break;

			default:
				/* All other input devices do not need
				 * to be cleared.
				 */
				break;
			}
			break;

		default:
			/* Interface and other record superclasses
			 * cannot be scan input devices.
			 */

			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Record superclass %ld for record '%s' cannot be a scan input.",
				input_device->mx_superclass,
				input_device->name );
			break;
		}
	}

#if DEBUG_CLEAR_TIMING
	MX_HRT_END( timing );

	MX_HRT_RESULTS( timing, fname, " " );
#endif
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT void
mx_log_scan_start( MX_LIST_HEAD *list_head, MX_SCAN *scan )
{
	char buffer[200];

	/* Do nothing if no log handler has been installed. */

	if ( list_head->log_handler == NULL )
		return;

	/* If this scan is a child of another scan,
	 * do not record a message.
	 */

	if ( scan->datafile.type == MXDF_CHILD )
		return;

	/* Record the fact that the scan has started. */

	sprintf( buffer, "Scan %s started", scan->record->name );

	mx_log_message( list_head->log_handler, buffer );

	return;
}

MX_EXPORT void
mx_log_scan_end( MX_LIST_HEAD *list_head, MX_SCAN *scan,
				mx_status_type mx_status )
{
	char buffer1[300];
	char buffer2[200];

	/* Do nothing if no log handler has been installed. */

	if ( list_head->log_handler == NULL )
		return;

	/* If this scan is a child of another scan,
	 * do not record a message.
	 */

	if ( scan->datafile.type == MXDF_CHILD )
		return;

	/* Record the status of the scan. */

	if ( mx_status.code == MXE_SUCCESS ) {
		sprintf( buffer1, "Scan %s ended", scan->record->name );
	} else {
		sprintf( buffer1, "Scan %s ended    %s   %s",
			scan->record->name,
			mx_strerror(mx_status.code, buffer2, sizeof buffer2),
			mx_status.message );
	}

	mx_log_message( list_head->log_handler, buffer1 );

	return;
}

MX_EXPORT void
mx_handle_abnormal_scan_termination( MX_LIST_HEAD *list_head,
				MX_SCAN *scan,
				mx_status_type mx_status )
{
	if ( scan->record->mx_class == MXS_QUICK_SCAN ) {
		(void) mx_scan_restore_speeds( scan );
	}

	(void) mx_deconfigure_measurement_type( &(scan->measurement) );

	(void) mx_scan_free_measurement_permit_and_fault_handlers( scan );

	if ( list_head->log_handler != NULL ) {
		mx_log_scan_end( list_head, scan, mx_status );
	}
}

MX_EXPORT double
mx_scan_get_measurement_time( MX_SCAN *scan )
{
	static const char fname[] = "mx_scan_get_measurement_time()";

	double measurement_time;
	int num_items;

	if ( scan == (MX_SCAN *) NULL ) {
		(void) mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SCAN pointer passed was NULL." );

		return -1.0;
	}

	if ( scan->measurement.type != MXM_PRESET_TIME ) {
		(void) mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The scan '%s' is not a preset time scan.",
			scan->record->name );

		return -1.0;
	}

	num_items = sscanf( scan->measurement_arguments, "%lg",
					&measurement_time );

	if ( num_items != 1 ) {
		(void) mx_error( MXE_UNPARSEABLE_STRING, fname,
			"The measurement time cannot be found in the "
			"measurement_arguments field '%s' for scan '%s'.",
			scan->measurement_arguments,
			scan->record->name );

		return -1.0;
	}

	return measurement_time;
}


MX_EXPORT mx_status_type
mx_scan_fixup_varargs_record_field_defaults(
		MX_RECORD_FIELD_DEFAULTS *record_field_defaults_array,
		long num_record_fields,
		long *num_independent_variables_varargs_cookie,
		long *num_motors_varargs_cookie,
		long *num_input_devices_varargs_cookie )
{
	static const char fname[] = "mx_scan_fixup_varargs_record_field_defaults()";

	MX_RECORD_FIELD_DEFAULTS *field;
	long referenced_field_index;
	mx_status_type mx_status;

	MX_DEBUG( 8,("%s invoked.", fname));

	/****
	 **** Put the 'num_motors' value in the appropriate places.
	 ****/

	mx_status = mx_find_record_field_defaults_index(
			record_field_defaults_array, num_record_fields,
			"num_motors", &referenced_field_index );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_construct_varargs_cookie(
			referenced_field_index, 0, num_motors_varargs_cookie );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 8,("%s: num_motors varargs cookie = %ld",
			fname, *num_motors_varargs_cookie));

	/*---*/

	mx_status = mx_find_record_field_defaults(
			record_field_defaults_array, num_record_fields,
			"motor_position", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = *num_motors_varargs_cookie;

	/*---*/

	mx_status = mx_find_record_field_defaults(
			record_field_defaults_array, num_record_fields,
			"motor_array", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = *num_motors_varargs_cookie;

	/*---*/

	mx_status = mx_find_record_field_defaults(
			record_field_defaults_array, num_record_fields,
			"motor_is_independent_variable", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = *num_motors_varargs_cookie;

	/****
	 **** Put the 'num_input_devices' value in the
	 **** 'input_device_array' field.
	 ****/

	mx_status = mx_find_record_field_defaults_index(
			record_field_defaults_array, num_record_fields,
			"num_input_devices", &referenced_field_index );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_construct_varargs_cookie(
			referenced_field_index, 0,
			num_input_devices_varargs_cookie );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 8,("%s: num_input_devices varargs cookie = %ld",
			fname, *num_input_devices_varargs_cookie));

	/*---*/

	mx_status = mx_find_record_field_defaults(
			record_field_defaults_array, num_record_fields,
			"input_device_array", &field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	field->dimension[0] = *num_input_devices_varargs_cookie;

	/****
	 **** Construct the 'num_independent_variables' varargs cookie
	 **** which is used by the individual device classes.
	 ****/

	mx_status = mx_find_record_field_defaults_index(
			record_field_defaults_array, num_record_fields,
			"num_independent_variables", &referenced_field_index );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_construct_varargs_cookie(
			referenced_field_index, 0,
			num_independent_variables_varargs_cookie );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 8,("%s: num_independent_variables varargs_cookie = %ld",
			fname, *num_independent_variables_varargs_cookie));

	return MX_SUCCESSFUL_RESULT;
}

/* If mx_compute_normalized_device_value() or 
 * mx_convert_normalized_device_value_to_string() are called with a
 * measurement time of zero or less, then the function returns an
 * unnormalized value.
 */

MX_EXPORT mx_status_type
mx_compute_normalized_device_value( MX_RECORD *input_device,
				double measurement_time,
				double *returned_value )
{
	static const char fname[] = "mx_compute_normalized_device_value()";

	long *dimension_array;
	long long_value, num_dimensions, field_type;
	unsigned long ulong_value;
	double double_value;
	void *ptr_to_value;
	mx_status_type mx_status;

	switch( input_device->mx_superclass ) {
	case MXR_DEVICE:
		switch( input_device->mx_class ) {
		case MXC_ANALOG_INPUT:
			mx_status = mx_analog_input_read( input_device,
							&double_value );
			*returned_value = double_value;
			break;

		case MXC_ANALOG_OUTPUT:
			mx_status = mx_analog_output_read( input_device,
							&double_value );
			*returned_value = double_value;
			break;

		case MXC_DIGITAL_INPUT:
			mx_status = mx_digital_input_read( input_device,
							&ulong_value );
			*returned_value = (double) ulong_value;
			break;

		case MXC_DIGITAL_OUTPUT:
			mx_status = mx_digital_output_read( input_device,
							&ulong_value );
			*returned_value = (double) ulong_value;
			break;

		case MXC_MOTOR:
			mx_status = mx_motor_get_position( input_device,
							&double_value );
			*returned_value = double_value;
			break;

		case MXC_SCALER:
			mx_status = mx_scaler_read( input_device, &long_value );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( measurement_time <= 0.0 ) {
				*returned_value = (double) long_value;
			} else {
				*returned_value = mx_divide_safely(
							(double) long_value,
							measurement_time );
			}
			break;

		case MXC_TIMER:
			mx_status = mx_timer_read( input_device, &double_value);

			*returned_value = double_value;
			break;

		case MXC_RELAY:
			mx_status = mx_get_relay_status( input_device,
							&long_value );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			*returned_value = (double) long_value;
			break;

		default:
			return mx_error( MXE_UNSUPPORTED, fname,
				"Computing normalized values is not supported "
				"for the MX '%s' driver used by record '%s'.",
					mx_get_driver_name( input_device ),
					input_device->name );
		}
		break;

	case MXR_VARIABLE:
		mx_status = mx_get_variable_parameters( input_device,
					&num_dimensions, &dimension_array,
					&field_type, &ptr_to_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( (num_dimensions != 1) || (dimension_array[0] != 1) ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"This operation is only supported for 1-dimensional "
			"MX variables with only one element.  Variable "
			"record '%s' does not meet these requirements.",
				input_device->name );
		}
		switch( field_type ) {
		case MXFT_SHORT:
			*returned_value = (double) *((short *) ptr_to_value);
			break;
		case MXFT_USHORT:
			*returned_value = (double)
					*((unsigned short *) ptr_to_value);
			break;
		case MXFT_BOOL:
			if ( *((mx_bool_type *) ptr_to_value) == FALSE ) {
				*returned_value = FALSE;
			} else {
				*returned_value = TRUE;
			}
			break;
		case MXFT_LONG:
			*returned_value = (double) *((long *) ptr_to_value);
			break;
		case MXFT_ULONG:
			*returned_value = (double)
					*((unsigned long *) ptr_to_value);
			break;
		case MXFT_INT64:
			*returned_value = (double) *((int32_t *) ptr_to_value);
			break;
		case MXFT_UINT64:
			*returned_value = (double) *((uint32_t *) ptr_to_value);
			break;
		case MXFT_FLOAT:
			*returned_value = (double) *((float *) ptr_to_value);
			break;
		case MXFT_DOUBLE:
			*returned_value = *((double *) ptr_to_value);
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"This operation is only supported for MX variables "
			"that return numerical values.  Variable record '%s' "
			"does not meet these requirements.",
				input_device->name );
		}
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Computing normalized values is only supported for "
			"device and variable records.  Record '%s' is "
			"not a device record or a variable record.",
				input_device->name );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_convert_normalized_device_value_to_string( MX_RECORD *input_device,
				double measurement_time,
				char *buffer, size_t buffer_length )
{
	static const char fname[] =
		"mx_convert_normalized_device_value_to_string()";

	MX_RECORD_FIELD *record_field;
	mx_status_type (*token_constructor) ( void *, char *, size_t,
					MX_RECORD *, MX_RECORD_FIELD * );
	MX_ANALOG_INPUT *analog_input;
	MX_ANALOG_OUTPUT *analog_output;
	MX_DIGITAL_INPUT *digital_input;
	MX_DIGITAL_OUTPUT *digital_output;
	MX_MOTOR *motor;
	MX_SCALER *scaler;
	MX_TIMER *timer;
	MX_RELAY *relay;
	MX_AMPLIFIER *amplifier;
	void *field_value_ptr;
	double position, raw_position, double_value;
	mx_status_type mx_status;

	if ( ((long)buffer_length) <= 0 ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"Buffer length %ld is too short.", (long) buffer_length );
	}

	switch( input_device->mx_superclass ) {
	case MXR_SCAN:
		strcpy(buffer, "");
		break;
	case MXR_DEVICE:
		if ( buffer_length <= 10 ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"Buffer length %ld <= 10 is too short.", (long)buffer_length );
		}
		switch( input_device->mx_class ) {
		case MXC_ANALOG_INPUT:
			analog_input = (MX_ANALOG_INPUT *)
				(input_device->record_class_struct);

			double_value = analog_input->value;

			sprintf(buffer, "%-10.*g", input_device->precision,
					double_value );
			break;
		case MXC_ANALOG_OUTPUT:
			analog_output = (MX_ANALOG_OUTPUT *)
				(input_device->record_class_struct);
			sprintf(buffer, "%-10.*g", input_device->precision,
					analog_output->value);
			break;
		case MXC_DIGITAL_INPUT:
			digital_input = (MX_DIGITAL_INPUT *)
				(input_device->record_class_struct);
			sprintf(buffer, "%10lu", digital_input->value);
			break;
		case MXC_DIGITAL_OUTPUT:
			digital_output = (MX_DIGITAL_OUTPUT *)
				(input_device->record_class_struct);
			sprintf(buffer, "%10lu", digital_output->value);
			break;
		case MXC_MOTOR:
			motor = (MX_MOTOR *)
				(input_device->record_class_struct);
			switch( motor->subclass ) {
			case MXC_MTR_ANALOG:
				raw_position = motor->raw_position.analog;
				break;
			case MXC_MTR_STEPPER:
				raw_position
				    = (double)(motor->raw_position.stepper);
				break;
			default:
				return mx_error(MXE_NOT_YET_IMPLEMENTED, fname,
				"Motor subclass %ld not yet implemented.",
					motor->subclass );
			}
			position = motor->offset + motor->scale * raw_position;

			sprintf(buffer, "%-10.*g", input_device->precision,
					position);
			break;
		case MXC_SCALER:
			scaler = (MX_SCALER *)
				(input_device->record_class_struct);

			if ( measurement_time > 0.0 ) {
				sprintf( buffer, "%-10.*g",
					input_device->precision,
					mx_divide_safely( (double)scaler->value,
						measurement_time ) );
			} else {
				sprintf( buffer, "%10ld", scaler->value );
			}
			break;
		case MXC_TIMER:
			timer = (MX_TIMER *)
				(input_device->record_class_struct);
			sprintf(buffer, "%10g", timer->value);
			break;
		case MXC_RELAY:
			relay = (MX_RELAY *)
				(input_device->record_class_struct);
			sprintf(buffer, "%10ld", relay->relay_status);
			break;
		case MXC_AMPLIFIER:
			amplifier = (MX_AMPLIFIER *)
				(input_device->record_class_struct);
			sprintf(buffer, "%10g", amplifier->gain);
			break;
		case MXC_MULTICHANNEL_ANALYZER:

			/* We currently don't print out the values for MCAs. */

			sprintf(buffer, "%10s", input_device->name );
			break;
		default:
			return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
			"Record class %ld not yet implemented.",
				input_device->mx_class );
		}
		break;
	case MXR_VARIABLE:
		mx_status = mx_find_record_field( input_device, "value",
						&record_field );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		field_value_ptr = mx_get_field_value_pointer( record_field );

		if ( field_value_ptr == NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"Field value pointer for record field '%s' in record '%s' is NULL.",
				record_field->name, input_device->name );
		}

		mx_status = mx_get_token_constructor( record_field->datatype,
						&token_constructor );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		strcpy( buffer, "" );

		mx_status = mx_create_array_description(
				field_value_ptr,
				(record_field->num_dimensions - 1),
				buffer, buffer_length,
				input_device, record_field,
				token_constructor );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		break;
	default:
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
			"Record superclass %ld not yet implemented.",
				input_device->mx_superclass );
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_scan_find_parent_scan( MX_SCAN *child_scan, MX_SCAN **parent_scan )
{
	static const char fname[] = "mx_scan_find_parent_scan()";

	MX_RECORD *parent_scan_record;

	if ( child_scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The child_scan pointer passed is NULL." );
	}
	if ( parent_scan == (MX_SCAN **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The parent_scan pointer passed is NULL." );
	}

	if ( child_scan->datafile.type != MXDF_CHILD ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Scan '%s' is not a child scan and does not have a parent.",
			child_scan->record->name );
	}

	/* For a child scan, the "filename" field contains the
	 * name of the parent scan that opened the datafile.
	 */

	parent_scan_record = mx_get_record( child_scan->record,
					child_scan->datafile.filename );

	if ( parent_scan_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NOT_FOUND, fname,
		"The parent of child scan '%s' is supposed to be named '%s', "
		"but a record of that name does not exist.",
			child_scan->record->name,
			child_scan->datafile.filename );
	}
	if ( parent_scan_record->mx_superclass != MXR_SCAN ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"The parent of child scan '%s' is supposed to be named '%s', "
		"but the record by that name is not a scan record.",
			child_scan->record->name,
			child_scan->datafile.filename );
	}

	*parent_scan = (MX_SCAN *) parent_scan_record->record_superclass_struct;

	if ( (*parent_scan) == (MX_SCAN *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_SCAN pointer for parent scan '%s' used by child scan '%s' is NULL.",
			parent_scan_record->name, child_scan->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_scan_get_pointer_to_measurement_number( MX_SCAN *scan, long **ptr )
{
	static const char fname[] =
		"mx_scan_get_pointer_to_measurement_number()";

	MX_SCAN *parent_scan;
	mx_status_type mx_status;

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_SCAN pointer passed was NULL." );
	}
	if ( ptr == (long **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"Measurement number pointer passed was NULL." );
	}

	if ( scan->datafile.type != MXDF_CHILD ) {
		*ptr = &(scan->measurement_number);
	} else {
		mx_status = mx_scan_find_parent_scan( scan, &parent_scan );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		*ptr = &(parent_scan->measurement_number);
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_scan_increment_measurement_number( MX_SCAN *scan )
{
	long *number_ptr = NULL;
	mx_status_type mx_status;

	mx_status = mx_scan_get_pointer_to_measurement_number(
						scan, &number_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	(*number_ptr)++;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_scan_get_pointer_to_datafile_filename( MX_SCAN *scan, char **ptr )
{
	static const char fname[] =
		"mx_scan_get_pointer_to_datafile_filename()";

	MX_SCAN *parent_scan;
	mx_status_type mx_status;

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_SCAN pointer passed was NULL." );
	}
	if ( ptr == (char **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"Measurement number pointer passed was NULL." );
	}

	if ( scan->datafile.type != MXDF_CHILD ) {
		*ptr = scan->datafile.filename;
	} else {
		mx_status = mx_scan_find_parent_scan( scan, &parent_scan );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		*ptr = parent_scan->datafile.filename;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_scan_handle_alternate_x_motors( MX_SCAN *scan )
{
	MX_RECORD *x_motor_record;
	double x_motor_position;
	long i, j;
	mx_bool_type get_position;
	mx_status_type mx_status;

	/* If alternate X axis motors have been specified,
	 * get and save their current positions for use
	 * by the datafile handling code.
	 *
	 * Note that x_position_array is an N by 1 array,
	 * where N is the number of motors, since we only
	 * handle one measurement at a time in step scans.
	 */

	for ( i = 0; i < scan->datafile.num_x_motors; i++ ) {

		x_motor_record = scan->datafile.x_motor_array[i];

		mx_status = mx_motor_get_position( x_motor_record,
							&x_motor_position );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		scan->datafile.x_position_array[i][0] = x_motor_position;
	}

	/* Do the same thing for the plot data.
	 *
	 * The list of alternate X motors for the plot will
	 * not necessarily be the same as the list for the
	 * datafile, but we do not want to unnecessarily
	 * read a motor position twice.
	 */

	for ( i = 0; i < scan->plot.num_x_motors; i++ ) {

		x_motor_record = scan->plot.x_motor_array[i];

		/* See if this motor's position was already
		 * read by the datafile loop above.
		 */

		get_position = TRUE;

		for ( j = 0; j < scan->datafile.num_x_motors; j++ ) {
			if ( x_motor_record == scan->datafile.x_motor_array[j] )
			{
				scan->plot.x_position_array[i][0] =
					scan->datafile.x_position_array[j][0];

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

			scan->plot.x_position_array[i][0] = x_motor_position;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_scan_get_early_move_flag( MX_SCAN *scan, mx_bool_type *early_move_flag )
{
	static const char fname[] = "mx_scan_get_early_move_flag()";

	MX_RECORD *early_move_record;
	unsigned long early_move_policy;
	mx_status_type mx_status;

	if ( scan == (MX_SCAN *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SCAN pointer passed was NULL." );
	}
	if ( early_move_flag == (mx_bool_type *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The early_move_flag pointer passed was NULL." );
	}

	early_move_record = mx_get_record( scan->record,
					MX_SCAN_EARLY_MOVE_RECORD_NAME );

	if ( early_move_record == (MX_RECORD *) NULL ) {

		early_move_policy = MXF_SCAN_ALLOW_EARLY_MOVE;
	} else {
		if ( early_move_record->mx_superclass != MXR_VARIABLE ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
			"Record '%s' is not a variable record.",
				early_move_record->name );
		}

		mx_status = mx_get_unsigned_long_variable( early_move_record,
							&early_move_policy );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	*early_move_flag = FALSE;

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( early_move_policy ) {
	case MXF_SCAN_PROHIBIT_EARLY_MOVE:
		*early_move_flag = FALSE;
		break;

	case MXF_SCAN_REQUIRE_EARLY_MOVE:
		*early_move_flag = TRUE;
		break;

	case MXF_SCAN_ALLOW_EARLY_MOVE:
		if ( scan->scan_flags & MXF_SCAN_EARLY_MOVE ) {
			*early_move_flag = TRUE;
		} else {
			*early_move_flag = FALSE;
		}
		break;

	default:
		*early_move_flag = FALSE;

		mx_status = mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The '%s' record is set to an illegal value (%lu).  "
			"The allowed values are 0, 1, and 2.",
				early_move_record->name, early_move_policy );
		break;
	}

	MX_DEBUG( 2,("%s: scan '%s', early_move_flag = %d",
		fname, scan->record->name, (int) *early_move_flag ));

	return mx_status;
}

