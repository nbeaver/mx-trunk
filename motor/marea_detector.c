/*
 * Name:    marea_detector.c
 *
 * Purpose: Video input control functions.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2006-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MAREA_DETECTOR_DEBUG_TIMING	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "motor.h"
#include "mx_key.h"
#include "mx_hrt.h"
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
	MX_SEQUENCE_PARAMETERS seq_params;
	char *filename, *endptr;
	char *filename_stem;
	char filename_ext[20];
	char frame_filename[MXU_FILENAME_LENGTH+1];
	unsigned long datafile_type, correction_flags;
	long frame_number;
	long x_binsize, y_binsize, x_framesize, y_framesize;
	long trigger_mode, bytes_per_frame, num_frames;
	long frame_type, src_frame_type, dest_frame_type;
	long i, last_frame_number, total_num_frames;
	long old_total_num_frames, starting_total_num_frames;
	unsigned long ad_status, roi_number;
	unsigned long roi[4];
	long long_parameter;
	long correction_type, num_measurements;
	double measurement_time;
	double exposure_time, gap_time, exposure_multiplier, gap_multiplier;
	double bytes_per_pixel;
	mx_bool_type busy;
	mx_status_type mx_status;

	static char usage[] =
"Usage:\n"
"  area_detector 'name' get bytes_per_frame\n"
"  area_detector 'name' get bytes_per_pixel\n"
"  area_detector 'name' get format\n"
"  area_detector 'name' get framesize\n"
"  area_detector 'name' set framesize 'x_framesize' 'y_framesize'\n"
"  area_detector 'name' get maximum_framesize\n"
"\n"
"  area_detector 'name' get trigger_mode\n"
"  area_detector 'name' set trigger_mode 'trigger mode'\n"
"  area_detector 'name' get correction_flags\n"
"  area_detector 'name' set correction_flags 'correction flags'\n"
"\n"
"  area_detector 'name' get long_parameter 'parameter_name'\n"
"  area_detector 'name' set long_parameter 'parameter_name' 'parameter_value'\n"
"\n"
"  area_detector 'name' get sequence_parameters\n"
"  area_detector 'name' set one_shot_mode 'exposure time in seconds'\n"
"  area_detector 'name' set continuous_mode 'exposure time in seconds'\n"
"  area_detector 'name' set multiframe_mode '# frames'\n"
"                                                'exposure time' 'frame_time'\n"
"  area_detector 'name' set circular_multiframe_mode '# frames'\n"
"                                                'exposure time' 'frame_time'\n"
"  area_detector 'name' set strobe_mode '# frames' 'exposure time'\n"
"  area_detector 'name' set bulb_mode '# frames'\n"
"  area_detector 'name' set geometrical_mode '# frames'\n"
"          'exposure time' 'gap_time' 'exposure multiplier' 'gap multiplier'\n"
"\n"
"  area_detector 'name' get binsize\n"
"  area_detector 'name' set binsize 'x_binsize' 'y_binsize'\n"
"  area_detector 'name' get roi 'roi_number'\n"
"  area_detector 'name' set roi 'roi_number' 'xmin' 'xmax' 'ymin' 'ymax'\n"
"\n"
"  area_detector 'name' snap 'exposure_time' 'file_format' 'filename'\n"
"  area_detector 'name' take frame\n"
"  area_detector 'name' write frame 'file_format' 'filename'\n"
"  area_detector 'name' write roiframe 'file_format' 'filename'\n"
"  area_detector 'name' sequence 'file_format' 'filename'\n"
"\n"
"  area_detector 'name' arm\n"
"  area_detector 'name' trigger\n"
"  area_detector 'name' start\n"
"  area_detector 'name' stop\n"
"  area_detector 'name' abort\n"
"  area_detector 'name' get last_frame_number\n"
"  area_detector 'name' get status\n"
"  area_detector 'name' get extended_status\n"
"  area_detector 'name' get busy\n"
"  area_detector 'name' get frame 'frame_number'\n"
"  area_detector 'name' get roiframe 'roi_number'\n"
"\n"
"  area_detector 'name' readout 'frame_number'\n"
"  area_detector 'name' correct\n"
"  area_detector 'name' transfer 'frame_type'\n"
"\n"
"  area_detector 'name' load frame 'frame_type' 'filename'\n"
"  area_detector 'name' save frame 'frame_type' 'filename'\n"
"  area_detector 'name' copy frame 'dest_frame_type' 'src_frame_type'\n"
"\n"
"  area_detector 'name' measure dark_current 'measurement_time' '# measurements'\n"
"  area_detector 'name' measure flood_field 'measurement_time' '# measurements'\n";

#if MAREA_DETECTOR_DEBUG_TIMING
	MX_HRT_TIMING measurement1, measurement2, measurement3, measurement4;
#endif

	strlcpy( frame_filename, "", sizeof(frame_filename) );

	if ( argc < 4 ) {
		fprintf( output, "%s\n", usage );
		return FAILURE;
	}

#if 0
	for ( i = 0; i < argc; i++ ) {
		MX_DEBUG(-2,("argv[%d] = '%s'", i, argv[i]));
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

		if ( argc < 6 ) {
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
		} else
		if ( strcmp( argv[5], "smv" ) == 0 ) {
			datafile_type = MXT_IMAGE_FILE_SMV;
		} else {
			fprintf( output,
				"%s: Unrecognized datafile type '%s'\n",
				cname, argv[5] );

			return FAILURE;
		}

		filename = argv[6];

		mx_status = mx_area_detector_set_one_shot_mode( ad_record,
								exposure_time );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

#if MAREA_DETECTOR_DEBUG_TIMING
		MX_HRT_START( measurement1 );
#endif

		fprintf( output,
		"Starting a %g second exposure for area detector '%s'.  ",
			exposure_time, ad_record->name );

		fflush( output );

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

		fprintf( output,
		"Exposure complete.\nNow transferring the frame.  " );

		fflush( output );

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

		fprintf( output,
		"Transfer complete.\nNow writing image file '%s'.\n",
			filename );

		fflush( output );

#if MAREA_DETECTOR_DEBUG_TIMING
		MX_HRT_START( measurement4 );
#endif
		mx_status = mx_image_write_file( frame,
						datafile_type, filename );
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
		fprintf( output,
			"Image file '%s' successfully written.\n", filename );
	} else
	if ( strncmp( "sequence", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc < 5 ) {
			fprintf( output,
			"%s: not enough arguments to 'snap' command\n",
				cname );

			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		if ( strcmp( argv[4], "pnm" ) == 0 ) {
			datafile_type = MXT_IMAGE_FILE_PNM;
			strlcpy( filename_ext, "pnm", sizeof(filename_ext) );
		} else
		if ( strcmp( argv[4], "tiff" ) == 0 ) {
			datafile_type = MXT_IMAGE_FILE_TIFF;
			strlcpy( filename_ext, "tiff", sizeof(filename_ext) );
		} else
		if ( strcmp( argv[4], "smv" ) == 0 ) {
			datafile_type = MXT_IMAGE_FILE_SMV;
			strlcpy( filename_ext, "smv", sizeof(filename_ext) );
		} else {
			fprintf( output,
				"%s: Unrecognized datafile type '%s'\n",
				cname, argv[4] );

			return FAILURE;
		}

		filename_stem = argv[5];

		mx_status = mx_area_detector_get_extended_status( ad_record,
						&last_frame_number,
						&starting_total_num_frames,
						&ad_status );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		fprintf( output, "Starting sequence\n" );
		fflush( output );

		mx_status = mx_area_detector_start( ad_record );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		old_total_num_frames = starting_total_num_frames;

		for(;;) {
			if ( mx_kbhit() ) {
				(void) mx_getch();

				(void) mx_area_detector_stop( ad_record );

				fprintf( output,
			    "%s: Area detector '%s' sequence interrupted.\n",
					cname, ad_record->name );

				return FAILURE;
			}

			mx_status = mx_area_detector_get_extended_status(
					ad_record,
					&last_frame_number,
					&total_num_frames,
					&ad_status );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			if ( total_num_frames != old_total_num_frames ) {
				fprintf( output, "Reading frame %ld.\n",
					last_frame_number );

#if 0
				mx_status = mx_area_detector_get_frame(
					ad_record, last_frame_number, &frame );

				if ( mx_status.code != MXE_SUCCESS )
					return FAILURE;

				snprintf(frame_filename, sizeof(frame_filename),
					"%s%ld.%s", filename_stem,
				   total_num_frames - starting_total_num_frames,
					filename_ext );

				fprintf( output, "Writing image file '%s'.  ",
					frame_filename );

				mx_status = mx_image_write_file( frame,
						datafile_type, frame_filename );

				if ( mx_status.code != MXE_SUCCESS )
					return FAILURE;

				fprintf( output, "Complete.\n" );
#endif
				old_total_num_frames = total_num_frames;
			}

			if ( (ad_status & MXSF_AD_IS_BUSY) == FALSE ) {

				break;		/* Exit the for(;;) loop. */
			}
			mx_msleep(10);
		}

		fprintf( output, "Sequence complete.\n" );
		fflush( output );
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

			mx_status = mx_image_write_file( frame,
						datafile_type, filename );
		} else
		if ( strncmp( "roiframe", argv[4], strlen(argv[4]) ) == 0 ) {

			if ( roi_frame == NULL ) {
				fprintf( output,
		"%s: no area detector ROI frame has been taken yet.\n",
					cname );

				return FAILURE;
			}

			mx_status = mx_image_write_file( roi_frame,
						datafile_type, filename );
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

		if ( argc != 4 ) {
			fprintf( output,
			"%s: not enough arguments to 'correct' command\n",
				cname );

			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

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
	if ( strncmp( "load", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc < 7 ) {
			fprintf( output,
			"%s: not enough arguments to 'load frame' command\n",
				cname );

			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		if ( strncmp( "frame", argv[4], strlen(argv[4]) ) != 0 ) {
			return FAILURE;
		}

		frame_type = strtol( argv[5], &endptr, 0 );

		if ( *endptr != '\0' ) {
			fprintf( output,
		"%s: Non-numeric characters found in frame type '%s'\n",
				cname, argv[5] );

			return FAILURE;
		}

		mx_status = mx_area_detector_load_frame( ad_record,
						frame_type, argv[6] );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;
	} else
	if ( strncmp( "save", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc < 7 ) {
			fprintf( output,
			"%s: not enough arguments to 'save frame' command\n",
				cname );

			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		if ( strncmp( "frame", argv[4], strlen(argv[4]) ) != 0 ) {
			return FAILURE;
		}

		frame_type = strtol( argv[5], &endptr, 0 );

		if ( *endptr != '\0' ) {
			fprintf( output,
		"%s: Non-numeric characters found in frame type '%s'\n",
				cname, argv[5] );

			return FAILURE;
		}

		mx_status = mx_area_detector_save_frame( ad_record,
						frame_type, argv[6] );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;
	} else
	if ( strncmp( "copy", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc < 7 ) {
			fprintf( output,
			"%s: not enough arguments to 'copy frame' command\n",
				cname );

			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		if ( strncmp( "frame", argv[4], strlen(argv[4]) ) != 0 ) {
			return FAILURE;
		}

		dest_frame_type = strtol( argv[5], &endptr, 0 );

		if ( *endptr != '\0' ) {
			fprintf( output,
	"%s: Non-numeric characters found in destination frame type '%s'\n",
				cname, argv[5] );

			return FAILURE;
		}

		src_frame_type = strtol( argv[6], &endptr, 0 );

		if ( *endptr != '\0' ) {
			fprintf( output,
		"%s: Non-numeric characters found in source frame type '%s'\n",
				cname, argv[6] );

			return FAILURE;
		}

		mx_status = mx_area_detector_copy_frame( ad_record,
					dest_frame_type, src_frame_type );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;
	} else
	if ( strncmp( "measure", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc < 5 ) {
			fprintf( output,
			"%s: not enough arguments to 'measure' command\n",
				cname );

			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		if ( strncmp( "dark_current", argv[4], strlen(argv[4]) ) == 0 )
		{
			correction_type = MXFT_AD_DARK_CURRENT_FRAME;
		} else
		if ( strncmp( "flood_field", argv[4], strlen(argv[4]) ) == 0 )
		{
			correction_type = MXFT_AD_FLOOD_FIELD_FRAME;
		} else {
			fprintf( output,
    "%s: unsupported correction type '%s' requested for 'measure' command.\n",
    				cname, argv[4] );
			return FAILURE;
		}

		if ( argc < 6 ) {
			measurement_time = 1.0;
		} else {
			measurement_time = atof( argv[5] );
		}

		if ( argc < 7 ) {
			num_measurements = 1;
		} else {
			num_measurements = strtol( argv[6], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
	"%s: Non-numeric characters found in destination frame type '%s'\n",
				cname, argv[6] );

				return FAILURE;
			}
		}

		/* Start the correction measurement. */

		switch( correction_type ) {
		case MXFT_AD_DARK_CURRENT_FRAME:
			fprintf( output,
				"Starting dark current measurement:\n");
			break;
		case MXFT_AD_FLOOD_FIELD_FRAME:
			fprintf( output,
				"Starting flood field measurement:\n");
			break;
		}

		fprintf( output,
"  measurement time per frame = %g seconds, number of measurements = %ld\n",
			measurement_time, num_measurements );

		mx_status = mx_area_detector_measure_correction_frame(
			ad_record, correction_type,
			measurement_time, num_measurements );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		/* Wait for the correction measurement to complete. */

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

		switch( correction_type ) {
		case MXFT_AD_DARK_CURRENT_FRAME:
			fprintf( output,
			  "Dark current measurement completed successfully.\n");
			break;
		case MXFT_AD_FLOOD_FIELD_FRAME:
			fprintf( output,
			  "Flood field measurement completed successfully.\n");
			break;
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
		"%s: Non-numeric characters found in frame number '%s'\n", cname, argv[5] );

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
		if ( strncmp( "bytes_per_pixel",
				argv[4], strlen(argv[4]) ) == 0 )
		{
			mx_status = mx_area_detector_get_bytes_per_pixel(
						ad_record, &bytes_per_pixel );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output,
			"Area detector '%s': bytes per pixel = %g\n",
				ad_record->name, bytes_per_pixel );
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
		if ( strncmp( "last_frame_number",
					argv[4], strlen(argv[4]) ) == 0 )
		{
			mx_status = mx_area_detector_get_last_frame_number(
						ad_record, &last_frame_number );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output,
			"Area detector '%s': last frame number = %ld\n",
					ad_record->name, last_frame_number );
		} else
		if ( strncmp( "status", argv[4], strlen(argv[4]) ) == 0 ) {
			mx_status = mx_area_detector_get_status(
						ad_record, &ad_status );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output, "Area detector '%s': status = %#lx\n",
						ad_record->name, ad_status );
		} else
		if ( strncmp( "extended_status",
					argv[4], strlen(argv[4]) ) == 0 )
		{
			mx_status = mx_area_detector_get_extended_status(
				ad_record, &last_frame_number,
				&total_num_frames, &ad_status );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output,
		"Area detector '%s': last frame number = %ld, "
		"total num frames = %ld, status = %#lx\n",
				ad_record->name, last_frame_number,
				total_num_frames, ad_status );
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
		if ( strncmp( "trigger_mode", argv[4], strlen(argv[4]) ) == 0 )
		{
			mx_status = mx_area_detector_get_trigger_mode(
						ad_record, &trigger_mode );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output,
				"Area detector '%s': trigger mode = %ld\n",
				ad_record->name, trigger_mode );
		} else
		if ( strncmp( "correction_flags",
					argv[4], strlen(argv[4]) ) == 0 )
		{
			mx_status = mx_area_detector_get_correction_flags(
						ad_record, &correction_flags );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output,
				"Area detector '%s': correction flags = %#lx\n",
				ad_record->name, correction_flags );
		} else
		if ( strncmp( "long_parameter",
					argv[4], strlen(argv[4]) ) == 0 )
		{
			if ( argc != 6 ) {
				fprintf( output,
	    "%s: wrong number of arguments to 'get long_parameter' command\n",
					cname );

				fprintf( output, "%s\n", usage );
				return FAILURE;
			}

			mx_status = mx_area_detector_get_long_parameter(
					ad_record, argv[5], &long_parameter );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output,
			"Area detector '%s': parameter '%s' long value = %ld\n",
				ad_record->name, argv[5], long_parameter );
		} else
		if ( strncmp( "sequence_parameters",
					argv[4], strlen(argv[4]) ) == 0 )
		{
			mx_status = mx_area_detector_get_sequence_parameters(
						ad_record, &seq_params );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output,
	"Area detector '%s': sequence type = %ld, num parameters = %ld\n"
	"    parameter array =", ad_record->name,
			seq_params.sequence_type, seq_params.num_parameters);

			for ( i = 0; i < seq_params.num_parameters; i++ ) {
				fprintf( output, " %g",
					seq_params.parameter_array[i] );
			}
			fprintf( output, "\n" );
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
							roi_number, roi );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output,
"Area detector '%s' ROI %lu: Xmin = %lu, Xmax = %lu, Ymin = %lu, Ymax = %lu\n",
				ad_record->name, roi_number,
				roi[0], roi[1], roi[2], roi[3] );

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
			
			if ( argc != 7 ) {
				fprintf( output,
			"Wrong number of arguments specified for 'set %s'.\n",
					argv[4] );
				return FAILURE;
			}

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

			fprintf( output,
		      "new binsize = (%ld, %ld), new framesize = (%ld, %ld)\n",
				ad->binsize[0], ad->binsize[1],
				ad->framesize[0], ad->framesize[1] );
		} else
		if ( strncmp( "framesize", argv[4], strlen(argv[4]) ) == 0 ) {
			
			if ( argc != 7 ) {
				fprintf( output,
			"Wrong number of arguments specified for 'set %s'.\n",
					argv[4] );
				return FAILURE;
			}

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

			fprintf( output,
		      "new binsize = (%ld, %ld), new framesize = (%ld, %ld)\n",
				ad->binsize[0], ad->binsize[1],
				ad->framesize[0], ad->framesize[1] );
		} else
		if ( strncmp( "trigger_mode", argv[4], strlen(argv[4]) ) == 0) {
			
			if ( argc != 6 ) {
				fprintf( output,
			"Wrong number of arguments specified for 'set %s'.\n",
					argv[4] );
				return FAILURE;
			}

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
		if ( strncmp( "correction_flags",
					argv[4], strlen(argv[4]) ) == 0)
		{
			if ( argc != 6 ) {
				fprintf( output,
			"Wrong number of arguments specified for 'set %s'.\n",
					argv[4] );
				return FAILURE;
			}

			correction_flags = strtoul( argv[5], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
		"%s: Non-numeric characters found in trigger mode '%s'\n",
					cname, argv[5] );

				return FAILURE;
			}


			mx_status = mx_area_detector_set_correction_flags(
					    ad_record, correction_flags );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;
		} else
		if ( strncmp( "long_parameter",
					argv[4], strlen(argv[4]) ) == 0 )
		{
			if ( argc != 7 ) {
				fprintf( output,
	    "%s: wrong number of arguments to 'set long_parameter' command\n",
					cname );

				fprintf( output, "%s\n", usage );
				return FAILURE;
			}

			long_parameter = strtol( argv[6], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
	"%s: Non-numeric characters found in the long parameter value '%s'\n",
					cname, argv[6] );

				return FAILURE;
			}

			mx_status = mx_area_detector_set_long_parameter(
					ad_record, argv[5], &long_parameter );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;
		} else
		if ( strncmp( "one_shot_mode", argv[4], strlen(argv[4])) == 0 ){

			if ( argc != 6 ) {
				fprintf( output,
			"Wrong number of arguments specified for 'set %s'.\n",
					argv[4] );
				return FAILURE;
			}

			exposure_time = atof( argv[5] );

			mx_status = mx_area_detector_set_one_shot_mode(
						    ad_record, exposure_time );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;
		} else
		if ( strncmp("continuous_mode", argv[4], strlen(argv[4])) == 0){

			if ( argc != 6 ) {
				fprintf( output,
			"Wrong number of arguments specified for 'set %s'.\n",
					argv[4] );
				return FAILURE;
			}

			exposure_time = atof( argv[5] );

			mx_status = mx_area_detector_set_continuous_mode(
						    ad_record, exposure_time );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;
		} else
		if ( strncmp("multiframe_mode", argv[4], strlen(argv[4])) == 0)
		{
			if ( argc != 8 ) {
				fprintf( output,
			"Wrong number of arguments specified for 'set %s'.\n",
					argv[4] );
				return FAILURE;
			}

			num_frames = strtol( argv[5], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
	"%s: Non-numeric characters found in the number of frames '%s'\n",
					cname, argv[5] );

				return FAILURE;
			}

			exposure_time = atof( argv[6] );

			gap_time = atof( argv[7] );

			mx_status = mx_area_detector_set_multiframe_mode(
				ad_record, num_frames, exposure_time, gap_time);

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;
		} else
		if ( strncmp("circular_multiframe_mode",
				argv[4], strlen(argv[4])) == 0)
		{
			if ( argc != 8 ) {
				fprintf( output,
			"Wrong number of arguments specified for 'set %s'.\n",
					argv[4] );
				return FAILURE;
			}

			num_frames = strtol( argv[5], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
	"%s: Non-numeric characters found in the number of frames '%s'\n",
					cname, argv[5] );

				return FAILURE;
			}

			exposure_time = atof( argv[6] );

			gap_time = atof( argv[7] );

			mx_status =
			    mx_area_detector_set_circular_multiframe_mode(
				ad_record, num_frames, exposure_time, gap_time);

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;
		} else
		if ( strncmp("strobe_mode", argv[4], strlen(argv[4])) == 0)
		{
			if ( argc != 7 ) {
				fprintf( output,
			"Wrong number of arguments specified for 'set %s'.\n",
					argv[4] );
				return FAILURE;
			}

			num_frames = strtol( argv[5], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
	"%s: Non-numeric characters found in the number of frames '%s'\n",
					cname, argv[5] );

				return FAILURE;
			}

			exposure_time = atof( argv[6] );

			mx_status = mx_area_detector_set_strobe_mode(
				ad_record, num_frames, exposure_time );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;
		} else
		if ( strncmp("bulb_mode", argv[4], strlen(argv[4])) == 0)
		{
			if ( argc != 6 ) {
				fprintf( output,
			"Wrong number of arguments specified for 'set %s'.\n",
					argv[4] );
				return FAILURE;
			}

			num_frames = strtol( argv[5], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
	"%s: Non-numeric characters found in the number of frames '%s'\n",
					cname, argv[5] );

				return FAILURE;
			}

			mx_status = mx_area_detector_set_bulb_mode(
						ad_record, num_frames );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;
		} else
		if ( strncmp("geometrical_mode", argv[4], strlen(argv[4])) == 0)
		{
			if ( argc != 10 ) {
				fprintf( output,
			"Wrong number of arguments specified for 'set %s'.\n",
					argv[4] );
				return FAILURE;
			}

			num_frames = strtol( argv[5], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
	"%s: Non-numeric characters found in the number of frames '%s'\n",
					cname, argv[5] );

				return FAILURE;
			}

			exposure_time = atof( argv[6] );

			gap_time = atof( argv[7] );

			exposure_multiplier = atof( argv[8] );

			gap_multiplier = atof( argv[9] );

			mx_status = mx_area_detector_set_geometrical_mode(
				ad_record, num_frames, exposure_time, gap_time,
				exposure_multiplier, gap_multiplier );

			if ( mx_status.code != MXE_SUCCESS )
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

			roi[0] = strtoul( argv[6], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
		"%s: Non-numeric characters found in ROI Xmin value '%s'\n",
					cname, argv[5] );

				return FAILURE;
			}

			roi[1] = strtoul( argv[7], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
		"%s: Non-numeric characters found in ROI Xmax value '%s'\n",
					cname, argv[7] );

				return FAILURE;
			}

			roi[2] = strtoul( argv[8], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
		"%s: Non-numeric characters found in ROI Ymin value '%s'\n",
					cname, argv[8] );

				return FAILURE;
			}

			roi[3] = strtoul( argv[9], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
		"%s: Non-numeric characters found in ROI Ymax value '%s'\n",
					cname, argv[9] );

				return FAILURE;
			}

			mx_status = mx_area_detector_set_roi( ad_record,
						      roi_number, roi );

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

