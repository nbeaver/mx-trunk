/*
 * Name:    mx_sca.c
 *
 * Purpose: MX function library for single channel analyzers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2002, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <limits.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_sca.h"

/*=======================================================================*/

/* This function is a utility function to consolidate all of the pointer
 * mangling that often has to happen at the beginning of an mx_sca_...
 * function.  The parameter 'calling_fname' is passed so that error messages
 * will appear with the name of the calling function.
 */

MX_EXPORT mx_status_type
mx_sca_get_pointers( MX_RECORD *sca_record,
			MX_SCA **sca,
			MX_SCA_FUNCTION_LIST **function_list_ptr,
			const char *calling_fname )
{
	const char fname[] = "mx_sca_get_pointers()";

	if ( sca_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The sca_record pointer passed by '%s' was NULL",
			calling_fname );
	}

	if ( sca_record->mx_class != MXC_SINGLE_CHANNEL_ANALYZER ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
			"The record '%s' passed by '%s' is not an SCA.",
			sca_record->name, calling_fname );
	}

	if ( sca != (MX_SCA **) NULL ) {
		*sca = (MX_SCA *) (sca_record->record_class_struct);

		if ( *sca == (MX_SCA *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_SCA pointer for record '%s' passed by '%s' is NULL",
				sca_record->name, calling_fname );
		}
	}

	if ( function_list_ptr != (MX_SCA_FUNCTION_LIST **) NULL ) {
		*function_list_ptr = (MX_SCA_FUNCTION_LIST *)
				(sca_record->class_specific_function_list);

		if ( *function_list_ptr == (MX_SCA_FUNCTION_LIST *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_SCA_FUNCTION_LIST pointer for record '%s' passed by '%s' is NULL.",
				sca_record->name, calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_sca_get_lower_level( MX_RECORD *sca_record, double *lower_level )
{
	const char fname[] = "mx_sca_get_lower_level()";

	MX_SCA *sca;
	MX_SCA_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter ) ( MX_SCA * );
	mx_status_type status;

	MX_DEBUG( 2,("%s invoked for SCA '%s'", fname, sca_record->name));

	status = mx_sca_get_pointers( sca_record, &sca, &function_list, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	get_parameter = function_list->get_parameter;

	if ( get_parameter == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"get_parameter function ptr for record '%s' is NULL.",
			sca_record->name);
	}

	sca->parameter_type = MXLV_SCA_LOWER_LEVEL;

	status = (*get_parameter)( sca );

	if ( lower_level != NULL ) {
		*lower_level = sca->lower_level;
	}

	return status;
}

MX_EXPORT mx_status_type
mx_sca_set_lower_level( MX_RECORD *sca_record, double lower_level )
{
	const char fname[] = "mx_sca_set_lower_level()";

	MX_SCA *sca;
	MX_SCA_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter ) ( MX_SCA * );
	mx_status_type status;

	MX_DEBUG( 2,("%s invoked for SCA '%s'", fname, sca_record->name));

	status = mx_sca_get_pointers( sca_record, &sca, &function_list, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	set_parameter = function_list->set_parameter;

	if ( set_parameter == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"set_parameter function ptr for record '%s' is NULL.",
			sca_record->name);
	}

	sca->parameter_type = MXLV_SCA_LOWER_LEVEL;

	sca->lower_level = lower_level;

	status = (*set_parameter)( sca );

	return status;
}

MX_EXPORT mx_status_type
mx_sca_get_upper_level( MX_RECORD *sca_record, double *upper_level )
{
	const char fname[] = "mx_sca_get_upper_level()";

	MX_SCA *sca;
	MX_SCA_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter ) ( MX_SCA * );
	mx_status_type status;

	MX_DEBUG( 2,("%s invoked for SCA '%s'", fname, sca_record->name));

	status = mx_sca_get_pointers( sca_record, &sca, &function_list, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	get_parameter = function_list->get_parameter;

	if ( get_parameter == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"get_parameter function ptr for record '%s' is NULL.",
			sca_record->name);
	}

	sca->parameter_type = MXLV_SCA_UPPER_LEVEL;

	status = (*get_parameter)( sca );

	if ( upper_level != NULL ) {
		*upper_level = sca->upper_level;
	}

	return status;
}

MX_EXPORT mx_status_type
mx_sca_set_upper_level( MX_RECORD *sca_record, double upper_level )
{
	const char fname[] = "mx_sca_set_upper_level()";

	MX_SCA *sca;
	MX_SCA_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter ) ( MX_SCA * );
	mx_status_type status;

	MX_DEBUG( 2,("%s invoked for SCA '%s'", fname, sca_record->name));

	status = mx_sca_get_pointers( sca_record, &sca, &function_list, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	set_parameter = function_list->set_parameter;

	if ( set_parameter == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"set_parameter function ptr for record '%s' is NULL.",
			sca_record->name);
	}

	sca->parameter_type = MXLV_SCA_UPPER_LEVEL;

	sca->upper_level = upper_level;

	status = (*set_parameter)( sca );

	return status;
}

MX_EXPORT mx_status_type
mx_sca_get_gain( MX_RECORD *sca_record, double *gain )
{
	const char fname[] = "mx_sca_get_gain()";

	MX_SCA *sca;
	MX_SCA_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter ) ( MX_SCA * );
	mx_status_type status;

	MX_DEBUG( 2,("%s invoked for SCA '%s'", fname, sca_record->name));

	status = mx_sca_get_pointers( sca_record, &sca, &function_list, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	get_parameter = function_list->get_parameter;

	if ( get_parameter == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"get_parameter function ptr for record '%s' is NULL.",
			sca_record->name);
	}

	sca->parameter_type = MXLV_SCA_GAIN;

	status = (*get_parameter)( sca );

	if ( gain != NULL ) {
		*gain = sca->gain;
	}

	return status;
}

MX_EXPORT mx_status_type
mx_sca_set_gain( MX_RECORD *sca_record, double gain )
{
	const char fname[] = "mx_sca_set_gain()";

	MX_SCA *sca;
	MX_SCA_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter ) ( MX_SCA * );
	mx_status_type status;

	MX_DEBUG( 2,("%s invoked for SCA '%s'", fname, sca_record->name));

	status = mx_sca_get_pointers( sca_record, &sca, &function_list, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	set_parameter = function_list->set_parameter;

	if ( set_parameter == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"set_parameter function ptr for record '%s' is NULL.",
			sca_record->name);
	}

	sca->parameter_type = MXLV_SCA_GAIN;

	sca->gain = gain;

	status = (*set_parameter)( sca );

	return status;
}

MX_EXPORT mx_status_type
mx_sca_get_time_constant( MX_RECORD *sca_record, double *time_constant )
{
	const char fname[] = "mx_sca_get_time_constant()";

	MX_SCA *sca;
	MX_SCA_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter ) ( MX_SCA * );
	mx_status_type status;

	MX_DEBUG( 2,("%s invoked for SCA '%s'", fname, sca_record->name));

	status = mx_sca_get_pointers( sca_record, &sca, &function_list, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	get_parameter = function_list->get_parameter;

	if ( get_parameter == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"get_parameter function ptr for record '%s' is NULL.",
			sca_record->name);
	}

	sca->parameter_type = MXLV_SCA_TIME_CONSTANT;

	status = (*get_parameter)( sca );

	if ( time_constant != NULL ) {
		*time_constant = sca->time_constant;
	}

	return status;
}

MX_EXPORT mx_status_type
mx_sca_set_time_constant( MX_RECORD *sca_record, double time_constant )
{
	const char fname[] = "mx_sca_set_time_constant()";

	MX_SCA *sca;
	MX_SCA_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter ) ( MX_SCA * );
	mx_status_type status;

	MX_DEBUG( 2,("%s invoked for SCA '%s'", fname, sca_record->name));

	status = mx_sca_get_pointers( sca_record, &sca, &function_list, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	set_parameter = function_list->set_parameter;

	if ( set_parameter == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"set_parameter function ptr for record '%s' is NULL.",
			sca_record->name);
	}

	sca->parameter_type = MXLV_SCA_TIME_CONSTANT;

	sca->time_constant = time_constant;

	status = (*set_parameter)( sca );

	return status;
}

MX_EXPORT mx_status_type
mx_sca_get_mode( MX_RECORD *sca_record, int *mode )
{
	const char fname[] = "mx_sca_get_mode()";

	MX_SCA *sca;
	MX_SCA_FUNCTION_LIST *function_list;
	mx_status_type ( *get_parameter ) ( MX_SCA * );
	mx_status_type status;

	MX_DEBUG( 2,("%s invoked for SCA '%s'", fname, sca_record->name));

	status = mx_sca_get_pointers( sca_record, &sca, &function_list, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	get_parameter = function_list->get_parameter;

	if ( get_parameter == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"get_parameter function ptr for record '%s' is NULL.",
			sca_record->name);
	}

	sca->parameter_type = MXLV_SCA_MODE;

	status = (*get_parameter)( sca );

	if ( mode != NULL ) {
		*mode = sca->sca_mode;
	}

	return status;
}

MX_EXPORT mx_status_type
mx_sca_set_mode( MX_RECORD *sca_record, int mode )
{
	const char fname[] = "mx_sca_set_mode()";

	MX_SCA *sca;
	MX_SCA_FUNCTION_LIST *function_list;
	mx_status_type ( *set_parameter ) ( MX_SCA * );
	mx_status_type status;

	MX_DEBUG( 2,("%s invoked for SCA '%s'", fname, sca_record->name));

	status = mx_sca_get_pointers( sca_record, &sca, &function_list, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	set_parameter = function_list->set_parameter;

	if ( set_parameter == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"set_parameter function ptr for record '%s' is NULL.",
			sca_record->name);
	}

	sca->parameter_type = MXLV_SCA_MODE;

	sca->sca_mode = mode;

	status = (*set_parameter)( sca );

	return status;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_sca_get_parameter( MX_RECORD *sca_record, int parameter_type )
{
	static const char fname[] = "mx_sca_get_parameter()";

	MX_SCA *sca;
	MX_SCA_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_SCA * );
	mx_status_type status;

	status = mx_sca_get_pointers( sca_record,
					&sca, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	fptr = fl_ptr->get_parameter;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
"The get_parameter function is not supported by the driver for record '%s'.",
			sca_record->name );
	}

	sca->parameter_type = parameter_type;

	status = ( *fptr ) ( sca );

	return status;
}

MX_EXPORT mx_status_type
mx_sca_set_parameter( MX_RECORD *sca_record, int parameter_type )
{
	static const char fname[] = "mx_sca_set_parameter()";

	MX_SCA *sca;
	MX_SCA_FUNCTION_LIST *fl_ptr;
	mx_status_type ( *fptr ) ( MX_SCA * );
	mx_status_type status;

	status = mx_sca_get_pointers( sca_record,
					&sca, &fl_ptr, fname );

	if ( status.code != MXE_SUCCESS )
		return status;

	fptr = fl_ptr->set_parameter;

	if ( fptr == NULL ) {
		return mx_error( MXE_UNSUPPORTED, fname,
"The set_parameter function is not supported by the driver for record '%s'.",
			sca_record->name );
	}

	sca->parameter_type = parameter_type;

	status = ( *fptr ) ( sca );

	return status;
}

/*-----------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mx_sca_default_get_parameter_handler( MX_SCA *sca )
{
	const char fname[] = "mx_sca_default_get_parameter_handler()";

	MX_DEBUG(-2,("%s invoked for SCA '%s', parameter type '%s' (%d).",
		fname, sca->record->name,
		mx_get_field_label_string(sca->record,sca->parameter_type),
		sca->parameter_type));

	switch( sca->parameter_type ) {
	case MXLV_SCA_LOWER_LEVEL:
	case MXLV_SCA_UPPER_LEVEL:
	case MXLV_SCA_GAIN:
	case MXLV_SCA_TIME_CONSTANT:
	case MXLV_SCA_MODE:

		/* None of these cases require any action since the value
		 * is already in the location it needs to be in.
		 */

		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type %d is not supported by this driver.",
			sca->parameter_type );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_sca_default_set_parameter_handler( MX_SCA *sca )
{
	const char fname[] = "mx_sca_default_set_parameter_handler()";

	MX_DEBUG(-2,("%s invoked for SCA '%s', parameter type '%s' (%d).",
		fname, sca->record->name,
		mx_get_field_label_string(sca->record,sca->parameter_type),
		sca->parameter_type));

	switch( sca->parameter_type ) {
	case MXLV_SCA_LOWER_LEVEL:
	case MXLV_SCA_UPPER_LEVEL:
	case MXLV_SCA_GAIN:
	case MXLV_SCA_TIME_CONSTANT:
	case MXLV_SCA_MODE:

		/* This case does not require any special action since the
		 * value is already in the right place by the time we get
		 * to this function.
		 */

		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Parameter type %d is not supported by this driver.",
			sca->parameter_type );
	}

	return MX_SUCCESSFUL_RESULT;
}

