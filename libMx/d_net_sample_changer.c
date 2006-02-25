/*
 * Name:    d_net_sample_changer.c
 *
 * Purpose: MX driver for network controlled sample changers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2004-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_NET_SAMPLE_CHANGER_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_net.h"
#include "mx_sample_changer.h"

#include "d_net_sample_changer.h"

MX_RECORD_FUNCTION_LIST mxd_net_sample_changer_record_function_list = {
	NULL,
	mxd_net_sample_changer_create_record_structures,
	mxd_net_sample_changer_finish_record_initialization
};

MX_SAMPLE_CHANGER_FUNCTION_LIST mxd_net_sample_changer_sample_changer_function_list
  = {
	mxd_net_sample_changer_initialize,
	mxd_net_sample_changer_shutdown,
	mxd_net_sample_changer_mount_sample,
	mxd_net_sample_changer_unmount_sample,
	mxd_net_sample_changer_grab_sample,
	mxd_net_sample_changer_ungrab_sample,
	mxd_net_sample_changer_select_sample_holder,
	mxd_net_sample_changer_unselect_sample_holder,
	mxd_net_sample_changer_soft_abort,
	mxd_net_sample_changer_immediate_abort,
	mxd_net_sample_changer_idle,
	mxd_net_sample_changer_reset,
	mxd_net_sample_changer_get_status,
	mx_sample_changer_default_get_parameter_handler,
	mxd_net_sample_changer_set_parameter
};

MX_RECORD_FIELD_DEFAULTS mxd_net_sample_changer_rf_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SAMPLE_CHANGER_STANDARD_FIELDS,
	MXD_NET_SAMPLE_CHANGER_STANDARD_FIELDS
};

long mxd_net_sample_changer_num_record_fields
		= sizeof( mxd_net_sample_changer_rf_field_defaults )
			/ sizeof( mxd_net_sample_changer_rf_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_net_sample_changer_rfield_def_ptr
			= &mxd_net_sample_changer_rf_field_defaults[0];

/* --- */

static mx_status_type
mxd_net_sample_changer_get_pointers( MX_SAMPLE_CHANGER *changer,
			MX_NET_SAMPLE_CHANGER **net_sample_changer,
			const char *calling_fname )
{
	static const char fname[] = "mxd_net_sample_changer_get_pointers()";

	if ( changer == (MX_SAMPLE_CHANGER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SAMPLE_CHANGER pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( net_sample_changer == (MX_NET_SAMPLE_CHANGER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NET_SAMPLE_CHANGER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*net_sample_changer = (MX_NET_SAMPLE_CHANGER *)
			changer->record->record_type_struct;

	if ( *net_sample_changer == (MX_NET_SAMPLE_CHANGER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_NET_SAMPLE_CHANGER pointer for record '%s' is NULL.",
			changer->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ====== */

MX_EXPORT mx_status_type
mxd_net_sample_changer_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_net_sample_changer_create_record_structures()";

	MX_SAMPLE_CHANGER *changer;
	MX_NET_SAMPLE_CHANGER *net_sample_changer;

	changer = (MX_SAMPLE_CHANGER *)
				malloc( sizeof(MX_SAMPLE_CHANGER) );

	if ( changer == (MX_SAMPLE_CHANGER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SAMPLE_CHANGER structure." );
	}

	net_sample_changer = (MX_NET_SAMPLE_CHANGER *)
				malloc( sizeof(MX_NET_SAMPLE_CHANGER) );

	if ( net_sample_changer == (MX_NET_SAMPLE_CHANGER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_NET_SAMPLE_CHANGER structure." );
	}

	record->record_class_struct = changer;
	record->record_type_struct = net_sample_changer;
	record->class_specific_function_list = 
		&mxd_net_sample_changer_sample_changer_function_list;

	changer->record = record;
	net_sample_changer->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_net_sample_changer_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_net_sample_changer_finish_record_initialization()";

	MX_SAMPLE_CHANGER *sample_changer;
	MX_NET_SAMPLE_CHANGER *net_sample_changer;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	sample_changer = (MX_SAMPLE_CHANGER *) record->record_class_struct;

	mx_status = mxd_net_sample_changer_get_pointers(
				sample_changer, &net_sample_changer, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_network_field_init( &(net_sample_changer->cooldown_nf),
		net_sample_changer->server_record,
		"%s.cooldown", net_sample_changer->remote_record_name );

	mx_network_field_init( &(net_sample_changer->deice_nf),
		net_sample_changer->server_record,
		"%s.deice", net_sample_changer->remote_record_name );

	mx_network_field_init( &(net_sample_changer->grab_sample_nf),
		net_sample_changer->server_record,
		"%s.grab_sample", net_sample_changer->remote_record_name );

	mx_network_field_init( &(net_sample_changer->idle_nf),
		net_sample_changer->server_record,
		"%s.idle", net_sample_changer->remote_record_name );

	mx_network_field_init( &(net_sample_changer->immediate_abort_nf),
		net_sample_changer->server_record,
		"%s.immediate_abort", net_sample_changer->remote_record_name );

	mx_network_field_init( &(net_sample_changer->initialize_nf),
		net_sample_changer->server_record,
		"%s.initialize", net_sample_changer->remote_record_name );

	mx_network_field_init( &(net_sample_changer->mount_sample_nf),
		net_sample_changer->server_record,
		"%s.mount_sample", net_sample_changer->remote_record_name );

	mx_network_field_init(&(net_sample_changer->requested_sample_holder_nf),
		net_sample_changer->server_record,
	"%s.requested_sample_holder", net_sample_changer->remote_record_name );

	mx_network_field_init( &(net_sample_changer->requested_sample_id_nf),
		net_sample_changer->server_record,
	"%s.requested_sample_id", net_sample_changer->remote_record_name );

	mx_network_field_init( &(net_sample_changer->reset_nf),
		net_sample_changer->server_record,
		"%s.reset", net_sample_changer->remote_record_name );

	mx_network_field_init( &(net_sample_changer->select_sample_holder_nf),
		net_sample_changer->server_record,
	"%s.select_sample_holder", net_sample_changer->remote_record_name );

	mx_network_field_init( &(net_sample_changer->shutdown_nf),
		net_sample_changer->server_record,
		"%s.shutdown", net_sample_changer->remote_record_name );

	mx_network_field_init( &(net_sample_changer->soft_abort_nf),
		net_sample_changer->server_record,
		"%s.soft_abort", net_sample_changer->remote_record_name );

	mx_network_field_init( &(net_sample_changer->status_nf),
		net_sample_changer->server_record,
		"%s.status", net_sample_changer->remote_record_name );

	mx_network_field_init( &(net_sample_changer->ungrab_sample_nf),
		net_sample_changer->server_record,
		"%s.ungrab_sample", net_sample_changer->remote_record_name );

	mx_network_field_init( &(net_sample_changer->unmount_sample_nf),
		net_sample_changer->server_record,
		"%s.unmount_sample", net_sample_changer->remote_record_name );

	mx_network_field_init( &(net_sample_changer->unselect_sample_holder_nf),
		net_sample_changer->server_record,
	"%s.unselect_sample_holder", net_sample_changer->remote_record_name );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_net_sample_changer_initialize( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_net_sample_changer_initialize()";

	MX_NET_SAMPLE_CHANGER *net_sample_changer;
	mx_bool_type initialize;
	mx_status_type mx_status;

	mx_status = mxd_net_sample_changer_get_pointers( changer,
						&net_sample_changer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, changer->record->name));

	initialize = TRUE;

	mx_status = mx_put( &(net_sample_changer->initialize_nf),
				MXFT_BOOL, &initialize );

	MX_DEBUG(-2,("%s complete.", fname));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_net_sample_changer_shutdown( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_net_sample_changer_shutdown()";

	MX_NET_SAMPLE_CHANGER *net_sample_changer;
	mx_bool_type shutdown;
	mx_status_type mx_status;

	mx_status = mxd_net_sample_changer_get_pointers( changer,
						&net_sample_changer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, changer->record->name));

	shutdown = TRUE;

	mx_status = mx_put( &(net_sample_changer->shutdown_nf),
				MXFT_BOOL, &shutdown );

	MX_DEBUG(-2,("%s complete.", fname));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_net_sample_changer_mount_sample( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_net_sample_changer_mount_sample()";

	MX_NET_SAMPLE_CHANGER *net_sample_changer;
	mx_bool_type mount_sample;
	mx_status_type mx_status;

	mx_status = mxd_net_sample_changer_get_pointers( changer,
						&net_sample_changer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, changer->record->name));

	/* Mount the sample. */

	mount_sample = TRUE;

	mx_status = mx_put( &(net_sample_changer->mount_sample_nf),
				MXFT_BOOL, &(mount_sample) );

	MX_DEBUG(-2,("%s complete.", fname));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_net_sample_changer_unmount_sample( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_net_sample_changer_unmount_sample()";

	MX_NET_SAMPLE_CHANGER *net_sample_changer;
	mx_bool_type unmount_sample;
	mx_status_type mx_status;

	mx_status = mxd_net_sample_changer_get_pointers( changer,
						&net_sample_changer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, changer->record->name));

	unmount_sample = TRUE;

	mx_status = mx_put( &(net_sample_changer->unmount_sample_nf),
				MXFT_BOOL, &(unmount_sample) );

	MX_DEBUG(-2,("%s complete.", fname));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_net_sample_changer_grab_sample( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_net_sample_changer_grab_sample()";

	MX_NET_SAMPLE_CHANGER *net_sample_changer;
	mx_bool_type grab_sample;
	mx_status_type mx_status;

	mx_status = mxd_net_sample_changer_get_pointers( changer,
						&net_sample_changer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, changer->record->name));

	/* Identify the sample ID to use. */

	mx_status = mx_put( &(net_sample_changer->requested_sample_id_nf),
			MXFT_ULONG, &(changer->requested_sample_id) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Mount the sample. */

	grab_sample = TRUE;

	mx_status = mx_put( &(net_sample_changer->grab_sample_nf),
				MXFT_BOOL, &(grab_sample) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	changer->current_sample_id = changer->requested_sample_id;

	MX_DEBUG(-2,("%s complete.", fname));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_net_sample_changer_ungrab_sample( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_net_sample_changer_ungrab_sample()";

	MX_NET_SAMPLE_CHANGER *net_sample_changer;
	mx_bool_type ungrab_sample;
	mx_status_type mx_status;

	mx_status = mxd_net_sample_changer_get_pointers( changer,
						&net_sample_changer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, changer->record->name));

	ungrab_sample = TRUE;

	mx_status = mx_put( &(net_sample_changer->ungrab_sample_nf),
				MXFT_BOOL, &(ungrab_sample) );

	MX_DEBUG(-2,("%s complete.", fname));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_net_sample_changer_select_sample_holder( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] =
		"mxd_net_sample_changer_select_sample_holder()";

	MX_NET_SAMPLE_CHANGER *net_sample_changer;
	long dimension_array[1];
	mx_bool_type select_sample_holder;
	mx_status_type mx_status;

	mx_status = mxd_net_sample_changer_get_pointers( changer,
						&net_sample_changer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, changer->record->name));

	/* Identify the sample holder to use. */

	dimension_array[0] = (long) strlen( changer->requested_sample_holder );

	mx_status = mx_put_array(
			&(net_sample_changer->requested_sample_holder_nf),
			MXFT_STRING, 1, dimension_array,
			changer->requested_sample_holder );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	select_sample_holder = TRUE;

	mx_status = mx_put( &(net_sample_changer->select_sample_holder_nf),
				MXFT_BOOL, &(select_sample_holder) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	strlcpy( changer->current_sample_holder,
			changer->requested_sample_holder,
			MXU_SAMPLE_HOLDER_NAME_LENGTH );

	MX_DEBUG(-2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_net_sample_changer_unselect_sample_holder( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] =
		"mxd_net_sample_changer_unselect_sample_holder()";

	MX_NET_SAMPLE_CHANGER *net_sample_changer;
	mx_bool_type unselect_sample_holder;
	mx_status_type mx_status;

	mx_status = mxd_net_sample_changer_get_pointers( changer,
						&net_sample_changer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, changer->record->name));

	unselect_sample_holder = TRUE;

	mx_status = mx_put( &(net_sample_changer->unselect_sample_holder_nf),
				MXFT_BOOL, &(unselect_sample_holder) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	strlcpy( changer->current_sample_holder,
			MX_CHG_NO_SAMPLE_HOLDER,
			MXU_SAMPLE_HOLDER_NAME_LENGTH );

	MX_DEBUG(-2,("%s complete.", fname));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_net_sample_changer_soft_abort( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_net_sample_changer_soft_abort()";

	MX_NET_SAMPLE_CHANGER *net_sample_changer;
	mx_bool_type soft_abort;
	mx_status_type mx_status;

	mx_status = mxd_net_sample_changer_get_pointers( changer,
						&net_sample_changer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, changer->record->name));

	soft_abort = TRUE;

	mx_status = mx_put( &(net_sample_changer->soft_abort_nf),
				MXFT_BOOL, &(soft_abort) );

	MX_DEBUG(-2,("%s complete.", fname));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_net_sample_changer_immediate_abort( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_net_sample_changer_immediate_abort()";

	MX_NET_SAMPLE_CHANGER *net_sample_changer;
	mx_bool_type immediate_abort;
	mx_status_type mx_status;

	mx_status = mxd_net_sample_changer_get_pointers( changer,
						&net_sample_changer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, changer->record->name));

	immediate_abort = TRUE;

	mx_status = mx_put( &(net_sample_changer->immediate_abort_nf),
				MXFT_BOOL, &(immediate_abort) );

	MX_DEBUG(-2,("%s complete.", fname));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_net_sample_changer_idle( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_net_sample_changer_idle()";

	MX_NET_SAMPLE_CHANGER *net_sample_changer;
	mx_bool_type idle;
	mx_status_type mx_status;

	mx_status = mxd_net_sample_changer_get_pointers( changer,
						&net_sample_changer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, changer->record->name));

	idle = TRUE;

	mx_status = mx_put( &(net_sample_changer->idle_nf), MXFT_BOOL, &(idle));

	MX_DEBUG(-2,("%s complete.", fname));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_net_sample_changer_reset( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_net_sample_changer_reset()";

	MX_NET_SAMPLE_CHANGER *net_sample_changer;
	mx_bool_type reset;
	mx_status_type mx_status;

	mx_status = mxd_net_sample_changer_get_pointers( changer,
						&net_sample_changer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, changer->record->name));

	reset = TRUE;

	mx_status = mx_put( &(net_sample_changer->reset_nf),
				MXFT_BOOL, &(reset) );

	MX_DEBUG(-2,("%s complete.", fname));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_net_sample_changer_get_status( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_net_sample_changer_status()";

	MX_NET_SAMPLE_CHANGER *net_sample_changer;
	mx_status_type mx_status;

	mx_status = mxd_net_sample_changer_get_pointers( changer,
						&net_sample_changer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, changer->record->name));

	mx_status = mx_get( &(net_sample_changer->status_nf),
				MXFT_ULONG, &(changer->status) );

	MX_DEBUG(-2,("%s complete.", fname));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_net_sample_changer_set_parameter( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_net_sample_changer_set_parameter()";

	MX_NET_SAMPLE_CHANGER *net_sample_changer;
	mx_bool_type bool_value;
	mx_status_type mx_status;

	mx_status = mxd_net_sample_changer_get_pointers( changer,
						&net_sample_changer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,
	("%s invoked for sample changer '%s', parameter type '%s' (%ld)",
		fname, changer->record->name,
		mx_get_field_label_string( changer->record,
					changer->parameter_type ),
		changer->parameter_type));

	switch( changer->parameter_type ) {
	case MXLV_CHG_COOLDOWN:
		bool_value = TRUE;

		mx_status = mx_put( &(net_sample_changer->cooldown_nf),
					MXFT_BOOL, &(bool_value) );
		break;
	case MXLV_CHG_DEICE:
		bool_value = TRUE;

		mx_status = mx_put( &(net_sample_changer->deice_nf),
					MXFT_BOOL, &(bool_value) );
		break;
	default:
		mx_status =  mx_sample_changer_default_set_parameter_handler(
								changer );
		break;
	}
	MX_DEBUG(-2,("%s complete.", fname));

	return mx_status;
}

