/*
 * Name:    mwvout.c
 *
 * Purpose: Waveform output control functions.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2008 Illinois Institute of Technology
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
#include "mx_waveform_output.h"
#include "mx_key.h"

#ifdef max
#undef max
#endif

#define max(a,b) ((a) >= (b) ? (a) : (b))

#define VALUES_PER_ROW	10
#define ROWS_PER_PAGE	20

#define VALUES_PER_PAGE ( VALUES_PER_ROW * ROWS_PER_PAGE )

static int motor_wvout_read( MX_RECORD *wvout_record,
				MX_WAVEFORM_OUTPUT *wvout,
				unsigned long channel );

static int motor_wvout_read_all( MX_RECORD *wvout_record,
				MX_WAVEFORM_OUTPUT *wvout );

#if 0
static int motor_wvout_write( MX_RECORD *wvout_record,
				MX_WAVEFORM_OUTPUT *wvout,
				unsigned long channel );

static int motor_wvout_write_all( MX_RECORD *wvout_record,
				MX_WAVEFORM_OUTPUT *wvout );
#endif

static int motor_wvout_display_plot( MX_RECORD *wvout_record,
				MX_WAVEFORM_OUTPUT *wvout,
				unsigned long channel_number );

static int motor_wvout_display_all( MX_RECORD *wvout_record,
				MX_WAVEFORM_OUTPUT *wvout );

int
motor_wvout_fn( int argc, char *argv[] )
{
	static const char cname[] = "wvout";

	MX_RECORD *wvout_record;
	MX_WAVEFORM_OUTPUT *wvout;
	FILE *savefile;
	int os_status, saved_errno;
	char *endptr;
	unsigned long i, j, channel, num_channels;
	unsigned long num_points, num_points_to_zero;
	double *channel_data;
	double **wvout_data;
	int status, num_items;
	char buffer[40];
	mx_status_type mx_status;

	static char usage[] =
  "Usage:  wvout 'wvout_name' arm\n"
  "        wvout 'wvout_name' trigger\n"
  "        wvout 'wvout_name' start\n"
  "        wvout 'wvout_name' stop\n"
  "        wvout 'wvout_name' readall\n"
  "        wvout 'wvout_name' read 'channel_number'\n"
  "        wvout 'wvout_name' rawreadall\n"
  "        wvout 'wvout_name' rawread 'channel_number'\n"
  "        wvout 'wvout_name' saveall 'datafile'\n"
  "        wvout 'wvout_name' save 'channel_number' 'datafile'\n"
  "        wvout 'wvout_name' loadall 'datafile'\n"
  "        wvout 'wvout_name' load 'channel_number' 'datafile'\n"
  "        wvout 'wvout_name' get num_points\n"
  "        wvout 'wvout_name' set num_points 'value'\n"
	;

	if ( argc < 4 ) {
		fprintf( output, "%s\n", usage );
		return FAILURE;
	}

	wvout_record = mx_get_record( motor_record_list, argv[2] );

	if ( wvout_record == NULL ) {
		fprintf( output, "The record '%s' does not exist.\n", argv[2] );
		return FAILURE;
	}

	wvout = (MX_WAVEFORM_OUTPUT *) wvout_record->record_class_struct;

	if ( wvout == (MX_WAVEFORM_OUTPUT *) NULL ) {
		fprintf( output,
		"MX_WAVEFORM_OUTPUT pointer for record '%s' is NULL.\n",
				wvout_record->name );
		return FAILURE;
	}

	status = SUCCESS;

	if (strncmp( "arm", argv[3], strlen(argv[3]) ) == 0) {

		if ( argc != 4 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		mx_status = mx_waveform_output_arm( wvout_record );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;
	} else
	if (strncmp( "trigger", argv[3], strlen(argv[3]) ) == 0) {

		if ( argc != 4 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		mx_status = mx_waveform_output_trigger( wvout_record );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;
	} else
	if (strncmp( "start", argv[3], strlen(argv[3]) ) == 0) {

		if ( argc != 4 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		mx_status = mx_waveform_output_start( wvout_record );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;
	} else
	if ( strncmp( "saveall", argv[3], max( strlen(argv[3]), 5 ) ) == 0 ) {

		if ( argc != 5 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		mx_status = mx_waveform_output_read_all( wvout_record,
					&num_channels,
					&num_points,
					&wvout_data );

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

		for ( i = 0; i < num_points; i++ ) {
			for ( j = 0; j < num_channels; j++ ) {
				fprintf(savefile, "%10g  ", wvout_data[j][i]);

				if ( feof(savefile) || ferror(savefile) ) {
					fprintf( output,
				"%s: error writing to save file '%s'\n",
					cname, argv[4] );

					break;	/* Exit the for() loop. */
				}
			}
			fprintf(savefile, "\n");
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
		"Waveform output save file '%s' successfully written.\n",
			argv[4] );
	} else
	if ( strncmp( "save", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc != 6 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		channel = atol( argv[4] );

		mx_status = mx_waveform_output_read_channel( wvout_record,
						channel,
						&num_points,
						&channel_data );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		savefile = fopen( argv[5], "w" );

		if ( savefile == NULL ) {
			saved_errno = errno;

			fprintf( output,
			"%s: cannot open save file '%s'.  Reason = '%s'\n",
				cname, argv[5], strerror(saved_errno) );

			return FAILURE;
		}

		for ( i = 0; i < num_points; i++ ) {

			fprintf(savefile, "%10g\n", channel_data[i]);

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
		"Waveform output save file '%s' successfully written.\n",
			argv[5] );
	} else
	if ( strncmp( "load", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc != 6 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		channel = atol( argv[4] );

		if ( channel >= wvout->maximum_num_channels ) {
			fprintf( output, "The requested channel number (%lu) "
			"for waveform output '%s' is larger than the "
			"maximum value of %lu.\n", channel, wvout_record->name,
				wvout->maximum_num_channels - 1 );

			return FAILURE;
		}

		/* Load the channel data from a file. */

		channel_data = (wvout->data_array)[channel];

		savefile = fopen( argv[5], "r" );

		if ( savefile == NULL ) {
			saved_errno = errno;

			fprintf( output,
			"%s: cannot open load file '%s'.  Reason = '%s'\n",
				cname, argv[5], strerror(saved_errno) );

			return FAILURE;
		}

		for ( i = 0; i < wvout->maximum_num_points; i++ ) {
			fgets( buffer, sizeof(buffer), savefile );

			if ( feof(savefile) || ferror(savefile) ) {
				break;
			}

			num_items = sscanf( buffer, "%lg", &(channel_data[i]) );

			if ( num_items != 1 ) {
				fprintf( output,
				"Line %lu of savefile '%s' does not contain a "
				"numerical value.  Instead, it contains '%s'.",
					i+1, argv[5], buffer );

				fclose( savefile );
				return FAILURE;
			}
		}

		fclose( savefile );

		if ( i < wvout->maximum_num_points ) {
			num_points_to_zero = wvout->maximum_num_points - 1;

			memset( &(channel_data[i]), 0,
				num_points_to_zero * sizeof(double) );
		}

		mx_status = mx_waveform_output_write_channel( wvout_record,
						channel,
						num_points,
						channel_data );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

	} else
	if ( strncmp( "readall", argv[3], max( strlen(argv[3]), 5 ) ) == 0 ) {

		if ( argc != 4 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		status = motor_wvout_display_all( wvout_record, wvout );
	} else
	if ( strncmp( "read", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc != 5 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		channel = atol( argv[4] );

		status = motor_wvout_display_plot( wvout_record,
						wvout, channel );
	} else
	if ( strncmp( "rawreadall", argv[3], max( strlen(argv[3]), 8 ) ) == 0 ){

		if ( argc != 4 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		status = motor_wvout_read_all( wvout_record, wvout );
	} else
	if ( strncmp( "rawread", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc != 5 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		channel = atol( argv[4] );

		status = motor_wvout_read( wvout_record, wvout, channel );
	} else
	if ( strncmp( "stop", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc != 4 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		mx_status = mx_waveform_output_stop( wvout_record );

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

#if 0
		if ( strncmp( "measurement_time",
				argv[4], strlen(argv[4]) ) == 0 )
		{
			mx_status = mx_waveform_output_get_measurement_time(
					wvout_record, &measurement_time );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output,
			"Waveform output '%s' measurement time = %g\n",
				wvout_record->name, measurement_time );
		} else
#endif
		if ( strncmp( "num_points",
				argv[4], strlen(argv[4]) ) == 0 )
		{
			mx_status = mx_waveform_output_get_num_points(
					wvout_record, &num_points );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output,
				"Waveform output '%s' num points = %lu\n",
				wvout_record->name, num_points );
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

#if 0
		if ( strncmp( "measurement_time",
			argv[4], strlen(argv[4]) ) == 0 )
		{
			if ( argc != 6 ) {
				fprintf( output,
	"%s: wrong number of arguments to 'set measurement_time' command\n",
					cname );

				fprintf( output, "%s\n", usage );
				return FAILURE;
			}

			measurement_time = atof( argv[5] );

			mx_status = mx_waveform_output_set_measurement_time(
					wvout_record, measurement_time );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;
		} else
#endif
		if ( strncmp( "num_points",
			argv[4], strlen(argv[4]) ) == 0)
		{
			if ( argc != 6 ) {
				fprintf( output,
	"%s: wrong number of arguments to 'set num_points' command\n",
					cname );

				fprintf( output, "%s\n", usage );
				return FAILURE;
			}

			num_points = strtoul( argv[5], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
			"%s: Non-numeric characters found in waveform output "
			"number of channels value '%s'\n",
					cname, argv[5] );
				return FAILURE;
			}

			mx_status = mx_waveform_output_set_num_points(
					wvout_record, num_points );

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
motor_wvout_read( MX_RECORD *wvout_record,
		MX_WAVEFORM_OUTPUT *wvout,
		unsigned long channel_number)
{
	unsigned long i, num_points;
	double *channel_data;
	mx_status_type mx_status;

	/* Read out the acquired data. */

	fprintf( output, "About to read waveform output data.\n" );

	mx_status = mx_waveform_output_read_channel( wvout_record,
				channel_number, &num_points, &channel_data );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	/* Display the data. */

	fprintf( output, "Waveform output data successfully read.\n" );
	fprintf( output, "\n" );

#if 1
	fprintf( output, "Hit any key to continue...\n" );
	(void) mx_getch();
#endif

	for ( i = 0; i < num_points; i++ ) {
		if ( (i % VALUES_PER_ROW) == 0 ) {
			fprintf( output, "\n%4lu: ", i );
		}

		fprintf( output, "%6g ", channel_data[i] );

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

static int
motor_wvout_read_all( MX_RECORD *wvout_record, MX_WAVEFORM_OUTPUT *wvout )
{
	unsigned long i, j, num_channels, num_points;
	double **wvout_data;
	mx_status_type mx_status;

	/* Read out the acquired data. */

	fprintf( output, "About to read waveform output data.\n" );

	mx_status = mx_waveform_output_read_all( wvout_record, &num_channels,
					&num_points,
					&wvout_data );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	/* Display the data. */

	fprintf( output, "Waveform output data successfully read.\n" );
	fprintf( output, "\n" );

#if 1
	fprintf( output, "Hit any key to continue...\n" );
	(void) mx_getch();
#endif

	for ( i = 0; i < num_points; i++ ) {
		fprintf( output, "\n%4lu: ", i );

		for ( j = 0; j < num_channels; j++ ) {

			fprintf( output, "%6g ", wvout_data[j][i] );
		}
#if 1
		if ( ((i+1) % ROWS_PER_PAGE) == 0 ) {
			fprintf( output, "\nHit any key to continue...\n" );
			(void) mx_getch();
		}
#endif
	}

	fprintf( output, "\n" );

	return SUCCESS;
}

/* Warning: motor_wvout_display_plot() and motor_wvout_display_all()
 * presuppose the presence of 'plotgnu.pl'.
 */

#ifdef OS_WIN32
#define popen _popen
#define pclose _pclose
#endif

static int
motor_wvout_display_plot( MX_RECORD *wvout_record,
				MX_WAVEFORM_OUTPUT *wvout,
				unsigned long channel_number )
{
	const char fname[] = "motor_wvout_display_plot()";

	MX_LIST_HEAD *list_head;
	FILE *plotgnu_pipe;
	unsigned long i, num_points;
	double *channel_data;
	int status;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	/* Don't display a plot if we have been asked not to. */

	list_head = mx_get_record_list_head_struct( wvout_record );

	if ( list_head == NULL ) {
		(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"Cannot find the record list list_head structure." );

		return FAILURE;
	}

	if ( list_head->plotting_enabled == MXPF_PLOT_OFF )
		return SUCCESS;

	/* Read the data from the waveform output device. */

	mx_status = mx_waveform_output_read_channel( wvout_record,
				channel_number, &num_points, &channel_data );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	/* Open a connetion to plotgnu. */

#if defined(OS_VXWORKS) || defined(OS_RTEMS) || defined(OS_ECOS)
	fprintf( output,
	 "Plotting with Gnuplot is not supported for this operating system.\n");
	return FAILURE;
#else
	plotgnu_pipe = popen( MXP_PLOTGNU_COMMAND, "w" );
#endif

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

	for ( i = 0; i < num_points; i++ ) {
		status = fprintf( plotgnu_pipe,
					"data %lu %g\n", i, channel_data[i] );
	}

	status = fprintf( plotgnu_pipe,
			"set title 'Waveform output display'\n" );

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

#if !defined(OS_VXWORKS) && !defined(OS_RTEMS) && !defined(OS_ECOS)
	status = pclose( plotgnu_pipe );
#endif

	return SUCCESS;
}

static int
motor_wvout_display_all( MX_RECORD *wvout_record, MX_WAVEFORM_OUTPUT *wvout )
{
	const char fname[] = "motor_wvout_display_all()";

	MX_LIST_HEAD *list_head;
	FILE *plotgnu_pipe;
	unsigned long i, j, num_channels, num_points;
	double **wvout_data;
	int status;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	/* Don't display a plot if we have been asked not to. */

	list_head = mx_get_record_list_head_struct( wvout_record );

	if ( list_head == NULL ) {
		(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"Cannot find the record list list_head structure." );

		return FAILURE;
	}

	if ( list_head->plotting_enabled == MXPF_PLOT_OFF )
		return SUCCESS;

	/* Read the data from the waveform output device. */

	mx_status = mx_waveform_output_read_all( wvout_record, &num_channels,
					&num_points, &wvout_data );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	/* Open a connetion to plotgnu. */

#if defined(OS_VXWORKS) || defined(OS_RTEMS) || defined(OS_ECOS)
	fprintf( output,
	 "Plotting with Gnuplot is not supported for this operating system.\n");
	return FAILURE;
#else
	plotgnu_pipe = popen( MXP_PLOTGNU_COMMAND, "w" );
#endif

	if ( plotgnu_pipe == NULL ) {
		(void) mx_error( MXE_IPC_IO_ERROR, fname,
		"Unable to start the 'plotgnu' program." );
	}

	/* Set the connection to be line buffered. */

	setvbuf( plotgnu_pipe, (char *)NULL, _IOLBF, BUFSIZ );

	/* Tell plotgnu that we do not have any motor positions for it. */

	status = fprintf( plotgnu_pipe, "start_plot;1;0" );


	for ( j = 0; j < num_channels; j++ ) {

		status = fprintf( plotgnu_pipe, ";$f[%lu]", j );
	}
	status = fprintf( plotgnu_pipe, "\n" );

	/* Send the data to the plotting program. */

	fprintf( output,
		"Sending data to the plotting program.  Please wait...\n" );

	for ( i = 0; i < num_points; i++ ) {
		status = fprintf( plotgnu_pipe, "data %lu", i );

		for ( j = 0; j < num_channels; j++ ) {

		status = fprintf( plotgnu_pipe, " %g", wvout_data[j][i] );
		}

		status = fprintf( plotgnu_pipe, "\n" );
	}

	status = fprintf( plotgnu_pipe,
				"set title 'Waveform output display'\n" );

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

#if !defined(OS_VXWORKS) && !defined(OS_RTEMS) && !defined(OS_ECOS)
	status = pclose( plotgnu_pipe );
#endif

	return SUCCESS;
}

