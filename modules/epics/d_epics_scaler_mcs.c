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
 * Copyright 2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_epics.h"
#include "mx_mcs.h"
#include "d_epics_scaler_mcs.h"

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
	mxd_epics_scaler_mcs_start,
	mxd_epics_scaler_mcs_stop,
	NULL,
	mxd_epics_scaler_mcs_busy,
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

/* === */

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

/* === */

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

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_scaler_mcs_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_epics_scaler_mcs_open()";

	MX_MCS *mcs = NULL;
	MX_EPICS_SCALER_MCS *epics_scaler_mcs = NULL;
	char pvname[MXU_EPICS_PVNAME_LENGTH+1];
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

	/* Verify that the EPICS scaler record is there by asking for
	 * its software version.
	 */

	snprintf( pvname, sizeof(pvname),
				"%s.VERS", epics_scaler_mcs->epics_record_name);

	mx_status = mx_caget_by_name( pvname, MX_CA_DOUBLE,
					1, &(epics_scaler_mcs->version) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: '%s' = %f",
		fname, pvname, epics_scaler_mcs->version));

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

	MX_DEBUG(-2,("%s: '%s' num_channels = %lu",
		fname, record->name, epics_scaler_mcs->num_channels));

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
			"%s.S%d", epics_scaler_mcs->epics_record_name, i );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epics_scaler_mcs_start( MX_MCS *mcs )
{
	static const char fname[] = "mxd_epics_scaler_mcs_start()";

	MX_EPICS_SCALER_MCS *epics_scaler_mcs;
	int32_t cnt;
	mx_status_type mx_status;

	mx_status = mxd_epics_scaler_mcs_get_pointers( mcs,
						&epics_scaler_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	cnt = 1;

	mx_status = mx_caput_nowait( &(epics_scaler_mcs->cnt_pv),
					MX_CA_LONG, 1, &cnt );

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
mxd_epics_scaler_mcs_busy( MX_MCS *mcs )
{
	static const char fname[] = "mxd_epics_scaler_mcs_busy()";

	MX_EPICS_SCALER_MCS *epics_scaler_mcs;
	int32_t cnt;
	mx_status_type mx_status;

	mx_status = mxd_epics_scaler_mcs_get_pointers( mcs,
						&epics_scaler_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_caput_nowait( &(epics_scaler_mcs->cnt_pv),
					MX_CA_LONG, 1, &cnt );

	if ( cnt ) {
		mcs->busy = TRUE;
	} else {
		mcs->busy = FALSE;
	}

	return mx_status;
}

