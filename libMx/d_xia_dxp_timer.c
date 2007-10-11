/*
 * Name:    d_xia_dxp_timer.c
 *
 * Purpose: MX timer driver to control XIA DXP MCA timers.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2006-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mxconfig.h"

#include "mx_util.h"
#include "mx_unistd.h"
#include "mx_driver.h"
#include "mx_net.h"
#include "mx_measurement.h"
#include "mx_timer.h"
#include "mx_mca.h"
#include "d_xia_dxp_timer.h"

#include "i_xia_network.h"

#if HAVE_XIA_HANDEL
#include "i_xia_handel.h"
#endif

#if HAVE_XIA_XERXES
#include "i_xia_xerxes.h"
#endif

/* Initialize the timer driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_xia_dxp_timer_record_function_list = {
	NULL,
	mxd_xia_dxp_timer_create_record_structures,
	mx_timer_finish_record_initialization
};

MX_TIMER_FUNCTION_LIST mxd_xia_dxp_timer_timer_function_list = {
	mxd_xia_dxp_timer_is_busy,
	mxd_xia_dxp_timer_start,
	mxd_xia_dxp_timer_stop,
	mxd_xia_dxp_timer_clear,
	mxd_xia_dxp_timer_read,
	mxd_xia_dxp_timer_get_mode,
	mxd_xia_dxp_timer_set_mode,
	NULL,
	mxd_xia_dxp_timer_get_last_measurement_time
};

/* MX mca timer data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_xia_dxp_timer_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_TIMER_STANDARD_FIELDS,
	MXD_XIA_DXP_TIMER_STANDARD_FIELDS
};

long mxd_xia_dxp_timer_num_record_fields
		= sizeof( mxd_xia_dxp_timer_record_field_defaults )
		  / sizeof( mxd_xia_dxp_timer_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_xia_dxp_timer_rfield_def_ptr
			= &mxd_xia_dxp_timer_record_field_defaults[0];

/*=======================================================================*/

static mx_status_type
mxd_xia_dxp_timer_get_pointers( MX_TIMER *timer,
			MX_XIA_DXP_TIMER **xia_dxp_timer,
			const char *calling_fname )
{
	static const char fname[] = "mxd_xia_dxp_timer_get_pointers()";

	if ( timer == (MX_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The timer pointer passed by '%s' was NULL",
			calling_fname );
	}

	if ( timer->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_RECORD pointer for timer pointer passed by '%s' is NULL.",
			calling_fname );
	}

	if ( xia_dxp_timer != (MX_XIA_DXP_TIMER **) NULL ) {

		*xia_dxp_timer = (MX_XIA_DXP_TIMER *)
				timer->record->record_type_struct;

		if ( *xia_dxp_timer == (MX_XIA_DXP_TIMER *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_XIA_DXP_TIMER pointer for timer record '%s' passed by '%s' is NULL",
				timer->record->name, calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_xia_dxp_timer_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_xia_dxp_timer_create_record_structures()";

	MX_TIMER *timer;
	MX_XIA_DXP_TIMER *xia_dxp_timer;

	/* Allocate memory for the necessary structures. */

	timer = (MX_TIMER *) malloc( sizeof(MX_TIMER) );

	if ( timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_TIMER structure." );
	}

	xia_dxp_timer = (MX_XIA_DXP_TIMER *)
				malloc( sizeof(MX_XIA_DXP_TIMER) );

	if ( xia_dxp_timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_XIA_DXP_TIMER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = timer;
	record->record_type_struct = xia_dxp_timer;
	record->class_specific_function_list
			= &mxd_xia_dxp_timer_timer_function_list;

	timer->record = record;

	timer->mode = MXCM_PRESET_MODE;
	xia_dxp_timer->preset_time = 0.0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_xia_dxp_timer_is_busy( MX_TIMER *timer )
{
	static const char fname[] = "mxd_xia_dxp_timer_is_busy()";

	MX_XIA_DXP_TIMER *xia_dxp_timer;
	mx_bool_type busy;
	mx_status_type mx_status;

	xia_dxp_timer = NULL;

	mx_status = mxd_xia_dxp_timer_get_pointers( timer,
						&xia_dxp_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_mca_is_busy( xia_dxp_timer->mca_record, &busy );

	timer->busy = busy;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_xia_dxp_timer_start( MX_TIMER *timer )
{
	static const char fname[] = "mxd_xia_dxp_timer_start()";

	MX_XIA_DXP_TIMER *xia_dxp_timer;
	double seconds_to_count;
	mx_status_type mx_status;

	xia_dxp_timer = NULL;

	mx_status = mxd_xia_dxp_timer_get_pointers( timer,
						&xia_dxp_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	seconds_to_count = timer->value;

	xia_dxp_timer->preset_time = seconds_to_count;

	if ( xia_dxp_timer->use_real_time ) {
		mx_status = mx_mca_start_for_preset_real_time(
			xia_dxp_timer->mca_record, seconds_to_count );
	} else {
		mx_status = mx_mca_start_for_preset_live_time(
			xia_dxp_timer->mca_record, seconds_to_count );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_xia_dxp_timer_stop( MX_TIMER *timer )
{
	static const char fname[] = "mxd_xia_dxp_timer_stop()";

	MX_XIA_DXP_TIMER *xia_dxp_timer;
	mx_status_type mx_status;

	xia_dxp_timer = NULL;

	mx_status = mxd_xia_dxp_timer_get_pointers( timer,
						&xia_dxp_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_mca_stop( xia_dxp_timer->mca_record );

	timer->value = 0.0;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_xia_dxp_timer_clear( MX_TIMER *timer )
{
	static const char fname[] = "mxd_xia_dxp_timer_clear()";

	MX_XIA_DXP_TIMER *xia_dxp_timer;
	mx_status_type mx_status;

	xia_dxp_timer = NULL;

	mx_status = mxd_xia_dxp_timer_get_pointers( timer,
						&xia_dxp_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_mca_clear( xia_dxp_timer->mca_record );

	timer->value = 0.0;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_xia_dxp_timer_read( MX_TIMER *timer )
{
	static const char fname[] = "mxd_xia_dxp_timer_read()";

	MX_XIA_DXP_TIMER *xia_dxp_timer;
	double live_time, real_time;
	mx_status_type mx_status;

	xia_dxp_timer = NULL;

	mx_status = mxd_xia_dxp_timer_get_pointers( timer,
						&xia_dxp_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( xia_dxp_timer->use_real_time ) {
		mx_status = mx_mca_get_real_time( xia_dxp_timer->mca_record,
							&real_time );

		timer->value = real_time;
	} else {
		mx_status = mx_mca_get_live_time( xia_dxp_timer->mca_record,
							&live_time );

		timer->value = live_time;
	}
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_xia_dxp_timer_get_mode( MX_TIMER *timer )
{
	timer->mode = MXCM_PRESET_MODE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_xia_dxp_timer_set_mode( MX_TIMER *timer )
{
	static const char fname[] = "mxd_xia_dxp_timer_set_mode()";

	if ( timer->mode != MXCM_PRESET_MODE ) {
		long mode;

		mode = timer->mode;
		timer->mode = MXCM_PRESET_MODE;

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Timer mode %ld is illegal for XIA timer '%s'",
			mode, timer->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_xia_dxp_timer_get_last_measurement_time( MX_TIMER *timer )
{
	static const char fname[] =
				"mxd_xia_dxp_timer_get_last_measurement_time()";

	MX_RECORD *xia_dxp_record;
	MX_XIA_DXP_TIMER *xia_dxp_timer;
	mx_status_type mx_status;

	MX_XIA_NETWORK *xia_network;

#if HAVE_XIA_HANDEL
	MX_XIA_HANDEL *xia_handel;
#endif
#if HAVE_XIA_XERXES
	MX_XIA_XERXES *xia_xerxes;
#endif

	xia_dxp_timer = NULL;

	mx_status = mxd_xia_dxp_timer_get_pointers( timer,
						&xia_dxp_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	xia_dxp_record = xia_dxp_timer->xia_dxp_record;

	if ( xia_dxp_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The xia_dxp_record pointer for XIA timer '%s' is NULL.",
			timer->record->name );
	}

	switch( xia_dxp_record->mx_type ) {
	case MXI_GEN_XIA_NETWORK:
		xia_network = (MX_XIA_NETWORK *)
					xia_dxp_record->record_type_struct;

		timer->last_measurement_time =
					xia_network->last_measurement_interval;
		break;

#if HAVE_XIA_HANDEL
	case MXI_GEN_XIA_HANDEL:
		xia_handel = (MX_XIA_HANDEL *)
					xia_dxp_record->record_type_struct;

		timer->last_measurement_time =
					xia_handel->last_measurement_interval;
		break;
#endif

#if HAVE_XIA_XERXES
	case MXI_GEN_XIA_XERXES:
		xia_xerxes = (MX_XIA_XERXES *)
					xia_dxp_record->record_type_struct;

		timer->last_measurement_time =
					xia_xerxes->last_measurement_interval;
		break;
#endif

	default:
		timer->last_measurement_time = -1.0e6;

		return mx_error( MXE_TYPE_MISMATCH, fname,
		"xia_dxp_record '%s' used by XIA DXP timer '%s' "
		"is not a supported record type.",
			xia_dxp_record->name, timer->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

