/*
 * Name:    mscan_xafs.c
 *
 * Purpose: XAFS scan setup and modify function.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2001 Illinois Institute of Technology
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
#include <ctype.h>
#include <math.h>

#include "motor.h"
#include "mdialog.h"
#include "mx_array.h"
#include "mx_variable.h"
#include "mx_scan_xafs.h"
#include "sxafs_std.h"

#define FREE_REGION_ARRAYS \
		do { \
			if ( region_boundary != NULL ) { \
				free( region_boundary ); \
			} \
			if ( region_step_size != NULL ) { \
				free( region_step_size ); \
			} \
			if ( region_measurement_time != NULL ) { \
				free( region_measurement_time ); \
			} \
		} while (0)

int
motor_setup_xafs_scan_parameters(
	char *scan_name,
	MX_SCAN *old_scan,
	char *record_description_buffer,
	size_t record_description_buffer_length,
	char *input_devices_string,
	char *measurement_parameters_string,
	char *datafile_and_plot_parameters_string )
{
	MX_RECORD *record;
	MX_RECORD *timer_record;
	MX_XAFS_SCAN *old_xafs_scan;
	mx_status_type mx_status;
	static char buffer[ MXU_RECORD_DESCRIPTION_LENGTH + 1 ];
	static char timer_name[ MXU_RECORD_NAME_LENGTH + 1 ];
	static char old_timer_name[ MXU_RECORD_NAME_LENGTH + 1 ];
	char prompt[100];
	char format_buffer[20];
	char region_type_string[5];
	char *ptr;
	int status, string_length, num_items;
	size_t buffer_left;
	long scan_class, scan_type;
	long i, j, n;
	long num_regions, num_boundaries;
	long num_energy_regions, num_k_regions;
	long scan_num_scans;
	double scan_settling_time;
	double *region_boundary, *region_step_size, *region_measurement_time;

	long default_long;
	double default_double;

	region_boundary = NULL;
	region_step_size = NULL;
	region_measurement_time = NULL;

	i = j = n = 0L;

	if ( old_scan == (MX_SCAN *) NULL ) {
		old_xafs_scan = NULL;
	} else {
		old_xafs_scan = (MX_XAFS_SCAN *)
				(old_scan->record->record_class_struct);

		if ( old_xafs_scan == (MX_XAFS_SCAN *) NULL ) {
			fprintf( output,
			"MX_XAFS_SCAN pointer for old scan '%s' is NULL.",
				old_scan->record->name );
			return FAILURE;
		}
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

	/* Format the beginning of the record description. */

	sprintf( record_description_buffer,
		"%s scan xafs_scan_class xafs_scan \"\" \"\" %ld 0 0 ",
		scan_name, scan_num_scans );

	/* Prompt for the common scan parameters such as input devices,
	 * data files and plot types.
	 */

	scan_class = MXS_XAFS_SCAN;
	scan_type = MXS_XAF_STANDARD;

	string_length = strlen(record_description_buffer);
	buffer_left = record_description_buffer_length - string_length;

	if ( input_devices_string != NULL ) {
		strncat( record_description_buffer,
			input_devices_string, buffer_left );
	} else {
		status = motor_setup_input_devices( old_scan,
						scan_class, scan_type,
						buffer, sizeof(buffer), NULL );
		if ( status != SUCCESS )
			return status;
		strncat( record_description_buffer, buffer, buffer_left );
	}

	/* The after scan action is currently hardcoded as 0. */

	string_length = strlen(record_description_buffer);
	buffer_left = record_description_buffer_length - string_length;

	strncat( record_description_buffer, "0 ", buffer_left );

	/* Prompt for the settling time. */

	string_length = strlen(record_description_buffer);
	buffer_left = record_description_buffer_length - string_length;

	if ( measurement_parameters_string != NULL ) {
		strncat( record_description_buffer,
			measurement_parameters_string, buffer_left );
	} else {
		if ( old_scan == NULL ) {
			default_double = 0.0;
		} else {
			default_double = old_scan->settling_time;
		}
		status = motor_get_double( output,
			"Enter settling time in seconds -> ",
			TRUE, default_double,
			&scan_settling_time, 0.0, 1.0e30 );

		if ( status != SUCCESS )
			return status;

		ptr = record_description_buffer + string_length;

#if 0
		sprintf( ptr, "%g preset_time \"0 timer1\" ",
					scan_settling_time );
#else
		sprintf( ptr, "%g preset_time ", scan_settling_time );

		string_length = strlen(record_description_buffer);

		ptr = record_description_buffer + string_length;

		if ( old_scan == NULL ) {
			strcpy( old_timer_name, "timer1" );
		} else {
			sprintf( format_buffer, "%%lg %%%ds",
						MXU_RECORD_NAME_LENGTH );

			num_items = sscanf(
				old_scan->measurement.measurement_arguments,
				format_buffer,
				&default_double,
				old_timer_name );

			if ( num_items != 2 ) {
				fprintf( output,
			"Error: Can't parse old measurement arguments '%s'.\n",
				old_scan->measurement.measurement_arguments );

				return FAILURE;
			}
		}

		status = motor_get_string( output,
				"Enter timer name -> ",
				old_timer_name, &string_length, timer_name );

		if ( status != SUCCESS )
			return status;

		/* Does the requested timer exist? */

		timer_record = mx_get_record( motor_record_list, timer_name );

		if ( timer_record == (MX_RECORD *) NULL ) {
			fprintf( output,
				"Error: The timer '%s' does not exist.\n",
				timer_name );

			return FAILURE;
		}

		if ( ( timer_record->mx_superclass != MXR_DEVICE )
		  || ( timer_record->mx_class != MXC_TIMER ) )
		{
			fprintf( output,
			  "Error: The record '%s' is not a timer record.\n",
				timer_record->name );

			return FAILURE;
		}

		sprintf( ptr, "\"0 %s\" ", timer_record->name );
#endif
	}

	/* Prompt for the datafile and plot parameters. */

	string_length = strlen(record_description_buffer);
	buffer_left = record_description_buffer_length - string_length;

	if ( datafile_and_plot_parameters_string != NULL ) {
		strncat( record_description_buffer,
			datafile_and_plot_parameters_string, buffer_left );
	} else {
		status = motor_setup_datafile_and_plot_parameters( old_scan,
					MXS_XAFS_SCAN, MXS_XAF_STANDARD,
					buffer, sizeof(buffer) );
		if ( status != SUCCESS )
			return status;
		strncat( record_description_buffer, buffer, buffer_left );
	}

	/* Prompt for the number of regions, etc. */

	if ( old_xafs_scan == NULL ) {
		default_long = 1;
	} else {
		default_long = old_xafs_scan->num_energy_regions;
	}

	status = motor_get_long( output,
		"Enter number of energy regions -> ",
		TRUE, default_long,
		&num_energy_regions, 1, 1000000 );

	if ( status != SUCCESS ) {
		return status;
	}

	if ( old_xafs_scan == NULL ) {
		default_long = 1;
	} else {
		default_long = old_xafs_scan->num_k_regions;
	}

	status = motor_get_long( output,
		"Enter number of k regions -> ",
		TRUE, default_long,
		&num_k_regions, 0, 1000000 );

	if ( status != SUCCESS ) {
		return status;
	}

	num_regions = num_energy_regions + num_k_regions;
	num_boundaries = num_regions + 1;

	string_length = strlen(record_description_buffer);
	ptr = record_description_buffer + string_length;

	sprintf( ptr, "%ld %ld %ld %ld ",
	    num_regions, num_energy_regions, num_k_regions, num_boundaries );

	/* Allocate arrays to hold the new region boundaries, step sizes,
	 * and measurement times.
	 */

	region_boundary = (double *) malloc(num_boundaries * sizeof(double));

	if ( region_boundary == (double *) NULL ) {
		fprintf(output,
	    "Out of memory trying to allocate new region boundary array.\n");
		FREE_REGION_ARRAYS;
		return FAILURE;
	}

	region_step_size = (double *) malloc(num_regions * sizeof(double));

	if ( region_step_size == (double *) NULL ) {
		fprintf(output,
	    "Out of memory trying to allocate new region step size array.\n");
		FREE_REGION_ARRAYS;
		return FAILURE;
	}

	region_measurement_time = (double *)
				malloc(num_regions * sizeof(double));

	if ( region_measurement_time == (double *) NULL ) {
		fprintf(output,
    "Out of memory trying to allocate new region measurement_time array.\n");
		FREE_REGION_ARRAYS;
		return FAILURE;
	}

	/* Prompt for the first region's start position. */

	fprintf( output, "\n" );

	if ( old_xafs_scan == NULL ) {
		default_double = 0.0;
	} else {
		default_double = old_xafs_scan->region_boundary[0];
	}

	status = motor_get_double( output,
		"Enter start of first E-E0 region -> ",
		TRUE, default_double,
		&(region_boundary[0]), -1.0e30, 1.0e30 );

	if ( status != SUCCESS ) {
		FREE_REGION_ARRAYS;
		return FAILURE;
	}

	for ( i = 0; i < num_regions; i++ ) {

		/* Compute some indices that allow us to find parameters
		 * for the k regions in the old_xafs_scan more easily.
		 * 'j == 0' corresponds to the first k region in the new scan,
		 * while 'n == old_xafs_scan->num_energy_regions' corresponds
		 * to the first k region in the old scan.
		 */

		if ( old_xafs_scan != (MX_XAFS_SCAN *) NULL ) {
			j = i - num_energy_regions;
			n = old_xafs_scan->num_energy_regions + j;
		}

		/* Print region prompt separator. */

		fprintf( output, "\n" );

		strcpy( region_type_string, "" );

		if ( i < num_energy_regions ) {
			strncat( region_type_string, "E-E0",
					sizeof(region_type_string) - 1 );
		} else {
			strncat( region_type_string, "k",
					sizeof(region_type_string) - 1 );
		}

		/* Region end. */

		sprintf( prompt, "Enter end of region %ld (%s) -> ",
						i, region_type_string );

		if ( old_xafs_scan == NULL ) {
			default_double = 0.0;
		} else {
			if ( i < num_energy_regions ) {
				if ( i < old_xafs_scan->num_energy_regions ) {
					default_double
					= old_xafs_scan->region_boundary[i+1];
				} else {
					default_double = 0.0;
				}
			} else {
				if ( j < old_xafs_scan->num_k_regions ) {
					default_double
					= old_xafs_scan->region_boundary[n+1];
				} else {
					default_double = 0.0;
				}
			}
		}

		status = motor_get_double( output, prompt, 
				TRUE, default_double,
				&(region_boundary[i+1]), -1.0e30, 1.0e30 );

		if ( status != SUCCESS ) {
			FREE_REGION_ARRAYS;
			return FAILURE;
		}

		/* Region step size */

		sprintf( prompt, "Enter step size for region %ld (%s) -> ",
						i, region_type_string );

		if ( old_xafs_scan == NULL ) {
			default_double = 0.0;
		} else {
			if ( i < num_energy_regions ) {
				if ( i < old_xafs_scan->num_energy_regions ) {
					default_double
					= old_xafs_scan->region_step_size[i];
				} else {
					default_double = 0.0;
				}
			} else {
				if ( j < old_xafs_scan->num_k_regions ) {
					default_double
					= old_xafs_scan->region_step_size[n];
				} else {
					default_double = 0.0;
				}
			}
		}

		status = motor_get_double( output, prompt,
				TRUE, default_double,
				&(region_step_size[i]), -1.0e30, 1.0e30 );

		if ( status != SUCCESS ) {
			FREE_REGION_ARRAYS;
			return FAILURE;
		}

		/* Region measurement time */

		sprintf( prompt,
			"Enter measurement time for region %ld (sec) -> ", i );

		if ( old_xafs_scan == NULL ) {
			default_double = 0.0;
		} else {
			if ( i < num_energy_regions ) {
				if ( i < old_xafs_scan->num_energy_regions ) {
					default_double
				= old_xafs_scan->region_measurement_time[i];
				} else {
					default_double = 0.0;
				}
			} else {
				if ( j < old_xafs_scan->num_k_regions ) {
					default_double
				= old_xafs_scan->region_measurement_time[n];
				} else {
					default_double = 0.0;
				}
			}
		}

		status = motor_get_double( output, prompt,
			TRUE, default_double,
			&(region_measurement_time[i]), -1.0e30, 1.0e30 );

		if ( status != SUCCESS ) {
			FREE_REGION_ARRAYS;
			return FAILURE;
		}
	}

	for ( i = 0; i < num_boundaries; i++ ) {
		string_length = strlen(record_description_buffer);
		ptr = record_description_buffer + string_length;

		sprintf( ptr, "%g ", region_boundary[i] );
	}
	for ( i = 0; i < num_regions; i++ ) {
		string_length = strlen(record_description_buffer);
		ptr = record_description_buffer + string_length;

		sprintf( ptr, "%g ", region_step_size[i] );
	}
	for ( i = 0; i < num_regions; i++ ) {
		string_length = strlen(record_description_buffer);
		ptr = record_description_buffer + string_length;

		sprintf( ptr, "%g ", region_measurement_time[i] );
	}

	/* Delete the old scan if it exists. */

	if ( old_scan != (MX_SCAN *) NULL ) {

		mx_status = mx_delete_record( old_scan->record );

		if ( mx_status.code != MXE_SUCCESS ) {
			FREE_REGION_ARRAYS;
			return FAILURE;
		}
	}

	FREE_REGION_ARRAYS;

	/* Add the scan to the record list */

	mx_status = mx_create_record_from_description( motor_record_list,
		record_description_buffer, &record, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	mx_status = mx_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	return SUCCESS;
}

