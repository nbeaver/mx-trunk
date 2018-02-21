/*
 * Name:    i_keithley199.c
 *
 * Purpose: MX driver to control Keithley 199 series dmm/scanners.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2018 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define KEITHLEY199_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_ascii.h"
#include "mx_gpib.h"
#include "i_keithley199.h"

MX_RECORD_FUNCTION_LIST mxi_keithley199_record_function_list = {
	NULL,
	mxi_keithley199_create_record_structures,
	mxi_keithley199_finish_record_initialization,
	NULL,
	NULL,
	mxi_keithley199_open,
	mxi_keithley199_close
};

/* Keithley 199 data structures. */

MX_RECORD_FIELD_DEFAULTS mxi_keithley199_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_KEITHLEY199_STANDARD_FIELDS
};

long mxi_keithley199_num_record_fields
		= sizeof( mxi_keithley199_record_field_defaults )
		  / sizeof( mxi_keithley199_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_keithley199_rfield_def_ptr
			= &mxi_keithley199_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxi_keithley199_get_pointers( MX_RECORD *record,
				MX_KEITHLEY199 **keithley199,
				MX_INTERFACE **gpib_interface,
				const char *calling_fname )
{
	static const char fname[] = "mxi_keithley199_get_pointers()";

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( keithley199 == (MX_KEITHLEY199 **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_KEITHLEY199 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( gpib_interface == (MX_INTERFACE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERFACE pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*keithley199 = (MX_KEITHLEY199 *) record->record_type_struct;

	if ( *keithley199 == (MX_KEITHLEY199 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_KEITHLEY199 pointer for record '%s' is NULL.",
			record->name );
	}

	*gpib_interface = &((*keithley199)->gpib_interface);

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxi_keithley199_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxi_keithley199_create_record_structures()";

	MX_KEITHLEY199 *keithley199 = NULL;

	/* Allocate memory for the necessary structures. */

	keithley199 = (MX_KEITHLEY199 *) malloc( sizeof(MX_KEITHLEY199) );

	if ( keithley199 == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_KEITHLEY199 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = keithley199;
	record->class_specific_function_list = NULL;

	keithley199->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_keithley199_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[]
		= "mxi_keithley199_finish_record_initialization()";

	MX_KEITHLEY199 *keithley199 = NULL;
	MX_RECORD *gpib_record;
	long gpib_address;

	/* Verify that the gpib_interface field actually corresponds
	 * to a GPIB or RS-232 interface record.
	 */

	keithley199 = (MX_KEITHLEY199 *) record->record_type_struct;

	gpib_record = keithley199->gpib_interface.record;

	if ( gpib_record->mx_superclass != MXR_INTERFACE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"'%s' used by record '%s' is not an interface record.",
			gpib_record->name, record->name );
	}

	gpib_address = keithley199->gpib_interface.address;

	/* Check that the GPIB address is valid. */

	if ( gpib_address < 0 || gpib_address > 30 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"GPIB address %ld for record '%s' is out of allowed range 0-30.",
			gpib_address, record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_keithley199_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_keithley199_open()";

	MX_KEITHLEY199 *keithley199 = NULL;
	MX_INTERFACE *interface = NULL;
	MX_GPIB *gpib = NULL;
	char command[20];
	char response[80];
	unsigned long read_terminator;
	mx_status_type mx_status;

	mx_status = mxi_keithley199_get_pointers( record,
			&keithley199, &interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,("%s: opening '%s' at interface '%s:%ld'",
	fname, record->name, interface->record->name, interface->address));

	mx_status = mx_gpib_open_device( interface->record, interface->address);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_gpib_selective_device_clear( interface->record,
						interface->address );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the ASCII terminator for the Keithley. */

	gpib = (MX_GPIB *) interface->record->record_class_struct;

	read_terminator = gpib->read_terminator[ interface->address - 1 ];

	MX_DEBUG(-2,("%s: read_terminator[%ld] = %#lx",
		fname, interface->address - 1, read_terminator));

	switch( read_terminator ) {
	case 0x0d0a:
		strlcpy( command, "Y0X", sizeof(command) );	/* CR LF */
		break;
	case 0x0a0d:
		strlcpy( command, "Y1X", sizeof(command) );	/* LF CR */
		break;
	case 0x0d:
		strlcpy( command, "Y2X", sizeof(command) );	/* CR */
		break;
	case 0x0a:
		strlcpy( command, "Y3X", sizeof(command) );	/* LF */
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal ASCII terminator %#lx specified for Keithley '%s'",
			read_terminator, record->name );
	}

	mx_status = mxi_keithley199_command( keithley199, command,
					NULL, 0, KEITHLEY199_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Verify that this is a Keithley 199. */

	mx_status = mxi_keithley199_command( keithley199, "U0X",
					response, sizeof(response),
					KEITHLEY199_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( strncmp( response, "199", 3 ) != 0 ) {
		(void) mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Interface '%s' is not a Keithley 199, since its response "
		"'%s' to a 'U0X' command did not begin with '199.'",
			record->name, response );
	}

	keithley199->last_measurement_type = MXT_KEITHLEY199_UNKNOWN;

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_keithley199_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_keithley199_close()";

	MX_KEITHLEY199 *keithley199 = NULL;
	MX_INTERFACE *interface = NULL;
	mx_status_type mx_status;

	mx_status = mxi_keithley199_get_pointers( record,
			&keithley199, &interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_gpib_go_to_local( interface->record, interface->address);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_gpib_close_device( interface->record,
				interface->address );
	return mx_status;
}

/* === Driver specific commands === */

MX_EXPORT mx_status_type
mxi_keithley199_command( MX_KEITHLEY199 *keithley199, char *command,
		char *response, int response_buffer_length,
		int debug_flag )
{
	static const char fname[] = "mxi_keithley199_command()";

	static struct {
		long mx_error_code;
		char error_message[80];
	} error_message_list[] = {
{ MXE_ILLEGAL_ARGUMENT,         "Invalid Device-dependent Command received." },
{ MXE_ILLEGAL_ARGUMENT,   "Invalid Device-dependent Command Option received."},
{ MXE_INTERFACE_IO_ERROR,       "Remote line was false."                     },
{ MXE_CONTROLLER_INTERNAL_ERROR,"Self-test failed."                          },
{ MXE_ILLEGAL_ARGUMENT,         "Suppression range/value conflict."          },
{ MXE_LIMIT_WAS_EXCEEDED,       "Input current too large to suppress."       },
{ MXE_ILLEGAL_ARGUMENT,      "Auto-suppression requested with zero check on."},
{ MXE_DEVICE_ACTION_FAILED,     "Zero correct failed."                       },
{ MXE_CONTROLLER_INTERNAL_ERROR,"EEPROM checksum error."                     },
{ MXE_LIMIT_WAS_EXCEEDED,       "Overload condition."                        },
{ MXE_ILLEGAL_ARGUMENT,         "Gain/rise time conflict."                   }
};

	/* The line above that the 'overload condition' message is on. */

#define OVERLOAD_INDEX	9

	int num_error_messages = sizeof( error_message_list )
				/ sizeof( error_message_list[0] );

	char error_status[40];
	char *ptr;
	int i;
	MX_INTERFACE *interface = NULL;
	unsigned long flags;
	mx_status_type mx_status;

	if ( keithley199 == (MX_KEITHLEY199 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_KEITHLEY199 pointer passed was NULL." );
	}

	interface = &(keithley199->gpib_interface);

	if ( interface == (MX_INTERFACE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_INTERFACE pointer for keithley '%s' was NULL.",
			keithley199->record->name );
	}

	flags = keithley199->keithley_flags;

	/* If requested, send command string. */

	if ( command != NULL ) {
		mx_status = mx_gpib_putline( interface->record,
						interface->address,
						command, NULL, debug_flag );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Get the response, if one is expected. */

	if ( response != NULL ) {
		mx_status = mx_gpib_getline( interface->record,
				interface->address,
				response, response_buffer_length,
				NULL, debug_flag );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Do we skip over checking for errors? */

	if ( flags & MXF_KEITHLEY199_BYPASS_ERROR_CHECK ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* Check to see if there was an error in the command we just did. */

	mx_status = mx_gpib_putline( interface->record, interface->address,
					"U1X", NULL, debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_gpib_getline( interface->record, interface->address,
		error_status, sizeof(error_status) - 1, NULL, debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Strip off trailing CRs or LFs from the error status string. */

	while (1) {
		size_t length;
		char last_byte;

		length = strlen( error_status );

		if ( length <= 0 ) {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Did not find any non-terminator characters in "
			"the response to the 'U1X' command by '%s'.",
				keithley199->record->name );
		}

		last_byte = error_status[length-1];

		if ( (last_byte == MX_CR) || (last_byte == MX_LF) ) {
			error_status[length-1] = '\0';
		} else {
			/* All trailing terminators have been stripped off,
			 * so we break out of the while() loop here.
			 */

			break;
		}
	}

	/* If no errors occurred, then we return now. */

	if ( strcmp( error_status, "199000000000000000000000000" ) == 0 ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* If the first three characters of the error_status string are
	 * not "199", then the Keithley is probably not connected or turned on.
	 */

	if ( strncmp( error_status, "199", 3 ) != 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Attempt to contact the Keithley 199 at GPIB address %ld "
			"on interface '%s' failed.  Is it turned on?",
			interface->address, interface->record->name );
	}

	/* Some error occurred.  Exit with the first error message that
	 * matches our condition.
	 */

	ptr = error_status + 3;

	for ( i = 0; i < num_error_messages; i++ ) {

		if ( ptr[i] != '0' ) {
			if ( i == OVERLOAD_INDEX ) {
				mx_warning(
				"Keithley overload detected for "
				"GPIB address %ld on interface '%s'.",
					interface->address,
					interface->record->name );

				if ( ptr[i+1] == '0' ) {
					return MX_SUCCESSFUL_RESULT;
				}
			} else {
				return mx_error(
					error_message_list[i].mx_error_code,
					fname,
					"Command '%s' failed.  Reason = '%s'",
					command,
					error_message_list[i].error_message );
			}
		}
	}
	return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The command '%s' sent to the Keithley at GPIB address %ld "
		"failed.  However, the U1 error status word returned was "
		"unrecognizable.  U1 error status word = '%s'",
			command, interface->address, error_status );
}

