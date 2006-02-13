/*
 * Name:    mx_ptz.c
 *
 * Purpose: MX function library for Pan/Tilt/Zoom camera supports.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2005-2006 Illinois Institute of Technology
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
mx_ptz_command( MX_RECORD *record, uint32_t command )
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
mx_ptz_get_status( MX_RECORD *record, uint32_t *status )
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
			uint32_t *parameter_value )
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
		*parameter_value = ptz->parameter_value[0];
	}

	return mx_status;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_ptz_set_parameter( MX_RECORD *record,
			int parameter_type,
			uint32_t parameter_value )
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
	ptz->parameter_value[0] = parameter_value;

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
	}

#if defined(__BORLANDC__)
	return MX_SUCCESSFUL_RESULT;
#endif
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
	}

#if defined(__BORLANDC__)
	return MX_SUCCESSFUL_RESULT;
#endif
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
	}

#if defined(__BORLANDC__)
	return MX_SUCCESSFUL_RESULT;
#endif
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mx_ptz_pan_left( MX_RECORD *ptz_record )
{
	static const char fname[] = "mx_ptz_pan_left()";

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

	ptz->command = MXF_PTZ_PAN_LEFT;

	mx_status = (*command_fn)( ptz );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_ptz_pan_right( MX_RECORD *ptz_record )
{
	static const char fname[] = "mx_ptz_pan_right()";

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

	ptz->command = MXF_PTZ_PAN_RIGHT;

	mx_status = (*command_fn)( ptz );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_ptz_pan_stop( MX_RECORD *ptz_record )
{
	static const char fname[] = "mx_ptz_pan_stop()";

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

	ptz->command = MXF_PTZ_PAN_STOP;

	mx_status = (*command_fn)( ptz );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_ptz_get_pan( MX_RECORD *ptz_record, int32_t *pan_value )
{
	static const char fname[] = "mx_ptz_get_pan()";

	MX_PAN_TILT_ZOOM *ptz;
	MX_PAN_TILT_ZOOM_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_PAN_TILT_ZOOM * );
	mx_status_type mx_status;

	mx_status = mx_ptz_get_pointers( ptz_record,
					&ptz, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ){
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"get_parameter function ptr for MX_PAN_TILT_ZOOM ptr 0x%p is NULL.",
			ptz );
	}

	ptz->parameter_type = MXF_PTZ_PAN_POSITION;

	mx_status = (*get_parameter_fn)( ptz );

	if ( pan_value != (int32_t *) NULL ) {
		*pan_value = ptz->pan_position;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_ptz_set_pan( MX_RECORD *ptz_record, int32_t pan_value )
{
	static const char fname[] = "mx_ptz_set_pan()";

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

	ptz->parameter_type = MXF_PTZ_PAN_DESTINATION;
	ptz->pan_destination = pan_value;

	mx_status = (*set_parameter_fn)( ptz );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_ptz_get_pan_speed( MX_RECORD *ptz_record, uint32_t *pan_speed )
{
	static const char fname[] = "mx_ptz_get_pan_speed()";

	MX_PAN_TILT_ZOOM *ptz;
	MX_PAN_TILT_ZOOM_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_PAN_TILT_ZOOM * );
	mx_status_type mx_status;

	mx_status = mx_ptz_get_pointers( ptz_record,
					&ptz, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ){
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"get_parameter function ptr for MX_PAN_TILT_ZOOM ptr 0x%p is NULL.",
			ptz );
	}

	ptz->parameter_type = MXF_PTZ_PAN_SPEED;

	mx_status = (*get_parameter_fn)( ptz );

	if ( pan_speed != (uint32_t *) NULL ) {
		*pan_speed = ptz->pan_speed;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_ptz_set_pan_speed( MX_RECORD *ptz_record, uint32_t pan_speed )
{
	static const char fname[] = "mx_ptz_set_pan_speed()";

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

	ptz->parameter_type = MXF_PTZ_PAN_SPEED;
	ptz->pan_speed = pan_speed;

	mx_status = (*set_parameter_fn)( ptz );

	return mx_status;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mx_ptz_tilt_up( MX_RECORD *ptz_record )
{
	static const char fname[] = "mx_ptz_tilt_up()";

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

	ptz->command = MXF_PTZ_TILT_UP;

	mx_status = (*command_fn)( ptz );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_ptz_tilt_down( MX_RECORD *ptz_record )
{
	static const char fname[] = "mx_ptz_tilt_down()";

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

	ptz->command = MXF_PTZ_TILT_DOWN;

	mx_status = (*command_fn)( ptz );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_ptz_tilt_stop( MX_RECORD *ptz_record )
{
	static const char fname[] = "mx_ptz_tilt_stop()";

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

	ptz->command = MXF_PTZ_TILT_STOP;

	mx_status = (*command_fn)( ptz );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_ptz_get_tilt( MX_RECORD *ptz_record, int32_t *tilt_value )
{
	static const char fname[] = "mx_ptz_get_tilt()";

	MX_PAN_TILT_ZOOM *ptz;
	MX_PAN_TILT_ZOOM_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_PAN_TILT_ZOOM * );
	mx_status_type mx_status;

	mx_status = mx_ptz_get_pointers( ptz_record,
					&ptz, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ){
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"get_parameter function ptr for MX_PAN_TILT_ZOOM ptr 0x%p is NULL.",
			ptz );
	}

	ptz->parameter_type = MXF_PTZ_TILT_POSITION;

	mx_status = (*get_parameter_fn)( ptz );

	if ( tilt_value != (int32_t *) NULL ) {
		*tilt_value = ptz->tilt_position;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_ptz_set_tilt( MX_RECORD *ptz_record, int32_t tilt_value )
{
	static const char fname[] = "mx_ptz_set_tilt()";

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

	ptz->parameter_type = MXF_PTZ_TILT_DESTINATION;
	ptz->tilt_destination = tilt_value;

	mx_status = (*set_parameter_fn)( ptz );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_ptz_get_tilt_speed( MX_RECORD *ptz_record, uint32_t *tilt_speed )
{
	static const char fname[] = "mx_ptz_get_tilt_speed()";

	MX_PAN_TILT_ZOOM *ptz;
	MX_PAN_TILT_ZOOM_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_PAN_TILT_ZOOM * );
	mx_status_type mx_status;

	mx_status = mx_ptz_get_pointers( ptz_record,
					&ptz, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ){
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"get_parameter function ptr for MX_PAN_TILT_ZOOM ptr 0x%p is NULL.",
			ptz );
	}

	ptz->parameter_type = MXF_PTZ_TILT_SPEED;

	mx_status = (*get_parameter_fn)( ptz );

	if ( tilt_speed != (uint32_t *) NULL ) {
		*tilt_speed = ptz->tilt_speed;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_ptz_set_tilt_speed( MX_RECORD *ptz_record, uint32_t tilt_speed )
{
	static const char fname[] = "mx_ptz_set_tilt_speed()";

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

	ptz->parameter_type = MXF_PTZ_TILT_SPEED;
	ptz->tilt_speed = tilt_speed;

	mx_status = (*set_parameter_fn)( ptz );

	return mx_status;
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
mx_ptz_get_zoom( MX_RECORD *ptz_record, uint32_t *zoom_value )
{
	static const char fname[] = "mx_ptz_get_zoom()";

	MX_PAN_TILT_ZOOM *ptz;
	MX_PAN_TILT_ZOOM_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_PAN_TILT_ZOOM * );
	mx_status_type mx_status;

	mx_status = mx_ptz_get_pointers( ptz_record,
					&ptz, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ){
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"get_parameter function ptr for MX_PAN_TILT_ZOOM ptr 0x%p is NULL.",
			ptz );
	}

	ptz->parameter_type = MXF_PTZ_ZOOM_POSITION;

	mx_status = (*get_parameter_fn)( ptz );

	if ( zoom_value != (uint32_t *) NULL ) {
		*zoom_value = ptz->zoom_position;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_ptz_set_zoom( MX_RECORD *ptz_record, uint32_t zoom_value )
{
	static const char fname[] = "mx_ptz_set_zoom()";

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

	ptz->parameter_type = MXF_PTZ_ZOOM_DESTINATION;
	ptz->zoom_destination = zoom_value;

	mx_status = (*set_parameter_fn)( ptz );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_ptz_get_zoom_speed( MX_RECORD *ptz_record, uint32_t *zoom_speed )
{
	static const char fname[] = "mx_ptz_get_zoom_speed()";

	MX_PAN_TILT_ZOOM *ptz;
	MX_PAN_TILT_ZOOM_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_PAN_TILT_ZOOM * );
	mx_status_type mx_status;

	mx_status = mx_ptz_get_pointers( ptz_record,
					&ptz, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ){
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"get_parameter function ptr for MX_PAN_TILT_ZOOM ptr 0x%p is NULL.",
			ptz );
	}

	ptz->parameter_type = MXF_PTZ_ZOOM_SPEED;

	mx_status = (*get_parameter_fn)( ptz );

	if ( zoom_speed != (uint32_t *) NULL ) {
		*zoom_speed = ptz->zoom_speed;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_ptz_set_zoom_speed( MX_RECORD *ptz_record, uint32_t zoom_speed )
{
	static const char fname[] = "mx_ptz_set_zoom_speed()";

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

	ptz->parameter_type = MXF_PTZ_ZOOM_SPEED;
	ptz->zoom_speed = zoom_speed;

	mx_status = (*set_parameter_fn)( ptz );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_ptz_zoom_off( MX_RECORD *ptz_record )
{
	static const char fname[] = "mx_ptz_zoom_off()";

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

	ptz->command = MXF_PTZ_ZOOM_OFF;
	ptz->zoom_on = FALSE;

	mx_status = (*command_fn)( ptz );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_ptz_zoom_on( MX_RECORD *ptz_record )
{
	static const char fname[] = "mx_ptz_zoom_off()";

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

	ptz->command = MXF_PTZ_ZOOM_ON;
	ptz->zoom_on = TRUE;

	mx_status = (*command_fn)( ptz );

	return mx_status;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mx_ptz_focus_manual( MX_RECORD *ptz_record )
{
	static const char fname[] = "mx_ptz_focus_manual()";

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

	ptz->command = MXF_PTZ_FOCUS_MANUAL;
	ptz->focus_auto = FALSE;

	mx_status = (*command_fn)( ptz );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_ptz_focus_auto( MX_RECORD *ptz_record )
{
	static const char fname[] = "mx_ptz_focus_auto()";

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

	ptz->command = MXF_PTZ_FOCUS_AUTO;
	ptz->focus_auto = TRUE;

	mx_status = (*command_fn)( ptz );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_ptz_focus_far( MX_RECORD *ptz_record )
{
	static const char fname[] = "mx_ptz_focus_far()";

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

	ptz->command = MXF_PTZ_FOCUS_FAR;

	mx_status = (*command_fn)( ptz );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_ptz_focus_near( MX_RECORD *ptz_record )
{
	static const char fname[] = "mx_ptz_focus_near()";

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

	ptz->command = MXF_PTZ_FOCUS_NEAR;

	mx_status = (*command_fn)( ptz );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_ptz_focus_stop( MX_RECORD *ptz_record )
{
	static const char fname[] = "mx_ptz_focus_stop()";

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

	ptz->command = MXF_PTZ_FOCUS_STOP;

	mx_status = (*command_fn)( ptz );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_ptz_get_focus( MX_RECORD *ptz_record, uint32_t *focus_value )
{
	static const char fname[] = "mx_ptz_get_focus()";

	MX_PAN_TILT_ZOOM *ptz;
	MX_PAN_TILT_ZOOM_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_PAN_TILT_ZOOM * );
	mx_status_type mx_status;

	mx_status = mx_ptz_get_pointers( ptz_record,
					&ptz, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ){
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"get_parameter function ptr for MX_PAN_TILT_ZOOM ptr 0x%p is NULL.",
			ptz );
	}

	ptz->parameter_type = MXF_PTZ_FOCUS_POSITION;

	mx_status = (*get_parameter_fn)( ptz );

	if ( focus_value != (uint32_t *) NULL ) {
		*focus_value = ptz->focus_position;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_ptz_set_focus( MX_RECORD *ptz_record, uint32_t focus_value )
{
	static const char fname[] = "mx_ptz_set_focus()";

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

	ptz->parameter_type = MXF_PTZ_FOCUS_DESTINATION;
	ptz->focus_destination = focus_value;

	mx_status = (*set_parameter_fn)( ptz );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_ptz_get_focus_speed( MX_RECORD *ptz_record, uint32_t *focus_speed )
{
	static const char fname[] = "mx_ptz_get_focus_speed()";

	MX_PAN_TILT_ZOOM *ptz;
	MX_PAN_TILT_ZOOM_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_PAN_TILT_ZOOM * );
	mx_status_type mx_status;

	mx_status = mx_ptz_get_pointers( ptz_record,
					&ptz, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ){
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"get_parameter function ptr for MX_PAN_TILT_ZOOM ptr 0x%p is NULL.",
			ptz );
	}

	ptz->parameter_type = MXF_PTZ_FOCUS_SPEED;

	mx_status = (*get_parameter_fn)( ptz );

	if ( focus_speed != (uint32_t *) NULL ) {
		*focus_speed = ptz->focus_speed;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_ptz_set_focus_speed( MX_RECORD *ptz_record, uint32_t focus_speed )
{
	static const char fname[] = "mx_ptz_set_focus_speed()";

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

	ptz->parameter_type = MXF_PTZ_FOCUS_SPEED;
	ptz->focus_speed = focus_speed;

	mx_status = (*set_parameter_fn)( ptz );

	return mx_status;
}

