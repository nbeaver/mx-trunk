/*
 * Name:    d_bkprecision_912x_timer.c
 *
 * Purpose: MX timer driver for the BK Precision 912x series
 *          of power supplies.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_BKPRECISION_912X_TIMER_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mx_util.h"
#include "mx_measurement.h"
#include "mx_timer.h"
#include "i_bkprecision_912x.h"
#include "d_bkprecision_912x_timer.h"

/* Initialize the timer driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_bkprecision_912x_timer_record_function_list = {
	NULL,
	mxd_bkprecision_912x_timer_create_record_structures,
	mx_timer_finish_record_initialization
};

MX_TIMER_FUNCTION_LIST mxd_bkprecision_912x_timer_timer_function_list = {
	mxd_bkprecision_912x_timer_is_busy,
	mxd_bkprecision_912x_timer_start,
	mxd_bkprecision_912x_timer_stop
};

/* MX spec timer data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_bkprecision_912x_timer_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_TIMER_STANDARD_FIELDS,
	MXD_BKPRECISION_912X_TIMER_STANDARD_FIELDS
};

long mxd_bkprecision_912x_timer_num_record_fields
		= sizeof( mxd_bkprecision_912x_timer_record_field_defaults )
		/ sizeof( mxd_bkprecision_912x_timer_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_bkprecision_912x_timer_rfield_def_ptr
			= &mxd_bkprecision_912x_timer_record_field_defaults[0];

/*=======================================================================*/

static mx_status_type
mxd_bkprecision_912x_timer_get_pointers( MX_TIMER *timer,
			MX_BKPRECISION_912X_TIMER **bkprecision_912x_timer,
			MX_BKPRECISION_912X **bkprecision_912x,
			const char *calling_fname )
{
	static const char fname[] = "mxd_bkprecision_912x_timer_get_pointers()";

	MX_RECORD *bkprecision_912x_record;
	MX_BKPRECISION_912X_TIMER *bkprecision_912x_timer_ptr;

	if ( timer == (MX_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_TIMER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	bkprecision_912x_timer_ptr = (MX_BKPRECISION_912X_TIMER *)
					timer->record->record_type_struct;

	if (bkprecision_912x_timer_ptr == (MX_BKPRECISION_912X_TIMER *) NULL)
	{
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_BKPRECISION_912X_TIMER pointer for record '%s' passed "
		"by '%s' is NULL.",
			timer->record->name, calling_fname );
	}

	if ( bkprecision_912x_timer != (MX_BKPRECISION_912X_TIMER **) NULL ) {
		*bkprecision_912x_timer = bkprecision_912x_timer_ptr;
	}

	if ( bkprecision_912x != (MX_BKPRECISION_912X **) NULL ) {
		bkprecision_912x_record =
			bkprecision_912x_timer_ptr->bkprecision_912x_record;

		if ( bkprecision_912x_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_BKPRECISION_912X pointer for timer "
			"record '%s' passed by '%s' is NULL.",
				timer->record->name, calling_fname );
		}

		*bkprecision_912x = (MX_BKPRECISION_912X *)
			bkprecision_912x_record->record_type_struct;

		if ( *bkprecision_912x == (MX_BKPRECISION_912X *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_BKPRECISION_912X pointer for BK Precision "
			"power supply '%s' used by timer record '%s'.",
				bkprecision_912x_record->name,
				timer->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_bkprecision_912x_timer_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_bkprecision_912x_timer_create_record_structures()";

	MX_TIMER *timer;
	MX_BKPRECISION_912X_TIMER *bkprecision_912x_timer;

	/* Allocate memory for the necessary structures. */

	timer = (MX_TIMER *) malloc( sizeof(MX_TIMER) );

	if ( timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannott allocate memory for MX_TIMER structure." );
	}

	bkprecision_912x_timer = (MX_BKPRECISION_912X_TIMER *)
				malloc( sizeof(MX_BKPRECISION_912X_TIMER) );

	if ( bkprecision_912x_timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Cannot allocate memory for MX_BKPRECISION_912X_TIMER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = timer;
	record->record_type_struct = bkprecision_912x_timer;
	record->class_specific_function_list
			= &mxd_bkprecision_912x_timer_timer_function_list;

	timer->record = record;

	timer->mode = MXCM_PRESET_MODE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bkprecision_912x_timer_is_busy( MX_TIMER *timer )
{
	static const char fname[] = "mxd_bkprecision_912x_timer_is_busy()";

	MX_BKPRECISION_912X_TIMER *bkprecision_912x_timer;
	MX_BKPRECISION_912X *bkprecision_912x;
	char command[40];
	char response[80];
	long busy;
	int num_items;
	mx_status_type mx_status;

	mx_status = mxd_bkprecision_912x_timer_get_pointers( timer,
			&bkprecision_912x_timer, &bkprecision_912x, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BKPRECISION_912X_TIMER_DEBUG
	MX_DEBUG(-2,("%s invoked for timer '%s'", fname, timer->record->name));
#endif

	strlcpy( command, "OUTPUT:TIMER?", sizeof(command) );

	mx_status = mxi_bkprecision_912x_command( bkprecision_912x,
					command, response, sizeof(response),
					MXD_BKPRECISION_912X_TIMER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%ld", &busy );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"No time value was found in the response '%s' to command '%s' "
		"sent to BK Precision timer '%s'.",
			response, command, timer->record->name );
	}

	if ( busy ) {
		timer->busy = TRUE;
	} else {
		timer->busy = FALSE;
	}

#if MXD_BKPRECISION_912X_TIMER_DEBUG
	MX_DEBUG(-2,("%s: timer->busy = %d", fname, timer->busy ));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bkprecision_912x_timer_start( MX_TIMER *timer )
{
	static const char fname[] = "mxd_bkprecision_912x_timer_start()";

	MX_BKPRECISION_912X_TIMER *bkprecision_912x_timer;
	MX_BKPRECISION_912X *bkprecision_912x;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_bkprecision_912x_timer_get_pointers( timer,
			&bkprecision_912x_timer, &bkprecision_912x, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BKPRECISION_912X_TIMER_DEBUG
	MX_DEBUG(-2,("%s invoked for timer '%s'", fname, timer->record->name));
#endif
	/* Make sure that we are in FIXED mode and not in list mode. */

	mx_status = mxi_bkprecision_912x_command( bkprecision_912x,
					"MODE FIXED", NULL, 0,
					MXD_BKPRECISION_912X_TIMER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Output timer times are specified as an integer number of seconds. */

	snprintf( command, sizeof(command), "OUTPUT:TIMER:DATA %lu",
		mx_round( timer->value ) );

	mx_status = mxi_bkprecision_912x_command( bkprecision_912x,
					command, NULL, 0,
					MXD_BKPRECISION_912X_TIMER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The output must be on for the output timer to run. */

	mx_status = mxi_bkprecision_912x_command( bkprecision_912x,
					"OUTPUT ON", NULL, 0,
					MXD_BKPRECISION_912X_TIMER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The 'OUTPUT:TIMER ON' command starts the timer. */

	mx_status = mxi_bkprecision_912x_command( bkprecision_912x,
					"OUTPUT:TIMER ON", NULL, 0,
					MXD_BKPRECISION_912X_TIMER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	timer->busy = TRUE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bkprecision_912x_timer_stop( MX_TIMER *timer )
{
	static const char fname[] = "mxd_bkprecision_912x_timer_stop()";

	MX_BKPRECISION_912X_TIMER *bkprecision_912x_timer;
	MX_BKPRECISION_912X *bkprecision_912x;
	mx_status_type mx_status;

	mx_status = mxd_bkprecision_912x_timer_get_pointers( timer,
			&bkprecision_912x_timer, &bkprecision_912x, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BKPRECISION_912X_TIMER_DEBUG
	MX_DEBUG(-2,("%s invoked for timer '%s'", fname, timer->record->name));
#endif
	mx_status = mxi_bkprecision_912x_command( bkprecision_912x,
					"OUTPUT:TIMER OFF", NULL, 0,
					MXD_BKPRECISION_912X_TIMER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_bkprecision_912x_command( bkprecision_912x,
					"OUTPUT OFF", NULL, 0,
					MXD_BKPRECISION_912X_TIMER_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	timer->busy = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

