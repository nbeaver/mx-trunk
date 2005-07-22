/*
 * Name:    msca.c
 *
 * Purpose: SCA control functions.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2002 Illinois Institute of Technology
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
#include "mx_sca.h"

int
motor_sca_fn( int argc, char *argv[] )
{
	const char cname[] = "sca";

	MX_RECORD *sca_record;
	MX_SCA *sca;
	int status, mode;
	double lower_level, upper_level, gain, time_constant;
	mx_status_type mx_status;

	static char usage[]
	= "Usage:  sca 'sca_name' get lower_level\n"
	  "        sca 'sca_name' get upper_level\n"
	  "        sca 'sca_name' get gain\n"
	  "        sca 'sca_name' get time_constant\n"
	  "        sca 'sca_name' get mode\n"
	  "\n"
	  "        sca 'sca_name' set lower_level 'value'\n"
	  "        sca 'sca_name' set upper_level 'value'\n"
	  "        sca 'sca_name' set gain 'value'\n"
	  "        sca 'sca_name' set time_constant 'value'\n"
	  "        sca 'sca_name' set mode 'value'\n"
	;

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

	sca_record = mx_get_record( motor_record_list, argv[2] );

	if ( sca_record == NULL ) {
		fprintf( output, "The record '%s' does not exist.\n", argv[2] );
		return FAILURE;
	}

	sca = (MX_SCA *) sca_record->record_class_struct;

	if ( sca == (MX_SCA *) NULL ) {
		fprintf( output, "MX_SCA pointer for record '%s' is NULL.\n",
				sca_record->name );
		return FAILURE;
	}

	status = SUCCESS;

	if ( strncmp( "get", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc < 5 ) {
			fprintf( output,
			"%s: not enough arguments to 'get' command\n",
				cname );

			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		if ( strncmp( "lower_level", argv[4], strlen(argv[4]) ) == 0 ) {

			if ( argc != 5 ) {
				fprintf( output,
		"%s: wrong number of arguments to 'get lower_level' command\n",
					cname );

				fprintf( output, "%s\n", usage );
				return FAILURE;
			}

			mx_status = mx_sca_get_lower_level( sca_record,
								&lower_level );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output,
				"SCA '%s' lower_level = %g\n",
				sca_record->name, lower_level );
		} else
		if ( strncmp( "upper_level", argv[4], strlen(argv[4]) ) == 0 ) {

			if ( argc != 5 ) {
				fprintf( output,
		"%s: wrong number of arguments to 'get upper_level' command\n",
					cname );

				fprintf( output, "%s\n", usage );
				return FAILURE;
			}

			mx_status = mx_sca_get_upper_level( sca_record,
								&upper_level );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output,
				"SCA '%s' upper_level = %g\n",
				sca_record->name, upper_level );
		} else
		if ( strncmp( "gain", argv[4], strlen(argv[4]) ) == 0 ) {

			if ( argc != 5 ) {
				fprintf( output,
		"%s: wrong number of arguments to 'get gain' command\n",
					cname );

				fprintf( output, "%s\n", usage );
				return FAILURE;
			}

			mx_status = mx_sca_get_gain( sca_record, &gain );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output,
				"SCA '%s' gain = %g\n",
				sca_record->name, gain );
		} else
		if ( strncmp( "time_constant", argv[4], strlen(argv[4]) ) == 0 )
		{

			if ( argc != 5 ) {
				fprintf( output,
	"%s: wrong number of arguments to 'get time_constant' command\n",
					cname );

				fprintf( output, "%s\n", usage );
				return FAILURE;
			}

			mx_status = mx_sca_get_time_constant( sca_record,
							&time_constant );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output,
				"SCA '%s' time_constant = %g\n",
				sca_record->name, time_constant );

		} else
		if ( strncmp( "mode", argv[4], strlen(argv[4]) ) == 0 ) {

			if ( argc != 5 ) {
				fprintf( output,
	"%s: wrong number of arguments to 'get mode' command\n",
					cname );

				fprintf( output, "%s\n", usage );
				return FAILURE;
			}

			mx_status = mx_sca_get_mode( sca_record, &mode );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

			fprintf( output,
				"SCA '%s' mode = %d\n",
				sca_record->name, mode );

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

		if ( strncmp( "lower_level", argv[4], strlen(argv[4]) ) == 0 ) {

			if ( argc != 6 ) {
				fprintf( output,
		"%s: wrong number of arguments to 'set lower_level' command\n",
					cname );

				fprintf( output, "%s\n", usage );
				return FAILURE;
			}

			lower_level = atof( argv[5] );

			mx_status = mx_sca_set_lower_level( sca_record,
								lower_level );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;
		} else
		if ( strncmp( "upper_level", argv[4], strlen(argv[4]) ) == 0 ) {

			if ( argc != 6 ) {
				fprintf( output,
		"%s: wrong number of arguments to 'set upper_level' command\n",
					cname );

				fprintf( output, "%s\n", usage );
				return FAILURE;
			}

			upper_level = atof( argv[5] );

			mx_status = mx_sca_set_upper_level( sca_record,
								upper_level );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;
		} else
		if ( strncmp( "gain", argv[4], strlen(argv[4]) ) == 0 ) {

			if ( argc != 6 ) {
				fprintf( output,
		"%s: wrong number of arguments to 'set gain' command\n",
					cname );

				fprintf( output, "%s\n", usage );
				return FAILURE;
			}

			gain = atof( argv[5] );

			mx_status = mx_sca_set_gain( sca_record, gain );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;
		} else
		if ( strncmp( "time_constant", argv[4], strlen(argv[4]) ) == 0 )
		{

			if ( argc != 6 ) {
				fprintf( output,
	"%s: wrong number of arguments to 'set time_constant' command\n",
					cname );

				fprintf( output, "%s\n", usage );
				return FAILURE;
			}

			time_constant = atof( argv[5] );

			mx_status = mx_sca_set_time_constant( sca_record,
								time_constant );

			if ( mx_status.code != MXE_SUCCESS )
				return FAILURE;

		} else
		if ( strncmp( "mode", argv[4], strlen(argv[4]) ) == 0 ) {

			if ( argc != 6 ) {
				fprintf( output,
	"%s: wrong number of arguments to 'set mode' command\n",
					cname );

				fprintf( output, "%s\n", usage );
				return FAILURE;
			}

			mode = atoi( argv[5] );

			mx_status = mx_sca_set_mode( sca_record, mode );

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

