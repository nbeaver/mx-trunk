/*
 * Name:    d_network_ptz.c
 *
 * Purpose: MX network Pan/Tilt/Zoom driver.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_NETWORK_PTZ_DEBUG	TRUE

#include <stdio.h>

#include "mx_record.h"
#include "mx_net.h"
#include "mx_ptz.h"
#include "d_network_ptz.h"

MX_RECORD_FUNCTION_LIST mxd_network_ptz_record_function_list = {
	NULL,
	mxd_network_ptz_create_record_structures,
	mxd_network_ptz_finish_record_initialization
};

MX_PAN_TILT_ZOOM_FUNCTION_LIST mxd_network_ptz_ptz_function_list = {
	mxd_network_ptz_command,
	mxd_network_ptz_get_status,
	mxd_network_ptz_get_parameter,
	mxd_network_ptz_set_parameter
};

MX_RECORD_FIELD_DEFAULTS mxd_network_ptz_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_PAN_TILT_ZOOM_STANDARD_FIELDS,
	MXD_NETWORK_PTZ_STANDARD_FIELDS
};

long mxd_network_ptz_num_record_fields
	= sizeof( mxd_network_ptz_rf_defaults )
		/ sizeof( mxd_network_ptz_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_network_ptz_rfield_def_ptr
			= &mxd_network_ptz_rf_defaults[0];

/* ===== */

static mx_status_type
mxd_network_ptz_get_pointers( MX_PAN_TILT_ZOOM *ptz,
			MX_NETWORK_PTZ **network_ptz,
			const char *calling_fname )
{
	static const char fname[] = "mxd_network_ptz_get_pointers()";

	if ( ptz == (MX_PAN_TILT_ZOOM *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PAN_TILT_ZOOM pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( network_ptz ==  (MX_NETWORK_PTZ **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NETWORK_PTZ pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( ptz->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for MX_PTZ %p is NULL.", ptz );
	}

	*network_ptz = (MX_NETWORK_PTZ *) ptz->record->record_type_struct;

	if ( (*network_ptz) == (MX_NETWORK_PTZ *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_NETWORK_PTZ pointer for record '%s' is NULL.",
			ptz->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxd_network_ptz_create_record_structures( MX_RECORD *record )
{
        static const char fname[] =
			"mxd_network_ptz_create_record_structures()";

        MX_PAN_TILT_ZOOM *ptz;
        MX_NETWORK_PTZ *network_ptz;

        /* Allocate memory for the necessary structures. */

        ptz = (MX_PAN_TILT_ZOOM *) malloc(sizeof(MX_PAN_TILT_ZOOM));

        if ( ptz == (MX_PAN_TILT_ZOOM *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
               "Unable to allocate memory for an MX_PAN_TILT_ZOOM structure." );
        }

        network_ptz = (MX_NETWORK_PTZ *)
				malloc( sizeof(MX_NETWORK_PTZ) );

        if ( network_ptz == (MX_NETWORK_PTZ *) NULL ) {
                return mx_error( MXE_OUT_OF_MEMORY, fname,
	    "Unable to allocate memory for an MX_NETWORK_PTZ structure." );
        }

        /* Now set up the necessary pointers. */

        record->record_class_struct = ptz;
        record->record_type_struct = network_ptz;
        record->class_specific_function_list =
				&mxd_network_ptz_ptz_function_list;

        ptz->record = record;
	network_ptz->record = record;

        return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_ptz_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_network_ptz_finish_record_initialization()";

	MX_PAN_TILT_ZOOM *ptz;
	MX_NETWORK_PTZ *network_ptz;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	ptz = (MX_PAN_TILT_ZOOM *) record->record_class_struct;

	mx_status = mxd_network_ptz_get_pointers( ptz, &network_ptz, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_PTZ_DEBUG
	MX_DEBUG(-2,("%s invoked for PTZ '%s'.", fname, record->name));
#endif

	mx_network_field_init( &(network_ptz->zoom_position_nf),
		network_ptz->server_record,
		"%s.zoom_position", network_ptz->remote_record_name );

	mx_network_field_init( &(network_ptz->zoom_destination_nf),
		network_ptz->server_record,
		"%s.zoom_destination", network_ptz->remote_record_name );

	mx_network_field_init( &(network_ptz->zoom_command_nf),
		network_ptz->server_record,
		"%s.zoom_command", network_ptz->remote_record_name );

	mx_network_field_init( &(network_ptz->zoom_on_nf),
		network_ptz->server_record,
		"%s.zoom_on", network_ptz->remote_record_name );

	return MX_SUCCESSFUL_RESULT;;
}

MX_EXPORT mx_status_type
mxd_network_ptz_command( MX_PAN_TILT_ZOOM *ptz )
{
	static const char fname[] = "mxd_network_ptz_command()";

	MX_NETWORK_PTZ *network_ptz;
	int int_value;
	mx_status_type mx_status;

	mx_status = mxd_network_ptz_get_pointers( ptz, &network_ptz, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_PTZ_DEBUG
	MX_DEBUG(-2,("%s invoked for PTZ '%s' for command type %#lx.",
		fname, ptz->record->name, ptz->command));
#endif

	switch( ptz->command ) {
	case MXF_PTZ_ZOOM_IN:
	case MXF_PTZ_ZOOM_OUT:
	case MXF_PTZ_ZOOM_STOP:
		int_value = ptz->command;

		MX_DEBUG(-2,("%s: int_value = %#x", fname, int_value));

		mx_status = mx_put( &(network_ptz->zoom_command_nf),
					MXFT_INT, &int_value );
		break;
	case MXF_PTZ_ZOOM_OFF:
	case MXF_PTZ_ZOOM_ON:
		int_value = ptz->zoom_on;

		MX_DEBUG(-2,("%s: int_value = %d", fname, int_value));

		mx_status = mx_put( &(network_ptz->zoom_on_nf),
					MXFT_INT, &(ptz->zoom_on) );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"The command %#lx received for soft PTZ '%s' "
			"is not a known command type.",
				ptz->command, network_ptz->record->name );
		break;
	}

	return MX_SUCCESSFUL_RESULT;;
}

MX_EXPORT mx_status_type
mxd_network_ptz_get_status( MX_PAN_TILT_ZOOM *ptz )
{
	static const char fname[] = "mxd_network_ptz_get_status()";

	MX_NETWORK_PTZ *network_ptz;
	mx_status_type mx_status;

	mx_status = mxd_network_ptz_get_pointers( ptz, &network_ptz, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_PTZ_DEBUG
	MX_DEBUG(-2,("%s: invoked for PTZ '%s'", fname, ptz->record->name));
#endif

	ptz->status = 0;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_ptz_get_parameter( MX_PAN_TILT_ZOOM *ptz )
{
	static const char fname[] = "mxd_network_ptz_get_parameter()";

	MX_NETWORK_PTZ *network_ptz;
	mx_status_type mx_status;

	mx_status = mxd_network_ptz_get_pointers( ptz, &network_ptz, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_PTZ_DEBUG
	MX_DEBUG(-2,("%s invoked for PTZ '%s' for command type %#x.",
		fname, ptz->record->name, ptz->parameter_type));
#endif

	switch( ptz->parameter_type ) {
	case MXF_PTZ_ZOOM_POSITION:
		mx_status = mx_get( &(network_ptz->zoom_position_nf),
					MXFT_ULONG, &(ptz->zoom_position) );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Parameter type %d received for soft PTZ '%s' "
			"is not a known parameter type.",
			ptz->parameter_type, network_ptz->record->name );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_ptz_set_parameter( MX_PAN_TILT_ZOOM *ptz )
{
	static const char fname[] = "mxd_network_ptz_set_parameter()";

	MX_NETWORK_PTZ *network_ptz;
	mx_status_type mx_status;

	mx_status = mxd_network_ptz_get_pointers( ptz, &network_ptz, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_PTZ_DEBUG
	MX_DEBUG(-2,("%s invoked for PTZ '%s' for command type %#x.",
		fname, ptz->record->name, ptz->parameter_type));
#endif

	switch( ptz->parameter_type ) {
	case MXF_PTZ_ZOOM_DESTINATION:
		mx_status = mx_put( &(network_ptz->zoom_destination_nf),
					MXFT_ULONG, &(ptz->zoom_destination) );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Parameter type %d received for soft PTZ '%s' "
			"is not a known parameter type.",
			ptz->parameter_type, network_ptz->record->name );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

