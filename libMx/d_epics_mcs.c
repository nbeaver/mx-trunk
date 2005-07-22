/*
 * Name:    d_epics_mcs.c 
 *
 * Purpose: MX multichannel scaler driver for the EPICS multichannel
 *          scaler support.  This currently is just for the Struck MCS.
 *
 *          Please note that the EPICS MCS record numbers scalers starting
 *          at 1, but the mcs->data_array numbers scalers starting at 0.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mxconfig.h"

#if HAVE_EPICS

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_epics.h"
#include "mx_measurement.h"
#include "mx_mcs.h"
#include "d_epics_mcs.h"

/* Initialize the mcs driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_epics_mcs_record_function_list = {
	mxd_epics_mcs_initialize_type,
	mxd_epics_mcs_create_record_structures,
	mxd_epics_mcs_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_epics_mcs_open
};

MX_MCS_FUNCTION_LIST mxd_epics_mcs_mcs_function_list = {
	mxd_epics_mcs_start,
	mxd_epics_mcs_stop,
	mxd_epics_mcs_clear,
	mxd_epics_mcs_busy,
	mxd_epics_mcs_read_all,
	mxd_epics_mcs_read_scaler,
	mxd_epics_mcs_read_measurement,
	NULL,
	mxd_epics_mcs_get_parameter,
	mxd_epics_mcs_set_parameter
};

/* EPICS mcs data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_epics_mcs_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_MCS_STANDARD_FIELDS,
	MXD_EPICS_MCS_STANDARD_FIELDS
};

long mxd_epics_mcs_num_record_fields
		= sizeof( mxd_epics_mcs_record_field_defaults )
			/ sizeof( mxd_epics_mcs_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_epics_mcs_rfield_def_ptr
			= &mxd_epics_mcs_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_epics_mcs_get_pointers( MX_MCS *mcs,
			MX_EPICS_MCS **epics_mcs,
			const char *calling_fname )
{
	const char fname[] = "mxd_epics_mcs_get_pointers()";

	if ( mcs == (MX_MCS *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MCS pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( epics_mcs == (MX_EPICS_MCS **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_EPICS_MCS pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( mcs->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
    "The MX_RECORD pointer for the MX_MCS pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*epics_mcs = (MX_EPICS_MCS *) mcs->record->record_type_struct;

	if ( *epics_mcs == (MX_EPICS_MCS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_EPICS_MCS pointer for record '%s' is NULL.",
			mcs->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_epics_mcs_initialize_type( long record_type )
{
	MX_RECORD_FIELD_DEFAULTS *record_field_defaults;
	long num_record_fields;
	long maximum_num_scalers_varargs_cookie;
	long maximum_num_measurements_varargs_cookie;
	mx_status_type status;

	status = mx_mcs_initialize_type( record_type,
				&num_record_fields,
				&record_field_defaults,
				&maximum_num_scalers_varargs_cookie,
				&maximum_num_measurements_varargs_cookie );

	return status;
}

MX_EXPORT mx_status_type
mxd_epics_mcs_create_record_structures( MX_RECORD *record )
{
	const char fname[] = "mxd_epics_mcs_create_record_structures()";

	MX_MCS *mcs;
	MX_EPICS_MCS *epics_mcs;

	/* Allocate memory for the necessary structures. */

	mcs = (MX_MCS *) malloc( sizeof(MX_MCS) );

	if ( mcs == (MX_MCS *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MCS structure." );
	}

	epics_mcs = (MX_EPICS_MCS *) malloc( sizeof(MX_EPICS_MCS) );

	if ( epics_mcs == (MX_EPICS_MCS *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_EPICS_MCS structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = mcs;
	record->record_type_struct = epics_mcs;
	record->class_specific_function_list = &mxd_epics_mcs_mcs_function_list;

	mcs->record = record;

	epics_mcs->scaler_value_buffer = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_mcs_finish_record_initialization( MX_RECORD *record )
{
	mx_status_type mx_status;

	mx_status = mx_mcs_finish_record_initialization( record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epics_mcs_open( MX_RECORD *record )
{
	const char fname[] = "mxd_epics_mcs_open()";

	MX_MCS *mcs;
	MX_EPICS_MCS *epics_mcs;
	double version_number;
	long i, nmax, allowed_maximum;
	unsigned long do_not_skip;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	mcs = (MX_MCS *) (record->record_class_struct);

	mx_status = mxd_epics_mcs_get_pointers( mcs, &epics_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize MX EPICS data structures whose PV names do not depend
	 * on the version of the MCA record in the remote EPICS crate.
	 *
	 * Please note that the following initialization steps do not
	 * invoke Channel Access and cause _no_ network I/O.  All they
	 * do is create the string representation of the process variable
	 * names for later use.
	 */

	mx_epics_pvname_init( &(epics_mcs->vers_pv),
				"%s1.VERS", epics_mcs->channel_prefix );

	mx_epics_pvname_init( &(epics_mcs->nmax_pv),
				"%s1.NMAX", epics_mcs->channel_prefix );

	epics_mcs->num_scaler_pvs = mcs->maximum_num_scalers;

	epics_mcs->dark_pv_array = (MX_EPICS_PV *)
		malloc( epics_mcs->num_scaler_pvs * sizeof(MX_EPICS_PV) );

	if ( epics_mcs->dark_pv_array == (MX_EPICS_PV *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Ran out of memory allocating %ld elments for dark_pv_array "
	"used by MCS record '%s'.", epics_mcs->num_scaler_pvs, record->name );
	}

	epics_mcs->read_pv_array = (MX_EPICS_PV *)
		malloc( epics_mcs->num_scaler_pvs * sizeof(MX_EPICS_PV) );

	if ( epics_mcs->read_pv_array == (MX_EPICS_PV *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Ran out of memory allocating %ld elments for read_pv_array "
	"used by MCS record '%s'.", epics_mcs->num_scaler_pvs, record->name );
	}

	epics_mcs->val_pv_array = (MX_EPICS_PV *)
		malloc( epics_mcs->num_scaler_pvs * sizeof(MX_EPICS_PV) );

	if ( epics_mcs->val_pv_array == (MX_EPICS_PV *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Ran out of memory allocating %ld elments for val_pv_array "
	"used by MCS record '%s'.", epics_mcs->num_scaler_pvs, record->name );
	}

	for ( i = 0; i < epics_mcs->num_scaler_pvs; i++ ) {
		mx_epics_pvname_init( &(epics_mcs->dark_pv_array[i]),
			"%s%ld_Dark.VAL", epics_mcs->channel_prefix, i + 1 );

		mx_epics_pvname_init( &(epics_mcs->read_pv_array[i]),
			"%s%ld.READ", epics_mcs->channel_prefix, i + 1 );

		mx_epics_pvname_init( &(epics_mcs->val_pv_array[i]),
			"%s%ld.VAL", epics_mcs->channel_prefix, i + 1 );
	}

#if 0
	for ( i = 0; i < epics_mcs->num_scaler_pvs; i++ ) {
		MX_DEBUG(-2,("i = %ld, %s, %s, %s", i,
			epics_mcs->dark_pv_array[i].pvname,
			epics_mcs->read_pv_array[i].pvname,
			epics_mcs->val_pv_array[i].pvname));
	}
#endif

	/* Find out if this EPICS device really exists in an IOC's database
	 * by asking for the record's version number.
	 */

	mx_status = mx_caget( &(epics_mcs->vers_pv),
				MX_CA_DOUBLE, 1, &version_number );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	epics_mcs->epics_record_version = version_number;

	/* Find out the maximum number of measurements that the EPICS
	 * records are configured for.  We _assume_ that it is the same
	 * for all channels.
	 */

	mx_status = mx_caget( &(epics_mcs->nmax_pv), MX_CA_LONG, 1, &nmax );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Check to see whether the MX MCS record is configurated for a larger
	 * number of measurements than the EPICS record is configured for.
	 * The precise value of the limit depends on whether we are skipping
	 * the first measurement or not.
	 */

	do_not_skip = epics_mcs->epics_mcs_flags
			& MXF_EPICS_MCS_DO_NOT_SKIP_FIRST_MEASUREMENT;

	if ( do_not_skip ) {
		allowed_maximum = nmax;
	} else {
		allowed_maximum = nmax - 1L;
	}

	if ( mcs->maximum_num_measurements > allowed_maximum ) {

		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
"MX MCS record '%s' is configured for a maximum number of measurements (%ld) "
"that is too large for the EPICS MCS '%s'.  The maximum allowed number of "
"measurements for the MX record is %ld.",
			record->name, mcs->maximum_num_measurements,
			epics_mcs->channel_prefix, allowed_maximum );
	}

	/* If MXF_EPICS_MCS_DO_NOT_SKIP_FIRST_MEASUREMENT is _not_ set,
	 * then we need to allocate a local buffer for mx_caget_by_name()
	 * to read the raw scaler array into.  Otherwise, we have no
	 * convenient way of skipping the first measurement.
	 */

	if ( do_not_skip == 0 ) {

		epics_mcs->scaler_value_buffer
				= (long *) malloc( nmax * sizeof(long) );

		if ( epics_mcs->scaler_value_buffer == (long *) NULL ) {

			return mx_error( MXE_OUT_OF_MEMORY, fname,
"Ran out of memory trying to allocate a %ld element scaler value buffer.",
				nmax );
		}
	}

	/* Initialize MX EPICS data structures whose PV names _do_ depend
	 * on the version of the MCA record in the remote EPICS crate.
	 */

	if ( epics_mcs->epics_record_version >= 5.0 ) {

		mx_epics_pvname_init(&(epics_mcs->acquiring_pv),
				"%sAcquiring.VAL", epics_mcs->common_prefix );

		mx_epics_pvname_init( &(epics_mcs->dwell_pv),
				"%sDwell.VAL", epics_mcs->common_prefix );

		mx_epics_pvname_init( &(epics_mcs->erase_pv),
				"%sEraseAll.VAL", epics_mcs->common_prefix );

		mx_epics_pvname_init( &(epics_mcs->nuse_pv),
				"%sNuseAll.VAL", epics_mcs->common_prefix );

		mx_epics_pvname_init( &(epics_mcs->pltm_pv), "" );

		if ( epics_mcs->epics_record_version >= 6.0 ) {
			mx_epics_pvname_init( &(epics_mcs->start_pv),
				"%sEraseStart.VAL", epics_mcs->common_prefix );
		} else {
			mx_epics_pvname_init( &(epics_mcs->start_pv),
				"%sStartAll.VAL", epics_mcs->common_prefix );
		}

		mx_epics_pvname_init( &(epics_mcs->stop_pv),
				"%sStopAll.VAL", epics_mcs->common_prefix );

	} else {

		mx_epics_pvname_init(&(epics_mcs->acquiring_pv),
				"%s1Start.VAL", epics_mcs->channel_prefix );

		mx_epics_pvname_init( &(epics_mcs->dwell_pv),
				"%s1.DWEL", epics_mcs->channel_prefix );

		mx_epics_pvname_init( &(epics_mcs->erase_pv),
				"%s1.ERAS", epics_mcs->channel_prefix );

		mx_epics_pvname_init( &(epics_mcs->nuse_pv), "" );

		mx_epics_pvname_init( &(epics_mcs->pltm_pv),
				"%s1.PLTM", epics_mcs->channel_prefix );

		mx_epics_pvname_init( &(epics_mcs->start_pv),
				"%s1Start.VAL", epics_mcs->channel_prefix );

		mx_epics_pvname_init( &(epics_mcs->stop_pv),
				"%s1Start.VAL", epics_mcs->channel_prefix );

	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_epics_mcs_start( MX_MCS *mcs )
{
	const char fname[] = "mxd_epics_mcs_start()";

	MX_EPICS_MCS *epics_mcs;
	long start;
	mx_status_type mx_status;

	mx_status = mxd_epics_mcs_get_pointers( mcs, &epics_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	start = 1;

	mx_status = mx_caput_nowait( &(epics_mcs->start_pv),
					MX_CA_LONG, 1, &start );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epics_mcs_stop( MX_MCS *mcs )
{
	const char fname[] = "mxd_epics_mcs_stop()";

	MX_EPICS_MCS *epics_mcs;
	long stop;
	mx_status_type mx_status;

	mx_status = mxd_epics_mcs_get_pointers( mcs, &epics_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	stop = 0;

	mx_status = mx_caput( &(epics_mcs->stop_pv),
				MX_CA_LONG, 1, &stop );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epics_mcs_clear( MX_MCS *mcs )
{
	const char fname[] = "mxd_epics_mcs_clear()";

	MX_EPICS_MCS *epics_mcs;
	long erase;
	mx_status_type mx_status;

	mx_status = mxd_epics_mcs_get_pointers( mcs, &epics_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	erase = 1;

	mx_status = mx_caput( &(epics_mcs->erase_pv),
				MX_CA_LONG, 1, &erase );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epics_mcs_busy( MX_MCS *mcs )
{
	const char fname[] = "mxd_epics_mcs_busy()";

	MX_EPICS_MCS *epics_mcs;
	long busy;
	mx_status_type mx_status;

	mx_status = mxd_epics_mcs_get_pointers( mcs, &epics_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_caget( &(epics_mcs->acquiring_pv),
				MX_CA_LONG, 1, &busy );

	if ( busy ) {
		mcs->busy = TRUE;
	} else {
		mcs->busy = FALSE;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_epics_mcs_read_all( MX_MCS *mcs )
{
	const char fname[] = "mxd_epics_mcs_read_all()";

	long i;
	mx_status_type mx_status;

	if ( mcs == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_MCS pointer passed was NULL." );
	}

	for ( i = 0; i < mcs->current_num_scalers; i++ ) {

		mcs->scaler_index = i;

		mx_status = mxd_epics_mcs_read_scaler( mcs );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_mcs_read_scaler( MX_MCS *mcs )
{
	const char fname[] = "mxd_epics_mcs_read_scaler()";

	MX_EPICS_MCS *epics_mcs;
	unsigned long do_not_skip, num_measurements_from_epics;
	long read_cmd;
	long *data_ptr, *source_ptr, *destination_ptr;
	size_t num_bytes_to_copy;
	mx_status_type mx_status;

	mx_status = mxd_epics_mcs_get_pointers( mcs, &epics_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	read_cmd = 1;

	mx_status = mx_caput( &(epics_mcs->read_pv_array[ mcs->scaler_index ]),
				MX_CA_LONG, 1, &read_cmd );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If we plan to skip the first measurement, then we need to copy
	 * the data first to a special buffer.
	 */

	do_not_skip = epics_mcs->epics_mcs_flags
			& MXF_EPICS_MCS_DO_NOT_SKIP_FIRST_MEASUREMENT;

	if ( do_not_skip ) {
		data_ptr = mcs->data_array[ mcs->scaler_index ];

		num_measurements_from_epics = mcs->current_num_measurements;
	} else {
		data_ptr = epics_mcs->scaler_value_buffer;

		num_measurements_from_epics
				= mcs->current_num_measurements + 1L;
	}

	mx_status = mx_caget( &(epics_mcs->val_pv_array[ mcs->scaler_index ]),
			MX_CA_LONG, num_measurements_from_epics, data_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0
	{
		long i;

		for ( i = 0; i < num_measurements_from_epics; i++ ) {
			fprintf(stderr,"%ld ", data_ptr[i]);
		}
		fprintf(stderr,"\n");
	}
#endif

	/* If we are skipping the first measurement, then we need to copy
	 * the data from the temporary buffer to the final destination.
	 */

	if ( do_not_skip == 0 ) {

		source_ptr = epics_mcs->scaler_value_buffer + 1;

		destination_ptr = mcs->data_array[ mcs->scaler_index ];

		num_bytes_to_copy
			= mcs->current_num_measurements * sizeof(long);

		memcpy( destination_ptr, source_ptr, num_bytes_to_copy );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_mcs_read_measurement( MX_MCS *mcs )
{
	const char fname[] = "mxd_epics_mcs_read_measurement()";

	return mx_error( MXE_UNSUPPORTED, fname,
	"This function is not supported for EPICS multichannel scalers." );
}

MX_EXPORT mx_status_type
mxd_epics_mcs_get_parameter( MX_MCS *mcs )
{
	const char fname[] = "mxd_epics_mcs_get_parameter()";

	MX_EPICS_MCS *epics_mcs;
	double dark_current;
	mx_status_type mx_status;

	mx_status = mxd_epics_mcs_get_pointers( mcs, &epics_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for MCS '%s', parameter type '%s' (%d)",
		fname, mcs->record->name,
		mx_get_field_label_string( mcs->record, mcs->parameter_type ),
		mcs->parameter_type));

	switch( mcs->parameter_type ) {
	case MXLV_MCS_DARK_CURRENT:

		mx_status = mx_caget(
			&(epics_mcs->dark_pv_array[ mcs->scaler_index ]),
				MX_CA_DOUBLE, 1, &dark_current );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mcs->dark_current_array[ mcs->scaler_index ] = dark_current;
		break;
	default:
		return mx_mcs_default_get_parameter_handler( mcs );
		break;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_epics_mcs_set_parameter( MX_MCS *mcs )
{
	const char fname[] = "mxd_epics_mcs_set_parameter()";

	MX_EPICS_MCS *epics_mcs;
	double dwell_time, preset_live_time, dark_current;
	unsigned long do_not_skip;
	long current_num_epics_measurements;
	mx_status_type mx_status;

	mx_status = mxd_epics_mcs_get_pointers( mcs, &epics_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for MCS '%s', parameter type '%s' (%d)",
		fname, mcs->record->name,
		mx_get_field_label_string( mcs->record, mcs->parameter_type ),
		mcs->parameter_type));

	switch( mcs->parameter_type ) {
	case MXLV_MCS_MODE:

		if ( mcs->mode != MXM_PRESET_TIME ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal MCS mode %d selected.  Only preset time mode is "
		"allowed for an EPICS MCS.", mcs->mode );
		}
		break;

	case MXLV_MCS_MEASUREMENT_TIME:

		dwell_time = mcs->measurement_time;

		mx_status = mx_caput( &(epics_mcs->dwell_pv),
					MX_CA_DOUBLE, 1, &dwell_time );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;

	case MXLV_MCS_CURRENT_NUM_MEASUREMENTS:

		do_not_skip = epics_mcs->epics_mcs_flags
			& MXF_EPICS_MCS_DO_NOT_SKIP_FIRST_MEASUREMENT;

		if ( do_not_skip ) {
			current_num_epics_measurements
				= (long) mcs->current_num_measurements;
		} else {
			current_num_epics_measurements
				= (long) mcs->current_num_measurements + 1L;
		}

		if ( epics_mcs->epics_record_version >= 5.0 ) {

			mx_status = mx_caput( &(epics_mcs->nuse_pv),
				MX_CA_LONG, 1, &current_num_epics_measurements);
		} else {
			preset_live_time = mcs->measurement_time
				* (double) current_num_epics_measurements;

			mx_status = mx_caput( &(epics_mcs->pltm_pv),
					MX_CA_DOUBLE, 1, &preset_live_time );
		}

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;

	case MXLV_MCS_DARK_CURRENT:

		dark_current = mcs->dark_current_array[ mcs->scaler_index ];

		mx_status = mx_caput(
			&(epics_mcs->dark_pv_array[ mcs->scaler_index ]),
				MX_CA_DOUBLE, 1, &dark_current );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;

	default:
		return mx_mcs_default_set_parameter_handler( mcs );
		break;
	}
	MX_DEBUG( 2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

#endif /* HAVE_EPICS */

