/*
 * Name:    i_pmc_mcapi.c
 *
 * Purpose: MX driver for Precision MicroControl MCAPI controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_PMC_MCAPI_DEBUG		FALSE

#define MXI_PMC_MCAPI_DEBUG_TIMING	FALSE

#include <stdio.h>

#include "mxconfig.h"

#if HAVE_PMC_MCAPI && defined(OS_WIN32)

#include "windows.h"

/* Vendor include files */

#ifndef _WIN32
#define _WIN32
#endif

#include "Mcapi.h"
#include "MCDlg.h"

/* MX include files */

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_rs232.h"
#include "i_pmc_mcapi.h"

MX_RECORD_FUNCTION_LIST mxi_pmc_mcapi_record_function_list = {
	NULL,
	mxi_pmc_mcapi_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_pmc_mcapi_open,
	mxi_pmc_mcapi_close,
	NULL,
	mxi_pmc_mcapi_resynchronize,
	mxi_pmc_mcapi_special_processing_setup
};

MX_RECORD_FIELD_DEFAULTS mxi_pmc_mcapi_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_PMC_MCAPI_STANDARD_FIELDS
};

mx_length_type mxi_pmc_mcapi_num_record_fields
		= sizeof( mxi_pmc_mcapi_record_field_defaults )
			/ sizeof( mxi_pmc_mcapi_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_pmc_mcapi_rfield_def_ptr
			= &mxi_pmc_mcapi_record_field_defaults[0];

static mx_status_type mxi_pmc_mcapi_process_function( void *record_ptr,
						void *record_field_ptr,
						int operation );

/* A private function for the use of the driver. */

static mx_status_type
mxi_pmc_mcapi_get_pointers( MX_RECORD *record,
			MX_PMC_MCAPI **pmc_mcapi,
			const char *calling_fname )
{
	static const char fname[] = "mxi_pmc_mcapi_get_pointers()";

	MX_PMC_MCAPI *pmc_mcapi_pointer;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed by '%s' was NULL.",
			calling_fname );
	}

	pmc_mcapi_pointer =
		(MX_PMC_MCAPI *) record->record_type_struct;

	if ( pmc_mcapi_pointer == (MX_PMC_MCAPI *) NULL )
	{
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_PMC_MCAPI pointer for record '%s' is NULL.",
			record->name );
	}

	if ( pmc_mcapi != (MX_PMC_MCAPI **) NULL ) {
		*pmc_mcapi = pmc_mcapi_pointer;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*==========================*/

MX_EXPORT mx_status_type
mxi_pmc_mcapi_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_pmc_mcapi_create_record_structures()";

	MX_PMC_MCAPI *pmc_mcapi;

	/* Allocate memory for the necessary structures. */

	pmc_mcapi = (MX_PMC_MCAPI *)
			malloc( sizeof(MX_PMC_MCAPI) );

	if ( pmc_mcapi == (MX_PMC_MCAPI *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Cannot allocate memory for an MX_PMC_MCAPI structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_type_struct = pmc_mcapi;

	record->record_class_struct = NULL;
	record->class_specific_function_list = NULL;

	pmc_mcapi->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_pmc_mcapi_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_pmc_mcapi_open()";

	MX_PMC_MCAPI *pmc_mcapi;
	short mcapi_status;
	char error_buffer[100];
	mx_status_type mx_status;

	mx_status = mxi_pmc_mcapi_get_pointers( record,
					&pmc_mcapi, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Initialize the MCDLG functions. */

	mcapi_status = MCDLG_Initialize();

	if ( mcapi_status != MCERR_NOERROR ) {
		mxi_pmc_mcapi_translate_error( mcapi_status,
				error_buffer, sizeof(error_buffer) );

		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Error initializing the MCDLG library for "
		"MCAPI controller %d of record '%s'.  "
		"MCAPI error message = '%s'",
			pmc_mcapi->controller_id,
			record->name, error_buffer );
	}

#if MXI_PMC_MCAPI_DEBUG
	MX_DEBUG(-2,("%s: Record '%s', MCDLG_Initialize() succeeded.",
		fname, record->name));
#endif

	/* Open a connection to the controller. */

#if MXI_PMC_MCAPI_DEBUG
	MX_DEBUG(-2,("%s: Record '%s', controller_id = %d",
		fname, record->name, pmc_mcapi->controller_id));
#endif

	pmc_mcapi->controller_handle =
		MCOpen( pmc_mcapi->controller_id, MC_OPEN_BINARY, NULL );

#if MXI_PMC_MCAPI_DEBUG
	MX_DEBUG(-2,("%s: Record '%s', pmc_mcapi->controller_handle = %hd",
		fname, record->name, pmc_mcapi->controller_handle));
#endif

	if ( pmc_mcapi->controller_handle <= 0 ) {

		mxi_pmc_mcapi_translate_error(
				(long) -(pmc_mcapi->controller_handle),
				error_buffer, sizeof(error_buffer) );

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Error opening a connection to MCAPI controller number %d "
		"of record '%s'.  MCAPI error message = '%s'",
			pmc_mcapi->controller_id, record->name, error_buffer );
	}

	/* Restore the controller configuration. */

#if MXI_PMC_MCAPI_DEBUG
	MX_DEBUG(-2,
	("%s: Record '%s', restoring configuration from savefile '%s'.",
		fname, record->name, pmc_mcapi->savefile_name));
#endif

	mcapi_status = MCDLG_RestoreAxis( pmc_mcapi->controller_handle,
						MC_ALL_AXES,
						MCDLG_CHECKACTIVE,
						pmc_mcapi->savefile_name );

	if ( mcapi_status != MCERR_NOERROR ) {
		mxi_pmc_mcapi_translate_error( mcapi_status,
				error_buffer, sizeof(error_buffer) );

		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Error restoring the motor configuration for "
		"MCAPI controller %d of record '%s'.  "
		"MCAPI error message = '%s'",
			pmc_mcapi->controller_id,
			record->name, error_buffer );
	}

#if MXI_PMC_MCAPI_DEBUG
	MX_DEBUG(-2,("%s: Record '%s', configuration of all axes restored.",
		fname, record->name));
#endif

	/* Download the startup file. */

	if ( strlen( pmc_mcapi->startup_file_name ) > 0 ) {
		mx_status = mxi_pmc_mcapi_download_file( pmc_mcapi,
						pmc_mcapi->startup_file_name );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if MXI_PMC_MCAPI_DEBUG
		MX_DEBUG(-2,("%s: Startup file '%s' downloaded for record '%s'",
			fname, pmc_mcapi->startup_file_name, record->name));
#endif
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_pmc_mcapi_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_pmc_mcapi_close()";

	MX_PMC_MCAPI *pmc_mcapi;
	HCTRLR handle;
	short mcapi_status;
	char error_buffer[100];
	mx_status_type mx_status;

	mx_status = mxi_pmc_mcapi_get_pointers( record,
					&pmc_mcapi, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Close the connection to the controller. */

#if MXI_PMC_MCAPI_DEBUG
	MX_DEBUG(-2,("%s: Record '%s', closing controller_id = %d",
		fname, record->name, pmc_mcapi->controller_id));
#endif

	mcapi_status = MCClose( pmc_mcapi->controller_handle );

	if ( mcapi_status != MCERR_NOERROR ) {
		mxi_pmc_mcapi_translate_error( (long) mcapi_status,
				error_buffer, sizeof(error_buffer) );

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Error closing the connection to MCAPI controller number %d "
		"of record '%s'.  MCAPI error message = '%s'",
			pmc_mcapi->controller_id, record->name, error_buffer );
	}

#if MXI_PMC_MCAPI_DEBUG
	MX_DEBUG(-2,("%s: Record '%s' closed.", fname, record->name));
#endif

	pmc_mcapi->controller_handle = -1;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_pmc_mcapi_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxi_pmc_mcapi_resynchronize()";

	MX_PMC_MCAPI *pmc_mcapi;
	mx_status_type mx_status;

	mx_status = mxi_pmc_mcapi_get_pointers( record,
					&pmc_mcapi, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Reset the connection to the controller. */

	MCReset( pmc_mcapi->controller_handle, MC_ALL_AXES );

#if MXI_PMC_MCAPI_DEBUG
	MX_DEBUG(-2,
	("%s: Record '%s', reset sent to controller id = %d, handle = %d",
		fname, record->name, pmc_mcapi->controller_id,
		pmc_mcapi->controller_handle));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_pmc_mcapi_special_processing_setup( MX_RECORD *record )
{
	static const char fname[] = "mxi_pmc_mcapi_special_processing_setup()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

#if MXI_PMC_MCAPI_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name));
#endif

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_PMC_MCAPI_COMMAND:
		case MXLV_PMC_MCAPI_RESPONSE:
		case MXLV_PMC_MCAPI_COMMAND_WITH_RESPONSE:
		case MXLV_PMC_MCAPI_DOWNLOAD_FILE:
			record_field->process_function
					    = mxi_pmc_mcapi_process_function;
			break;
		default:
			break;
		}
	}
	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

#ifndef MX_PROCESS_GET

#define MX_PROCESS_GET	1
#define MX_PROCESS_PUT	2

#endif

static mx_status_type
mxi_pmc_mcapi_process_function( void *record_ptr,
			void *record_field_ptr, int operation )
{
	static const char fname[] = "mxi_pmc_mcapi_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_PMC_MCAPI *pmc_mcapi;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	pmc_mcapi = (MX_PMC_MCAPI *)
					record->record_type_struct;

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_PMC_MCAPI_RESPONSE:
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
		case MXLV_PMC_MCAPI_COMMAND:
			mx_status = mxi_pmc_mcapi_command(
				pmc_mcapi,
				pmc_mcapi->command,
				NULL, 0, MXI_PMC_MCAPI_DEBUG );

			break;
		case MXLV_PMC_MCAPI_COMMAND_WITH_RESPONSE:
			mx_status = mxi_pmc_mcapi_command(
				pmc_mcapi,
				pmc_mcapi->command,
				pmc_mcapi->response,
				MXU_PMC_MCAPI_MAX_COMMAND_LENGTH,
				MXI_PMC_MCAPI_DEBUG );

			break;
		case MXLV_PMC_MCAPI_DOWNLOAD_FILE:
			mx_status = mxi_pmc_mcapi_download_file(
				pmc_mcapi,
				pmc_mcapi->download_file );

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

/* === Functions specific to this driver. === */

#if MXI_PMC_MCAPI_DEBUG_TIMING	
#include "mx_hrt_debug.h"
#endif

static mx_status_type
mxi_pmc_mcapi_handle_errors(
		MX_PMC_MCAPI *pmc_mcapi,
		char *response_buffer )
{
	static const char fname[] = "mxi_pmc_mcapi_handle_errors()";

	MX_DEBUG(-2,("%s: Warning or error response = '%s'",
		fname, response_buffer ));

	return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Warning or error in response '%s' for Picomotor "
		"controller '%s'.", response_buffer,
		pmc_mcapi->record->name );
}

MX_EXPORT mx_status_type
mxi_pmc_mcapi_command( MX_PMC_MCAPI *pmc_mcapi,
		char *command,
		char *response, size_t max_response_length,
		int debug_flag )
{
	static const char fname[] = "mxi_pmc_mcapi_command()";

	char command_buffer[MXU_PMC_MCAPI_MAX_COMMAND_LENGTH+1];
	char response_buffer[MXU_PMC_MCAPI_MAX_COMMAND_LENGTH+1];
	char error_buffer[100];
	size_t command_length;
	short num_chars_written, num_chars_read;
	mx_status_type mx_status, mx_status2;

#if MXI_PMC_MCAPI_DEBUG_TIMING	
	MX_HRT_RS232_TIMING command_timing, response_timing;
#endif

	MX_DEBUG(-2,("%s invoked.", fname));

	if ( pmc_mcapi == (MX_PMC_MCAPI *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PMC_MCAPI pointer passed was NULL." );
	}

	if ( command == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"'command' buffer pointer passed was NULL.  No command sent.");
	}

	command_length = strlen(command);

	if ( command_length >= MXU_PMC_MCAPI_MAX_COMMAND_LENGTH ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The length (%d) of the command to be sent to PMC MCAPI "
		"controller '%s' is longer than the maximum allowed length "
		"of %d.  Command = '%s'",
			(int) command_length, pmc_mcapi->record->name,
			MXU_PMC_MCAPI_MAX_COMMAND_LENGTH, command );
	}

	/* Copy the command to a local command buffer and terminate it with
	 * a carriage return character.
	 */

	sprintf( command_buffer, "%s\r", command );

	/* The do...while(0) block is used to make it easy to skip to
	 * the end of the block.
	 */

	mx_status = MX_SUCCESSFUL_RESULT;

	do {
		/* Switch to ASCII mode for sending the command. */

		pmc_mcapi->controller_handle =
		    MCOpen( pmc_mcapi->controller_id, MC_OPEN_ASCII, NULL );

		MX_DEBUG(-2,("%s: pmc_mcapi->controller_handle = %ld",
			fname, (long) pmc_mcapi->controller_handle));

		if ( pmc_mcapi->controller_handle <= 0 ) {
			mxi_pmc_mcapi_translate_error(
				(long) -(pmc_mcapi->controller_handle),
				error_buffer, sizeof(error_buffer) );

			mx_status = mx_error( MXE_INTERFACE_IO_ERROR, fname,
				"Error switching the connection to ASCII mode "
				"for MCAPI controller number %d of record '%s'."
				"  MCAPI error message = '%s'",
				pmc_mcapi->controller_id,
				pmc_mcapi->record->name, error_buffer );

			break;	/* Exit the do...while(0) loop. */
		}

		/* Send the command. */

		if ( debug_flag ) {
			MX_DEBUG(-2,("%s: sending '%s' to '%s'",
				fname, command, pmc_mcapi->record->name));
		}

#if MXI_PMC_MCAPI_DEBUG_TIMING	
		MX_HRT_RS232_START_COMMAND( command_timing,
						2 + strlen(command) );
#endif

		num_chars_written = pmcputs( pmc_mcapi->controller_handle,
						command_buffer );

#if MXI_PMC_MCAPI_DEBUG_TIMING
		MX_HRT_RS232_END_COMMAND( command_timing );
#endif

		if ( num_chars_written < command_length + 1 ) {
			mx_status = mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Fewer characters (%hd) were written to PMC MCAPI "
			"controller '%s' for command '%s' than were "
			"supposed to be written (%d).",
				num_chars_written, pmc_mcapi->record->name,
				command, command_length+1 );

			break;	/* Exit the do...while(0) loop. */
		}

#if MXI_PMC_MCAPI_DEBUG_TIMING
		MX_HRT_RS232_COMMAND_RESULTS( command_timing, command, fname );
#endif

		/* Get the response. */

#if MXI_PMC_MCAPI_DEBUG_TIMING
		MX_HRT_RS232_START_RESPONSE( response_timing, NULL );
#endif

		num_chars_read = pmcgets( pmc_mcapi->controller_handle,
				response_buffer, sizeof(response_buffer) );

#if MXI_PMC_MCAPI_DEBUG_TIMING
		MX_HRT_RS232_END_RESPONSE( response_timing, strlen(response) );

		MX_HRT_TIME_BETWEEN_MEASUREMENTS( command_timing,
						response_timing, fname );

		MX_HRT_RS232_RESPONSE_RESULTS( response_timing,
						response, fname );
#endif

	} while(0);	/* End of the do...while(0) loop. */

	/* Switch back to binary mode. */

	pmc_mcapi->controller_handle =
		    MCOpen( pmc_mcapi->controller_id, MC_OPEN_BINARY, NULL );

	MX_DEBUG(-2,("%s: pmc_mcapi->controller_handle = %ld",
			fname, (long) pmc_mcapi->controller_handle));

	if ( pmc_mcapi->controller_handle <= 0 ) {
		mxi_pmc_mcapi_translate_error(
			(long) -(pmc_mcapi->controller_handle),
			error_buffer, sizeof(error_buffer) );

		mx_status2 = mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Error switching the connection back to binary mode "
			"for MCAPI controller number %d of record '%s'."
			"  MCAPI error message = '%s'",
			pmc_mcapi->controller_id,
			pmc_mcapi->record->name, error_buffer );
	}

	/* If the caller wanted a response, extract it from the
	 * response buffer.
	 */

	if ( response != (char *) NULL ) {

		strlcpy( response, response_buffer, max_response_length );

		if ( debug_flag ) {
			MX_DEBUG(-2,("%s: received '%s' from '%s'",
					fname, response,
					pmc_mcapi->record->name));
		}
	}

	MX_DEBUG(-2,("%s complete.", fname));

	if ( mx_status2.code != MXE_SUCCESS ) {
		return mx_status2;
	} else {
		return mx_status;
	}
}

MX_EXPORT mx_status_type
mxi_pmc_mcapi_download_file( MX_PMC_MCAPI *pmc_mcapi, char *filename )
{
	static const char fname[] = "mxi_pmc_mcapi_download_file()";

	long download_status;
	char error_buffer[100];

	if ( pmc_mcapi == (MX_PMC_MCAPI *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PMC_MCAPI pointer passed was NULL." );
	}

	download_status = MCDLG_DownloadFile( NULL,
				pmc_mcapi->controller_handle, 0, filename );

	if ( download_status != MCERR_NOERROR ) {
		mxi_pmc_mcapi_translate_error( download_status,
				error_buffer, sizeof(error_buffer) );

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"An error occurred while download file '%s' to "
		"PMC MCAPI controller '%s'.  MCAPI error message = '%s'",
			filename, pmc_mcapi->record->name, error_buffer );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT void
mxi_pmc_mcapi_translate_error( long mcapi_error_code,
				char *buffer,
				long buffer_length )
{
	static const char fname[] = "mxi_pmc_mcapi_translate_error()";

	long i, translate_status;

	translate_status = MCTranslateErrorEx( (short) mcapi_error_code,
					buffer, buffer_length );

	if ( translate_status != MCERR_NOERROR ) {
		if ( buffer_length >= 80 ) {
			sprintf( buffer,
			"Translation of MCAPI error code %hd failed.  "
			"New MCAPI error code was %ld",
				mcapi_error_code, translate_status );
		} else if ( buffer_length >= 40 ) {
			strcpy( buffer, "MCAPI error translation failed." );
		} else {
			for ( i = 0; i < (buffer_length - 1); i++ ) {
				buffer[i] = '*';
			}
			buffer[ buffer_length - 1 ] = '\0';
		}
	}
}

#endif /* HAVE_PMC_MCAPI */
