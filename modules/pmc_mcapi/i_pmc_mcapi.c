/*
 * Name:    i_pmc_mcapi.c
 *
 * Purpose: MX driver for Precision MicroControl MCAPI controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004-2005, 2008, 2010, 2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_PMC_MCAPI_DEBUG		TRUE

#define MXI_PMC_MCAPI_DEBUG_TIMING	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#if defined(OS_WIN32)
  #include "windows.h"

  #ifndef _WIN32
    #define _WIN32
  #endif
#endif

#include "mcapi.h"

/* MX include files */

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_rs232.h"
#include "i_pmc_mcapi.h"

#define MXU_PMC_MCAPI_BUFFER_LENGTH	200

MX_RECORD_FUNCTION_LIST mxi_pmc_mcapi_record_function_list = {
	NULL,
	mxi_pmc_mcapi_create_record_structures,
	NULL,
	NULL,
	mxi_pmc_mcapi_print_structure,
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

long mxi_pmc_mcapi_num_record_fields
		= sizeof( mxi_pmc_mcapi_record_field_defaults )
			/ sizeof( mxi_pmc_mcapi_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_pmc_mcapi_rfield_def_ptr
			= &mxi_pmc_mcapi_record_field_defaults[0];

static mx_status_type
mxi_pmc_mcapi_initialize_dialog_functions( MX_PMC_MCAPI *pmc_mcapi );

static mx_status_type
mxi_pmc_mcapi_save_axes( MX_PMC_MCAPI *pmc_mcapi, char *filename );

static mx_status_type
mxi_pmc_mcapi_restore_axes( MX_PMC_MCAPI *pmc_mcapi, char *filename );

static mx_status_type
mxi_pmc_mcapi_download_file( MX_PMC_MCAPI *pmc_mcapi, char *filename );

static mx_status_type
mxi_pmc_mcapi_process_function( void *record_ptr,
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

/*----*/

MX_EXPORT char *
mxi_pmc_mcapi_controller_type_name( long controller_type )
{
	static struct {
		long type;
		char name[20];
	} controller_type_table[] = {
		{ NO_CONTROLLER,"None" },
		{ DCXPC100,	"DCX-PC100" },
		{ DCXAT100,	"DCX-AT100" },
		{ DCXAT200,	"DCX-AT200" },
		{ DC2PC100,	"DC2-PC100" },
		{ DC2STN,	"DC2-STN" },
		{ DCXAT300,	"DCX-AT300" },
		{ DCXPCI300,	"DCX-PCI300" },
		{ DCXPCI100,	"DCX-PCI100" },
		{ MFXPCI1000,	"MFX-PCI1000" },
		{ MFXETH1000,	"MFX-ETH1000" },
	};

	static int num_controller_types = sizeof(controller_type_table)
					/ sizeof(controller_type_table[0]);

	static char unknown_name[40];

	char *controller_name = NULL;
	int n;

	for ( n = 0; n < num_controller_types; n++ ) {
		if ( controller_type == controller_type_table[n].type ) {

			controller_name = controller_type_table[n].name;
			break;
		}
	}

	if ( n >= num_controller_types ) {
		snprintf( unknown_name, sizeof(unknown_name),
			"Unknown (%ld)", controller_type );

		controller_name = unknown_name;
	}

	return controller_name;
}

/*----*/

MX_EXPORT char *
mxi_pmc_mcapi_module_type_name( long module_type )
{
	static struct {
		long type;
		char name[20];
	} module_type_table[] = {
		{ NO_MODULE,	"None" },
		{ MC100,	"MC100" },
		{ MC110,	"MC110" },
		{ MC150,	"MC150" },
		{ MC160,	"MC160" },
		{ MC200,	"MC200" },
		{ MC210,	"MC210" },
		{ MC260,	"MC260" },
		{ MC300,	"MC300" },
		{ MC302,	"MC302" },
		{ MC320,	"MC320" },
		{ MC360,	"MC360" },
		{ MC362,	"MC362" },
		{ MC400,	"MC400" },
		{ MC500,	"MC500" },
		{ MF300,	"MF300" },
		{ MF310,	"MF310" },
		{ MFXSERVO,	"MFXSERVO" },
		{ MFXSTEPPER,	"MFXSTEPPER" },
		{ DC2SERVO,	"DC2SERVO" },
		{ DC2STEPPER,	"DC2STEPPER" },
	};

	static int num_module_types = sizeof(module_type_table)
				/ sizeof(module_type_table[0]);

	static char unknown_name[40];

	char *module_name = NULL;
	int n;

	for ( n = 0; n < num_module_types; n++ ) {
		if ( module_type == module_type_table[n].type ) {

			module_name = module_type_table[n].name;
			break;
		}
	}

	if ( n >= num_module_types ) {
		snprintf( unknown_name, sizeof(unknown_name),
			"Unknown (%ld)", module_type );

		module_name = unknown_name;
	}

	return module_name;
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

	pmc_mcapi->binary_handle = -1;
	pmc_mcapi->ascii_handle  = -1;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_pmc_mcapi_print_structure( FILE *file, MX_RECORD *record )
  {
	static const char fname[] = "mxi_pmc_mcapi_print_structure()";

	MCPARAMEX *cc;
	MCAXISCONFIG axis;
	long i, mcapi_status;

	MX_PMC_MCAPI *pmc_mcapi;
	char error_buffer[100];
	mx_status_type mx_status;

	mx_status = mxi_pmc_mcapi_get_pointers( record,
					&pmc_mcapi, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	cc = &(pmc_mcapi->configuration);

	fprintf( file, "Parameters for PMC MCAPI controller '%s'.\n\n",
		pmc_mcapi->record->name );

	fprintf( file, " MX parameters:\n" );

	fprintf( file, "  controller_id     = %d\n",
					pmc_mcapi->controller_id );
	fprintf( file, "  restore_file_name = '%s'\n",
					pmc_mcapi->restore_file_name );
	fprintf( file, "  startup_file_name = '%s'\n\n",
					pmc_mcapi->startup_file_name );

	fprintf( file, " MCAPI parameters:\n" );

	fprintf( file, "  ControllerType    = '%s'\n", 
		mxi_pmc_mcapi_controller_type_name( cc->ControllerType ) );

	fprintf( file, "  NumberAxes        = %d\n", cc->NumberAxes );

	fprintf( file, "  MaximumAxes       = %d\n", cc->MaximumAxes );

	fprintf( file, "  Precision         = %d\n", cc->Precision );

	fprintf( file, "  DigitalIO         = %d\n", cc->DigitalIO );

	fprintf( file, "  AnalogInput       = %d\n", cc->AnalogInput );

	fprintf( file, "  AnalogOutput      = %d\n", cc->AnalogOutput );

	fprintf( file, "  PointStorage      = %d\n", cc->PointStorage );

	if ( cc->CanDoScaling ) {
		fprintf( file, "  CanDoScaling      is supported\n" );
	} else {
		fprintf( file, "  CanDoScaling      is not supported\n" );
	}

	if ( cc->CanDoContouring ) {
		fprintf( file, "  CanDoContouring   is supported\n" );
	} else {
		fprintf( file, "  CanDoContouring   is not supported\n" );
	}

	if ( cc->CanChangeProfile ) {
		fprintf( file, "  CanChangeProfile  is supported\n" );
	} else {
		fprintf( file, "  CanChangeProfile  is not supported\n" );
	}

	if ( cc->CanChangeRates ) {
		fprintf( file, "  CanChangeRates    is supported\n" );
	} else {
		fprintf( file, "  CanChangeRates    is not supported\n" );
	}

	if ( cc->SoftLimits ) {
		fprintf( file, "  SoftLimits        are supported\n" );
	} else {
		fprintf( file, "  SoftLimits        are not supported\n" );
	}

	if ( cc->MultiTasking ) {
		fprintf( file, "  MultiTasking      is supported\n" );
	} else {
		fprintf( file, "  MultiTasking      is not supported\n" );
	}

	if ( cc->AmpFault ) {
		fprintf( file, "  AmpFault input    is supported\n" );
	} else {
		fprintf( file, "  AmpFault input    is not supported\n" );
	}

	fprintf( file, "\n" );
	fprintf( file, " Axis    Type\n" );

	for ( i = 1; i <= cc->MaximumAxes; i++ ) {

		axis.cbSize = sizeof(MCAXISCONFIG);

		mcapi_status = MCGetAxisConfiguration( pmc_mcapi->binary_handle,
							i, &axis );

		if ( mcapi_status != MCERR_NOERROR ) {
			mxi_pmc_mcapi_translate_error( mcapi_status,
				error_buffer, sizeof(error_buffer) );

			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Unable to get information about axis %ld for "
			"MCAPI controller '%s'.  MCAPI error message = '%s'",
				i, pmc_mcapi->record->name, error_buffer );
		}

		fprintf( file, "  %2ld    '%s'\n",
			i, mxi_pmc_mcapi_module_type_name( axis.ModuleType ) );
	}

	fprintf( file, "\n" );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_pmc_mcapi_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_pmc_mcapi_open()";

	MX_PMC_MCAPI *pmc_mcapi = NULL;
	char error_buffer[100];
	mx_status_type mx_status;

	long mcapi_status;

	mx_status = mxi_pmc_mcapi_get_pointers( record,
					&pmc_mcapi, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXI_PMC_MCAPI_DEBUG
	MX_DEBUG(-2,("%s: Record '%s', controller_id = %d",
		fname, record->name, pmc_mcapi->controller_id));
#endif

	/* Initialize the dialog functions. */

	mx_status = mxi_pmc_mcapi_initialize_dialog_functions( pmc_mcapi );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Open a binary handle for the controller. */

	pmc_mcapi->binary_handle =
		MCOpen( pmc_mcapi->controller_id, MC_OPEN_BINARY, NULL );

#if MXI_PMC_MCAPI_DEBUG
	MX_DEBUG(-2,("%s: Record '%s', pmc_mcapi->binary_handle = %hd",
		fname, record->name, pmc_mcapi->binary_handle));
#endif

	if ( pmc_mcapi->binary_handle <= 0 ) {

		mxi_pmc_mcapi_translate_error(
				(long) -(pmc_mcapi->binary_handle),
				error_buffer, sizeof(error_buffer) );

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Error opening a binary connection to MCAPI controller "
		"number %d of record '%s'.  MCAPI error message = '%s'",
			pmc_mcapi->controller_id, record->name, error_buffer );
	}

	/* Get and save the controller configuration for later. */

	pmc_mcapi->configuration.cbSize = sizeof(MCPARAMEX);

	mcapi_status = MCGetConfigurationEx( pmc_mcapi->binary_handle,
						&(pmc_mcapi->configuration) );

	if ( mcapi_status != MCERR_NOERROR ) {
		mxi_pmc_mcapi_translate_error( mcapi_status,
			error_buffer, sizeof(error_buffer) );

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Attempting to read the controller configuration for "
		"MCAPI controller '%s' failed.  "
		"MCAPI error message = '%s'",
			pmc_mcapi->record->name, error_buffer );
	}

	/* Initialize an array to contain pointers to motor records
	 * belonging to this controller.
	 */

	pmc_mcapi->num_axes = pmc_mcapi->configuration.MaximumAxes;

	pmc_mcapi->axis_array = calloc( pmc_mcapi->num_axes,
					sizeof(MX_RECORD *) );

	if ( pmc_mcapi->axis_array == (MX_RECORD **) NULL ) {
		pmc_mcapi->num_axes = 0;

		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Ran out of memory trying to allocate a %lu element array "
		"of MX_RECORD pointers for MCAPI controller '%s'.",
			pmc_mcapi->num_axes, pmc_mcapi->record->name );
	}

	/* Open an ASCII handle for the controller.  This handle
	 * is used for sending MCCL commands to the controller.
	 */

	pmc_mcapi->ascii_handle =
		MCOpen( pmc_mcapi->controller_id, MC_OPEN_ASCII, NULL );

#if MXI_PMC_MCAPI_DEBUG
	MX_DEBUG(-2,("%s: Record '%s', pmc_mcapi->ascii_handle = %hd",
		fname, record->name, pmc_mcapi->ascii_handle));
#endif

	if ( pmc_mcapi->ascii_handle <= 0 ) {

		mxi_pmc_mcapi_translate_error(
				(long) -(pmc_mcapi->ascii_handle),
				error_buffer, sizeof(error_buffer) );

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Error opening an ASCII connection to MCAPI controller "
		"number %d of record '%s'.  MCAPI error message = '%s'",
			pmc_mcapi->controller_id, record->name, error_buffer );
	}

	/* Restore the controller configuration. */

	if ( strlen( pmc_mcapi->restore_file_name ) > 0 ) {

#if MXI_PMC_MCAPI_DEBUG
		MX_DEBUG(-2,
	("%s: Record '%s', restoring configuration from restore file '%s'.",
			fname, record->name, pmc_mcapi->restore_file_name));
#endif

		mx_status = mxi_pmc_mcapi_restore_axes( pmc_mcapi,
						pmc_mcapi->restore_file_name );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if MXI_PMC_MCAPI_DEBUG
		MX_DEBUG(-2,
		("%s: Record '%s', configuration of all axes restored.",
			fname, record->name));
#endif
	}

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

	MX_PMC_MCAPI *pmc_mcapi = NULL;
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

	mcapi_status = MCClose( pmc_mcapi->binary_handle );

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

	pmc_mcapi->binary_handle = -1;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_pmc_mcapi_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxi_pmc_mcapi_resynchronize()";

	MX_PMC_MCAPI *pmc_mcapi = NULL;
	mx_status_type mx_status;

	mx_status = mxi_pmc_mcapi_get_pointers( record,
					&pmc_mcapi, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Reset the connection to the controller. */

	MCReset( pmc_mcapi->binary_handle, MC_ALL_AXES );

#if MXI_PMC_MCAPI_DEBUG
	MX_DEBUG(-2,
	("%s: Record '%s', reset sent to controller id = %d, handle = %d",
		fname, record->name, pmc_mcapi->controller_id,
		pmc_mcapi->binary_handle));
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
		case MXLV_PMC_MCCL_COMMAND:
		case MXLV_PMC_MCCL_RESPONSE:
		case MXLV_PMC_MCCL_COMMAND_WITH_RESPONSE:
		case MXLV_PMC_MCAPI_SAVE_FILE_NAME:
		case MXLV_PMC_MCAPI_DOWNLOAD_FILE_NAME:
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
		case MXLV_PMC_MCCL_RESPONSE:
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
		case MXLV_PMC_MCCL_COMMAND:
			mx_status = mxi_pmc_mccl_command(
				pmc_mcapi,
				pmc_mcapi->command,
				NULL, 0, MXI_PMC_MCAPI_DEBUG );

			break;
		case MXLV_PMC_MCCL_COMMAND_WITH_RESPONSE:
			mx_status = mxi_pmc_mccl_command(
				pmc_mcapi,
				pmc_mcapi->command,
				pmc_mcapi->response,
				MXU_PMC_MCAPI_MAX_COMMAND_LENGTH,
				MXI_PMC_MCAPI_DEBUG );

			break;
		case MXLV_PMC_MCAPI_SAVE_FILE_NAME:
			mx_status = mxi_pmc_mcapi_save_axes(
				pmc_mcapi,
				pmc_mcapi->save_file_name );

			break;
		case MXLV_PMC_MCAPI_DOWNLOAD_FILE_NAME:
			mx_status = mxi_pmc_mcapi_download_file(
				pmc_mcapi,
				pmc_mcapi->download_file_name );

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

static struct {
	long code;
	char message[40];
} command_error_table[] = {
	{ 0,	"No error" },
	{ 1,	"Unrecognized command" },
	{ 2,	"Bad command format" },
	{ 3,	"I/O error" },
	{ 4,	"Command string to long" },
	{ -1,	"Command parameter error" },
	{ -2,	"Command code invalid" },
	{ -3,	"Negative repeat count" },
	{ -4,	"Macro define command not first" },
	{ -5,	"Macro number out of range" },
	{ -6,	"Macro does not exist" },
	{ -7,	"Command canceled by user" },
	{ -14,	"No axis specified" },
	{ -15,	"Axis not assigned" },
	{ -16,	"Axis already assigned" },
	{ -17,	"Axis duplicate assigned" },
};

static int num_command_errors = sizeof(command_error_table)
				/ sizeof(command_error_table[0]);

static mx_status_type
mxi_pmc_mccl_command_error( MX_PMC_MCAPI *pmc_mcapi,
				char *command,
				char *response )
{
	static const char fname[] = "mxi_pmc_mccl_command_error()";

	long i, error_code;
	char *error_message;

#if MXI_PMC_MCAPI_DEBUG
	MX_DEBUG(-2,("%s: response = '%s'", fname, response));
#endif

	response++;

	error_code = mx_string_to_long( response );

	for ( i = 0; i < num_command_errors; i++ ) {
		if ( error_code == command_error_table[i].code ) {
			error_message = command_error_table[i].message;
			break;
		}
	}

	if ( i >= num_command_errors ) {
		error_message = "Unknown error";
	}

	return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
	"Received error '%s' (%ld) for command '%s' to MCAPI controller '%s'.",
		error_message, error_code, command, pmc_mcapi->record->name );
}

MX_EXPORT mx_status_type
mxi_pmc_mccl_command( MX_PMC_MCAPI *pmc_mcapi,
		char *command,
		char *response, size_t max_response_length,
		int debug_flag )
{
	static const char fname[] = "mxi_pmc_mccl_command()";

	char command_buffer[MXU_PMC_MCAPI_MAX_COMMAND_LENGTH+1];
	char response_buffer[MXU_PMC_MCAPI_MAX_COMMAND_LENGTH+1];
	size_t command_length, response_length;
	short num_chars_written, num_chars_read;
	char *response_ptr;
	mx_status_type mx_status;

#if MXI_PMC_MCAPI_DEBUG_TIMING	
	MX_HRT_RS232_TIMING command_timing, response_timing;
#endif

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
		/* See if ASCII mode is available. */

		if ( pmc_mcapi->ascii_handle < 0 ) {
			return mx_error( MXE_NOT_VALID_FOR_CURRENT_STATE, fname,
			"The ASCII MCCL interface is not available "
			"for MCAPI controller '%s'.",
				pmc_mcapi->record->name );
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

		num_chars_written = pmcputs( pmc_mcapi->ascii_handle,
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

		num_chars_read = pmcgets( pmc_mcapi->ascii_handle,
				response_buffer, sizeof(response_buffer) );

#if MXI_PMC_MCAPI_DEBUG_TIMING
		MX_HRT_RS232_END_RESPONSE( response_timing, strlen(response) );

		MX_HRT_TIME_BETWEEN_MEASUREMENTS( command_timing,
						response_timing, fname );

		MX_HRT_RS232_RESPONSE_RESULTS( response_timing,
						response, fname );
#endif

	} while(0);	/* End of the do...while(0) loop. */

	/* Sometimes we get an extraneous > character at the start
	 * of the response.
	 */

	response_ptr = response_buffer;

	if ( *response_ptr == '>' ) {
		response_ptr++;
	}

	/* Eliminate trailing CR LF characters if present. */

	response_length = strlen( response_ptr );

	if ( response_ptr[response_length-1] == '\n' ) {
		response_ptr[response_length-1] = '\0';
		if ( response_ptr[response_length-2] == '\r' ) {
			response_ptr[response_length-2] = '\0';
		}
	}

	/* Check to see if we got an error code back from the controller. */

	if ( *response_ptr == '?' ) {
		return mxi_pmc_mccl_command_error( pmc_mcapi,
						command, response_ptr );
	}

	/* If the caller wanted a response, copy it from the response buffer. */

	if ( response != (char *) NULL ) {

		/* Copy the response. */

		strlcpy( response, response_ptr, max_response_length );

		if ( debug_flag ) {
			MX_DEBUG(-2,("%s: received '%s' from '%s'",
					fname, response,
					pmc_mcapi->record->name));
		}
	}

	return mx_status;
}

MX_EXPORT void
mxi_pmc_mcapi_translate_error( long mcapi_error_code,
				char *buffer,
				long buffer_length )
{
	long i, translate_status;

	translate_status = MCTranslateErrorEx( (short) mcapi_error_code,
					buffer, buffer_length );

	if ( translate_status != MCERR_NOERROR ) {
		if ( buffer_length >= 80 ) {
			sprintf( buffer,
			"Translation of MCAPI error code %ld failed.  "
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

/****************** Windows ******************/

#if defined(OS_WIN32)

/* On Windows, we can use the MCDLG_ functions. */

#include "MCDlg.h"

static mx_status_type
mxi_pmc_mcapi_initialize_dialog_functions( MX_PMC_MCAPI *pmc_mcapi )
{
	static const char fname[] =
		"mxi_pmc_mcapi_initialize_dialog_functions()";

	short mcdlg_status;
	char error_buffer[100];
	
	if ( pmc_mcapi == (MX_PMC_MCAPI *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PMC_MCAPI pointer passed was NULL." );
	}

	/* Initialize the MCDLG functions. */

	mcdlg_status = MCDLG_Initialize();

	if ( mcdlg_status != MCERR_NOERROR ) {
		mxi_pmc_mcapi_translate_error( mcdlg_status,
				error_buffer, sizeof(error_buffer) );

		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Error initializing the MCDLG library for "
		"MCAPI controller %d of record '%s'.  "
		"MCAPI error message = '%s'",
			pmc_mcapi->controller_id,
			pmc_mcapi->record->name, error_buffer );
	}

#if MXI_PMC_MCAPI_DEBUG
	MX_DEBUG(-2,("%s: Record '%s', MCDLG_Initialize() succeeded.",
		fname, record->name));
#endif

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxi_pmc_mcapi_save_axes( MX_PMC_MCAPI *pmc_mcapi, char *filename )
{
	static const char fname[] = "mxi_pmc_mcapi_save_axes()";

	short mcdlg_status;
	char error_buffer[100];

	if ( pmc_mcapi == (MX_PMC_MCAPI *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PMC_MCAPI pointer passed was NULL." );
	}

	if ( filename == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The filename pointer passed was NULL." );
	}

	mcdlg_status = MCDLG_SaveAxis( pmc_mcapi->controller_handle,
						MC_ALL_AXES,
						MCDLG_CHECKACTIVE,
						filename );

	if ( mcdlg_status != MCERR_NOERROR ) {
		mxi_pmc_mcapi_translate_error( mcdlg_status,
				error_buffer, sizeof(error_buffer) );

		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Error restoring the motor configuration for "
		"MCAPI controller %d of record '%s'.  "
		"MCAPI error message = '%s'",
			pmc_mcapi->controller_id,
			pmc_mcapi->record->name, error_buffer );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxi_pmc_mcapi_restore_axes( MX_PMC_MCAPI *pmc_mcapi, char *filename )
{
	static const char fname[] = "mxi_pmc_mcapi_restore_axes()";

	short mcdlg_status;
	char error_buffer[100];

	if ( pmc_mcapi == (MX_PMC_MCAPI *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PMC_MCAPI pointer passed was NULL." );
	}

	if ( filename == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The filename pointer passed was NULL." );
	}

	mcdlg_status = MCDLG_RestoreAxis( pmc_mcapi->controller_handle,
						MC_ALL_AXES,
						0,
						filename );

	if ( mcdlg_status != MCERR_NOERROR ) {
		mxi_pmc_mcapi_translate_error( mcdlg_status,
				error_buffer, sizeof(error_buffer) );

		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"Error restoring the motor configuration for "
		"MCAPI controller %d of record '%s'.  "
		"MCAPI error message = '%s'",
			pmc_mcapi->controller_id,
			pmc_mcapi->record->name, error_buffer );
	}

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxi_pmc_mcapi_download_file( MX_PMC_MCAPI *pmc_mcapi, char *filename )
{
	static const char fname[] = "mxi_pmc_mcapi_download_file()";

	long download_status;
	char error_buffer[100];

	if ( pmc_mcapi == (MX_PMC_MCAPI *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PMC_MCAPI pointer passed was NULL." );
	}

	if ( filename == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The filename pointer passed was NULL." );
	}

	download_status = MCDLG_DownloadFile( NULL,
				pmc_mcapi->controller_handle, 0, filename );

	if ( download_status != MCERR_NOERROR ) {
		mxi_pmc_mcapi_translate_error( download_status,
			error_buffer, sizeof(error_buffer) );

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"An error occurred while download file '%s' to PMC "
		"MCAPI controller '%s'.  MCAPI error message = '%s'",
		    filename, pmc_mcapi->record->name, error_buffer );
	}

	return MX_SUCCESSFUL_RESULT;
}

/****************** Linux ******************/

#elif defined(OS_LINUX)

/* The MCDLG functions are not available on Linux, so we implement
 * replacements for them.  Since I do not know the format of the files
 * read and written by the MCDLG functions, I use a save file format
 * that merely consists of the MCCL commands necessary to reprogram
 * the controller state.  Thus, mxi_pmc_mcapi_restore_axes() becomes
 * merely a call to mxi_pmc_mcapi_download_file().
 */

static mx_status_type
mxi_pmc_mcapi_initialize_dialog_functions( MX_PMC_MCAPI *pmc_mcapi )
{
	/* On Linux, we do not have anything to do here. */

	return MX_SUCCESSFUL_RESULT;
}

#if 0
static mx_status_type
mxi_pmc_mccl_save_parameter( FILE *savefile,
			MX_PMC_MCAPI *pmc_mcapi,
			long axis_id,
			long mx_datatype,
			char *read_format,
			long read_offset,
			char *write_format )
{
	static const char fname[] = "mxi_pmc_mccl_save_parameter()";

	char command[40];
	char response[80];
	long long_value;
	unsigned long ulong_value;
	double double_value;
	int argc;
	char **argv;
	char *duplicate;
	mx_status_type mx_status;

	snprintf( command, sizeof(command), read_format, axis_id );

	mx_status = mxi_pmc_mccl_command( pmc_mcapi, command,
					response, sizeof(response),
					MXI_PMC_MCAPI_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	duplicate = strdup( response );

	mx_string_split( duplicate, " ", &argc, &argv );

	if ( argc <= read_offset ) {
		free(argv); free(duplicate);

		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"The response '%s' to command '%s' for MCAPI controller '%s' "
		"did not include the minimum required number of tokens.  "
		"There should have been at least %ld tokens.",
			response, command, pmc_mcapi->record->name,
			read_offset + 1 );
	}

#if MXI_PMC_MCAPI_DEBUG
	MX_DEBUG(-2,("%s: response '%s', argv[%ld] = '%s'",
		fname, response, read_offset, argv[read_offset] ));
#endif

	switch( mx_datatype ) {
	case MXFT_LONG:
		long_value = mx_string_to_long( argv[read_offset] );

		fprintf( savefile, write_format, axis_id, long_value );
		break;
	case MXFT_ULONG:
		ulong_value = mx_string_to_unsigned_long( argv[read_offset] );

		fprintf( savefile, write_format, axis_id, ulong_value );
		break;
	case MXFT_DOUBLE:
		double_value = atof( argv[read_offset] );

		fprintf( savefile, write_format, axis_id, double_value );
		break;
	default:
		break;
	}

	free(argv); free(duplicate);

	return MX_SUCCESSFUL_RESULT;
}
#endif

static mx_status_type
mxi_pmc_mccl_save_common( FILE *savefile,
			MX_PMC_MCAPI *pmc_mcapi,
			long axis_id )
{
	static const char fname[] = "mxi_pmc_mccl_save_common()";

	MCAXISCONFIG axis_config;
	MCMOTIONEX motion_config;
	MCFILTEREX filter_config;

	unsigned long mcapi_status;
	unsigned long limit_on, limit_off;
	unsigned long hard_limit_mode, soft_limit_mode;
	unsigned long fr_value;
	double servo_loop_period;
	char error_buffer[100];

	if ( pmc_mcapi == (MX_PMC_MCAPI *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PMC_MCAPI pointer passed was NULL." );
	}

#if MXI_PMC_MCAPI_DEBUG
	MX_DEBUG(-2,("%s invoked for axis %ld", fname, axis_id ));
#endif

	/* We save the motor settings in the form of an MCCL script, which
	 * can be directly executed by the controller at restore time.
	 * 
	 * However, we fetch as much of the information as possible from
	 * MCAPI calls to reduce the amount of non-portable code here.
	 */

	axis_config.cbSize = sizeof(MCAXISCONFIG);

	mcapi_status = MCGetAxisConfiguration( pmc_mcapi->binary_handle,
					axis_id, &axis_config );

	if ( mcapi_status != MCERR_NOERROR ) {
		mxi_pmc_mcapi_translate_error( mcapi_status,
			error_buffer, sizeof(error_buffer) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unable to get the axis configuration for axis %lu of "
		"MCAPI controller '%s'.  MCAPI error message = '%s'",
			axis_id, pmc_mcapi->record->name, error_buffer );
	}

	/* If this axis is not actually a motor, then skip it. */

	if ( ( axis_config.MotorType != MC_TYPE_SERVO )
	  && ( axis_config.MotorType != MC_TYPE_STEPPER ) )
	{
		return MX_SUCCESSFUL_RESULT;
	}

	/* Get the motion parameters. */

	motion_config.cbSize = sizeof(MCMOTIONEX);

	mcapi_status = MCGetMotionConfigEx( pmc_mcapi->binary_handle,
					axis_id, &motion_config );

	if ( mcapi_status != MCERR_NOERROR ) {
		mxi_pmc_mcapi_translate_error( mcapi_status,
			error_buffer, sizeof(error_buffer) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unable to get the motion configuration for axis %lu of "
		"MCAPI controller '%s'.  MCAPI error message = '%s'",
			axis_id, pmc_mcapi->record->name, error_buffer );
	}

	/* Get the PID filter parameters. */

	filter_config.cbSize = sizeof(MCFILTEREX);

	mcapi_status = MCGetFilterConfigEx( pmc_mcapi->binary_handle,
					axis_id, &filter_config );

	if ( mcapi_status != MCERR_NOERROR ) {
		mxi_pmc_mcapi_translate_error( mcapi_status,
			error_buffer, sizeof(error_buffer) );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unable to get the PID filter configuration for axis %lu of "
		"MCAPI controller '%s'.  MCAPI error message = '%s'",
			axis_id, pmc_mcapi->record->name, error_buffer );
	}

	/*------------------------------------------------------------------*/

	/* Enable servo amplifier fault. */

	if ( motion_config.EnableAmpFault ) {
		fprintf( savefile, "%luFN0\n", axis_id );
	} else {
		fprintf( savefile, "%luFF\n", axis_id );
	}

	/* Hard limit mode */

	if ( motion_config.HardLimitMode & MC_LIMIT_ABRUPT ) {
		hard_limit_mode = 1;
	} else
	if ( motion_config.HardLimitMode & MC_LIMIT_SMOOTH ) {
		hard_limit_mode = 2;
	} else {
		hard_limit_mode = 0;
	}

	if ( motion_config.HardLimitMode & MC_LIMIT_INVERT ) {
		hard_limit_mode += 128;
	}

	/* Soft limit mode */

	if ( motion_config.SoftLimitMode & MC_LIMIT_ABRUPT ) {
		soft_limit_mode = 4;
	} else
	if ( motion_config.SoftLimitMode & MC_LIMIT_SMOOTH ) {
		soft_limit_mode = 8;
	} else {
		soft_limit_mode = 0;
	}

	/* Set limit modes */

	fprintf( savefile, "%luLM%lu\n",
		axis_id, hard_limit_mode + soft_limit_mode );

	/* Soft limits */

	fprintf( savefile, "%luHL%.6f\n",
					axis_id, motion_config.SoftLimitHigh );

	fprintf( savefile, "%luLL%.6f\n", axis_id, motion_config.SoftLimitLow );

	/* Hard and Soft Limit Enable */

	limit_on  = 0;
	limit_off = 0;

	if ( motion_config.HardLimitMode & MC_LIMIT_HIGH ) {
		limit_on  += 1;
	} else {
		limit_off += 1;
	}
	if ( motion_config.HardLimitMode & MC_LIMIT_LOW ) {
		limit_on  += 2;
	} else {
		limit_off += 2;
	}
	if ( motion_config.SoftLimitMode & MC_LIMIT_HIGH ) {
		limit_on  += 4;
	} else {
		limit_off += 4;
	}
	if ( motion_config.SoftLimitMode & MC_LIMIT_LOW ) {
		limit_on  += 8;
	} else {
		limit_off += 8;
	}

	if ( limit_on ) {
		if ( limit_on == 15 ) {
			limit_on = 0;
		}
		fprintf( savefile, "%luLN%lu\n", axis_id, limit_on );
	}

	if ( limit_off ) {
		if ( limit_off == 15 ) {
			limit_off = 0;
		}
		fprintf( savefile, "%luLF%lu\n", axis_id, limit_off );
	}

	/*------------------------------------------------------------------*/

	servo_loop_period = MX_PMC_MCAPI_DCX_PCI100_SERVO_LOOP_PERIOD;

	/* Proportional gain. */

	fprintf( savefile, "%luSG%lu\n", axis_id,
				mx_round( filter_config.Gain ) );

	/* Integral gain. */

	fprintf( savefile, "%luSI%lu\n", axis_id,
				mx_round( filter_config.IntegralGain ) );

	fprintf( savefile, "%luIL%lu\n", axis_id,
				mx_round( filter_config.IntegrationLimit ) );

#if 0
	fprintf( savefile, "%lu??%ld\n", axis_id,
					(long) filter_config.IntegralOption );
#endif

	/* Derivative gain. */

	fprintf( savefile, "%luSD%lu\n", axis_id,
				mx_round( filter_config.DerivativeGain ) );

	fr_value = mx_round( filter_config.DerSamplePeriod
				/ servo_loop_period ) - 1;

	fprintf( savefile, "%luFR%lu\n", axis_id, fr_value );

	/* Maximum following error. */

	fprintf( savefile, "%luSE%lu\n", axis_id,
				mx_round( filter_config.FollowingError ) );

#if 0
	/* Velocity gain. */

	fprintf( savefile, "%lu??%.6f\n", axis_id, filter_config.VelocityGain );

	/* Acceleration/deceleration gain. */

	fprintf( savefile, "%lu??%.6f\n", axis_id, filter_config.AccelGain );

	fprintf( savefile, "%lu??%.6f\n", axis_id, filter_config.DecelGain );

	/* Scaling and update rate. */

	fprintf( savefile, "%lu??%.6f\n",
				axis_id, filter_config.EncoderScaling );

	fprintf( savefile, "%lu??%ld\n", axis_id,
					(long) filter_config.UpdateRate );
#endif

	/*------------------------------------------------------------------*/

	fprintf( savefile, "%luSA%.6f\n", axis_id, motion_config.Acceleration );

	fprintf( savefile, "%luSV%.6f\n", axis_id, motion_config.Velocity );

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxi_pmc_mccl_save_dcx_pci100_servo( FILE *savefile,
			MX_PMC_MCAPI *pmc_mcapi,
			long axis_id )
{
	mx_status_type mx_status;

	mx_status = mxi_pmc_mccl_save_common( savefile, pmc_mcapi, axis_id );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;
	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxi_pmc_mcapi_save_axes( MX_PMC_MCAPI *pmc_mcapi, char *filename )
{
	static const char fname[] = "mxi_pmc_mcapi_save_axes()";

	MCPARAMEX *cc;
	MCAXISCONFIG axis;
	long mcapi_status;

	time_t time_struct;
	struct tm current_time;
	char time_buffer[40];
	char username_buffer[80];

	FILE *savefile;
	int saved_errno;
	long i, maximum_num_axes;
	char error_buffer[100];
	mx_status_type mx_status;

	if ( pmc_mcapi == (MX_PMC_MCAPI *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PMC_MCAPI pointer passed was NULL." );
	}

	if ( filename == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The filename pointer passed was NULL." );
	}

	savefile = fopen( filename, "w" );

	if ( savefile == NULL ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"The attempt to open file '%s' for saving axes from "
		"PMC MCAPI controller '%s' failed.  "
		"Errno = %d, error message = '%s'",
			filename, pmc_mcapi->record->name,
			saved_errno, strerror( saved_errno ) );
	}

	/* Create a timestamp for this save file. */

	time( &time_struct );

	(void) localtime_r( &time_struct, &current_time );

	strftime( time_buffer, sizeof(time_buffer),
			"%b %d %H:%M:%S", &current_time );

	/* Get the user name. */

	mx_username( username_buffer, sizeof(username_buffer) );

	/* Write the save file header. */

	fprintf( savefile, "#\n" );
	fprintf( savefile,
		"# MX PMC MCAPI parameter file for controller '%s'.\n",
		pmc_mcapi->record->name );
	fprintf( savefile, "#\n" );
	fprintf( savefile, "# Created %s by user '%s'.\n",
		time_buffer, username_buffer );
	fprintf( savefile, "#\n" );

	cc = &(pmc_mcapi->configuration);

	maximum_num_axes = cc->MaximumAxes;

	for ( i = 1; i <= maximum_num_axes; i++ ) {

		axis.cbSize = sizeof(MCAXISCONFIG);

		mcapi_status = MCGetAxisConfiguration( pmc_mcapi->binary_handle,
							i, &axis );

		if ( mcapi_status != MCERR_NOERROR ) {
			fclose( savefile );

			mxi_pmc_mcapi_translate_error( mcapi_status,
				error_buffer, sizeof(error_buffer) );

			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Unable to get information about axis %ld for "
			"MCAPI controller '%s'.  MCAPI error message = '%s'",
				i, pmc_mcapi->record->name, error_buffer );
		}

		fprintf( savefile, "# Axis %ld\n", i );

		mx_status = MX_SUCCESSFUL_RESULT;

		switch( axis.MotorType ) {
		case MC_TYPE_SERVO:
			switch( cc->ControllerType ) {
			case DCXPCI100:
				mx_status = mxi_pmc_mccl_save_dcx_pci100_servo(
						savefile, pmc_mcapi, i );
				break;
			default:
				return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
				"Servo motor support for controller type %d "
				"of MCAPI controller '%s' has not yet "
				"been implemented.", cc->ControllerType,
					pmc_mcapi->record->name );
					
			}
			break;
		case MC_TYPE_STEPPER:
			switch( cc->ControllerType ) {
			default:
				return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
				"Stepper motor support for controller type %d "
				"of MCAPI controller '%s' has not yet "
				"been implemented.", cc->ControllerType,
					pmc_mcapi->record->name );
					
			}
			break;
		default:
#if MXI_PMC_MCAPI_DEBUG
			MX_DEBUG(-2,("%s: axis %ld not saved", fname, i));
#endif
			break;
		}

		if ( mx_status.code != MXE_SUCCESS ) {
			fclose( savefile );
			return mx_status;
		}
	}

	fclose( savefile );

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxi_pmc_mcapi_restore_axes( MX_PMC_MCAPI *pmc_mcapi, char *filename )
{
	static const char fname[] = "mxi_pmc_mcapi_restore_axes()";

	mx_status_type mx_status;

	if ( pmc_mcapi == (MX_PMC_MCAPI *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PMC_MCAPI pointer passed was NULL." );
	}

	if ( filename == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The filename pointer passed was NULL." );
	}

	if ( strlen(filename) == 0 ) {
		mx_warning("The savefile name for PMC MCAPI controller '%s' "
			"is empty, so nothing will be restored to the "
			"controller.", pmc_mcapi->record->name );

		return MX_SUCCESSFUL_RESULT;
	}

	mx_status = mxi_pmc_mcapi_download_file( pmc_mcapi, filename );

	return mx_status;
}

static mx_status_type
mxi_pmc_mcapi_download_file( MX_PMC_MCAPI *pmc_mcapi, char *filename )
{
	static const char fname[] = "mxi_pmc_mcapi_download_file()";

	FILE *download_file;
	int saved_errno;
	char buffer[MXU_PMC_MCAPI_BUFFER_LENGTH+1];
	size_t length;
	mx_status_type mx_status;

	if ( pmc_mcapi == (MX_PMC_MCAPI *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PMC_MCAPI pointer passed was NULL." );
	}

	if ( filename == (char *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The filename pointer passed was NULL." );
	}

	if ( strlen(filename) == 0 ) {
		mx_warning("The requested filename for PMC MCAPI "
			"controller '%s' is empty, so nothing will be "
			"downloaded to the controller.",
				pmc_mcapi->record->name );

		return MX_SUCCESSFUL_RESULT;
	}

	download_file = fopen( filename, "r" );

	if ( download_file == NULL ) {
		saved_errno = errno;

		return mx_error( MXE_FILE_IO_ERROR, fname,
		"The attempt to open file '%s' for downloading to "
		"PMC MCAPI controller '%s' failed.  "
		"Errno = %d, error message = '%s'",
			filename, pmc_mcapi->record->name,
			saved_errno, strerror( saved_errno ) );
	}

	while(1) {
		fgets( buffer, sizeof(buffer), download_file );

		if ( feof(download_file) ) {
			fclose(download_file);
			break;		/* Exit the while() loop. */
		}
		if ( ferror(download_file) ) {
			saved_errno = errno;
			fclose(download_file);

			return mx_error( MXE_FILE_IO_ERROR, fname,
			"An error occurred while reading from download "
			"file '%s' for PMC MCAPI controller '%s'.  "
			"Errno = %d, error message = '%s'",
				filename, pmc_mcapi->record->name,
				saved_errno, strerror(saved_errno) );
		}

		/* Ignore empty lines read from the download file. */

		length = strlen(buffer);

		if ( length == 0 ) {
			continue;	/* Go back to read the next line. */
		}

		/* Ignore lines that start with the comment character '#'. */

		if ( buffer[0] == '#' ) {
			continue;	/* Go back to read the next line. */
		}

		/* If present, remove a trailing newline character. */

		if ( buffer[length-1] == '\n' ) {
			buffer[length-1] = '\0';
		}

		/* Now we just send the line to the controller as
		 * an MCCL command.
		 */

		mx_status = mxi_pmc_mccl_command( pmc_mcapi, buffer,
						NULL, 0, MXI_PMC_MCAPI_DEBUG );

		if ( mx_status.code != MXE_SUCCESS ) {
			fclose(download_file);

			return mx_status;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/****************** Other platforms ******************/

#else /* Not Windows or Linux */

#error Support for platforms other than Windows or Linux has not yet been implemented.

#endif /* MCDLG_ code. */


