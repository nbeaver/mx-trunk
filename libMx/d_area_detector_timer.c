/*
 * Name:    d_area_detector_timer.c
 *
 * Purpose: MX timer driver for using an area detector timer as
 *          an MX timer device.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_AREA_DETECTOR_TIMER_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_measurement.h"
#include "mx_image.h"
#include "mx_area_detector.h"
#include "mx_timer.h"
#include "d_area_detector_timer.h"

/* Initialize the timer driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_area_detector_timer_record_function_list = {
	NULL,
	mxd_area_detector_timer_create_record_structures,
	mxd_area_detector_timer_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_area_detector_timer_open
};

MX_TIMER_FUNCTION_LIST mxd_area_detector_timer_timer_function_list = {
	mxd_area_detector_timer_is_busy,
	mxd_area_detector_timer_start,
	mxd_area_detector_timer_stop
};

/* MX area detector timer data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_area_detector_timer_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_TIMER_STANDARD_FIELDS,
	MXD_AREA_DETECTOR_TIMER_STANDARD_FIELDS
};

long mxd_area_detector_timer_num_record_fields
		= sizeof( mxd_area_detector_timer_record_field_defaults )
		  / sizeof( mxd_area_detector_timer_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_area_detector_timer_rfield_def_ptr
			= &mxd_area_detector_timer_record_field_defaults[0];

/*=======================================================================*/

static mx_status_type
mxd_area_detector_timer_get_pointers( MX_TIMER *timer,
			MX_AREA_DETECTOR_TIMER **ad_timer,
			MX_RECORD **ad_record,
			const char *calling_fname )
{
	static const char fname[] = "mxd_area_detector_timer_get_pointers()";

	MX_AREA_DETECTOR_TIMER *ad_timer_ptr;

	if ( timer == (MX_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_TIMER pointer passed by '%s' was NULL",
			calling_fname );
	}

	ad_timer_ptr = (MX_AREA_DETECTOR_TIMER *)
				timer->record->record_type_struct;

	if ( ad_timer_ptr == (MX_AREA_DETECTOR_TIMER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_AREA_DETECTOR_TIMER pointer for timer record '%s' "
		"passed by '%s' is NULL",
			timer->record->name, calling_fname );
	}

	if ( ad_timer != (MX_AREA_DETECTOR_TIMER **) NULL ) {
		*ad_timer = ad_timer_ptr;
	}

	if ( ad_record != (MX_RECORD **) NULL ) {
		if (ad_timer_ptr->area_detector_record == (MX_RECORD *) NULL) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_RECORD pointer for the area detector used "
			"by area detector timer '%s' is NULL.",
				timer->record->name );
		}

		*ad_record = ad_timer_ptr->area_detector_record;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_area_detector_timer_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_area_detector_timer_create_record_structures()";

	MX_TIMER *timer;
	MX_AREA_DETECTOR_TIMER *ad_timer;

	/* Allocate memory for the necessary structures. */

	timer = (MX_TIMER *) malloc( sizeof(MX_TIMER) );

	if ( timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_TIMER structure." );
	}

	ad_timer = (MX_AREA_DETECTOR_TIMER *)
				malloc( sizeof(MX_AREA_DETECTOR_TIMER) );

	if ( ad_timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_AREA_DETECTOR_TIMER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = timer;
	record->record_type_struct = ad_timer;
	record->class_specific_function_list
			= &mxd_area_detector_timer_timer_function_list;

	timer->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_area_detector_timer_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_area_detector_timer_finish_record_initialization()";

	MX_TIMER *timer;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	timer = (MX_TIMER *) record->record_class_struct;

	if ( timer == (MX_TIMER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_TIMER pointer for record '%s' is NULL.",
			record->name );
	}

	timer->mode = MXCM_UNKNOWN_MODE;

	mx_status = mx_timer_finish_record_initialization( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_area_detector_timer_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_area_detector_timer_open()";

	MX_TIMER *timer;
	MX_AREA_DETECTOR_TIMER *ad_timer;
	MX_RECORD *ad_record;
	mx_status_type mx_status;

	ad_timer = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	timer = (MX_TIMER *) record->record_class_struct;

	mx_status = mxd_area_detector_timer_get_pointers( timer,
						&ad_timer, &ad_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_area_detector_timer_is_busy( MX_TIMER *timer )
{
	static const char fname[] = "mxd_area_detector_timer_is_busy()";

	MX_AREA_DETECTOR_TIMER *ad_timer;
	MX_RECORD *ad_record = NULL;
	mx_bool_type busy;
	mx_status_type mx_status;

	ad_timer = NULL;

	mx_status = mxd_area_detector_timer_get_pointers( timer,
						&ad_timer, &ad_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_is_busy( ad_record, &busy );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( busy ) {
		timer->busy = TRUE;
	} else {
		timer->busy = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_area_detector_timer_start( MX_TIMER *timer )
{
	static const char fname[] = "mxd_area_detector_timer_start()";

	MX_AREA_DETECTOR_TIMER *ad_timer;
	MX_RECORD *ad_record = NULL;
	mx_status_type mx_status;

	ad_timer = NULL;

	mx_status = mxd_area_detector_timer_get_pointers( timer,
						&ad_timer, &ad_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_set_one_shot_mode( ad_record,
							timer->value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_start( ad_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_area_detector_timer_stop( MX_TIMER *timer )
{
	static const char fname[] = "mxd_area_detector_timer_stop()";

	MX_AREA_DETECTOR_TIMER *ad_timer;
	MX_RECORD *ad_record = NULL;
	mx_status_type mx_status;

	ad_timer = NULL;

	mx_status = mxd_area_detector_timer_get_pointers( timer,
						&ad_timer, &ad_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_area_detector_stop( ad_record );

	timer->value = 0.0;

	return mx_status;
}

