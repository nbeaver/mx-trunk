/*
 * Name:    i_pmac.c
 *
 * Purpose: MX driver for Delta Tau PMAC controllers.  Note that this is
 *          the driver for talking to the PMAC itself.  PMAC-controlled
 *          motors are interfaced via the driver in libMx/d_pmac.c.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_PMAC_DEBUG			FALSE

#define MXI_PMAC_DEBUG_TIMING		FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "mxconfig.h"
#include "mx_util.h"
#include "mx_inttypes.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_rs232.h"

#if HAVE_EPICS
#include "mx_epics.h"
#endif /* HAVE_EPICS */

#include "i_pmac.h"

#if MXI_PMAC_DEBUG_TIMING
#  include "mx_hrt_debug.h"
#endif

MX_RECORD_FUNCTION_LIST mxi_pmac_record_function_list = {
	NULL,
	mxi_pmac_create_record_structures,
	mxi_pmac_finish_record_initialization,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_pmac_open,
	NULL,
	NULL,
	mxi_pmac_resynchronize,
	mxi_pmac_special_processing_setup
};

MX_RECORD_FIELD_DEFAULTS mxi_pmac_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_PMAC_STANDARD_FIELDS
};

mx_length_type mxi_pmac_num_record_fields
		= sizeof( mxi_pmac_record_field_defaults )
			/ sizeof( mxi_pmac_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_pmac_rfield_def_ptr
			= &mxi_pmac_record_field_defaults[0];

static mx_status_type mxi_pmac_process_function( void *record_ptr,
						void *record_field_ptr,
						int operation );

/*==========================*/

MX_EXPORT mx_status_type
mxi_pmac_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_pmac_create_record_structures()";

	MX_PMAC *pmac;

	/* Allocate memory for the necessary structures. */

	pmac = (MX_PMAC *) malloc( sizeof(MX_PMAC) );

	if ( pmac == (MX_PMAC *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_PMAC structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_type_struct = pmac;
	record->record_class_struct = NULL;
	record->class_specific_function_list = NULL;

	pmac->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_pmac_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] = "mxi_pmac_finish_record_initialization()";

	MX_PMAC *pmac;
	MX_RS232 *rs232;
	char *port_type_name;
	int i, length;

	pmac = (MX_PMAC *) record->record_type_struct;

	/* Identify the port_type of the record. */

	port_type_name = pmac->port_type_name;

	length = strlen( port_type_name );

	for ( i = 0; i < length; i++ ) {
		if ( isupper( (int) (port_type_name[i]) ) ) {
			port_type_name[i] = tolower( (int)(port_type_name[i]) );
		}
	}

	if ( strcmp( port_type_name, "rs232" ) == 0 ) {
		pmac->port_type = MX_PMAC_PORT_TYPE_RS232;

	} else if ( strcmp( port_type_name, "epics_ect" ) == 0 ) {
		pmac->port_type = MX_PMAC_PORT_TYPE_EPICS_TC;

	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The PMAC interface port_type '%s' is not a valid port_type.",
			port_type_name );
	}

	/* Perform port_type specific processing. */

	switch( pmac->port_type ) {
	case MX_PMAC_PORT_TYPE_RS232:
		pmac->port_record = mx_get_record( record->list_head,
							pmac->port_args );

		if ( pmac->port_record == NULL ) {
			return mx_error( MXE_NOT_FOUND, fname,
			"The RS-232 interface record '%s' does not exist.",
				pmac->port_args );
		}
		if ( pmac->port_record->mx_superclass != MXR_INTERFACE ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
				"'%s' is not an interface record.",
				pmac->port_record->name );
		}
		if ( pmac->port_record->mx_class != MXI_RS232 ) {
			return mx_error( MXE_TYPE_MISMATCH, fname,
				"'%s' is not an RS-232 interface.",
				pmac->port_record->name );
		}
		rs232 = (MX_RS232 *) pmac->port_record->record_class_struct;

		if ( rs232 == (MX_RS232 *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_RS232 pointer for RS-232 record '%s' is NULL.",
				pmac->port_record->name );
		}

		if ( ( rs232->read_terminators != MX_CR )
		  || ( rs232->write_terminators != MX_CR ) )
		{
			return mx_error(MXE_SOFTWARE_CONFIGURATION_ERROR, fname,
		"RS-232 port '%s' used by PMAC interface '%s' "
		"has incorrectly configured read "
		"and/or write terminators.  The MX PMAC RS-232 support "
		"requires that the read and write terminators for the "
		"RS-232 port both be set to <CR> (0x0d).",
				pmac->port_record->name, record->name );
		}
		break;

	case MX_PMAC_PORT_TYPE_EPICS_TC:
		pmac->port_record = NULL;

#if HAVE_EPICS
		mx_epics_pvname_init( &(pmac->strcmd_pv),
					"%sStrCmd", pmac->port_args );

		mx_epics_pvname_init( &(pmac->strrsp_pv),
					"%sStrRsp", pmac->port_args );
#endif
		break;
	}

	/* Mark the PMAC device as being closed. */

	record->record_flags |= ( ~MXF_REC_OPEN );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_pmac_open( MX_RECORD *record )
{
	return mxi_pmac_resynchronize( record );
}

MX_EXPORT mx_status_type
mxi_pmac_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxi_pmac_resynchronize()";

	MX_PMAC *pmac;
	char response[80];
	char pmac_type_name[20];
	int num_items;
	mx_status_type mx_status;

	MX_DEBUG( 2, ("%s invoked.", fname));

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	pmac = (MX_PMAC *) record->record_type_struct;

	if ( pmac == (MX_PMAC *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_PMAC pointer for record '%s' is NULL.", record->name);
	}

	/* Verify that the PMAC controller is active by asking it for
	 * its card type.
	 */

	mx_status = mxi_pmac_command( pmac, "TYPE",
				response, sizeof response, MXI_PMAC_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%s,", pmac_type_name );

	if ( num_items != 1 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"The device attached to this interface does not "
			"seem to be a PMAC controller."
			"Response to PMAC TYPE command = '%s'",
				response );
	}

	if ( strncmp( pmac_type_name, "PMAC1",5 ) == 0 ) {
		pmac->pmac_type = MX_PMAC_TYPE_PMAC1;

	} else
	if ( strncmp( pmac_type_name, "PMAC2",5 ) == 0 ) {
		pmac->pmac_type = MX_PMAC_TYPE_PMAC2;

	} else
	if ( strncmp( pmac_type_name, "PMACUL",6 ) == 0 ) {
		pmac->pmac_type = MX_PMAC_TYPE_PMACUL;

	} else
	if ( strncmp( pmac_type_name, "TURBO1",6 ) == 0 ) {
		pmac->pmac_type = MX_PMAC_TYPE_TURBO1;

	} else
	if ( strncmp( pmac_type_name, "TURBO2",6 ) == 0 ) {
		pmac->pmac_type = MX_PMAC_TYPE_TURBO2;

	} else
	if ( strncmp( pmac_type_name, "TURBOU",6 ) == 0 ) {
		pmac->pmac_type = MX_PMAC_TYPE_TURBOU;

	} else {
		if ( ( strncmp( pmac_type_name, "PMAC", 4 ) != 0 )
		  && ( strncmp( pmac_type_name, "TURBO", 5 ) != 0 ) )
		{
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
				"The device attached to this interface does "
				"not seem to be a PMAC controller."
				"Response to PMAC TYPE command = '%s'",
					response );
		} else {
			return mx_error( MXE_TYPE_MISMATCH, fname,
			"Unrecognized PMAC type '%s' for PMAC '%s'.",
				pmac_type_name, record->name );
		}
	}

	/* Ask for the style of error message reporting. */

	mx_status = mxi_pmac_command( pmac, "I6",
				response, sizeof response, MXI_PMAC_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%d", &(pmac->i6_variable) );

	if ( num_items != 1 ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
		"Unable to parse response to I6 command.  Response = '%s'",
			response );
	}

	/* Now ask for PROM firmware version. */

	mx_status = mxi_pmac_command( pmac, "VERSION",
				response, sizeof response, MXI_PMAC_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%d.%d",
			&(pmac->major_version), &(pmac->minor_version) );

	if ( num_items != 2 ) {
		return mx_error( MXE_UNPARSEABLE_STRING, fname,
			"Unable to parse response to VERSION command.  "
			"Response = '%s'", response );
	}

#if MXI_PMAC_DEBUG
	MX_DEBUG(-2, ("%s: PMAC version: major = %d, minor = %d",
		fname, pmac->major_version, pmac->minor_version));

	MX_DEBUG(-2,("%s: I6 = %d", fname, pmac->i6_variable));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_pmac_special_processing_setup( MX_RECORD *record )
{
	static const char fname[] = "mxi_pmac_special_processing_setup()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

	MX_DEBUG(2,("%s invoked.", fname));

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_PMAC_COMMAND:
		case MXLV_PMAC_RESPONSE:
		case MXLV_PMAC_COMMAND_WITH_RESPONSE:
			record_field->process_function
					    = mxi_pmac_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

/* === Functions specific to this driver. === */

static mx_status_type
mxi_pmac_send_command( MX_PMAC *, char *, int );

static mx_status_type
mxi_pmac_receive_response( MX_PMAC *, char *, int, int );

/*--*/

static mx_status_type
mxi_pmac_rs232_send_command( MX_PMAC *, char *, int );

static mx_status_type
mxi_pmac_rs232_receive_response( MX_PMAC *, char *, int, int );
				

#if HAVE_EPICS
static mx_status_type
mxi_pmac_epics_tc_command( MX_PMAC *, char *, char *, int, int );
#endif

static struct {
	int mx_error_code;
	char error_message[100];
} mxi_pmac_error_messages[] = {
	{ MXE_INTERFACE_ACTION_FAILED,				/* 0 */
		"Unknown error code 0" },
	{ MXE_CLIENT_REQUEST_DENIED,				/* 1 */
		"The requested command is not allowed while "
			"a PMAC motion program is executing" },
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

static int mxi_pmac_num_error_messages =
	sizeof( mxi_pmac_error_messages )
	/ sizeof( mxi_pmac_error_messages[0] );

/*----*/

MX_EXPORT mx_status_type
mxi_pmac_command( MX_PMAC *pmac, char *command,
		char *response, int response_buffer_length,
		int debug_flag )
{
	static const char fname[] = "mxi_pmac_command()";

	char alt_response_buffer[40];
	char *receive_response, *error_code_ptr;
	int receive_response_length;
	int length, error_code, num_items;
	mx_status_type mx_status;

	/* WARNING: Please note that this routine _assumes_ that I3 == 2. */

	if ( pmac == (MX_PMAC *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_PMAC pointer passed was NULL." );
	}

#if HAVE_EPICS
	/* Tom Coleman's EPICS driver is handled separately. */

	if ( pmac->port_type == MX_PMAC_PORT_TYPE_EPICS_TC ) {
		return mxi_pmac_epics_tc_command( pmac, command, 
					response, response_buffer_length,
					debug_flag );
	}
#endif

	/* Send the command string. */

	mx_status = mxi_pmac_send_command( pmac, command, debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the response. */

	if ( response == NULL ) {
		receive_response = alt_response_buffer;
		receive_response_length = sizeof(alt_response_buffer);
	} else {
		receive_response = response;
		receive_response_length = response_buffer_length;
	}

	mx_status = mxi_pmac_receive_response( pmac,
					receive_response,
					receive_response_length,
					debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If the command completed without an error, delete the <ACK>
	 * character at the end of the response and return the rest
	 * of the response to the caller.
	 */

	if ( receive_response[0] != MX_BELL ) {

		if ( response != NULL ) {
			length = strlen( response );

			if ( response[length - 1] == MX_ACK ) {
				response[length - 1] = '\0';
			}
		}
		return MX_SUCCESSFUL_RESULT;
	}

	/* Otherwise, we must handle the error. */

	error_code = -1;

	switch( pmac->i6_variable ) {
	case 0:
	case 2:
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
	"The PMAC interface '%s' reported an error for the command '%s'.  "
	"There is no information available about the reason.",
			pmac->record->name, command );
		break;
	case 3:
		receive_response++;	/* Skip over the <CR> character. */
	case 1:
		receive_response++;	/* Skip over the <BEL> character. */

		/* If present, delete the <CR> at the end of the
		 * error message.
		 */

		length = strlen( receive_response );

		if ( receive_response[ length - 1 ] == MX_CR ) {
			receive_response[ length - 1 ] = '\0';
		}

		/* The first three letters in the response should be ERR. */

		if ( strncmp( receive_response, "ERR", 3 ) != 0 ) {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
	"The first three letters of the response '%s' by PMAC interface '%s' "
	"to the command '%s' are not 'ERR', even though an error code was "
	"supposedly returned.", receive_response, pmac->record->name, command );
		}

		/* Skip over the first three letters and parse the rest
		 * as a number.
		 */

		error_code_ptr = receive_response + 3;

		num_items = sscanf( error_code_ptr, "%d", &error_code );

		if ( num_items != 1 ) {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
	"The PMAC interface '%s' reported an error for the command '%s'.  "
	"An attempt to parse the error status failed.  "
	"PMAC error message = '%s'",
			pmac->record->name, command, receive_response );
		}
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The PMAC interface '%s' reported an error for the command '%s', "
	"but the PMAC I6 variable has the unexpected value of %d.  "
	"This interferes with the parsing of error messages from the PMAC.  "
	"PMAC error message = '%s'",
			pmac->record->name, command,
			pmac->i6_variable, receive_response );
		break;
	}

	if ( error_code < 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"The error code (%d) returned for PMAC command '%s' was less than 0.  "
	"This shouldn't be able to happen and is probably an MX driver bug."
	"PMAC error message = '%s'",
			error_code, command, receive_response );
	}

	if ( error_code >= mxi_pmac_num_error_messages ) {
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
	"The PMAC interface '%s' reported an error for the command '%s'.  "
	"Error code = %d, error message = 'Unknown error code %d'",
			pmac->record->name, command,
			error_code, error_code );
	}

	return mx_error( mxi_pmac_error_messages[error_code].mx_error_code,
			fname,
	"The PMAC interface '%s' reported an error for the command '%s'.  "
	"Error code = %d, error message = '%s'",
			pmac->record->name, command, error_code, 
			mxi_pmac_error_messages[error_code].error_message );
}

/*----*/

MX_EXPORT mx_status_type
mxi_pmac_card_command( MX_PMAC *pmac, int card_number,
		char *command,
		char *response, int response_buffer_length,
		int debug_flag )
{
	static const char fname[] = "mxi_pmac_card_command()";

	char local_command_buffer[100];
	char *command_ptr, *ptr;
	size_t prefix_length, total_length;
	mx_status_type mx_status;

	if ( pmac->num_cards == 1 ) {
		command_ptr = command;
	
	} else {
		command_ptr = local_command_buffer;

		/* Construct the card number prefix. */

		sprintf( command_ptr, "@%x", card_number );

		prefix_length = strlen( command_ptr );

		total_length = strlen( command ) + prefix_length;

		if ( total_length > ( sizeof(local_command_buffer) - 1 ) ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"The command '%s' is too long to fit in the local command buffer.  "
	"Maximum length = %ld.",  command,
		   (long)( sizeof(local_command_buffer) - prefix_length - 1 ));
		}

		ptr = command_ptr + prefix_length;

		strcpy( ptr, command );
	}

	mx_status = mxi_pmac_command( pmac, command,
				response, response_buffer_length, debug_flag );
	return mx_status;
}

/*----*/

MX_EXPORT mx_status_type
mxi_pmac_get_variable( MX_PMAC *pmac,
			int card_number,
			char *variable_name,
			int variable_type,
			void *variable_ptr,
			int debug_flag )
{
	static const char fname[] = "mxi_pmac_get_variable()";

	char command_buffer[100];
	char response[100];
	char *command_ptr;
	int num_items;
	int32_t int32_value;
	int32_t *int32_ptr;
	double double_value;
	double *double_ptr;
	mx_status_type mx_status;

	if ( pmac == (MX_PMAC *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PMAC pointer passed was NULL." );
	}
	if ( variable_ptr == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The variable pointer passed was NULL." );
	}

	if ( pmac->num_cards > 1 ) {
		sprintf( command_buffer, "@%x%s", card_number, variable_name );

		command_ptr = command_buffer;
	} else {
		command_ptr = variable_name;
	}

	mx_status = mxi_pmac_command( pmac, command_ptr,
				response, sizeof response, debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( variable_type ) {
	case MXFT_INT32:
		num_items = sscanf( response, "%" SCNd32, &int32_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"The response from PMAC '%s', card %d for the "
				"command '%s' is not an integer number.  "
				"Response = '%s'",
				pmac->record->name, card_number,
				command_buffer, response );
		}
		int32_ptr = (int32_t *) variable_ptr;

		*int32_ptr = int32_value;
		break;
	case MXFT_DOUBLE:
		num_items = sscanf( response, "%lg", &double_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_UNPARSEABLE_STRING, fname,
				"The response from PMAC '%s', card %d for the "
				"command '%s' is not a number.  "
				"Response = '%s'",
				pmac->record->name, card_number,
				command_buffer, response );
		}
		double_ptr = (double *) variable_ptr;

		*double_ptr = double_value;
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Only MXFT_INT32 and MXFT_DOUBLE are supported." );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*----*/

MX_EXPORT mx_status_type
mxi_pmac_set_variable( MX_PMAC *pmac,
			int card_number,
			char *variable_name,
			int variable_type,
			void *variable_ptr,
			int debug_flag )
{
	static const char fname[] = "mxi_pmac_set_variable()";

	char command_buffer[100];
	char response[100];
	char *ptr;
	int32_t *int32_ptr;
	double *double_ptr;
	mx_status_type mx_status;

	if ( pmac == (MX_PMAC *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PMAC pointer passed was NULL." );
	}
	if ( variable_ptr == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The variable pointer passed was NULL." );
	}

	if ( pmac->num_cards > 1 ) {
		sprintf( command_buffer, "@%x%s=", card_number, variable_name );
	} else {
		sprintf( command_buffer, "%s=", variable_name );
	}

	ptr = command_buffer + strlen( command_buffer );

	switch( variable_type ) {
	case MXFT_INT32:
		int32_ptr = (int32_t *) variable_ptr;

		sprintf( ptr, "%" PRId32, *int32_ptr );
		break;
	case MXFT_DOUBLE:
		double_ptr = (double *) variable_ptr;

		sprintf( ptr, "%f", *double_ptr );
		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Only MXFT_INT32 and MXFT_DOUBLE are supported." );
		break;
	}

	mx_status = mxi_pmac_command( pmac, command_buffer,
				response, sizeof response, debug_flag );

	return mx_status;
}

/*==================================================================*/

static mx_status_type
mxi_pmac_send_command( MX_PMAC *pmac,
			char *command,
			int debug_flag )
{
	static const char fname[] = "mxi_pmac_send_command()";

	mx_status_type mx_status;
#if MXI_PMAC_DEBUG_TIMING
	MX_HRT_RS232_TIMING command_timing;
#endif

	if ( pmac == (MX_PMAC *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_PMAC pointer passed was NULL." );
	}
	if ( command == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"command buffer pointer passed was NULL." );
	}

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: sent '%s' to '%s'",
			fname, command, pmac->record->name));
	}

#if MXI_PMAC_DEBUG_TIMING
	MX_HRT_RS232_START_COMMAND( command_timing, 1 + strlen(command) );
#endif

	switch( pmac->port_type ) {
	case MX_PMAC_PORT_TYPE_RS232:
		mx_status = mxi_pmac_rs232_send_command( pmac,
						command, debug_flag );
		break;
	default:
		mx_status = mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unknown PMAC interface port_type = %d.",
			pmac->port_type );
		break;
	}

#if MXI_PMAC_DEBUG_TIMING
	MX_HRT_RS232_END_COMMAND( command_timing );

	MX_HRT_RS232_COMMAND_RESULTS( command_timing, command, fname );
#endif

	return mx_status;
}

static mx_status_type
mxi_pmac_receive_response( MX_PMAC *pmac,
			char *response,
			int response_buffer_length,
			int debug_flag )
{
	static const char fname[] = "mxi_pmac_receive_response()";

	size_t length;
	mx_status_type mx_status;
#if MXI_PMAC_DEBUG_TIMING
	MX_HRT_RS232_TIMING response_timing;
#endif

	if ( pmac == (MX_PMAC *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_PMAC pointer passed was NULL." );
	}
	if ( response == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"response buffer pointer passed was NULL." );
	}

#if MXI_PMAC_DEBUG_TIMING
	MX_HRT_RS232_START_RESPONSE( response_timing, pmac->port_record );
#endif

	switch( pmac->port_type ) {
	case MX_PMAC_PORT_TYPE_RS232:
		mx_status = mxi_pmac_rs232_receive_response( pmac,
					response, response_buffer_length,
					debug_flag );
		break;
	default:
		mx_status = mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Unknown PMAC interface port_type = %d.",
			pmac->port_type );
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

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: received '%s' from '%s'",
			fname, response, pmac->record->name));
	}

#if MXI_PMAC_DEBUG_TIMING
	MX_HRT_RS232_END_RESPONSE( response_timing, length );

	MX_HRT_RS232_RESPONSE_RESULTS( response_timing, response, fname );
#endif

	return MX_SUCCESSFUL_RESULT;
}

/*----*/

static mx_status_type
mxi_pmac_rs232_send_command( MX_PMAC *pmac,
			char *command,
			int debug_flag )
{
	char c;
	mx_status_type mx_status;

	/* First, send a ctrl-Z to make the serial port the active interface. */

	c = MX_CTRL_Z;

	mx_status = mx_rs232_putchar( pmac->port_record, c, MXF_232_WAIT );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Now send the main string. */

	mx_status = mx_rs232_putline( pmac->port_record,
					command, NULL, debug_flag );
	return mx_status;
}

static mx_status_type
mxi_pmac_rs232_receive_response( MX_PMAC *pmac,
			char *response,
			int response_buffer_length,
			int debug_flag )
{
	static const char fname[] = "mxi_pmac_rs232_receive_response()";

	int i, bell_received;
	char c;
	mx_status_type mx_status;

	/* This function implicitly assumes I3 == 2.  This means that
	 * all PMAC responses will be terminated by either an <ACK>
	 * or a <BELL> character.  If I6 == 1, a <BELL> character
	 * will be followed by an error code terminated by a <CR>.
	 *
	 * Note that we must leave space at the end of the response
	 * buffer for the terminating null byte.
	 */

	bell_received = FALSE;

	for ( i = 0; i < (response_buffer_length - 1); i++ ) {

		response[i] = '\0';	/* Just in case we get an error. */

		mx_status = mx_rs232_getchar( pmac->port_record,
							&c, MXF_232_WAIT );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		response[i] = c;

		if ( c == MX_ACK ) {

			response[i] = '\0';
			break;			/* Exit the while() loop. */
		} else
		if ( c == MX_BELL ) {

			bell_received = TRUE;

			if ( pmac->i6_variable != 1 ) {

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

		mx_status = mx_rs232_discard_unread_input( pmac->port_record,
								debug_flag );

		if ( mx_status.code != MXE_SUCCESS ) {
			return mx_status;
		}

		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
	"The PMAC command response was longer than the buffer length of %d.",
			response_buffer_length );
	}

	return MX_SUCCESSFUL_RESULT;
}

#if HAVE_EPICS

static mx_status_type
mxi_pmac_epics_tc_command( MX_PMAC *pmac, char *command,
			char *response, int response_buffer_length,
			int debug_flag )
{
	static const char fname[] = "mxi_pmac_epics_tc_command()";

	mx_status_type mx_status;

	if ( pmac == (MX_PMAC *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_PMAC pointer passed was NULL." );
	}
	if ( command == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"command buffer pointer passed was NULL." );
	}

	/* Send the command. */

	mx_status = mx_caput( &(pmac->strcmd_pv), MX_CA_STRING, 1, command );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the response, if any. */

	if ( response != NULL ) {
		mx_status = mx_caget( &(pmac->strrsp_pv),
					MX_CA_STRING, 1, response );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}
	return MX_SUCCESSFUL_RESULT;
}

#endif

/*-------------------------------------------------------------------------*/

#ifndef MX_PROCESS_GET

#define MX_PROCESS_GET	1
#define MX_PROCESS_PUT	2

#endif

static mx_status_type
mxi_pmac_process_function( void *record_ptr,
			void *record_field_ptr, int operation )
{
	static const char fname[] = "mxi_pmac_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_PMAC *pmac;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	pmac = (MX_PMAC *) (record->record_type_struct);

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_PMAC_RESPONSE:
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
		case MXLV_PMAC_COMMAND:
			mx_status = mxi_pmac_command( pmac, pmac->command,
						NULL, 0, MXI_PMAC_DEBUG );

			break;
		case MXLV_PMAC_COMMAND_WITH_RESPONSE:
			mx_status = mxi_pmac_command( pmac, pmac->command,
				pmac->response, MX_PMAC_MAX_COMMAND_LENGTH,
				MXI_PMAC_DEBUG );

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
		break;
	}

	return mx_status;
}

