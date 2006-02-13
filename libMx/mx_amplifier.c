/*
 * Name:    mx_amplifier.c
 *
 * Purpose: MX function library of generic amplifier operations.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2003, 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_amplifier.h"

/*=======================================================================*/

/* This function is a utility function to consolidate all of the pointer
 * mangling that often has to happen at the beginning of an mx_amplifier_...
 * function.  The parameter 'calling_fname' is passed so that error messages
 * will appear with the name of the calling function.
 */

static mx_status_type
mx_amplifier_get_pointers( MX_RECORD *amplifier_record,
			MX_AMPLIFIER **amplifier,
			MX_AMPLIFIER_FUNCTION_LIST **function_list_ptr,
			const char *calling_fname )
{
	static const char fname[] = "mx_amplifier_get_pointers()";

	if ( amplifier_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The amplifier_record pointer passed by '%s' was NULL",
			calling_fname );
	}

	if ( amplifier_record->mx_class != MXC_AMPLIFIER ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
			"The record '%s' passed by '%s' is not a amplifier.",
			amplifier_record->name, calling_fname );
	}

	if ( amplifier != (MX_AMPLIFIER **) NULL ) {
		*amplifier = (MX_AMPLIFIER *)
				(amplifier_record->record_class_struct);

		if ( *amplifier == (MX_AMPLIFIER *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_AMPLIFIER pointer for record '%s' passed by '%s' is NULL",
				amplifier_record->name, calling_fname );
		}
	}

	if ( function_list_ptr != (MX_AMPLIFIER_FUNCTION_LIST **) NULL ) {
		*function_list_ptr = (MX_AMPLIFIER_FUNCTION_LIST *)
			(amplifier_record->class_specific_function_list);

		if (*function_list_ptr == (MX_AMPLIFIER_FUNCTION_LIST *) NULL){
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_AMPLIFIER_FUNCTION_LIST pointer for record '%s' passed by '%s' is NULL.",
				amplifier_record->name, calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mx_amplifier_get_gain( MX_RECORD *amplifier_record, double *gain )
{
	static const char fname[] = "mx_amplifier_get_gain()";

	MX_AMPLIFIER *amplifier;
	MX_AMPLIFIER_FUNCTION_LIST *function_list;
	mx_status_type ( *get_gain_fn ) ( MX_AMPLIFIER * );
	mx_status_type mx_status;

	mx_status = mx_amplifier_get_pointers( amplifier_record,
					&amplifier, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_gain_fn = function_list->get_gain;

	if ( get_gain_fn != NULL ) {
		mx_status = (*get_gain_fn)( amplifier );
	}

	if ( gain != NULL ) {
		*gain = amplifier->gain;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_amplifier_set_gain( MX_RECORD *amplifier_record, double gain )
{
	static const char fname[] = "mx_amplifier_set_gain()";

	MX_AMPLIFIER *amplifier;
	MX_AMPLIFIER_FUNCTION_LIST *function_list;
	mx_status_type ( *set_gain_fn ) ( MX_AMPLIFIER * );
	mx_status_type mx_status;

	mx_status = mx_amplifier_get_pointers( amplifier_record,
					&amplifier, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_gain_fn = function_list->set_gain;

	amplifier->gain = gain;

	if ( set_gain_fn != NULL ) {
		mx_status = (*set_gain_fn)( amplifier );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_amplifier_get_offset( MX_RECORD *amplifier_record, double *offset )
{
	static const char fname[] = "mx_amplifier_get_offset()";

	MX_AMPLIFIER *amplifier;
	MX_AMPLIFIER_FUNCTION_LIST *function_list;
	mx_status_type ( *get_offset_fn ) ( MX_AMPLIFIER * );
	mx_status_type mx_status;

	mx_status = mx_amplifier_get_pointers( amplifier_record,
					&amplifier, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_offset_fn = function_list->get_offset;

	if ( get_offset_fn != NULL ) {
		mx_status = (*get_offset_fn)( amplifier );
	}

	if ( offset != NULL ) {
		*offset = amplifier->offset;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_amplifier_set_offset( MX_RECORD *amplifier_record, double offset )
{
	static const char fname[] = "mx_amplifier_set_offset()";

	MX_AMPLIFIER *amplifier;
	MX_AMPLIFIER_FUNCTION_LIST *function_list;
	mx_status_type ( *set_offset_fn ) ( MX_AMPLIFIER * );
	mx_status_type mx_status;

	mx_status = mx_amplifier_get_pointers( amplifier_record,
					&amplifier, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_offset_fn = function_list->set_offset;

	amplifier->offset = offset;

	if ( set_offset_fn != NULL ) {
		mx_status = (*set_offset_fn)( amplifier );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_amplifier_get_time_constant( MX_RECORD *amplifier_record,
						double *time_constant )
{
	static const char fname[] = "mx_amplifier_get_time_constant()";

	MX_AMPLIFIER *amplifier;
	MX_AMPLIFIER_FUNCTION_LIST *function_list;
	mx_status_type ( *get_time_constant_fn ) ( MX_AMPLIFIER * );
	mx_status_type mx_status;

	mx_status = mx_amplifier_get_pointers( amplifier_record,
					&amplifier, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	get_time_constant_fn = function_list->get_time_constant;

	if ( get_time_constant_fn != NULL ) {
		mx_status = (*get_time_constant_fn)( amplifier );
	}

	if ( time_constant != NULL ) {
		*time_constant = amplifier->time_constant;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mx_amplifier_set_time_constant( MX_RECORD *amplifier_record,
						double time_constant )
{
	static const char fname[] = "mx_amplifier_set_time_constant()";

	MX_AMPLIFIER *amplifier;
	MX_AMPLIFIER_FUNCTION_LIST *function_list;
	mx_status_type ( *set_time_constant_fn ) ( MX_AMPLIFIER * );
	mx_status_type mx_status;

	mx_status = mx_amplifier_get_pointers( amplifier_record,
					&amplifier, &function_list, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	set_time_constant_fn = function_list->set_time_constant;

	amplifier->time_constant = time_constant;

	if ( set_time_constant_fn != NULL ) {
		mx_status = (*set_time_constant_fn)( amplifier );
	}

	return mx_status;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_amplifier_get_parameter( MX_RECORD *amplifier_record, int parameter_type )
{
	static const char fname[] = "mx_amplifier_get_parameter()";

	MX_AMPLIFIER *amplifier;
	MX_AMPLIFIER_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_AMPLIFIER * );
	mx_status_type status;

	status = mx_amplifier_get_pointers( amplifier_record,
					&amplifier, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	fptr = fl_ptr->get_parameter;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
"The get_parameter function is not supported by the driver for record '%s'.",
			amplifier_record->name );
	}

	amplifier->parameter_type = parameter_type;

	status = ( *fptr ) ( amplifier );

	return status;
}

MX_EXPORT mx_status_type
mx_amplifier_set_parameter( MX_RECORD *amplifier_record, int parameter_type )
{
	static const char fname[] = "mx_amplifier_set_parameter()";

	MX_AMPLIFIER *amplifier;
	MX_AMPLIFIER_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_AMPLIFIER * );
	mx_status_type status;

	status = mx_amplifier_get_pointers( amplifier_record,
					&amplifier, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	fptr = fl_ptr->set_parameter;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
"The set_parameter function is not supported by the driver for record '%s'.",
			amplifier_record->name );
	}

	amplifier->parameter_type = parameter_type;

	status = ( *fptr ) ( amplifier );

	return status;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_amplifier_default_get_parameter_handler( MX_AMPLIFIER *amplifier )
{
	static const char fname[] =
		"mx_amplifier_default_get_parameter_handler()";

	switch( amplifier->parameter_type ) {
	case MXLV_AMP_GAIN_RANGE:

		/* This one does not require anything to be done. */

		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Parameter type '%s' (%d) is not supported "
			"by the MX driver for amplifier '%s'.",
			mx_get_field_label_string( amplifier->record,
						amplifier->parameter_type ),
			(int) amplifier->parameter_type,
			amplifier->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_amplifier_default_set_parameter_handler( MX_AMPLIFIER *amplifier )
{
	static const char fname[] =
		"mx_amplifier_default_set_parameter_handler()";

	mx_status_type mx_status;

	switch( amplifier->parameter_type ) {

	default:
		mx_status = mx_error( MXE_UNSUPPORTED, fname,
			"Parameter type '%s' (%d) is not supported "
			"by the MX driver for amplifier '%s'.",
			mx_get_field_label_string( amplifier->record,
						amplifier->parameter_type ),
			(int) amplifier->parameter_type,
			amplifier->record->name );
		break;
	}

	return mx_status;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_amplifier_get_gain_range( MX_RECORD *amplifier_record,
				double *gain_range_array )
{
	static const char fname[] = "mx_amplifier_get_gain_range()";

	MX_AMPLIFIER *amplifier;
	MX_AMPLIFIER_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_AMPLIFIER * );
	mx_status_type mx_status;

	mx_status = mx_amplifier_get_pointers( amplifier_record,
					&amplifier, &fl_ptr, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fptr = fl_ptr->get_parameter;

	if ( fptr == NULL ) {
		fptr = mx_amplifier_default_get_parameter_handler;
	}

	amplifier->parameter_type = MXLV_AMP_GAIN_RANGE;

	mx_status = ( *fptr ) ( amplifier );

	if ( gain_range_array != NULL ) {
		gain_range_array[0] = amplifier->gain_range[0];
		gain_range_array[1] = amplifier->gain_range[1];
	}

	return mx_status;
}

