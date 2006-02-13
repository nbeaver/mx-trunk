/*
 * Name:    mmcs.c
 *
 * Purpose: MCS control functions.
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
#include "mx_mcs.h"
#include "mx_key.h"

#ifdef max
#undef max
#endif

#define max(a,b) ((a) >= (b) ? (a) : (b))

#define VALUES_PER_ROW	10
#define ROWS_PER_PAGE	20

#define VALUES_PER_PAGE ( VALUES_PER_ROW * ROWS_PER_PAGE )

static int motor_mcs_read(
		MX_RECORD *mcs_record, MX_MCS *mcs, unsigned long channel );

static int motor_mcs_read_all( MX_RECORD *mcs_record, MX_MCS *mcs );

static int motor_mcs_display_plot(
		MX_RECORD *mcs_record, MX_MCS *mcs, unsigned long channel );

static int motor_mcs_display_all( MX_RECORD *mcs_record, MX_MCS *mcs );

static int motor_mcs_measurement(
		MX_RECORD *mcs_record, MX_MCS *mcs, unsigned long measurement );

int
motor_mcs_fn( int argc, char *argv[] )
{
	static const char cname[] = "mcs";

	MX_RECORD *mcs_record;
	MX_MCS *mcs;
	FILE *savefile;
	int os_status, saved_errno;
	char *endptr;
	double measurement_time;
	mx_length_type i, j, channel, measurement;
	mx_length_type num_scalers, num_measurements;
	int32_t *scaler_data;
	int32_t **mcs_data;
	int busy, status;
	mx_status_type mx_status;

	static char usage[] =
  "Usage:  mcs 'mcs_name' count 'measurement_time' 'num_measurements\n"
  "        mcs 'mcs_name' rawcount 'measurement_time' 'num_measurements\n"
  "        mcs 'mcs_name' start [ 'measurement_time' 'num_measurements' ]\n"
  "        mcs 'mcs_name' stop\n"
  "        mcs 'mcs_name' clear\n"
  "        mcs 'mcs_name' readall\n"
  "        mcs 'mcs_name' rawreadall\n"
  "        mcs 'mcs_name' read 'channel_number'\n"
  "        mcs 'mcs_name' rawread 'channel_number'\n"
  "        mcs 'mcs_name' measurement 'measurement_number'\n"
  "        mcs 'mcs_name' saveall 'savefile'\n"
  "        mcs 'mcs_name' save 'channel_number' 'savefile'\n"
  "        mcs 'mcs_name' get measurement_time\n"
  "        mcs 'mcs_name' set measurement_time 'seconds'\n"
  "        mcs 'mcs_name' get num_measurements\n"
  "        mcs 'mcs_name' set num_measurements 'value'\n"
	;

	if ( argc < 4 ) {
		fprintf( output, "%s\n", usage );
		return FAILURE;
	}

	mcs_record = mx_get_record( motor_record_list, argv[2] );

	if ( mcs_record == NULL ) {
		fprintf( output, "The record '%s' does not exist.\n", argv[2] );
		return FAILURE;
	}

	mcs = (MX_MCS *) mcs_record->record_class_struct;

	if ( mcs == (MX_MCS *) NULL ) {
		fprintf( output, "MX_MCS pointer for record '%s' is NULL.\n",
				mcs_record->name );
		return FAILURE;
	}

	status = SUCCESS;

	if ( (strncmp( "count", argv[3], strlen(argv[3]) ) == 0)
	  || (strncmp( "rawcount", argv[3], strlen(argv[3]) ) == 0) )
	{
		if ( argc != 6 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		measurement_time = atof( argv[4] );

		num_measurements = atol( argv[5] );

		mx_status = mx_mcs_set_measurement_time(
				mcs_record, measurement_time );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = mx_mcs_set_num_measurements(
				mcs_record, num_measurements );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = mx_mcs_start( mcs_record );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		busy = TRUE;

		while( busy ) {
			if ( mx_kbhit() ) {
				(void) mx_getch();

				mx_status = mx_mcs_stop( mcs_record );

				if ( mx_status.code != MXE_SUCCESS )
					return FAILURE;

				fprintf( output, "MCS measurement aborted.\n" );
			}

			mx_status = mx_mcs_is_busy( mcs_record, &busy );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			mx_msleep(500);
		}

		if ( strncmp( "rawcount", argv[3], strlen(argv[3]) ) == 0 ) {
			status = motor_mcs_read_all( mcs_record, mcs );
		} else {
			status = motor_mcs_display_all( mcs_record, mcs );
		}
	} else
	if ( strncmp( "start", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc == 4 ) {
			/* A start was requested without specifying
			 * a measurement time.
			 */

			mx_status = mx_mcs_start( mcs_record );

			if ( mx_status.code != MXE_SUCCESS ) {
				return FAILURE;
			} else {
				return SUCCESS;
			}
		}
		/* If we get here a measurement time _was_ specified. */

		if ( argc != 6 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		measurement_time = atof( argv[4] );

		num_measurements = atol( argv[5] );

		mx_status = mx_mcs_set_measurement_time(
				mcs_record, measurement_time );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = mx_mcs_set_num_measurements(
				mcs_record, num_measurements );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = mx_mcs_start( mcs_record );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		busy = TRUE;

		while( busy ) {
			if ( mx_kbhit() ) {
				(void) mx_getch();

				mx_status = mx_mcs_stop( mcs_record );

				if ( mx_status.code != MXE_SUCCESS )
					return FAILURE;

				fprintf( output, "MCS measurement aborted.\n" );
			}

			mx_status = mx_mcs_is_busy( mcs_record, &busy );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			mx_msleep(500);
		}
	} else
	if ( strncmp( "saveall", argv[3], max( strlen(argv[3]), 5 ) ) == 0 ) {

		if ( argc != 5 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		mx_status = mx_mcs_read_all( mcs_record,
					&num_scalers,
					&num_measurements,
					&mcs_data );

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

		for ( i = 0; i < num_measurements; i++ ) {
			for ( j = 0; j < num_scalers; j++ ) {
				fprintf(savefile, "%10ld  ",
					(long) mcs_data[j][i]);

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
			"MCS Save file '%s' successfully written.\n",
			argv[4] );
	} else
	if ( strncmp( "save", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc != 6 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		channel = atol( argv[4] );

		mx_status = mx_mcs_read_scaler( mcs_record, channel,
						&num_measurements,
						&scaler_data );

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

		for ( i = 0; i < num_measurements; i++ ) {

			fprintf(savefile, "%10ld\n", (long) scaler_data[i]);

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
			"MCS Save file '%s' successfully written.\n",
			argv[5] );
	} else
	if ( strncmp( "readall", argv[3], max( strlen(argv[3]), 5 ) ) == 0 ) {

		if ( argc != 4 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		status = motor_mcs_display_all( mcs_record, mcs );
	} else
	if ( strncmp( "read", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc != 5 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		channel = atol( argv[4] );

		status = motor_mcs_display_plot( mcs_record, mcs, channel );
	} else
	if ( strncmp( "rawreadall", argv[3], max( strlen(argv[3]), 8 ) ) == 0 ){

		if ( argc != 4 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		status = motor_mcs_read_all( mcs_record, mcs );
	} else
	if ( strncmp( "rawread", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc != 5 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		channel = atol( argv[4] );

		status = motor_mcs_read( mcs_record, mcs, channel );
	} else
	if ( strncmp( "measurement", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc != 5 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		measurement = atol( argv[4] );

		status = motor_mcs_measurement( mcs_record, mcs, measurement );

	} else
	if ( strncmp( "stop", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc != 4 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		mx_status = mx_mcs_stop( mcs_record );

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

		mx_status = mx_mcs_clear( mcs_record );

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

		if ( strncmp( "measurement_time",
				argv[4], strlen(argv[4]) ) == 0 )
		{
			mx_status = mx_mcs_get_measurement_time(
					mcs_record, &measurement_time );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output, "MCS '%s' measurement time = %g\n",
				mcs_record->name, measurement_time );
		} else
		if ( strncmp( "num_measurements",
				argv[4], strlen(argv[4]) ) == 0 )
		{
			mx_status = mx_mcs_get_num_measurements(
					mcs_record, &num_measurements );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output,
				"MCS '%s' num measurements = %lu\n",
				mcs_record->name,
				(unsigned long) num_measurements );
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

			mx_status = mx_mcs_set_measurement_time(
					mcs_record, measurement_time );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;
		} else
		if ( strncmp( "num_measurements",
			argv[4], strlen(argv[4]) ) == 0)
		{
			if ( argc != 6 ) {
				fprintf( output,
	"%s: wrong number of arguments to 'set num_measurements' command\n",
					cname );

				fprintf( output, "%s\n", usage );
				return FAILURE;
			}

			num_measurements = strtoul( argv[5], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
"%s: Non-numeric characters found in MCS number of channels value '%s'\n",
					cname, argv[5] );
				return FAILURE;
			}

			mx_status = mx_mcs_set_num_measurements(
					mcs_record, num_measurements );

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
motor_mcs_read( MX_RECORD *mcs_record, MX_MCS *mcs, unsigned long scaler_number)
{
	mx_length_type i, num_measurements;
	int32_t *scaler_data;
	mx_status_type mx_status;

	/* Read out the acquired data. */

	fprintf( output, "About to read MCS data.\n" );

	mx_status = mx_mcs_read_scaler( mcs_record, scaler_number,
					&num_measurements, &scaler_data );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	/* Display the data. */

	fprintf( output, "MCS data successfully read.\n" );
	fprintf( output, "\n" );

#if 1
	fprintf( output, "Hit any key to continue...\n" );
	(void) mx_getch();
#endif

	for ( i = 0; i < num_measurements; i++ ) {
		if ( (i % VALUES_PER_ROW) == 0 ) {
			fprintf( output, "\n%4lu: ", (unsigned long) i );
		}

		fprintf( output, "%6ld ", (long) scaler_data[i] );

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
motor_mcs_read_all( MX_RECORD *mcs_record, MX_MCS *mcs )
{
	mx_length_type i, j, num_scalers, num_measurements;
	int32_t **mcs_data;
	mx_status_type mx_status;

	/* Read out the acquired data. */

	fprintf( output, "About to read MCS data.\n" );

	mx_status = mx_mcs_read_all( mcs_record, &num_scalers,
					&num_measurements,
					&mcs_data );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	/* Display the data. */

	fprintf( output, "MCS data successfully read.\n" );
	fprintf( output, "\n" );

#if 1
	fprintf( output, "Hit any key to continue...\n" );
	(void) mx_getch();
#endif

	for ( i = 0; i < num_measurements; i++ ) {
		fprintf( output, "\n%4lu: ", (unsigned long) i );

		for ( j = 0; j < num_scalers; j++ ) {

			fprintf( output, "%6ld ", (long) mcs_data[j][i] );
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

static int
motor_mcs_measurement( MX_RECORD *mcs_record,
			MX_MCS *mcs,
			unsigned long measurement )
{
	mx_length_type i, num_scalers;
	int32_t *measurement_data;
	mx_status_type mx_status;

	if ( measurement >= mcs->current_num_measurements ) {
		fprintf( output,
	"Illegal measurement number %lu.  The allowed range is (0-%lu).\n",
			measurement, mcs->current_num_measurements - 1L );
		return FAILURE;
	}

	mx_status = mx_mcs_read_measurement( mcs_record, measurement,
					&num_scalers, &measurement_data );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	fprintf( output, "%4lu: ", measurement );

	for ( i = 0; i < num_scalers; i++ ) {
		fprintf( output, "%6ld ", (long) measurement_data[i] );
	}

	fprintf( output, "\n" );

	return SUCCESS;
}

/* Warning: motor_mcs_display_plot() and motor_mcs_display_all() presuppose the
 *          presence of 'plotgnu.pl'.
 */

#ifdef OS_WIN32
#define popen _popen
#define pclose _pclose
#endif

static int
motor_mcs_display_plot( MX_RECORD *mcs_record,
				MX_MCS *mcs, unsigned long scaler_number )
{
	const char fname[] = "motor_mcs_display_plot()";

	MX_LIST_HEAD *list_head;
	FILE *plotgnu_pipe;
	mx_length_type i, num_measurements;
	int32_t *scaler_data;
	int status;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	/* Don't display a plot if we have been asked not to. */

	list_head = mx_get_record_list_head_struct( mcs_record );

	if ( list_head == NULL ) {
		(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"Cannot find the record list list_head structure." );

		return FAILURE;
	}

	if ( list_head->plotting_enabled == MXPF_PLOT_OFF )
		return SUCCESS;

	/* Read the data from the MCS. */

	mx_status = mx_mcs_read_scaler( mcs_record, scaler_number,
					&num_measurements, &scaler_data );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	/* Open a connetion to plotgnu. */

#if defined(OS_VXWORKS)
	fprintf( output,
	 "Plotting with Gnuplot is not supported for this operating system.\n");
	return FAILURE;
#else
	plotgnu_pipe = popen( "perl -S -- plotgnu.pl", "w" );
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

	for ( i = 0; i < num_measurements; i++ ) {
		status = fprintf( plotgnu_pipe,
				"data %lu %ld\n", (unsigned long) i,
					(long) scaler_data[i] );
	}

	status = fprintf( plotgnu_pipe, "set title 'MCS display'\n" );

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

#if !defined(OS_VXWORKS)
	status = pclose( plotgnu_pipe );
#endif

	return SUCCESS;
}

static int
motor_mcs_display_all( MX_RECORD *mcs_record, MX_MCS *mcs )
{
	const char fname[] = "motor_mcs_display_all()";

	MX_LIST_HEAD *list_head;
	FILE *plotgnu_pipe;
	mx_length_type i, j, num_scalers, num_measurements;
	int32_t **mcs_data;
	int status;
	mx_status_type mx_status;

	MX_DEBUG( 2,("%s invoked.", fname));

	/* Don't display a plot if we have been asked not to. */

	list_head = mx_get_record_list_head_struct( mcs_record );

	if ( list_head == NULL ) {
		(void) mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"Cannot find the record list list_head structure." );

		return FAILURE;
	}

	if ( list_head->plotting_enabled == MXPF_PLOT_OFF )
		return SUCCESS;

	/* Read the data from the MCS. */

	mx_status = mx_mcs_read_all( mcs_record, &num_scalers,
					&num_measurements, &mcs_data );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	/* Open a connetion to plotgnu. */

#if defined(OS_VXWORKS)
	fprintf( output,
	 "Plotting with Gnuplot is not supported for this operating system.\n");
	return FAILURE;
#else
	plotgnu_pipe = popen( "perl -S -- plotgnu.pl", "w" );
#endif

	if ( plotgnu_pipe == NULL ) {
		(void) mx_error( MXE_IPC_IO_ERROR, fname,
		"Unable to start the 'plotgnu' program." );
	}

	/* Set the connection to be line buffered. */

	setvbuf( plotgnu_pipe, (char *)NULL, _IOLBF, BUFSIZ );

	/* Tell plotgnu that we do not have any motor positions for it. */

	status = fprintf( plotgnu_pipe, "start_plot;1;0" );


	for ( j = 0; j < num_scalers; j++ ) {

		status = fprintf( plotgnu_pipe, ";$f[%lu]", (unsigned long) j );
	}
	status = fprintf( plotgnu_pipe, "\n" );

	/* Send the data to the plotting program. */

	fprintf( output,
		"Sending data to the plotting program.  Please wait...\n" );

	for ( i = 0; i < num_measurements; i++ ) {
		status = fprintf( plotgnu_pipe, "data %lu", (unsigned long) i );

		for ( j = 0; j < num_scalers; j++ ) {

		status = fprintf( plotgnu_pipe, " %ld", (long) mcs_data[j][i] );
		}

		status = fprintf( plotgnu_pipe, "\n" );
	}

	status = fprintf( plotgnu_pipe, "set title 'MCS display'\n" );

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

#if !defined(OS_VXWORKS)
	status = pclose( plotgnu_pipe );
#endif

	return SUCCESS;
}

