/*
 * Name:    marea_detector.c
 *
 * Purpose: Video input control functions.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MAREA_DETECTOR_DEBUG_TIMING	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "motor.h"
#include "mx_key.h"
#include "mx_image.h"
#include "mx_area_detector.h"

#if MAREA_DETECTOR_DEBUG_TIMING
#  include "mx_hrt_debug.h"
#endif

int
motor_area_detector_fn( int argc, char *argv[] )
{
	static const char cname[] = "area_detector";

	static MX_IMAGE_FRAME *frame = NULL;
	static MX_IMAGE_FRAME *roi_frame = NULL;

	MX_RECORD *ad_record;
	MX_AREA_DETECTOR *ad;
	char *filename, *endptr;
	unsigned long datafile_type, correction_flags;
	long frame_number, roi_number, x_min, x_max, y_min, y_max;
	long x_binsize, y_binsize, x_framesize, y_framesize;
	long trigger_mode, bytes_per_frame, frame_type;
	double exposure_time;
	mx_bool_type busy;
	mx_status_type mx_status;

	static char usage[] =
"Usage:  area_detector 'name' snap 'exposure_time' 'file_format' 'filename'\n"
"        area_detector 'name' take frame\n"
"        area_detector 'name' write frame 'file_format' 'filename'\n"
"        area_detector 'name' write roiframe 'file_format' 'filename'\n"
"\n"
"        area_detector 'name' set exposure_time 'time in seconds'\n"
"        area_detector 'name' set continuous_mode 'time in seconds'\n"
"        area_detector 'name' set geometrical_mode '\?\?\?\?'\n"
"\n"
"        area_detector 'name' get binsize\n"
"        area_detector 'name' get bytes_per_frame\n"
"        area_detector 'name' get format\n"
"        area_detector 'name' get framesize\n"
"        area_detector 'name' get maximum_framesize\n"
"        area_detector 'name' get roi 'roi_number'\n"
"\n"
"        area_detector 'name' set binsize 'x_binsize' 'y_binsize'\n"
"        area_detector 'name' set framesize 'x_framesize' 'y_framesize'\n"
"        area_detector 'name' set roi 'roi_number' 'xmin' 'xmax' 'ymin' 'ymax'\n"
"\n"
"        area_detector 'name' set trigger_mode 'trigger mode'\n"
"        area_detector 'name' arm\n"
"        area_detector 'name' trigger\n"
"        area_detector 'name' start\n"
"        area_detector 'name' stop\n"
"        area_detector 'name' abort\n"
"        area_detector 'name' get busy\n"
"        area_detector 'name' get frame 'frame_number'\n"
"        area_detector 'name' get roiframe 'roi_number'\n"
"\n"
"        area_detector 'name' readout 'frame_number'\n"
"        area_detector 'name' correct 'correction_flags'\n"
"        area_detector 'name' transfer 'frame_type'\n";

#if MAREA_DETECTOR_DEBUG_TIMING
	MX_HRT_TIMING measurement1, measurement2, measurement3, measurement4;
#endif

	if ( argc < 4 ) {
		fprintf( output, "%s\n", usage );
		return FAILURE;
	}

#if 0
	{
		int i;

		for ( i = 0; i < argc; i++ ) {
			MX_DEBUG(-2,("argv[%d] = '%s'", i, argv[i]));
		}
	}
#endif

	ad_record = mx_get_record( motor_record_list, argv[2] );

	if ( ad_record == NULL ) {
		fprintf( output, "The record '%s' does not exist.\n", argv[2] );
		return FAILURE;
	}

	ad = (MX_AREA_DETECTOR *) ad_record->record_class_struct;

	if ( ad == (MX_AREA_DETECTOR *) NULL ) {
		fprintf( output,
			"MX_AREA_DETECTOR pointer for record '%s' is NULL.\n",
				ad_record->name );
		return FAILURE;
	}

	if ( strncmp( "snap", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc < 5 ) {
			fprintf( output,
			"%s: not enough arguments to 'snap' command\n",
				cname );

			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		exposure_time = atof( argv[4] );

		if ( strcmp( argv[5], "pnm" ) == 0 ) {
			datafile_type = MXT_IMAGE_FILE_PNM;
		} else
		if ( strcmp( argv[5], "tiff" ) == 0 ) {
			datafile_type = MXT_IMAGE_FILE_TIFF;
		} else {
			fprintf( output,
				"%s: Unrecognized datafile type '%s'\n",
				cname, argv[5] );

			return FAILURE;
		}

		filename = argv[6];

		mx_status = mx_area_detector_set_exposure_time( ad_record,
								exposure_time );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

#if MAREA_DETECTOR_DEBUG_TIMING
		MX_HRT_START( measurement1 );
#endif

		mx_status = mx_area_detector_start( ad_record );

#if MAREA_DETECTOR_DEBUG_TIMING
		MX_HRT_END( measurement1 );
#endif
		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

#if MAREA_DETECTOR_DEBUG_TIMING
		MX_HRT_START( measurement2 );
#endif

		for(;;) {
			mx_status = mx_area_detector_is_busy(ad_record, &busy);

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			if ( mx_kbhit() ) {
				(void) mx_getch();

				(void) mx_area_detector_stop( ad_record );

				fprintf( output,
				"%s: The wait for area detector '%s' "
				"to finish was interrupted.\n",
					cname, ad_record->name );

				return FAILURE;
			}

			if ( busy == FALSE ) {
				break;		/* Exit the for(;;) loop. */
			}
			mx_msleep(10);
		}
#if MAREA_DETECTOR_DEBUG_TIMING
		MX_HRT_END( measurement2 );
#endif

#if MAREA_DETECTOR_DEBUG_TIMING
		MX_HRT_START( measurement3 );
#endif
		mx_status = mx_area_detector_get_frame( ad_record,
							-1, &frame );

#if MAREA_DETECTOR_DEBUG_TIMING
		MX_HRT_END( measurement3 );
#endif
		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

#if MAREA_DETECTOR_DEBUG_TIMING
		MX_HRT_START( measurement4 );
#endif
		mx_status = mx_write_image_file( frame,
						datafile_type, NULL,
						filename );
#if MAREA_DETECTOR_DEBUG_TIMING
		MX_HRT_END( measurement4 );
#endif

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

#if MAREA_DETECTOR_DEBUG_TIMING
		MX_HRT_RESULTS( measurement1, cname, "for Start" );
		MX_HRT_RESULTS( measurement2, cname, "for Busy/Wait" );
		MX_HRT_RESULTS( measurement3, cname, "for Get Frame" );
		MX_HRT_RESULTS( measurement4, cname, "for Write Frame" );
#endif
	} else
	if ( strncmp( "take", argv[3], strlen(argv[3]) ) == 0 ) {

		mx_status = mx_area_detector_start( ad_record );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		for(;;) {
			mx_status = mx_area_detector_is_busy(ad_record, &busy);

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			if ( mx_kbhit() ) {
				(void) mx_getch();

				(void) mx_area_detector_stop( ad_record );

				fprintf( output,
				"%s: The wait for area detector '%s' "
				"to finish was interrupted.\n",
					cname, ad_record->name );

				return FAILURE;
			}

			if ( busy == FALSE ) {
				break;		/* Exit the for(;;) loop. */
			}
			mx_msleep(10);
		}

		mx_status = mx_area_detector_get_frame( ad_record,
							-1, &frame );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

	} else
	if ( strncmp( "write", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc < 7 ) {
			fprintf( output,
			"%s: not enough arguments to 'write' command\n",
				cname );

			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		if ( strcmp( argv[5], "pnm" ) == 0 ) {
			datafile_type = MXT_IMAGE_FILE_PNM;
		} else
		if ( strcmp( argv[5], "tiff" ) == 0 ) {
			datafile_type = MXT_IMAGE_FILE_TIFF;
		} else {
			fprintf( output,
				"%s: Unrecognized datafile type '%s'\n",
				cname, argv[5] );

			return FAILURE;
		}

		filename = argv[6];

		if ( strncmp( "frame", argv[4], strlen(argv[4]) ) == 0 ) {

			if ( frame == NULL ) {
				fprintf( output,
		"%s: no area detector image frame has been taken yet.\n",
					cname );

				return FAILURE;
			}

			mx_status = mx_write_image_file( frame,
						datafile_type, NULL,
						filename );
		} else
		if ( strncmp( "roiframe", argv[4], strlen(argv[4]) ) == 0 ) {

			if ( roi_frame == NULL ) {
				fprintf( output,
		"%s: no area detector ROI frame has been taken yet.\n",
					cname );

				return FAILURE;
			}

			mx_status = mx_write_image_file( roi_frame,
						datafile_type, NULL,
						filename );
		} else {
			fprintf( output,
			"%s: Illegal 'write' argument '%s'\n",
				cname, argv[4] );
			return FAILURE;
		}

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;
	} else
	if ( strncmp( "arm", argv[3], strlen(argv[3]) ) == 0 ) {

#if MAREA_DETECTOR_DEBUG_TIMING
		MX_HRT_START( measurement1 );
#endif
		mx_status = mx_area_detector_arm( ad_record );

#if MAREA_DETECTOR_DEBUG_TIMING
		MX_HRT_END( measurement1 );
#endif
		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

#if MAREA_DETECTOR_DEBUG_TIMING
		MX_HRT_RESULTS( measurement1, cname, "for Arm" );
#endif
	} else
	if ( strncmp( "trigger", argv[3], strlen(argv[3]) ) == 0 ) {

#if MAREA_DETECTOR_DEBUG_TIMING
		MX_HRT_START( measurement1 );
#endif
		mx_status = mx_area_detector_trigger( ad_record );

#if MAREA_DETECTOR_DEBUG_TIMING
		MX_HRT_END( measurement1 );
#endif
		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

#if MAREA_DETECTOR_DEBUG_TIMING
		MX_HRT_RESULTS( measurement1, cname, "for Trigger" );
#endif
	} else
	if ( strncmp( "start", argv[3], strlen(argv[3]) ) == 0 ) {

		mx_status = mx_area_detector_start( ad_record );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;
	} else
	if ( strncmp( "stop", argv[3], strlen(argv[3]) ) == 0 ) {

		mx_status = mx_area_detector_stop( ad_record );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;
	} else
	if ( strncmp( "abort", argv[3], strlen(argv[3]) ) == 0 ) {

		mx_status = mx_area_detector_abort( ad_record );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

	} else
	if ( strncmp( "readout", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc < 5 ) {
			fprintf( output,
			"%s: not enough arguments to 'readout' command\n",
				cname );

			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		frame_number = strtol( argv[4], &endptr, 0 );

		if ( *endptr != '\0' ) {
			fprintf( output,
		"%s: Non-numeric characters found in frame number '%s'\n",
				cname, argv[4] );

			return FAILURE;
		}

		mx_status = mx_area_detector_setup_frame( ad_record, &frame );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = mx_area_detector_readout_frame(
						ad_record, frame_number );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;
	} else
	if ( strncmp( "correct", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc < 5 ) {
			fprintf( output,
			"%s: not enough arguments to 'correct' command\n",
				cname );

			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		correction_flags = mx_hex_string_to_unsigned_long( argv[4] );

		mx_status = mx_area_detector_correct_frame( ad_record );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;
	} else
	if ( strncmp( "transfer", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc < 5 ) {
			fprintf( output,
			"%s: not enough arguments to 'transfer' command\n",
				cname );

			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		frame_type = strtol( argv[4], &endptr, 0 );

		if ( *endptr != '\0' ) {
			fprintf( output,
		"%s: Non-numeric characters found in frame type '%s'\n",
				cname, argv[4] );

			return FAILURE;
		}

		mx_status = mx_area_detector_transfer_frame( ad_record,
								frame_type );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;
	} else
	if ( strncmp( "get", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc < 5 ) {
			fprintf( output,
			"%s: not enough arguments to 'get' command\n",
				cname );

			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		if ( strncmp( "frame", argv[4], strlen(argv[4]) ) == 0 ) {

			if ( argc != 6 ) {
				fprintf( output,
		"%s: wrong number of arguments to 'get frame' command\n",
					cname );

				fprintf( output, "%s\n", usage );
				return FAILURE;
			}

			frame_number = strtol( argv[5], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
		"%s: Non-numeric characters found in frame number '%s'\n",
					cname, argv[5] );

				return FAILURE;
			}

			mx_status = mx_area_detector_get_frame( ad_record,
							frame_number, &frame );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;
		} else
		if ( strncmp( "bytes_per_frame",
				argv[4], strlen(argv[4]) ) == 0 )
		{
			mx_status = mx_area_detector_get_bytes_per_frame(
						ad_record, &bytes_per_frame );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output,
			"Area detector '%s': bytes per frame = %lu\n",
				ad_record->name, bytes_per_frame );
		} else
		if ( strncmp( "format", argv[4], strlen(argv[4]) ) == 0 ) {

			mx_status = mx_area_detector_get_image_format(
						ad_record, NULL );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output,
			"Area detector '%s': image format = '%s'\n",
				ad_record->name, ad->image_format_name );
		} else
		if ( strncmp( "binsize", argv[4], strlen(argv[4]) ) == 0 ) {

			mx_status = mx_area_detector_get_binsize( ad_record,
						&x_binsize, &y_binsize );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output,
		"Area detector '%s': X bin size = %lu, Y bin size = %lu\n",
				ad_record->name, x_binsize, y_binsize );
		} else
		if ( strncmp( "busy", argv[4], strlen(argv[4]) ) == 0 ) {

			mx_status = mx_area_detector_is_busy( ad_record, &busy);

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output, "Area detector '%s': busy = %d\n",
						ad_record->name, (int) busy );
		} else
		if ( strncmp( "framesize", argv[4], strlen(argv[4]) ) == 0 ) {

			mx_status = mx_area_detector_get_framesize( ad_record,
						&x_framesize, &y_framesize );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output,
		"Area detector '%s': X frame size = %lu, Y frame size = %lu\n",
				ad_record->name, x_framesize, y_framesize );
		} else
		if ( strncmp( "maximum_framesize",
				argv[4], strlen(argv[4]) ) == 0 )
		{
			mx_status = mx_area_detector_get_maximum_framesize(
				ad_record, &x_framesize, &y_framesize );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output,
"Area detector '%s': X maximum frame size = %lu, Y maximum frame size = %lu\n",
				ad_record->name, x_framesize, y_framesize );
		} else
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

			mx_status = mx_area_detector_get_roi( ad_record,
				roi_number, &x_min, &x_max, &y_min, &y_max );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output,
"Area detector '%s' ROI %lu: Xmin = %lu, Xmax = %lu, Ymin = %lu, Ymax = %lu\n",
				ad_record->name, roi_number,
				x_min, x_max, y_min, y_max );

		} else
		if ( strncmp( "roiframe", argv[4], strlen(argv[4]) ) == 0 ) {

			if ( argc != 6 ) {
				fprintf( output,
		"%s: wrong number of arguments to 'get roiframe' command\n",
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

			mx_status = mx_area_detector_get_roi_frame(
						ad_record, frame,
						roi_number, &roi_frame );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;
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

		if ( strncmp( "binsize", argv[4], strlen(argv[4]) ) == 0 ) {
			
			x_binsize = strtoul( argv[5], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
		"%s: Non-numeric characters found in X binsize '%s'\n",
					cname, argv[5] );

				return FAILURE;
			}

			y_binsize = strtoul( argv[6], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
		"%s: Non-numeric characters found in Y binsize value '%s'\n",
					cname, argv[5] );

				return FAILURE;
			}

			mx_status = mx_area_detector_set_binsize( ad_record,
							x_binsize, y_binsize );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;
		} else
		if ( strncmp( "framesize", argv[4], strlen(argv[4]) ) == 0 ) {
			
			x_framesize = strtoul( argv[5], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
		"%s: Non-numeric characters found in X framesize '%s'\n",
					cname, argv[5] );

				return FAILURE;
			}

			y_framesize = strtoul( argv[6], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
		"%s: Non-numeric characters found in Y framesize value '%s'\n",
					cname, argv[5] );

				return FAILURE;
			}

			mx_status = mx_area_detector_set_framesize( ad_record,
						x_framesize, y_framesize );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;
		} else
		if ( strncmp( "trigger_mode", argv[4], strlen(argv[4]) ) == 0) {
			
			trigger_mode = strtoul( argv[5], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
		"%s: Non-numeric characters found in trigger mode '%s'\n",
					cname, argv[5] );

				return FAILURE;
			}

			mx_status = mx_area_detector_set_trigger_mode(
						    ad_record, trigger_mode );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;
		} else
		if ( strncmp( "exposure_time", argv[4], strlen(argv[4])) == 0 ){

			exposure_time = atof( argv[5] );

			mx_status = mx_area_detector_set_exposure_time(
						    ad_record, exposure_time );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;
		} else
		if ( strncmp("continuous_mode", argv[4], strlen(argv[4])) == 0){

			exposure_time = atof( argv[5] );

			mx_status = mx_area_detector_set_continuous_mode(
						    ad_record, exposure_time );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;
		} else
		if ( strncmp("geometrical_mode", argv[4], strlen(argv[4])) == 0)
		{
			fprintf( stderr,
			"%s: Geometrical mode not yet implemented.\n", cname );
				return FAILURE;
		} else
		if ( strncmp( "roi", argv[4], strlen(argv[4]) ) == 0 ) {

			if ( argc != 10 ) {
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

			x_min = strtoul( argv[6], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
		"%s: Non-numeric characters found in ROI Xmin value '%s'\n",
					cname, argv[5] );

				return FAILURE;
			}

			x_max = strtoul( argv[7], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
		"%s: Non-numeric characters found in ROI Xmax value '%s'\n",
					cname, argv[7] );

				return FAILURE;
			}

			y_min = strtoul( argv[8], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
		"%s: Non-numeric characters found in ROI Ymin value '%s'\n",
					cname, argv[8] );

				return FAILURE;
			}

			y_max = strtoul( argv[9], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
		"%s: Non-numeric characters found in ROI Ymax value '%s'\n",
					cname, argv[9] );

				return FAILURE;
			}

			mx_status = mx_area_detector_set_roi( ad_record,
				      roi_number, x_min, x_max, y_min, y_max );

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

	return SUCCESS;
}

