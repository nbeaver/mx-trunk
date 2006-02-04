/*
 * Name:    d_auto_net.c
 *
 * Purpose: Driver for control of an autoscale record in a network server.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2001, 2003-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_net.h"
#include "mx_autoscale.h"
#include "d_auto_net.h"

MX_RECORD_FUNCTION_LIST mxd_auto_network_record_function_list = {
	NULL,
	mxd_auto_network_create_record_structures,
	mxd_auto_network_finish_record_initialization,
	mxd_auto_network_delete_record,
	NULL,
	mxd_auto_network_dummy_function,
	mxd_auto_network_dummy_function,
	mxd_auto_network_open,
	mxd_auto_network_close
};

MX_AUTOSCALE_FUNCTION_LIST mxd_auto_network_autoscale_function_list = {
	mxd_auto_network_read_monitor,
	mxd_auto_network_get_change_request,
	mxd_auto_network_change_control,
	mxd_auto_network_get_offset_index,
	mxd_auto_network_set_offset_index,
	mxd_auto_network_get_parameter,
	mxd_auto_network_set_parameter
};

MX_RECORD_FIELD_DEFAULTS mxd_auto_network_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AUTOSCALE_STANDARD_FIELDS,
	MX_AUTO_NETWORK_STANDARD_FIELDS
};

mx_length_type mxd_auto_network_num_record_fields
	= sizeof( mxd_auto_network_record_field_defaults )
	/ sizeof( mxd_auto_network_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_auto_network_rfield_def_ptr
		= &mxd_auto_network_record_field_defaults[0];

static mx_status_type
mxd_auto_network_get_pointers( MX_AUTOSCALE *autoscale,
			MX_AUTO_NETWORK **auto_network,
			const char *calling_fname )
{
	static const char fname[] = "mxd_auto_network_get_pointers()";

	if ( autoscale == (MX_AUTOSCALE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AUTOSCALE pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( auto_network == (MX_AUTO_NETWORK **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AUTO_NETWORK pointer passed was NULL." );
	}
	if ( autoscale->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
  "The MX_RECORD pointer for the MX_AUTOSCALE structure passed is NULL." );
	}
	if ( autoscale->record->mx_class != MXC_AUTOSCALE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"Record '%s' passed is not an autoscale record.",
			autoscale->record->name );
	}

	*auto_network = (MX_AUTO_NETWORK *)
				autoscale->record->record_type_struct;

	if ( *auto_network == (MX_AUTO_NETWORK *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_AUTO_NETWORK pointer for record '%s' is NULL.",
				autoscale->record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/*****/

MX_EXPORT mx_status_type
mxd_auto_network_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_auto_network_create_record_structures()";

	MX_AUTOSCALE *autoscale;
	MX_AUTO_NETWORK *auto_network;

	/* Allocate memory for the necessary structures. */

	autoscale = (MX_AUTOSCALE *) malloc( sizeof(MX_AUTOSCALE) );

	if ( autoscale == (MX_AUTOSCALE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_AUTOSCALE structure." );
	}

	auto_network = (MX_AUTO_NETWORK *)
					malloc( sizeof(MX_AUTO_NETWORK) );

	if ( auto_network == (MX_AUTO_NETWORK *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_AUTO_NETWORK structure." );
	}

	/* Now set up the necessary pointers. */

	autoscale->record = record;

	record->record_superclass_struct = NULL;
	record->record_class_struct = autoscale;
	record->record_type_struct = auto_network;

	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list =
			&mxd_auto_network_autoscale_function_list;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_auto_network_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[]
		= "mxd_auto_network_finish_record_initialization()";

	MX_AUTOSCALE *autoscale;
	MX_AUTO_NETWORK *auto_network;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	autoscale = (MX_AUTOSCALE *) record->record_class_struct;

	mx_status = mxd_auto_network_get_pointers( autoscale,
						&auto_network, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The record types of the monitor, control, and timer records
	 * for this record do not matter since they are not used.
	 * Instead, the read monitor and update control commands
	 * are forwarded across the network.
	 */

	/* Initialize record values. */

	autoscale->monitor_value = 0L;

	autoscale->num_monitor_offsets = 0L;
	autoscale->monitor_offset_array = NULL;

	autoscale->last_limit_tripped = MXF_AUTO_NO_LIMIT_TRIPPED;

	/* Set up MX network field structures. */

	mx_network_field_init( &(auto_network->change_control_nf),
		auto_network->server_record,
		"%s.change_control", auto_network->remote_record_name );

	mx_network_field_init( &(auto_network->enabled_nf),
		auto_network->server_record,
		"%s.enabled", auto_network->remote_record_name );

	mx_network_field_init( &(auto_network->get_change_request_nf),
		auto_network->server_record,
		"%s.get_change_request", auto_network->remote_record_name );

	mx_network_field_init( &(auto_network->high_deadband_nf),
		auto_network->server_record,
		"%s.high_deadband", auto_network->remote_record_name );

	mx_network_field_init( &(auto_network->high_limit_nf),
		auto_network->server_record,
		"%s.high_limit", auto_network->remote_record_name );

	mx_network_field_init( &(auto_network->low_deadband_nf),
		auto_network->server_record,
		"%s.low_deadband", auto_network->remote_record_name );

	mx_network_field_init( &(auto_network->low_limit_nf),
		auto_network->server_record,
		"%s.low_limit", auto_network->remote_record_name );

	mx_network_field_init( &(auto_network->monitor_offset_array_nf),
		auto_network->server_record,
		"%s.monitor_offset_array", auto_network->remote_record_name );

	mx_network_field_init( &(auto_network->monitor_offset_index_nf),
		auto_network->server_record,
		"%s.monitor_offset_index", auto_network->remote_record_name );

	mx_network_field_init( &(auto_network->monitor_value_nf),
		auto_network->server_record,
		"%s.monitor_value", auto_network->remote_record_name );

	mx_network_field_init( &(auto_network->num_monitor_offsets_nf),
		auto_network->server_record,
		"%s.num_monitor_offsets", auto_network->remote_record_name );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_auto_network_delete_record( MX_RECORD *record )
{
	MX_AUTOSCALE *autoscale;

	if ( record == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}
	if ( record->record_type_struct != NULL ) {
		free( record->record_type_struct );

		record->record_type_struct = NULL;
	}

	autoscale = (MX_AUTOSCALE *) record->record_class_struct;

	if ( autoscale != NULL ) {

		if ( autoscale->monitor_offset_array != NULL ) {
			free( autoscale->monitor_offset_array );

			autoscale->monitor_offset_array = NULL;
		}

		free( record->record_type_struct );

		record->record_type_struct = NULL;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_auto_network_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_auto_network_open()";

	MX_AUTOSCALE *autoscale;
	MX_AUTO_NETWORK *auto_network;
	uint32_t num_monitor_offsets;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	MX_DEBUG( 2,("%s invoked for record '%s'", fname, record->name));

	autoscale = (MX_AUTOSCALE *) record->record_class_struct;

	mx_status = mxd_auto_network_get_pointers( autoscale,
						&auto_network, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Find out how big the monitor offset array is in the remote server. */

	mx_status = mx_get( &(auto_network->num_monitor_offsets_nf),
				MXFT_UINT32, &num_monitor_offsets );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	autoscale->num_monitor_offsets = num_monitor_offsets;

	/* Allocate a local array to contain a copy of the remote monitor
	 * offset array.
	 */

	mx_status = mx_autoscale_create_monitor_offset_array( autoscale );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize the contents of the local array to the values in
	 * the remote array.
	 */

	mx_status = mx_autoscale_get_offset_array( record, NULL, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0
	{
		unsigned long i;
		for ( i = 0; i < autoscale->num_monitor_offsets; i++ ) {
			MX_DEBUG( 2,
			  ("%s: autoscale->monitor_offset_array[%lu] = %g",
				fname, i, autoscale->monitor_offset_array[i]));
		}
	}
#endif

	MX_DEBUG( 2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_auto_network_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_auto_network_close()";

	MX_AUTOSCALE *autoscale;
	MX_AUTO_NETWORK *auto_network;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	autoscale = (MX_AUTOSCALE *) record->record_class_struct;

	mx_status = mxd_auto_network_get_pointers( autoscale,
						&auto_network, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( autoscale->monitor_offset_array != (double *) NULL ) {

		free( autoscale->monitor_offset_array );

		autoscale->monitor_offset_array = NULL;
	}

	autoscale->num_monitor_offsets = 0L;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_auto_network_dummy_function( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_auto_network_read_monitor( MX_AUTOSCALE *autoscale )
{
	static const char fname[] = "mxd_auto_network_read_monitor()";

	MX_AUTO_NETWORK *auto_network;
	int32_t scaler_value;
	mx_status_type mx_status;

#if 0
	MX_CLOCK_TICK clock_ticks_before, clock_ticks_after, clock_ticks_diff;
	double seconds_diff;
#endif

	mx_status = mxd_auto_network_get_pointers( autoscale,
						&auto_network, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* We need to check as to how long this network request takes. */

#if 0
	clock_ticks_before = mx_current_clock_tick();
#endif

	mx_status = mx_get( &(auto_network->monitor_value_nf),
				MXFT_INT32, &scaler_value );

#if 0
	clock_ticks_after = mx_current_clock_tick();

	clock_ticks_diff = mx_subtract_clock_ticks( clock_ticks_after,
							clock_ticks_before );

	seconds_diff = mx_convert_clock_ticks_to_seconds( clock_ticks_diff );

	MX_DEBUG( 2,("%s: clock_ticks_before = (%lu, %ld)", fname,
			(unsigned long) clock_ticks_before.high_order,
			(long) clock_ticks_before.low_order));

	MX_DEBUG( 2,("%s: clock_ticks_after = (%lu, %ld)", fname,
			(unsigned long) clock_ticks_after.high_order,
			(long) clock_ticks_after.low_order));

	MX_DEBUG( 2,("%s: mx_get took %ld clock ticks ( %g seconds )",
		fname, (long) clock_ticks_diff.low_order, seconds_diff));
#endif
	autoscale->monitor_value = scaler_value;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_auto_network_get_change_request( MX_AUTOSCALE *autoscale )
{
	static const char fname[] = "mxd_auto_network_get_change_request()";

	MX_AUTO_NETWORK *auto_network;
	int32_t get_change_request;
	mx_status_type mx_status;

	mx_status = mxd_auto_network_get_pointers( autoscale,
						&auto_network, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get( &(auto_network->get_change_request_nf),
				MXFT_INT32, &get_change_request );

	autoscale->get_change_request = get_change_request;

	MX_DEBUG( 2,("%s: control = '%s', get_change_request = %d",
			fname, autoscale->control_record->name,
			autoscale->get_change_request));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_auto_network_change_control( MX_AUTOSCALE *autoscale )
{
	static const char fname[] = "mxd_auto_network_change_control()";

	MX_AUTO_NETWORK *auto_network;
	int32_t change_control;
	mx_status_type mx_status;

	mx_status = mxd_auto_network_get_pointers( autoscale,
						&auto_network, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	change_control = autoscale->change_control;

	MX_DEBUG( 2,("%s: control = '%s', change_control = %d",
			fname, autoscale->control_record->name,
			change_control));

	mx_status = mx_put( &(auto_network->change_control_nf),
				MXFT_INT32, &change_control );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_auto_network_get_offset_index( MX_AUTOSCALE *autoscale )
{
	static const char fname[] = "mxd_auto_network_get_offset_index()";

	MX_AUTO_NETWORK *auto_network;
	uint32_t offset_index;
	mx_status_type mx_status;

	mx_status = mxd_auto_network_get_pointers( autoscale,
						&auto_network, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get( &(auto_network->monitor_offset_index_nf),
				MXFT_UINT32, &offset_index );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( offset_index >= autoscale->num_monitor_offsets ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"The monitor offset value %lu returned by the remote server is larger "
	"than the maximum value %lu allowed by the client MX database.",
		(unsigned long) offset_index,
		autoscale->num_monitor_offsets - 1L );
	}

	autoscale->monitor_offset_index = offset_index;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_auto_network_set_offset_index( MX_AUTOSCALE *autoscale )
{
	static const char fname[] = "mxd_auto_network_set_offset_index()";

	MX_AUTO_NETWORK *auto_network;
	uint32_t offset_index;
	mx_status_type mx_status;

	mx_status = mxd_auto_network_get_pointers( autoscale,
						&auto_network, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	offset_index = autoscale->monitor_offset_index;

	if ( offset_index >= autoscale->num_monitor_offsets ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested offset_index has an "
		"illegal_value of %lu.  The allowed values are (0 - %lu)",
			(unsigned long) offset_index,
			autoscale->num_monitor_offsets - 1L );
	}

	mx_status = mx_put( &(auto_network->monitor_offset_index_nf),
					MXFT_UINT32, &offset_index );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_auto_network_get_parameter( MX_AUTOSCALE *autoscale )
{
	static const char fname[] = "mxd_auto_network_get_parameter()";

	MX_AUTO_NETWORK *auto_network;
	mx_length_type dimension_array[1];
	int32_t int32_value;
	double double_value;
	mx_status_type mx_status;

	mx_status = mxd_auto_network_get_pointers( autoscale,
						&auto_network, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,(
	"%s invoked for autoscale '%s' for parameter type '%s' (%d).",
		fname, autoscale->record->name,
		mx_get_field_label_string( autoscale->record,
			autoscale->parameter_type ),
		autoscale->parameter_type));

	switch( autoscale->parameter_type ) {
	case MXLV_AUT_ENABLED:
		mx_status = mx_get( &(auto_network->enabled_nf),
					MXFT_INT32, &int32_value );

		autoscale->enabled = int32_value;
		break;

	case MXLV_AUT_LOW_LIMIT:
		mx_status = mx_get( &(auto_network->low_limit_nf),
					MXFT_DOUBLE, &double_value );

		autoscale->low_limit = double_value;
		break;

	case MXLV_AUT_HIGH_LIMIT:
		mx_status = mx_get( &(auto_network->high_limit_nf),
					MXFT_DOUBLE, &double_value );

		autoscale->high_limit = double_value;
		break;

	case MXLV_AUT_LOW_DEADBAND:
		mx_status = mx_get( &(auto_network->low_deadband_nf),
					MXFT_DOUBLE, &double_value );

		autoscale->low_deadband = double_value;
		break;

	case MXLV_AUT_HIGH_DEADBAND:
		mx_status = mx_get( &(auto_network->high_deadband_nf),
					MXFT_DOUBLE, &double_value );

		autoscale->high_deadband = double_value;
		break;

	case MXLV_AUT_MONITOR_OFFSET_ARRAY:
		dimension_array[0] = (long) autoscale->num_monitor_offsets;

		mx_status = mx_get_array(
				&(auto_network->monitor_offset_array_nf),
				MXFT_DOUBLE, 1, dimension_array,
				autoscale->monitor_offset_array );
		break;

	default:
		return mx_autoscale_default_get_parameter_handler( autoscale );
		break;
	}
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_auto_network_set_parameter( MX_AUTOSCALE *autoscale )
{
	static const char fname[] = "mxd_auto_network_set_parameter()";

	MX_AUTO_NETWORK *auto_network;
	mx_length_type dimension_array[1];
	int32_t int32_value;
	double double_value;
	mx_status_type mx_status;

	mx_status = mxd_auto_network_get_pointers( autoscale,
						&auto_network, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,(
	"%s invoked for autoscale '%s' for parameter type '%s' (%d).",
		fname, autoscale->record->name,
		mx_get_field_label_string( autoscale->record,
			autoscale->parameter_type ),
		autoscale->parameter_type));

	switch( autoscale->parameter_type ) {
	case MXLV_AUT_ENABLED:
		int32_value = autoscale->enabled;

		mx_status = mx_put( &(auto_network->enabled_nf),
					MXFT_INT32, &int32_value );
		break;

	case MXLV_AUT_LOW_LIMIT:
		double_value = autoscale->low_limit;

		mx_status = mx_put( &(auto_network->low_limit_nf),
					MXFT_DOUBLE, &double_value );
		break;

	case MXLV_AUT_HIGH_LIMIT:
		double_value = autoscale->high_limit;

		mx_status = mx_put( &(auto_network->high_limit_nf),
					MXFT_DOUBLE, &double_value );
		break;

	case MXLV_AUT_LOW_DEADBAND:
		double_value = autoscale->low_deadband;

		mx_status = mx_put( &(auto_network->low_deadband_nf),
					MXFT_DOUBLE, &double_value );
		break;

	case MXLV_AUT_HIGH_DEADBAND:
		double_value = autoscale->high_deadband;

		mx_status = mx_put( &(auto_network->high_deadband_nf),
					MXFT_DOUBLE, &double_value );
		break;

	case MXLV_AUT_MONITOR_OFFSET_ARRAY:
		dimension_array[0] = (long) autoscale->num_monitor_offsets;

		mx_status = mx_put_array(
				&(auto_network->monitor_offset_array_nf),
				MXFT_DOUBLE, 1, dimension_array,
				autoscale->monitor_offset_array );
		break;

	default:
		return mx_autoscale_default_set_parameter_handler( autoscale );
		break;
	}

	return mx_status;
}

