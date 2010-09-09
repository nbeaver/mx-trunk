/*
 * Name:    i_powerpmac.c
 *
 * Purpose: MX driver for Delta Tau Power PMAC controllers.  Note that
 *          this is the driver for talking to the POWERPMAC itself.
 *          Power PMAC-controlled motors are interfaced via the driver
 *          in libMx/d_powerpmac.c.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_POWERPMAC_DEBUG		TRUE

#define MXI_POWERPMAC_DEBUG_TIMING	FALSE

#include <stdio.h>
#include <stdlib.h>
#include "mxconfig.h"

#if HAVE_POWER_PMAC_LIBRARY

#include "gplib.h"	/* Delta Tau-provided include file. */

#include "mx_util.h"
#include "mx_record.h"
#include "mx_hrt_debug.h"

#include "i_powerpmac.h"

MX_RECORD_FUNCTION_LIST mxi_powerpmac_record_function_list = {
	NULL,
	mxi_powerpmac_create_record_structures,
	mxi_powerpmac_finish_record_initialization,
	NULL,
	NULL,
	mxi_powerpmac_open,
	NULL,
	NULL,
	NULL,
	mxi_powerpmac_special_processing_setup
};

MX_RECORD_FIELD_DEFAULTS mxi_powerpmac_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_POWERPMAC_STANDARD_FIELDS
};

long mxi_powerpmac_num_record_fields
		= sizeof( mxi_powerpmac_record_field_defaults )
			/ sizeof( mxi_powerpmac_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_powerpmac_rfield_def_ptr
			= &mxi_powerpmac_record_field_defaults[0];

/*---*/

static mx_status_type mxi_powerpmac_process_function( void *record_ptr,
						void *record_field_ptr,
						int operation );

/*--*/

/*==========================*/

MX_EXPORT mx_status_type
mxi_powerpmac_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_powerpmac_create_record_structures()";

	MX_POWERPMAC *powerpmac;

	/* Allocate memory for the necessary structures. */

	powerpmac = (MX_POWERPMAC *) malloc( sizeof(MX_POWERPMAC) );

	if ( powerpmac == (MX_POWERPMAC *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_POWERPMAC structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_type_struct = powerpmac;
	record->record_class_struct = NULL;
	record->class_specific_function_list = NULL;

	powerpmac->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_powerpmac_finish_record_initialization( MX_RECORD *record )
{
#if 0
	static const char fname[] =
		"mxi_powerpmac_finish_record_initialization()";
#endif

	MX_POWERPMAC *powerpmac;

	powerpmac = (MX_POWERPMAC *) record->record_type_struct;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_powerpmac_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_powerpmac_open()";

	MX_POWERPMAC *powerpmac;
	char response[80];
	int num_items;
	int powerpmac_status;
	mx_status_type mx_status;

	MX_DEBUG(-2, ("%s invoked.", fname));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	powerpmac = (MX_POWERPMAC *) record->record_type_struct;

	if ( powerpmac == (MX_POWERPMAC *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_POWERPMAC pointer for record '%s' is NULL.", record->name);
	}

	/* Initialize the libppmac library. */

	powerpmac_status = InitLibrary();

	if ( powerpmac_status != 0 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Initialization of the Power PMAC library "
		"failed with error code = %d.",
			powerpmac_status );
	}

	/* Verify that the Power PMAC controller is active by asking it for
	 * its card type.
	 */

	mx_status = mxi_powerpmac_command( powerpmac, "TYPE",
					response, sizeof(response),
					MXI_POWERPMAC_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Now ask for the PROM firmware version. */

	mx_status = mxi_powerpmac_command( powerpmac, "VERS",
					response, sizeof(response),
					MXI_POWERPMAC_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%ld.%ld",
		&(powerpmac->major_version), &(powerpmac->minor_version) );

	if ( num_items != 2 ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
			"Unable to parse response to VERSION command.  "
			"Response = '%s'", response );
	}

#if MXI_POWERPMAC_DEBUG
	MX_DEBUG(-2, ("%s: PowerPMAC version: major = %ld, minor = %ld",
		fname, powerpmac->major_version, powerpmac->minor_version));
#endif

	/* Get a pointer to the PowerPMAC shared memory. */

	powerpmac->shared_mem = GetSharedMemPtr();

	if ( powerpmac->shared_mem == (SHM *) NULL ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"The attempt to get the shared memory pointer "
		"for PowerPMAC '%s' using GetSharedMemPtr() failed.",
			record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_powerpmac_special_processing_setup( MX_RECORD *record )
{
	static const char fname[] = "mxi_powerpmac_special_processing_setup()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	MX_DEBUG(2,("%s invoked.", fname));

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_POWERPMAC_COMMAND:
		case MXLV_POWERPMAC_RESPONSE:
		case MXLV_POWERPMAC_COMMAND_WITH_RESPONSE:
			record_field->process_function
					    = mxi_powerpmac_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

/*----*/

MX_EXPORT mx_status_type
mxi_powerpmac_command( MX_POWERPMAC *powerpmac, char *command,
		char *response, size_t response_buffer_length,
		mx_bool_type debug_flag )
{
	static const char fname[] = "mxi_powerpmac_command()";

	int powerpmac_status, length;

	if ( powerpmac == (MX_POWERPMAC *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_POWERPMAC pointer passed was NULL." );
	}

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: sending '%s' to '%s'",
			fname, command, powerpmac->record->name ));
	}

	if ( (response == NULL) || (response_buffer_length == 0) ) {
		powerpmac_status = Command( command );
	} else {
		powerpmac_status = GetResponse( command,
					response, response_buffer_length, 0 );

		if ( powerpmac_status == 0 ) {
			length = strlen( response );

			if ( response[length-1] == '\n' ) {
				response[length-1] = '\0';
			}
		}

		if ( debug_flag ) {
			MX_DEBUG(-2,("%s: received '%s' from '%s'",
				fname, response, powerpmac->record->name ));
		}
	}

	if ( powerpmac_status == 0 ) {
		return MX_SUCCESSFUL_RESULT;
	} else {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Power PMAC command '%s' returned with error code = %d.",
			command, powerpmac_status );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT long
mxi_powerpmac_get_long( MX_POWERPMAC *powerpmac,
			char *command,
			mx_bool_type debug_flag )
{
	char response[100];
	char *ptr;
	long result;
	mx_status_type mx_status;

	mx_status = mxi_powerpmac_command( powerpmac, command,
					response, sizeof(response),
					debug_flag );

	if ( mx_status.code != MXE_SUCCESS ) {
		return (-1L);
	}

	/* If an equals sign is present in the response, skip over it. */

	ptr = strchr( response, '=' );

	if ( ptr == NULL ) {
		ptr = response;
	} else {
		ptr++;
	}

	result = mx_string_to_long( ptr );

	return result;
}

MX_EXPORT double
mxi_powerpmac_get_double( MX_POWERPMAC *powerpmac,
			char *command,
			mx_bool_type debug_flag )
{
	char response[100];
	char *ptr;
	double result;
	mx_status_type mx_status;

	mx_status = mxi_powerpmac_command( powerpmac, command,
					response, sizeof(response),
					debug_flag );

	if ( mx_status.code != MXE_SUCCESS ) {
		return 0.0;
	}

	/* If an equals sign is present in the response, skip over it. */

	ptr = strchr( response, '=' );

	if ( ptr == NULL ) {
		ptr = response;
	} else {
		ptr++;
	}

	result = atof( ptr );

	return result;
}

/*==================================================================*/

#ifndef MX_PROCESS_GET

#define MX_PROCESS_GET	1
#define MX_PROCESS_PUT	2

#endif

static mx_status_type
mxi_powerpmac_process_function( void *record_ptr,
			void *record_field_ptr, int operation )
{
	static const char fname[] = "mxi_powerpmac_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_POWERPMAC *powerpmac;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	powerpmac = (MX_POWERPMAC *) (record->record_type_struct);

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_POWERPMAC_RESPONSE:
			/* Nothing to do since the necessary string is
			 * already stored in the 'response' field.
			 */

			break;
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_GET label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	case MX_PROCESS_PUT:
		switch( record_field->label_value ) {
		case MXLV_POWERPMAC_COMMAND:
			mx_status = mxi_powerpmac_command( powerpmac,
						powerpmac->command,
						NULL, 0, MXI_POWERPMAC_DEBUG );

			break;
		case MXLV_POWERPMAC_COMMAND_WITH_RESPONSE:
			mx_status = mxi_powerpmac_command( powerpmac,
						powerpmac->command,
						powerpmac->response,
						MX_POWERPMAC_MAX_COMMAND_LENGTH,
						MXI_POWERPMAC_DEBUG );

			break;
		default:
			MX_DEBUG( 1,(
			    "%s: *** Unknown MX_PROCESS_PUT label value = %ld",
				fname, record_field->label_value));
			break;
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unknown operation code = %d", operation );
	}

	return mx_status;
}

#endif /* HAVE_POWER_PMAC_LIBRARY */

