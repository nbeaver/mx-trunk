/*
 * Name:    mvinput.c
 *
 * Purpose: Video input control functions.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2006-2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "motor.h"
#include "mx_key.h"
#include "mx_hrt.h"
#include "mx_image.h"
#include "mx_video_input.h"

int
motor_vinput_fn( int argc, char *argv[] )
{
	static const char cname[] = "vinput";

	MX_RECORD *vinput_record;
	MX_VIDEO_INPUT *vinput;
	MX_IMAGE_FRAME *frame;
	char *filename;
	unsigned long datafile_type;
	int status;
	double exposure_time;
	mx_bool_type busy;
	mx_status_type mx_status;

	static char usage[]
= "Usage:  vinput 'vinput_name' snap 'exposure_time' 'file_format' 'filename'\n"
	;

	if ( argc < 7 ) {
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

	vinput_record = mx_get_record( motor_record_list, argv[2] );

	if ( vinput_record == NULL ) {
		fprintf( output, "The record '%s' does not exist.\n", argv[2] );
		return FAILURE;
	}

	vinput = (MX_VIDEO_INPUT *) vinput_record->record_class_struct;

	if ( vinput == (MX_VIDEO_INPUT *) NULL ) {
		fprintf( output,
			"MX_VIDEO_INPUT pointer for record '%s' is NULL.\n",
				vinput_record->name );
		return FAILURE;
	}

	status = SUCCESS;

	if ( strncmp( "snap", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc < 7 ) {
			fprintf( output,
			"%s: not enough arguments to 'snap' command\n",
				cname );

			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		exposure_time = atof( argv[4] );

		if ( strcmp( argv[5], "pnm" ) == 0 ) {
			datafile_type = MXT_IMAGE_FILE_PNM;
		} else {
			fprintf( output,
				"%s: Unrecognized datafile type '%s'\n",
				cname, argv[5] );

			return FAILURE;
		}

		filename = argv[6];

		mx_status = mx_video_input_set_exposure_time( vinput_record,
								exposure_time );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = mx_video_input_start( vinput_record );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		for(;;) {
			mx_status = mx_video_input_is_busy(
							vinput_record, &busy );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			if ( mx_kbhit() ) {
				(void) mx_getch();

				(void) mx_video_input_stop( vinput_record );

				fprintf( output,
				"%s: The wait for video input '%s' "
				"to finish was interrupted.\n",
					cname, vinput_record->name );

				return FAILURE;
			}

			if ( busy == FALSE ) {
				break;		/* Exit the for(;;) loop. */
			}

			mx_msleep(10);
		}

		frame = NULL;

		mx_status = mx_video_input_get_frame( vinput_record,
							-1, &frame );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = mx_image_write_file( frame,
					datafile_type, filename );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		free( frame );
	} else {
		/* Unrecognized command. */

		fprintf( output, "Unrecognized option '%s'\n\n", argv[3] );
		fprintf( output, "%s\n", usage );
		return FAILURE;
	}

	return SUCCESS;
}

