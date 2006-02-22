/*
 * Name:    mccd.c
 *
 * Purpose: CCD control functions.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>

#include "motor.h"
#include "mx_ccd.h"
#include "mx_key.h"

#define VALUES_PER_ROW	10
#define ROWS_PER_PAGE	20

#define VALUES_PER_PAGE ( VALUES_PER_ROW * ROWS_PER_PAGE )

static mx_status_type
motor_wait_for_ccd( MX_RECORD *ccd_record, char *label )
{
	unsigned long busy, ccd_status;
	mx_status_type mx_status;

	busy = 1;

	while( busy ) {
		if ( mx_kbhit() ) {
			(void) mx_getch();

			mx_status = mx_ccd_stop( ccd_record );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			fprintf( output, "%s\n", label );
		}

		mx_status = mx_ccd_get_status(ccd_record, &ccd_status);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		busy = ccd_status & MXSF_CCD_IS_BUSY;

		if ( busy ) {
			mx_msleep(500);
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

int
motor_ccd_fn( int argc, char *argv[] )
{
	static const char cname[] = "ccd";

	MX_RECORD *ccd_record;
	MX_CCD *ccd;
	char *endptr;
	double counting_time;
	int status, x_size, y_size, destination;
	size_t length;
	unsigned long ccd_status;
	mx_status_type mx_status;

	static char usage[]
	= "Usage:  ccd 'ccd_name' still 'counting_time' 'ccd_frame_name'\n"
	  "        ccd 'ccd_name' rawstill 'counting_time' 'ccd_frame_name'\n"
	  "        ccd 'ccd_name' measure_dark 'counting_time'\n"
	  "        ccd 'ccd_name' start [ 'counting_time' ]\n"
	  "        ccd 'ccd_name' stop\n"
	  "        ccd 'ccd_name' read ['destination']\n"
	  "           where destination = raw, data, dark, or background\n"
	  "        ccd 'ccd_name' dezinger\n"
	  "        ccd 'ccd_name' correct\n"
	  "        ccd 'ccd_name' write 'savefile'\n"
	  "        ccd 'ccd_name' rawwrite 'savefile'\n"
	  "\n"
	  "        ccd 'ccd_name' get status\n"
	  "        ccd 'ccd_name' get frame_size\n"
	  "        ccd 'ccd_name' get bin_size\n"
	  "\n"
	  "        ccd 'ccd_name' set frame_size 'x_size' 'y_size'\n"
	  "        ccd 'ccd_name' set bin_size 'x_size' 'y_size'\n"
	  "\n"
	  "        ccd 'ccd_name' get header 'variable_name'\n"
	  "        ccd 'ccd_name' set header 'variable_name' 'variable_contents'\n"
	;

	if ( argc < 4 ) {
		fprintf( output, "%s\n", usage );
		return FAILURE;
	}

	ccd_record = mx_get_record( motor_record_list, argv[2] );

	if ( ccd_record == NULL ) {
		fprintf( output, "The record '%s' does not exist.\n", argv[2] );
		return FAILURE;
	}

	ccd = (MX_CCD *) ccd_record->record_class_struct;

	if ( ccd == (MX_CCD *) NULL ) {
		fprintf( output, "MX_CCD pointer for record '%s' is NULL.\n",
				ccd_record->name );
		return FAILURE;
	}

	status = SUCCESS;

	if ( (strncmp( "still", argv[3], strlen(argv[3]) ) == 0)
	  || (strncmp( "rawstill", argv[3], strlen(argv[3]) ) == 0) )
	{
		MX_DEBUG(-2,("%s: argc = %d", cname, argc));
		if ( argc == 6 ) {
			if ( strspn( argv[4], "0123456789." ) == 0 ) {
				fprintf( output,
				"%s: Illegal counting time '%s'\n\n",
					cname, argv[4] );

				fprintf( output, "%s\n", usage );
				return FAILURE;
			}

			counting_time = atof( argv[4] );
		} else {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		mx_status = mx_ccd_start_for_preset_time( ccd_record,
							counting_time );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = motor_wait_for_ccd( ccd_record,
						"CCD acquisition aborted." );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = mx_ccd_readout( ccd_record,
						MXF_CCD_TO_RAW_DATA_FRAME );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = motor_wait_for_ccd( ccd_record,
						"CCD readout aborted." );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		if ( strncmp( "rawstill", argv[3], strlen(argv[3]) ) == 0 ) {

			/* Write out the raw still frame. */

			mx_status = mx_ccd_writefile( ccd_record, argv[5],
						MXF_CCD_FROM_RAW_DATA_FRAME );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;
		} else {
			/* We want a corrected still. */

			mx_status = mx_ccd_correct( ccd_record );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			mx_status = motor_wait_for_ccd( ccd_record,
						"CCD correction aborted." );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			/* Write out the still frame. */

			mx_status = mx_ccd_writefile( ccd_record, argv[5],
						MXF_CCD_FROM_CORRECTED_FRAME );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;
		}

		mx_status = motor_wait_for_ccd( ccd_record,
						"CCD frame writing aborted." );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		fprintf( output,
			"CCD frame '%s' successfully written.\n",
			argv[5] );
	} else
	if ( (strncmp( "measure_dark", argv[3], strlen(argv[3]) ) == 0)
	  || (strncmp( "measure_background", argv[3], strlen(argv[3]) ) == 0) )
	{
		MX_DEBUG(-2,("%s: argc = %d", cname, argc));
		if ( argc == 5 ) {
			if ( strspn( argv[4], "0123456789." ) == 0 ) {
				fprintf( output,
				"%s: Illegal counting time '%s'\n\n",
					cname, argv[4] );

				fprintf( output, "%s\n", usage );
				return FAILURE;
			}

			counting_time = atof( argv[4] );
		} else {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		mx_status = mx_ccd_start_for_preset_time( ccd_record,
							counting_time );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = motor_wait_for_ccd( ccd_record,
						"CCD acquisition aborted." );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = mx_ccd_readout( ccd_record,
						MXF_CCD_TO_BACKGROUND_FRAME );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = motor_wait_for_ccd( ccd_record,
						"CCD readout aborted." );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		fprintf( output,
			"CCD background dark frame successfully taken.\n" );
	} else
	if ( strncmp( "start", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc == 4 ) {
			/* A start was requested without specifying
			 * a counting time.
			 */

			mx_status = mx_ccd_start( ccd_record );

			if ( mx_status.code != MXE_SUCCESS ) {
				return FAILURE;
			} else {
				return SUCCESS;
			}
		}
		/* If we get here a counting time _was_ specified. */

		if ( argc == 5 ) {
			if ( strspn( argv[4], "0123456789." ) == 0 ) {
				fprintf( output,
				"%s: Illegal counting time '%s'\n\n",
					cname, argv[4] );

				fprintf( output, "%s\n", usage );
				return FAILURE;
			}

			counting_time = atof( argv[4] );
		} else {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		mx_status = mx_ccd_start_for_preset_time( ccd_record,
							counting_time );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = motor_wait_for_ccd( ccd_record,
						"CCD acquisition aborted." );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

	} else
	if ( strncmp( "stop", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc != 4 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		mx_status = mx_ccd_stop( ccd_record );

		if ( mx_status.code != MXE_SUCCESS ) {
			return FAILURE;
		} else {
			return SUCCESS;
		}
	} else
	if ( strncmp( "read", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc == 4 ) {
			destination = MXF_CCD_TO_RAW_DATA_FRAME;
		} else if ( argc == 5 ) {
			length = strlen(argv[4]);

			if ( ( strncmp( "raw", argv[4], length ) == 0 )
			  || ( strncmp( "data", argv[4], length ) == 0 ) )
			{
				destination = MXF_CCD_TO_RAW_DATA_FRAME;
			} else
			if ( ( strncmp( "dark", argv[4], length ) == 0 )
			  || ( strncmp( "background", argv[4], length ) == 0 ) )
			{
				destination = MXF_CCD_TO_BACKGROUND_FRAME;
			} else {
				fprintf( output,
				"Error: invalid read destination '%s'\n",
					argv[4] );
				fprintf( output, "%s\n", usage );
				return FAILURE;
			}
		} else {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		mx_status = mx_ccd_readout( ccd_record, destination );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = motor_wait_for_ccd( ccd_record,
						"CCD read aborted." );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;
	} else
	if ( strncmp( "dezinger", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc != 4 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		mx_status = mx_ccd_dezinger( ccd_record,
						MXF_CCD_TO_RAW_DATA_FRAME );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = motor_wait_for_ccd( ccd_record,
						"CCD dezinger aborted." );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;
	} else
	if ( strncmp( "correct", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc != 4 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		mx_status = mx_ccd_correct( ccd_record );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = motor_wait_for_ccd( ccd_record,
						"CCD correction aborted." );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;
	} else
	if ( strncmp( "write", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc != 5 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		mx_status = mx_ccd_writefile( ccd_record, argv[4],
						MXF_CCD_FROM_CORRECTED_FRAME );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = motor_wait_for_ccd( ccd_record,
						"CCD frame writing aborted." );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		fprintf( output,
			"CCD frame '%s' successfully written.\n",
			argv[4] );
	} else
	if ( strncmp( "rawwrite", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc != 5 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		mx_status = mx_ccd_writefile( ccd_record, argv[4],
						MXF_CCD_FROM_RAW_DATA_FRAME );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = motor_wait_for_ccd( ccd_record,
						"CCD frame writing aborted." );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		fprintf( output,
			"CCD frame '%s' successfully written.\n",
			argv[4] );
	} else
	if ( strncmp( "get", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc < 5 ) {
			fprintf( output,
			"%s: not enough arguments to 'get' command\n",
				cname );

			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		if ( strncmp( "status", argv[4], strlen(argv[4]) ) == 0 ) {

			if ( argc != 5 ) {
				fprintf( output,
	"%s: too many number of arguments to the 'get status' command\n",
					cname );

				fprintf( output, "%s\n", usage );
				return FAILURE;
			}

			mx_status = mx_ccd_get_status(ccd_record, &ccd_status);

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output, "CCD '%s' status = %#lx\n",
				ccd_record->name, ccd_status );
		} else
		if ( strncmp( "frame_size", argv[4], strlen(argv[4]) ) == 0 ) {

			if ( argc != 5 ) {
				fprintf( output,
	"%s: too many number of arguments to the 'get frame_size' command\n",
					cname );

				fprintf( output, "%s\n", usage );
				return FAILURE;
			}

			mx_status = mx_ccd_get_data_frame_size( ccd_record,
							&x_size, &y_size );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output,
				"CCD '%s' frame size is %d by %d\n",
				ccd_record->name, x_size, y_size );
		} else
		if ( strncmp( "bin_size", argv[4], strlen(argv[4]) ) == 0 ) {

			if ( argc != 5 ) {
				fprintf( output,
	"%s: too many number of arguments to the 'get bin_size' command\n",
					cname );

				fprintf( output, "%s\n", usage );
				return FAILURE;
			}

			mx_status = mx_ccd_get_bin_size( ccd_record,
							&x_size, &y_size );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output,
				"CCD '%s' bin size is %d by %d\n",
				ccd_record->name, x_size, y_size );
		} else
		if ( strncmp( "header", argv[4], strlen(argv[4]) ) == 0 ) {

			char buffer[200];

			if ( argc != 6 ) {
				fprintf( output,
		"%s: wrong number of arguments to 'get header' command\n",
					cname );

				fprintf( output, "%s\n", usage );
				return FAILURE;
			}

			mx_status = mx_ccd_read_header_variable( ccd_record,
					argv[5], buffer, sizeof(buffer) );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output,
				"CCD '%s' header '%s' = '%s'\n",
				ccd_record->name, argv[5], buffer );
		} else {
			fprintf( output,
				"%s: unknown get command argument '%s'\n",
				cname, argv[4] );

			return FAILURE;
		}
	} else
	if ( strncmp( "set", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc <= 5 ) {
			fprintf( output,
			"%s: wrong number of arguments to 'set' command\n",
				cname );

			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		if ( strncmp( "frame_size", argv[4], strlen(argv[4]) ) == 0 ) {

			if ( argc != 7 ) {
				fprintf( output,
		"%s: wrong number of arguments to 'set frame_size' command\n",
					cname );

				fprintf( output, "%s\n", usage );
				return FAILURE;
			}

			x_size = (int) strtoul( argv[5], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
		"%s: Non-numeric characters found in frame X size '%s'\n",
					cname, argv[5] );

				return FAILURE;
			}

			y_size = (int) strtoul( argv[6], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
		"%s: Non-numeric characters found in frame Y size '%s'\n",
					cname, argv[6] );

				return FAILURE;
			}

			fprintf( output,
			"%s: 'set frame_size' is not implemented.\n", cname );

			return FAILURE;
		} else
		if ( strncmp( "bin_size", argv[4], strlen(argv[4]) ) == 0 ) {

			if ( argc != 7 ) {
				fprintf( output,
		"%s: wrong number of arguments to 'set bin_size' command\n",
					cname );

				fprintf( output, "%s\n", usage );
				return FAILURE;
			}

			x_size = (int) strtoul( argv[5], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
		"%s: Non-numeric characters found in bin X size '%s'\n",
					cname, argv[5] );

				return FAILURE;
			}

			y_size = (int) strtoul( argv[6], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
		"%s: Non-numeric characters found in bin Y size '%s'\n",
					cname, argv[6] );

				return FAILURE;
			}

			mx_status = mx_ccd_set_bin_size( ccd_record,
							x_size, y_size );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;
		} else
		if ( strncmp( "header", argv[4], strlen(argv[4]) ) == 0 ) {

			if ( argc != 7 ) {
				fprintf( output,
		"%s: wrong number of arguments to 'set header' command\n",
					cname );

				fprintf( output, "%s\n", usage );
				return FAILURE;
			}

			mx_status = mx_ccd_write_header_variable( ccd_record,
							argv[5], argv[6] );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;
		} else {
			fprintf( output,
				"%s: unknown set command argument '%s'\n",
				cname, argv[4] );

			return FAILURE;
		}
	} else {
		/* Unrecognized command. */

		fprintf( output, "Unrecognized option '%s'\n\n", argv[3] );
		fprintf( output, "%s\n", usage );
		return FAILURE;
	}
	return status;
}

