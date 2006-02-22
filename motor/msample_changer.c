/*
 * Name:    msample_changer.c
 *
 * Purpose: MX sample changer control functions.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003-2004, 2006 Illinois Institute of Technology
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
#include "mx_sample_changer.h"
#include "mx_key.h"

static mx_status_type
motor_wait_for_sample_changer( MX_RECORD *changer_record, char *label )
{
	unsigned long busy, changer_status;
	mx_status_type mx_status;

	busy = 1;

	while( busy ) {
		if ( mx_kbhit() ) {
			(void) mx_getch();

			mx_status = mx_sample_changer_soft_abort(
							changer_record );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			fprintf( output, "%s\n", label );
		}

		mx_status = mx_sample_changer_get_status(changer_record,
							&changer_status);

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		busy = changer_status & MXSF_CHG_IS_BUSY;

		if ( busy ) {
			mx_msleep(500);
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

int
motor_sample_changer_fn( int argc, char *argv[] )
{
	static const char cname[] = "sample_changer";

	MX_RECORD *changer_record;
	MX_SAMPLE_CHANGER *changer;
	char *endptr;
	int status;
	long sample_id;
	mx_status_type mx_status;

	static char usage[]
	= "Usage:  changer 'changer_name' initialize\n"
	  "        changer 'changer_name' shutdown\n"
	  "        changer 'changer_name' select_sample_holder 'holder_name'\n"
	  "        changer 'changer_name' unselect_sample_holder\n"
	  "        changer 'changer_name' grab_sample 'sample_id'\n"
	  "        changer 'changer_name' ungrab_sample\n"
	  "        changer 'changer_name' mount_sample\n"
	  "        changer 'changer_name' unmount_sample\n"
	  "        changer 'changer_name' abort\n"
	  "        changer 'changer_name' estop\n"
	  "        changer 'changer_name' idle\n"
	  "        changer 'changer_name' reset\n"
	  "        changer 'changer_name' cooldown\n"
	  "        changer 'changer_name' deice\n"
	;

	if ( argc < 4 ) {
		fprintf( output, "%s\n", usage );
		return FAILURE;
	}

	changer_record = mx_get_record( motor_record_list, argv[2] );

	if ( changer_record == NULL ) {
		fprintf( output, "The record '%s' does not exist.\n", argv[2] );
		return FAILURE;
	}

	changer = (MX_SAMPLE_CHANGER *) changer_record->record_class_struct;

	if ( changer == (MX_SAMPLE_CHANGER *) NULL ) {
		fprintf( output,
			"MX_SAMPLE_CHANGER pointer for record '%s' is NULL.\n",
				changer_record->name );
		return FAILURE;
	}

	status = SUCCESS;

	if ( strncmp( "initialize", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc != 4 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		mx_status = mx_sample_changer_initialize( changer_record );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = motor_wait_for_sample_changer( changer_record,
				"Sample changer initialization aborted." );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

	} else
	if ( strncmp( "shutdown", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc != 4 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		mx_status = mx_sample_changer_shutdown( changer_record );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = motor_wait_for_sample_changer( changer_record,
				"Sample changer shutdown aborted." );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

	} else
	if ( strncmp( "mount_sample", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc != 4 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		mx_status = mx_sample_changer_mount_sample( changer_record );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = motor_wait_for_sample_changer( changer_record,
					"Sample mount aborted." );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

	} else
	if ( strncmp( "unmount_sample", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc != 4 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		mx_status = mx_sample_changer_unmount_sample( changer_record );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = motor_wait_for_sample_changer( changer_record,
					"Sample unmount aborted." );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

	} else
	if ( strncmp( "grab_sample", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc == 5 ) {
			sample_id = (long) strtoul( argv[4], &endptr, 0 );

			if ( *endptr != '\0' ) {
				fprintf( output,
		"%s: Non-numeric characters found in sample ID number '%s'\n",
					cname, argv[4] );

				return FAILURE;
			}

		} else {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		mx_status = mx_sample_changer_grab_sample( changer_record,
								sample_id );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = motor_wait_for_sample_changer( changer_record,
					"Sample get aborted." );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

	} else
	if ( strncmp( "ungrab_sample", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc != 4 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		mx_status = mx_sample_changer_ungrab_sample( changer_record );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = motor_wait_for_sample_changer( changer_record,
					"Sample replace aborted." );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

	} else
	if ( strncmp( "select_sample_holder", argv[3], strlen(argv[3]) ) == 0 ){

		if ( argc != 5 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		mx_status = mx_sample_changer_select_sample_holder(
						changer_record, argv[4] );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = motor_wait_for_sample_changer( changer_record,
					"Sample holder selection aborted." );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

	} else
	if ( strncmp( "unselect_sample_holder", argv[3], strlen(argv[3]) ) == 0)
	{

		if ( argc != 4 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		mx_status = mx_sample_changer_unselect_sample_holder(
						changer_record );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = motor_wait_for_sample_changer( changer_record,
					"Sample holder unselection aborted." );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

	} else
	if ( strncmp( "abort", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc != 4 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		mx_status = mx_sample_changer_soft_abort( changer_record );

		if ( mx_status.code != MXE_SUCCESS ) {
			return FAILURE;
		} else {
			return SUCCESS;
		}
	} else
	if ( strncmp( "estop", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc != 4 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		mx_status = mx_sample_changer_immediate_abort( changer_record );

		if ( mx_status.code != MXE_SUCCESS ) {
			return FAILURE;
		} else {
			return SUCCESS;
		}
	} else
	if ( strncmp( "idle", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc != 4 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		mx_status = mx_sample_changer_idle( changer_record );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = motor_wait_for_sample_changer( changer_record,
				"Sample changer idle aborted." );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

	} else
	if ( strncmp( "reset", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc != 4 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		mx_status = mx_sample_changer_reset( changer_record );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

	} else
	if ( strncmp( "cooldown", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc != 4 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		mx_status = mx_sample_changer_cooldown( changer_record );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = motor_wait_for_sample_changer( changer_record,
				"Sample changer cooldown aborted." );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

	} else
	if ( strncmp( "deice", argv[3], strlen(argv[3]) ) == 0 ) {

		if ( argc != 4 ) {
			fprintf( output, "%s\n", usage );
			return FAILURE;
		}

		mx_status = mx_sample_changer_deice( changer_record );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

		mx_status = motor_wait_for_sample_changer( changer_record,
			"Sample changer deice aborted." );

		if ( mx_status.code != MXE_SUCCESS )
			return FAILURE;

	} else {
		/* Unrecognized command. */

		fprintf( output, "Unrecognized option '%s'\n\n", argv[3] );
		fprintf( output, "%s\n", usage );
		return FAILURE;
	}
	return status;
}

