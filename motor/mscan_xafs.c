/*
 * Name:    mscan_xafs.c
 *
 * Purpose: XAFS scan setup and modify function.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2006, 2009, 2015-2016 Illinois Institute of Technology
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
	MX_RECORD *old_scan_record;
	MX_XAFS_SCAN *old_xafs_scan;
	mx_status_type mx_status;
	static char buffer[ MXU_RECORD_DESCRIPTION_LENGTH + 1 ];
	static char timer_name[ MXU_RECORD_NAME_LENGTH + 1 ];
	static char old_timer_name[ MXU_RECORD_NAME_LENGTH + 1 ];
	char prompt[100];
	char format_buffer[80];
	char scan_type_string[40];
	char region_type_string[5];
	char *ptr;
	int status, string_length, num_items;
	long scan_class, scan_type;
	long i, j, n;
	long num_regions, num_boundaries;
	long num_energy_regions, num_k_regions;
	long scan_num_scans;
	unsigned long scan_flags;
	double k_power_law_exponent;
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

	/* Prompt for the XAFS scan type. */

	scan_class = MXS_XAFS_SCAN;
	scan_type = MXS_XAF_STANDARD;

	if ( old_scan == (MX_SCAN *) NULL ) {
		fprintf(output,
			"Select XAFS scan type:\n"
			"    1.  Standard XAFS scan.\n"
			"    2,  XAFS scan with K power law regions\n"
			"\n");

		status = motor_get_long( output, "--> ", TRUE, 1,
						&scan_type, 1, 2 );

		if ( status == FAILURE ) {
			return FAILURE;
		}
		scan_type += (MXS_XAF_STANDARD - 1);
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

	switch( scan_type ) {
	case MXS_XAF_STANDARD:
		strlcpy( scan_type_string, "xafs_scan",
			sizeof(scan_type_string) );
		break;
	case MXS_XAF_K_POWER_LAW:
		strlcpy( scan_type_string, "xafs_k_power_law_scan",
			sizeof(scan_type_string) );
		break;
	default:
		fprintf( output,
			"Error: Unrecognized XAFS scan type %ld\n", scan_type );
		return FAILURE;
		break;
	}

	/* Format the beginning of the record description. */

	snprintf( record_description_buffer,
		record_description_buffer_length,
		"%s scan xafs_scan_class %s \"\" \"\" %ld 0 0 ",
		scan_name, scan_type_string, scan_num_scans );

	/* Prompt for the common scan parameters such as input devices,
	 * data files and plot types.
	 */

	string_length = (int) strlen(record_description_buffer);

	if ( input_devices_string != NULL ) {
		strlcat( record_description_buffer,
				input_devices_string,
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

	/* Prompt for the settling time. */

	if ( measurement_parameters_string != NULL ) {
		strlcat( record_description_buffer,
				measurement_parameters_string,
				record_description_buffer_length );
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

		string_length = (int) strlen(record_description_buffer);

		ptr = record_description_buffer + string_length;

		snprintf( ptr, record_description_buffer_length - string_length,
			"%g preset_time ", scan_settling_time );

		/* Get the K power law exponent (if needed)
		 * and the timer name.
		 */

		string_length = (int) strlen(record_description_buffer);

		k_power_law_exponent = 0.0;

		if ( old_scan == NULL ) {
			strlcpy( old_timer_name, "timer1",
						sizeof(old_timer_name) );
		} else {
			snprintf( format_buffer, sizeof(format_buffer),
					"%%lg %%%ds",
					MXU_RECORD_NAME_LENGTH );

			num_items = sscanf(
				old_scan->measurement.measurement_arguments,
				format_buffer,
				&k_power_law_exponent,
				old_timer_name );

			if ( num_items != 2 ) {
				fprintf( output,
			"Error: Can't parse old measurement arguments '%s'.\n",
				old_scan->measurement.measurement_arguments );

				return FAILURE;
			}
		}

		if ( scan_type == MXS_XAF_K_POWER_LAW ) {

			status = motor_get_double( output,
				"Enter K power law exponent -> ",
				TRUE, k_power_law_exponent,
				&k_power_law_exponent, 0.0, 1.0e30 );

			if ( status != SUCCESS ) {
				return status;
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

		string_length = (int) strlen(record_description_buffer);

		ptr = record_description_buffer + string_length;

		snprintf( ptr, record_description_buffer_length - string_length,
		    "\"%g %s\" ", k_power_law_exponent, timer_record->name );
	}

	/* Prompt for the datafile and plot parameters. */

	if ( datafile_and_plot_parameters_string != NULL ) {
		strlcat( record_description_buffer,
				datafile_and_plot_parameters_string,
				record_description_buffer_length );
	} else {
		status = motor_setup_datafile_and_plot_parameters( old_scan,
					MXS_XAFS_SCAN, MXS_XAF_STANDARD,
					buffer, sizeof(buffer) );
		if ( status != SUCCESS )
			return status;

		strlcat( record_description_buffer, buffer,
				record_description_buffer_length );
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

	if ( scan_type == MXS_XAF_K_POWER_LAW ) {
		num_k_regions = 1;
	} else {
		status = motor_get_long( output,
			"Enter number of k regions -> ",
			TRUE, default_long,
			&num_k_regions, 0, 1000000 );

		if ( status != SUCCESS ) {
			return status;
		}
	}

	num_regions = num_energy_regions + num_k_regions;
	num_boundaries = num_regions + 1;

	string_length = (int) strlen(record_description_buffer);
	ptr = record_description_buffer + string_length;

	snprintf( ptr, record_description_buffer_length - string_length,
		"%ld %ld %ld %ld ",
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

		if ( i < num_energy_regions ) {
			strlcpy( region_type_string, "E-E0",
					sizeof(region_type_string) );
		} else {
			strlcpy( region_type_string, "k",
					sizeof(region_type_string) );
		}

		/* Region end. */

		snprintf( prompt, sizeof(prompt),
				"Enter end of region %ld (%s) -> ",
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

		snprintf( prompt, sizeof(prompt),
			"Enter step size for region %ld (%s) -> ",
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

		snprintf( prompt, sizeof(prompt),
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
		string_length = (int) strlen(record_description_buffer);
		ptr = record_description_buffer + string_length;

		snprintf( ptr, record_description_buffer_length - string_length,
			"%g ", region_boundary[i] );
	}
	for ( i = 0; i < num_regions; i++ ) {
		string_length = (int) strlen(record_description_buffer);
		ptr = record_description_buffer + string_length;

		snprintf( ptr, record_description_buffer_length - string_length,
			"%g ", region_step_size[i] );
	}
	for ( i = 0; i < num_regions; i++ ) {
		string_length = (int) strlen(record_description_buffer);
		ptr = record_description_buffer + string_length;

		snprintf( ptr, record_description_buffer_length - string_length,
			"%g ", region_measurement_time[i] );
	}

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

	FREE_REGION_ARRAYS;

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

