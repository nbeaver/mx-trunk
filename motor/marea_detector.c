/*
 * Name:    marea_detector.c
 *
 * Purpose: Video input control functions.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2006-2009, 2011-2013, 2015-2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MAREA_DETECTOR_DEBUG		FALSE

#define MAREA_DETECTOR_DEBUG_TIMING	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "motor.h"
#include "mx_key.h"
#include "mx_hrt.h"
#include "mx_dictionary.h"
#include "mx_image.h"
#include "mx_area_detector.h"

#if MAREA_DETECTOR_DEBUG_TIMING
#  include "mx_hrt_debug.h"
#endif

#define MAREA_BUFFER_OVERRUN_CHECK(s,r) \
	do {                                                                 \
	    if ( 0 & (s) & MXSF_AD_BUFFER_OVERRUN ) {                            \
                mx_warning("Buffer overrun detected for area detector '%s'", \
                        (r)->name );                                         \
	    }                                                                \
	} while (0)

int
motor_area_detector_fn( int argc, char *argv[] )
{
	static const char cname[] = "area_detector";

	static MX_IMAGE_FRAME *roi_frame = NULL;

	MX_RECORD *ad_record;
	MX_AREA_DETECTOR *ad;
	MX_RECORD *record_list, *motor_record, *shutter_record, *trigger_record;
	double oscillation_distance, shutter_time;
	MX_SEQUENCE_PARAMETERS sp;
	char *filename, *endptr;
	char *filename_stem;
	char filename_ext[20];
	char frame_filename[MXU_FILENAME_LENGTH+1];
	unsigned long datafile_type, correction_flags;
	unsigned long ad_status_mask, trigger_mask;
	long frame_number, measurement_number;
	long x_binsize, y_binsize, x_framesize, y_framesize;
	long trigger_mode, raw_trigger_mode;
	long bytes_per_frame, bits_per_pixel, num_frames;
	long frame_type, src_frame_type, dest_frame_type;
	long i, last_frame_number, total_num_frames;
	long n, starting_total_num_frames, starting_last_frame_number;
	long old_last_frame_number, old_total_num_frames, num_unread_frames;
	long num_frames_difference, num_new_frames;
	long num_lines, num_lines_per_subimage, num_subimages, testvar;
	int num_digits_in_filename;
	char digits_format[40];
	size_t length;
	unsigned long ad_status, roi_number, acquisition_in_progress;
	unsigned long roi[4];
	long register_value;
	long correction_type, num_measurements;
	unsigned long saved_correction_flags;
	double measurement_time, total_sequence_time;
	double exposure_time, frame_time, exposure_multiplier, gap_multiplier;
	double gate_time;
	double exposure_time_per_line, total_time_per_line, subimage_time;
	double bytes_per_pixel;
	mx_bool_type busy, ignore_num_frames;
	mx_status_type mx_status;

	static char usage[] =
"Usage:\n"
"  area_detector 'name' get bytes_per_frame\n"
"  area_detector 'name' get bytes_per_pixel\n"
"  area_detector 'name' get bits_per_pixel\n"
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
"  area_detector 'name' get register 'register_name'\n"
"  area_detector 'name' set register 'register_name' 'register_value'\n"
"\n"
"  area_detector 'name' get sequence_parameters\n"
"  area_detector 'name' set one_shot_mode 'exposure time in seconds'\n"
"  area_detector 'name' set stream_mode 'exposure time in seconds'\n"
"  area_detector 'name' set multiframe_mode '# frames'\n"
"                                                'exposure time' 'frame_time'\n"
"  area_detector 'name' set strobe_mode '# frames' 'exposure time'\n"
"  area_detector 'name' set duration_mode '# frames'\n"
"  area_detector 'name' set gated_mode '# frames' 'exposure time' 'gate_time'\n"
"  area_detector 'name' set geometrical_mode '# frames'\n"
"      'exposure time' 'frame_time' 'exposure multiplier' 'gap multiplier'\n"
"  area_detector 'name' set streak_camera_mode '# lines'\n"
"                             'exposure time per line' 'total time per line'\n"
"  area_detector 'name' set subimage_mode '# lines per subimage' '#subimages'\n"
"    'exposure time' 'subimage time' ['exposure multiplier' ['gap multiplier']]\n"
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
"  area_detector 'name' oscillation 'motor_name' 'shutter_name' 'trigger_name' \n"
"                             'oscillation_distance' 'exposure_time'\n"
"  area_detector 'name' arm\n"
"  area_detector 'name' trigger\n"
"  area_detector 'name' start\n"
"  area_detector 'name' stop\n"
"  area_detector 'name' abort\n"
"  area_detector 'name' get last_frame_number\n"
"  area_detector 'name' get total_num_frames\n"
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
"  area_detector 'name' copy frame 'src_frame_type' 'dest_frame_type'\n"
"\n"
"  area_detector 'name' measure dark_current 'measurement_time' '# measurements'\n"
"  area_detector 'name' measure flat_field 'measurement_time' '# measurements'\n";

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

	if ( strncmp( "test", argv[3], strlen(argv[3]) ) == 0 ) {

		/* This is a test command not intended for end users.
		 * The behavior of the test command may change between
		 * release of MX, so you should not count on the options
		 * to this command staying the same.
		 */

		if ( argc < 5 ) {
			return SUCCESS;
		}

		mx_status = MX_SUCCESSFUL_RESULT;

		if ( strcmp( argv[4], "nosave" ) == 0 ) {
			ad->datafile_management_handler = NULL;

			fprintf( stderr,
			"ad->datafile_management_handler = %p\n",
				ad->datafile_management_handler );
		}
			
		if ( mx_status.code != MXE_SUCCESS ) {
			return FAILURE;
		} else {
			return SUCCESS;
		}

	} else
	if ( strncmp( "snap", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc < 5 ) {
			fprintf( output,
			"%s: not enough arguments to 'snap' command\n",
				cname );

			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		exposure_time = atof( argv[4] );

		if ( argc < 6 ) {
			datafile_type = MXT_IMAGE_FILE_NONE;
		} else
		if ( strcmp( argv[5], "pnm" ) == 0 ) {
			datafile_type = MXT_IMAGE_FILE_PNM;
		} else
		if ( strcmp( argv[5], "tiff" ) == 0 ) {
			datafile_type = MXT_IMAGE_FILE_TIFF;
		} else
		if ( strcmp( argv[5], "marccd" ) == 0 ) {
			datafile_type = MXT_IMAGE_FILE_MARCCD;
		} else
		if ( strcmp( argv[5], "smv" ) == 0 ) {
			datafile_type = MXT_IMAGE_FILE_SMV;
		} else
		if ( strcmp( argv[5], "noir" ) == 0 ) {
			datafile_type = MXT_IMAGE_FILE_NOIR;
		} else
		if ( strcmp( argv[5], "raw" ) == 0 ) {
			datafile_type = MXT_IMAGE_FILE_RAW_GREY16;
		} else
		if ( strcmp( argv[5], "none" ) == 0 ) {
			datafile_type = MXT_IMAGE_FILE_NONE;
		} else {
			fprintf( output,
				"%s: Unsupported datafile type '%s'\n",
				cname, argv[5] );

			return FAILURE;
		}

		if ( datafile_type == MXT_IMAGE_FILE_NONE ) {
			filename = NULL;
		} else
		if ( argc < 7 ) {
			fprintf( output,
			"%s: not enough arguments to 'snap' command\n",
				cname );

			fprintf( output, "%s\n", usage );
			return FAILURE;
		} else {
			filename = argv[6];
		}

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
			mx_status = mx_area_detector_get_status(
							ad_record, &ad_status );

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

			if ( ( ad_status & MXSF_AD_IS_BUSY ) == 0 ) {
				break;		/* Exit the for(;;) loop. */
			}
			mx_msleep(10);
		}
#if MAREA_DETECTOR_DEBUG_TIMING
		MX_HRT_END( measurement2 );
#endif

		if ( ad_status & MXSF_AD_ERROR ) {
			fprintf( output,
		"The exposure for area detector '%s' aborted with an error.\n",
				ad_record->name );

			return FAILURE;
		}

		if ( datafile_type == MXT_IMAGE_FILE_NONE ) {
			/* If we do not actually want a file, then we
			 * return here.
			 */

			fprintf( output,
			"\nAcquisition complete.  No local copy saved.\n" );
			fflush ( output );
			return SUCCESS;
		}

		fprintf( output,
		"Exposure complete.\nNow transferring the frame.  " );

		fflush( output );

#if MAREA_DETECTOR_DEBUG_TIMING
		MX_HRT_START( measurement3 );
#endif
		mx_status = mx_area_detector_get_frame( ad_record,
						-1, &(ad->image_frame) );

#if MAREA_DETECTOR_DEBUG_TIMING
		MX_HRT_END( measurement3 );
#endif
		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		/* We are writing out a real file. */

		fprintf( output,
		"Transfer complete.\nNow writing image file '%s'.\n",
			filename );

		fflush( output );

#if MAREA_DETECTOR_DEBUG_TIMING
		MX_HRT_START( measurement4 );
#endif
		mx_status = mx_image_write_file( ad->image_frame,
						NULL, datafile_type, filename );
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

		if ( argc < 4 ) {
			fprintf( output,
			"%s: not enough arguments to 'sequence' command\n",
				cname );

			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		if ( argc < 5 ) {
			datafile_type = MXT_IMAGE_FILE_NONE;
		} else
		if ( strcmp( argv[4], "pnm" ) == 0 ) {
			datafile_type = MXT_IMAGE_FILE_PNM;
			strlcpy( filename_ext, "pnm", sizeof(filename_ext) );
		} else
		if ( strcmp( argv[4], "marccd" ) == 0 ) {
			datafile_type = MXT_IMAGE_FILE_MARCCD;
			strlcpy( filename_ext, "mar", sizeof(filename_ext) );
		} else
		if ( strcmp( argv[4], "tiff" ) == 0 ) {
			datafile_type = MXT_IMAGE_FILE_TIFF;
			strlcpy( filename_ext, "tiff", sizeof(filename_ext) );
		} else
		if ( strcmp( argv[4], "smv" ) == 0 ) {
			datafile_type = MXT_IMAGE_FILE_SMV;
			strlcpy( filename_ext, "smv", sizeof(filename_ext) );
		} else
		if ( strcmp( argv[4], "noir" ) == 0 ) {
			datafile_type = MXT_IMAGE_FILE_NOIR;
			strlcpy( filename_ext, "noir", sizeof(filename_ext) );
		} else
		if ( strcmp( argv[4], "raw" ) == 0 ) {
			datafile_type = MXT_IMAGE_FILE_RAW_GREY16;
			strlcpy( filename_ext, "raw", sizeof(filename_ext) );
		} else
		if ( strcmp( argv[4], "none" ) == 0 ) {
			datafile_type = MXT_IMAGE_FILE_NONE;
			strlcpy( filename_ext, "none", sizeof(filename_ext) );
		} else {
			fprintf( output,
				"%s: Unrecognized datafile type '%s'\n",
				cname, argv[4] );

			return FAILURE;
		}

		mx_status = mx_area_detector_get_sequence_parameters(
						ad_record, &sp );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		ignore_num_frames = FALSE;

		switch( sp.sequence_type ) {
		case MXT_SQ_ONE_SHOT:
		case MXT_SQ_STREAK_CAMERA:
		case MXT_SQ_SUBIMAGE:
			num_frames = 1;
			break;

		case MXT_SQ_MULTIFRAME:
		case MXT_SQ_STROBE:
		case MXT_SQ_DURATION:
		case MXT_SQ_GEOMETRICAL:
			num_frames = mx_round( sp.parameter_array[0] );
			break;

		case MXT_SQ_STREAM:
			ignore_num_frames = TRUE;
			num_frames = 0;
			break;

		case MXT_SQ_GATED:
			num_frames = mx_round( sp.parameter_array[0] );

			if ( num_frames <= 0 ) {
				ignore_num_frames = TRUE;
			}
			break;

		default:
			num_frames = 0;
			fprintf( output,
		"%s: Area detector '%s' is currently configured for\n"
		"unrecognized sequence type %ld.\n",
				cname, ad_record->name, sp.sequence_type );
			break;
		}

		filename_stem = argv[5];

		mx_status = mx_area_detector_get_trigger_mode( ad_record,
							&trigger_mode );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = mx_area_detector_get_extended_status( ad_record,
						&starting_last_frame_number,
						&starting_total_num_frames,
						&ad_status );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		MAREA_BUFFER_OVERRUN_CHECK(ad_status, ad_record);

#if MAREA_DETECTOR_DEBUG
		MX_DEBUG(-2,("%s: num_frames = %ld", cname, num_frames));

		MX_DEBUG(-2,("%s: starting_last_frame_number = %ld",
			cname, starting_last_frame_number));
		MX_DEBUG(-2,("%s: starting_total_num_frames = %ld",
			cname, starting_total_num_frames));
		MX_DEBUG(-2,("%s: ad_status = %#lx", cname, ad_status));
#endif

		if ( starting_total_num_frames > (LONG_MAX - num_frames) ) {
			fprintf( output,
	"%s: FIXME! The sum of the starting total number of frames (%ld)\n"
	"    and the number of frames in this sequence (%ld) is larger than\n"
	"    the largest long integer (%ld).  Support in 'motor' has not\n"
	"    been implemented for this case.  This should only happen if\n"
	"    the MX server has taken close to %ld image frames without\n"
	"    being restarted.  The simplest way of dealing with this\n"
	"    situation is to restart the MX server.  If you ever actually\n"
	"    see this error message, please tell Bill Lavender about it.\n",
			cname, starting_total_num_frames, num_frames,
			(long) LONG_MAX, (long) LONG_MAX );

			return FAILURE;
		}

		trigger_mask =
		    MXF_DEV_INTERNAL_TRIGGER | MXF_DEV_EXTERNAL_TRIGGER;

		if ( (trigger_mode & trigger_mask) == trigger_mask ) {
			fprintf( output,
		    "Starting sequence in internal/external trigger mode.\n" );
		} else
		if ( trigger_mode & MXF_DEV_INTERNAL_TRIGGER ) {
			fprintf( output,
			"Starting sequence in internal trigger mode.\n" );
		} else
		if ( trigger_mode & MXF_DEV_EXTERNAL_TRIGGER ) {
			fprintf( output,
			"Starting sequence in external trigger mode.\n" );
		} else {
			fprintf( output,
			"Illegal trigger mode %ld requested.  Aborting...\n",
				trigger_mode );

			return FAILURE;
		}

		fflush( output );

		mx_status = mx_area_detector_start( ad_record );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		/* Construct the format string for the image files.  We want
		 * all of the files to have the same number of digits in
		 * their names.  So first we find out how many digits are
		 * in the largest file number.
		 */

		num_digits_in_filename = 0;

		testvar = num_frames - 1;

		while(1) {
			if ( testvar == 0 ) {
				break;		/* Exit the while() loop. */
			}
			num_digits_in_filename++;
			testvar /= 10;
		}

#if MAREA_DETECTOR_DEBUG
		MX_DEBUG(-2,("%s: num_digits_in_filename = %d",
			cname, num_digits_in_filename));
#endif
		snprintf(digits_format, sizeof(digits_format),
			"%%s%%0%dld.%%s", num_digits_in_filename );

#if MAREA_DETECTOR_DEBUG
		MX_DEBUG(-2,("%s: digits_format = '%s'",
			cname, digits_format));
#endif
		/* Loop over the frames in the sequence.  Please note that
		 * the final section of the for() statement is empty, since
		 * 'n' does not get incremented on every pass through
		 * the loop.
		 */

		old_last_frame_number = -1L;
		old_total_num_frames  = starting_total_num_frames;

		num_unread_frames = 0;

		for ( n = 0; ; )
		{
			if ( mx_kbhit() ) {
				(void) mx_getch();

				(void) mx_area_detector_stop( ad_record );

				fprintf( output,
			    "%s: Area detector '%s' sequence interrupted.\n",
					cname, ad_record->name );

				return FAILURE;
			}

#if MAREA_DETECTOR_DEBUG
			MX_DEBUG(-2,("%s", mx_ctime_string() ));
			MX_DEBUG(-2,
			("Seq: n = %ld vvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvvv",n));
#endif

			mx_status = mx_area_detector_get_extended_status(
					ad_record,
					&last_frame_number,
					&total_num_frames,
					&ad_status );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			MAREA_BUFFER_OVERRUN_CHECK(ad_status, ad_record);

			if ( ad_status & MXSF_AD_ERROR ) {
				fprintf( output,
				"The sequence for area detector '%s' aborted "
				"with an error.\n", ad_record->name );

				return FAILURE;
			}

			num_frames_difference =
				total_num_frames - old_total_num_frames;

			num_unread_frames += num_frames_difference;

#if MAREA_DETECTOR_DEBUG
			MX_DEBUG(-2,
		    ("%s: old_total_num_frames = %lu, total_num_frames = %lu",
				cname, old_total_num_frames, total_num_frames));

			MX_DEBUG(-2,("%s: num_frames_difference = %ld",
				cname, num_frames_difference));

			MX_DEBUG(-2,("%s: num_unread_frames = %ld",
				cname, num_unread_frames));
#endif

			if ( datafile_type == MXT_IMAGE_FILE_NONE ) {
			    /* If there are any unread frames, let the
			     * user know that the next one has appeared.
			     */

			    if ( num_unread_frames > 0 ) {
				fprintf( output, "Frame %ld acquired.\n", n );

				/* Increment the frame number. */

				n++;
				num_unread_frames--;

				if ( ( sp.sequence_type != MXT_SQ_STREAM )
				  && ( n >= num_frames ) )
				{
					break;	/* Exit the for() loop. */
				}
			    }

			} else {
			    /* If there are any unread frames, write the
			     * next one in the sequence.
			     */

			    if ( num_unread_frames > 0 ) {

				fprintf( output, "Reading frame %ld.\n", n );

				if ( sp.sequence_type == MXT_SQ_STREAM ) {
					frame_number = 0;
				} else {
					frame_number = n;
				}

				mx_status = mx_area_detector_get_frame(
				    ad_record, frame_number,
				    &(ad->image_frame) );

				if ( mx_status.code != MXE_SUCCESS )
					return FAILURE;

				snprintf(frame_filename, sizeof(frame_filename),
					digits_format,
					filename_stem, n, filename_ext );

				fprintf( output, "Writing image file '%s'.  ",
					frame_filename );

				mx_status = mx_image_write_file(ad->image_frame,
					NULL, datafile_type, frame_filename );

				if ( mx_status.code != MXE_SUCCESS )
					return FAILURE;

				fprintf( output, "Complete.\n" );

				/* Increment the frame number. */

				n++;
				num_unread_frames--;

				if ( ( sp.sequence_type != MXT_SQ_STREAM )
				  && ( n >= num_frames ) )
				{
					break;	/* Exit the for() loop. */
				}
			    }
			}

			old_last_frame_number = last_frame_number;
			old_total_num_frames  = total_num_frames;

			acquisition_in_progress
				= ad_status & MXSF_AD_ACQUISITION_IN_PROGRESS;

			if ( (acquisition_in_progress == FALSE)
			  && (num_unread_frames <= 0) )
			{
				break;		/* Exit the for() loop. */
			}

#if MAREA_DETECTOR_DEBUG
			MX_DEBUG(-2,
			("Seq: n = %ld ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^",n));
#endif

			mx_msleep(10);
		}

		fprintf( output, "Sequence complete.\n" );
		fflush( output );

	} else
	if ( strncmp( "oscillation", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc != 9 ) {
			fprintf( output,
			"%s: not enough arguments to 'oscillation' command\n",
				cname );

			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		record_list = ad_record->list_head;

		motor_record = mx_get_record( record_list, argv[4] );

		if ( motor_record == NULL ) {
			if ( strlen(argv[4]) > 0 ) {
				fprintf( output,
				"Motor '%s' does not exist.\n", argv[4] );
				return FAILURE;
			}
		} else {
			if ( motor_record->mx_class != MXC_MOTOR ) {
				fprintf( output,
				"Record '%s' is not a motor.\n", argv[4] );
				return FAILURE;
			}
		}

		shutter_record = mx_get_record( record_list, argv[5] );

		if ( shutter_record == NULL ) {
			if ( strlen(argv[5]) > 0 ) {
				fprintf( output,
				"Shutter '%s' does not exist.\n", argv[5] );
				return FAILURE;
			}
		} else {
			switch( shutter_record->mx_class ) {
			case MXC_DIGITAL_OUTPUT:
			case MXC_RELAY:
				break;
			default:
				fprintf( output,
			    "Record '%s' is not a digital output or a relay.\n",
					argv[5] );
				return FAILURE;
			}
		}

		trigger_record = mx_get_record( record_list, argv[6] );

		if ( trigger_record == NULL ) {
			if ( strlen(argv[6]) > 0 ) {
				fprintf( output,
				"Motor '%s' does not exist.\n", argv[6] );
				return FAILURE;
			}
		} else {
			if ( trigger_record->mx_class != MXC_RELAY ) {
				fprintf( output,
				"Record '%s' is not a relay.\n", argv[6] );
				return FAILURE;
			}
		}

		oscillation_distance = atof( argv[7] );

		shutter_time = atof( argv[8] );

		mx_status = mx_area_detector_setup_oscillation( ad_record,
							motor_record,
							shutter_record,
							trigger_record,
							oscillation_distance,
							shutter_time );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = mx_area_detector_trigger_oscillation( ad_record );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = mx_area_detector_wait_for_oscillation_complete(
				ad_record, 10.0 * shutter_time );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = mx_area_detector_get_frame( ad_record,
						-1, &(ad->image_frame) );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

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
						-1, &(ad->image_frame) );

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
		} else
		if ( strcmp( argv[5], "marccd" ) == 0 ) {
			datafile_type = MXT_IMAGE_FILE_MARCCD;
		} else
		if ( strcmp( argv[5], "smv" ) == 0 ) {
			datafile_type = MXT_IMAGE_FILE_SMV;
		} else
		if ( strcmp( argv[5], "noir" ) == 0 ) {
			datafile_type = MXT_IMAGE_FILE_NOIR;
		} else
		if ( strcmp( argv[5], "raw" ) == 0 ) {
			datafile_type = MXT_IMAGE_FILE_RAW_GREY16;
		} else
		if ( strcmp( argv[5], "none" ) == 0 ) {
			datafile_type = MXT_IMAGE_FILE_NONE;
		} else {
			fprintf( output,
				"%s: Unrecognized datafile type '%s'\n",
				cname, argv[5] );

			return FAILURE;
		}

		filename = argv[6];

		if ( strncmp( "frame", argv[4], strlen(argv[4]) ) == 0 ) {

			if ( ad->image_frame == NULL ) {
				fprintf( output,
		"%s: no area detector image frame has been taken yet.\n",
					cname );

				return FAILURE;
			}

			mx_status = mx_image_write_file( ad->image_frame,
						NULL, datafile_type, filename );
		} else
		if ( strncmp( "roiframe", argv[4], strlen(argv[4]) ) == 0 ) {

			if ( roi_frame == NULL ) {
				fprintf( output,
		"%s: no area detector ROI frame has been taken yet.\n",
					cname );

				return FAILURE;
			}

			mx_status = mx_image_write_file( roi_frame,
						NULL, datafile_type, filename );
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

		mx_status = mx_area_detector_setup_frame( ad_record,
							&(ad->image_frame) );

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
					frame_type, &(ad->image_frame) );

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

		src_frame_type = strtol( argv[5], &endptr, 0 );

		if ( *endptr != '\0' ) {
			fprintf( output,
		"%s: Non-numeric characters found in source frame type '%s'\n",
				cname, argv[5] );

			return FAILURE;
		}

		dest_frame_type = strtol( argv[6], &endptr, 0 );

		if ( *endptr != '\0' ) {
			fprintf( output,
	"%s: Non-numeric characters found in destination frame type '%s'\n",
				cname, argv[6] );

			return FAILURE;
		}

		mx_status = mx_area_detector_copy_frame( ad_record,
					src_frame_type, dest_frame_type );

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
		if ( strncmp( "flat_field", argv[4], strlen(argv[4]) ) == 0 )
		{
			correction_type = MXFT_AD_FLAT_FIELD_FRAME;
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
		case MXFT_AD_FLAT_FIELD_FRAME:
			fprintf( output,
				"Starting flat field measurement:\n");
			break;
		}

		fprintf( output,
"  measurement time per frame = %g seconds, number of measurements = %ld\n",
			measurement_time, num_measurements );

		/* Make sure that automatic frame corrections are
		 * turned off.
		 */

		mx_status = mx_area_detector_get_correction_flags( ad_record,
						&saved_correction_flags );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = mx_area_detector_set_correction_flags(
						ad_record, 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = mx_area_detector_get_extended_status( ad_record,
							&last_frame_number,
							&total_num_frames,
							&ad_status );
		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		old_total_num_frames = total_num_frames;

		/* Now start the measurement. */

#if MAREA_DETECTOR_DEBUG
		MX_DEBUG(-2,
		("%s: Calling mx_area_detector_measure_correction_frame()  "
		"with measurement_time = %f, num_measurements = %ld",
			cname, measurement_time, num_measurements));
#endif

		mx_status = mx_area_detector_measure_correction_frame(
			ad_record, correction_type,
			measurement_time, num_measurements );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

#if MAREA_DETECTOR_DEBUG
		MX_DEBUG(-2,
		("%s: Waiting for the correction measurement to complete.",
			cname));
#endif

		ad_status_mask = MXSF_AD_ACQUISITION_IN_PROGRESS
				| MXSF_AD_CORRECTION_MEASUREMENT_IN_PROGRESS;

		measurement_number = 0;

		/* Wait for the correction measurement to complete. */

		for(;;) {
			mx_status = mx_area_detector_get_extended_status(
							ad_record,
							&last_frame_number,
							&total_num_frames,
							&ad_status );

			if ( mx_status.code != MXE_SUCCESS )  {
				fprintf( output,
				"%s: mx_area_detector_get_extended_status() "
				"returned MX error code %ld\n", 
					cname, mx_status.code );

				return FAILURE;
			}

#if MAREA_DETECTOR_DEBUG
			MX_DEBUG(-2,("%s: last_frame_number = %ld, "
				"total_num_frames = %ld, status = %ld.",
				cname, last_frame_number,
				total_num_frames, ad_status ));
#endif
			if ( total_num_frames != old_total_num_frames ) {
				num_new_frames = total_num_frames
							- old_total_num_frames;

				for ( i = 0; i < num_new_frames; i++ ) {
					fprintf( output,
						"Frame %ld acquired.\n",
						measurement_number );

					measurement_number++;
				}

				old_total_num_frames = total_num_frames;
			}

			if ( mx_kbhit() ) {
				(void) mx_getch();

				(void) mx_area_detector_stop( ad_record );

				fprintf( output,
				"%s: The wait for area detector '%s' "
				"to finish was interrupted.\n",
					cname, ad_record->name );

				return FAILURE;
			}

			if ( (ad_status & ad_status_mask) == 0 ) {
				break;		/* Exit the for(;;) loop. */
			}
			mx_msleep(10);
		}

#if 0
		mx_status = mx_area_detector_get_frame( ad_record,
						-1, &(ad->image_frame) );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;
#endif

		/* Restore the correction flags. */

		mx_status = mx_area_detector_set_correction_flags(
					ad_record, saved_correction_flags );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		switch( correction_type ) {
		case MXFT_AD_DARK_CURRENT_FRAME:
			fprintf( output,
			  "Dark current measurement completed successfully.\n");
			break;
		case MXFT_AD_FLAT_FIELD_FRAME:
			fprintf( output,
			  "Flat field measurement completed successfully.\n");
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
					frame_number, &(ad->image_frame) );

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
		if ( strncmp( "bits_per_pixel",
				argv[4], strlen(argv[4]) ) == 0 )
		{
			mx_status = mx_area_detector_get_bits_per_pixel(
						ad_record, &bits_per_pixel );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output,
			"Area detector '%s': bits per pixel = %ld\n",
				ad_record->name, bits_per_pixel );
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
		if ( strncmp( "total_num_frames",
					argv[4], strlen(argv[4]) ) == 0 )
		{
			mx_status = mx_area_detector_get_total_num_frames(
						ad_record, &total_num_frames );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output,
			"Area detector '%s': total num frames = %ld\n",
					ad_record->name, total_num_frames );
		} else
		if ( strncmp( "status", argv[4], strlen(argv[4]) ) == 0 ) {
			mx_status = mx_area_detector_get_status(
						ad_record, &ad_status );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output, "Area detector '%s': status = %#lx\n",
						ad_record->name, ad_status );

			MAREA_BUFFER_OVERRUN_CHECK(ad_status, ad_record);
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

			MAREA_BUFFER_OVERRUN_CHECK(ad_status, ad_record);
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

			trigger_mask =
		    MXF_DEV_INTERNAL_TRIGGER | MXF_DEV_EXTERNAL_TRIGGER;

			if ( (trigger_mode & trigger_mask) == trigger_mask ) {
				fprintf( output,
		"Area detector '%s': trigger mode = internal/external (%#lx)\n",
				ad_record->name, trigger_mode );
			} else
			if( trigger_mode & MXF_DEV_INTERNAL_TRIGGER ) {
				fprintf( output,
			"Area detector '%s': trigger mode = internal (%#lx)\n",
				ad_record->name, trigger_mode );
			} else
			if ( trigger_mode & MXF_DEV_EXTERNAL_TRIGGER ) {
				fprintf( output,
			"Area detector '%s': trigger mode = external (%#lx)\n",
				ad_record->name, trigger_mode );
			} else {
				fprintf( output,
			"Area detector '%s': trigger mode = ILLEGAL (%#lx)\n",
				ad_record->name, trigger_mode );
			}
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
		if ( strncmp( "register",
					argv[4], strlen(argv[4]) ) == 0 )
		{
			if ( argc != 6 ) {
				fprintf( output,
	    "%s: wrong number of arguments to 'get register' command\n",
					cname );

				fprintf( output, "%s\n", usage );
				return FAILURE;
			}

			mx_status = mx_area_detector_get_register(
					ad_record, argv[5], &register_value );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output,
			"Area detector '%s': register '%s' long value = %ld\n",
				ad_record->name, argv[5], register_value );
		} else
		if ( strncmp( "sequence_parameters",
					argv[4], strlen(argv[4]) ) == 0 )
		{
			mx_status = mx_area_detector_get_sequence_parameters(
						ad_record, &sp );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output,
	"Area detector '%s': sequence type = %ld, num parameters = %ld\n"
	"    parameter array =", ad_record->name,
			sp.sequence_type, sp.num_parameters);

			for ( i = 0; i < sp.num_parameters; i++ ) {
				fprintf( output, " %g",
					sp.parameter_array[i] );
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
						ad_record, ad->image_frame,
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

			x_binsize = strtol( argv[5], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
		"%s: Non-numeric characters found in X binsize '%s'\n",
					cname, argv[5] );

				return FAILURE;
			}

			y_binsize = strtol( argv[6], &endptr, 0 );

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

			x_framesize = strtol( argv[5], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
		"%s: Non-numeric characters found in X framesize '%s'\n",
					cname, argv[5] );

				return FAILURE;
			}

			y_framesize = strtol( argv[6], &endptr, 0 );

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

			/* Since the framesize will be rounded to the nearest
			 * matching binsize, we must now read the framesize
			 * to find out what framesize was actually chosen.
			 */

			mx_status = mx_area_detector_get_framesize( ad_record,
								NULL, NULL );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output,
		      "new binsize = (%ld, %ld), new framesize = (%ld, %ld)\n",
				ad->binsize[0], ad->binsize[1],
				ad->framesize[0], ad->framesize[1] );
		} else
		if ( strncmp( "trigger_mode", argv[4], strlen(argv[4]) ) == 0) {

			unsigned long mask;
			
			if ( argc != 6 ) {
				fprintf( output,
			"Wrong number of arguments specified for 'set %s'.\n",
					argv[4] );
				return FAILURE;
			}

			mx_status = mx_area_detector_get_trigger_mode(
						ad_record, &raw_trigger_mode );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			mask = MXF_DEV_INTERNAL_TRIGGER
				| MXF_DEV_EXTERNAL_TRIGGER;

			trigger_mode = raw_trigger_mode & (~mask);

			length = strlen(argv[5]);

			if (mx_strncasecmp( argv[5], "internal", length ) == 0)
			{
				trigger_mode |= MXF_DEV_INTERNAL_TRIGGER;
			} else
			if (mx_strncasecmp( argv[5], "external", length ) == 0)
			{
				trigger_mode |= MXF_DEV_EXTERNAL_TRIGGER;
			} else {
				trigger_mode = strtol( argv[5], &endptr, 0 );

				if ( *endptr != '\0' ) {
					fprintf( output,
		"%s: Non-numeric characters found in trigger mode '%s'\n",
						cname, argv[5] );

					return FAILURE;
				}
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
		if ( strncmp( "register",
					argv[4], strlen(argv[4]) ) == 0 )
		{
			if ( argc != 7 ) {
				fprintf( output,
	    "%s: wrong number of arguments to 'set register' command\n",
					cname );

				fprintf( output, "%s\n", usage );
				return FAILURE;
			}

			register_value = strtol( argv[6], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
	"%s: Non-numeric characters found in the register value '%s'\n",
					cname, argv[6] );

				return FAILURE;
			}

			mx_status = mx_area_detector_set_register(
					ad_record, argv[5], register_value );

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

			mx_status = mx_area_detector_get_total_sequence_time(
					ad_record, &total_sequence_time );

			fprintf( output,
			"The sequence will take %g seconds.\n",
				total_sequence_time );
		} else
		if ( strncmp("stream_mode", argv[4], strlen(argv[4])) == 0){

			if ( argc != 6 ) {
				fprintf( output,
			"Wrong number of arguments specified for 'set %s'.\n",
					argv[4] );
				return FAILURE;
			}

			exposure_time = atof( argv[5] );

			mx_status = mx_area_detector_set_stream_mode(
						    ad_record, exposure_time );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			mx_status = mx_area_detector_get_total_sequence_time(
					ad_record, &total_sequence_time );

			fprintf( output,
			"The sequence will take %g seconds.\n",
				total_sequence_time );
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

			frame_time = atof( argv[7] );

			mx_status = mx_area_detector_set_multiframe_mode(
			    ad_record, num_frames, exposure_time, frame_time );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			mx_status = mx_area_detector_get_total_sequence_time(
					ad_record, &total_sequence_time );

			fprintf( output,
			"The sequence will take %g seconds.\n",
				total_sequence_time );
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

			mx_status = mx_area_detector_get_total_sequence_time(
					ad_record, &total_sequence_time );

			fprintf( output,
			"The sequence will take %g seconds.\n",
				total_sequence_time );
		} else
		if ( strncmp("duration_mode", argv[4], strlen(argv[4])) == 0)
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

			mx_status = mx_area_detector_set_duration_mode(
						ad_record, num_frames );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			mx_status = mx_area_detector_get_total_sequence_time(
					ad_record, &total_sequence_time );

			fprintf( output,
			"The sequence will take %g seconds.\n",
				total_sequence_time );
		} else
		if ( strncmp("gated_mode", argv[4], strlen(argv[4])) == 0)
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

			gate_time = atof( argv[7] );

			mx_status = mx_area_detector_set_gated_mode(
				ad_record, num_frames, exposure_time,
				gate_time );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			mx_status = mx_area_detector_get_total_sequence_time(
					ad_record, &total_sequence_time );

			fprintf( output,
			"The sequence will take %g seconds.\n",
				total_sequence_time );
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

			frame_time = atof( argv[7] );

			exposure_multiplier = atof( argv[8] );

			gap_multiplier = atof( argv[9] );

			mx_status = mx_area_detector_set_geometrical_mode(
				ad_record, num_frames,
				exposure_time, frame_time,
				exposure_multiplier, gap_multiplier );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			mx_status = mx_area_detector_get_total_sequence_time(
					ad_record, &total_sequence_time );

			fprintf( output,
			"The sequence will take %g seconds.\n",
				total_sequence_time );
		} else
		if ( strncmp("streak_camera_mode",
				argv[4], strlen(argv[4])) == 0)
		{
			if ( argc != 8 ) {
				fprintf( output,
			"Wrong number of arguments specified for 'set %s'.\n",
					argv[4] );
				return FAILURE;
			}

			num_lines = strtol( argv[5], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
	"%s: Non-numeric characters found in the number of lines '%s'\n",
					cname, argv[5] );

				return FAILURE;
			}

			exposure_time_per_line = atof( argv[6] );

			total_time_per_line = atof( argv[7] );

			mx_status = mx_area_detector_set_streak_camera_mode(
				ad_record, num_lines, 
				exposure_time_per_line,
				total_time_per_line );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			mx_status = mx_area_detector_get_total_sequence_time(
					ad_record, &total_sequence_time );

			fprintf( output,
			"The sequence will take %g seconds.\n",
				total_sequence_time );
		} else
		if ( strncmp("subimage_mode", argv[4], strlen(argv[4])) == 0)
		{
			if ( argc < 9 ) {
				fprintf( output,
			"Wrong number of arguments specified for 'set %s'.\n",
					argv[4] );
				return FAILURE;
			}

			num_lines_per_subimage = strtol( argv[5], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
"%s: Non-numeric characters found in the number of lines per subimage '%s'\n",
					cname, argv[5] );

				return FAILURE;
			}

			num_subimages = strtol( argv[6], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
	"%s: Non-numeric characters found in the number of subimages '%s'\n",
					cname, argv[6] );

				return FAILURE;
			}

			exposure_time = atof( argv[7] );

			subimage_time = atof( argv[8] );

			if ( argc < 10 ) {
				exposure_multiplier = 1.0;
			} else {
				exposure_multiplier = atof( argv[9] );
			}

			if ( argc < 11 ) {
				gap_multiplier = 1.0;
			} else {
				gap_multiplier = atof( argv[10] );
			}

			mx_status = mx_area_detector_set_subimage_mode(
				ad_record, num_lines_per_subimage,
				num_subimages, exposure_time, subimage_time,
				exposure_multiplier, gap_multiplier );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			mx_status = mx_area_detector_get_total_sequence_time(
					ad_record, &total_sequence_time );

			fprintf( output,
			"The sequence will take %g seconds.\n",
				total_sequence_time );
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
	} else
	if ( strncmp( "show", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc <= 4 ) {
			fprintf( output,
			"%s: wrong number of arguments to 'show' command\n",
				cname );

			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		if ( strncmp( "frame", argv[4], strlen(argv[4]) ) == 0 ) {
			if ( argc == 5 ) {
				frame_number = -1;
			} else {
				frame_number = atol( argv[5] );
			}

			mx_status = mx_area_detector_get_frame( ad_record,
						-1, &(ad->image_frame) );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			mx_status = mx_image_display_ascii( output,
							ad->image_frame,
							0, 65535 );
		} else {
			fprintf( output,
				"%s: unknown show command argument '%s'\n",
				cname, argv[4] );

			return FAILURE;
		}

	} else {
		/* Unrecognized command. */

		fprintf( output, "Unrecognized option '%s'\n\n", argv[3] );
		fprintf( output, "%s\n", usage );
		return FAILURE;
	}

	MXW_UNUSED( ignore_num_frames );
	MXW_UNUSED( old_last_frame_number );

	return SUCCESS;
}

