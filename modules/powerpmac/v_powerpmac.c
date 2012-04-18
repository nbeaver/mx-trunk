/*
 * Name:    v_powerpmac.c
 *
 * Purpose: Support for Power PMAC motor controller variables.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXV_POWERPMAC_DEBUG	TRUE

#include <stdio.h>
#include "mxconfig.h"

#if HAVE_POWERPMAC_LIBRARY

#include <stdlib.h>
#include <math.h>
#include <float.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_variable.h"
#include "i_powerpmac.h"
#include "v_powerpmac.h"

MX_RECORD_FUNCTION_LIST mxv_powerpmac_record_function_list = {
	mx_variable_initialize_driver,
	mxv_powerpmac_create_record_structures,
	mxv_powerpmac_finish_record_initialization,
};

MX_VARIABLE_FUNCTION_LIST mxv_powerpmac_variable_function_list = {
	mxv_powerpmac_send_variable,
	mxv_powerpmac_receive_variable
};

/*---*/

MX_RECORD_FIELD_DEFAULTS mxv_powerpmac_long_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXV_POWERPMAC_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_LONG_VARIABLE_STANDARD_FIELDS
};

long mxv_powerpmac_long_num_record_fields
	= sizeof( mxv_powerpmac_long_field_defaults )
	/ sizeof( mxv_powerpmac_long_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_powerpmac_long_rfield_def_ptr
		= &mxv_powerpmac_long_field_defaults[0];

/*---*/

MX_RECORD_FIELD_DEFAULTS mxv_powerpmac_ulong_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXV_POWERPMAC_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_ULONG_VARIABLE_STANDARD_FIELDS
};

long mxv_powerpmac_ulong_num_record_fields
	= sizeof( mxv_powerpmac_ulong_field_defaults )
	/ sizeof( mxv_powerpmac_ulong_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_powerpmac_ulong_rfield_def_ptr
		= &mxv_powerpmac_ulong_field_defaults[0];

/*---*/

MX_RECORD_FIELD_DEFAULTS mxv_powerpmac_double_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXV_POWERPMAC_VARIABLE_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_DOUBLE_VARIABLE_STANDARD_FIELDS
};

long mxv_powerpmac_double_num_record_fields
	= sizeof( mxv_powerpmac_double_field_defaults )
	/ sizeof( mxv_powerpmac_double_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_powerpmac_double_rfield_def_ptr
		= &mxv_powerpmac_double_field_defaults[0];

static mx_status_type
mxv_powerpmac_get_pointers( MX_VARIABLE *variable,
			MX_POWERPMAC_VARIABLE **powerpmac_variable,
			MX_POWERPMAC **powerpmac,
			const char *calling_fname )
{
	static const char fname[] = "mxv_powerpmac_get_pointers()";

	MX_RECORD *powerpmac_record;
	MX_POWERPMAC_VARIABLE *powerpmac_variable_ptr;

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

	powerpmac_variable_ptr = (MX_POWERPMAC_VARIABLE *)
				variable->record->record_type_struct;

	if ( powerpmac_variable != (MX_POWERPMAC_VARIABLE **) NULL ) {
		*powerpmac_variable = powerpmac_variable_ptr;
	}

	powerpmac_record = powerpmac_variable_ptr->powerpmac_record;

	if ( powerpmac_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_POWERPMAC pointer for Power PMAC variable record '%s' "
		"passed by '%s' is NULL.",
			variable->record->name, calling_fname );
	}

	if ( powerpmac_record->mx_type != MXI_CTRL_POWERPMAC ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"powerpmac_record '%s' for Power PMAC variable '%s' is not "
		"a Power PMAC record.  Instead, it is a '%s' record.",
			powerpmac_record->name, variable->record->name,
			mx_get_driver_name( powerpmac_record ) );
	}

	if ( powerpmac != (MX_POWERPMAC **) NULL ) {
		*powerpmac = (MX_POWERPMAC *)
				powerpmac_record->record_type_struct;
	}

	return MX_SUCCESSFUL_RESULT;
}

/********************************************************************/

MX_EXPORT mx_status_type
mxv_powerpmac_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxv_powerpmac_create_record_structures()";

	MX_VARIABLE *variable_struct;
	MX_POWERPMAC_VARIABLE *powerpmac_variable;

	/* Allocate memory for the necessary structures. */

	variable_struct = (MX_VARIABLE *) malloc( sizeof(MX_VARIABLE) );

	if ( variable_struct == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_VARIABLE structure." );
	}

	powerpmac_variable = (MX_POWERPMAC_VARIABLE *)
				malloc( sizeof(MX_POWERPMAC_VARIABLE) );

	if ( powerpmac_variable == (MX_POWERPMAC_VARIABLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_POWERPMAC_VARIABLE structure." );
	}

	/* Now set up the necessary pointers. */

	variable_struct->record = record;

	record->record_superclass_struct = variable_struct;
	record->record_class_struct = NULL;
	record->record_type_struct = powerpmac_variable;

	record->superclass_specific_function_list =
				&mxv_powerpmac_variable_function_list;
	record->class_specific_function_list = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_powerpmac_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
			"mxv_powerpmac_finish_record_initialization()";

	MX_POWERPMAC_VARIABLE *powerpmac_variable;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	powerpmac_variable = (MX_POWERPMAC_VARIABLE *)
					record->record_type_struct;

	if ( powerpmac_variable == (MX_POWERPMAC_VARIABLE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_POWERPMAC_VARIABLE pointer for record '%s' is NULL.",
			record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_powerpmac_send_variable( MX_VARIABLE *variable )
{
	static const char fname[] = "mxv_powerpmac_send_variable()";

	MX_POWERPMAC_VARIABLE *powerpmac_variable;
	MX_POWERPMAC *powerpmac;
	char command[80];
	void *value_ptr;
	long long_value;
	unsigned long ulong_value;
	double double_value;
	mx_status_type mx_status;

	mx_status = mxv_powerpmac_get_pointers( variable,
				&powerpmac_variable, &powerpmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get_variable_pointer( variable->record, &value_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( variable->record->mx_type ) {
	case MXV_PMA_LONG:
		long_value = *((long *) value_ptr);

		sprintf( command, "%s=%ld",
			powerpmac_variable->powerpmac_variable_name,
			long_value );
		break;
	case MXV_PMA_ULONG:
		ulong_value = *((unsigned long *) value_ptr);

		sprintf( command, "%s=%lu",
			powerpmac_variable->powerpmac_variable_name,
			ulong_value );
		break;
	case MXV_PMA_DOUBLE:
		double_value = *((double *) value_ptr);

		sprintf( command, "%s=%g",
			powerpmac_variable->powerpmac_variable_name,
			double_value );
		break;
	}

	mx_status = mxi_powerpmac_command( powerpmac, command,
					NULL, 0, MXV_POWERPMAC_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxv_powerpmac_receive_variable( MX_VARIABLE *variable )
{
	static const char fname[] = "mxv_powerpmac_receive_variable()";

	MX_POWERPMAC_VARIABLE *powerpmac_variable;
	MX_POWERPMAC *powerpmac;
	char command[80];
	char response[80];
	void *value_ptr;
	long *long_ptr;
	long long_value;
	unsigned long *ulong_ptr;
	unsigned long ulong_value;
	double *double_ptr;
	double double_value;
	char *string_ptr;
	int num_items;
	mx_status_type mx_status;

	mx_status = mxv_powerpmac_get_pointers( variable,
				&powerpmac_variable, &powerpmac, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get_variable_pointer( variable->record, &value_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, "%s", powerpmac_variable->powerpmac_variable_name );

	mx_status = mxi_powerpmac_command( powerpmac, command,
			response, sizeof(response), MXV_POWERPMAC_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* There should be an equals sign (=) in the response.
	 * The value we want follows the equals sign.
	 */

	string_ptr = strchr( response, '=' );

	if ( string_ptr == NULL ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"No equals sign was found in the response '%s' "
		"by Power PMAC '%s' to the command '%s'.",
			response, powerpmac->record->name, command );
	}

	string_ptr++;	/* Skip over the equals sign. */

	/* Now parse the value returned. */

	num_items = 0;

	switch( variable->record->mx_type ) {
	case MXV_PMA_LONG:
		num_items = sscanf( string_ptr, "%ld", &long_value );
		break;
	case MXV_PMA_ULONG:
		num_items = sscanf( string_ptr, "%lu", &ulong_value );
		break;
	case MXV_PMA_DOUBLE:
		num_items = sscanf( string_ptr, "%lg", &double_value );
		break;
	}

	if ( num_items != 1 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Unable to parse the response '%s' by Power PMAC '%s' "
		"to the command '%s'.",
			response, powerpmac->record->name, command );
	}

	switch( variable->record->mx_type ) {
	case MXV_PMA_LONG:
		long_ptr = (long *) value_ptr;

		*long_ptr = long_value;
		break;
	case MXV_PMA_ULONG:
		ulong_ptr = (unsigned long *) value_ptr;

		*ulong_ptr = ulong_value;
		break;
	case MXV_PMA_DOUBLE:
		double_ptr = (double *) value_ptr;

		*double_ptr = double_value;
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

#endif /* HAVE_POWERPMAC_LIBRARY */
