/*
 * Name:    d_network_mcs.c 
 *
 * Purpose: MX multichannel scaler driver for MX network multichannel scalers.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------------
 *
 * Copyright 2000-2006, 2008, 2010, 2012, 2014-2016, 2018-2020
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_mcs.h"
#include "mx_net.h"
#include "d_network_mcs.h"
#include "d_network_timer.h"
#include "d_mcs_timer.h"

/* Initialize the mcs driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_network_mcs_record_function_list = {
	mxd_network_mcs_initialize_driver,
	mxd_network_mcs_create_record_structures,
	mxd_network_mcs_finish_record_initialization,
	NULL,
	mxd_network_mcs_print_structure,
	mxd_network_mcs_open
};

MX_MCS_FUNCTION_LIST mxd_network_mcs_mcs_function_list = {
	mxd_network_mcs_arm,
	mxd_network_mcs_trigger,
	mxd_network_mcs_stop,
	mxd_network_mcs_clear,
	NULL,
	mxd_network_mcs_status,
	mxd_network_mcs_read_all,
	mxd_network_mcs_read_scaler,
	mxd_network_mcs_read_measurement,
	mxd_network_mcs_read_scaler_measurement,
	mxd_network_mcs_read_timer,
	mxd_network_mcs_get_parameter,
	mxd_network_mcs_set_parameter
};

/* Network mcs data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_network_mcs_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_MCS_STANDARD_FIELDS,
	MXD_NETWORK_MCS_STANDARD_FIELDS
};

long mxd_network_mcs_num_record_fields
		= sizeof( mxd_network_mcs_record_field_defaults )
			/ sizeof( mxd_network_mcs_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_network_mcs_rfield_def_ptr
			= &mxd_network_mcs_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_network_mcs_get_pointers( MX_MCS *mcs,
			MX_NETWORK_MCS **network_mcs,
			const char *calling_fname )
{
	static const char fname[] = "mxd_network_mcs_get_pointers()";

	if ( mcs == (MX_MCS *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MCS pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( network_mcs == (MX_NETWORK_MCS **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NETWORK_MCS pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( mcs->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
    "The MX_RECORD pointer for the MX_MCS pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*network_mcs = (MX_NETWORK_MCS *) mcs->record->record_type_struct;

	if ( *network_mcs == (MX_NETWORK_MCS *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_NETWORK_MCS pointer for record '%s' is NULL.",
			mcs->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_network_mcs_initialize_driver( MX_DRIVER *driver )
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
mxd_network_mcs_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_network_mcs_create_record_structures()";

	MX_MCS *mcs;
	MX_NETWORK_MCS *network_mcs = NULL;

	/* Allocate memory for the necessary structures. */

	mcs = (MX_MCS *) malloc( sizeof(MX_MCS) );

	if ( mcs == (MX_MCS *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MCS structure." );
	}

	network_mcs = (MX_NETWORK_MCS *) malloc( sizeof(MX_NETWORK_MCS) );

	if ( network_mcs == (MX_NETWORK_MCS *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_NETWORK_MCS structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = mcs;
	record->record_type_struct = network_mcs;
	record->class_specific_function_list
				= &mxd_network_mcs_mcs_function_list;

	mcs->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_mcs_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[]
		= "mxd_network_mcs_finish_record_initialization()";

	MX_MCS *mcs;
	MX_NETWORK_MCS *network_mcs = NULL;
	mx_status_type mx_status;

	mcs = (MX_MCS *) record->record_class_struct;

	mx_status = mxd_network_mcs_get_pointers( mcs, &network_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	strlcpy(record->network_type_name, "mx", MXU_NETWORK_TYPE_NAME_LENGTH);

	mx_status = mx_mcs_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_network_field_init( &(network_mcs->arm_nf),
		network_mcs->server_record,
		"%s.arm", network_mcs->remote_record_name );

	mx_network_field_init( &(network_mcs->busy_nf),
		network_mcs->server_record,
		"%s.busy", network_mcs->remote_record_name );

	mx_network_field_init( &(network_mcs->clear_nf),
		network_mcs->server_record,
		"%s.clear", network_mcs->remote_record_name );

	mx_network_field_init( &(network_mcs->counting_mode_nf),
		network_mcs->server_record,
		"%s.counting_mode", network_mcs->remote_record_name );

	mx_network_field_init( &(network_mcs->current_num_measurements_nf),
		network_mcs->server_record,
	    "%s.current_num_measurements", network_mcs->remote_record_name );

	mx_network_field_init( &(network_mcs->current_num_scalers_nf),
		network_mcs->server_record,
	    "%s.current_num_scalers", network_mcs->remote_record_name );

	mx_network_field_init( &(network_mcs->dark_current_nf),
		network_mcs->server_record,
		"%s.dark_current", network_mcs->remote_record_name );

	mx_network_field_init( &(network_mcs->dark_current_array_nf),
		network_mcs->server_record,
		"%s.dark_current_array", network_mcs->remote_record_name );

	mx_network_field_init( &(network_mcs->data_array_nf),
		network_mcs->server_record,
		"%s.data_array", network_mcs->remote_record_name );

	mx_network_field_init( &(network_mcs->external_next_measurement_nf),
		network_mcs->server_record,
	    "%s.external_next_measurement", network_mcs->remote_record_name );

	mx_network_field_init( &(network_mcs->external_prescale_nf),
		network_mcs->server_record,
		"%s.external_prescale", network_mcs->remote_record_name );

	mx_network_field_init( &(network_mcs->manual_next_measurement_nf),
		network_mcs->server_record,
		"%s.manual_next_measurement", network_mcs->remote_record_name );

	mx_network_field_init( &(network_mcs->measurement_counts_nf),
		network_mcs->server_record,
		"%s.measurement_counts", network_mcs->remote_record_name );

	mx_network_field_init( &(network_mcs->measurement_data_nf),
		network_mcs->server_record,
		"%s.measurement_data", network_mcs->remote_record_name );

	mx_network_field_init( &(network_mcs->measurement_index_nf),
		network_mcs->server_record,
		"%s.measurement_index", network_mcs->remote_record_name );

	mx_network_field_init( &(network_mcs->last_measurement_number_nf),
		network_mcs->server_record,
		"%s.last_measurement_number", network_mcs->remote_record_name );

	mx_network_field_init( &(network_mcs->measurement_time_nf),
		network_mcs->server_record,
		"%s.measurement_time", network_mcs->remote_record_name );

	mx_network_field_init( &(network_mcs->readout_preference_nf),
		network_mcs->server_record,
		"%s.readout_preference", network_mcs->remote_record_name );

	mx_network_field_init( &(network_mcs->scaler_data_nf),
		network_mcs->server_record,
		"%s.scaler_data", network_mcs->remote_record_name );

	mx_network_field_init( &(network_mcs->scaler_index_nf),
		network_mcs->server_record,
		"%s.scaler_index", network_mcs->remote_record_name );

	mx_network_field_init( &(network_mcs->scaler_measurement_nf),
		network_mcs->server_record,
		"%s.scaler_measurement", network_mcs->remote_record_name );

	mx_network_field_init( &(network_mcs->start_nf),
		network_mcs->server_record,
		"%s.start", network_mcs->remote_record_name );

	mx_network_field_init( &(network_mcs->status_nf),
		network_mcs->server_record,
		"%s.status", network_mcs->remote_record_name );

	mx_network_field_init( &(network_mcs->stop_nf),
		network_mcs->server_record,
		"%s.stop", network_mcs->remote_record_name );

	mx_network_field_init( &(network_mcs->timer_data_nf),
		network_mcs->server_record,
		"%s.timer_data", network_mcs->remote_record_name );

	mx_network_field_init( &(network_mcs->timer_name_nf),
		network_mcs->server_record,
		"%s.timer_name", network_mcs->remote_record_name );

	mx_network_field_init( &(network_mcs->trigger_nf),
		network_mcs->server_record,
		"%s.trigger", network_mcs->remote_record_name );

	mx_network_field_init( &(network_mcs->trigger_mode_nf),
		network_mcs->server_record,
		"%s.trigger_mode", network_mcs->remote_record_name );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_mcs_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_network_mcs_print_structure()";

	MX_MCS *mcs;
	MX_NETWORK_MCS *network_mcs = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	mcs = (MX_MCS *) (record->record_class_struct);

	mx_status = mxd_network_mcs_get_pointers( mcs, &network_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "MCS parameters for record '%s':\n", record->name);

	fprintf(file, "  MCS type                  = NETWORK_MCS.\n\n");
	fprintf(file, "  server                    = %s\n",
					network_mcs->server_record->name);
	fprintf(file, "  remote record             = %s\n",
					network_mcs->remote_record_name);
	fprintf(file, "  maximum # of scalers      = %ld\n",
					mcs->maximum_num_scalers);
	fprintf(file, "  maximum # of measurements = %ld\n",
					mcs->maximum_num_measurements);

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_mcs_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_network_mcs_open()";

	MX_MCS *mcs;
	MX_NETWORK_MCS *network_mcs = NULL;
	MX_NETWORK_TIMER *network_timer;
	MX_MCS_TIMER *mcs_timer;
	MX_NETWORK_SERVER *network_server;
	MX_RECORD *current_record, *list_head_record;
	char timer_name[ MXU_RECORD_NAME_LENGTH + 1 ];
	char remote_rf_name[ MXU_RECORD_FIELD_NAME_LENGTH + 1 ];
	long dimension[1];
	int current_record_matches, is_a_timer;
	unsigned long flags;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	mcs = (MX_MCS *) (record->record_class_struct);

	mx_status = mxd_network_mcs_get_pointers( mcs, &network_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If the remote MX server is using MX 1.5.7 or higher, then ask for
	 * the readout preference of the remote network_mcs record.
	 */

	network_server = (MX_NETWORK_SERVER *)
			network_mcs->server_record->record_class_struct;

	if ( network_server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_NETWORK_SERVER pointer for the MX server record '%s' "
		"used by record '%s' is NULL.",
			network_mcs->server_record->name,
			record->name );
	}

	if ( network_server->remote_mx_version < 1005007L ) {
		mcs->readout_preference = MXF_MCS_PREFER_READ_SCALER;
	} else {
		mx_status = mx_get( &(network_mcs->readout_preference_nf),
				MXFT_LONG, &(mcs->readout_preference) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

#if 0
	MX_DEBUG(-2,("%s: network MCS '%s' remote_mx_version = %lu",
		fname, record->name, network_server->remote_mx_version));
	MX_DEBUG(-2,("%s: network MCS '%s' readout_preference = %ld",
		fname, record->name, mcs->readout_preference));
#endif

	snprintf( remote_rf_name, sizeof(remote_rf_name),
		"%s.maximum_num_scalers", network_mcs->remote_record_name );

	mx_status = mx_get_by_name( network_mcs->server_record,
				remote_rf_name, MXFT_LONG,
				&(network_mcs->remote_maximum_num_scalers) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( remote_rf_name, sizeof(remote_rf_name),
	    "%s.maximum_num_measurements", network_mcs->remote_record_name );

	mx_status = mx_get_by_name( network_mcs->server_record,
				remote_rf_name, MXFT_LONG,
			    &(network_mcs->remote_maximum_num_measurements) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If we are communicating with the remote server using a data format
	 * of MX_NETWORK_DATAFMT_RAW and if the dimensions and native datatypes
	 * of mcs->data_array in the remote record and the local record agree,
	 * then mxd_network_mcs_read_all() can just do a raw direct read of
	 * mcs->data_array from the remote server.
	 */

	flags = network_mcs->network_mcs_flags;

	network_mcs->read_raw_data_array = FALSE;

	if ( flags & MXF_NETWORK_MCS_FORCE_RAW_DATA_ARRAY ) {
		network_mcs->read_raw_data_array = TRUE;
	} else
	if ( flags & MXF_NETWORK_MCS_AVOID_RAW_DATA_ARRAY ) {
		network_mcs->read_raw_data_array = FALSE;
	} else
	if ( ( network_server->data_format == MX_NETWORK_DATAFMT_RAW )
	  && ( mcs->maximum_num_scalers ==
			network_mcs->remote_maximum_num_scalers )
	  && ( mcs->maximum_num_measurements ==
			network_mcs->remote_maximum_num_measurements ) )
	{
		network_mcs->read_raw_data_array = TRUE;
	}

	/* If the timer record has not already been found for this record,
	 * then we must go looking for it.
	 */

	if ( mcs->timer_record == (MX_RECORD *) NULL ) {

		/* Find out the name of the timer record used by the
		 * remote MX server.
		 */

		dimension[0] = MXU_RECORD_NAME_LENGTH;

		mx_status = mx_get_array(
				&(network_mcs->timer_name_nf),
				MXFT_STRING, 1, dimension,
				timer_name );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Now go looking for either a network timer record that uses
		 * the same server and the same remore_record_name or an
		 * MCS timer record that uses this record as its MCS record.
		 */

		list_head_record = record->list_head;

		current_record = list_head_record->next_record;

		while ( current_record != list_head_record ) {

		    current_record_matches = FALSE;

		    is_a_timer = mx_verify_driver_type( current_record,
				MXR_DEVICE, MXC_TIMER, MXT_ANY );

		    if ( is_a_timer ) {

			switch( current_record->mx_type ) {
			case MXT_TIM_NETWORK:
			    network_timer = (MX_NETWORK_TIMER *)
					current_record->record_type_struct;

			    if ( network_timer->server_record
					== network_mcs->server_record )
			    {
				if ( strcmp( network_timer->remote_record_name,
					timer_name ) == 0 ) {

				    /* We have a match!  The remote server and
				     * remote record names match, so this must
				     * be the right record.
				     */

				    current_record_matches = TRUE;
				}
			    }
			    break;

			case MXT_TIM_MCS:
			    mcs_timer = (MX_MCS_TIMER *)
					current_record->record_type_struct;

			    if ( mcs_timer->mcs_record == record ) {

				/* We have a match!  The MCS timer's 
				 * mcs_record pointer points to this
				 * record.
				 */

				current_record_matches = TRUE;
			    }
			    break;
			}
		    }

		    if ( current_record_matches ) {
			mcs->timer_record = current_record;

			strlcpy( mcs->timer_name,
				current_record->name,
				MXU_RECORD_NAME_LENGTH );

			break;		/* Exit the while() loop. */
		    }

		    current_record = current_record->next_record;
		}
	}

	if ( mcs->timer_record == (MX_RECORD *) NULL ) {
		mx_warning(
	"No timer has been specified for network MCS '%s'.  "
	"MCS quick scans may not work correctly if a timer is not specified "
	"in the MX database.",
			record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_network_mcs_arm( MX_MCS *mcs )
{
	static const char fname[] = "mxd_network_mcs_arm()";

	MX_NETWORK_MCS *network_mcs = NULL;
	mx_bool_type arm;
	mx_status_type mx_status;

	mx_status = mxd_network_mcs_get_pointers( mcs, &network_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	arm = TRUE;

	mx_status = mx_put( &(network_mcs->arm_nf), MXFT_BOOL, &arm );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* For certain multichannel scalers, like the 'handel_mcs' driver,
	 * arming the MCS can change the value of 'current_num_scalers',
	 * so we must fetch that value now.
	 */

	mx_status = mx_get( &(network_mcs->current_num_scalers_nf),
				MXFT_LONG, &(mcs->current_num_scalers) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_mcs_trigger( MX_MCS *mcs )
{
	static const char fname[] = "mxd_network_mcs_trigger()";

	MX_NETWORK_MCS *network_mcs = NULL;
	mx_bool_type trigger;
	mx_status_type mx_status;

	mx_status = mxd_network_mcs_get_pointers( mcs, &network_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	trigger = TRUE;

	mx_status = mx_put( &(network_mcs->trigger_nf), MXFT_BOOL, &trigger );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_mcs_stop( MX_MCS *mcs )
{
	static const char fname[] = "mxd_network_mcs_stop()";

	MX_NETWORK_MCS *network_mcs = NULL;
	mx_bool_type stop;
	mx_status_type mx_status;

	mx_status = mxd_network_mcs_get_pointers( mcs, &network_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	stop = TRUE;

	mx_status = mx_put( &(network_mcs->stop_nf), MXFT_BOOL, &stop );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_mcs_clear( MX_MCS *mcs )
{
	static const char fname[] = "mxd_network_mcs_clear()";

	MX_NETWORK_MCS *network_mcs = NULL;
	mx_bool_type clear;
	mx_status_type mx_status;

	mx_status = mxd_network_mcs_get_pointers( mcs, &network_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	clear = TRUE;

	mx_status = mx_put( &(network_mcs->clear_nf), MXFT_BOOL, &clear );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_mcs_status( MX_MCS *mcs )
{
	static const char fname[] = "mxd_network_mcs_status()";

	MX_NETWORK_MCS *network_mcs = NULL;
	MX_NETWORK_SERVER *network_server = NULL;
	mx_bool_type mcs_is_busy;
	unsigned long mcs_status;
	mx_status_type mx_status;

	mx_status = mxd_network_mcs_get_pointers( mcs, &network_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	network_server = (MX_NETWORK_SERVER *)
			network_mcs->server_record->record_class_struct;

	if ( network_server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_NETWORK_SERVER pointer for the MX server record '%s' "
		"used by record '%s' is NULL.",
			network_mcs->server_record->name,
			mcs->record->name );
	}

	if ( network_server->remote_mx_version < 2001005L ) {
		/* Before MX 2.1.5, call the old 'busy' field. */

		mx_status = mx_get( &(network_mcs->busy_nf),
						MXFT_BOOL, &mcs_is_busy );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mcs->busy = mcs_is_busy;

		if ( mcs->busy ) {
			mcs->status = MXSF_MCS_IS_BUSY;
		} else {
			mcs->status = 0;
		}
	} else {
		/* Starting with 2.1.5, call the 'status' field. */

		mx_status = mx_get( &(network_mcs->status_nf),
						MXFT_HEX, &mcs_status );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mcs->status = mcs_status;

		if ( mcs_status & MXSF_MCS_IS_BUSY ) {
			mcs->busy = TRUE;
		} else {
			mcs->busy = FALSE;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_mcs_read_all( MX_MCS *mcs )
{
	static const char fname[] = "mxd_network_mcs_read_all()";

	MX_NETWORK_MCS *network_mcs = NULL;
	long i;
	long dimension[2];
	mx_status_type mx_status;

	mx_status = mxd_network_mcs_get_pointers( mcs, &network_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( network_mcs->read_raw_data_array == FALSE ) {
		for ( i = 0; i < mcs->current_num_scalers; i++ ) {

			mcs->scaler_index = i;

			mx_status = mxd_network_mcs_read_scaler( mcs );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	} else {
		dimension[0] = network_mcs->remote_maximum_num_scalers;
		dimension[1] = network_mcs->remote_maximum_num_measurements;

		mx_status = mx_get_array( &(network_mcs->data_array_nf),
				MXFT_LONG, 2, dimension, mcs->data_array );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_mcs_read_scaler( MX_MCS *mcs )
{
	static const char fname[] = "mxd_network_mcs_read_scaler()";

	MX_NETWORK_MCS *network_mcs = NULL;
	long dimension_array[1];
	long *data_ptr;
	mx_status_type mx_status;

	mx_status = mxd_network_mcs_get_pointers( mcs, &network_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_put( &(network_mcs->scaler_index_nf),
				MXFT_LONG, &(mcs->scaler_index) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: reading '%s'.", fname,
				network_mcs->scaler_data_nf.nfname));

	data_ptr = mcs->data_array[ mcs->scaler_index ];

#if 0
	dimension_array[0] = (long) mcs->current_num_measurements;
#else
	/* !!! FIXME FIXME FIXME !!! - Transmitting the entire remote buffer
	 * if we only want a subset of the values is VERY inefficient.
	 */

	{
		MX_NETWORK_SERVER *network_server;

		network_server = (MX_NETWORK_SERVER *)
			network_mcs->server_record->record_class_struct;

		if ( network_server->data_format == MX_NETWORK_DATAFMT_XDR ) {

			/* XDR gets VERY upset if you do not read all
			 * of the values that the remote server sent.
			 */

			dimension_array[0] = (long)
					mcs->maximum_num_measurements;
		} else {
			dimension_array[0] = (long)
					mcs->current_num_measurements;
		}
	}
#endif

	mx_status = mx_get_array( &(network_mcs->scaler_data_nf),
				MXFT_LONG, 1, dimension_array, data_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: data received for '%s'.", fname,
				network_mcs->scaler_data_nf.nfname));

#if 0
	{
		long i;

		for ( i = 0; i < mcs->current_num_measurements; i++ ) {
			fprintf(stderr,"%ld ", data_ptr[i]);
		}
		fprintf(stderr,"\n");
	}
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_mcs_read_measurement( MX_MCS *mcs )
{
	static const char fname[] = "mxd_network_mcs_read_measurement()";

	MX_NETWORK_MCS *network_mcs = NULL;
	long dimension_array[1];
	mx_status_type mx_status;

	mx_status = mxd_network_mcs_get_pointers( mcs, &network_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_put( &(network_mcs->measurement_index_nf),
				MXFT_LONG, &(mcs->measurement_index) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: reading '%s'.", fname,
				network_mcs->measurement_data_nf.nfname));

	dimension_array[0] = (long) mcs->current_num_scalers;

	mx_status = mx_get_array( &(network_mcs->measurement_data_nf),
					MXFT_LONG, 1, dimension_array,
					mcs->measurement_data );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: data received for '%s'.", fname,
				network_mcs->measurement_data_nf.nfname));

#if 0
	{
		long i;

		for ( i = 0; i < mcs->current_num_scalers; i++ ) {
			fprintf(stderr,"%ld ", mcs->measurement_data[i]);
		}
		fprintf(stderr,"\n");
	}
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_mcs_read_scaler_measurement( MX_MCS *mcs )
{
	static const char fname[] = "mxd_network_mcs_read_scaler_measurement()";

	MX_NETWORK_MCS *network_mcs = NULL;
	mx_status_type mx_status;

	mx_status = mxd_network_mcs_get_pointers( mcs, &network_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_put( &(network_mcs->scaler_index_nf),
			MXFT_LONG, &(mcs->scaler_index) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_put( &(network_mcs->measurement_index_nf),
			MXFT_LONG, &(mcs->measurement_index) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get( &(network_mcs->scaler_measurement_nf),
			MXFT_LONG, &(mcs->scaler_measurement) );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_mcs_read_timer( MX_MCS *mcs )
{
	static const char fname[] = "mxd_network_mcs_read_timer()";

	MX_NETWORK_MCS *network_mcs = NULL;
	long dimension_array[1];
	mx_status_type mx_status;

	mx_status = mxd_network_mcs_get_pointers( mcs, &network_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: reading '%s'.", fname,
				network_mcs->timer_data_nf.nfname));

	dimension_array[0] = (long) mcs->current_num_measurements;

	mx_status = mx_get_array( &(network_mcs->timer_data_nf),
					MXFT_DOUBLE, 1, dimension_array,
					mcs->timer_data );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: data received for '%s'.", fname,
				network_mcs->timer_data_nf.nfname));

#if 0
	{
		long i;

		for ( i = 0; i < mcs->current_num_measurements; i++ ) {
			fprintf(stderr,"%g ", mcs->timer_data[i]);
		}
		fprintf(stderr,"\n");
	}
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_mcs_get_parameter( MX_MCS *mcs )
{
	static const char fname[] = "mxd_network_mcs_get_parameter()";

	MX_NETWORK_MCS *network_mcs = NULL;
	long counting_mode = 0;
	long trigger_mode = 0;
	mx_bool_type external_next_measurement = FALSE;
	unsigned long external_prescale = 0;
	unsigned long num_measurements, measurement_counts;
	long last_measurement_number, current_num_scalers;
	double measurement_time, dark_current;
	long dimension_array[1];
	mx_status_type mx_status;

	mx_status = mxd_network_mcs_get_pointers( mcs, &network_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for MCS '%s', type = %ld",
		fname, mcs->record->name, mcs->parameter_type));

	switch( mcs->parameter_type ) {
	case MXLV_MCS_COUNTING_MODE:
		mx_status = mx_get( &(network_mcs->counting_mode_nf),
						MXFT_LONG, &counting_mode );

		mcs->counting_mode = counting_mode;
		break;

	case MXLV_MCS_TRIGGER_MODE:
		mx_status = mx_get( &(network_mcs->trigger_mode_nf),
						MXFT_LONG, &trigger_mode );

		mcs->trigger_mode = trigger_mode;
		break;

	case MXLV_MCS_EXTERNAL_NEXT_MEASUREMENT:
		mx_status = mx_get( &(network_mcs->external_next_measurement_nf),
					MXFT_BOOL, &external_next_measurement );

		mcs->external_next_measurement = external_next_measurement;
		break;

	case MXLV_MCS_EXTERNAL_PRESCALE:
		mx_status = mx_get( &(network_mcs->external_prescale_nf),
					MXFT_ULONG, &external_prescale );

		mcs->external_prescale = external_prescale;
		break;

	case MXLV_MCS_MEASUREMENT_TIME:
		mx_status = mx_get( &(network_mcs->measurement_time_nf),
					MXFT_DOUBLE, &measurement_time );

		mcs->measurement_time = measurement_time;
		break;

	case MXLV_MCS_MEASUREMENT_COUNTS:
		mx_status = mx_get( &(network_mcs->measurement_counts_nf),
					MXFT_ULONG, &measurement_counts );

		mcs->measurement_counts = measurement_counts;
		break;

	case MXLV_MCS_CURRENT_NUM_MEASUREMENTS:
		mx_status = mx_get( &(network_mcs->current_num_measurements_nf),
					MXFT_ULONG, &num_measurements );

		mcs->current_num_measurements = num_measurements;
		break;

	case MXLV_MCS_LAST_MEASUREMENT_NUMBER:
		mx_status = mx_get( &(network_mcs->last_measurement_number_nf),
					MXFT_LONG, &last_measurement_number );

		mcs->last_measurement_number = last_measurement_number;
		break;

	case MXLV_MCS_DARK_CURRENT:
		mx_status = mx_put( &(network_mcs->scaler_index_nf),
					MXFT_LONG, &(mcs->scaler_index) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_get( &(network_mcs->dark_current_nf),
					MXFT_DOUBLE, &dark_current );

		mcs->dark_current_array[ mcs->scaler_index ] = dark_current;

		MX_DEBUG( 2,("%s: mcs->dark_current_array[%ld] = %g",
			fname, mcs->scaler_index,
			mcs->dark_current_array[mcs->scaler_index]));
		break;

	case MXLV_MCS_DARK_CURRENT_ARRAY:
		mx_status = mx_get( &(network_mcs->current_num_scalers_nf),
					MXFT_LONG, &current_num_scalers );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( current_num_scalers > mcs->maximum_num_scalers ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The value of 'current_num_scalers' (%ld) reported for "
			"network field '%s' is larger than the maximum allowed "
			"scalers (%ld) for local MCS record '%s'.",
				current_num_scalers,
				network_mcs->current_num_scalers_nf.nfname,
				mcs->maximum_num_scalers,
				mcs->record->name );
		}

		mcs->current_num_scalers = current_num_scalers;

		dimension_array[0] = current_num_scalers;

		mx_status = mx_get_array( &(network_mcs->dark_current_array_nf),
				MXFT_DOUBLE, 1, dimension_array,
				mcs->dark_current_array );
		break;

	default:
		mx_status = mx_mcs_default_get_parameter_handler( mcs );
		break;
	}

	MX_DEBUG( 2,("%s complete.", fname));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_mcs_set_parameter( MX_MCS *mcs )
{
	static const char fname[] = "mxd_network_mcs_set_parameter()";

	MX_NETWORK_MCS *network_mcs = NULL;
	long counting_mode, trigger_mode;
	mx_bool_type external_next_measurement;
	unsigned long external_prescale;
	mx_bool_type manual_next_measurement;
	unsigned long num_measurements, measurement_counts;
	double measurement_time, dark_current;
	mx_status_type mx_status;

	mx_status = mxd_network_mcs_get_pointers( mcs, &network_mcs, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for MCS '%s', type = %ld",
		fname, mcs->record->name, mcs->parameter_type));

	switch( mcs->parameter_type ) {
	case MXLV_MCS_COUNTING_MODE:
		counting_mode = mcs->counting_mode;

		mx_status = mx_put( &(network_mcs->counting_mode_nf),
						MXFT_LONG, &counting_mode );
		break;

	case MXLV_MCS_TRIGGER_MODE:
		trigger_mode = mcs->trigger_mode;

		mx_status = mx_put( &(network_mcs->trigger_mode_nf),
						MXFT_LONG, &trigger_mode );
		break;

	case MXLV_MCS_EXTERNAL_NEXT_MEASUREMENT:
		external_next_measurement = mcs->external_next_measurement;

		MX_DEBUG( 2,("%s: sending %d to '%s'",
				fname, (int) external_next_measurement,
			network_mcs->external_next_measurement_nf.nfname ));

		mx_status = mx_put(&(network_mcs->external_next_measurement_nf),
					MXFT_BOOL, &external_next_measurement );
		break;

	case MXLV_MCS_EXTERNAL_PRESCALE:
		external_prescale = mcs->external_prescale;

		MX_DEBUG( 2,("%s: sending %lu to '%s'",
				fname, external_prescale,
				network_mcs->external_prescale_nf.nfname ));

		mx_status = mx_put( &(network_mcs->external_prescale_nf),
					MXFT_ULONG, &external_prescale );
		break;

	case MXLV_MCS_MANUAL_NEXT_MEASUREMENT:
		manual_next_measurement = TRUE;

		MX_DEBUG( 2,("%s: sending %d to '%s'",
			fname, (int) manual_next_measurement,
			network_mcs->manual_next_measurement_nf.nfname ));

		mx_status = mx_put( &(network_mcs->manual_next_measurement_nf),
					MXFT_BOOL, &manual_next_measurement );

		mcs->manual_next_measurement = FALSE;
		break;

	case MXLV_MCS_MEASUREMENT_TIME:
		measurement_time = mcs->measurement_time;

		MX_DEBUG( 2,("%s: sending %g to '%s'",
				fname, measurement_time,
				network_mcs->measurement_time_nf.nfname ));

		mx_status = mx_put( &(network_mcs->measurement_time_nf),
					MXFT_DOUBLE, &measurement_time );
		break;

	case MXLV_MCS_MEASUREMENT_COUNTS:
		measurement_counts = mcs->measurement_counts;

		MX_DEBUG( 2,("%s: sending %lu to '%s'",
				fname, measurement_counts,
				network_mcs->measurement_counts_nf.nfname ));

		mx_status = mx_put( &(network_mcs->measurement_counts_nf),
					MXFT_ULONG, &measurement_counts );
		break;

	case MXLV_MCS_CURRENT_NUM_MEASUREMENTS:
		num_measurements = mcs->current_num_measurements;

		MX_DEBUG( 2,("%s: sending %lu to '%s'",
			fname, num_measurements,
			network_mcs->current_num_measurements_nf.nfname ));

		mx_status = mx_put( &(network_mcs->current_num_measurements_nf),
					MXFT_ULONG, &num_measurements );
		break;

	case MXLV_MCS_DARK_CURRENT:
		mx_status = mx_put( &(network_mcs->scaler_index_nf),
					MXFT_LONG, &(mcs->scaler_index) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		dark_current = mcs->dark_current_array[ mcs->scaler_index ];

		MX_DEBUG( 2,("%s: mcs->dark_current_array[%ld] = %g",
			fname, mcs->scaler_index,
			mcs->dark_current_array[mcs->scaler_index]));

		mx_status = mx_put( &(network_mcs->dark_current_nf),
					MXFT_DOUBLE, &dark_current );
		break;

	default:
		mx_status = mx_mcs_default_set_parameter_handler( mcs );
		break;
	}

	MX_DEBUG( 2,("%s complete.", fname));

	return mx_status;
}

