/*
 * Name:    mx_sample_changer.c
 *
 * Purpose: MX function library of generic sample changer control operations.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003-2006 Illinois Institute of Technology
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
#include "mx_sample_changer.h"

/*=======================================================================*/

/* This function is a utility function to consolidate all of the pointer
 * mangling that often has to happen at the beginning of an mx_sample_changer_
 * function.  The parameter 'calling_fname' is passed so that error messages
 * will appear with the name of the calling function.
 */

MX_EXPORT mx_status_type
mx_sample_changer_get_pointers( MX_RECORD *record,
			MX_SAMPLE_CHANGER **changer,
			MX_SAMPLE_CHANGER_FUNCTION_LIST **function_list_ptr,
			const char *calling_fname )
{
	static const char fname[] = "mx_sample_changer_get_pointers()";

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The SAMPLE_CHANGER_record pointer passed by '%s' was NULL",
			calling_fname );
	}

	if ( record->mx_class != MXC_SAMPLE_CHANGER ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"The record '%s' passed by '%s' is not an MX sample changer.",
			record->name, calling_fname );
	}

	if ( changer != (MX_SAMPLE_CHANGER **) NULL ) {
		*changer = (MX_SAMPLE_CHANGER *)
				record->record_class_struct;

		if ( *changer == (MX_SAMPLE_CHANGER *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_SAMPLE_CHANGER pointer for record '%s' "
			"passed by '%s' is NULL",
				record->name, calling_fname );
		}
	}

	if ( function_list_ptr != (MX_SAMPLE_CHANGER_FUNCTION_LIST **) NULL ) {
		*function_list_ptr = (MX_SAMPLE_CHANGER_FUNCTION_LIST *)
			record->class_specific_function_list;

		if ( *function_list_ptr ==
			(MX_SAMPLE_CHANGER_FUNCTION_LIST *) NULL )
		{
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_SAMPLE_CHANGER_FUNCTION_LIST pointer for "
			"record '%s' passed by '%s' is NULL.",
				record->name, calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mx_sample_changer_initialize( MX_RECORD *record )
{
	static const char fname[] = "mx_sample_changer_initialize()";

	MX_SAMPLE_CHANGER *changer;
	MX_SAMPLE_CHANGER_FUNCTION_LIST *function_list;
	mx_status_type ( *initialize_fn ) ( MX_SAMPLE_CHANGER * );
	mx_status_type mx_status;

	mx_status = mx_sample_changer_get_pointers( record,
				&changer, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	initialize_fn = function_list->initialize;

	if ( initialize_fn == NULL ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Initializing '%s' sample changer '%s' is not yet implemented.",
			mx_get_driver_name( record ),
			record->name );
	}

	mx_status = (*initialize_fn)( changer );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_sample_changer_shutdown( MX_RECORD *record )
{
	static const char fname[] = "mx_sample_changer_shutdown()";

	MX_SAMPLE_CHANGER *changer;
	MX_SAMPLE_CHANGER_FUNCTION_LIST *function_list;
	mx_status_type ( *shutdown_fn ) ( MX_SAMPLE_CHANGER * );
	mx_status_type mx_status;

	mx_status = mx_sample_changer_get_pointers( record,
				&changer, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	shutdown_fn = function_list->shutdown;

	if ( shutdown_fn == NULL ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
	"Shutting down '%s' sample changer '%s' is not yet implemented.",
			mx_get_driver_name( record ),
			record->name );
	}

	mx_status = (*shutdown_fn)( changer );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_sample_changer_mount_sample( MX_RECORD *record )
{
	static const char fname[] = "mx_sample_changer_mount_sample()";

	MX_SAMPLE_CHANGER *changer;
	MX_SAMPLE_CHANGER_FUNCTION_LIST *function_list;
	mx_status_type ( *mount_sample_fn ) ( MX_SAMPLE_CHANGER * );
	mx_status_type mx_status;

	mx_status = mx_sample_changer_get_pointers( record,
				&changer, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mount_sample_fn = function_list->mount_sample;

	if ( mount_sample_fn == NULL ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
"'mount_sample' for '%s' sample changer '%s' is not yet implemented.",
			mx_get_driver_name( record ),
			record->name );
	}

	mx_status = (*mount_sample_fn)( changer );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mx_sample_changer_unmount_sample( MX_RECORD *record )
{
	static const char fname[] = "mx_sample_changer_unmount_sample()";

	MX_SAMPLE_CHANGER *changer;
	MX_SAMPLE_CHANGER_FUNCTION_LIST *function_list;
	mx_status_type ( *unmount_sample_fn ) ( MX_SAMPLE_CHANGER * );
	mx_status_type mx_status;

	mx_status = mx_sample_changer_get_pointers( record,
				&changer, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	unmount_sample_fn = function_list->unmount_sample;

	if ( unmount_sample_fn == NULL ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
"'unmount_sample' for '%s' sample changer '%s' is not yet implemented.",
			mx_get_driver_name( record ),
			record->name );
	}

	mx_status = (*unmount_sample_fn)( changer );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mx_sample_changer_grab_sample( MX_RECORD *record, long sample_id )
{
	static const char fname[] = "mx_sample_changer_grab_sample()";

	MX_SAMPLE_CHANGER *changer;
	MX_SAMPLE_CHANGER_FUNCTION_LIST *function_list;
	mx_status_type ( *grab_sample_fn ) ( MX_SAMPLE_CHANGER * );
	mx_status_type mx_status;

	mx_status = mx_sample_changer_get_pointers( record,
				&changer, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	grab_sample_fn = function_list->grab_sample;

	if ( grab_sample_fn == NULL ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
"'grab_sample' for '%s' sample changer '%s' is not yet implemented.",
			mx_get_driver_name( record ),
			record->name );
	}

	changer->requested_sample_id = sample_id;

	mx_status = (*grab_sample_fn)( changer );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	changer->current_sample_id = sample_id;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_sample_changer_ungrab_sample( MX_RECORD *record )
{
	static const char fname[] = "mx_sample_changer_ungrab_sample()";

	MX_SAMPLE_CHANGER *changer;
	MX_SAMPLE_CHANGER_FUNCTION_LIST *function_list;
	mx_status_type ( *ungrab_sample_fn ) ( MX_SAMPLE_CHANGER * );
	mx_status_type mx_status;

	mx_status = mx_sample_changer_get_pointers( record,
				&changer, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ungrab_sample_fn = function_list->ungrab_sample;

	if ( ungrab_sample_fn == NULL ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
"'ungrab_sample' for '%s' sample changer '%s' is not yet implemented.",
			mx_get_driver_name( record ),
			record->name );
	}

	mx_status = (*ungrab_sample_fn)( changer );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	changer->current_sample_id = MX_CHG_NO_SAMPLE_ID;

	return mx_status;
}

MX_EXPORT mx_status_type
mx_sample_changer_select_sample_holder( MX_RECORD *record,
					char *sample_holder )
{
	static const char fname[] = "mx_sample_changer_select_sample_holder()";

	MX_SAMPLE_CHANGER *changer;
	MX_SAMPLE_CHANGER_FUNCTION_LIST *function_list;
	mx_status_type ( *select_sample_holder_fn ) ( MX_SAMPLE_CHANGER * );
	mx_status_type mx_status;

	mx_status = mx_sample_changer_get_pointers( record,
				&changer, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( sample_holder == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The 'sample_holder' pointer passed was NULL." );
	}

	select_sample_holder_fn = function_list->select_sample_holder;

	if ( select_sample_holder_fn == NULL ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
"'select_sample_holder' for '%s' sample changer '%s' is not yet implemented.",
			mx_get_driver_name( record ),
			record->name );
	}

	strlcpy( changer->requested_sample_holder, sample_holder,
					MXU_SAMPLE_HOLDER_NAME_LENGTH );

	mx_status = (*select_sample_holder_fn)( changer );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	strlcpy( changer->current_sample_holder, sample_holder,
					MXU_SAMPLE_HOLDER_NAME_LENGTH );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_sample_changer_unselect_sample_holder( MX_RECORD *record )
{
	static const char fname[] =
			"mx_sample_changer_unselect_sample_holder()";

	MX_SAMPLE_CHANGER *changer;
	MX_SAMPLE_CHANGER_FUNCTION_LIST *function_list;
	mx_status_type ( *unselect_sample_holder_fn ) ( MX_SAMPLE_CHANGER * );
	mx_status_type mx_status;

	mx_status = mx_sample_changer_get_pointers( record,
				&changer, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	unselect_sample_holder_fn = function_list->unselect_sample_holder;

	if ( unselect_sample_holder_fn == NULL ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
"'unselect_sample_holder' for '%s' sample changer '%s' is not yet implemented.",
			mx_get_driver_name( record ),
			record->name );
	}

	mx_status = (*unselect_sample_holder_fn)( changer );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	strcpy( changer->current_sample_holder, MX_CHG_NO_SAMPLE_HOLDER );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_sample_changer_get_sample_holder( MX_RECORD *record,
					size_t maximum_name_length,
					char *sample_holder_name )
{
	static const char fname[] = "mx_sample_changer_get_sample_holder()";

	MX_SAMPLE_CHANGER *changer;
	MX_SAMPLE_CHANGER_FUNCTION_LIST *function_list;
	mx_status_type mx_status;

	mx_status = mx_sample_changer_get_pointers( record,
				&changer, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( sample_holder_name != NULL ) {
		strlcpy( sample_holder_name,
			changer->current_sample_holder,
			maximum_name_length );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_sample_changer_get_sample_id( MX_RECORD *record, long *sample_id )
{
	static const char fname[] = "mx_sample_changer_get_sample_id()";

	MX_SAMPLE_CHANGER *changer;
	MX_SAMPLE_CHANGER_FUNCTION_LIST *function_list;
	mx_status_type mx_status;

	mx_status = mx_sample_changer_get_pointers( record,
				&changer, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( sample_id != (long *) NULL ) {
		*sample_id = changer->current_sample_id;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_sample_changer_set_sample_id( MX_RECORD *record, long sample_id )
{
	static const char fname[] = "mx_sample_changer_get_sample_id()";

	MX_SAMPLE_CHANGER *changer;
	MX_SAMPLE_CHANGER_FUNCTION_LIST *function_list;
	mx_status_type mx_status;

	mx_status = mx_sample_changer_get_pointers( record,
				&changer, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	changer->current_sample_id = sample_id;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_sample_changer_soft_abort( MX_RECORD *record )
{
	static const char fname[] = "mx_sample_changer_soft_abort()";

	MX_SAMPLE_CHANGER *changer;
	MX_SAMPLE_CHANGER_FUNCTION_LIST *function_list;
	mx_status_type ( *soft_abort_fn ) ( MX_SAMPLE_CHANGER * );
	mx_status_type mx_status;

	mx_status = mx_sample_changer_get_pointers( record,
				&changer, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	soft_abort_fn = function_list->soft_abort;

	if ( soft_abort_fn == NULL ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
	"Soft abort for '%s' sample changer '%s' is not yet implemented.",
			mx_get_driver_name( record ),
			record->name );
	}

	mx_status = (*soft_abort_fn)( changer );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_sample_changer_immediate_abort( MX_RECORD *record )
{
	static const char fname[] = "mx_sample_changer_immediate_abort()";

	MX_SAMPLE_CHANGER *changer;
	MX_SAMPLE_CHANGER_FUNCTION_LIST *function_list;
	mx_status_type ( *immediate_abort_fn ) ( MX_SAMPLE_CHANGER * );
	mx_status_type mx_status;

	mx_status = mx_sample_changer_get_pointers( record,
				&changer, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	immediate_abort_fn = function_list->immediate_abort;

	if ( immediate_abort_fn == NULL ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
	"Immediate abort for '%s' sample changer '%s' is not yet implemented.",
			mx_get_driver_name( record ),
			record->name );
	}

	mx_status = (*immediate_abort_fn)( changer );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_sample_changer_idle( MX_RECORD *record )
{
	static const char fname[] = "mx_sample_changer_idle()";

	MX_SAMPLE_CHANGER *changer;
	MX_SAMPLE_CHANGER_FUNCTION_LIST *function_list;
	mx_status_type ( *idle_fn ) ( MX_SAMPLE_CHANGER * );
	mx_status_type mx_status;

	mx_status = mx_sample_changer_get_pointers( record,
				&changer, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	idle_fn = function_list->idle;

	if ( idle_fn == NULL ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
	"Idle for '%s' sample changer '%s' is not yet implemented.",
			mx_get_driver_name( record ),
			record->name );
	}

	mx_status = (*idle_fn)( changer );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_sample_changer_reset( MX_RECORD *record )
{
	static const char fname[] = "mx_sample_changer_reset()";

	MX_SAMPLE_CHANGER *changer;
	MX_SAMPLE_CHANGER_FUNCTION_LIST *function_list;
	mx_status_type ( *reset_fn ) ( MX_SAMPLE_CHANGER * );
	mx_status_type mx_status;

	mx_status = mx_sample_changer_get_pointers( record,
				&changer, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	reset_fn = function_list->reset;

	if ( reset_fn == NULL ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
	"Reset for '%s' sample changer '%s' is not yet implemented.",
			mx_get_driver_name( record ),
			record->name );
	}

	mx_status = (*reset_fn)( changer );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_sample_changer_get_status( MX_RECORD *record, unsigned long *changer_status )
{
	static const char fname[] = "mx_sample_changer_get_status()";

	MX_SAMPLE_CHANGER *changer;
	MX_SAMPLE_CHANGER_FUNCTION_LIST *function_list;
	mx_status_type ( *get_status_fn ) ( MX_SAMPLE_CHANGER * );
	mx_status_type mx_status;

	mx_status = mx_sample_changer_get_pointers( record,
				&changer, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_status_fn = function_list->get_status;

	if ( get_status_fn == NULL ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
	"Get status for '%s' sample changer '%s' is not yet implemented.",
			mx_get_driver_name( record ),
			record->name );
	}

	mx_status = (*get_status_fn)( changer );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( changer_status != (unsigned long *) NULL ) {
		*changer_status = changer->status;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_sample_changer_is_busy( MX_RECORD *record, mx_bool_type *busy )
{
	static const char fname[] = "mx_sample_changer_is_busy()";

	MX_SAMPLE_CHANGER *changer;
	unsigned long changer_status;
	mx_status_type mx_status;

	if ( busy == (mx_bool_type *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The 'busy' pointer passed was NULL." );
	}

	mx_status = mx_sample_changer_get_pointers( record,
					&changer, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_sample_changer_get_status( record, &changer_status );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( changer_status & MXSF_CHG_IS_BUSY ) {
		*busy = TRUE;
	} else {
		*busy = FALSE;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_sample_changer_default_get_parameter_handler( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] =
		"mx_sample_changer_default_get_parameter_handler()";

	mx_status_type mx_status;

	switch( changer->parameter_type ) {
	default:
		mx_status = mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type '%s' (%ld) is not supported by the "
		"MX driver for SAMPLE_CHANGER '%s'.",
			mx_get_field_label_string( changer->record,
					changer->parameter_type ),
			changer->parameter_type,
			changer->record->name );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_sample_changer_default_set_parameter_handler( MX_SAMPLE_CHANGER *changer )
{
	static const char fname[] =
		"mx_sample_changer_default_set_parameter_handler()";

	mx_status_type mx_status;

	switch( changer->parameter_type ) {
	default:
		mx_status = mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type '%s' (%ld) is not supported by the "
		"MX driver for SAMPLE_CHANGER '%s'.",
			mx_get_field_label_string( changer->record,
					changer->parameter_type ),
			changer->parameter_type,
			changer->record->name );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_sample_changer_cooldown( MX_RECORD *record )
{
	static const char fname[] = "mx_sample_changer_cooldown()";

	MX_SAMPLE_CHANGER *changer;
	MX_SAMPLE_CHANGER_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_SAMPLE_CHANGER * );
	mx_status_type mx_status;

	mx_status = mx_sample_changer_get_pointers( record,
				&changer, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn =
			mx_sample_changer_default_get_parameter_handler;
	}

	changer->parameter_type = MXLV_CHG_COOLDOWN;

	mx_status = (*get_parameter_fn)( changer );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_sample_changer_deice( MX_RECORD *record )
{
	static const char fname[] = "mx_sample_changer_deice()";

	MX_SAMPLE_CHANGER *changer;
	MX_SAMPLE_CHANGER_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_SAMPLE_CHANGER * );
	mx_status_type mx_status;

	mx_status = mx_sample_changer_get_pointers( record,
				&changer, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn =
			mx_sample_changer_default_get_parameter_handler;
	}

	changer->parameter_type = MXLV_CHG_DEICE;

	mx_status = (*get_parameter_fn)( changer );

	return mx_status;
}

