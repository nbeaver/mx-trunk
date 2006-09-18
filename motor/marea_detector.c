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

	MX_RECORD *ad_record;
	MX_AREA_DETECTOR *ad;
	MX_IMAGE_FRAME *frame;
	char *filename;
	unsigned long datafile_type;
	double exposure_time;
	mx_bool_type busy;
	mx_status_type mx_status;

	static char usage[]
= "Usage:  area_detector 'ad_name' snap 'exposure_time' 'format' 'filename'\n";

#if MAREA_DETECTOR_DEBUG_TIMING
	MX_HRT_TIMING measurement1, measurement2;
#endif

	if ( argc < 5 ) {
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

		MX_HRT_RESULTS( measurement1, cname, "for Start" );
		MX_HRT_RESULTS( measurement2, cname, "for Busy/Wait" );
#endif

		frame = NULL;

		mx_status = mx_area_detector_get_frame( ad_record,
							-1, &frame );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = mx_write_image_file( frame,
						datafile_type, NULL,
						filename );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		if ( frame->header_data != NULL ) {
			free( frame->header_data );
		}
		if ( frame->image_data != NULL ) {
			free( frame->image_data );
		}
		free( frame );
	} else {
		/* Unrecognized command. */

		fprintf( output, "Unrecognized option '%s'\n\n", argv[3] );
		fprintf( output, "%s\n", usage );
		return FAILURE;
	}

	return SUCCESS;
}

