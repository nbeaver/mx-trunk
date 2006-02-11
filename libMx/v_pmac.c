/*
 * Name:    v_pmac.c
 *
 * Purpose: Support for PMAC motor controller variables.
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

#define MXV_PMAC_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <float.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_variable.h"
#include "i_pmac.h"
#include "v_pmac.h"

MX_RECORD_FUNCTION_LIST mxv_pmac_record_function_list = {
	mx_variable_initialize_type,
	mxv_pmac_create_record_structures,
	mxv_pmac_finish_record_initialization,
};

MX_VARIABLE_FUNCTION_LIST mxv_pmac_variable_function_list = {
	mxv_pmac_send_variable,
	mxv_pmac_receive_variable
};

/*---*/

MX_RECORD_FIELD_DEFAULTS mxv_pmac_int32_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXV_PMAC_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_INT32_VARIABLE_STANDARD_FIELDS
};

mx_length_type mxv_pmac_int32_num_record_fields
	= sizeof( mxv_pmac_int32_field_defaults )
	/ sizeof( mxv_pmac_int32_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_pmac_int32_rfield_def_ptr
		= &mxv_pmac_int32_field_defaults[0];

/*---*/

MX_RECORD_FIELD_DEFAULTS mxv_pmac_uint32_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXV_PMAC_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_UINT32_VARIABLE_STANDARD_FIELDS
};

mx_length_type mxv_pmac_uint32_num_record_fields
	= sizeof( mxv_pmac_uint32_field_defaults )
	/ sizeof( mxv_pmac_uint32_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_pmac_uint32_rfield_def_ptr
		= &mxv_pmac_uint32_field_defaults[0];

/*---*/

MX_RECORD_FIELD_DEFAULTS mxv_pmac_double_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXV_PMAC_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_DOUBLE_VARIABLE_STANDARD_FIELDS
};

mx_length_type mxv_pmac_double_num_record_fields
	= sizeof( mxv_pmac_double_field_defaults )
	/ sizeof( mxv_pmac_double_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_pmac_double_rfield_def_ptr
		= &mxv_pmac_double_field_defaults[0];

static mx_status_type
mxv_pmac_get_pointers( MX_VARIABLE *variable,
			MX_PMAC_VARIABLE **pmac_variable,
			MX_PMAC **pmac,
			const char *calling_fname )
{
	const char fname[] = "mxv_pmac_get_pointers()";

	MX_RECORD *pmac_record;
	MX_PMAC_VARIABLE *pmac_variable_ptr;

	if ( variable == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_VARIABLE pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( variable->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_RECORD pointer for MX_VARIABLE pointer passed by '%s' was NULL.",
			calling_fname );
	}

	pmac_variable_ptr = (MX_PMAC_VARIABLE *)
				variable->record->record_type_struct;

	if ( pmac_variable != (MX_PMAC_VARIABLE **) NULL ) {
		*pmac_variable = pmac_variable_ptr;
	}

	pmac_record = pmac_variable_ptr->pmac_record;

	if ( pmac_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"MX_PMAC pointer for PMAC digital input record '%s' passed by '%s' is NULL.",
			variable->record->name, calling_fname );
	}

	if ( pmac_record->mx_type != MXI_GEN_PMAC ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
"pmac_record '%s' for PMAC variable '%s' is not a PMAC record.  "
"Instead, it is a '%s' record.",
			pmac_record->name, variable->record->name,
			mx_get_driver_name( pmac_record ) );
	}

	if ( pmac != (MX_PMAC **) NULL ) {
		*pmac = (MX_PMAC *) pmac_record->record_type_struct;
	}

	return MX_SUCCESSFUL_RESULT;
}

/********************************************************************/

MX_EXPORT mx_status_type
mxv_pmac_create_record_structures( MX_RECORD *record )
{
	const char fname[] = "mxv_pmac_create_record_structures()";

	MX_VARIABLE *variable_struct;
	MX_PMAC_VARIABLE *pmac_variable;

	/* Allocate memory for the necessary structures. */

	variable_struct = (MX_VARIABLE *) malloc( sizeof(MX_VARIABLE) );

	if ( variable_struct == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_VARIABLE structure." );
	}

	pmac_variable = (MX_PMAC_VARIABLE *)
				malloc( sizeof(MX_PMAC_VARIABLE) );

	if ( pmac_variable == (MX_PMAC_VARIABLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_PMAC_VARIABLE structure." );
	}

	/* Now set up the necessary pointers. */

	variable_struct->record = record;

	record->record_superclass_struct = variable_struct;
	record->record_class_struct = NULL;
	record->record_type_struct = pmac_variable;

	record->superclass_specific_function_list =
				&mxv_pmac_variable_function_list;
	record->class_specific_function_list = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_pmac_finish_record_initialization( MX_RECORD *record )
{
	const char fname[] = "mxv_pmac_finish_record_initialization()";

	MX_PMAC_VARIABLE *pmac_variable;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	pmac_variable = (MX_PMAC_VARIABLE *) record->record_type_struct;

	if ( pmac_variable == (MX_PMAC_VARIABLE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_PMAC_VARIABLE pointer for record '%s' is NULL.",
			record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_pmac_send_variable( MX_VARIABLE *variable )
{
	static const char fname[] = "mxv_pmac_send_variable()";

	MX_PMAC_VARIABLE *pmac_variable;
	MX_PMAC *pmac;
	char command[80];
	void *value_ptr;
	int32_t int32_value;
	uint32_t uint32_value;
	double double_value;
	mx_status_type mx_status;

	mx_status = mxv_pmac_get_pointers( variable,
				&pmac_variable, &pmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get_variable_pointer( variable->record, &value_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( variable->record->mx_type ) {
	case MXV_PMA_INT32:
		int32_value = *((int32_t *) value_ptr);

		if ( pmac->num_cards > 1 ) {
			sprintf( command, "@%x%s=%ld",
				pmac_variable->card_number,
				pmac_variable->pmac_variable_name,
				(long) int32_value );
		} else {
			sprintf( command, "%s=%ld",
				pmac_variable->pmac_variable_name,
				(long) int32_value );
		}
		break;
	case MXV_PMA_UINT32:
		uint32_value = *((uint32_t *) value_ptr);

		if ( pmac->num_cards > 1 ) {
			sprintf( command, "@%x%s=%lu",
				pmac_variable->card_number,
				pmac_variable->pmac_variable_name,
				(unsigned long) uint32_value );
		} else {
			sprintf( command, "%s=%lu",
				pmac_variable->pmac_variable_name,
				(unsigned long) uint32_value );
		}
		break;
	case MXV_PMA_DOUBLE:
		double_value = *((double *) value_ptr);

		if ( pmac->num_cards > 1 ) {
			sprintf( command, "@%x%s=%g",
				pmac_variable->card_number,
				pmac_variable->pmac_variable_name,
				double_value );
		} else {
			sprintf( command, "%s=%g",
				pmac_variable->pmac_variable_name,
				double_value );
		}
		break;
	}

	mx_status = mxi_pmac_command( pmac, command, NULL, 0, MXV_PMAC_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxv_pmac_receive_variable( MX_VARIABLE *variable )
{
	static const char fname[] = "mxv_pmac_receive_variable()";

	MX_PMAC_VARIABLE *pmac_variable;
	MX_PMAC *pmac;
	char command[80];
	char response[80];
	void *value_ptr;
	int32_t *int32_ptr;
	uint32_t *uint32_ptr;
	long long_value;
	unsigned long ulong_value;
	double *double_ptr;
	double double_value;
	int num_items;
	mx_status_type mx_status;

	mx_status = mxv_pmac_get_pointers( variable,
				&pmac_variable, &pmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get_variable_pointer( variable->record, &value_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( pmac->num_cards > 1 ) {
		sprintf( command, "@%x%s", pmac_variable->card_number,
				pmac_variable->pmac_variable_name );
	} else {
		sprintf( command, "%s", pmac_variable->pmac_variable_name );
	}

	mx_status = mxi_pmac_command( pmac, command,
				response, sizeof(response), MXV_PMAC_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = 0;

	switch( variable->record->mx_type ) {
	case MXV_PMA_INT32:
		num_items = sscanf( response, "%ld", &long_value );
		break;
	case MXV_PMA_UINT32:
		num_items = sscanf( response, "%lu", &ulong_value );
		break;
	case MXV_PMA_DOUBLE:
		num_items = sscanf( response, "%lg", &double_value );
		break;
	}

	if ( num_items != 1 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
	"Unable to parse the response '%s' by PMAC '%s' to the command '%s'.",
			response, pmac->record->name, command );
	}

	switch( variable->record->mx_type ) {
	case MXV_PMA_INT32:
		int32_ptr = (int32_t *) value_ptr;

		*int32_ptr = long_value;
		break;
	case MXV_PMA_UINT32:
		uint32_ptr = (uint32_t *) value_ptr;

		*uint32_ptr = ulong_value;
		break;
	case MXV_PMA_DOUBLE:
		double_ptr = (double *) value_ptr;

		*double_ptr = double_value;
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

