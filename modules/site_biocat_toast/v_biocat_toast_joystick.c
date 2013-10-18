/*
 * Name:    v_biocat_toast_joystick.c
 *
 * Purpose: MX variable driver for a driver to enable or disable
 *          the BioCAT "toast" device joystick.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXV_BIOCAT_TOAST_JOYSTICK_DEBUG			FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_socket.h"
#include "mx_process.h"
#include "mx_variable.h"

#include "i_compumotor.h"
#include "v_biocat_toast_joystick.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxv_biocat_toast_joystick_record_function_list = {
	mx_variable_initialize_driver,
	mxv_biocat_toast_joystick_create_record_structures
};

MX_VARIABLE_FUNCTION_LIST mxv_biocat_toast_joystick_variable_function_list = {
	mxv_biocat_toast_joystick_send_variable,
	mxv_biocat_toast_joystick_receive_variable
};

/* Record field defaults for 'biocat_toast_joystick'. */

MX_RECORD_FIELD_DEFAULTS mxv_biocat_toast_joystick_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXV_BIOCAT_TOAST_JOYSTICK_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_ULONG_VARIABLE_STANDARD_FIELDS
};

long mxv_biocat_toast_joystick_num_record_fields
	= sizeof( mxv_biocat_toast_joystick_record_field_defaults )
	/ sizeof( mxv_biocat_toast_joystick_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_biocat_toast_joystick_rfield_def_ptr
		= &mxv_biocat_toast_joystick_record_field_defaults[0];

/*---*/

static mx_status_type
mxv_biocat_toast_joystick_get_pointers( MX_VARIABLE *variable,
			MX_BIOCAT_TOAST_JOYSTICK **biocat_toast_joystick,
			MX_COMPUMOTOR_INTERFACE **compumotor_interface,
			const char *calling_fname )
{
	static const char fname[] =
			"mxv_biocat_toast_joystick_get_pointers()";

	MX_BIOCAT_TOAST_JOYSTICK *biocat_toast_joystick_ptr;
	MX_RECORD *compumotor_interface_record;

	if ( variable == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_VARIABLE pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( variable->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for MX_VARIABLE %p is NULL.",
			variable );
	}

	biocat_toast_joystick_ptr = (MX_BIOCAT_TOAST_JOYSTICK *)
					variable->record->record_type_struct;

	if ( biocat_toast_joystick != (MX_BIOCAT_TOAST_JOYSTICK **) NULL ) {
		*biocat_toast_joystick = biocat_toast_joystick_ptr;
	}

	if ( compumotor_interface != (MX_COMPUMOTOR_INTERFACE **) NULL ) {

		compumotor_interface_record =
			biocat_toast_joystick_ptr->compumotor_interface_record;

		if ( compumotor_interface_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The compumotor_interface_record pointer for "
			"record '%s'.", variable->record->name );
		}

		*compumotor_interface = (MX_COMPUMOTOR_INTERFACE *)
			compumotor_interface_record->record_type_struct;

		if ( (*compumotor_interface)
			== (MX_COMPUMOTOR_INTERFACE *) NULL )
		{
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_COMPUMOTOR_INTERFACE pointer for "
			"record '%s' is NULL.",
				variable->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*---*/

MX_EXPORT mx_status_type
mxv_biocat_toast_joystick_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxv_biocat_toast_joystick_create_record_structures()";

	MX_VARIABLE *variable;
	MX_BIOCAT_TOAST_JOYSTICK *biocat_toast_joystick = NULL;

	variable = (MX_VARIABLE *) malloc( sizeof(MX_VARIABLE) );

	if ( variable == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_VARIABLE structure." );
	}

	biocat_toast_joystick = (MX_BIOCAT_TOAST_JOYSTICK *)
				malloc( sizeof(MX_BIOCAT_TOAST_JOYSTICK));

	if ( biocat_toast_joystick == (MX_BIOCAT_TOAST_JOYSTICK *) NULL )
	{
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for an MX_BIOCAT_TOAST_JOYSTICK structure.");
	}

	record->record_superclass_struct = variable;
	record->record_class_struct = NULL;
	record->record_type_struct = biocat_toast_joystick;
	record->superclass_specific_function_list = 
			&mxv_biocat_toast_joystick_variable_function_list;
	record->class_specific_function_list = NULL;

	variable->record = record;
	biocat_toast_joystick->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_biocat_toast_joystick_send_variable( MX_VARIABLE *variable )
{
	static const char fname[] =
		"mxv_biocat_toast_joystick_send_variable()";

	MX_BIOCAT_TOAST_JOYSTICK *biocat_toast_joystick = NULL;
	MX_COMPUMOTOR_INTERFACE *compumotor_interface = NULL;
	char format[20];
	char command[80];
	char response[80];
	unsigned long joystick_mode;
	mx_status_type mx_status;

	mx_status = mxv_biocat_toast_joystick_get_pointers( variable,
						&biocat_toast_joystick,
						&compumotor_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: variable->pointer_to_value = %p",
		fname, variable->pointer_to_value));

	joystick_mode = *( (unsigned long *) variable->pointer_to_value );

	MX_DEBUG(-2,("%s: joystick_mode = %lu", fname, joystick_mode));

	snprintf( format, sizeof(format),
		"JOY%%0%lulx",
		biocat_toast_joystick->num_joystick_axes );

	MX_DEBUG(-2,("%s: format = '%s'", fname, format));

	snprintf( command, sizeof(command), format, joystick_mode );

	MX_DEBUG(-2,("%s: command = '%s'", fname, command));

	return MX_SUCCESSFUL_RESULT;

	mx_status = mxi_compumotor_command( compumotor_interface, command,
					response, sizeof(response),
					MXV_BIOCAT_TOAST_JOYSTICK_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxv_biocat_toast_joystick_receive_variable( MX_VARIABLE *variable )
{
#if 0
	static const char fname[] =
		"mxv_biocat_toast_joystick_receive_variable()";
#endif
	/* For now, we just return the value most recently written. */

	return MX_SUCCESSFUL_RESULT;
}

