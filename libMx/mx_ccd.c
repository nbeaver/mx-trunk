/*
 * Name:    mx_ccd.c
 *
 * Purpose: MX function library of generic CCD control operations.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003 Illinois Institute of Technology
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
#include "mx_ccd.h"

/*=======================================================================*/

/* This function is a utility function to consolidate all of the pointer
 * mangling that often has to happen at the beginning of an mx_ccd_
 * function.  The parameter 'calling_fname' is passed so that error messages
 * will appear with the name of the calling function.
 */

MX_EXPORT mx_status_type
mx_ccd_get_pointers( MX_RECORD *ccd_record,
			MX_CCD **ccd,
			MX_CCD_FUNCTION_LIST **function_list_ptr,
			const char *calling_fname )
{
	static const char fname[] = "mx_ccd_get_pointers()";

	if ( ccd_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The CCD_record pointer passed by '%s' was NULL",
			calling_fname );
	}

	if ( ccd_record->mx_class != MXC_CCD ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"The record '%s' passed by '%s' is not a CCD.",
			ccd_record->name, calling_fname );
	}

	if ( ccd != (MX_CCD **) NULL ) {
		*ccd = (MX_CCD *) ccd_record->record_class_struct;

		if ( *ccd == (MX_CCD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_CCD pointer for record '%s' "
			"passed by '%s' is NULL",
				ccd_record->name, calling_fname );
		}
	}

	if ( function_list_ptr != (MX_CCD_FUNCTION_LIST **) NULL ) {
		*function_list_ptr = (MX_CCD_FUNCTION_LIST *)
			ccd_record->class_specific_function_list;

		if ( *function_list_ptr ==
			(MX_CCD_FUNCTION_LIST *) NULL )
		{
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_CCD_FUNCTION_LIST pointer for "
			"record '%s' passed by '%s' is NULL.",
				ccd_record->name, calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mx_ccd_start( MX_RECORD *ccd_record )
{
	static const char fname[] = "mx_ccd_start()";

	MX_CCD *ccd;
	MX_CCD_FUNCTION_LIST *function_list;
	mx_status_type ( *start_fn ) ( MX_CCD * );
	mx_status_type mx_status;

	mx_status = mx_ccd_get_pointers( ccd_record,
				&ccd, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	start_fn = function_list->start;

	if ( start_fn == NULL ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Starting '%s' CCD '%s' is not yet implemented.",
			mx_get_driver_name( ccd_record ),
			ccd_record->name );
	}

	/* A negative preset time means ignore the preset. */

	ccd->preset_time = -1.0;

	mx_status = (*start_fn)( ccd );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_ccd_start_for_preset_time( MX_RECORD *ccd_record, double time_in_seconds )
{
	static const char fname[] = "mx_ccd_start_for_preset_time()";

	MX_CCD *ccd;
	MX_CCD_FUNCTION_LIST *function_list;
	mx_status_type ( *start_fn ) ( MX_CCD * );
	mx_status_type mx_status;

	mx_status = mx_ccd_get_pointers( ccd_record,
				&ccd, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	start_fn = function_list->start;

	if ( start_fn == NULL ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Starting '%s' CCD '%s' is not yet implemented.",
			mx_get_driver_name( ccd_record ),
			ccd_record->name );
	}

	ccd->preset_time = time_in_seconds;

	mx_status = (*start_fn)( ccd );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_ccd_stop( MX_RECORD *ccd_record )
{
	static const char fname[] = "mx_ccd_stop()";

	MX_CCD *ccd;
	MX_CCD_FUNCTION_LIST *function_list;
	mx_status_type ( *stop_fn ) ( MX_CCD * );
	mx_status_type mx_status;

	mx_status = mx_ccd_get_pointers( ccd_record,
				&ccd, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	stop_fn = function_list->stop;

	if ( stop_fn == NULL ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Stopping '%s' CCD '%s' is not yet implemented.",
			mx_get_driver_name( ccd_record ),
			ccd_record->name );
	}

	mx_status = (*stop_fn)( ccd );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_ccd_get_status( MX_RECORD *ccd_record, unsigned long *status )
{
	static const char fname[] = "mx_ccd_get_status()";

	MX_CCD *ccd;
	MX_CCD_FUNCTION_LIST *function_list;
	mx_status_type ( *get_status_fn ) ( MX_CCD * );
	mx_status_type mx_status;

	mx_status = mx_ccd_get_pointers( ccd_record,
				&ccd, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_status_fn = function_list->get_status;

	if ( get_status_fn == NULL ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Getting the status of '%s' CCD '%s' is not yet implemented.",
			mx_get_driver_name( ccd_record ),
			ccd_record->name );
	}

	mx_status = (*get_status_fn)( ccd );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( status != NULL ) {
		*status = ccd->status;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_ccd_readout( MX_RECORD *ccd_record, unsigned long flags )
{
	static const char fname[] = "mx_ccd_readout()";

	MX_CCD *ccd;
	MX_CCD_FUNCTION_LIST *function_list;
	mx_status_type ( *readout_fn ) ( MX_CCD * );
	mx_status_type mx_status;

	mx_status = mx_ccd_get_pointers( ccd_record,
				&ccd, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	readout_fn = function_list->readout;

	if ( readout_fn == NULL ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Reading out '%s' CCD '%s' is not yet implemented.",
			mx_get_driver_name( ccd_record ),
			ccd_record->name );
	}

	ccd->ccd_flags = flags;

	mx_status = (*readout_fn)( ccd );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_ccd_dezinger( MX_RECORD *ccd_record, unsigned long flags )
{
	static const char fname[] = "mx_ccd_dezinger()";

	MX_CCD *ccd;
	MX_CCD_FUNCTION_LIST *function_list;
	mx_status_type ( *dezinger_fn ) ( MX_CCD * );
	mx_status_type mx_status;

	mx_status = mx_ccd_get_pointers( ccd_record,
				&ccd, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	dezinger_fn = function_list->dezinger;

	if ( dezinger_fn == NULL ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Reading out '%s' CCD '%s' is not yet implemented.",
			mx_get_driver_name( ccd_record ),
			ccd_record->name );
	}

	ccd->ccd_flags = flags;

	mx_status = (*dezinger_fn)( ccd );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_ccd_correct( MX_RECORD *ccd_record )
{
	static const char fname[] = "mx_ccd_correct()";

	MX_CCD *ccd;
	MX_CCD_FUNCTION_LIST *function_list;
	mx_status_type ( *correct_fn ) ( MX_CCD * );
	mx_status_type mx_status;

	mx_status = mx_ccd_get_pointers( ccd_record,
				&ccd, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	correct_fn = function_list->correct;

	if ( correct_fn == NULL ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Reading out '%s' CCD '%s' is not yet implemented.",
			mx_get_driver_name( ccd_record ),
			ccd_record->name );
	}

	mx_status = (*correct_fn)( ccd );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_ccd_writefile( MX_RECORD *ccd_record, char *filename, unsigned long flags )
{
	static const char fname[] = "mx_ccd_writefile()";

	MX_CCD *ccd;
	MX_CCD_FUNCTION_LIST *function_list;
	mx_status_type ( *writefile_fn ) ( MX_CCD * );
	mx_status_type mx_status;

	mx_status = mx_ccd_get_pointers( ccd_record,
				&ccd, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	writefile_fn = function_list->writefile;

	if ( writefile_fn == NULL ) {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Reading out '%s' CCD '%s' is not yet implemented.",
			mx_get_driver_name( ccd_record ),
			ccd_record->name );
	}

	ccd->ccd_flags = flags;

	mx_strncpy( ccd->writefile_name, filename, MXU_FILENAME_LENGTH );

	mx_status = (*writefile_fn)( ccd );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_ccd_default_get_parameter_handler( MX_CCD *ccd )
{
	static const char fname[] =
		"mx_ccd_default_get_parameter_handler()";

	mx_status_type mx_status;

	switch( ccd->parameter_type ) {
	default:
		mx_status = mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type '%s' (%d) is not supported by the "
		"MX driver for CCD '%s'.",
			mx_get_field_label_string( ccd->record,
					ccd->parameter_type ),
			ccd->parameter_type,
			ccd->record->name );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_ccd_default_set_parameter_handler( MX_CCD *ccd )
{
	static const char fname[] =
		"mx_ccd_default_set_parameter_handler()";

	mx_status_type mx_status;

	switch( ccd->parameter_type ) {
	default:
		mx_status = mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type '%s' (%d) is not supported by the "
		"MX driver for CCD '%s'.",
			mx_get_field_label_string( ccd->record,
					ccd->parameter_type ),
			ccd->parameter_type,
			ccd->record->name );
		break;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_ccd_get_data_frame_size( MX_RECORD *ccd_record, int *x_size, int *y_size )
{
	static const char fname[] = "mx_ccd_get_data_frame_size()";

	MX_CCD *ccd;
	MX_CCD_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_CCD * );
	mx_status_type mx_status;

	mx_status = mx_ccd_get_pointers( ccd_record,
				&ccd, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn =
			mx_ccd_default_get_parameter_handler;
	}

	ccd->parameter_type = MXLV_CCD_DATA_FRAME_SIZE;

	mx_status = (*get_parameter_fn)( ccd );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( x_size != NULL ) {
		*x_size = ccd->data_frame_size[0];
	}
	if ( y_size != NULL ) {
		*y_size = ccd->data_frame_size[1];
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_ccd_get_bin_size( MX_RECORD *ccd_record, int *x_size, int *y_size )
{
	static const char fname[] = "mx_ccd_get_bin_size()";

	MX_CCD *ccd;
	MX_CCD_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_CCD * );
	mx_status_type mx_status;

	mx_status = mx_ccd_get_pointers( ccd_record,
				&ccd, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn =
			mx_ccd_default_get_parameter_handler;
	}

	ccd->parameter_type = MXLV_CCD_BIN_SIZE;

	mx_status = (*get_parameter_fn)( ccd );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( x_size != NULL ) {
		*x_size = ccd->bin_size[0];
	}
	if ( y_size != NULL ) {
		*y_size = ccd->bin_size[1];
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_ccd_set_bin_size( MX_RECORD *ccd_record, int x_size, int y_size )
{
	static const char fname[] = "mx_ccd_get_bin_size()";

	MX_CCD *ccd;
	MX_CCD_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter_fn ) ( MX_CCD * );
	mx_status_type mx_status;

	mx_status = mx_ccd_get_pointers( ccd_record,
				&ccd, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter_fn = function_list->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn =
			mx_ccd_default_set_parameter_handler;
	}

	ccd->parameter_type = MXLV_CCD_BIN_SIZE;

	ccd->bin_size[0] = x_size;
	ccd->bin_size[1] = y_size;

	mx_status = (*set_parameter_fn)( ccd );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_ccd_read_header_variable( MX_RECORD *ccd_record,
				char *header_variable_name,
				char *header_variable_contents,
				size_t maximum_header_contents_length )
{
	static const char fname[] = "mx_ccd_read_header_variable()";

	MX_CCD *ccd;
	MX_CCD_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter_fn ) ( MX_CCD * );
	mx_status_type ( *set_parameter_fn ) ( MX_CCD * );
	mx_status_type mx_status;

	mx_status = mx_ccd_get_pointers( ccd_record,
				&ccd, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_parameter_fn = function_list->get_parameter;

	if ( get_parameter_fn == NULL ) {
		get_parameter_fn = mx_ccd_default_get_parameter_handler;
	}

	set_parameter_fn = function_list->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn = mx_ccd_default_set_parameter_handler;
	}

	ccd->parameter_type = MXLV_CCD_HEADER_VARIABLE_NAME;

	mx_strncpy( ccd->header_variable_name, header_variable_name,
				MXU_CCD_HEADER_NAME_LENGTH );

	mx_status = (*set_parameter_fn)( ccd );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ccd->parameter_type = MXLV_CCD_HEADER_VARIABLE_CONTENTS;

	mx_status = (*get_parameter_fn)( ccd );

	mx_strncpy( header_variable_contents, ccd->header_variable_contents,
				maximum_header_contents_length );

	return mx_status;
}

MX_EXPORT mx_status_type
mx_ccd_write_header_variable( MX_RECORD *ccd_record,
				char *header_variable_name,
				char *header_variable_contents )
{
	static const char fname[] = "mx_ccd_write_header_variable()";

	MX_CCD *ccd;
	MX_CCD_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter_fn ) ( MX_CCD * );
	mx_status_type mx_status;

	mx_status = mx_ccd_get_pointers( ccd_record,
				&ccd, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_parameter_fn = function_list->set_parameter;

	if ( set_parameter_fn == NULL ) {
		set_parameter_fn = mx_ccd_default_set_parameter_handler;
	}

	ccd->parameter_type = MXLV_CCD_HEADER_VARIABLE_NAME;

	mx_strncpy( ccd->header_variable_name, header_variable_name,
				MXU_CCD_HEADER_NAME_LENGTH );

	mx_status = (*set_parameter_fn)( ccd );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	ccd->parameter_type = MXLV_CCD_HEADER_VARIABLE_CONTENTS;

	mx_strncpy( ccd->header_variable_contents, header_variable_contents,
				MXU_BUFFER_LENGTH );

	mx_status = (*set_parameter_fn)( ccd );

	return mx_status;
}

