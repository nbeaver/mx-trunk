/*
 * Name:    d_epics_timer.c
 *
 * Purpose: MX timer driver for the EPICS APS-XFD developed scaler record.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2004, 2006, 2008-2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "mxconfig.h"

#if HAVE_EPICS

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_epics.h"
#include "mx_measurement.h"
#include "mx_timer.h"
#include "d_epics_timer.h"

/* Initialize the timer driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_epics_timer_record_function_list = {
	NULL,
	mxd_epics_timer_create_record_structures,
	mxd_epics_timer_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_epics_timer_open,
};

MX_TIMER_FUNCTION_LIST mxd_epics_timer_timer_function_list = {
	mxd_epics_timer_is_busy,
	mxd_epics_timer_start,
	mxd_epics_timer_stop,
	mxd_epics_timer_clear,
	mxd_epics_timer_read,
	mxd_epics_timer_get_mode,
	mxd_epics_timer_set_mode,
	mxd_epics_timer_set_modes_of_associated_counters
};

/* EPICS timer data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_epics_timer_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_TIMER_STANDARD_FIELDS,
	MXD_EPICS_TIMER_STANDARD_FIELDS
};

long mxd_epics_timer_num_record_fields
		= sizeof( mxd_epics_timer_record_field_defaults )
		  / sizeof( mxd_epics_timer_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_epics_timer_rfield_def_ptr
			= &mxd_epics_timer_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_epics_timer_get_pointers( MX_TIMER *timer,
			MX_EPICS_TIMER **epics_timer,
			const char *calling_fname )
{
	static const char fname[] = "mxd_epics_timer_get_pointers()";

	if ( timer == (MX_TIMER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_TIMER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( epics_timer == (MX_EPICS_TIMER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EPICS_TIMER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( timer->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for the MX_TIMER pointer "
		"passed by '%s' was NULL.", calling_fname );
	}

	*epics_timer = (MX_EPICS_TIMER *) timer->record->record_type_struct;

	if ( *epics_timer == (MX_EPICS_TIMER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_EPICS_TIMER pointer for record '%s' is NULL.",
			timer->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*======*/

MX_EXPORT mx_status_type
mxd_epics_timer_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_epics_timer_create_record_structures()";

	MX_TIMER *timer;
	MX_EPICS_TIMER *epics_timer;

	/* Allocate memory for the necessary structures. */

	timer = (MX_TIMER *) malloc( sizeof(MX_TIMER) );

	if ( timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_TIMER structure." );
	}

	epics_timer = (MX_EPICS_TIMER *)
				malloc( sizeof(MX_EPICS_TIMER) );

	if ( epics_timer == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_EPICS_TIMER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = timer;
	record->record_type_struct = epics_timer;
	record->class_specific_function_list
			= &mxd_epics_timer_timer_function_list;

	timer->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_timer_finish_record_initialization( MX_RECORD *record )
{
	return mx_timer_finish_record_initialization( record );
}

MX_EXPORT mx_status_type
mxd_epics_timer_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_epics_timer_open()";

	MX_TIMER *timer;
	MX_EPICS_TIMER *epics_timer = NULL;
	double version_number;
	long i;
	char pvname[80];
	char driver_name[80];
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	timer = (MX_TIMER *) (record->record_class_struct);

	mx_status = mxd_epics_timer_get_pointers( timer, &epics_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	timer->mode = MXCM_COUNTER_MODE;

	/* Find out if this EPICS record really exists in an IOC's database
	 * by asking for the record's version number.
	 */

	snprintf( pvname, sizeof(pvname),
		"%s.VERS", epics_timer->epics_record_name );

	mx_status = mx_caget_by_name(pvname, MX_CA_DOUBLE, 1, &version_number);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	epics_timer->epics_record_version = version_number;

	MX_DEBUG( 2,("%s: epics_timer->epics_record_version = %g",
			fname, epics_timer->epics_record_version));

	/* Initialize MX EPICS variables. */

	mx_epics_pvname_init( &(epics_timer->cnt_pv),
				"%s.CNT", epics_timer->epics_record_name);

	if ( epics_timer->epics_record_version < 3.0 ) {
		mx_epics_pvname_init( &(epics_timer->cont_pv),
			"%s_mode.VAL", epics_timer->epics_record_name);
	} else {
		mx_epics_pvname_init( &(epics_timer->cont_pv),
			"%s.CONT", epics_timer->epics_record_name);
	}

	mx_epics_pvname_init( &(epics_timer->freq_pv),
				"%s.FREQ", epics_timer->epics_record_name);

	mx_epics_pvname_init( &(epics_timer->nch_pv),
				"%s.NCH", epics_timer->epics_record_name);

	mx_epics_pvname_init( &(epics_timer->t_pv),
				"%s.T", epics_timer->epics_record_name);

	mx_epics_pvname_init( &(epics_timer->tp_pv),
				"%s.TP", epics_timer->epics_record_name);

	/* Find out what type of EPICS scaler record this is. */

	snprintf( pvname, sizeof(pvname),
		"%s.DTYP", epics_timer->epics_record_name );

	mx_status = mx_caget_by_name( pvname, MX_CA_STRING, 1, driver_name );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: record '%s' driver_name = '%s'",
		fname, record->name, driver_name));

	if ( strcmp( driver_name, "MxScaler" ) == 0 ) {
		epics_timer->driver_type = MXT_EPICS_SCALER_MX;
	} else {
		epics_timer->driver_type = MXT_EPICS_SCALER_BCDA;
	}

	/* Find out how many counter channels the EPICS scaler record has. */

	mx_status = mx_caget( &(epics_timer->nch_pv),
			MX_CA_SHORT, 1, &(epics_timer->num_epics_counters) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: Record '%s' num_epics_counters = %hd",
		fname, record->name, epics_timer->num_epics_counters));

	epics_timer->gate_control_pv_array = (MX_EPICS_PV *)
		malloc( epics_timer->num_epics_counters * sizeof(MX_EPICS_PV) );

	if ( epics_timer->gate_control_pv_array == (MX_EPICS_PV *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Ran out of memory allocating %hd elements for gate_control_pv_array "
	"used by timer record '%s'.",
			epics_timer->num_epics_counters, record->name );
	}

	for ( i = 0; i < epics_timer->num_epics_counters; i++ ) {
		mx_epics_pvname_init( &(epics_timer->gate_control_pv_array[i]),
			"%s.G%ld", epics_timer->epics_record_name, i+1 );
	}

	/* Get the clock frequency for the timer. */

	mx_status = mx_caget( &(epics_timer->freq_pv),
			MX_CA_DOUBLE, 1, &(epics_timer->clock_frequency) );

	MX_DEBUG( 2,("%s: epics_timer->clock_frequency = %g",
			fname, epics_timer->clock_frequency));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epics_timer_is_busy( MX_TIMER *timer )
{
	static const char fname[] = "mxd_epics_timer_is_busy()";

	MX_EPICS_TIMER *epics_timer = NULL;
	int32_t count_field;
	mx_status_type mx_status;

	mx_status = mxd_epics_timer_get_pointers( timer, &epics_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_caget( &(epics_timer->cnt_pv),
				MX_CA_LONG, 1, &count_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch ( count_field ) {
	case 0:
		timer->busy = FALSE;
		break;
	case 1:
		timer->busy = TRUE;
		break;
	default:
		timer->busy = FALSE;

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Count field (.CNT) for scaler '%s' had an unexpected value of %ld",
			epics_timer->epics_record_name, (long) count_field );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_timer_start( MX_TIMER *timer )
{
	static const char fname[] = "mxd_epics_timer_start()";

	MX_EPICS_TIMER *epics_timer = NULL;
	double seconds;
	int32_t count_field;
	mx_status_type mx_status;

	mx_status = mxd_epics_timer_get_pointers( timer, &epics_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	seconds = timer->value;

	/* Set the number of seconds to count for.  This assumes that
	 * the .FREQ field has already been set up correctly.
	 */

	mx_status = mx_caput( &(epics_timer->tp_pv),
				MX_CA_DOUBLE, 1, &seconds );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0
	/* Set the count field to zero first. */

	MX_DEBUG(-2,("%s: About to set the count field to 0.", fname));

	count_field = 0;

	if ( epics_timer->epics_record_version < 3.0 ) {
		mx_status = mx_caput( &(epics_timer->cnt_pv),
					MX_CA_LONG, 1, &count_field );
	} else {
		mx_status = mx_caput_nowait( &(epics_timer->cnt_pv),
					MX_CA_LONG, 1, &count_field );
	}

	MX_DEBUG(-2,("%s: Done setting the count field to 0.", fname));

#endif

	/* Start the timer counting. */

	MX_DEBUG( 2,("%s: About to set the count field to 1.", fname));

	count_field = 1;

	if ( epics_timer->epics_record_version < 3.0 ) {
		mx_status = mx_caput( &(epics_timer->cnt_pv),
					MX_CA_LONG, 1, &count_field );
	} else {
		mx_status = mx_caput_nowait( &(epics_timer->cnt_pv),
					MX_CA_LONG, 1, &count_field );
	}

	MX_DEBUG( 2,("%s: Done setting the count field to 1.", fname));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epics_timer_stop( MX_TIMER *timer )
{
	static const char fname[] = "mxd_epics_timer_stop()";

	MX_EPICS_TIMER *epics_timer = NULL;
	int32_t count_field;
	mx_status_type mx_status;

	mx_status = mxd_epics_timer_get_pointers( timer, &epics_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Stop the timer from counting. */

	MX_DEBUG( 2,("%s: About to stop the timer.", fname));

	count_field = 0;

#if 1
	mx_status = mx_caput( &(epics_timer->cnt_pv),
				MX_CA_LONG, 1, &count_field );
#else
	mx_status = mx_caput_nowait( &(epics_timer->cnt_pv),
				MX_CA_LONG, 1, &count_field );
#endif

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: Done stopping the timer.", fname));

	timer->value = 0.0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_timer_clear( MX_TIMER *timer )
{
	/* It appears that it is not possible to directly clear
	 * an EPICS scaler channel.  So all we can do is return
	 * a success code.
	 */

	timer->value = 0.0;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_timer_read( MX_TIMER *timer )
{
	static const char fname[] = "mxd_epics_timer_read()";

	MX_EPICS_TIMER *epics_timer = NULL;
	double seconds;
	mx_status_type mx_status;

	mx_status = mxd_epics_timer_get_pointers( timer, &epics_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_caget( &(epics_timer->t_pv),
				MX_CA_DOUBLE, 1, &seconds );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	timer->value = seconds;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_timer_get_mode( MX_TIMER *timer )
{
	static const char fname[] = "mxd_epics_timer_get_mode()";

	MX_EPICS_TIMER *epics_timer = NULL;
	int32_t gate_control;
	mx_status_type mx_status;

	mx_status = mxd_epics_timer_get_pointers( timer, &epics_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the gate control field G1. */

	mx_status = mx_caget( &(epics_timer->gate_control_pv_array[0]),
				MX_CA_LONG, 1, &gate_control );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( gate_control ) {
	case 0:
		timer->mode = MXCM_COUNTER_MODE;
		break;

	case 1:
		timer->mode = MXCM_PRESET_MODE;
		break;

	default:
		return mx_error( MXE_CONTROLLER_INTERNAL_ERROR, fname,
		"The gate control field for EPICS timer '%s' (%s) "
		"has the unexpected value %ld.",
			timer->record->name,
			epics_timer->epics_record_name,
			(long) gate_control );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_timer_set_mode( MX_TIMER *timer )
{
	static const char fname[] = "mxd_epics_timer_set_mode()";

	MX_EPICS_TIMER *epics_timer = NULL;
	int32_t gate_control, counter_mode;
	mx_status_type mx_status;

	mx_status = mxd_epics_timer_get_pointers( timer, &epics_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Choose the correct gate control mode. */

	switch( timer->mode ) {
	case MXCM_PRESET_MODE:
		gate_control = 1;
		break;

	case MXCM_COUNTER_MODE:
		gate_control = 0;
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The counter mode %ld requested for timer '%s' is not supported.",
			timer->mode, timer->record->name );
		break;
	}

	/* Make sure the counter is _not_ in autocount mode. */

	counter_mode = 0;	/* One-shot mode. */

	mx_status = mx_caput( &(epics_timer->cont_pv),
				MX_CA_LONG, 1, &counter_mode );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Tell the timer to stop counting. */

	mx_status = mxd_epics_timer_stop( timer );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the gate control field G1. */

	mx_status = mx_caput( &(epics_timer->gate_control_pv_array[0]),
				MX_CA_LONG, 1, &gate_control );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epics_timer_set_modes_of_associated_counters( MX_TIMER *timer )
{
	static const char fname[]
		= "mxd_epics_timer_set_modes_of_associated_counters()";

	MX_EPICS_TIMER *epics_timer = NULL;
	MX_EPICS_GROUP epics_group;
	int i;
	int32_t gate_control;
	mx_status_type mx_status;

	mx_status = mxd_epics_timer_get_pointers( timer, &epics_timer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* FIXME - If the EPICS scaler is of DTYP 'MxScaler', then
	 * skip this part, since 'mxca_server' does not yet support
	 * synchronous groups.
	 */

	if ( epics_timer->driver_type == MXT_EPICS_SCALER_MX )
		return MX_SUCCESSFUL_RESULT;

	/* Start an EPICS synchronous group. */

	mx_epics_start_group( &epics_group );

	/* Set the gate control to 0 for fields for channels 2 through n.
	 * This is equivalent to setting them all to counter mode.
	 */

	for ( i = 2; i <= epics_timer->num_epics_counters; i++ ) {

		gate_control = 0;

		mx_status = mx_group_caput( &epics_group,
				&(epics_timer->gate_control_pv_array[i-1]),
					MX_CA_LONG, 1, &gate_control );

		if ( mx_status.code != MXE_SUCCESS ) {
			mx_epics_delete_group( &epics_group );

			return mx_status;
		}
	}

	/* Send the synchronous group to EPICS. */

	mx_epics_end_group( &epics_group );

	return MX_SUCCESSFUL_RESULT;
}

#endif /* HAVE_EPICS */

