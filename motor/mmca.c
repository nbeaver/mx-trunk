/*
 * Name:    mmca.c
 *
 * Purpose: MCA control functions.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2003, 2005-2006 Illinois Institute of Technology
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
#include "mx_mca.h"
#include "mx_key.h"

#define VALUES_PER_ROW	10
#define ROWS_PER_PAGE	20

#define VALUES_PER_PAGE ( VALUES_PER_ROW * ROWS_PER_PAGE )

static int motor_mca_read( MX_RECORD *mca_record, MX_MCA *mca );
static int motor_mca_display_plot( MX_RECORD *mca_record, MX_MCA *mca );

int
motor_mca_fn( int argc, char *argv[] )
{
	static const char cname[] = "mca";

	MX_RECORD *mca_record;
	MX_MCA *mca;
	FILE *savefile;
	char *endptr;
	size_t length;
	double counting_time;
	int status, os_status, saved_errno, use_real_time;
	unsigned long i, roi_number;
	unsigned long *channel_array;
	unsigned long roi[2], roi_integral, num_channels;
	unsigned long channel_number, channel_value;
	double real_time, live_time, icr, ocr;
	mx_bool_type busy;
	mx_status_type mx_status;

	static char usage[]
	= "Usage:  mca 'mca_name' count [ real | live ] 'counting_time'\n"
	  "        mca 'mca_name' rawcount [ real | live ] 'counting_time'\n"
	  "        mca 'mca_name' start [ real | live ] [ 'counting_time' ]\n"
	  "        mca 'mca_name' stop\n"
	  "        mca 'mca_name' clear\n"
	  "        mca 'mca_name' read\n"
	  "        mca 'mca_name' rawread\n"
	  "        mca 'mca_name' save 'savefile'\n"
	  "\n"
	  "        mca 'mca_name' get roi 'roi_number'\n"
	  "        mca 'mca_name' get integral 'roi_number'\n"
	  "        mca 'mca_name' get num_channels\n"
	  "        mca 'mca_name' get channel 'channel_number'\n"
	  "        mca 'mca_name' get real_time\n"
	  "        mca 'mca_name' get live_time\n"
	  "\n"
	  "        mca 'mca_name' set roi 'roi_number' 'start' 'end'\n"
	  "        mca 'mca_name' set num_channels 'num_channels'\n"
	  "\n"
	  "        mca 'mca_name' get soft_roi 'soft_roi_number'\n"
	  "        mca 'mca_name' set soft_roi 'soft_roi_number' 'start' 'end'\n"
	  "        mca 'mca_name' get soft_integral 'soft_roi_number'\n"
	  "        mca 'mca_name' get icr\n"
	  "        mca 'mca_name' get ocr\n"
	;

	if ( argc < 4 ) {
		fprintf( output, "%s\n", usage );
		return FAILURE;
	}

	mca_record = mx_get_record( motor_record_list, argv[2] );

	if ( mca_record == NULL ) {
		fprintf( output, "The record '%s' does not exist.\n", argv[2] );
		return FAILURE;
	}

	mca = (MX_MCA *) mca_record->record_class_struct;

	if ( mca == (MX_MCA *) NULL ) {
		fprintf( output, "MX_MCA pointer for record '%s' is NULL.\n",
				mca_record->name );
		return FAILURE;
	}

	status = SUCCESS;

	if ( (strncmp( "count", argv[3], strlen(argv[3]) ) == 0)
	  || (strncmp( "rawcount", argv[3], strlen(argv[3]) ) == 0) )
	{
		if ( argc == 5 ) {
			if ( strspn( argv[4], "0123456789." ) == 0 ) {
				fprintf( output,
				"%s: Illegal counting time '%s'\n\n",
					cname, argv[4] );

				fprintf( output, "%s\n", usage );
				return FAILURE;
			}

			counting_time = atof( argv[4] );
			use_real_time = FALSE;

		} else if ( argc == 6 ) {
			if ( strspn( argv[5], "0123456789." ) == 0 ) {
				fprintf( output,
				"%s: Illegal counting time '%s'\n\n",
					cname, argv[5] );

				fprintf( output, "%s\n", usage );
				return FAILURE;
			}

			counting_time = atof( argv[5] );

			length = strlen( argv[4] );

			if ( strncmp( argv[4], "live", length ) == 0 ) {
				use_real_time = FALSE;
			} else
			if ( strncmp( argv[4], "real", length ) == 0 ) {
				use_real_time = TRUE;
			} else {
				fprintf( output, "%s\n", usage );
				return FAILURE;
			}
		} else {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		if ( use_real_time ) {
			mx_status = mx_mca_start_for_preset_real_time(
				mca_record, counting_time );
		} else {
			mx_status = mx_mca_start_for_preset_live_time(
				mca_record, counting_time );
		}

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		busy = TRUE;

		while( busy ) {
			if ( mx_kbhit() ) {
				(void) mx_getch();

				mx_status = mx_mca_stop( mca_record );

				if ( mx_status.code != MXE_SUCCESS )
					return FAILURE;

				fprintf( output, "MCA counting aborted.\n" );
			}

			mx_status = mx_mca_is_busy( mca_record, &busy );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			mx_msleep(500);
		}

		if ( strncmp( "rawcount", argv[3], strlen(argv[3]) ) == 0 ) {
			status = motor_mca_read( mca_record, mca );
		} else {
			status = motor_mca_display_plot( mca_record, mca );
		}
	} else
	if ( strncmp( "start", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc == 4 ) {
			/* A start was requested without specifying
			 * a counting time.
			 */

			mx_status = mx_mca_start_without_preset( mca_record );

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
			use_real_time = FALSE;

		} else if ( argc == 6 ) {
			if ( strspn( argv[5], "0123456789." ) == 0 ) {
				fprintf( output,
				"%s: Illegal counting time '%s'\n\n",
					cname, argv[5] );

				fprintf( output, "%s\n", usage );
				return FAILURE;
			}

			counting_time = atof( argv[5] );

			length = strlen( argv[4] );

			if ( strncmp( argv[4], "live", length ) == 0 ) {
				use_real_time = FALSE;
			} else
			if ( strncmp( argv[4], "real", length ) == 0 ) {
				use_real_time = TRUE;
			} else {
				fprintf( output, "%s\n", usage );
				return FAILURE;
			}
		} else {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		if ( use_real_time ) {
			mx_status = mx_mca_start_for_preset_real_time(
				mca_record, counting_time );
		} else {
			mx_status = mx_mca_start_for_preset_live_time(
				mca_record, counting_time );
		}

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		busy = TRUE;

		while( busy ) {
			if ( mx_kbhit() ) {
				(void) mx_getch();

				mx_status = mx_mca_stop( mca_record );

				if ( mx_status.code != MXE_SUCCESS )
					return FAILURE;

				fprintf( output, "MCA counting aborted.\n" );
			}

			mx_status = mx_mca_is_busy( mca_record, &busy );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			mx_msleep(500);
		}
	} else
	if ( strncmp( "save", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc != 5 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		mx_status = mx_mca_read( mca_record,
				&num_channels, &channel_array );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		savefile = fopen( argv[4], "w" );

		if ( savefile == NULL ) {
			saved_errno = errno;

			fprintf( output,
			"%s: cannot open save file '%s'.  Reason = '%s'\n",
				cname, argv[4], strerror(saved_errno) );

			return FAILURE;
		}

		for ( i = 0; i < num_channels; i++ ) {

			fprintf(savefile, "%10lu\n", channel_array[i]);

			if ( feof(savefile) || ferror(savefile) ) {
				fprintf( output,
				"%s: error writing to save file '%s'\n",
					cname, argv[4] );

				break;		/* Exit the for() loop. */
			}
		}

		os_status = fclose( savefile );

		if ( os_status != 0 ) {
			saved_errno = errno;

			fprintf( output,
			"%s: cannot close save file '%s'.  Reason = '%s'\n",
				cname, argv[4], strerror(saved_errno) );

			return FAILURE;
		}
		fprintf( output,
			"MCA Save file '%s' successfully written.\n",
			argv[4] );
	} else
	if ( strncmp( "read", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc != 4 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		status = motor_mca_display_plot( mca_record, mca );
	} else
	if ( strncmp( "rawread", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc != 4 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		status = motor_mca_read( mca_record, mca );
	} else
	if ( strncmp( "stop", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc != 4 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		mx_status = mx_mca_stop( mca_record );

		if ( mx_status.code != MXE_SUCCESS ) {
			return FAILURE;
		} else {
			return SUCCESS;
		}
	} else
	if ( strncmp( "clear", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc != 4 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		mx_status = mx_mca_clear( mca_record );

		if ( mx_status.code != MXE_SUCCESS ) {
			return FAILURE;
		} else {
			return SUCCESS;
		}
	} else
	if ( strncmp( "get", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc < 5 ) {
			fprintf( output,
			"%s: not enough arguments to 'get' command\n",
				cname );

			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		if ( strncmp( "roi", argv[4], strlen(argv[4]) ) == 0 ) {

			if ( argc != 6 ) {
				fprintf( output,
		"%s: wrong number of arguments to 'get roi' command\n",
					cname );

				fprintf( output, "%s\n", usage );
				return FAILURE;
			}

			roi_number = strtoul( argv[5], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
		"%s: Non-numeric characters found in ROI number '%s'\n",
					cname, argv[5] );

				return FAILURE;
			}

			mx_status = mx_mca_get_roi( mca_record,
							roi_number, roi );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output,
				"MCA '%s' ROI %lu: start = %lu, end = %lu\n",
				mca_record->name, roi_number,
				roi[0], roi[1] );
		} else
		if ( strncmp( "integral", argv[4], strlen(argv[4]) ) == 0 ) {

			if ( argc != 6 ) {
				fprintf( output,
		"%s: wrong number of arguments to 'get integral' command\n",
					cname );

				fprintf( output, "%s\n", usage );
				return FAILURE;
			}

			roi_number = strtoul( argv[5], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
		"%s: Non-numeric characters found in ROI number '%s'\n",
					cname, argv[5] );

				return FAILURE;
			}

			mx_status = mx_mca_get_roi_integral( mca_record,
						roi_number, &roi_integral );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output,
				"MCA '%s' ROI integrated counts = %lu\n",
				mca_record->name, roi_integral );
		} else
		if ( strncmp( "soft_roi", argv[4], strlen(argv[4]) ) == 0 ) {

			if ( argc != 6 ) {
				fprintf( output,
		"%s: wrong number of arguments to 'get soft_roi' command\n",
					cname );

				fprintf( output, "%s\n", usage );
				return FAILURE;
			}

			roi_number = strtoul( argv[5], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
		"%s: Non-numeric characters found in soft ROI number '%s'\n",
					cname, argv[5] );

				return FAILURE;
			}

			mx_status = mx_mca_get_soft_roi( mca_record,
							roi_number, roi );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output,
			"MCA '%s' soft ROI %lu: start = %lu, end = %lu\n",
				mca_record->name, roi_number,
				roi[0], roi[1] );
		} else
		if ( strncmp( "soft_integral", argv[4], strlen(argv[4])) == 0) {

			if ( argc != 6 ) {
				fprintf( output,
	"%s: wrong number of arguments to 'get soft_integral' command\n",
					cname );

				fprintf( output, "%s\n", usage );
				return FAILURE;
			}

			roi_number = strtoul( argv[5], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
	"%s: Non-numeric characters found in soft ROI number '%s'\n",
					cname, argv[5] );

				return FAILURE;
			}

			mx_status = mx_mca_get_soft_roi_integral( mca_record,
						roi_number, &roi_integral );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output,
				"MCA '%s' soft ROI integrated counts = %lu\n",
				mca_record->name, roi_integral );
		} else
		if ( strncmp( "num_channels", argv[4], strlen(argv[4]) ) == 0) {

			mx_status = mx_mca_get_num_channels(
						mca_record, &num_channels );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output,
				"MCA '%s' number of channels = %lu\n",
				mca_record->name, num_channels );

		} else
		if ( strncmp( "real_time", argv[4], strlen(argv[4]) ) == 0 ) {

			mx_status = mx_mca_get_real_time(
						mca_record, &real_time );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output,
				"MCA '%s' real time = %f seconds\n",
				mca_record->name, real_time );
		} else
		if ( strncmp( "live_time", argv[4], strlen(argv[4]) ) == 0 ) {

			mx_status = mx_mca_get_live_time(
						mca_record, &live_time );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output,
				"MCA '%s' live time = %f seconds\n",
				mca_record->name, live_time );
		} else
		if ( strncmp( "channel", argv[4], strlen(argv[4]) ) == 0 ) {

			if ( argc != 6 ) {
				fprintf( output,
		"%s: wrong number of arguments to 'get channel' command\n",
					cname );

				fprintf( output, "%s\n", usage );
				return FAILURE;
			}

			channel_number = strtoul( argv[5], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
					"'%s' is not a channel number.\n",
					argv[5] );
				return FAILURE;
			}

			mx_status = mx_mca_get_channel( mca_record,
					channel_number, &channel_value );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output, "MCA '%s' channel %lu = %lu\n",
				mca_record->name, channel_number,
				channel_value );
		} else
		if ( strncmp( "icr", argv[4], strlen(argv[4]) ) == 0 ) {

			mx_status = mx_mca_get_input_count_rate(
						mca_record, &icr );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output,
				"MCA '%s' input count rate = %f\n",
				mca_record->name, icr );

		} else
		if ( strncmp( "ocr", argv[4], strlen(argv[4]) ) == 0 ) {

			mx_status = mx_mca_get_output_count_rate(
						mca_record, &ocr );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output,
				"MCA '%s' output count rate = %f\n",
				mca_record->name, ocr );

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

		if ( strncmp( "roi", argv[4], strlen(argv[4]) ) == 0 ) {

			if ( argc != 8 ) {
				fprintf( output,
			"%s: wrong number of arguments to 'set roi' command\n",
					cname );

				fprintf( output, "%s\n", usage );
				return FAILURE;
			}

			roi_number = strtoul( argv[5], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
		"%s: Non-numeric characters found in ROI number '%s'\n",
					cname, argv[5] );

				return FAILURE;
			}

			roi[0] = strtoul( argv[6], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
		"%s: Non-numeric characters found in ROI start value '%s'\n",
					cname, argv[5] );

				return FAILURE;
			}

			roi[1] = strtoul( argv[7], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
		"%s: Non-numeric characters found in ROI end value '%s'\n",
					cname, argv[7] );

				return FAILURE;
			}

			mx_status = mx_mca_set_roi( mca_record,
							roi_number, roi );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;
		} else
		if ( strncmp( "soft_roi", argv[4], strlen(argv[4]) ) == 0 ) {

			if ( argc != 8 ) {
				fprintf( output,
		"%s: wrong number of arguments to 'set soft_roi' command\n",
					cname );

				fprintf( output, "%s\n", usage );
				return FAILURE;
			}

			roi_number = strtoul( argv[5], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
		"%s: Non-numeric characters found in soft ROI number '%s'\n",
					cname, argv[5] );

				return FAILURE;
			}

			roi[0] = strtoul( argv[6], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
	"%s: Non-numeric characters found in soft ROI start value '%s'\n",
					cname, argv[5] );

				return FAILURE;
			}

			roi[1] = strtoul( argv[7], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
	"%s: Non-numeric characters found in soft ROI end value '%s'\n",
					cname, argv[6] );

				return FAILURE;
			}

			mx_status = mx_mca_set_soft_roi( mca_record,
							roi_number, roi );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;
		} else
		if ( strncmp( "num_channels", argv[4], strlen(argv[4]) ) == 0) {

			if ( argc != 6 ) {
				fprintf( output,
		"%s: wrong number of arguments to 'set channels' command\n",
					cname );

				fprintf( output, "%s\n", usage );
				return FAILURE;
			}

			num_channels = strtoul( argv[5], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
"%s: Non-numeric characters found in MCA number of channels value '%s'\n",
					cname, argv[5] );
				return FAILURE;
			}

			mx_status = mx_mca_set_num_channels(
						mca_record, num_channels );

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

static int
motor_mca_read( MX_RECORD *mca_record, MX_MCA *mca )
{
	unsigned long i, num_channels;
	unsigned long *channel_array;
	mx_status_type mx_status;

	/* Read out the acquired data. */

	fprintf( output, "About to read MCA data.\n" );

	mx_status = mx_mca_read( mca_record, &num_channels, &channel_array );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	/* Display the data. */

	fprintf( output, "MCA data successfully read.\n" );
	fprintf( output, "\n" );

#if 1
	fprintf( output, "Hit any key to continue...\n" );
	(void) mx_getch();
#endif

	for ( i = 0; i < num_channels; i++ ) {
		if ( (i % VALUES_PER_ROW) == 0 ) {
			fprintf( output, "\n%4lu: ", i );
		}

		fprintf( output, "%6lu ", channel_array[i] );

#if 1
		if ( ((i+1) % VALUES_PER_PAGE) == 0 ) {
			fprintf( output, "\nHit any key to continue...\n" );
			(void) mx_getch();
		}
#endif
	}

	fprintf( output, "\n" );

	return SUCCESS;
}

/* Warning: This version of motor_mca_display_plot() presupposes the
 *          presence of 'plotgnu.pl'.
 */

#ifdef OS_WIN32
#define popen _popen
#define pclose _pclose
#endif

#if defined(OS_VXWORKS) || defined(OS_RTEMS) || defined(OS_ECOS)

static int
motor_mca_display_plot( MX_RECORD *mca_record, MX_MCA *mca )
{
	fprintf( output,
	"Plotting with Gnuplot is not supported for this operating system.\n");

	return FAILURE;
}

#else

static int
motor_mca_display_plot( MX_RECORD *mca_record, MX_MCA *mca )
{
	const char fname[] = "motor_mca_display_plot()";

	MX_LIST_HEAD *list_head;
	FILE *plotgnu_pipe;
	unsigned long *channel_array;
	int status;
	unsigned long i, num_channels;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	/* Don't display a plot if we have been asked not to. */

	list_head = mx_get_record_list_head_struct( mca_record );

	if ( list_head == NULL ) {
		(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"Cannot find the record list list_head structure." );

		return FAILURE;
	}

	if ( list_head->plotting_enabled == MXPF_PLOT_OFF )
		return SUCCESS;

	/* Read the data from the MCA. */

	mx_status = mx_mca_read( mca_record, &num_channels, &channel_array );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	/* Open a connetion to plotgnu. */

	plotgnu_pipe = popen( MXP_PLOTGNU_COMMAND, "w" );

	if ( plotgnu_pipe == NULL ) {
		(void) mx_error( MXE_IPC_IO_ERROR, fname,
		"Unable to start the 'plotgnu' program." );
	}

	/* Set the connection to be line buffered. */

	setvbuf( plotgnu_pipe, (char *)NULL, _IOLBF, BUFSIZ );

	/* Tell plotgnu that we do not have any motor positions for it. */

	status = fprintf( plotgnu_pipe, "start_plot;1;0;$f[0]\n" );

	/* Send the data to the plotting program. */

	fprintf( output,
		"Sending data to the plotting program.  Please wait...\n" );

	for ( i = 0; i < num_channels; i++ ) {
		status = fprintf( plotgnu_pipe,
					"data %lu %lu\n", i, channel_array[i] );
	}

	status = fprintf( plotgnu_pipe, "set title 'MCA display'\n" );

	status = fprintf( plotgnu_pipe, "set data style lines\n" );

	/* Tell plotgnu to replot the data display. */

	status = fprintf( plotgnu_pipe, "plot\n" );

	fflush( plotgnu_pipe );

	/* Prompt the user before closing the plot. */

	fprintf( output, "\nHit any key to close the plot.\n" );

	for (;;) {
		if ( mx_kbhit() ) {
			(void) mx_getch();

			break;		/* Exit the for(;;) loop */
		}
		mx_msleep(500);
	}

	status = fprintf( plotgnu_pipe, "exit\n" );

	mx_msleep(500);

	status = pclose( plotgnu_pipe );

	return SUCCESS;
}

#endif

