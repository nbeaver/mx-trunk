/*
 * Name:    d_epics_scaler_mcs.c 
 *
 * Purpose: MX multichannel scaler driver for counter/timers controlled
 *          using the EPICS Scaler record.
 *
 *          Please note that the EPICS Scaler record numbers scalers starting
 *          at 1, but the mcs->data_array numbers scalers starting at 0.
 *
 * Author:  William Lavender
 *
 * Warning: The EPICS Scaler record was not written to be used in this way,
 *          so do not be surprised if odd things happen.
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2014-2015, 2018-2020 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_EPICS_SCALER_MCS_DEBUG_OPEN		FALSE

#define MXD_EPICS_SCALER_MCS_DEBUG_CLEAR	FALSE

#define MXD_EPICS_SCALER_MCS_DEBUG_CALLBACK	FALSE

#define MXD_EPICS_SCALER_MCS_DEBUG_TIMING	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_callback.h"
#include "mx_hrt_debug.h"

#include "mx_epics.h"
#include "mx_mcs.h"
#include "d_epics_scaler_mcs.h"

#define MXD_EPICS_SCALER_MCS_MAX_CHANNELS	64

/* MCS driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_epics_scaler_mcs_record_function_list = {
	mxd_epics_scaler_mcs_initialize_driver,
	mxd_epics_scaler_mcs_create_record_structures,
	mx_mcs_finish_record_initialization,
	NULL,
	NULL,
	mxd_epics_scaler_mcs_open
};

MX_MCS_FUNCTION_LIST mxd_epics_scaler_mcs_mcs_function_list = {
	mxd_epics_scaler_mcs_arm,
	NULL,
	mxd_epics_scaler_mcs_stop,
	mxd_epics_scaler_mcs_clear,
	mxd_epics_scaler_mcs_busy
};

/* MCS data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_epics_scaler_mcs_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_MCS_STANDARD_FIELDS,
	MXD_EPICS_SCALER_MCS_STANDARD_FIELDS
};

long mxd_epics_scaler_mcs_num_record_fields
		= sizeof( mxd_epics_scaler_mcs_record_field_defaults )
		/ sizeof( mxd_epics_scaler_mcs_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_epics_scaler_mcs_rfield_def_ptr
			= &mxd_epics_scaler_mcs_record_field_defaults[0];

/*-------------------------------------------------------------------*/

static mx_status_type
mxd_epics_scaler_mcs_get_pointers( MX_MCS *mcs,
			MX_EPICS_SCALER_MCS **epics_scaler_mcs,
			const char *calling_fname )
{
	static const char fname[] = "mxd_epics_scaler_mcs_get_pointers()";

	if ( mcs == (MX_MCS *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MCS pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( epics_scaler_mcs == (MX_EPICS_SCALER_MCS **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EPICS_SCALER_MCS pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( mcs->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
    "The MX_RECORD pointer for the MX_MCS pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*epics_scaler_mcs = (MX_EPICS_SCALER_MCS *)
				mcs->record->record_type_struct;

	if ( *epics_scaler_mcs == (MX_EPICS_SCALER_MCS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_EPICS_SCALER_MCS pointer for record '%s' is NULL.",
			mcs->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------*/

static mx_status_type
mxd_epics_scaler_mcs_callback( MX_CALLBACK_MESSAGE *message )
{
	static const char fname[] = "mxd_epics_scaler_mcs_callback()";

	MX_RECORD *record;
	MX_MCS *mcs;
	MX_EPICS_SCALER_MCS *epics_scaler_mcs;
	MX_EPICS_GROUP epics_group;
	unsigned long i, j;
	double motor_position;
	long **data_array;
	int32_t cnt;
	static int32_t s_value_array_A[MXD_EPICS_SCALER_MCS_MAX_CHANNELS];
	static int32_t s_value_array_B[MXD_EPICS_SCALER_MCS_MAX_CHANNELS];
	static int32_t *s_value_array_curr = NULL;
	static int32_t *s_value_array_prev = NULL;
	mx_status_type mx_status;

#if MXD_EPICS_SCALER_MCS_DEBUG_TIMING
	MX_HRT_TIMING epics_measurement;
#endif

	if ( message == (MX_CALLBACK_MESSAGE *) NULL ) {
		return mx_error( MXE_UNKNOWN_ERROR, fname,
		"This callback was invoked with a NULL callback message!" );
	}

	if ( message->callback_type != MXCBT_FUNCTION ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Callback message %p sent to this callback function is not "
		"of type MXCBT_FUNCTION.  Instead, it is of type %lu",
			message, message->callback_type );
	}

	record = (MX_RECORD *) message->u.function.callback_args;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for callback message %p is NULL.",
			message );
	}

	mcs = (MX_MCS *) record->record_class_struct;

	mx_status = mxd_epics_scaler_mcs_get_pointers( mcs,
						&epics_scaler_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Restart the virtual timer to arrange for the next callback. */

	mx_status = mx_virtual_timer_start(
			message->u.function.oneshot_timer,
			message->u.function.callback_interval );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/********************************************************************/

	if ( mcs->last_measurement_number < 0 ) {
		mcs->last_measurement_number = -1;
	}

	/* We are about to readout a new measurement using an EPICS
	 * synchronous group, so increment the measurement number.
	 */

	mcs->last_measurement_number++;

	j = mcs->last_measurement_number;

	/* If this is the first step in a quick scan sequence, set both
	 * of the data arrays to 0.
	 */

	if ( j == 0 ) {
		memset( s_value_array_A, 0,
			MXD_EPICS_SCALER_MCS_MAX_CHANNELS * sizeof(int32_t) );
		memset( s_value_array_B, 0,
			MXD_EPICS_SCALER_MCS_MAX_CHANNELS * sizeof(int32_t) );

		s_value_array_curr = s_value_array_A;
		s_value_array_prev = s_value_array_B;

#if 0
		MX_DEBUG(-2,("%s: starting with A", fname));
#endif
	} else {
		/* Otherwise, switch the buffers that we are using. */

		if ( s_value_array_curr == s_value_array_A ) {
			s_value_array_curr = s_value_array_B;
			s_value_array_prev = s_value_array_A;

#if 0
			MX_DEBUG(-2,("%s: switching to B", fname));
#endif
		} else {
			s_value_array_curr = s_value_array_A;
			s_value_array_prev = s_value_array_B;

#if 0
			MX_DEBUG(-2,("%s: switching to A", fname));
#endif
		}
	}

#if MXD_EPICS_SCALER_MCS_DEBUG_TIMING
	MX_HRT_START( epics_measurement );
#endif

	/* All of our EPICS I/O needs to be synchronized as well as possible,
	 * so we make all of the calls in an EPICS synchronous group.
	 */

	mx_status = mx_epics_start_group( &epics_group );

	/* What is the current status of the EPICS Scaler record? */

	mx_status = mx_caget( &(epics_scaler_mcs->cnt_pv),
				MX_CA_LONG, 1, &cnt );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If requested, read out the position of the motor. */

	if ( epics_scaler_mcs->motor_record != (MX_RECORD *) NULL ) {

		mx_status = mx_caget( &(epics_scaler_mcs->motor_position_pv),
				MX_CA_DOUBLE, 1, &motor_position );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Read out all of the scaler channels. */

	for ( i = 0; i < epics_scaler_mcs->num_channels; i++ ) {
		mx_status = mx_caget( &(epics_scaler_mcs->s_pv_array[i]),
				MX_CA_LONG, 1, &(s_value_array_curr[i]) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Send the synchronous group to EPICS. */

	mx_epics_end_group( &epics_group );

#if MXD_EPICS_SCALER_MCS_DEBUG_TIMING
	MX_HRT_END( epics_measurement );
	MX_HRT_RESULTS( epics_measurement, fname, "acquisition" );
#endif

	/********************************************************************/

#if MXD_EPICS_SCALER_MCS_DEBUG_CALLBACK
	MX_DEBUG(-2,("%s: cnt = %ld", fname, (long) cnt));
#endif

	if ( cnt ) {
		mcs->busy = TRUE;
	} else {
		mcs->busy = FALSE;
	}

#if MXD_EPICS_SCALER_MCS_DEBUG_CALLBACK
	fprintf( stderr, "%s: meas(%lu) ", fname, j );
#endif

	/* If requested, save the position of the motor. */

	if ( epics_scaler_mcs->motor_record != (MX_RECORD *) NULL ) {

		epics_scaler_mcs->motor_position_array[j] = motor_position;

#if MXD_EPICS_SCALER_MCS_DEBUG_CALLBACK
		fprintf( stderr, "%g ", motor_position );
#endif
	}

	/* Copy the scaler data into the MCS data array. */

	data_array = mcs->data_array;

	for ( i = 0; i < epics_scaler_mcs->num_channels; i++ ) {

		data_array[i][j] =
			s_value_array_curr[i] - s_value_array_prev[i];

#if MXD_EPICS_SCALER_MCS_DEBUG_CALLBACK
#if 1
		fprintf(stderr,"%lu ", data_array[i][j]);
#else
		fprintf(stderr,"A%lu ",(unsigned long)s_value_array_prev[i]);
		fprintf(stderr,"B%lu ",(unsigned long)s_value_array_curr[i]);
		fprintf(stderr,"C%lu ", data_array[i][j]);
#endif
#endif
	}

#if MXD_EPICS_SCALER_MCS_DEBUG_CALLBACK
	fprintf(stderr,"\n");
#endif

	/* If the EPICS Scaler record is no longer busy, then
	 * delete this callback.
	 */

	if ( mcs->busy == FALSE ) {
		mx_status = mx_function_delete_callback(
				epics_scaler_mcs->callback_message );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_epics_scaler_mcs_initialize_driver( MX_DRIVER *driver )
{
	long maximum_num_scalers_varargs_cookie;
	long maximum_num_measurements_varargs_cookie;
	mx_status_type mx_status;

	mx_status = mx_mcs_initialize_driver( driver,
				&maximum_num_scalers_varargs_cookie,
				&maximum_num_measurements_varargs_cookie );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epics_scaler_mcs_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_epics_scaler_mcs_create_record_structures()";

	MX_MCS *mcs;
	MX_EPICS_SCALER_MCS *epics_scaler_mcs = NULL;

	/* Allocate memory for the necessary structures. */

	mcs = (MX_MCS *) malloc( sizeof(MX_MCS) );

	if ( mcs == (MX_MCS *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MCS structure." );
	}

	epics_scaler_mcs = (MX_EPICS_SCALER_MCS *)
				malloc( sizeof(MX_EPICS_SCALER_MCS) );

	if ( epics_scaler_mcs == (MX_EPICS_SCALER_MCS *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_EPICS_SCALER_MCS structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = mcs;
	record->record_type_struct = epics_scaler_mcs;
	record->class_specific_function_list =
				&mxd_epics_scaler_mcs_mcs_function_list;

	mcs->record = record;
	epics_scaler_mcs->record = record;

	strlcpy( record->network_type_name, "epics",
				MXU_NETWORK_TYPE_NAME_LENGTH );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_scaler_mcs_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_epics_scaler_mcs_open()";

	MX_MCS *mcs = NULL;
	MX_EPICS_SCALER_MCS *epics_scaler_mcs = NULL;
	MX_LIST_HEAD *list_head = NULL;
	char pvname[2*MXU_EPICS_PVNAME_LENGTH+20];
	uint16_t num_channels;
	int i;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	mcs = (MX_MCS *) record->record_class_struct;

	mx_status = mxd_epics_scaler_mcs_get_pointers( mcs,
						&epics_scaler_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mcs->last_measurement_number = -1;

	epics_scaler_mcs->motor_record = NULL;

	/* Invalidate the contents of the motor position PV. */

	epics_scaler_mcs->motor_position_pv.pvname[0] = '\0';
	epics_scaler_mcs->motor_position_pv.channel_id = NULL;

	/* The 'epics_scaler_mcs' driver acquires a measurement at a time
	 * rather than a scaler at a time, so we configure the readout
	 * preference to reflect that.
	 */

	mcs->readout_preference = MXF_MCS_PREFER_READ_MEASUREMENT;

	/* Allocate memory for the motor position array. */

	epics_scaler_mcs->motor_position_array =
		(double *) calloc( 1, sizeof(double) );

	if ( epics_scaler_mcs->motor_position_array == (double *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %lu element "
		"array of motor positions for record '%s'.",
			mcs->maximum_num_measurements,
			record->name );
	}

	/* Check to see if this database process has a callback pipe.
	 * If it does not, then warn the user that this driver cannot
	 * operate correctly.
	 */

	list_head = mx_get_record_list_head_struct( record );

	if ( list_head == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_LIST_HEAD pointer for the database containing "
		"record '%s' is NULL.  This should not be possible.",
			record->name );
	}

	if ( list_head->callback_pipe == NULL ) {

		return mx_error( MXE_SOFTWARE_CONFIGURATION_ERROR, fname,
		"The driver '%s' for record '%s' uses the MX callback pipe "
		"to service its callbacks.  However, this process does not "
		"have an MX callback pipe, so the driver _CANNOT_ run "
		"correctly.  Perhaps you need to be running this driver in "
		"an MX server?",
			mx_get_driver_name( record ),
			record->name );
	}

	/* Verify that the EPICS scaler record is there by asking for
	 * its software version.
	 */

	snprintf( pvname, sizeof(pvname),
				"%s.VERS", epics_scaler_mcs->epics_record_name);

	mx_status = mx_caget_by_name( pvname, MX_CA_DOUBLE,
					1, &(epics_scaler_mcs->version) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPICS_SCALER_MCS_DEBUG_OPEN
	MX_DEBUG(-2,("%s: '%s' = %f",
		fname, pvname, epics_scaler_mcs->version));
#endif

	/* Initialize the PVs we will use. */

	mx_epics_pvname_init( &(epics_scaler_mcs->cnt_pv),
				"%s.CNT", epics_scaler_mcs->epics_record_name );

	mx_epics_pvname_init( &(epics_scaler_mcs->rate_pv),
				"%s.RATE", epics_scaler_mcs->epics_record_name);

	mx_epics_pvname_init( &(epics_scaler_mcs->tp_pv),
				"%s.TP", epics_scaler_mcs->epics_record_name );

	/* Find out how many channels the EPICS scaler has. */

	snprintf( pvname, sizeof(pvname),
				"%s.NCH", epics_scaler_mcs->epics_record_name);

	mx_status = mx_caget_by_name( pvname, MX_CA_SHORT, 1, &num_channels );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	epics_scaler_mcs->num_channels = num_channels;

#if MXD_EPICS_SCALER_MCS_DEBUG_OPEN
	MX_DEBUG(-2,("%s: '%s' num_channels = %lu",
		fname, record->name, epics_scaler_mcs->num_channels));
#endif

	/* Allocate an array of MX_EPICS_PV structures to contain the values
	 * from the S1 to S64 readout PVs.
	 */

	epics_scaler_mcs->s_pv_array = (MX_EPICS_PV *)
		malloc( num_channels * sizeof(MX_EPICS_PV) );

	if ( epics_scaler_mcs->s_pv_array == (MX_EPICS_PV *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %d element "
		"array of MX_EPICS_PV structures for MCS '%s'.",
			(int) num_channels, record->name );
	}

	/* Initialize all of the channels in 's_pv_array'. */

	for ( i = 0; i < num_channels; i++ ) {
		mx_epics_pvname_init( &(epics_scaler_mcs->s_pv_array[i]),
			"%s.S%d", epics_scaler_mcs->epics_record_name, (i+1) );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epics_scaler_mcs_arm( MX_MCS *mcs )
{
	static const char fname[] = "mxd_epics_scaler_mcs_arm()";

	MX_EPICS_SCALER_MCS *epics_scaler_mcs;
	int32_t cnt;
	double time_preset;
	mx_status_type mx_status;

	mx_status = mxd_epics_scaler_mcs_get_pointers( mcs,
						&epics_scaler_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mcs->last_measurement_number = -1;

	/* Compute and set the EPICS scaler time preset. */

	time_preset = mcs->measurement_time * mcs->current_num_measurements;

	mx_status = mx_caput( &(epics_scaler_mcs->tp_pv),
				MX_CA_DOUBLE, 1, &time_preset );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Start the "MCS" counting. */

	cnt = 1;

	mx_status = mx_caput_nowait( &(epics_scaler_mcs->cnt_pv),
					MX_CA_LONG, 1, &cnt );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Setup the callback that we will use to monitor "MCS" data 
	 * acquisition.
	 */

	mx_status = mx_function_add_callback( mcs->record,
					mxd_epics_scaler_mcs_callback,
					NULL,
					mcs->record,
					mcs->measurement_time,
					&(epics_scaler_mcs->callback_message) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epics_scaler_mcs_stop( MX_MCS *mcs )
{
	static const char fname[] = "mxd_epics_scaler_mcs_stop()";

	MX_EPICS_SCALER_MCS *epics_scaler_mcs;
	int32_t cnt;
	mx_status_type mx_status;

	mx_status = mxd_epics_scaler_mcs_get_pointers( mcs,
						&epics_scaler_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	cnt = 0;

	mx_status = mx_caput_nowait( &(epics_scaler_mcs->cnt_pv),
					MX_CA_LONG, 1, &cnt );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epics_scaler_mcs_clear( MX_MCS *mcs )
{
	static const char fname[] = "mxd_epics_scaler_mcs_clear()";

	MX_EPICS_SCALER_MCS *epics_scaler_mcs;
	long i, j;
	mx_status_type mx_status;

	mx_status = mxd_epics_scaler_mcs_get_pointers( mcs,
						&epics_scaler_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_EPICS_SCALER_MCS_DEBUG_CLEAR
	MX_DEBUG(-2,("%s invoked for '%s'", fname, mcs->record->name));
#endif

	mcs->last_measurement_number = -1;

	for ( i = 0; i < mcs->maximum_num_scalers; i++ ) {
		for ( j = 0; j < mcs->maximum_num_measurements; j++ ) {
			mcs->data_array[i][j] = 0;
		}
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epics_scaler_mcs_busy( MX_MCS *mcs )
{
	/* mcs->busy will have been set by the callback function
	 * mxd_epics_scaler_mcs_callback(), so we just return the
	 * value already in the variable.
	 */

	return MX_SUCCESSFUL_RESULT;
}

