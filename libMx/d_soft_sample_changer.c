/*
 * Name:    d_soft_sample_changer.c
 *
 * Purpose: MX driver for software emulated sample changers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_SOFT_SAMPLE_CHANGER_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_types.h"
#include "mx_net.h"
#include "mx_sample_changer.h"

#include "d_soft_sample_changer.h"

MX_RECORD_FUNCTION_LIST mxd_soft_sample_changer_record_function_list = {
	NULL,
	mxd_soft_sample_changer_create_record_structures
};

MX_SAMPLE_CHANGER_FUNCTION_LIST
			mxd_soft_sample_changer_sample_changer_function_list
  = {
	mxd_soft_sample_changer_initialize,
	mxd_soft_sample_changer_shutdown,
	mxd_soft_sample_changer_mount_sample,
	mxd_soft_sample_changer_unmount_sample,
	mxd_soft_sample_changer_grab_sample,
	mxd_soft_sample_changer_ungrab_sample,
	mxd_soft_sample_changer_select_sample_holder,
	mxd_soft_sample_changer_unselect_sample_holder,
	mxd_soft_sample_changer_soft_abort,
	mxd_soft_sample_changer_immediate_abort,
	mxd_soft_sample_changer_idle,
	mxd_soft_sample_changer_reset,
	mxd_soft_sample_changer_get_status,
	mx_sample_changer_default_get_parameter_handler,
	mxd_soft_sample_changer_set_parameter
};

MX_RECORD_FIELD_DEFAULTS mxd_soft_sample_changer_rf_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_SAMPLE_CHANGER_STANDARD_FIELDS,
	MXD_SOFT_SAMPLE_CHANGER_STANDARD_FIELDS
};

long mxd_soft_sample_changer_num_record_fields
		= sizeof( mxd_soft_sample_changer_rf_defaults )
		/ sizeof( mxd_soft_sample_changer_rf_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_soft_sample_changer_rfield_def_ptr
			= &mxd_soft_sample_changer_rf_defaults[0];

/* --- */

static mx_status_type
mxd_soft_sample_changer_get_pointers( MX_SAMPLE_CHANGER *changer,
			MX_SOFT_SAMPLE_CHANGER **soft_sample_changer,
			const char *calling_fname )
{
	static const char fname[] = "mxd_soft_sample_changer_get_pointers()";

	if ( changer == (MX_SAMPLE_CHANGER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SAMPLE_CHANGER pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( soft_sample_changer == (MX_SOFT_SAMPLE_CHANGER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SOFT_SAMPLE_CHANGER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*soft_sample_changer = (MX_SOFT_SAMPLE_CHANGER *)
			changer->record->record_type_struct;

	if ( *soft_sample_changer == (MX_SOFT_SAMPLE_CHANGER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SOFT_SAMPLE_CHANGER pointer for record '%s' is NULL.",
			changer->record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/* ====== */

MX_EXPORT mx_status_type
mxd_soft_sample_changer_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_soft_sample_changer_create_record_structures()";

	MX_SAMPLE_CHANGER *changer;
	MX_SOFT_SAMPLE_CHANGER *soft_sample_changer;

	changer = (MX_SAMPLE_CHANGER *)
				malloc( sizeof(MX_SAMPLE_CHANGER) );

	if ( changer == (MX_SAMPLE_CHANGER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SAMPLE_CHANGER structure." );
	}

	soft_sample_changer = (MX_SOFT_SAMPLE_CHANGER *)
				malloc( sizeof(MX_SOFT_SAMPLE_CHANGER) );

	if ( soft_sample_changer == (MX_SOFT_SAMPLE_CHANGER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_SOFT_SAMPLE_CHANGER structure." );
	}

	record->record_class_struct = changer;
	record->record_type_struct = soft_sample_changer;
	record->class_specific_function_list = 
		&mxd_soft_sample_changer_sample_changer_function_list;

	changer->record = record;
	soft_sample_changer->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_sample_changer_initialize( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_soft_sample_changer_initialize()";

	MX_SOFT_SAMPLE_CHANGER *soft_sample_changer;
	mx_status_type mx_status;

	mx_status = mxd_soft_sample_changer_get_pointers( changer,
						&soft_sample_changer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, changer->record->name));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_sample_changer_shutdown( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_soft_sample_changer_shutdown()";

	MX_SOFT_SAMPLE_CHANGER *soft_sample_changer;
	mx_status_type mx_status;

	mx_status = mxd_soft_sample_changer_get_pointers( changer,
						&soft_sample_changer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, changer->record->name));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_sample_changer_mount_sample( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_soft_sample_changer_mount_sample()";

	MX_SOFT_SAMPLE_CHANGER *soft_sample_changer;
	mx_status_type mx_status;

	mx_status = mxd_soft_sample_changer_get_pointers( changer,
						&soft_sample_changer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, changer->record->name));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_sample_changer_unmount_sample( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_soft_sample_changer_unmount_sample()";

	MX_SOFT_SAMPLE_CHANGER *soft_sample_changer;
	mx_status_type mx_status;

	mx_status = mxd_soft_sample_changer_get_pointers( changer,
						&soft_sample_changer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, changer->record->name));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_sample_changer_grab_sample( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_soft_sample_changer_grab_sample()";

	MX_SOFT_SAMPLE_CHANGER *soft_sample_changer;
	mx_status_type mx_status;

	mx_status = mxd_soft_sample_changer_get_pointers( changer,
						&soft_sample_changer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, changer->record->name));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_sample_changer_ungrab_sample( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_soft_sample_changer_ungrab_sample()";

	MX_SOFT_SAMPLE_CHANGER *soft_sample_changer;
	mx_status_type mx_status;

	mx_status = mxd_soft_sample_changer_get_pointers( changer,
						&soft_sample_changer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, changer->record->name));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_sample_changer_select_sample_holder( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] =
		"mxd_soft_sample_changer_select_sample_holder()";

	MX_SOFT_SAMPLE_CHANGER *soft_sample_changer;
	mx_status_type mx_status;

	mx_status = mxd_soft_sample_changer_get_pointers( changer,
						&soft_sample_changer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, changer->record->name));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_sample_changer_unselect_sample_holder( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] =
		"mxd_soft_sample_changer_unselect_sample_holder()";

	MX_SOFT_SAMPLE_CHANGER *soft_sample_changer;
	mx_status_type mx_status;

	mx_status = mxd_soft_sample_changer_get_pointers( changer,
						&soft_sample_changer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, changer->record->name));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_sample_changer_soft_abort( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_soft_sample_changer_soft_abort()";

	MX_SOFT_SAMPLE_CHANGER *soft_sample_changer;
	mx_status_type mx_status;

	mx_status = mxd_soft_sample_changer_get_pointers( changer,
						&soft_sample_changer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, changer->record->name));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_sample_changer_immediate_abort( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_soft_sample_changer_immediate_abort()";

	MX_SOFT_SAMPLE_CHANGER *soft_sample_changer;
	mx_status_type mx_status;

	mx_status = mxd_soft_sample_changer_get_pointers( changer,
						&soft_sample_changer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, changer->record->name));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_sample_changer_idle( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_soft_sample_changer_idle()";

	MX_SOFT_SAMPLE_CHANGER *soft_sample_changer;
	mx_status_type mx_status;

	mx_status = mxd_soft_sample_changer_get_pointers( changer,
						&soft_sample_changer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, changer->record->name));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_sample_changer_reset( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_soft_sample_changer_reset()";

	MX_SOFT_SAMPLE_CHANGER *soft_sample_changer;
	mx_status_type mx_status;

	mx_status = mxd_soft_sample_changer_get_pointers( changer,
						&soft_sample_changer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, changer->record->name));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_sample_changer_get_status( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_soft_sample_changer_status()";

	MX_SOFT_SAMPLE_CHANGER *soft_sample_changer;
	mx_status_type mx_status;

	mx_status = mxd_soft_sample_changer_get_pointers( changer,
						&soft_sample_changer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, changer->record->name));

	changer->status = 0;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_sample_changer_set_parameter( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] = "mxd_soft_sample_changer_set_parameter()";

	MX_SOFT_SAMPLE_CHANGER *soft_sample_changer;
	mx_status_type mx_status;

	mx_status = mxd_soft_sample_changer_get_pointers( changer,
						&soft_sample_changer, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,
	("%s invoked for sample changer '%s', parameter type '%s' (%d)",
		fname, changer->record->name,
		mx_get_field_label_string( changer->record,
					changer->parameter_type ),
		changer->parameter_type));

	switch( changer->parameter_type ) {
	case MXLV_CHG_COOLDOWN:
		break;
	case MXLV_CHG_DEICE:
		break;
	default:
		mx_status =  mx_sample_changer_default_set_parameter_handler(
								changer );
		break;
	}
	MX_DEBUG(-2,("%s complete.", fname));

	return mx_status;
}

