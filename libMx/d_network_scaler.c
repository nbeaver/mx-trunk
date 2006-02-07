/*
 * Name:    d_network_scaler.c
 *
 * Purpose: MX scaler driver to control MX network scalers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_net.h"
#include "mx_scaler.h"
#include "d_network_scaler.h"

/* Initialize the scaler driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_network_scaler_record_function_list = {
	NULL,
	mxd_network_scaler_create_record_structures,
	mxd_network_scaler_finish_record_initialization
};

MX_SCALER_FUNCTION_LIST mxd_network_scaler_scaler_function_list = {
	mxd_network_scaler_clear,
	mxd_network_scaler_overflow_set,
	mxd_network_scaler_read,
	mxd_network_scaler_read_raw,
	mxd_network_scaler_is_busy,
	mxd_network_scaler_start,
	mxd_network_scaler_stop,
	mxd_network_scaler_get_parameter,
	mxd_network_scaler_set_parameter
};

/* MX network scaler data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_network_scaler_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SCALER_STANDARD_FIELDS,
	MXD_NETWORK_SCALER_STANDARD_FIELDS
};

mx_length_type mxd_network_scaler_num_record_fields
		= sizeof( mxd_network_scaler_record_field_defaults )
		  / sizeof( mxd_network_scaler_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_network_scaler_rfield_def_ptr
			= &mxd_network_scaler_record_field_defaults[0];

/*=======================================================================*/

static mx_status_type
mxd_network_scaler_get_pointers( MX_SCALER *scaler,
			MX_NETWORK_SCALER **network_scaler,
			const char *calling_fname )
{
	static const char fname[] = "mxd_network_scaler_get_pointers()";

	if ( scaler == (MX_SCALER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_SCALER pointer passed by '%s' was NULL",
			calling_fname );
	}
	if ( network_scaler == (MX_NETWORK_SCALER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_NETWORK_SCALER pointer passed by '%s' was NULL",
			calling_fname );
	}

	*network_scaler = (MX_NETWORK_SCALER *)
				scaler->record->record_type_struct;

	if ( *network_scaler == (MX_NETWORK_SCALER *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_NETWORK_SCALER pointer for scaler record '%s' passed by '%s' is NULL",
				scaler->record->name, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_network_scaler_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_network_scaler_create_record_structures()";

	MX_SCALER *scaler;
	MX_NETWORK_SCALER *network_scaler;

	/* Allocate memory for the necessary structures. */

	scaler = (MX_SCALER *) malloc( sizeof(MX_SCALER) );

	if ( scaler == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SCALER structure." );
	}

	network_scaler = (MX_NETWORK_SCALER *)
				malloc( sizeof(MX_NETWORK_SCALER) );

	if ( network_scaler == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_NETWORK_SCALER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = scaler;
	record->record_type_struct = network_scaler;
	record->class_specific_function_list
			= &mxd_network_scaler_scaler_function_list;

	scaler->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_scaler_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_network_scaler_finish_record_initialization()";

	MX_SCALER *scaler;
	MX_NETWORK_SCALER *network_scaler;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	scaler = (MX_SCALER *) record->record_class_struct;

	mx_status = mxd_network_scaler_get_pointers(
				scaler, &network_scaler, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_scaler_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_network_field_init( &(network_scaler->busy_nf),
		network_scaler->server_record,
		"%s.busy", network_scaler->remote_record_name );

	mx_network_field_init( &(network_scaler->clear_nf),
		network_scaler->server_record,
		"%s.clear", network_scaler->remote_record_name );

	mx_network_field_init( &(network_scaler->dark_current_nf),
		network_scaler->server_record,
		"%s.dark_current", network_scaler->remote_record_name );

	mx_network_field_init( &(network_scaler->mode_nf),
		network_scaler->server_record,
		"%s.mode", network_scaler->remote_record_name );

	mx_network_field_init( &(network_scaler->overflow_set_nf),
		network_scaler->server_record,
		"%s.overflow_set", network_scaler->remote_record_name );

	mx_network_field_init( &(network_scaler->raw_value_nf),
		network_scaler->server_record,
		"%s.raw_value", network_scaler->remote_record_name );

	mx_network_field_init( &(network_scaler->stop_nf),
		network_scaler->server_record,
		"%s.stop", network_scaler->remote_record_name );

	mx_network_field_init( &(network_scaler->value_nf),
		network_scaler->server_record,
		"%s.value", network_scaler->remote_record_name );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_scaler_clear( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_network_scaler_clear()";

	MX_NETWORK_SCALER *network_scaler;
	mx_bool_type clear;
	mx_status_type mx_status;

	mx_status = mxd_network_scaler_get_pointers(
				scaler, &network_scaler, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	clear = TRUE;

	mx_status = mx_put( &(network_scaler->clear_nf), MXFT_BOOL, &clear );

	scaler->raw_value = 0L;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_scaler_overflow_set( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_network_scaler_overflow_set()";

	MX_NETWORK_SCALER *network_scaler;
	mx_bool_type overflow_set;
	mx_status_type mx_status;

	mx_status = mxd_network_scaler_get_pointers(
				scaler, &network_scaler, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get( &(network_scaler->overflow_set_nf),
				MXFT_BOOL, &overflow_set );

	scaler->overflow_set = overflow_set;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_scaler_read( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_network_scaler_read()";

	MX_NETWORK_SCALER *network_scaler;
	int32_t value;
	mx_status_type mx_status;

	mx_status = mxd_network_scaler_get_pointers(
				scaler, &network_scaler, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get( &(network_scaler->value_nf), MXFT_INT32, &value );

	scaler->raw_value = value;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_scaler_read_raw( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_network_scaler_read_raw()";

	MX_NETWORK_SCALER *network_scaler;
	int32_t raw_value;
	mx_status_type mx_status;

	mx_status = mxd_network_scaler_get_pointers(
				scaler, &network_scaler, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get( &(network_scaler->raw_value_nf),
				MXFT_INT32, &raw_value );

	scaler->raw_value = raw_value;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_scaler_is_busy( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_network_scaler_is_busy()";

	MX_NETWORK_SCALER *network_scaler;
	mx_bool_type busy;
	mx_status_type mx_status;

	mx_status = mxd_network_scaler_get_pointers(
				scaler, &network_scaler, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get( &(network_scaler->busy_nf), MXFT_BOOL, &busy );

	scaler->busy = busy;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_scaler_start( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_network_scaler_start()";

	MX_NETWORK_SCALER *network_scaler;
	int32_t value;
	mx_status_type mx_status;

	mx_status = mxd_network_scaler_get_pointers(
				scaler, &network_scaler, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	value = scaler->raw_value;

	mx_status = mx_put( &(network_scaler->value_nf), MXFT_INT32, &value );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_scaler_stop( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_network_scaler_stop()";

	MX_NETWORK_SCALER *network_scaler;
	mx_bool_type stop;
	int32_t value;
	mx_status_type mx_status;

	mx_status = mxd_network_scaler_get_pointers(
				scaler, &network_scaler, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	stop = TRUE;

	mx_status = mx_put( &(network_scaler->stop_nf), MXFT_BOOL, &stop );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get( &(network_scaler->value_nf), MXFT_INT32, &value );

	scaler->raw_value = value;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_scaler_get_parameter( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_network_scaler_get_parameter()";

	MX_NETWORK_SCALER *network_scaler;
	int32_t mode;
	double dark_current;
	mx_status_type mx_status;

	mx_status = mxd_network_scaler_get_pointers(
				scaler, &network_scaler, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( scaler->parameter_type ) {
	case MXLV_SCL_MODE:
		mx_status = mx_get( &(network_scaler->mode_nf),
					MXFT_INT32, &mode );
		scaler->mode = mode;
		break;
	case MXLV_SCL_DARK_CURRENT:
		/* If the database is configured such that the server
		 * maintains the dark current rather than the client, then
		 * ask the server for the value.  Otherwise, just return
		 * our locally cached value.
		 */

		if ( scaler->scaler_flags
			& MXF_SCL_SERVER_SUBTRACTS_DARK_CURRENT )
		{
			mx_status = mx_get( &(network_scaler->dark_current_nf),
						MXFT_DOUBLE, &dark_current );

			scaler->dark_current = dark_current;
		}
		break;
	default:
		mx_status = mx_scaler_default_get_parameter_handler( scaler );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_scaler_set_parameter( MX_SCALER *scaler )
{
	static const char fname[] = "mxd_network_scaler_set_parameter()";

	MX_NETWORK_SCALER *network_scaler;
	int32_t mode;
	double dark_current;
	mx_status_type mx_status;

	mx_status = mxd_network_scaler_get_pointers(
				scaler, &network_scaler, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( scaler->parameter_type ) {
	case MXLV_SCL_MODE:
		mode = scaler->mode;

		mx_status = mx_put( &(network_scaler->mode_nf),
					MXFT_INT32, &mode );
		break;
	case MXLV_SCL_DARK_CURRENT:
		/* If the database is configured such that the server
		 * maintains the dark current rather than the client, then
		 * send the value to the server.  Otherwise, just cache
		 * it locally.
		 */

		if ( scaler->scaler_flags
			& MXF_SCL_SERVER_SUBTRACTS_DARK_CURRENT )
		{
			dark_current = scaler->dark_current;

			mx_status = mx_put( &(network_scaler->dark_current_nf),
						MXFT_DOUBLE, &dark_current );
		}
		break;
	default:
		mx_status = mx_scaler_default_set_parameter_handler( scaler );
		break;
	}

	return mx_status;
}

