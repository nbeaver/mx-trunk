/*
 * Name:    mmeasure.c
 *
 * Purpose: Measure scaler dark currents or amplifier offsets.
 *
 *          This function expects the existence of either a record called
 *          'dark_timer' or 'timer1' which will be used to gate on the
 *          timers for the dark current measurement.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2002, 2004, 2006, 2010, 2017
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define DEBUG_DARK_CURRENT_TIMING	FALSE

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "motor.h"
#include "mdialog.h"
#include "mx_hrt_debug.h"
#include "mx_analog_input.h"
#include "mx_autoscale.h"
#include "d_auto_scaler.h"
#include "d_timer_fanout.h"

#define FREE_DEVICE_ARRAYS \
	do { \
		if ( device_array != NULL ) { \
			free( device_array ); \
			device_array = NULL; \
		} \
		if ( device_sum_array != NULL ) { \
			free( device_sum_array ); \
			device_sum_array = NULL; \
		} \
	} while(0)

static int motor_measure_dark_currents( int argc, char *argv[] );
static int motor_measure_autoscale( int argc, char *argv[] );
static int motor_measure_device_offsets( long, MX_RECORD **,
				double *, MX_RECORD *, double, long );

int
motor_measure_fn( int argc, char *argv[] )
{
	static char usage[] =
"Usage:  measure dark_currents [measurement time] [# of measurements]\n"
"        measure autoscale [] []\n";

	size_t length;
	int status;

	if ( argc <= 2 ) {
		fprintf( output, "%s\n", usage );
		return FAILURE;
	}

	length = strlen( argv[2] );

	if ( length == 0 )
		length = 1;

	if ( (strncmp( argv[2], "dark_currents", length ) == 0)
	  || (strncmp( argv[2], "pedestals", length ) == 0) )
	{
		status = motor_measure_dark_currents( argc, argv );
	} else
	if ( strncmp( argv[2], "autoscale", length ) == 0) {

		status = motor_measure_autoscale( argc, argv );
	} else {
		fprintf( output, "%s", usage );
		status = FAILURE;
	}

	return status;
}

static mx_bool_type
mxp_belongs_to_this_timer( MX_RECORD *current_record,
			MX_RECORD *dark_timer_record )
{
	MX_SCALER *current_scaler = NULL;
	MX_ANALOG_INPUT *current_ainput = NULL;
	MX_RECORD *current_timer_record = NULL;

	switch( current_record->mx_class ) {
	case MXC_SCALER:
		current_scaler = (MX_SCALER *)
					current_record->record_class_struct;

		current_timer_record = current_scaler->timer_record;
		break;
	case MXC_ANALOG_INPUT:
		current_ainput = (MX_ANALOG_INPUT *)
					current_record->record_class_struct;

		current_timer_record = current_ainput->timer_record;
		break;
	}

	if ( current_timer_record == (MX_RECORD *) NULL ) {
		return FALSE;
	}

	if ( current_timer_record == dark_timer_record ) {
		return TRUE;
	}

	/* If dark_timer_record is a 'timer_fanout' record, then we must
	 * look at the children of the timer fanout record to see if one
	 * of them match current_timer_record.
	 */

	if ( dark_timer_record->mx_type == MXT_TIM_FANOUT ) {
	    long i;
	    MX_RECORD *timer_array_member = NULL;
	    MX_TIMER_FANOUT *timer_fanout = (MX_TIMER_FANOUT *)
					dark_timer_record->record_type_struct;

	    for ( i = 0; i < timer_fanout->num_timers; i++ ) {
		timer_array_member = timer_fanout->timer_record_array[i];

		if ( timer_array_member == current_timer_record ) {
			return TRUE;
		}
	    }
	}

	return FALSE;
}

static int
motor_measure_dark_currents( int argc, char *argv[] )
{
	static const char cname[] = "motor_measure_dark_currents()";

	MX_RECORD *timer_record;
	MX_RECORD *current_record;
	MX_RECORD **device_array;
	MX_SCALER *scaler;
	MX_ANALOG_INPUT *ainput;
	double *device_sum_array;
	long i, num_devices;
	double measurement_time;
	long num_measurements;
	double new_dark_current;
	unsigned long scaler_mask, ainput_mask;
	int status;
	mx_bool_type timer_found;
	mx_status_type mx_status;

	device_array = NULL;
	device_sum_array = NULL;

#if DEBUG_DARK_CURRENT_TIMING
	MX_HRT_TIMING start_timing;
	MX_HRT_TIMING offset_timing;
	MX_HRT_TIMING save_timing;

	MX_HRT_START( start_timing );
#endif

	timer_found = FALSE;

	/* Is argv[3] the name of a timer record? */

	timer_record = mx_get_record( motor_record_list, argv[3] );

	if ( timer_record != (MX_RECORD *) NULL ) {
		if ( (timer_record->mx_superclass == MXR_DEVICE)
		  && (timer_record->mx_class == MXC_TIMER) )
		{
			timer_found = TRUE;

			/* Now skip over this entry in the argument list. */

			argc--;
			argv++;
		}
	}

	if ( timer_found == FALSE ) { 
		/* Alternately, look for a record called 'dark_timer'. */

		timer_record = mx_get_record( motor_record_list, "dark_timer" );

		if ( timer_record != (MX_RECORD *) NULL ) {
			if ( (timer_record->mx_superclass == MXR_DEVICE)
			  && (timer_record->mx_class == MXC_TIMER) )
			{
				timer_found = TRUE;
			}
		}
	}

	if ( timer_found == FALSE ) { 
		/* Alternately, look for a record called 'timer1'. */

		timer_record = mx_get_record( motor_record_list, "timer1" );

		if ( timer_record != (MX_RECORD *) NULL ) {
			if ( (timer_record->mx_superclass == MXR_DEVICE)
			  && (timer_record->mx_class == MXC_TIMER) )
			{
				timer_found = TRUE;
			}
		}
	}

	if ( timer_found == FALSE ) { 
		fprintf(output,
"Dark current measurements require the presence of either an explicitly\n"
"specified timer name, or a timer called 'dark_timer' or a timer record\n"
"called 'timer1'.  None of these are true, so a dark current measurement\n"
"cannot be performed.\n");
		return FAILURE;
	}

	/* Get a list of all the scaler records. */

	/* We will be measuring dark currents for any scaler that has
	 * either the MXF_SCL_SUBTRACT_DARK_CURRENT flag or the 
	 * MXF_SCL_SERVER_SUBTRACTS_DARK_CURRENT flag set.
	 */

	scaler_mask = MXF_SCL_SUBTRACT_DARK_CURRENT
				| MXF_SCL_SERVER_SUBTRACTS_DARK_CURRENT;

	/* Do the same thing for analog inputs. */

	ainput_mask = MXF_AIN_SUBTRACT_DARK_CURRENT
				| MXF_AIN_SERVER_SUBTRACTS_DARK_CURRENT;

	num_devices = 0L;

	current_record = motor_record_list->next_record;

	while ( current_record != motor_record_list ) {

		if ( current_record->mx_class == MXC_SCALER ) {

			scaler = (MX_SCALER *)
				current_record->record_class_struct;

			if ( scaler == (MX_SCALER *) NULL ) {
				fprintf( output,
			"The MX_SCALER pointer for scaler '%s' is NULL.",
					current_record->name );

				return FAILURE;
			}
			if ( ( scaler->scaler_flags & scaler_mask ) != 0 ) {
				if ( mxp_belongs_to_this_timer( current_record,
								timer_record ) )
				{
					num_devices++;
				}
			}
		} else
		if ( current_record->mx_class == MXC_ANALOG_INPUT ) {

			ainput = (MX_ANALOG_INPUT *)
				current_record->record_class_struct;

			if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
				fprintf( output,
		"The MX_ANALOG_INPUT pointer for analog input '%s' is NULL.",
					current_record->name );

				return FAILURE;
			}
			if ( ( ainput->analog_input_flags & ainput_mask ) != 0 )
			{
				if ( mxp_belongs_to_this_timer( current_record,
								timer_record ) )
				{
					num_devices++;
				}
			}
		}

		current_record = current_record->next_record;
	}

	/* Allocate an array to store the list of devices. */

	device_array = (MX_RECORD **) malloc(num_devices * sizeof(MX_RECORD *));

	if ( device_array == (MX_RECORD **) NULL ) {
		fprintf( output,
		"Ran out of memory trying to allocate an array of %ld "
		"record pointers.\n", num_devices );
		return FAILURE;
	}

	device_sum_array = (double *) malloc( num_devices * sizeof(double) );

	if ( device_sum_array == (double *) NULL ) {
		fprintf( output,
"Ran out of memory trying to allocate an array of %ld device values.\n",
			num_devices );
		FREE_DEVICE_ARRAYS;
		return FAILURE;
	}

	/* Now fill the array with the record pointers.  Skip any records
	 * that have dark current subtraction disabled.
	 */

	i = 0;

	current_record = motor_record_list->next_record;

	while ( (current_record != motor_record_list) && (i < num_devices) ) {

		if ( current_record->mx_class == MXC_SCALER ) {

			scaler = (MX_SCALER *)
				current_record->record_class_struct;

			if ( scaler == (MX_SCALER *) NULL ) {
				fprintf( output,
			"The MX_SCALER pointer for scaler '%s' is NULL.",
					current_record->name );

				return FAILURE;
			}
			if (( scaler->scaler_flags & scaler_mask ) != 0) {
				if ( mxp_belongs_to_this_timer( current_record,
								timer_record ) )
				{
					device_array[i] = current_record;

					MX_DEBUG(-2,(
					"%s: scaler '%s' will be measured.",
						cname, current_record->name ));
					i++;
				}
			}
		} else
		if ( current_record->mx_class == MXC_ANALOG_INPUT ) {

			ainput = (MX_ANALOG_INPUT *)
				current_record->record_class_struct;

			if ( ainput == (MX_ANALOG_INPUT *) NULL ) {
				fprintf( output,
		"The MX_ANALOG_INPUT pointer for analog input '%s' is NULL.",
					current_record->name );

				return FAILURE;
			}
			if (( ainput->analog_input_flags & ainput_mask ) != 0) {
				if ( mxp_belongs_to_this_timer( current_record,
								timer_record ) )
				{
					device_array[i] = current_record;

					MX_DEBUG(-2,(
				    "%s: analog input '%s' will be measured.",
						cname, current_record->name ));
					i++;
				}
			}
		}

		current_record = current_record->next_record;
	}

	if ( i < num_devices ) {
		fprintf( output,
"Warning: The number of devices changed during startup of this command.\n" );
		fprintf( output, "num_devices = %ld, i = %ld\n",num_devices,i);
	}

#if 0
	fprintf( output, "Number of devices = %ld\n", num_devices );
#endif

	/* Prompt the user for the measurement parameters. */

	if ( argc > 3 ) {
		measurement_time = atof( argv[3] );
	} else {
		measurement_time = 0.0;

		status = motor_get_double( output,
			"Enter measurement time (seconds) -> ",
			TRUE, 1.0,
			&measurement_time, 0.0, 1.0e38 );

		if ( status != SUCCESS ) {
			FREE_DEVICE_ARRAYS;
			return status;
		}
	}

	if ( argc > 4 ) {
		num_measurements = atol( argv[4] );
	} else {
		num_measurements = 0L;

		status = motor_get_long( output,
			"Enter number of measurements to average -> ",
			TRUE, 1,
			&num_measurements, 1L, 1000000L );

		if ( status != SUCCESS ) {
			FREE_DEVICE_ARRAYS;
			return status;
		}
	}

#if DEBUG_DARK_CURRENT_TIMING
	MX_HRT_END( start_timing );
	MX_HRT_RESULTS( start_timing, cname, "Start Timing" );

	MX_HRT_START( offset_timing );
#endif

	/***** Perform the measurements. *****/

	fprintf(output,"Measuring dark currents using timer '%s'.\n",
		timer_record->name);

	status = motor_measure_device_offsets( num_devices, device_array,
				device_sum_array, timer_record,
				measurement_time, num_measurements );

	if ( status != SUCCESS ) {
		FREE_DEVICE_ARRAYS;
		return status;
	}

#if DEBUG_DARK_CURRENT_TIMING
	MX_HRT_END( offset_timing );
	MX_HRT_RESULTS( offset_timing, cname, "Offset Timing" );

	MX_HRT_START( save_timing );
#endif

	/* Now save the new dark current values. */

	for ( i = 0; i < num_devices; i++ ) {
		current_record = device_array[i];

		if ( current_record->mx_class == MXC_SCALER ) {
			new_dark_current = device_sum_array[i]
			    / (measurement_time * (double) num_measurements);

			mx_status = mx_scaler_set_dark_current(
					current_record, new_dark_current );
		} else {
			ainput = (MX_ANALOG_INPUT *)
					current_record->record_class_struct;

			if ( ainput->analog_input_flags &
				MXF_AIN_PERFORM_TIME_NORMALIZATION )
			{
			    new_dark_current = device_sum_array[i]
			      / (measurement_time * (double) num_measurements);
			} else {
			    new_dark_current = device_sum_array[i]
					      / (double) num_measurements;
			}

			mx_status = mx_analog_input_set_dark_current(
					current_record, new_dark_current );
		}

		if ( mx_status.code != MXE_SUCCESS ) {
			FREE_DEVICE_ARRAYS;
			return FAILURE;
		}

		fprintf( output,
			"Dark current for '%s' = %.*g counts per second.\n",
			device_array[i]->name,
			device_array[i]->precision,
			new_dark_current );
	}

	FREE_DEVICE_ARRAYS;

#if DEBUG_DARK_CURRENT_TIMING
	MX_HRT_END( save_timing );
	MX_HRT_RESULTS( save_timing, cname, "Save Timing" );
#endif

	return SUCCESS;
}

static int
motor_measure_autoscale( int argc, char *argv[] )
{
	static char cname[] = "motor_measure_autoscale()";

	MX_RECORD *autoscale_scaler_record;
	MX_AUTOSCALE_SCALER *autoscale_scaler;
	MX_AUTOSCALE *autoscale;
	char *autoscale_scaler_name;
	double measurement_time, scaler_sum;
	long num_measurements;
	unsigned long i, num_monitor_offsets, old_monitor_offset_index;
	double *monitor_offset_array;
	int status;
	mx_status_type mx_status;

	static char usage[] =
"Usage: measure autoscale 'scaler_name' [measurement_time] [# measurements]\n";

	MX_DEBUG(-2,("%s invoked.", cname));

	/**** Parse the arguments to 'measure autoscale'. ****/

	/* The first argument (required) is the name of the autoscale scaler.*/

	if ( argc <= 3 ) {
		fputs(usage, output);
		return FAILURE;
	}

	autoscale_scaler_name = argv[3];

	/* Does this record exist? */

	autoscale_scaler_record = mx_get_record( motor_record_list,
						autoscale_scaler_name );

	if ( autoscale_scaler_record == (MX_RECORD *) NULL ) {
		fprintf(output, "The record '%s' does not exist.\n",
				autoscale_scaler_name );
		return FAILURE;
	}

	/* Is it the right kind of record? */

	if ( (autoscale_scaler_record->mx_superclass != MXR_DEVICE)
	  || (autoscale_scaler_record->mx_class != MXC_SCALER)
	  || (autoscale_scaler_record->mx_type != MXT_SCL_AUTOSCALE) )
	{
		fprintf(output,
		"The record '%s' is not an autoscaling scaler record.\n",
			autoscale_scaler_record->name );
		return FAILURE;
	}

	/* Get the various records that the autoscale scaler depends on. */

	autoscale_scaler = (MX_AUTOSCALE_SCALER *)
				autoscale_scaler_record->record_type_struct;

	if ( autoscale_scaler == (MX_AUTOSCALE_SCALER *) NULL ) {
		fprintf(output,
		"The MX_AUTOSCALE_SCALER pointer for record '%s' is NULL.\n",
			autoscale_scaler_record->name);
		return FAILURE;
	}

	if ( autoscale_scaler->autoscale_record == (MX_RECORD *) NULL ) {
		fprintf(output,
	"The autoscale_record pointer for autoscale scaler '%s' is NULL.\n",
			autoscale_scaler_record->name );
		return FAILURE;
	}

	autoscale = (MX_AUTOSCALE *)
		autoscale_scaler->autoscale_record->record_class_struct;

	if ( autoscale == (MX_AUTOSCALE *) NULL ) {
		fprintf(output,
		"The MX_AUTOSCALE pointer for autoscale record '%s' is NULL.\n",
			autoscale_scaler->autoscale_record->name );
		return FAILURE;
	}

	MX_DEBUG(-2,("%s: autoscale record = '%s'",
		cname, autoscale->record->name ));
	MX_DEBUG(-2,("%s: monitor record   = '%s'",
		cname, autoscale->monitor_record->name));
	MX_DEBUG(-2,("%s: control record   = '%s'",
		cname, autoscale->control_record->name));
	MX_DEBUG(-2,("%s: timer record     = '%s'",
		cname, autoscale->timer_record->name));

	/* Prompt the user for the measurement parameters. */

	if ( argc > 4 ) {
		measurement_time = atof( argv[4] );
	} else {
		measurement_time = 0.0;

		status = motor_get_double( output,
			"Enter measurement time (seconds) -> ",
			TRUE, 1.0,
			&measurement_time, 0.0, 1.0e38 );

		if ( status != SUCCESS )
			return status;
	}

	if ( argc > 5 ) {
		num_measurements = atol( argv[5] );
	} else {
		num_measurements = 0L;

		status = motor_get_long( output,
			"Enter number of measurements to average -> ",
			TRUE, 1,
			&num_measurements, 1L, 1000000L );

		if ( status != SUCCESS )
			return status;
	}

	MX_DEBUG(-2,("%s: measurement_time = %g",
		cname, measurement_time));
	MX_DEBUG(-2,("%s: num_measurements = %ld",
		cname, num_measurements));

	/* Find out how many monitor offsets there are. */

	mx_status = mx_autoscale_get_offset_array( autoscale->record,
						&num_monitor_offsets,
						&monitor_offset_array );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	/* Save the value of which monitor offset is currently being used. */

	mx_status = mx_autoscale_get_offset_index( autoscale->record,
						&old_monitor_offset_index );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	/* Measure the dark currents for all the possible monitor 
	 * offset indices.
	 */

	for ( i = 0; i < num_monitor_offsets; i++ ) {

		mx_status = mx_autoscale_set_offset_index(autoscale->record, i);

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		status = motor_measure_device_offsets( 1L,
					&(autoscale->monitor_record),
					&scaler_sum, autoscale->timer_record,
					measurement_time, num_measurements );

		monitor_offset_array[i] = scaler_sum
			/ (measurement_time * (double) num_measurements);

		MX_DEBUG(-2,(
		"%s: i = %lu, scaler_sum = %g, dark_current = %g",
			cname, i, scaler_sum, monitor_offset_array[i]));
	}

	/* Set the new values of the monitor offsets. */

	mx_status = mx_autoscale_set_offset_array( autoscale->record,
						num_monitor_offsets,
						monitor_offset_array );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	/* Restore the old monitor offset index. */

	mx_status = mx_autoscale_set_offset_index( autoscale->record,
						old_monitor_offset_index );
	
	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	return SUCCESS;
}

static int
motor_measure_device_offsets( long num_devices,
				MX_RECORD **device_record_array,
				double *device_sum_array,
				MX_RECORD *timer_record,
				double measurement_time,
				long num_measurements )
{
	MX_RECORD *record;
	MX_ANALOG_INPUT *ainput;
	double value, seconds_left;
	long i, n, long_value;
	mx_bool_type busy;
	mx_status_type mx_status;

	/* First, zero the device value array. */

	for ( i = 0; i < num_devices; i++ ) {
		device_sum_array[i] = 0L;
	}

	/* Stop the timer if it is on. */

	mx_status = mx_timer_stop( timer_record, &seconds_left );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	/* Configure the timer and scalers for preset time mode. */

	mx_status = mx_timer_set_mode( timer_record, MXCM_PRESET_MODE );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	mx_status = mx_timer_set_modes_of_associated_counters( timer_record );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	for ( i = 0; i < num_devices; i++ ) {
		record = device_record_array[i];

		if ( record->mx_class == MXC_SCALER ) {

			mx_status = mx_scaler_set_mode( record,
							MXCM_COUNTER_MODE );
			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;
		}
	}

	/* Loop through the number of measurements. */

	for ( n = 0; n < num_measurements; n++ ) {

		fprintf( output, "Starting measurement %ld of %ld\n",
						n+1, num_measurements );

		/* Clear all the scalers. */

		for ( i = 0; i < num_devices; i++ ) {
			record = device_record_array[i];

			if ( record->mx_class == MXC_SCALER ) {
				mx_status = mx_scaler_clear( record );

				if ( mx_status.code != MXE_SUCCESS )
					return FAILURE;
			}
		}

		/* Turn the timer on and wait for it to finish. */

		mx_status = mx_timer_start( timer_record, measurement_time );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		busy = TRUE;

		while ( busy ) {
			if ( mx_user_requested_interrupt() ) {
				fprintf( output,
				"Dark current measurement was interrupted.\n");
				return FAILURE;
			}

			mx_status = mx_timer_is_busy( timer_record, &busy );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			mx_msleep(1);	/* Wait at least 1 millisecond. */
		}

		/* Read out the devices. */

		for ( i = 0; i < num_devices; i++ ) {

			record = device_record_array[i];

			if ( record->mx_class == MXC_SCALER ) {
				mx_status = mx_scaler_read_raw( record,
								&long_value );

				value = (double) long_value;
			} else {
				ainput = (MX_ANALOG_INPUT *)
						record->record_class_struct;

				if ( ainput->subclass == MXT_AIN_DOUBLE ) {
				    mx_status = mx_analog_input_read_raw_double(
							record, &value );
				} else {
				    mx_status = mx_analog_input_read_raw_long(
							record, &long_value );

				    value = (double) long_value;
				}

				value = ainput->offset + ainput->scale * value;
			}

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			device_sum_array[i] += value;
		}
	}
	return SUCCESS;
}

