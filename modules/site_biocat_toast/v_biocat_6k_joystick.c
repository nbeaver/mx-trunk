/*
 * Name:    v_biocat_6k_joystick.c
 *
 * Purpose: MX variable driver for a driver to enable or disable
 *          the BioCAT Compumotor 6K joystick.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013-2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXV_BIOCAT_6K_JOYSTICK_DEBUG			TRUE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_socket.h"
#include "mx_process.h"
#include "mx_variable.h"

#include "i_compumotor.h"
#include "v_biocat_6k_joystick.h"

/*---*/

MX_RECORD_FUNCTION_LIST mxv_biocat_6k_joystick_record_function_list = {
	mx_variable_initialize_driver,
	mxv_biocat_6k_joystick_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxv_biocat_6k_joystick_open
};

MX_VARIABLE_FUNCTION_LIST mxv_biocat_6k_joystick_variable_function_list = {
	mxv_biocat_6k_joystick_send_variable,
	mxv_biocat_6k_joystick_receive_variable
};

/* Record field defaults for 'biocat_6k_joystick'. */

MX_RECORD_FIELD_DEFAULTS mxv_biocat_6k_joystick_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXV_BIOCAT_6K_JOYSTICK_STANDARD_FIELDS,
	MX_VARIABLE_STANDARD_FIELDS,
	MX_ULONG_VARIABLE_STANDARD_FIELDS
};

long mxv_biocat_6k_joystick_num_record_fields
	= sizeof( mxv_biocat_6k_joystick_record_field_defaults )
	/ sizeof( mxv_biocat_6k_joystick_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxv_biocat_6k_joystick_rfield_def_ptr
		= &mxv_biocat_6k_joystick_record_field_defaults[0];

/*---*/

static mx_status_type
mxv_biocat_6k_joystick_get_pointers( MX_VARIABLE *variable,
			MX_BIOCAT_6K_JOYSTICK **biocat_6k_joystick,
			MX_COMPUMOTOR_INTERFACE **compumotor_interface,
			const char *calling_fname )
{
	static const char fname[] =
			"mxv_biocat_6k_joystick_get_pointers()";

	MX_BIOCAT_6K_JOYSTICK *biocat_6k_joystick_ptr;
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

	biocat_6k_joystick_ptr = (MX_BIOCAT_6K_JOYSTICK *)
					variable->record->record_type_struct;

	if ( biocat_6k_joystick != (MX_BIOCAT_6K_JOYSTICK **) NULL ) {
		*biocat_6k_joystick = biocat_6k_joystick_ptr;
	}

	if ( compumotor_interface != (MX_COMPUMOTOR_INTERFACE **) NULL ) {

		compumotor_interface_record =
			biocat_6k_joystick_ptr->compumotor_interface_record;

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
mxv_biocat_6k_joystick_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxv_biocat_6k_joystick_create_record_structures()";

	MX_VARIABLE *variable;
	MX_BIOCAT_6K_JOYSTICK *biocat_6k_joystick = NULL;

	variable = (MX_VARIABLE *) malloc( sizeof(MX_VARIABLE) );

	if ( variable == (MX_VARIABLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_VARIABLE structure." );
	}

	biocat_6k_joystick = (MX_BIOCAT_6K_JOYSTICK *)
				malloc( sizeof(MX_BIOCAT_6K_JOYSTICK));

	if ( biocat_6k_joystick == (MX_BIOCAT_6K_JOYSTICK *) NULL )
	{
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for an MX_BIOCAT_6K_JOYSTICK structure.");
	}

	record->record_superclass_struct = variable;
	record->record_class_struct = NULL;
	record->record_type_struct = biocat_6k_joystick;
	record->superclass_specific_function_list = 
			&mxv_biocat_6k_joystick_variable_function_list;
	record->class_specific_function_list = NULL;

	variable->record = record;
	biocat_6k_joystick->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_biocat_6k_joystick_open( MX_RECORD *record )
{
	static const char fname[] = "mxv_biocat_6k_joystick_open()";

	MX_VARIABLE *variable = NULL;
	MX_BIOCAT_6K_JOYSTICK *biocat_6k_joystick = NULL;
	MX_COMPUMOTOR_INTERFACE *compumotor_interface = NULL;
	char command[80];
	char response[80];
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	variable = (MX_VARIABLE *) record->record_superclass_struct;

	mx_status = mxv_biocat_6k_joystick_get_pointers( variable,
						&biocat_6k_joystick,
						&compumotor_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If the joystick is on at the time that MX starts up, it can
	 * interfere with some of the commands executed in the
	 * mxd_compumotor_open() method of that driver.  If you need
	 * to preserve the state of the joystick over restarts of the
	 * MX server, then add the joystick variable to the list of
	 * variables saved and restored by the 'mxautosave' program.
	 */

	snprintf( command, sizeof(command), "%ld_!JOY00000000",
		biocat_6k_joystick->controller_number );

	mx_status = mxi_compumotor_command( compumotor_interface, command,
				NULL, 0, MXV_BIOCAT_6K_JOYSTICK_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Read back the joystick settings to verify that they have
	 * taken effect.
	 */

	snprintf( command, sizeof(command), "%ld_!JOY",
		biocat_6k_joystick->controller_number );

	mx_status = mxi_compumotor_command( compumotor_interface, command,
					response, sizeof(response),
					MXV_BIOCAT_6K_JOYSTICK_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If we got a "MOTION IN PROGRESS" message here, then we have
	 * one more line to read.
	 */

	if ( strncmp( response, "MOTION IN PROGRESS", 18 ) == 0 ) {
		mx_status = mx_rs232_getline(compumotor_interface->rs232_record,
					response, sizeof(response),
					NULL, MXV_BIOCAT_6K_JOYSTICK_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* It takes a while for the controller to react to this command,
	 * so we insert a wait here.
	 */

	mx_msleep(1000);

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxv_biocat_6k_joystick_send_variable( MX_VARIABLE *variable )
{
	static const char fname[] =
		"mxv_biocat_6k_joystick_send_variable()";

	MX_BIOCAT_6K_JOYSTICK *biocat_6k_joystick = NULL;
	MX_COMPUMOTOR_INTERFACE *compumotor_interface = NULL;
	char command[80];
	void *value_ptr;
	unsigned long joystick_mode, i, num_axes;
	mx_status_type mx_status;

	mx_status = mxv_biocat_6k_joystick_get_pointers( variable,
						&biocat_6k_joystick,
						&compumotor_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_get_variable_pointer( variable->record, &value_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	joystick_mode = *( (unsigned long *) value_ptr );

	snprintf( command, sizeof(command), "%ld_!JOY",
		biocat_6k_joystick->controller_number );

	num_axes = biocat_6k_joystick->num_joystick_axes;

	for ( i = 0; i < num_axes; i++ ) {

		if ( (joystick_mode >> i) & 0x1 ) {
			strlcat( command, "1", sizeof(command) );
		} else {
			strlcat( command, "0", sizeof(command) );
		}
	}

	mx_status = mxi_compumotor_command( compumotor_interface, command,
				NULL, 0, MXV_BIOCAT_6K_JOYSTICK_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxv_biocat_6k_joystick_receive_variable( MX_VARIABLE *variable )
{
	static const char fname[] =
		"mxv_biocat_6k_joystick_receive_variable()";

	MX_BIOCAT_6K_JOYSTICK *biocat_6k_joystick = NULL;
	MX_COMPUMOTOR_INTERFACE *compumotor_interface = NULL;
	char command[80];
	char response[80];
	unsigned long joystick_mode, axis;
	void *value_ptr;
	char *ptr;
	mx_status_type mx_status;

	mx_status = mxv_biocat_6k_joystick_get_pointers( variable,
						&biocat_6k_joystick,
						&compumotor_interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command), "%ld_!JOY",
		biocat_6k_joystick->controller_number );

	mx_status = mxi_compumotor_command( compumotor_interface, command,
					response, sizeof(response),
					MXV_BIOCAT_6K_JOYSTICK_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	joystick_mode = 0;

	ptr = response;

	axis = 0;

	while (1) {
		if ( *ptr == '*' ) {
			/* Skip over asterisk characters. */
		} else
		if ( *ptr == '_' ) {
			/* Skip over underscore characters. */

		} else
		if ( *ptr == '\0' ) {
			/* If we see a null byte, then we have reached
			 * the end of the string and must break out
			 * of the while loop.
			 */

			break;
		} else
		if ( *ptr == '1' ) {
			joystick_mode += mx_round( pow( 2.0, axis ) );
			axis++;
		} else
		if ( *ptr == '0' ) {
			axis++;
		} else {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Unexpected character '%c' (%#lx) seen in "
			"response '%s' to command '%s' for joystick '%s'.",
				*ptr, (unsigned long) *ptr,
				response, command, variable->record->name );
		}

		ptr++;
	}

	mx_status = mx_get_variable_pointer( variable->record, &value_ptr );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	*( (unsigned long *) value_ptr ) = joystick_mode;

	return MX_SUCCESSFUL_RESULT;
}

