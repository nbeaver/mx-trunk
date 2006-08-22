/*
 * Name:    mvinput.c
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

#include <stdio.h>
#include <stdlib.h>

#include "motor.h"
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
	int status;
	mx_status_type mx_status;

	static char usage[]
	= "Usage:  vinput 'vinput_name' snap 'filename'\n"
	;

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

		if ( argc < 5 ) {
			fprintf( output,
			"%s: not enough arguments to 'snap' command\n",
				cname );

			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		filename = argv[4];

		mx_status = mx_video_input_start( vinput_record );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		frame = NULL;

		mx_status = mx_video_input_get_frame( vinput_record,
							&frame );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = mx_write_image_file( frame,
						MX_IMAGE_FILE_PNM, NULL,
						filename );

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

