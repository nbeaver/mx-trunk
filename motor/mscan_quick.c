/*
 * Name:    mscan_quick.c
 *
 * Purpose: Motor quick scan setup and modify function.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 1999-2003, 2005 Illinois Institute of Technology
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
#include <float.h>
#include <math.h>

#include "motor.h"
#include "mdialog.h"
#include "mx_array.h"
#include "mx_scan_quick.h"
#include "mx_mcs.h"
#include "mx_variable.h"
#include "d_mcs_scaler.h"

#include "mx_epics.h"
#include "sq_joerger.h"

#define FREE_MOTOR_NAME_ARRAY \
	do { \
		if ( scan_num_motors > 0 ) { \
			(void) mx_free_array( motor_name_array, \
				2, motor_name_dimension_array, \
				name_element_size_array ); \
			motor_name_array = NULL; \
		} \
	} while(0)

int
motor_setup_quick_scan_parameters(
	char *scan_name,
	MX_SCAN *old_scan,
	char *record_description_buffer,
	size_t record_description_buffer_length,
	char *input_devices_string,
	char *measurement_parameters_string,
	char *datafile_and_plot_parameters_string )
{
	const char fname[] = "motor_setup_quick_scan_parameters()";

	MX_RECORD *record;
	MX_MOTOR *motor;
	MX_RECORD **motor_record_array;
	MX_RECORD *first_input_device_record;
	MX_RECORD *joerger_quick_scan_enable_record;
	MX_QUICK_SCAN *quick_scan;
	MX_LIST_HEAD *list_head_struct;
	MX_MCS *mcs;
	MX_MCS_SCALER *mcs_scaler;
	mx_status_type mx_status;
	char default_clock_name[ MXU_RECORD_NAME_LENGTH + 1 ];
	char clock_name[ MXU_RECORD_NAME_LENGTH + 1 ];
	static char buffer[ MXU_RECORD_DESCRIPTION_LENGTH + 1 ];
	char prompt[100];
	MX_RECORD *clock_record;
	double negative_limit, positive_limit;
	double scale, offset;
	int i, j, status;
	int num_items, string_length, valid_clock_name;
	size_t buffer_left;
	long scan_class, scan_type;
	long scan_num_scans;
	long scan_num_independent_variables;
	long scan_num_motors;
	long old_scan_num_motors;
	double *scan_start;
	double *scan_end;
	long *scan_num_measurements;
	int *motor_is_independent_variable;
	double raw_num_measurements;
	double scan_measurement_time;
	double scan_step_size;
	double old_measurement_time;

#if HAVE_EPICS
	long selection;
#endif

	char **motor_name_array;
	long motor_name_dimension_array[2];

	static size_t name_element_size_array[2] = {
		sizeof(char), sizeof(char *) };

	int default_precision, joerger_quick_scan_enabled;
	long default_long;
	double default_double;

	list_head_struct = mx_get_record_list_head_struct( motor_record_list );

	if ( list_head_struct == (MX_LIST_HEAD *) NULL ) {
		fprintf(output, "MX_LIST_HEAD structure pointer is NULL!\n");

		return FAILURE;
	}

	default_precision = list_head_struct->default_precision;

	/* Get the scan type. */

	scan_class = MXS_QUICK_SCAN;

	joerger_quick_scan_enable_record = NULL;
	joerger_quick_scan_enabled = FALSE;

	if ( old_scan == (MX_SCAN *) NULL ) {

#if HAVE_EPICS
		joerger_quick_scan_enable_record = mx_get_record(
					motor_record_list,
					MX_JOERGER_QUICK_SCAN_ENABLE_RECORD );

		if ( joerger_quick_scan_enable_record != (MX_RECORD *) NULL ) {
			mx_status = mx_get_int_variable(
					joerger_quick_scan_enable_record,
					&joerger_quick_scan_enabled );
		}

		if ( joerger_quick_scan_enabled ) {
			fprintf(output,
				"Select scan type:\n"
				"    1.  MCS quick scan.\n"
				"    2.  APS insertion device quick scan.\n"
				"    3.  Joerger quick scan.\n"
				"\n");

			status = motor_get_long( output, "--> ", TRUE, 1,
						&selection, 1, 3 );
		} else {
			fprintf(output,
				"Select scan type:\n"
				"    1.  MCS quick scan.\n"
				"    2.  APS insertion device quick scan.\n"
				"\n");

			status = motor_get_long( output, "--> ", TRUE, 1,
						&selection, 1, 2 );
		}

		if ( status == FAILURE ) {
			return FAILURE;
		}
		switch( selection ) {
		case 1:
			scan_type = MXS_QUI_MCS;
			break;
		case 2:
			scan_type = MXS_QUI_APS_ID;
			break;
		case 3:
			scan_type = MXS_QUI_JOERGER;
			break;
		default:
			fprintf( output,
"Illegal scan type %ld.  This should not be able to happen and is a bug.\n",
				selection );

			return FAILURE;
			break;
		}

#else  /* HAVE_EPICS */

		scan_type = MXS_QUI_MCS;

		fprintf( output, "MCS quick scan selected.\n" );

#endif  /* HAVE_EPICS */

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

	/* Set the number of independent variables and motors. */

	switch( scan_type ) {
	case MXS_QUI_MCS:
	case MXS_QUI_JOERGER:
	case MXS_QUI_APS_ID:
		scan_num_independent_variables = 1;
		scan_num_motors = 1;
		break;
	default:
		fprintf( output, "%s: Unrecognized scan type %ld.\n",
			fname, scan_type );

		return FAILURE;
		break;
	}

	/* Add the record type info to the record description. */

	sprintf(record_description_buffer, "%s scan quick_scan ", scan_name);

	string_length = strlen(record_description_buffer);
	buffer_left = record_description_buffer_length - string_length;

	switch( scan_type ) {
	case MXS_QUI_MCS:
		strncat(record_description_buffer,
				"mcs_qscan \"\" \"\" ",buffer_left);
		break;
	case MXS_QUI_JOERGER:
		strncat(record_description_buffer,
				"joerger_qscan \"\" \"\" ",buffer_left);
		break;
	case MXS_QUI_APS_ID:
		strncat(record_description_buffer,
				"aps_id_qscan \"\" \"\" ",buffer_left);
		break;
	default:
		fprintf( output, "Unknown quick scan type = %ld\n",
			scan_type );
		break;
	}

	string_length = strlen(record_description_buffer);
	buffer_left = record_description_buffer_length - string_length;
	sprintf( buffer, "%ld %ld %ld ", scan_num_scans,
			scan_num_independent_variables, scan_num_motors );
	strncat( record_description_buffer, buffer, buffer_left );

	/* Create the arrays needed to contain the scan start, etc. */

	if ( scan_num_independent_variables <= 0 ) {
		fprintf( output,
	"%s: This scan has %ld independent variables.  That can't be right.\n",
			fname, scan_num_independent_variables );

		return FAILURE;
	} else {
		scan_start = (double *)
		    malloc( scan_num_independent_variables * sizeof(double));

		if ( scan_start == NULL ) {
			fprintf( output,
			"%s: Out of memory allocating scan start array.\n",
				fname );
			return FAILURE;
		}

		scan_end = (double *)
		    malloc( scan_num_independent_variables * sizeof(double));

		if ( scan_end == NULL ) {
			fprintf( output,
		"%s: Out of memory allocating scan end array.\n",
				fname );
			return FAILURE;
		}

		scan_num_measurements = (long *)
		    malloc( scan_num_independent_variables * sizeof(long));

		if ( scan_num_measurements == NULL ) {
			fprintf( output,
		"%s: Out of memory allocating scan num measurements array.\n",
				fname );
			return FAILURE;
		}
	}

	if ( scan_num_motors <= 0 ) {
		fprintf( output,
		"%s: This scan has %ld motors.  That can't be right.\n",
			fname, scan_num_motors );
		return FAILURE;

	} else {
		motor_is_independent_variable = (int *)
				malloc( scan_num_motors * sizeof(int));

		if ( motor_is_independent_variable == NULL ) {
			fprintf( output,
"%s: Out of memory allocating 'motor_is_independent_variable' array.\n",
				fname );
			return FAILURE;
		}
	}

	for ( i = 0; i < scan_num_motors; i++ ) {
		motor_is_independent_variable[i] = TRUE;
	}

	/*---*/

	if ( old_scan == (MX_SCAN *) NULL ) {
		quick_scan = NULL;
		motor_record_array = NULL;
		old_scan_num_motors = 0;
	} else {
		quick_scan = (MX_QUICK_SCAN *)
				(old_scan->record->record_class_struct);
		motor_record_array = old_scan->motor_record_array;
		old_scan_num_motors = old_scan->num_motors;
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

	if ( status != SUCCESS ) {
		FREE_MOTOR_NAME_ARRAY;
		return status;
	}

	/* Get the limits for each independent motor.
	 *
	 * i = motor array index.
	 * j = independent variable array index.
	 *
	 * That is, not all motors are independent variables.
	 * 
	 * The information prompted for here such as step size, etc.
	 * is not added to the record description until after we
	 * have prompted for the common information such as
	 * input devices and plot types and added the common info
	 * to the record description.
	 */

	for ( i = 0, j = -1; i < scan_num_motors; i++ ) {

		if ( motor_is_independent_variable[i] == FALSE ) {

			/* Go back to the top of the for() loop
			 * for the next value of i.
			 */
			continue;
		}

		/* Increment the independent variable array index. */
		j++;

		/* Get parameters for this motor record. */

		record = mx_get_record(
			motor_record_list, motor_name_array[i]);

		motor = (MX_MOTOR *)(record->record_class_struct);

		if ( motor == (MX_MOTOR *) NULL ) {
			fprintf( output,
			  "MX_MOTOR pointer for record '%s' is NULL.",
				record->name );
			FREE_MOTOR_NAME_ARRAY;
			return FAILURE;
		}

		scale = motor->scale;
		offset = motor->offset;

		switch ( motor->subclass ) {
		case MXC_MTR_STEPPER:
			if ( scale >= 0.0 ) {
			    negative_limit = offset + scale * (double)
				( motor->raw_negative_limit.stepper );
			    positive_limit = offset + scale * (double)
				( motor->raw_positive_limit.stepper );
			} else {
			    negative_limit = offset + scale * (double)
				( motor->raw_positive_limit.stepper );
			    positive_limit = offset + scale * (double)
				( motor->raw_negative_limit.stepper );
			}
			break;
		case MXC_MTR_ANALOG:
			if ( scale >= 0.0 ) {
			    negative_limit = offset + scale *
				( motor->raw_negative_limit.analog );
			    positive_limit = offset + scale *
				( motor->raw_positive_limit.analog );
			} else {
			    negative_limit = offset + scale *
				( motor->raw_positive_limit.analog );
			    positive_limit = offset + scale *
				( motor->raw_negative_limit.analog );
			}
			break;
		default:
			fprintf(output,"Unknown motor subclass %d.\n",
				motor->subclass);
			FREE_MOTOR_NAME_ARRAY;
			return FAILURE;
		}

		fprintf(output,
		    "Software limits for %s are %.*g %s to %.*g %s.\n",
			record->name,
			record->precision,
			negative_limit, motor->units,
			record->precision,
			positive_limit, motor->units );

		/* Prompt for the scan start position. */

		if ( scan_num_motors == 1 ) {
			sprintf( prompt, "Enter start position -> " );
		} else {
			sprintf( prompt,
			"Enter start position for motor '%s' -> ",
				record->name );
		}

		if ( quick_scan == NULL ) {
			default_double = 0.0;
		} else {
			if ( j < old_scan->num_motors ) {
				default_double
				    = quick_scan->start_position[j];
			} else {
				default_double = 0.0;
			}
		}

		status = motor_get_double( output, prompt,
			TRUE, default_double,
			&scan_start[j],
			negative_limit, positive_limit );

		if ( status != SUCCESS ) {
			FREE_MOTOR_NAME_ARRAY;
			return status;
		}

		/* Prompt for the step size. */

		if ( scan_num_motors == 1 ) {
			sprintf( prompt,
				"Enter step size -> " );
		} else {
			sprintf( prompt,
			"Enter step size for motor '%s' -> ",
				record->name );
		}
		if ( quick_scan == NULL ) {
			default_double = 1.0;
		} else {
			if ( j < old_scan->num_motors ) {

				default_double = mx_divide_safely(
		quick_scan->end_position[j] - quick_scan->start_position[j],
			quick_scan->requested_num_measurements - 1L );

			} else {
				default_double = 1.0;
			}
		}

		status = motor_get_double(output, prompt,
			TRUE, default_double,
			&scan_step_size,
			-DBL_MAX, DBL_MAX );

		if ( status != SUCCESS ) {
			FREE_MOTOR_NAME_ARRAY;
			return status;
		}

		/* Prompt for the scan end position. */

		if ( scan_num_motors == 1 ) {
			sprintf( prompt,
				"Enter end position -> " );
		} else {
			sprintf( prompt,
			"Enter end position for motor '%s' -> ",
				record->name );
		}
		if ( quick_scan == NULL ) {
			default_double = 1.0;
		} else {
			if ( j < old_scan->num_motors ) {
				default_double = quick_scan->end_position[j];
			} else {
				default_double = 0.0;
			}
		}
		status = motor_get_double(output, prompt,
			TRUE, default_double,
			&scan_end[i],
			negative_limit, positive_limit );

		if ( status != SUCCESS ) {
			FREE_MOTOR_NAME_ARRAY;
			return status;
		}

		/* Make sure that scan_step_size has a sign that is compatible
		 * with the values of scan_start[j] and scan_end[j].
		 */

		if ( scan_end[j] >= scan_start[j] ) {
			scan_step_size = fabs( scan_step_size );
		} else {
			scan_step_size = - fabs( scan_step_size );
		}

		/* Compute the number of measurements from the above numbers,
		 * rounding up, and then recompute the scan end position
		 * accordingly.
		 */

		raw_num_measurements = 1.0 + fabs( mx_divide_safely(
						scan_end[i] - scan_start[j],
						    scan_step_size ) );

		/* Round up to the next integer. */

		scan_num_measurements[j]
		    = mx_round_away_from_zero( raw_num_measurements, 1.0e-6 );

		/* Compute the new end position. */

		scan_end[j] = scan_start[j] + scan_step_size * (double)
					( scan_num_measurements[j] - 1L );

		/* Tell the user what we have done. */

		fprintf( output,
	"\n    %s: %g %s to %g %s, %g %s step size, %ld measurements\n\n",
			record->name,
			scan_start[i], motor->units,
			scan_end[i], motor->units,
			scan_step_size, motor->units,
			scan_num_measurements[j] );

#if 0		/* This is the old way of doing things. */

		if ( scan_num_motors == 1 ) {
			sprintf( prompt,
			"Enter number of measurements -> " );
		} else {
			sprintf( prompt,
	"Enter number of measurements for motor '%s' -> ",
				record->name );
		}
		if ( quick_scan == NULL ) {
			default_long = 1;
		} else {
			if ( j < old_scan->num_motors ) {
				default_long
			    = quick_scan->requested_num_measurements;
			} else {
				default_long = 1;
			}
		}
		status = motor_get_long( output, prompt,
			TRUE, default_long,
			&scan_num_measurements[j],
			0, 100000000 );

		if ( status != SUCCESS ) {
			FREE_MOTOR_NAME_ARRAY;
			return status;
		}
#endif
	}
	FREE_MOTOR_NAME_ARRAY;

	/* Prompt for the common scan parameters such as input devices,
	 * data files and plot types.
	 */

	if ( scan_type == MXS_QUI_JOERGER ) {
		fprintf( output,
"\n"
"For Joerger quick scans, the first input device must be the Joerger timer,\n"
"while the other input devices must be Joerger scalers.\n\n" );
	}

	string_length = strlen(record_description_buffer);
	buffer_left = record_description_buffer_length - string_length;

	first_input_device_record = NULL;

	if ( input_devices_string != NULL ) {
		strncat( record_description_buffer,
			input_devices_string, buffer_left );
	} else {
		status = motor_setup_input_devices( old_scan,
						scan_class, scan_type,
						buffer, sizeof(buffer),
						&first_input_device_record );
		if ( status != SUCCESS )
			return status;
		strncat( record_description_buffer, buffer, buffer_left );
	}

	/* Figure out the name of the clock to be used for this scan. */

	if ( old_scan != (MX_SCAN *) NULL ) {

	    num_items = sscanf( old_scan->measurement_arguments, "%lg %s",
				&old_measurement_time,
				default_clock_name );

	    if ( num_items != 2 ) {
		fprintf(output,
			"%s: Cannot find the old clock name for scan '%s' "
			"in the measurement arguments string '%s'",
			fname, old_scan->record->name,
			old_scan->measurement_arguments);
		return FAILURE;
	    }
	} else {
	    switch ( scan_type ) {
	    case MXS_QUI_MCS:
	    case MXS_QUI_APS_ID:
		if ( first_input_device_record == NULL ) {
			strcpy( default_clock_name, "timer1" );
		} else {
			MX_DEBUG( 2,("%s: first_input_device_record = '%s'",
				fname, first_input_device_record->name ));

			if (first_input_device_record->mx_type != MXT_SCL_MCS){
				fprintf( output,
	"The input device '%s' mentioned for this scan is not an MCS scaler.",
					first_input_device_record->name );
					
				return FAILURE;
			}
			
			mcs_scaler = (MX_MCS_SCALER *)
				first_input_device_record->record_type_struct;

			if ( mcs_scaler == (MX_MCS_SCALER *) NULL ) {
				fprintf(output,
		"%s: MX_MCS_SCALER pointer for input device '%s' is NULL.\n",
					fname, first_input_device_record->name);
				return FAILURE;
			}
			if ( mcs_scaler->mcs_record == (MX_RECORD *) NULL ) {
				fprintf(output,
		"%s: mcs_record pointer for MCS scaler '%s' is NULL.\n",
					fname, first_input_device_record->name);
				return FAILURE;
			}

			mcs = (MX_MCS *)
				mcs_scaler->mcs_record->record_class_struct;

			if ( mcs == (MX_MCS *) NULL ) {
				fprintf(output,
		"%s: MX_MCS pointer for MCS record '%s' is NULL.\n",
					fname, mcs_scaler->mcs_record->name );
				return FAILURE;
			}
			if ( mcs->timer_record == (MX_RECORD *) NULL ) {
				fprintf(output,
		"%s: No timer record has been configured for MCS '%s'.\n",
					fname, mcs->record->name );
				return FAILURE;
			}

			strcpy( default_clock_name, mcs->timer_record->name );
		}
		break;
	    default:
		strcpy( default_clock_name, "timer1" );
		break;
	    }
	}

	MX_DEBUG( 2,("%s: default_clock_name = '%s'",
				fname, default_clock_name));

	valid_clock_name = FALSE;

	while ( valid_clock_name == FALSE ) {

		sprintf(prompt, "Enter timer or pulse generator name -> ");

		string_length = sizeof( clock_name ) - 1;

		status = motor_get_string( output, prompt,
					default_clock_name,
					&string_length, clock_name );

		if ( status != SUCCESS )
			return status;

		clock_record = mx_get_record(
					motor_record_list, clock_name );

		if ( clock_record == (MX_RECORD *) NULL ) {
			fprintf(output, "%s: Record '%s' does not exist.\n",
				fname, clock_name );

			continue;	/* cycle the while() loop. */
		}

		if ( clock_record->mx_superclass != MXR_DEVICE ) {
			fprintf(output,
		"%s: Record '%s' is not a timer or pulse generator record.\n",
				fname, clock_record->name );

			continue;	/* cycle the while() loop. */
		}

		switch( clock_record->mx_class ) {
		case MXC_TIMER:
		case MXC_PULSE_GENERATOR:
			valid_clock_name = TRUE;
			break;
		default:
			fprintf(output,
		"%s: Record '%s' is not a timer or pulse generator record.\n",
				fname, clock_record->name );

			continue;	/* cycle the while() loop. */
		}
	}

	MX_DEBUG( 2,("%s: clock_name = '%s'", fname, clock_name));

	/* The after scan action is currently hardcoded as 0. */

	string_length = strlen(record_description_buffer);
	buffer_left = record_description_buffer_length - string_length;

	strncat( record_description_buffer, "0 ", buffer_left );

	/* Prompt for the measurement time. */

	if ( quick_scan == NULL ) {
		default_double = 0.0;
	} else {
		if ( j < old_scan->num_motors ) {
			default_double =
				mx_quick_scan_get_measurement_time(quick_scan);
		} else {
			default_double = 0.0;
		}
	}

	status = motor_get_double( output, "Enter measurement time -> ",
		TRUE, default_double,
		&scan_measurement_time,
		0.0, DBL_MAX );

	if ( status != SUCCESS )
		return status;

	if ( clock_record->mx_class == MXC_PULSE_GENERATOR ) {

		sprintf( buffer, "0 preset_pulse_period \"%g %s\" ",
				scan_measurement_time, clock_name );
	} else {
		sprintf( buffer, "0 preset_time \"%g %s\" ",
				scan_measurement_time, clock_name );
	}

	string_length = strlen(record_description_buffer);
	buffer_left = record_description_buffer_length - string_length;

	strncat( record_description_buffer, buffer, buffer_left );

	/* Prompt for the datafile and plot parameters. */

	string_length = strlen(record_description_buffer);
	buffer_left = record_description_buffer_length - string_length;

	if ( datafile_and_plot_parameters_string != NULL ) {
		strncat( record_description_buffer,
			datafile_and_plot_parameters_string, buffer_left );
	} else {
		status = motor_setup_datafile_and_plot_parameters( old_scan,
						scan_class, scan_type,
						buffer, sizeof(buffer) );
		if ( status != SUCCESS )
			return status;
		strncat( record_description_buffer, buffer, buffer_left );
	}

	/* Add the scan start, scan end, and num measurements information
	 * to the record description that we prompted for earlier.
	 */

	for ( j = 0; j < scan_num_independent_variables; j++ ) {
		string_length = strlen(record_description_buffer);
		buffer_left = record_description_buffer_length - string_length;

		sprintf( buffer, " %.*g", default_precision, scan_start[j] );

		if ( strlen(buffer) < buffer_left ) {
			strcat( record_description_buffer, buffer );
		} else {
			fprintf( output,
			"%s: Ran out of record description buffer space.\n",
				fname);
			fprintf( output,
			"Perhaps %ld motors in the scan is too many to fit.\n",
				scan_num_motors );
		}
	}

	for ( j = 0; j < scan_num_independent_variables; j++ ) {

		string_length = strlen(record_description_buffer);
		buffer_left = record_description_buffer_length - string_length;

		sprintf( buffer, " %.*g",
			default_precision, scan_end[j] );

		if ( strlen(buffer) < buffer_left ) {
			strcat( record_description_buffer, buffer );
		} else {
			fprintf( output,
			"%s: Ran out of record description buffer space.\n",
				fname);
			fprintf( output,
			"Perhaps %ld motors in the scan is too many to fit.\n",
				scan_num_motors );
		}
	}

	for ( j = 0; j < scan_num_independent_variables; j++ ) {
		string_length = strlen(record_description_buffer);
		buffer_left = record_description_buffer_length - string_length;

		sprintf( buffer, " %ld", scan_num_measurements[j] );

		if ( strlen(buffer) < buffer_left ) {
			strcat( record_description_buffer, buffer );
		} else {
			fprintf( output,
			"%s: Ran out of record description buffer space.\n",
				fname);
			fprintf( output,
			"Perhaps %ld motors in the scan is too many to fit.\n",
				scan_num_motors );
		}
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

