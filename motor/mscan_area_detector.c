/*
 * Name:    mscan_area_detector.c
 *
 * Purpose: Motor area detector scan setup and modify function.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2004-2006, 2009-2011, 2013, 2015-2016
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define USE_NUM_MEASUREMENTS	FALSE

/* If old_scan == NULL, we are doing a new scan setup.  Otherwise, we are
 * modifying a previous scan.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "motor.h"
#include "mdialog.h"
#include "mx_array.h"
#include "mx_scan_area_detector.h"
#include "sa_wedge.h"

#define FREE_MOTOR_NAME_ARRAY \
	do { \
		if ( scan_num_motors > 0 ) { \
			(void) mx_free_array( motor_name_array ); \
			motor_name_array = NULL; \
		} \
	} while(0)

int
motor_setup_area_detector_scan_parameters(
	char *scan_name,
	MX_SCAN *old_scan,
	char *record_description_buffer,
	size_t record_description_buffer_length,
	char *input_devices_string,
	char *measurement_parameters_string,
	char *datafile_and_plot_parameters_string )
{
	static const char fname[] =
			"motor_setup_area_detector_scan_parameters()";

	MX_RECORD *record;
	MX_MOTOR *motor;
	MX_RECORD **motor_record_array;
	MX_RECORD *old_scan_record;
	MX_AREA_DETECTOR_SCAN *area_detector_scan;
	MX_WEDGE_SCAN *wedge_scan;
	MX_LIST_HEAD *list_head_struct;
	mx_status_type mx_status;
	static char buffer[ MXU_RECORD_DESCRIPTION_LENGTH + 1 ];
	char prompt[100];
	double end_position, negative_limit, positive_limit;
	double scale, offset;
	int i, j, status;
	int valid_end_position;
	long scan_class, scan_type;
	long scan_num_scans;
	long scan_num_independent_variables;
	long scan_num_motors;
	long old_scan_num_motors;
	unsigned long scan_flags;
	double *scan_start;
	double *scan_step_size;
	long *scan_num_frames;
	int *motor_is_independent_variable;
	long use_inverse_beam, number_of_energies;
	char *default_string_ptr;
	int string_length;
	MX_RECORD *energy_record;
	long old_number_of_energies;
	double *old_energy_array = NULL;
	double new_energy, wedge_size;

#if ( USE_NUM_MEASUREMENTS == FALSE )
	double requested_end_position, raw_num_frames;
#endif

	char **motor_name_array;
	long motor_name_dimension_array[2];

	static size_t name_element_size_array[2] = {
		sizeof(char), sizeof(char *) };

	int default_precision;
	long default_long;
	double default_double;

	list_head_struct = mx_get_record_list_head_struct( motor_record_list );

	if ( list_head_struct == (MX_LIST_HEAD *) NULL ) {
		fprintf(output, "MX_LIST_HEAD structure pointer is NULL!\n");

		return FAILURE;
	}

	default_precision = (int) list_head_struct->default_precision;

	/* Get the scan type. */

	scan_class = MXS_AREA_DETECTOR_SCAN;

	if ( old_scan == (MX_SCAN *) NULL ) {
		fprintf(output,
			"Select scan type:\n"
			"    1.  Wedge scan\n"
			"\n");

		status = motor_get_long( output, "--> ", TRUE, 1,
						&scan_type, 1, 1 );

		if ( status == FAILURE ) {
			return FAILURE;
		}
		scan_type += (MXS_AD_WEDGE - 1);
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
	case MXS_AD_WEDGE:
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

	snprintf( record_description_buffer, record_description_buffer_length,
			"%s scan ad_scan_class ", scan_name);

	switch( scan_type ) {
	case MXS_AD_WEDGE:
		strlcat( record_description_buffer,
			"wedge_scan \"\" \"\" ",
			record_description_buffer_length );
		break;
	}

	snprintf( buffer, sizeof(buffer),
			"%ld %ld %ld ", scan_num_scans,
			scan_num_independent_variables, scan_num_motors );

	strlcat( record_description_buffer, buffer,
			record_description_buffer_length );

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

		scan_step_size = (double *)
		    malloc( scan_num_independent_variables * sizeof(double));

		if ( scan_step_size == NULL ) {
			fprintf( output,
			"%s: Out of memory allocating scan step size array.\n",
				fname );
			return FAILURE;
		}

		scan_num_frames = (long *)
		    malloc( scan_num_independent_variables * sizeof(long));

		if ( scan_num_frames == NULL ) {
			fprintf( output,
		"%s: Out of memory allocating scan num frames array.\n",
				fname );
			return FAILURE;
		}
	}

	if ( scan_num_motors < 0 ) {
		fprintf( output,
		"%s: This scan has %ld motors.  That can't be right.\n",
			fname, scan_num_motors );
		return FAILURE;

	} else if ( scan_num_motors == 0 ) {
		motor_is_independent_variable = NULL;
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

	switch( scan_type ) {
	default:
		for ( i = 0; i < scan_num_motors; i++ ) {
			motor_is_independent_variable[i] = TRUE;
		}
		break;
	}

	/*---*/

	if ( old_scan == (MX_SCAN *) NULL ) {
		area_detector_scan = NULL;
	} else {
		area_detector_scan = (MX_AREA_DETECTOR_SCAN *)
				(old_scan->record->record_class_struct);
	}

	if ( scan_num_motors <= 0 ) {
		scan_start[0] = 0.0;
		scan_step_size[0] = 1.0;
		motor_name_array = NULL;

		if ( area_detector_scan == NULL ) {
			default_long = 1;
		} else {
			default_long = area_detector_scan->num_frames[0];
		}

		status = motor_get_long( output,
			"Enter number of frames -> ",
			TRUE, default_long,
			&scan_num_frames[0], 0, 100000000 );

		if ( status != SUCCESS )
			return status;
	} else {
		if ( old_scan == (MX_SCAN *) NULL ) {
			motor_record_array = NULL;
			old_scan_num_motors = 0;
		} else {
			motor_record_array = old_scan->motor_record_array;
			old_scan_num_motors = old_scan->num_motors;
		}

		motor_name_dimension_array[0] = scan_num_motors;
		motor_name_dimension_array[1] = MXU_RECORD_NAME_LENGTH + 1;

		motor_name_array = (char **) mx_allocate_array(
			MXFT_STRING,
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
				fprintf(output,"Unknown motor subclass %ld.\n",
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

			if ( scan_num_motors == 1 ) {
				snprintf( prompt, sizeof(prompt),
				"Enter scan start position -> " );
			} else {
				snprintf( prompt, sizeof(prompt),
				"Enter scan start position for motor '%s' -> ",
					record->name );
			}

			if ( area_detector_scan == NULL ) {
				default_double = 0.0;
			} else {
				if ( j < old_scan->num_motors ) {
					default_double
				    = area_detector_scan->start_position[j];
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

			valid_end_position = FALSE;

			while ( ! valid_end_position ) {
				if ( scan_num_motors == 1 ) {
					snprintf( prompt, sizeof(prompt),
						"Enter step size -> " );
				} else {
					snprintf( prompt, sizeof(prompt),
					"Enter step size for motor '%s' -> ",
						record->name );
				}
				if ( area_detector_scan == NULL ) {
					default_double = 1.0;
				} else {
					if ( j < old_scan->num_motors ) {
						default_double
					  = area_detector_scan->step_size[j];
					} else {
						default_double = 0.0;
					}
				}
				status = motor_get_double(output, prompt,
					TRUE, default_double,
					&scan_step_size[j],
					-(positive_limit - negative_limit),
					(positive_limit - negative_limit) );

				if ( status != SUCCESS ) {
					FREE_MOTOR_NAME_ARRAY;
					return status;
				}

#if USE_NUM_MEASUREMENTS
				if ( scan_num_motors == 1 ) {
					snprintf( prompt, sizeof(prompt),
					"Enter number of frames -> " );
				} else {
					snprintf( prompt, sizeof(prompt),
			"Enter number of frames for motor '%s' -> ",
						record->name );
				}
				if ( area_detector_scan == NULL ) {
					default_long = 1;
				} else {
					if ( j < old_scan->num_motors ) {
						default_long
					    = area_detector_scan->num_frames[j];
					} else {
						default_long = 1;
					}
				}
				status = motor_get_long( output, prompt,
					TRUE, default_long,
					&scan_num_frames[j],
					0, 100000000 );

				if ( status != SUCCESS ) {
					FREE_MOTOR_NAME_ARRAY;
					return status;
				}

#else /* do not USE_NUM_MEASUREMENTS */

				if ( scan_num_motors == 1 ) {
					snprintf( prompt, sizeof(prompt),
					"Enter scan end position -> " );
				} else {
					snprintf( prompt, sizeof(prompt),
			"Enter scan end position for motor '%s' -> ",
						record->name );
				}

				raw_num_frames = 0.0;

				if ( area_detector_scan == NULL ) {
					default_double = scan_start[j];
				} else {
					if ( j < old_scan->num_motors ) {

						raw_num_frames =
				(double)(area_detector_scan->num_frames[j]);

						default_double = scan_start[j]
							+ scan_step_size[j]
						* (raw_num_frames - 1.0);

					} else {
						default_double = scan_start[j];
					}
				}

				if ( scan_step_size[j] >= 0.0 ) {
					status = motor_get_double(
						output, prompt,
						TRUE, default_double,
						&requested_end_position,
						scan_start[j],
						positive_limit );
				} else {
					status = motor_get_double(
						output, prompt,
						TRUE, default_double,
						&requested_end_position,
						negative_limit,
						scan_start[j] );
				}

				if ( status != SUCCESS ) {
					FREE_MOTOR_NAME_ARRAY;
					return status;
				}

				/* Recompute the number of frames for
				 * this scan and round up to the next
				 * integer.
				 */

				raw_num_frames = 1.0 + mx_divide_safely(
					requested_end_position - scan_start[j],
						scan_step_size[j] );

				scan_num_frames[j] =
			mx_round_away_from_zero( raw_num_frames, 1.0e-6 );

#endif /* USE_NUM_MEASUREMENTS */

				end_position = scan_start[j]
				    + scan_step_size[j]
				    * ((double)(scan_num_frames[j])-1.0);

				if ( end_position < negative_limit 
				 ||  end_position > positive_limit ) {
					fprintf(output,
"Computed scan end position %.*g is outside allowed range (%.*g to %.*g).\n",
					record->precision,
					end_position,
					record->precision,
					negative_limit,
					record->precision,
					positive_limit);
				} else {
					valid_end_position = TRUE;
				}
			}
#if 1
			fprintf( output,
	"\n    %s: %g %s to %g %s, %g %s step size, %ld frames\n\n",
				record->name,
				scan_start[j], motor->units,
				end_position, motor->units,
				scan_step_size[j], motor->units,
				scan_num_frames[j] );
#endif
		}
		FREE_MOTOR_NAME_ARRAY;
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
						buffer, sizeof(buffer), TRUE );
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

	/* Prompt for energy related parameters. */

	if ( old_scan == NULL ) {
		default_long = 1;
	} else {
		default_long = area_detector_scan->use_inverse_beam;
	}

	snprintf( prompt, sizeof(prompt),
		"Use inverse beam ( 0 = FALSE, 1 = TRUE ) -> " );

	status = motor_get_long( output, prompt,
				TRUE, default_long,
				&use_inverse_beam,
				0, 1 );

	if ( status != SUCCESS )
		return status;

	if ( old_scan == NULL ) {
		default_string_ptr = NULL;
	} else {
		default_string_ptr = area_detector_scan->energy_record->name;
	}

	snprintf( prompt, sizeof(prompt), "Name of the energy record -> " );

	string_length = sizeof(buffer);

	status = motor_get_string( output, prompt,
			default_string_ptr,
			&string_length, buffer );

	if ( status != SUCCESS )
		return status;

	energy_record = mx_get_record( motor_record_list, buffer );

	if ( energy_record == NULL ) {
		fprintf( output, "The record '%s' does not exist.\n", buffer );
		return FAILURE;
	}

	if ( old_scan == NULL ) {
		default_long = 0;
	} else {
		default_long = area_detector_scan->num_energies;
	}

	snprintf( prompt, sizeof(prompt), "Number of energies -> " );

	status = motor_get_long( output, prompt,
				TRUE, default_long,
				&number_of_energies,
				0, 1000000 );

	if ( status != SUCCESS )
		return status;

	snprintf( buffer, sizeof(buffer), "%ld %s %ld ",
		use_inverse_beam, energy_record->name, number_of_energies );

	strlcat( record_description_buffer, buffer,
			record_description_buffer_length );

	/* Prompt for the energies used by this scan. */

	if ( old_scan == NULL ) {
		old_number_of_energies = 0;
	} else {
		old_number_of_energies = area_detector_scan->num_energies;

		old_energy_array = area_detector_scan->energy_array;
	}

	MXW_UNUSED( old_number_of_energies );

	for ( i = 0; i < number_of_energies; i++ ) {
		if ( old_energy_array == NULL ) {
			default_double = 0;
		} else {
			default_double = old_energy_array[i];
		}

		snprintf( prompt, sizeof(prompt),
			"Energy %d value -> ", i );

		status = motor_get_double( output, prompt,
					TRUE, default_double,
					&new_energy, 0.0, 1.0e30 );

		if ( status != SUCCESS )
			return status;

		snprintf( buffer, sizeof(buffer), "%g ", new_energy );

		strlcat( record_description_buffer, buffer,
				record_description_buffer_length );
	}

	/* FIXME: Hardcode the num_static_motors as 0. */

	strlcat( record_description_buffer, "0 ",
				record_description_buffer_length );

	/* Add the scan start, step size, and num frames information
	 * to the record description that we prompted for earlier.
	 */

	for ( j = 0; j < scan_num_independent_variables; j++ ) {
		snprintf( buffer, sizeof(buffer),
				" %.*g", default_precision, scan_start[j] );

		strlcat( record_description_buffer, buffer,
					record_description_buffer_length );
	}

	for ( j = 0; j < scan_num_independent_variables; j++ ) {
		snprintf( buffer, sizeof(buffer),
				" %.*g", default_precision, scan_step_size[j] );

		strlcat( record_description_buffer, buffer,
					record_description_buffer_length );
	}

	for ( j = 0; j < scan_num_independent_variables; j++ ) {
		snprintf( buffer, sizeof(buffer),
				" %ld", scan_num_frames[j] );

		strlcat( record_description_buffer, buffer,
					record_description_buffer_length );
	}

	/* Add scan type specific parameters */

	switch( scan_type ) {
	case MXS_AD_WEDGE:
		if ( old_scan == (MX_SCAN *) NULL ) {
			default_double = 0;
		} else {
			wedge_scan = (MX_WEDGE_SCAN *)
				old_scan->record->record_type_struct;

			default_double = wedge_scan->wedge_size;
		}

		status = motor_get_double( output, "Enter wedge size -> ",
					TRUE, default_double,
					&wedge_size, 0.0, 1.0e30 );

		if ( status != SUCCESS )
			return status;

		snprintf( buffer, sizeof(buffer), " %g ", wedge_size );

		strlcat( record_description_buffer, buffer,
				record_description_buffer_length );
		break;
	}

#if 1
	fprintf(output, "%s: record_description_buffer = '%s'\n",
			fname, record_description_buffer);
#endif

	/* Delete the old scan if it exists. */

	if ( old_scan != (MX_SCAN *) NULL ) {

		mx_status = mx_delete_record( old_scan->record );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;
	} else {
		old_scan_record = mx_get_record( motor_record_list, scan_name );

		if ( old_scan_record != (MX_RECORD *) NULL ) {

			if ( old_scan_record->mx_superclass == MXR_SCAN ) {
				mx_status = mx_delete_record( old_scan_record );

				if ( mx_status.code != MXE_SUCCESS )
					return FAILURE;
			} else {
				fprintf( output,
				"The requested scan name '%s' conflicts with "
				"an existing MX record which is not a scan.  "
				"Pick a different name for your scan.\n",
					scan_name );

				return FAILURE;
			}
		}
	}

	/* Add the scan to the record list. */

	mx_status = mx_create_record_from_description( motor_record_list,
		record_description_buffer, &record, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	mx_status = mx_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	mx_status = mx_open_hardware( record );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	mx_status = mx_finish_delayed_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	return SUCCESS;
}

