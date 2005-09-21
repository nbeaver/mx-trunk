/*
 * Name:    mx_ptz.c
 *
 * Purpose: MX function library for Pan/Tilt/Zoom camera supports.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_ptz.h"

static mx_status_type
mx_ptz_get_pointers( MX_RECORD *record,
		MX_PAN_TILT_ZOOM **ptz,
		MX_PAN_TILT_ZOOM_FUNCTION_LIST **flist_ptr,
		const char *calling_fname )
{
	static const char fname[] = "mx_ptz_get_pointers()";

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed by '%s' is NULL.",
			calling_fname );
	}

	if ( record->mx_class != MXC_PAN_TILT_ZOOM ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
"The record '%s' passed by '%s' is not a Pan/Tilt/Zoom record.  "
"(superclass = %ld, class = %ld, type = %ld)",
			record->name, calling_fname,
			record->mx_superclass,
			record->mx_class,
			record->mx_type );
	}

	if (ptz != (MX_PAN_TILT_ZOOM **) NULL) {
		*ptz = (MX_PAN_TILT_ZOOM *) record->record_class_struct;

		if ( *ptz == (MX_PAN_TILT_ZOOM *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_PAN_TILT_ZOOM pointer for record '%s' passed by '%s' is NULL.",
				record->name, calling_fname );
		}
	}

	if (flist_ptr != (MX_PAN_TILT_ZOOM_FUNCTION_LIST **) NULL) {
		*flist_ptr = (MX_PAN_TILT_ZOOM_FUNCTION_LIST *)
				record->class_specific_function_list;

		if ( *flist_ptr ==
			(MX_PAN_TILT_ZOOM_FUNCTION_LIST *) NULL )
		{
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_PAN_TILT_ZOOM_FUNCTION_LIST ptr for "
			"record '%s' passed by '%s' is NULL.",
				record->name, calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mx_ptz_command( MX_RECORD *record, unsigned long command )
{
	static const char fname[] = "mx_ptz_command()";

	MX_PAN_TILT_ZOOM *ptz;
	MX_PAN_TILT_ZOOM_FUNCTION_LIST *function_list;
	mx_status_type ( *command_fn ) ( MX_PAN_TILT_ZOOM * );
	mx_status_type mx_status;

	mx_status = mx_ptz_get_pointers( record, &ptz, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	command_fn = function_list->command;

	if ( command_fn == NULL ){
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"command function ptr for MX_PAN_TILT_ZOOM ptr 0x%p is NULL.",
			ptz );
	}

	ptz->command = command;

	mx_status = (*command_fn)( ptz );

	return mx_status;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_ptz_get_status( MX_RECORD *record, unsigned long *status )
{
	static const char fname[] = "mx_ptz_get_status()";

	MX_PAN_TILT_ZOOM *ptz;
	MX_PAN_TILT_ZOOM_FUNCTION_LIST *function_list;
	mx_status_type ( *status_fn ) ( MX_PAN_TILT_ZOOM * );
	mx_status_type mx_status;

	mx_status = mx_ptz_get_pointers( record, &ptz, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	status_fn = function_list->get_status;

	if ( status_fn == NULL ){
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"status function ptr for MX_PAN_TILT_ZOOM ptr 0x%p is NULL.",
			ptz );
	}

	mx_status = (*status_fn)( ptz );

	if ( status != NULL ) {
		*status = ptz->status;
	}

	return mx_status;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_ptz_get_parameter( MX_RECORD *record,
			int parameter_type,
			unsigned long *parameter_value )
{
	static const char fname[] = "mx_ptz_get_parameter()";

	MX_PAN_TILT_ZOOM *ptz;
	MX_PAN_TILT_ZOOM_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_PAN_TILT_ZOOM * );
	mx_status_type mx_status;

	mx_status = mx_ptz_get_pointers( record, &ptz, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ){
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"get_parameter function ptr for MX_PAN_TILT_ZOOM ptr 0x%p is NULL.",
			ptz );
	}

	ptz->parameter_type = parameter_type;

	mx_status = (*get_parameter_fn)( ptz );

	if ( parameter_value != NULL ) {
		*parameter_value = ptz->parameter_value;
	}

	return mx_status;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_ptz_set_parameter( MX_RECORD *record,
			int parameter_type,
			unsigned long parameter_value )
{
	static const char fname[] = "mx_ptz_get_parameter()";

	MX_PAN_TILT_ZOOM *ptz;
	MX_PAN_TILT_ZOOM_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_PAN_TILT_ZOOM * );
	mx_status_type mx_status;

	mx_status = mx_ptz_get_pointers( record, &ptz, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ){
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"get_parameter function ptr for MX_PAN_TILT_ZOOM ptr 0x%p is NULL.",
			ptz );
	}

	ptz->parameter_type = parameter_type;
	ptz->parameter_value = parameter_value;

	mx_status = (*get_parameter_fn)( ptz );

	return mx_status;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_ptz_default_command_handler( MX_PAN_TILT_ZOOM *ptz )
{
	static const char fname[] = "mx_ptz_default_command_handler()";

	MX_DEBUG( 2,("%s invoked for PTZ '%s', command type '%s' (%d).",
		fname, ptz->record->name,
		mx_get_field_label_string(ptz->record,ptz->parameter_type),
		ptz->parameter_type));

	switch( ptz->command ) {
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
"Command type '%s' (%d) is not supported by the MX driver for PTZ '%s'.",
			mx_get_field_label_string( ptz->record,
						ptz->parameter_type ),
			ptz->parameter_type,
			ptz->record->name );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_ptz_default_get_parameter_handler( MX_PAN_TILT_ZOOM *ptz )
{
	static const char fname[] = "mx_ptz_default_get_parameter_handler()";

	MX_DEBUG( 2,("%s invoked for PTZ '%s', parameter type '%s' (%d).",
		fname, ptz->record->name,
		mx_get_field_label_string(ptz->record,ptz->parameter_type),
		ptz->parameter_type));

	switch( ptz->parameter_type ) {
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
"Parameter type '%s' (%d) is not supported by the MX driver for PTZ '%s'.",
			mx_get_field_label_string( ptz->record,
						ptz->parameter_type ),
			ptz->parameter_type,
			ptz->record->name );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_ptz_default_set_parameter_handler( MX_PAN_TILT_ZOOM *ptz )
{
	static const char fname[] = "mx_ptz_default_set_parameter_handler()";

	MX_DEBUG( 2,("%s invoked for PTZ '%s', parameter type '%s' (%d).",
		fname, ptz->record->name,
		mx_get_field_label_string(ptz->record,ptz->parameter_type),
		ptz->parameter_type));

	switch( ptz->parameter_type ) {
	default:
		return mx_error( MXE_UNSUPPORTED, fname,
"Parameter type '%s' (%d) is not supported by the MX driver for PTZ '%s'.",
			mx_get_field_label_string( ptz->record,
						ptz->parameter_type ),
			ptz->parameter_type,
			ptz->record->name );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mx_ptz_zoom_in( MX_RECORD *ptz_record )
{
	static const char fname[] = "mx_ptz_zoom_in()";

	MX_PAN_TILT_ZOOM *ptz;
	MX_PAN_TILT_ZOOM_FUNCTION_LIST *function_list;
	mx_status_type ( *command_fn ) ( MX_PAN_TILT_ZOOM * );
	mx_status_type mx_status;

	mx_status = mx_ptz_get_pointers( ptz_record,
					&ptz, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	command_fn = function_list->command;

	if ( command_fn == NULL ){
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"command function ptr for MX_PAN_TILT_ZOOM ptr 0x%p is NULL.",
			ptz );
	}

	ptz->command = MXF_PTZ_ZOOM_IN;

	mx_status = (*command_fn)( ptz );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_ptz_zoom_out( MX_RECORD *ptz_record )
{
	static const char fname[] = "mx_ptz_zoom_out()";

	MX_PAN_TILT_ZOOM *ptz;
	MX_PAN_TILT_ZOOM_FUNCTION_LIST *function_list;
	mx_status_type ( *command_fn ) ( MX_PAN_TILT_ZOOM * );
	mx_status_type mx_status;

	mx_status = mx_ptz_get_pointers( ptz_record,
					&ptz, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	command_fn = function_list->command;

	if ( command_fn == NULL ){
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"command function ptr for MX_PAN_TILT_ZOOM ptr 0x%p is NULL.",
			ptz );
	}

	ptz->command = MXF_PTZ_ZOOM_OUT;

	mx_status = (*command_fn)( ptz );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_ptz_zoom_stop( MX_RECORD *ptz_record )
{
	static const char fname[] = "mx_ptz_zoom_stop()";

	MX_PAN_TILT_ZOOM *ptz;
	MX_PAN_TILT_ZOOM_FUNCTION_LIST *function_list;
	mx_status_type ( *command_fn ) ( MX_PAN_TILT_ZOOM * );
	mx_status_type mx_status;

	mx_status = mx_ptz_get_pointers( ptz_record,
					&ptz, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	command_fn = function_list->command;

	if ( command_fn == NULL ){
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"command function ptr for MX_PAN_TILT_ZOOM ptr 0x%p is NULL.",
			ptz );
	}

	ptz->command = MXF_PTZ_ZOOM_STOP;

	mx_status = (*command_fn)( ptz );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_ptz_zoom_to( MX_RECORD *ptz_record,
		unsigned long zoom_value )
{
	static const char fname[] = "mx_ptz_zoom_stop()";

	MX_PAN_TILT_ZOOM *ptz;
	MX_PAN_TILT_ZOOM_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter_fn ) ( MX_PAN_TILT_ZOOM * );
	mx_status_type mx_status;

	mx_status = mx_ptz_get_pointers( ptz_record,
					&ptz, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter_fn = function_list->set_parameter;

	if ( set_parameter_fn == NULL ){
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"set_parameter function ptr for MX_PAN_TILT_ZOOM ptr 0x%p is NULL.",
			ptz );
	}

	ptz->parameter_type = MXF_PTZ_ZOOM_TO;
	ptz->parameter_value = zoom_value;

	mx_status = (*set_parameter_fn)( ptz );

	return mx_status;
}

