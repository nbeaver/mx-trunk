/*
 * Name:    mmcs.c
 *
 * Purpose: MCS control functions.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2003, 2005-2006, 2015-2016, 2019-2021
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXMTR_DEBUG_MEASUREMENT_RANGE	FALSE

#define MXMTR_DEBUG_MEASUREMENT_STATUS	FALSE

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
	unsigned long i, j, channel_number, measurement_number;
	unsigned long num_scalers;
	long last_measurement_number, total_num_measurements;
	unsigned long returned_num_measurements;
	long meas, old_last_measurement_number;
	long num_measurements_to_read;
	unsigned long requested_num_measurements, acquired_num_measurements;
	unsigned long saved_num_measurements, measurement_range_end;
	unsigned long mcs_status;
	long trigger_mode, raw_trigger_mode;
	long *scaler_data;
	long *measurement_data = NULL;
	long **measurement_range_data = NULL;
	long **mcs_data;
	int status;
	mx_status_type mx_status;

	static char usage[] =
  "Usage:  mcs 'mcs_name' count 'measurement_time' 'num_measurements\n"
  "        mcs 'mcs_name' rawcount 'measurement_time' 'num_measurements\n"
  "\n"
  "        mcs 'mcs_name' stream 'measurement_time' 'num_measurements\n"
  "        mcs 'mcs_name' rawstream 'measurement_time' 'num_measurements\n"
  "\n"
  "        mcs 'mcs_name' start [ 'measurement_time' 'num_measurements' ]\n"
  "        mcs 'mcs_name' arm [ 'measurement_time' 'num_measurements' ]\n"
  "        mcs 'mcs_name' trigger\n"
  "        mcs 'mcs_name' stop\n"
  "        mcs 'mcs_name' clear\n"
  "        mcs 'mcs_name' readall\n"
  "        mcs 'mcs_name' rawreadall\n"
  "        mcs 'mcs_name' read 'channel_number'\n"
  "        mcs 'mcs_name' rawread 'channel_number'\n"
  "        mcs 'mcs_name' measurement 'measurement_number'\n"
  "        mcs 'mcs_name' saveall 'savefile'\n"
  "        mcs 'mcs_name' save 'channel_number' 'savefile'\n"
  "        mcs 'mcs_name' savemeas 'measurement_number' 'savefile'\n"
  "        mcs 'mcs_name' get measurement_time\n"
  "        mcs 'mcs_name' set measurement_time 'seconds'\n"
  "        mcs 'mcs_name' get num_measurements\n"
  "        mcs 'mcs_name' set num_measurements 'value'\n"
  "        mcs 'mcs_name' get status\n"
  "        mcs 'mcs_name' get trigger_mode\n"
  "        mcs 'mcs_name' set trigger_mode 'trigger_mode'\n"
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

		requested_num_measurements = atol( argv[5] );

		mx_status = mx_mcs_set_measurement_time(
				mcs_record, measurement_time );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = mx_mcs_set_num_measurements(
				mcs_record, requested_num_measurements );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = mx_mcs_start( mcs_record );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mcs_status = MXSF_MCS_IS_BUSY;
		old_last_measurement_number = -1L;

		while( mcs_status & MXSF_MCS_IS_BUSY ) {
			if ( mx_kbhit() ) {
				(void) mx_getch();

				mx_status = mx_mcs_stop( mcs_record );

				if ( mx_status.code != MXE_SUCCESS )
					return FAILURE;

				fprintf( output, "MCS measurement aborted.\n" );
			}

			mx_status = mx_mcs_get_extended_status(
					mcs_record,
					&last_measurement_number,
					&total_num_measurements,
					&mcs_status );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			if ( last_measurement_number
				!= old_last_measurement_number )
			{
				fprintf( output,
				"MCS '%s': *** measurement %ld acquired ***\n",
					mcs_record->name,
					last_measurement_number );

				old_last_measurement_number
					= last_measurement_number;
			}

			mx_msleep(500);
		}

		if ( strncmp( "rawcount", argv[3], strlen(argv[3]) ) == 0 ) {
			status = motor_mcs_read_all( mcs_record, mcs );
		} else {
			status = motor_mcs_display_all( mcs_record, mcs );
		}
	} else
	if ( (strncmp( "stream", argv[3], strlen(argv[3]) ) == 0)
	  || (strncmp( "rawstream", argv[3], strlen(argv[3]) ) == 0) )
	{
		if ( argc != 6 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		measurement_time = atof( argv[4] );

		requested_num_measurements = atol( argv[5] );

		mx_status = mx_mcs_set_measurement_time(
				mcs_record, measurement_time );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = mx_mcs_set_num_measurements(
				mcs_record, requested_num_measurements );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = mx_mcs_start( mcs_record );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mcs_status = MXSF_MCS_IS_BUSY;
		old_last_measurement_number = -1L;

		acquired_num_measurements = 0;
		saved_num_measurements = 0;

		/* Wait for measurements to show up.  We read them out
		 * and print them when they do.
		 */

		while( TRUE ) {
#if MXMTR_DEBUG_MEASUREMENT_STATUS
			MX_DEBUG(-2,("mcs_status = %#lx, "
				"acquired_num_measurements = %lu, "
				"saved_num_measurements = %lu",
				mcs_status,
				acquired_num_measurements,
				saved_num_measurements ));
#endif
			if( ( mcs_status == 0 )
		    && ( acquired_num_measurements <= saved_num_measurements ) )
			{
				break;	/* Exit the while(TRUE) loop. */
			}

			if ( mx_kbhit() ) {
				(void) mx_getch();

				mx_status = mx_mcs_stop( mcs_record );

				if ( mx_status.code != MXE_SUCCESS )
					return FAILURE;

				fprintf( output, "MCS measurement aborted.\n" );
			}

			mx_status = mx_mcs_get_extended_status(
					mcs_record,
					&last_measurement_number,
					&total_num_measurements,
					&mcs_status );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

#if MXMTR_DEBUG_MEASUREMENT_STATUS
			MX_DEBUG(-2,
		    ("***: last_measurement_number = %ld, mcs_status = %#lx",
				last_measurement_number, mcs_status));
#endif

			acquired_num_measurements
				= (last_measurement_number + 1);

			num_measurements_to_read = acquired_num_measurements
						- saved_num_measurements;

			if ( num_measurements_to_read >
				mcs->maximum_measurement_range )
			{
				num_measurements_to_read = 
					mcs->maximum_measurement_range;
			}

			if ( num_measurements_to_read <= 0 ) {
				/* Go back to the top of the mcs_status
				 * while() loop.
				 */

				continue;
			}

			measurement_range_end = saved_num_measurements
					  + num_measurements_to_read - 1;

			MXW_UNUSED( measurement_range_end );

#if MXMTR_DEBUG_MEASUREMENT_RANGE
			MX_DEBUG(-2,("meas: from %ld to %ld, mcs_status = %#lx",
			    saved_num_measurements,
			    measurement_range_end,
			    mcs_status));
#endif

#if 0
			**measurement_range_data = NULL;
#endif

			mx_status = mx_mcs_read_measurement_range(
					mcs_record,
					saved_num_measurements,
					num_measurements_to_read,
					&returned_num_measurements,
					&num_scalers,
					&measurement_range_data );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			saved_num_measurements += returned_num_measurements;

#if MXMTR_DEBUG_MEASUREMENT_STATUS
			MX_DEBUG(-2,("%s: MARKER B", cname));
#endif

#if 0 && MXMTR_DEBUG_MEASUREMENT_RANGE
			MX_DEBUG(-2,("meas range read"));
#endif

			for ( meas = 0;
			    meas < returned_num_measurements;
			    meas++ )
			{
				for ( i = 0; i < num_scalers; i++ ) {
					fprintf( output, " %lu",
					    measurement_range_data[meas][i] );
				}

				fprintf( output, "\n" );
			}

			old_last_measurement_number = last_measurement_number;

			mx_msleep(500);
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

		requested_num_measurements = atol( argv[5] );

		mx_status = mx_mcs_set_measurement_time(
				mcs_record, measurement_time );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = mx_mcs_set_num_measurements(
				mcs_record, requested_num_measurements );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = mx_mcs_start( mcs_record );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mcs_status = MXSF_MCS_IS_BUSY;
		old_last_measurement_number = -1L;

		while( mcs_status & MXSF_MCS_IS_BUSY ) {
			if ( mx_kbhit() ) {
				(void) mx_getch();

				mx_status = mx_mcs_stop( mcs_record );

				if ( mx_status.code != MXE_SUCCESS )
					return FAILURE;

				fprintf( output, "MCS measurement aborted.\n" );
			}

			mx_status = mx_mcs_get_extended_status(
					mcs_record,
					&last_measurement_number,
					&total_num_measurements,
					&mcs_status );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			if ( last_measurement_number
				!= old_last_measurement_number )
			{
				fprintf( output,
				"MCS '%s': *** measurement %ld acquired ***\n",
					mcs_record->name,
					last_measurement_number );

				old_last_measurement_number
					= last_measurement_number;
			}

			mx_msleep(500);
		}
	} else
	if ( strncmp( "trigger", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc != 4 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		mx_status = mx_mcs_trigger( mcs_record );

		if ( mx_status.code != MXE_SUCCESS ) {
			return FAILURE;
		} else {
			return SUCCESS;
		}
	} else
	if ( strncmp( "arm", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc == 4 ) {
			/* A arm was requested without specifying
			 * a measurement time.
			 */

			mx_status = mx_mcs_arm( mcs_record );

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

		requested_num_measurements = atol( argv[5] );

		mx_status = mx_mcs_set_measurement_time(
				mcs_record, measurement_time );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = mx_mcs_set_num_measurements(
				mcs_record, requested_num_measurements );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = mx_mcs_arm( mcs_record );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;
	} else
	if ( strncmp( "saveall", argv[3], max( strlen(argv[3]), 5 ) ) == 0 ) {

		if ( argc != 5 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		mx_status = mx_mcs_read_all( mcs_record,
					&num_scalers,
					&requested_num_measurements,
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

		for ( i = 0; i < requested_num_measurements; i++ ) {
			for ( j = 0; j < num_scalers; j++ ) {
				fprintf(savefile, "%10ld  ", mcs_data[j][i]);

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

		channel_number = atol( argv[4] );

		mx_status = mx_mcs_read_scaler( mcs_record,
						channel_number,
						&requested_num_measurements,
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

		for ( i = 0; i < requested_num_measurements; i++ ) {

			fprintf(savefile, "%10ld\n", scaler_data[i]);

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
			"MCS scaler save file '%s' successfully written.\n",
			argv[5] );
	} else
	if ( strncmp( "savemeas", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc != 6 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		measurement_number = atol( argv[4] );

		mx_status = mx_mcs_read_measurement( mcs_record,
						measurement_number,
						&num_scalers,
						&measurement_data );

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
						
		for ( i = 0; i < num_scalers; i++ ) {
			fprintf( savefile, "%10ld\n", measurement_data[i]);

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
			"%s: cannot close save file '%s'.   Reason = '%s'\n",
				cname, argv[4], strerror(saved_errno) );

			return FAILURE;
		}
		fprintf( output,
			"MCS measurement save file '%s' successfully written\n",
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

		channel_number = atol( argv[4] );

		status = motor_mcs_display_plot( mcs_record, mcs,
							channel_number );
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

		channel_number = atol( argv[4] );

		status = motor_mcs_read( mcs_record, mcs, channel_number );
	} else
	if ( strncmp( "measurement", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc != 5 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		measurement_number = atol( argv[4] );

		status = motor_mcs_measurement( mcs_record,
						mcs, measurement_number );

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
				mcs_record, &requested_num_measurements );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output,
				"MCS '%s' num measurements = %lu\n",
				mcs_record->name, requested_num_measurements );
		} else
		if ( strncmp( "status",
				argv[4], strlen(argv[4]) ) == 0 )
		{
			mx_status = mx_mcs_get_status(
					mcs_record, &mcs_status );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output,
				"MCS '%s' status = %#lx\n",
				mcs_record->name, mcs_status );
		} else
		if ( strncmp( "trigger_mode",
				argv[4], strlen(argv[4]) ) == 0 )
		{
			mx_status = mx_mcs_get_trigger_mode(
					mcs_record, &raw_trigger_mode );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			trigger_mode = raw_trigger_mode & 0xFFF;

			if ( trigger_mode & MXF_DEV_INTERNAL_TRIGGER ) {
				fprintf( output,
				"MCS '%s' trigger mode = internal (%#lx)\n",
				mcs_record->name, trigger_mode );
			} else 
			if ( trigger_mode & MXF_DEV_EXTERNAL_TRIGGER ) {
				fprintf( output,
				"MCS '%s' trigger mode = external (%#lx)\n",
				mcs_record->name, trigger_mode );
			} else 
			if ( trigger_mode & MXF_DEV_LINE_TRIGGER ) {
				fprintf( output,
				"MCS '%s' trigger mode = line (%#lx)\n",
				mcs_record->name, trigger_mode );
			} else 
			if ( trigger_mode & MXF_DEV_AUTO_TRIGGER ) {
				fprintf( output,
				"MCS '%s' trigger mode = auto (%#lx)\n",
				mcs_record->name, trigger_mode );
			} else 
			if ( trigger_mode & MXF_DEV_DATABASE_TRIGGER ) {
				fprintf( output,
				"MCS '%s' trigger mode = database (%#lx)\n",
				mcs_record->name, trigger_mode );
			} else 
			if ( trigger_mode & MXF_DEV_MANUAL_TRIGGER ) {
				fprintf( output,
				"MCS '%s' trigger mode = manual (%#lx)\n",
				mcs_record->name, trigger_mode );
			} else {
				fprintf( output,
				"MCS '%s' trigger mode = ILLEGAL (%#lx)\n",
				mcs_record->name, trigger_mode );
			}

			if ( raw_trigger_mode & MXF_DEV_EDGE_TRIGGER ) {
				fprintf( output,
				"  Trigger is edge triggered.\n" );
			} else
			if ( raw_trigger_mode & MXF_DEV_LEVEL_TRIGGER ) {
				fprintf( output,
				"  Trigger is level triggered.\n" );
			}
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

			requested_num_measurements =
				strtoul( argv[5], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
"%s: Non-numeric characters found in MCS number of channels value '%s'\n",
					cname, argv[5] );
				return FAILURE;
			}

			mx_status = mx_mcs_set_num_measurements(
				mcs_record, requested_num_measurements );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;
		} else
		if ( strncmp( "trigger_mode", argv[4], strlen(argv[4]) ) == 0) {
			unsigned long mask;
			size_t length;

			if ( argc != 6 ) {
				fprintf( output,
			"Wrong number of arguments specified for 'set %s'.\n",
					argv[4] );
				return FAILURE;
			}

			mx_status = mx_mcs_get_trigger_mode(
						mcs_record, &raw_trigger_mode );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			mask = 0xFFF;

			trigger_mode = raw_trigger_mode & (~mask);

			length = strlen(argv[5]);

			if (mx_strncasecmp( argv[5], "internal", length ) == 0)
			{
				trigger_mode |= MXF_DEV_INTERNAL_TRIGGER;
			} else
			if (mx_strncasecmp( argv[5], "external", length ) == 0)
			{
				trigger_mode |= MXF_DEV_EXTERNAL_TRIGGER;
			} else
			if (mx_strncasecmp( argv[5], "line", length ) == 0)
			{
				trigger_mode |= MXF_DEV_LINE_TRIGGER;
			} else
			if (mx_strncasecmp( argv[5], "auto", length ) == 0)
			{
				trigger_mode |= MXF_DEV_AUTO_TRIGGER;
			} else
			if (mx_strncasecmp( argv[5], "database", length ) == 0)
			{
				trigger_mode |= MXF_DEV_DATABASE_TRIGGER;
			} else
			if (mx_strncasecmp( argv[5], "manual", length ) == 0)
			{
				trigger_mode |= MXF_DEV_MANUAL_TRIGGER;
			} else {
				trigger_mode = strtol( argv[5], &endptr, 0 );

				if ( *endptr != '\0' ) {
					fprintf( output,
		"%s: Non-numeric characters found in trigger mode '%s'\n",
						cname, argv[5] );

					return FAILURE;
				}
			}

			mx_status = mx_mcs_set_trigger_mode(
						mcs_record, trigger_mode );

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
	unsigned long i, num_measurements;
	long *scaler_data;
	mx_status_type mx_status;

	mx_breakpoint();

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
			fprintf( output, "\n%4lu: ", i );
		}

		fprintf( output, "%6ld ", scaler_data[i] );

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
	unsigned long i, j, num_scalers, num_measurements;
	long **mcs_data;
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
		fprintf( output, "\n%4lu: ", i );

		for ( j = 0; j < num_scalers; j++ ) {

			fprintf( output, "%6ld ", mcs_data[j][i] );
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
	unsigned long i, num_scalers;
	long *measurement_data;
	mx_status_type mx_status;

	if ( measurement >= mcs->current_num_measurements ) {
		fprintf( output,
	"Illegal measurement number %lu.  The allowed range is (0-%lu).\n",
			measurement, mcs->current_num_measurements - 1 );
		return FAILURE;
	}

	mx_status = mx_mcs_read_measurement( mcs_record, measurement,
					&num_scalers, &measurement_data );

	if ( mx_status.code != MXE_SUCCESS )
		return FAILURE;

	fprintf( output, "%4lu: ", measurement );

	for ( i = 0; i < num_scalers; i++ ) {
		fprintf( output, "%6ld ", measurement_data[i] );
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
	static const char fname[] = "motor_mcs_display_plot()";

	MX_LIST_HEAD *list_head;
	FILE *plotgnu_pipe;
	unsigned long i, num_measurements;
	long *scaler_data;
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

	for ( i = 0; i < num_measurements; i++ ) {
		status = fprintf( plotgnu_pipe,
					"data %lu %ld\n", i, scaler_data[i] );
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

#if !defined(OS_VXWORKS) && !defined(OS_RTEMS) && !defined(OS_ECOS)
	status = pclose( plotgnu_pipe );
#endif

	MXW_UNUSED( status );

	return SUCCESS;
}

static int
motor_mcs_display_all( MX_RECORD *mcs_record, MX_MCS *mcs )
{
	static const char fname[] = "motor_mcs_display_all()";

	MX_LIST_HEAD *list_head;
	FILE *plotgnu_pipe;
	unsigned long i, j, num_scalers, num_measurements;
	long **mcs_data;
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


	for ( j = 0; j < num_scalers; j++ ) {

		status = fprintf( plotgnu_pipe, ";$f[%lu]", j );
	}
	status = fprintf( plotgnu_pipe, "\n" );

	/* Send the data to the plotting program. */

	fprintf( output,
		"Sending data to the plotting program.  Please wait...\n" );

	for ( i = 0; i < num_measurements; i++ ) {
		status = fprintf( plotgnu_pipe, "data %lu", i );

		for ( j = 0; j < num_scalers; j++ ) {

		status = fprintf( plotgnu_pipe, " %ld", mcs_data[j][i] );
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

#if !defined(OS_VXWORKS) && !defined(OS_RTEMS) && !defined(OS_ECOS)
	status = pclose( plotgnu_pipe );
#endif

	MXW_UNUSED( status );

	return SUCCESS;
}

