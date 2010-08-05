/*
 * Name:    i_powerpmac.c
 *
 * Purpose: MX driver for Delta Tau POWERPMAC controllers.  Note that this is
 *          the driver for talking to the POWERPMAC itself.  POWERPMAC-controlled
 *          motors are interfaced via the driver in libMx/d_powerpmac.c.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2004, 2006, 2009-2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_POWERPMAC_DEBUG			FALSE

#define MXI_POWERPMAC_DEBUG_TIMING		FALSE

#define MXI_POWERPMAC_DEBUG_LOGIN		FALSE

#define MXI_POWERPMAC_DEBUG_RS232		FALSE

#define MXI_POWERPMAC_DEBUG_TCP		FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_rs232.h"
#include "mx_socket.h"

#if HAVE_EPICS
#  include "mx_epics.h"
#endif

#if HAVE_POWER_POWERPMAC_LIBRARY
#  include "gplib.h"
#endif

#include "i_powerpmac.h"
#include "i_powerpmac_ethernet.h"
#include "i_telnet.h"

#if MXI_POWERPMAC_DEBUG_TIMING
#  include "mx_hrt_debug.h"
#endif

MX_RECORD_FUNCTION_LIST mxi_powerpmac_record_function_list = {
	NULL,
	mxi_powerpmac_create_record_structures,
	mxi_powerpmac_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_powerpmac_open,
	NULL,
	NULL,
	mxi_powerpmac_resynchronize,
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

static mx_status_type
mxi_powerpmac_getchar( MX_POWERPMAC *, char * );

static mx_status_type
mxi_powerpmac_receive_response_with_getchar( MX_POWERPMAC *, char *, int, int );

static mx_status_type
mxi_powerpmac_check_for_errors( MX_POWERPMAC *, char *, char *, char * );

/*--*/

static mx_status_type
mxi_powerpmac_std_command( MX_POWERPMAC *, char *, char *, size_t, int );

static mx_status_type
mxi_powerpmac_std_send_command( MX_POWERPMAC *, char *, int );

static mx_status_type
mxi_powerpmac_std_receive_response( MX_POWERPMAC *, char *, int, int );

/*--*/

static mx_status_type
mxi_powerpmac_tcp_flush( MX_POWERPMAC *, int );

static mx_status_type
mxi_powerpmac_tcp_command( MX_POWERPMAC *, char *, char *, size_t, int );

static mx_status_type
mxi_powerpmac_tcp_receive_response( MX_SOCKET *, char *, size_t, size_t *, int );

/*--*/

static mx_status_type
mxi_powerpmac_gpascii_login( MX_POWERPMAC *powerpmac );

/*--*/

#if HAVE_POWER_POWERPMAC_LIBRARY
static mx_status_type
mxi_powerpmac_gplib_command( MX_POWERPMAC *, char *, char *, size_t, int );
#endif

#if HAVE_EPICS
static mx_status_type
mxi_powerpmac_epics_ect_command( MX_POWERPMAC *, char *, char *, size_t, int );
#endif

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
		"Can't allocate memory for MX_POWERPMAC structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_type_struct = powerpmac;
	record->record_class_struct = NULL;
	record->class_specific_function_list = NULL;

	powerpmac->record = record;

	powerpmac->powerpmac_socket = NULL;
	powerpmac->hostname[0] = '\0';

	powerpmac->gpascii_username[0] = '\0';
	powerpmac->gpascii_password[0] = '\0';

	powerpmac->gplib_initialized = FALSE;

	powerpmac->port_record = NULL;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_powerpmac_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] = "mxi_powerpmac_finish_record_initialization()";

	MX_POWERPMAC *powerpmac;
	MX_RS232 *rs232;
	char *port_type_name;
	char *port_args;
	int argc;
	char **argv;
	int i, length;
	char port_record_name[MXU_RECORD_NAME_LENGTH+1];

	powerpmac = (MX_POWERPMAC *) record->record_type_struct;

	/* Identify the port_type of the record. */

	port_type_name = powerpmac->port_type_name;

	length = (int) strlen( port_type_name );

	for ( i = 0; i < length; i++ ) {
		if ( isupper( (int) (port_type_name[i]) ) ) {
			port_type_name[i] = tolower( (int)(port_type_name[i]) );
		}
	}

	if ( strcmp( port_type_name, "rs232" ) == 0 ) {
		powerpmac->port_type = MX_POWERPMAC_PORT_TYPE_RS232;

	} else if ( strcmp( port_type_name, "tcp" ) == 0 ) {
		powerpmac->port_type = MX_POWERPMAC_PORT_TYPE_TCP;

	} else if ( strcmp( port_type_name, "gpascii" ) == 0 ) {
		powerpmac->port_type = MX_POWERPMAC_PORT_TYPE_GPASCII;

	} else if ( strcmp( port_type_name, "gplib" ) == 0 ) {
		powerpmac->port_type = MX_POWERPMAC_PORT_TYPE_GPLIB;

	} else if ( strcmp( port_type_name, "epics_ect" ) == 0 ) {
		powerpmac->port_type = MX_POWERPMAC_PORT_TYPE_EPICS_TC;

	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The POWERPMAC interface port_type '%s' is not a valid port_type.",
			port_type_name );
	}

	/* Perform port_type specific processing. */

	switch( powerpmac->port_type ) {
	case MX_POWERPMAC_PORT_TYPE_RS232:
	case MX_POWERPMAC_PORT_TYPE_GPASCII:
		if ( powerpmac->port_type == MX_POWERPMAC_PORT_TYPE_RS232 ) {
			strlcpy( port_record_name, powerpmac->port_args,
				sizeof(port_record_name) );
		} else
		if ( powerpmac->port_type == MX_POWERPMAC_PORT_TYPE_GPASCII ) {

			port_args = strdup( powerpmac->port_args );

			if ( port_args == NULL ) {
				return mx_error( MXE_OUT_OF_MEMORY, fname,
				"Ran out of memory trying to create a copy "
				"of the POWERPMAC port_args." );
			}

			mx_string_split( port_args, " ", &argc, &argv );

			if ( argc != 3 ) {
				mx_free(argv);
				mx_free(port_args);

				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
				"Incorrect number of arguments (%d) specified "
				"for the 'port_args' string '%s' for POWERPMAC "
				"controller '%s'.  The correct number of "
				"arguments should be 3, namely, the socket "
				"RS-232 record name, the PowerPOWERPMAC username, "
				"and the PowerPOWERPMAC password.",
					argc, powerpmac->port_args, record->name );
			}

			strlcpy( port_record_name, argv[0],
				sizeof(port_record_name) );

			strlcpy( powerpmac->gpascii_username, argv[1],
				sizeof(powerpmac->gpascii_username) );

			strlcpy( powerpmac->gpascii_password, argv[2],
				sizeof(powerpmac->gpascii_password) );

			mx_free(argv);
			mx_free(port_args);
		}

		powerpmac->port_record = mx_get_record( record->list_head,
							port_record_name );

		if ( powerpmac->port_record == NULL ) {
			return mx_error( MXE_NOT_FOUND, fname,
			"The RS-232 interface record '%s' does not exist.",
				port_record_name );
		}
		if ( powerpmac->port_record->mx_superclass != MXR_INTERFACE ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
				"'%s' is not an interface record.",
				powerpmac->port_record->name );
		}
		if ( powerpmac->port_record->mx_class != MXI_RS232 ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
				"'%s' is not an RS-232 interface.",
				powerpmac->port_record->name );
		}
		rs232 = (MX_RS232 *) powerpmac->port_record->record_class_struct;

		if ( rs232 == (MX_RS232 *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_RS232 pointer for RS-232 record '%s' is NULL.",
				powerpmac->port_record->name );
		}

		if ( powerpmac->port_type == MX_POWERPMAC_PORT_TYPE_RS232 ) {
			if ( ( rs232->read_terminators != MX_CR )
			  || ( rs232->write_terminators != MX_CR ) )
			{
				return mx_error(
				MXE_SOFTWARE_CONFIGURATION_ERROR, fname,
			"RS-232 port '%s' used by POWERPMAC interface '%s' "
			"has incorrectly configured read "
			"and/or write terminators.  The MX POWERPMAC RS-232 support "
			"requires that the read and write terminators for the "
			"RS-232 port both be set to <CR> (0x0d).",
				powerpmac->port_record->name, record->name );
			}
		}
		break;

	case MX_POWERPMAC_PORT_TYPE_TCP:
		powerpmac->port_record = NULL;

		strlcpy( powerpmac->hostname, powerpmac->port_args,
						sizeof(powerpmac->hostname) );
		break;

	case MX_POWERPMAC_PORT_TYPE_GPLIB:
		powerpmac->port_record = NULL;

		break;

	case MX_POWERPMAC_PORT_TYPE_EPICS_TC:
		powerpmac->port_record = NULL;

#if HAVE_EPICS
		mx_epics_pvname_init( &(powerpmac->strcmd_pv),
					"%sStrCmd", powerpmac->port_args );

		mx_epics_pvname_init( &(powerpmac->strrsp_pv),
					"%sStrRsp", powerpmac->port_args );
#endif
		break;
	}

	/* Mark the POWERPMAC device as being closed. */

	record->record_flags |= ( ~MXF_REC_OPEN );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_powerpmac_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_powerpmac_open()";

	MX_POWERPMAC *powerpmac;
	char command[80];
	char response[80];
	char powerpmac_type_name[20];
	int num_items;
	mx_status_type mx_status;

	MX_DEBUG( 2, ("%s invoked.", fname));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	powerpmac = (MX_POWERPMAC *) record->record_type_struct;

	if ( powerpmac == (MX_POWERPMAC *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_POWERPMAC pointer for record '%s' is NULL.", record->name);
	}

	/* Perform initial port_type-specific initialization. */

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( powerpmac->port_type ) {
	case MX_POWERPMAC_PORT_TYPE_TCP:
		mx_status = mx_tcp_socket_open_as_client( &powerpmac->powerpmac_socket,
						powerpmac->hostname,
						MX_POWERPMAC_TCP_PORT_NUMBER, 0,
						MX_SOCKET_DEFAULT_BUFFER_SIZE );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mxi_powerpmac_tcp_flush( powerpmac, MXI_POWERPMAC_DEBUG );
		break;

	case MX_POWERPMAC_PORT_TYPE_GPASCII:
		mx_status = mxi_powerpmac_gpascii_login( powerpmac );
		break;

#if HAVE_POWER_POWERPMAC_LIBRARY
	case MX_POWERPMAC_PORT_TYPE_GPLIB:
		if ( powerpmac->gplib_initialized == FALSE ) {
			int powerpowerpmac_status;

			powerpmac->gplib_initialized = TRUE;

#if USE_FAKE_POWER_POWERPMAC
			/* The 'fake_power_powerpmac' library needs the following
			 * assignment, so that it can find the MX database.
			 */

			power_powerpmac_mx_record_list = powerpmac->record->list_head;
#endif

			powerpowerpmac_status = InitLibrary();

			if ( powerpowerpmac_status != 0 ) {
				return mx_error( MXE_INTERFACE_IO_ERROR, fname,
				"Initialization of the Power POWERPMAC library "
				"failed with error code = %d.",
					powerpowerpmac_status );
			}
		}
		break;
#endif
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Verify that the POWERPMAC controller is active by asking it for
	 * its card type.
	 */

	mx_status = mxi_powerpmac_command( powerpmac, "TYPE",
				response, sizeof response, MXI_POWERPMAC_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Original POWERPMACs and Turbo POWERPMACs have the name terminated by
	 * a comma (') character.  However, PowerPOWERPMACs have the name
	 * terminated by CR-LF.
	 *
	 * So, we initially search for a comma in the string and
	 * NUL it out if found.
	 *
	 * If a comma was _not_ found in the string, we search for a
	 * carriage return (0xd) and NUL it out if found.
	 *
	 * Otherwise, we leave the string alone.
	 */

#if 1
	{
		char *ptr;

		ptr = strchr( response, ',' );

		if ( ptr != NULL ) {
			*ptr = '\0';
		} else {
			ptr = strchr( response, MX_CR );

			if ( ptr != NULL ) {
				*ptr = '\0';
			}
		}

		strlcpy( powerpmac_type_name, response, sizeof(powerpmac_type_name) );
	}
#else
	/* OBSOLETE: The old method before Power POWERPMAC. */

	/* Now scan for the string. */

	num_items = sscanf( response, "%s,", powerpmac_type_name );

	if ( num_items != 1 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"The device attached to this interface does not "
			"seem to be a POWERPMAC controller.  "
			"Response to POWERPMAC TYPE command = '%s'",
				response );
	}
#endif

	if ( strncmp( powerpmac_type_name, "POWERPMAC1",5 ) == 0 ) {
		powerpmac->powerpmac_type = MX_POWERPMAC_TYPE_POWERPMAC1;

	} else
	if ( strncmp( powerpmac_type_name, "POWERPMAC2",5 ) == 0 ) {
		powerpmac->powerpmac_type = MX_POWERPMAC_TYPE_POWERPMAC2;

	} else
	if ( strncmp( powerpmac_type_name, "POWERPMACUL",6 ) == 0 ) {
		powerpmac->powerpmac_type = MX_POWERPMAC_TYPE_POWERPMACUL;

	} else
	if ( strncmp( powerpmac_type_name, "TURBO1",6 ) == 0 ) {
		powerpmac->powerpmac_type = MX_POWERPMAC_TYPE_TURBO1;

	} else
	if ( strncmp( powerpmac_type_name, "TURBO2",6 ) == 0 ) {
		powerpmac->powerpmac_type = MX_POWERPMAC_TYPE_TURBO2;

	} else
	if ( strncmp( powerpmac_type_name, "TURBOU",6 ) == 0 ) {
		powerpmac->powerpmac_type = MX_POWERPMAC_TYPE_TURBOU;

	} else
	if ( strncmp( powerpmac_type_name, "PWR POWERPMAC UMAC", 13 ) == 0 ) {
		powerpmac->powerpmac_type = MX_POWERPMAC_TYPE_POWERPOWERPMAC;

	} else {
		if ( ( strncmp( powerpmac_type_name, "POWERPMAC", 4 ) != 0 )
		  && ( strncmp( powerpmac_type_name, "TURBO", 5 ) != 0 )
		  && ( strncmp( powerpmac_type_name, "PWR POWERPMAC", 8 ) != 0 ) )
		{
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
				"The device attached to this interface does "
				"not seem to be a POWERPMAC controller.  "
				"Response to POWERPMAC TYPE command = '%s'",
					response );
		} else {
			return mx_error( MXE_TYPE_MISMATCH, fname,
			"Unrecognized POWERPMAC type '%s' for POWERPMAC '%s'.",
				powerpmac_type_name, record->name );
		}
	}

	if ( powerpmac->powerpmac_type == MX_POWERPMAC_TYPE_POWERPOWERPMAC ) {

		/* For PowerPOWERPMACs, the low numbered I-variables now
		 * actually refer to the motor parameters for motor 0.
		 * So the I6 variable no longer is the error reporting
		 * mode.  Instead we hard code the type of error
		 * reporting here.
		 */

		powerpmac->error_reporting_mode = 3;
	} else {
		/* Ask for the style of error message reporting. */

		mx_status = mxi_powerpmac_command( powerpmac, "I6",
				response, sizeof response, MXI_POWERPMAC_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%ld",
				&(powerpmac->error_reporting_mode) );

		if ( num_items != 1 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
			"Unable to parse response to I6 command.  "
			"Response = '%s'", response );
		}
	}

#if MXI_POWERPMAC_DEBUG
	MX_DEBUG(-2,("%s: error_reporting_mode = %ld",
		fname, powerpmac->error_reporting_mode));
#endif

	/* Now ask for PROM firmware version. */

	if ( powerpmac->powerpmac_type == MX_POWERPMAC_TYPE_POWERPOWERPMAC ) {
		strlcpy( command, "VERS", sizeof(command) );
	} else {
		strlcpy( command, "VERSION", sizeof(command) );
	}

	mx_status = mxi_powerpmac_command( powerpmac, command,
				response, sizeof response, MXI_POWERPMAC_DEBUG );

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
	MX_DEBUG(-2, ("%s: POWERPMAC version: major = %ld, minor = %ld",
		fname, powerpmac->major_version, powerpmac->minor_version));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_powerpmac_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxi_powerpmac_resynchronize()";

	MX_POWERPMAC *powerpmac;
	mx_status_type mx_status;

	MX_DEBUG( 2, ("%s invoked.", fname));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	powerpmac = (MX_POWERPMAC *) record->record_type_struct;

	if ( powerpmac == (MX_POWERPMAC *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_POWERPMAC pointer for record '%s' is NULL.", record->name);
	}

	/* Perform port_type-specific resynchronization. */

	switch( powerpmac->port_type ) {
	case MX_POWERPMAC_PORT_TYPE_RS232:
	case MX_POWERPMAC_PORT_TYPE_GPASCII:
		mx_status = mx_resynchronize_record( powerpmac->port_record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;
	}

	/* Now reopen the POWERPMAC record. */

	mx_status = mxi_powerpmac_open( record );

	return mx_status;
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

/* === Functions specific to this driver. === */

static struct {
	int mx_error_code;
	char error_message[100];
} mxi_powerpmac_error_messages[] = {
	{ MXE_INTERFACE_ACTION_FAILED,				/* 0 */
		"Unknown error code 0" },
	{ MXE_CLIENT_REQUEST_DENIED,				/* 1 */
		"The requested command is not allowed while "
			"a POWERPMAC motion program is executing" },
	{ MXE_PERMISSION_DENIED,				/* 2 */
		"Password error" },
	{ MXE_INTERFACE_ACTION_FAILED,				/* 3 */
		"Data error or unrecognized command" },
	{ MXE_INTERFACE_IO_ERROR,				/* 4 */
		"Illegal character: bad value or serial parity/framing error"},
	{ MXE_CLIENT_REQUEST_DENIED,				/* 5 */
		"The requested command is not allowed unless a buffer is open"},
	{ MXE_CLIENT_REQUEST_DENIED,				/* 6 */
		"There was no room in the buffer for the requested command" },
	{ MXE_CLIENT_REQUEST_DENIED,				/* 7 */
		"The buffer is already in use" },
	{ MXE_INTERFACE_IO_ERROR,				/* 8 */
		"MACRO auxiliary communications error" },
	{ MXE_ILLEGAL_ARGUMENT,					/* 9 */
		"Program structural error" },
	{ MXE_LIMIT_WAS_EXCEEDED,				/* 10 */
	"Both overtravel limits are set for a motor in the coordinate system" },
	{ MXE_DEVICE_ACTION_FAILED,				/* 11 */
		"A previous move has not yet completed" },
	{ MXE_DEVICE_ACTION_FAILED,				/* 12 */
		"A motor in the coordinate system is open-loop" },
	{ MXE_DEVICE_ACTION_FAILED,				/* 13 */
		"A motor in the coordinate system is not activated" },
	{ MXE_ILLEGAL_ARGUMENT,					/* 14 */
		"No motors are defined in the coordinate system" },
	{ MXE_ILLEGAL_ARGUMENT,					/* 15 */
		"Not pointing to a valid program buffer" },
	{ MXE_ILLEGAL_ARGUMENT,					/* 16 */
		"Running a improperly structured program" },
	{ MXE_DEVICE_ACTION_FAILED,				/* 17 */
		"Trying to resume after H or Q with motors out of "
		"stopped position" },
	{ MXE_CLIENT_REQUEST_DENIED,				/* 18 */
		"Attempt to perform phase reference during a move, "
		"or a move during phase reference" },
	{ MXE_CLIENT_REQUEST_DENIED,				/* 19 */
		"Illegal position-change command while moves stored "
		"in CCBUFFER" },
};

static int mxi_powerpmac_num_error_messages =
	sizeof( mxi_powerpmac_error_messages )
	/ sizeof( mxi_powerpmac_error_messages[0] );

/*----*/

MX_EXPORT mx_status_type
mxi_powerpmac_command( MX_POWERPMAC *powerpmac, char *command,
		char *response, size_t response_buffer_length,
		int debug_flag )
{
	static const char fname[] = "mxi_powerpmac_command()";

	mx_status_type mx_status;

	if ( powerpmac == (MX_POWERPMAC *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_POWERPMAC pointer passed was NULL." );
	}

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: sending '%s' to '%s'",
			fname, command, powerpmac->record->name ));
	}

	switch( powerpmac->port_type ) {
	case MX_POWERPMAC_PORT_TYPE_RS232:
	case MX_POWERPMAC_PORT_TYPE_GPASCII:
		mx_status = mxi_powerpmac_std_command( powerpmac, command,
					response, response_buffer_length, 0 );
		break;

	case MX_POWERPMAC_PORT_TYPE_TCP:
		mx_status = mxi_powerpmac_tcp_command( powerpmac, command,
					response, response_buffer_length, 0 );
		break;

#if HAVE_POWER_POWERPMAC_LIBRARY
	case MX_POWERPMAC_PORT_TYPE_GPLIB:
		mx_status = mxi_powerpmac_gplib_command( powerpmac, command,
					response, response_buffer_length, 0 );
		break;
#endif

#if HAVE_EPICS
	case MX_POWERPMAC_PORT_TYPE_EPICS_TC:
		mx_status = mxi_powerpmac_epics_ect_command( powerpmac, command, 
					response, response_buffer_length, 0 );
		break;
#endif
	default:
		mx_status = mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Unsupported POWERPMAC port type %ld requested for "
		"POWERPMAC controller '%s'.",
			powerpmac->port_type, powerpmac->record->name );
		break;
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( debug_flag && (response != NULL) ) {
		MX_DEBUG(-2,("%s: received '%s' from '%s'",
			fname, response, powerpmac->record->name ));
	}

	return MX_SUCCESSFUL_RESULT;
}

/*----*/

MX_EXPORT mx_status_type
mxi_powerpmac_card_command( MX_POWERPMAC *powerpmac, long card_number,
		char *command,
		char *response, size_t response_buffer_length,
		int debug_flag )
{
	char local_buffer[100];
	char *command_ptr, *ptr;
	size_t prefix_length;
	mx_status_type mx_status;

	if ( powerpmac->num_cards == 1 ) {
		command_ptr = command;
	
	} else {
		command_ptr = local_buffer;

		/* Construct the card number prefix. */

		snprintf( command_ptr, sizeof(local_buffer),
			"@%lx", card_number );

		prefix_length = strlen( command_ptr );

		ptr = command_ptr + prefix_length;

		strlcpy( ptr, command, sizeof(local_buffer) - prefix_length );
	}

	mx_status = mxi_powerpmac_command( powerpmac, command,
				response, response_buffer_length, debug_flag );
	return mx_status;
}

/*----*/

MX_EXPORT mx_status_type
mxi_powerpmac_get_variable( MX_POWERPMAC *powerpmac,
			long card_number,
			char *variable_name,
			long variable_type,
			void *variable_ptr,
			int debug_flag )
{
	static const char fname[] = "mxi_powerpmac_get_variable()";

	char command_buffer[100];
	char response[100];
	char *command_ptr;
	int num_items;
	long long_value;
	long *long_ptr;
	double double_value;
	double *double_ptr;
	mx_status_type mx_status;

	if ( powerpmac == (MX_POWERPMAC *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_POWERPMAC pointer passed was NULL." );
	}
	if ( variable_ptr == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The variable pointer passed was NULL." );
	}

	if ( powerpmac->num_cards > 1 ) {
		snprintf( command_buffer, sizeof(command_buffer),
				"@%lx%s", card_number, variable_name );

		command_ptr = command_buffer;
	} else {
		command_ptr = variable_name;
	}

	mx_status = mxi_powerpmac_command( powerpmac, command_ptr,
				response, sizeof response, debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( variable_type ) {
	case MXFT_LONG:
		num_items = sscanf( response, "%ld", &long_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"The response from POWERPMAC '%s', card %ld for the "
				"command '%s' is not an integer number.  "
				"Response = '%s'",
				powerpmac->record->name, card_number,
				command_buffer, response );
		}
		long_ptr = (long *) variable_ptr;

		*long_ptr = long_value;
		break;
	case MXFT_DOUBLE:
		num_items = sscanf( response, "%lg", &double_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"The response from POWERPMAC '%s', card %ld for the "
				"command '%s' is not a number.  "
				"Response = '%s'",
				powerpmac->record->name, card_number,
				command_buffer, response );
		}
		double_ptr = (double *) variable_ptr;

		*double_ptr = double_value;
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Only MXFT_LONG and MXFT_DOUBLE are supported." );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*----*/

MX_EXPORT mx_status_type
mxi_powerpmac_set_variable( MX_POWERPMAC *powerpmac,
			long card_number,
			char *variable_name,
			long variable_type,
			void *variable_ptr,
			int debug_flag )
{
	static const char fname[] = "mxi_powerpmac_set_variable()";

	char command_buffer[100];
	char response[100];
	char *ptr;
	long *long_ptr;
	double *double_ptr;
	size_t string_length;
	mx_status_type mx_status;

	if ( powerpmac == (MX_POWERPMAC *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_POWERPMAC pointer passed was NULL." );
	}
	if ( variable_ptr == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The variable pointer passed was NULL." );
	}

	if ( powerpmac->num_cards > 1 ) {
		snprintf( command_buffer, sizeof(command_buffer),
				"@%lx%s=", card_number, variable_name );
	} else {
		snprintf( command_buffer, sizeof(command_buffer),
				"%s=", variable_name );
	}

	string_length = strlen( command_buffer );

	ptr = command_buffer + string_length;

	switch( variable_type ) {
	case MXFT_LONG:
		long_ptr = (long *) variable_ptr;

		snprintf( ptr, sizeof(command_buffer) - string_length,
				"%ld", *long_ptr );
		break;
	case MXFT_DOUBLE:
		double_ptr = (double *) variable_ptr;

		snprintf( ptr, sizeof(command_buffer) - string_length,
				"%f", *double_ptr );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Only MXFT_LONG and MXFT_DOUBLE are supported." );
	}

	mx_status = mxi_powerpmac_command( powerpmac, command_buffer,
				response, sizeof response, debug_flag );

	return mx_status;
}

/*==================================================================*/

static mx_status_type
mxi_powerpmac_getchar( MX_POWERPMAC *powerpmac, char *c )
{
	static const char fname[] = "mxi_powerpmac_getchar()";

	size_t num_bytes_received;
	mx_status_type mx_status;

	if ( powerpmac == (MX_POWERPMAC *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_POWERPMAC pointer passed was NULL." );
	}
	if ( c == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The character pointer passed was NULL." );
	}

	switch( powerpmac->port_type ) {
	case MX_POWERPMAC_PORT_TYPE_RS232:
	case MX_POWERPMAC_PORT_TYPE_GPASCII:
		mx_status = mx_rs232_getchar( powerpmac->port_record,
						c, MXF_232_WAIT );
		break;

	case MX_POWERPMAC_PORT_TYPE_TCP:
		mx_status = mx_socket_wait_for_event( powerpmac->powerpmac_socket, 5.0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_socket_receive( powerpmac->powerpmac_socket,
						c, sizeof(char),
						&num_bytes_received,
						NULL, 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( num_bytes_received != 1 ) {
			return mx_error( MXE_NETWORK_IO_ERROR, fname,
			"Did not receive a byte from POWERPMAC '%s' "
			"but mx_socket_receive() did not return "
			"an error.",  powerpmac->record->name );
		}
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
		"Port type %ld for POWERPMAC '%s' is not supported by "
		"this function.", powerpmac->port_type, powerpmac->record->name );
		break;
	}

	return mx_status;
}

/*----*/

static mx_status_type
mxi_powerpmac_receive_response_with_getchar( MX_POWERPMAC *powerpmac,
			char *response,
			int response_buffer_length,
			int debug_flag )
{
	static const char fname[] = "mxi_powerpmac_receive_response_with_getchar()";

	int i, bell_received;
	char c;
	mx_status_type mx_status;

	/* This function implicitly assumes I3 == 2.  This means that
	 * all POWERPMAC responses will be terminated by either an <ACK>
	 * or a <BELL> character.  If I6 == 1, a <BELL> character
	 * will be followed by an error code terminated by a <CR>.
	 *
	 * Note that we must leave space at the end of the response
	 * buffer for the terminating null byte.
	 */

	bell_received = FALSE;

	for ( i = 0; i < (response_buffer_length - 1); i++ ) {

		response[i] = '\0';	/* Just in case we get an error. */

		mx_status = mxi_powerpmac_getchar( powerpmac, &c );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		response[i] = c;

		if ( c == MX_ACK ) {

			response[i] = '\0';
			break;			/* Exit the while() loop. */
		} else
		if ( c == MX_BELL ) {

			bell_received = TRUE;

			if ( powerpmac->error_reporting_mode != 1 ) {

				response[i] = '\0';
				break;		/* Exit the while() loop. */
			}
		} else
		if ( c == MX_CR ) {
			if ( bell_received ) {

				response[i] = '\0';
				break;		/* Exit the while() loop. */
			}
		}
	}

	/* Was the response too long to fit in the response buffer? */

	if ( i >= (response_buffer_length - 1) ) {

		/* Null terminate the response buffer. */

		response[ response_buffer_length - 1 ] = '\0';

		/* Clear out any remaining unread characters. */

		mx_status = mx_rs232_discard_unread_input( powerpmac->port_record,
								debug_flag );

		if ( mx_status.code != MXE_SUCCESS ) {
			return mx_status;
		}

		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"The POWERPMAC command response was longer than the buffer length of %d.",
			response_buffer_length );
	}

	/* 'gpascii' appends an additional CR-LF sequence after the
	 * ACK or BEL.
	 */

	if ( powerpmac->port_type == MX_POWERPMAC_PORT_TYPE_GPASCII ) {
		mx_status = mxi_powerpmac_getchar( powerpmac, &c );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( c != MX_CR ) {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Did not receive the CR of a CR-LF pair from "
			"'gpascii' after the ACK or BEL character "
			"for POWERPMAC '%s'.  Instead, we received %#x.",
				powerpmac->record->name, c & 0xff );
		}

		mx_status = mxi_powerpmac_getchar( powerpmac, &c );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( c != MX_LF ) {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Did not receive the LF of a CR-LF pair from "
			"'gpascii' after the ACK or BEL character "
			"for POWERPMAC '%s'.  Instead, we received %#x.",
				powerpmac->record->name, c & 0xff );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*----*/

static mx_status_type
mxi_powerpmac_check_for_errors( MX_POWERPMAC *powerpmac,
			char *command,
			char *response_buffer,
			char *received_response )
{
	static const char fname[] = "mxi_powerpmac_check_for_errors()";

	char *error_code_ptr;
	int length, error_code, num_items;

	/* If the command completed without an error, delete the <ACK>
	 * character at the end of the response and return the rest
	 * of the response to the caller.
	 */

	if ( received_response[0] != MX_BELL ) {

		if ( response_buffer != NULL ) {
			length = (int) strlen( response_buffer );

			if ( response_buffer[length - 1] == MX_ACK ) {
				response_buffer[length - 1] = '\0';
			}
		}
		return MX_SUCCESSFUL_RESULT;
	}

	/* Otherwise, we must handle the error. */

	error_code = -1;

	switch( powerpmac->error_reporting_mode ) {
	case 0:
	case 2:
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
	"The POWERPMAC interface '%s' reported an error for the command '%s'.  "
	"There is no information available about the reason.",
			powerpmac->record->name, command );
	case 3:
		received_response++;	/* Skip over the <CR> character. */
	case 1:
		received_response++;	/* Skip over the <BEL> character. */

		/* If present, delete the <CR> at the end of the
		 * error message.
		 */

		length = (int) strlen( received_response );

		if ( received_response[ length - 1 ] == MX_CR ) {
			received_response[ length - 1 ] = '\0';
		}

		/* The first three letters in the response should be ERR. */

		if ( strncmp( received_response, "ERR", 3 ) != 0 ) {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
	"The first three letters of the response '%s' by POWERPMAC interface '%s' "
	"to the command '%s' are not 'ERR', even though an error code was "
	"supposedly returned.",
			received_response, powerpmac->record->name, command );
		}

		/* Skip over the first three letters and parse the rest
		 * as a number.
		 */

		error_code_ptr = received_response + 3;

		num_items = sscanf( error_code_ptr, "%d", &error_code );

		if ( num_items != 1 ) {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
	"The POWERPMAC interface '%s' reported an error for the command '%s'.  "
	"An attempt to parse the error status failed.  "
	"POWERPMAC error message = '%s'",
			powerpmac->record->name, command, received_response );
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The POWERPMAC interface '%s' reported an error for the command '%s', "
	"but the POWERPMAC error reporting mode has the unexpected value of %ld.  "
	"This interferes with the parsing of error messages from the POWERPMAC.  "
	"POWERPMAC error message = '%s'",
			powerpmac->record->name, command,
			powerpmac->error_reporting_mode, received_response );
	}

	if ( error_code < 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The error code (%d) returned for POWERPMAC command '%s' was less than 0.  "
	"This shouldn't be able to happen and is probably an MX driver bug."
	"POWERPMAC error message = '%s'",
			error_code, command, received_response );
	}

	if ( error_code >= mxi_powerpmac_num_error_messages ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
	"The POWERPMAC interface '%s' reported an error for the command '%s'.  "
	"Error code = %d, error message = 'Unknown error code %d'",
			powerpmac->record->name, command,
			error_code, error_code );
	}

	return mx_error( mxi_powerpmac_error_messages[error_code].mx_error_code,
			fname,
	"The POWERPMAC interface '%s' reported an error for the command '%s'.  "
	"Error code = %d, error message = '%s'",
			powerpmac->record->name, command, error_code, 
			mxi_powerpmac_error_messages[error_code].error_message );
}

/*==================================================================*/

static mx_status_type
mxi_powerpmac_std_command( MX_POWERPMAC *powerpmac, char *command,
		char *response_buffer, size_t response_buffer_length,
		int debug_flag )
{
	static const char fname[] = "mxi_powerpmac_std_command()";

	char alt_response_buffer[40];
	char *received_response;
	int received_response_length;
	mx_status_type mx_status;

	/* WARNING: Please note that this routine _assumes_ that I3 == 2. */

	if ( powerpmac == (MX_POWERPMAC *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_POWERPMAC pointer passed was NULL." );
	}

	/* Send the command string. */

	mx_status = mxi_powerpmac_std_send_command( powerpmac, command, debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the response. */

	if ( response_buffer == NULL ) {
		received_response = alt_response_buffer;
		received_response_length = sizeof(alt_response_buffer);
	} else {
		received_response = response_buffer;
		received_response_length = (int) response_buffer_length;
	}

	mx_status = mxi_powerpmac_std_receive_response( powerpmac,
					received_response,
					received_response_length,
					debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_powerpmac_check_for_errors( powerpmac, command,
						response_buffer,
						received_response );

	return mx_status;
}

/*----*/

static mx_status_type
mxi_powerpmac_std_send_command( MX_POWERPMAC *powerpmac,
			char *command,
			int debug_flag )
{
	static const char fname[] = "mxi_powerpmac_std_send_command()";

	mx_status_type mx_status;
#if MXI_POWERPMAC_DEBUG_TIMING
	MX_HRT_RS232_TIMING command_timing;
#endif

	if ( powerpmac == (MX_POWERPMAC *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_POWERPMAC pointer passed was NULL." );
	}
	if ( command == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"command buffer pointer passed was NULL." );
	}

#if MXI_POWERPMAC_DEBUG_TIMING
	MX_HRT_RS232_START_COMMAND( command_timing, 1 + strlen(command) );
#endif

	switch( powerpmac->port_type ) {
	case MX_POWERPMAC_PORT_TYPE_RS232:
		/* First, send a ctrl-Z to make the serial port
		 * the active interface.
		 */

		mx_status = mx_rs232_putchar( powerpmac->port_record,
						MX_CTRL_Z, MXF_232_WAIT );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Note: We do not 'break' here.  Instead, we fall through
		 * to the gpascii case.
		 */

	case MX_POWERPMAC_PORT_TYPE_GPASCII:
		mx_status = mx_rs232_putline( powerpmac->port_record,
						command, NULL, debug_flag );
		break;
	default:
		mx_status = mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unknown POWERPMAC interface port_type = %ld.",
			powerpmac->port_type );
		break;
	}

#if MXI_POWERPMAC_DEBUG_TIMING
	MX_HRT_RS232_END_COMMAND( command_timing );

	MX_HRT_RS232_COMMAND_RESULTS( command_timing, command, fname );
#endif

	return mx_status;
}

static mx_status_type
mxi_powerpmac_std_receive_response( MX_POWERPMAC *powerpmac,
			char *response,
			int response_buffer_length,
			int debug_flag )
{
	static const char fname[] = "mxi_powerpmac_std_receive_response()";

	size_t length;
	mx_status_type mx_status;
#if MXI_POWERPMAC_DEBUG_TIMING
	MX_HRT_RS232_TIMING response_timing;
#endif

	if ( powerpmac == (MX_POWERPMAC *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_POWERPMAC pointer passed was NULL." );
	}
	if ( response == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"response buffer pointer passed was NULL." );
	}

#if MXI_POWERPMAC_DEBUG_TIMING
	MX_HRT_RS232_START_RESPONSE( response_timing, powerpmac->port_record );
#endif

	switch( powerpmac->port_type ) {
	case MX_POWERPMAC_PORT_TYPE_RS232:
	case MX_POWERPMAC_PORT_TYPE_GPASCII:
		mx_status = mxi_powerpmac_receive_response_with_getchar( powerpmac,
					response, response_buffer_length,
					debug_flag );
		break;
	default:
		mx_status = mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unknown POWERPMAC interface port_type = %ld.",
			powerpmac->port_type );
		break;
	}

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_status;
	}

	/* Is there a trailing CR?  If so, then overwrite it with
	 * a null character.
	 */

	length = strlen( response );

	if ( length > 0 ) {
		if ( response[length-1] == MX_CR ) {
			response[length-1] = '\0';
		}
	}

#if MXI_POWERPMAC_DEBUG_TIMING
	MX_HRT_RS232_END_RESPONSE( response_timing, length );

	MX_HRT_RS232_RESPONSE_RESULTS( response_timing, response, fname );
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

static mx_status_type
mxi_powerpmac_tcp_flush( MX_POWERPMAC *powerpmac,
		mx_bool_type debug_flag )
{
	static const char fname[] = "mxi_powerpmac_tcp_flush()";

	ETHERNETCMD ethernet_cmd;
	uint8_t ack_byte;
	mx_status_type mx_status;

	if ( powerpmac == (MX_POWERPMAC *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_POWERPMAC pointer passed was NULL." );
	}

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: flushing unread POWERPMAC input.", fname));
	}

	/* Send the flush (^X) command to the POWERPMAC. */

	ethernet_cmd.RequestType = VR_DOWNLOAD;
	ethernet_cmd.Request     = VR_POWERPMAC_FLUSH;
	ethernet_cmd.wValue      = 0;
	ethernet_cmd.wIndex      = 0;
	ethernet_cmd.wLength     = 0;
	ethernet_cmd.bData[0]    = '\0';

	mx_status = mx_socket_send( powerpmac->powerpmac_socket, &ethernet_cmd,
							ETHERNETCMDSIZE );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait for the POWERPMAC to send us a response to the flush. */

	mx_status = mx_socket_wait_for_event( powerpmac->powerpmac_socket, 5.0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* See if there was an error. */

	mx_status = mx_socket_receive( powerpmac->powerpmac_socket,
				&ack_byte, sizeof(ack_byte),
				NULL, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( ack_byte != '@' ) {
		return mx_error( MXE_NETWORK_IO_ERROR, fname,
		"Did not see the expected '@' response to a POWERPMAC flush "
		"command for POWERPMAC '%s'", powerpmac->record->name );
	}

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: POWERPMAC flush complete.", fname));
	}

	/* Discard any remaining bytes in the host's input queue. */

	mx_status = mx_socket_discard_unread_input( powerpmac->powerpmac_socket );

	return mx_status;
}

/*----*/

static mx_status_type
mxi_powerpmac_tcp_command( MX_POWERPMAC *powerpmac, char *command,
			char *response, size_t response_buffer_length,
			mx_bool_type debug_flag )
{
	static const char fname[] = "mxi_powerpmac_tcp_command()";

	ETHERNETCMD ethernet_cmd;
	uint8_t ack_byte;
	size_t num_bytes_received, length;
	mx_status_type mx_status;

	if ( powerpmac == (MX_POWERPMAC *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_POWERPMAC pointer passed was NULL." );
	}
	if ( command == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"command buffer pointer passed was NULL." );
	}

	if ( response == (char *) NULL ) {
		/* If no response is expected from the POWERPMAC, then send
		 * the command using VR_POWERPMAC_SENDLINE.
		 */

		/* Send the command to the POWERPMAC. */

		ethernet_cmd.RequestType = VR_DOWNLOAD;
		ethernet_cmd.Request     = VR_POWERPMAC_SENDLINE;
		ethernet_cmd.wValue      = 0;
		ethernet_cmd.wIndex      = 0;
		ethernet_cmd.wLength     = htons( (uint16_t) strlen(command) );

		strlcpy( (char *) &ethernet_cmd.bData[0],
			command, sizeof(ethernet_cmd.bData) );

		mx_status = mx_socket_send( powerpmac->powerpmac_socket, &ethernet_cmd,
					ETHERNETCMDSIZE + strlen( command ) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* See if there was an error. */

		mx_status = mx_socket_receive( powerpmac->powerpmac_socket,
					&ack_byte, sizeof(ack_byte),
					NULL, NULL, 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if MXI_POWERPMAC_DEBUG_TCP
		MX_DEBUG(-2,("%s: ack_byte = '%c' (%#x)",
			fname, ack_byte, ack_byte));
#endif
		return MX_SUCCESSFUL_RESULT;
	}

	/* If we get here, then we _do_ expect a response from the POWERPMAC. */

	ethernet_cmd.RequestType = VR_DOWNLOAD;
	ethernet_cmd.Request     = VR_POWERPMAC_GETRESPONSE;
	ethernet_cmd.wValue      = 0;
	ethernet_cmd.wIndex      = 0;
	ethernet_cmd.wLength     = htons( (uint16_t) strlen(command) );

	strlcpy( (char *) &ethernet_cmd.bData[0],
			command, sizeof(ethernet_cmd.bData) );


	mx_status = mx_socket_send( powerpmac->powerpmac_socket, &ethernet_cmd,
					ETHERNETCMDSIZE + strlen(command) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_powerpmac_tcp_receive_response( powerpmac->powerpmac_socket,
					response, response_buffer_length,
					&num_bytes_received, debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;
#if MXI_POWERPMAC_DEBUG_TCP
	MX_DEBUG(-2,("%s: num_bytes_received = %d",
		fname, (int) num_bytes_received));

	{
		int i;

		for ( i = 0; i < num_bytes_received; i++ ) {
			MX_DEBUG(-2,("%s: response[%d] = '%c' %#x",
				fname, i, response[i], response[i]));
		}
	}
#endif

	mx_status = mxi_powerpmac_check_for_errors( powerpmac, command,
						response, response );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	length = strlen(response);

	if ( response[length - 1] == '\r' ) {
		response[length - 1] = '\0';
	} else
	if ( response[length - 1] == '\n' ) {
		response[length - 1] = '\0';
	}

	return MX_SUCCESSFUL_RESULT;
}

/*----*/

static mx_status_type
mxi_powerpmac_tcp_receive_response( MX_SOCKET *mx_socket,
				char *response,
				size_t response_buffer_length,
				size_t *num_bytes_received,
				int debug_flag )
{
	static const char fname[] = "mxi_powerpmac_tcp_receive_response()";

	size_t local_num_bytes_received;
	mx_status_type mx_status;

	if ( mx_socket == (MX_SOCKET *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_SOCKET pointer passed was NULL." );
	}

	if ( response == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The response buffer pointer passed was NULL." );
	}

	memset( response, 0, response_buffer_length );

#if MXI_POWERPMAC_DEBUG_TCP
	MX_DEBUG(-2,("%s: Before mx_socket_wait_for_event()", fname));
#endif

	mx_status = mx_socket_wait_for_event( mx_socket, 5.0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_socket_receive( mx_socket,
					response, response_buffer_length,
					&local_num_bytes_received, "\r\006", 2);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_POWERPMAC_DEBUG_TCP
	MX_DEBUG(-2,("%s: num_bytes_received = %d",
		fname, (int) local_num_bytes_received));

	{
		int i;

		for ( i = 0; i < local_num_bytes_received; i++ ) {
			MX_DEBUG(-2,("%s: response[%d] = '%c' %#x",
				fname, i, response[i], response[i]));
		}
	}
#endif

	if ( response[local_num_bytes_received - 1] == '\r' ) {
		response[local_num_bytes_received - 1] = '\0';
	} else
	if ( response[local_num_bytes_received - 1] == '\n' ) {
		response[local_num_bytes_received - 1] = '\0';
	} else
	if ( response[local_num_bytes_received - 1] == MX_ACK ) {
		response[local_num_bytes_received - 1] = '\0';
	}

	if ( num_bytes_received != NULL ) {
		*num_bytes_received = local_num_bytes_received;
	}

	return mx_status;
}

/*==================================================================*/

static mx_status_type
mxi_powerpmac_gpascii_login( MX_POWERPMAC *powerpmac )
{
	static const char fname[] = "mxi_powerpmac_gpascii_login()";

	MX_RS232 *rs232;
	char response[1000];
	unsigned long num_bytes_received;
	mx_status_type mx_status;

	rs232 = (MX_RS232 *) powerpmac->port_record->record_class_struct;

	if ( rs232 == (MX_RS232 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_RS232 pointer for POWERPMAC port "
			"record '%s' is NULL.",
				powerpmac->port_record->name );
	}

	/* We must login to the PowerPOWERPMAC. */

#if MXI_POWERPMAC_DEBUG_LOGIN
	MX_DEBUG(-2,("%s: Beginning login to PowerPOWERPMAC", fname));
#endif

	/* Give the server a chance to send Telnet negotiation commands. */

	mx_status = mxi_telnet_handle_commands( rs232, 200, 50 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_POWERPMAC_DEBUG_LOGIN
	MX_DEBUG(-2,("%s: Logging in as '%s'.",
			fname, powerpmac->gpascii_username));
#endif

	/* Send the user name. */

	mx_status = mx_rs232_putline( powerpmac->port_record,
				powerpmac->gpascii_username,
				NULL, MXI_POWERPMAC_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait until a response is available. */

	while (1) {
		mx_status = mx_rs232_num_input_bytes_available(
					powerpmac->port_record,
					&num_bytes_received );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( num_bytes_received > 0 )
			break;

		mx_msleep(100);
	}

	/* Discard the response. */

	mx_status = mx_rs232_discard_unread_input( powerpmac->port_record,
						MXI_POWERPMAC_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_POWERPMAC_DEBUG_LOGIN
	MX_DEBUG(-2,("%s: Sending the password '%s'.",
			fname, powerpmac->gpascii_password));
#endif

	/* Send the password. */

	mx_status = mx_rs232_putline( powerpmac->port_record,
				powerpmac->gpascii_password,
				NULL, MXI_POWERPMAC_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait until a response is available. */

	while (1) {
		mx_status = mx_rs232_num_input_bytes_available(
					powerpmac->port_record,
					&num_bytes_received );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( num_bytes_received > 0 )
			break;

		mx_msleep(100);
	}

	mx_msleep(500);

	/* Discard all login messages. */

	mx_status = mx_rs232_discard_unread_input( powerpmac->port_record,
						MXI_POWERPMAC_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Turn off echoing of commands. */

#if MXI_POWERPMAC_DEBUG_LOGIN
	MX_DEBUG(-2,("%s: disabling echoing of input.", fname));
#endif

	mx_status = mx_rs232_putline( powerpmac->port_record,
					"stty -echo",
					NULL, MXI_POWERPMAC_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_msleep(500);

	/* Discard the response to the stty -echo command. */

	mx_status = mx_rs232_discard_unread_input( powerpmac->port_record,
						MXI_POWERPMAC_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_POWERPMAC_DEBUG_LOGIN
	MX_DEBUG(-2,("%s: sending the 'gpascii' command.", fname));
#endif

	/* Send the 'gpascii' command. */

	mx_status = mx_rs232_putline( powerpmac->port_record, "gpascii", NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Wait until a response is available. */

#if MXI_POWERPMAC_DEBUG_LOGIN
	MX_DEBUG(-2,
	("%s: waiting for the 'gpascii' startup message.", fname));
#endif

	while (1) {
		mx_status = mx_rs232_num_input_bytes_available(
					powerpmac->port_record,
					&num_bytes_received );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( num_bytes_received > 0 )
			break;

		mx_msleep(100);
	}

	mx_msleep(500);

	/* Look for the expected response. */

	mx_status = mx_rs232_getline( powerpmac->port_record,
					response, sizeof(response),
					NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_POWERPMAC_DEBUG_LOGIN
	MX_DEBUG(-2,("%s: received response '%s'.", fname, response));
#endif

	if ( strcmp( response, "STDIN Open for ASCII Input" ) != 0 ) {
		return mx_error( MXE_IPC_IO_ERROR, fname,
		"Did not get the expected response to the 'gpascii' "
		"command for PowerPOWERPMAC '%s'.  Instead, we got '%s'.",
			powerpmac->record->name, response );
	}

	mx_msleep(500);

	/* Discard any leftover characters. */

	mx_status = mx_rs232_discard_unread_input( powerpmac->port_record,
						MXI_POWERPMAC_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If we get here, then we are done with the login and
	 * setup process, so we can treat the connection as a
	 * normal serial connection from now on.
	 */

#if MXI_POWERPMAC_DEBUG_LOGIN
	MX_DEBUG(-2,("%s: login complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*==================================================================*/

#if HAVE_POWER_POWERPMAC_LIBRARY

static mx_status_type
mxi_powerpmac_gplib_command( MX_POWERPMAC *powerpmac, char *command,
			char *response, size_t response_buffer_length,
			mx_bool_type debug_flag )
{
	static const char fname[] = "mxi_powerpmac_gplib_command()";

	int powerpowerpmac_status;

	if ( powerpmac == (MX_POWERPMAC *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_POWERPMAC pointer passed was NULL." );
	}
	if ( command == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"command buffer pointer passed was NULL." );
	}

	if ( (response == NULL) || (response_buffer_length == 0) ) {
		powerpowerpmac_status = Command( command );
	} else {
		powerpowerpmac_status = GetResponse( command,
					response, response_buffer_length, 0 );
	}

	if ( powerpowerpmac_status == 0 ) {
		return MX_SUCCESSFUL_RESULT;
	} else {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Power POWERPMAC command '%s' returned with error code = %d.",
			command, powerpowerpmac_status );
	}
}

#endif /* HAVE_POWER_POWERPMAC_LIBRARY */

/*==================================================================*/

#if HAVE_EPICS

static mx_status_type
mxi_powerpmac_epics_ect_command( MX_POWERPMAC *powerpmac, char *command,
			char *response, size_t response_buffer_length,
			mx_bool_type debug_flag )
{
	static const char fname[] = "mxi_powerpmac_epics_ect_command()";

	mx_status_type mx_status;

	if ( powerpmac == (MX_POWERPMAC *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_POWERPMAC pointer passed was NULL." );
	}
	if ( command == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"command buffer pointer passed was NULL." );
	}

	/* Send the command. */

	mx_status = mx_caput( &(powerpmac->strcmd_pv), MX_CA_STRING, 1, command );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the response, if any. */

	if ( response != NULL ) {
		mx_status = mx_caget( &(powerpmac->strrsp_pv),
					MX_CA_STRING, 1, response );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}
	return MX_SUCCESSFUL_RESULT;
}

#endif

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
			mx_status = mxi_powerpmac_command( powerpmac, powerpmac->command,
						NULL, 0, MXI_POWERPMAC_DEBUG );

			break;
		case MXLV_POWERPMAC_COMMAND_WITH_RESPONSE:
			mx_status = mxi_powerpmac_command( powerpmac, powerpmac->command,
				powerpmac->response, MX_POWERPMAC_MAX_COMMAND_LENGTH,
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

