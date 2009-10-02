/*
 * Name:    i_u500.c
 *
 * Purpose: MX driver for Aerotech Unidex 500 series controllers.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2004, 2006, 2008-2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_U500_DEBUG			FALSE

#define MXI_U500_DEBUG_TIMING		FALSE

#include <stdio.h>

#include "mxconfig.h"

#if HAVE_U500

#include <stdarg.h>
#include <string.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "i_u500.h"

#if MXI_U500_DEBUG_TIMING
#  include "mx_hrt_debug.h"
#endif

/* Aerotech includes */

#include "Aerotype.h"
#include "Build.h"
#include "Quick.h"
#include "Wapi.h"

MX_RECORD_FUNCTION_LIST mxi_u500_record_function_list = {
	mxi_u500_initialize_type,
	mxi_u500_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_u500_open,
	mxi_u500_close,
	mxi_u500_resynchronize,
	mxi_u500_resynchronize,
	mxi_u500_special_processing_setup
};

MX_RECORD_FIELD_DEFAULTS mxi_u500_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_U500_STANDARD_FIELDS
};

long mxi_u500_num_record_fields
		= sizeof( mxi_u500_record_field_defaults )
			/ sizeof( mxi_u500_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_u500_rfield_def_ptr
			= &mxi_u500_record_field_defaults[0];

static mx_status_type mxi_u500_process_function( void *record_ptr,
						void *record_field_ptr,
						int operation );

static mx_status_type
mxi_u500_get_pointers( MX_RECORD *record,
			MX_U500 **u500,
			const char *calling_fname )
{
	static const char fname[] = "mxi_u500_get_pointers()";

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( u500 == (MX_U500 **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_U500 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*u500 = (MX_U500 *) record->record_type_struct;

	if ( (*u500) == (MX_U500 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_U500 pointer for U500 record '%s' is NULL.",
			record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*==========================*/

#define NUM_U500_FIELDS    5

MX_EXPORT mx_status_type
mxi_u500_initialize_type( long type )
{
        static const char fname[] = "mxi_u500_initialize_type()";

	static const char field_name[NUM_U500_FIELDS][MXU_FIELD_NAME_LENGTH+1]
	    = {
		"board_type",
		"firmware_filename",
		"parameter_filename",
		"calibration_filename",
		"pso_firmware_filename" };

        MX_DRIVER *driver;
        MX_RECORD_FIELD_DEFAULTS *record_field_defaults;
        MX_RECORD_FIELD_DEFAULTS **record_field_defaults_ptr;
        MX_RECORD_FIELD_DEFAULTS *field;
        int i;
        long num_record_fields;
	long referenced_field_index;
        long num_boards_varargs_cookie;
        mx_status_type status;

        driver = mx_get_driver_by_type( type );

        if ( driver == (MX_DRIVER *) NULL ) {
                return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
                        "Record type %ld not found.", type );
        }

        record_field_defaults_ptr = driver->record_field_defaults_ptr;

        if (record_field_defaults_ptr == (MX_RECORD_FIELD_DEFAULTS **) NULL) {
                return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
                "'record_field_defaults_ptr' for record type '%s' is NULL.",
                        driver->name );
        }

        record_field_defaults = *record_field_defaults_ptr;

        if ( record_field_defaults == (MX_RECORD_FIELD_DEFAULTS *) NULL ) {
                return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
                "'record_field_defaults_ptr' for record type '%s' is NULL.",
                        driver->name );
        }

        if ( driver->num_record_fields == (long *) NULL ) {
                return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
                "'num_record_fields' pointer for record type '%s' is NULL.",
                        driver->name );
        }

	num_record_fields = *(driver->num_record_fields);

        status = mx_find_record_field_defaults_index(
                        record_field_defaults, num_record_fields,
                        "num_boards", &referenced_field_index );

        if ( status.code != MXE_SUCCESS )
                return status;

        status = mx_construct_varargs_cookie(
                        referenced_field_index, 0, &num_boards_varargs_cookie);

        if ( status.code != MXE_SUCCESS )
                return status;

	for ( i = 0; i < NUM_U500_FIELDS; i++ ) {
		status = mx_find_record_field_defaults(
			record_field_defaults, num_record_fields,
			field_name[i], &field );

		if ( status.code != MXE_SUCCESS )
			return status;

		field->dimension[0] = num_boards_varargs_cookie;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_u500_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_u500_create_record_structures()";

	MX_U500 *u500;

	/* Allocate memory for the necessary structures. */

	u500 = (MX_U500 *) malloc( sizeof(MX_U500) );

	if ( u500 == (MX_U500 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_U500 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_type_struct = u500;
	record->record_class_struct = NULL;
	record->class_specific_function_list = NULL;

	u500->record = record;

	/* Initialize some variables. */

	strcpy( u500->load_program, "" );

	u500->program_number = -1;
	u500->unload_program = -1;
	u500->run_program = -1;
	u500->fault_acknowledge = -1;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_u500_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_u500_open()";

	MX_U500 *u500 = NULL;
	char *calibration_filename, *pso_filename;
	int not_initialized;
	SHORT board_number;
	AERERR_CODE wapi_status;
	mx_status_type mx_status;

#if MXI_U500_DEBUG
	MX_DEBUG(-2, ("%s invoked.", fname));
#endif

	mx_status = mxi_u500_get_pointers( record, &u500, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	u500->current_board_number = 0;

	/* Setup each of the U500 boards. */

	for ( board_number = 1;
		board_number <= u500->num_boards;
		board_number++ )
	{
		/* Open the board. */

		wapi_status = WAPIAerOpen((SHORT)(board_number - 1));

		if ( wapi_status != 0 ) {
			return mxi_u500_error( wapi_status, fname,
			"Cannot open U500 board %d for record '%s'.",
				board_number, record->name );
		}

		/* Switch to the requested board number  */

#if 0
		mx_status = mxi_u500_select_board( u500, board_number );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
#endif

		/* Initialize the board as needed. */

		not_initialized = WAPIAerCheckInitz((SHORT)(board_number - 1));

#if U500_DEBUG
		MX_DEBUG( 2,("%s: not_initialized = %d",
				fname, not_initialized));
#endif

		if ( not_initialized == 0 ) {
			/* Hardware initialization not needed */

#if MXI_U500_DEBUG
			MX_DEBUG(-2,
	("%s: WAPIAerSoftwareInitialize for board %d using parameter file '%s'",
		 	  fname, (int) board_number,
			  u500->parameter_filename[board_number-1]));
#endif

			wapi_status = WAPIAerSoftwareInitialize(
				u500->parameter_filename[board_number-1] );

			if ( wapi_status != 0 ) {
				return mxi_u500_error( wapi_status, fname,
				"Cannot soft initialize U500 '%s' board %d",
					record->name, board_number );
			}
		} else {
			/* Need to initialize the hardware */

#if MXI_U500_DEBUG
			MX_DEBUG(-2,
	  ("%s: WAPIAerInitialize() for board %d using firmware file '%s', "
		 	   "parameter file '%s'", fname, (int) board_number,
				u500->firmware_filename[board_number-1],
				u500->parameter_filename[board_number-1] ));
#endif

			wapi_status = WAPIAerInitialize( NULL,
				u500->firmware_filename[board_number-1],
				u500->parameter_filename[board_number-1] );

			if ( wapi_status != 0 ) {
				return mxi_u500_error( wapi_status, fname,
				"Cannot hard initialize U500 '%s' board %d",
					record->name, board_number );
			}

			/* If requested, load the calibration data. */

			calibration_filename =
				u500->calibration_filename[board_number-1];

			if ( strlen(calibration_filename) > 0 ) {
#if MXI_U500_DEBUG
				MX_DEBUG(-2,
				("%s: Loading calibration file '%s'",
				 	fname, calibration_filename));
#endif

				wapi_status = WAPILoadCalFile(
						calibration_filename );
				if ( wapi_status != 0 ) {
					return mxi_u500_error(wapi_status,fname,
		"Cannot load calibration file '%s' for U500 '%s' board %d",
						calibration_filename,
						record->name, board_number );
				}
			}

			/* If requested, load the PSO firmware. */

			pso_filename =
				u500->pso_firmware_filename[board_number-1];

			if ( strlen(pso_filename) > 0 ) {
#if MXI_U500_DEBUG
				MX_DEBUG(-2,
				("%s: Loading PSO firmware file '%s'",
				 	fname, pso_filename));
#endif

				wapi_status = WAPIAerInitializePSO(
								pso_filename );
				if ( wapi_status != 0 ) {
					return mxi_u500_error(
						wapi_status, fname,
		"Cannot load PSO firmware file '%s' for U500 '%s' board %d",
						pso_filename,
						record->name, board_number );
				}
			}
		}

		u500->board_type[board_number-1] = WAPIGetBoardType();

#if MXI_U500_DEBUG
		MX_DEBUG(-2,("%s: U500 '%s' board %d, board_type = %d",
			fname, record->name, board_number,
			u500->board_type[board_number-1]));
#endif

		/* Get the version numbers of the software we are running. */

		wapi_status = WAPIAerGetQlibVersion( &(u500->qlib_version[0]),
						&(u500->qlib_version[1]) );

		if ( wapi_status != 0 ) {
			return mxi_u500_error(wapi_status, fname,
			"Cannot get Qlib version number for U500 '%s'.",
				record->name );
		}

		wapi_status = WAPIAerGetDrvVersion( &(u500->drv_version[0]),
						&(u500->drv_version[1]) );

		if ( wapi_status != 0 ) {
			return mxi_u500_error(wapi_status, fname,
			"Cannot get Drv version number for U500 '%s'.",
				record->name );
		}

		wapi_status = WAPIAerGetWapiVersion( &(u500->wapi_version[0]),
						&(u500->wapi_version[1]) );

		if ( wapi_status != 0 ) {
			return mxi_u500_error(wapi_status, fname,
			"Cannot get WAPI version number for U500 '%s'.",
				record->name );
		}

#if U500_DEBUG
		MX_DEBUG(-2,
		("%s: U500 '%s', Qlib = %hd.%hd, Drv = %hd.%hd, WAPI = %hd.%hd",
			fname, record->name,
			u500->qlib_version[0], u500->qlib_version[1],
			u500->drv_version[0], u500->drv_version[1],
			u500->wapi_version[0], u500->wapi_version[1] ));
#endif

		/* Use absolute positioning, motor steps, and steps per sec */

		mx_status = mxi_u500_command( u500,
				board_number, "PR AB ST ST/SE" );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

#if 0
		/* Open the parameter file for this board, so that we can
		 * read and change parameters.
		 */

		wapi_status = WAPIAerOpenParam( 
				u500->parameter_filename[board_number-1] );

		if ( wapi_status != 0 ) {
			return mxi_u500_error(wapi_status, fname,
		"Cannot open parameter file '%s' for U500 '%s' board %d",
				u500->parameter_filename[board_number-1],
				record->name, board_number );
		}
#endif
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_u500_close( MX_RECORD *record )
{
	static const char fname[] = "mxi_u500_close()";

	MX_U500 *u500 = NULL;
	SHORT i;
	AERERR_CODE wapi_status;
	mx_status_type mx_status;

#if MXI_U500_DEBUG
	MX_DEBUG(-2, ("%s invoked.", fname));
#endif

	mx_status = mxi_u500_get_pointers( record, &u500, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	u500->current_board_number = 0;

	/* Close connection to each of the U500 boards. */

	for ( i = 0; i < u500->num_boards; i++ ) {

		/* Open the board. */

		wapi_status = WAPIAerOpen(i);

		if ( wapi_status != 0 ) {
			(void) mxi_u500_error( wapi_status, fname,
			"Cannot close U500 board %d for record '%s'.",
				i+1, record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_u500_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxi_u500_resynchronize()";

	MX_U500 *u500 = NULL;
	int board_number;
	AERERR_CODE wapi_status;
	mx_status_type mx_status;

	mx_status = mxi_u500_get_pointers( record, &u500, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send aborts to stop any moves in progress. */

	for ( board_number = 1;
		board_number <= u500->num_boards;
		board_number++ )
	{
		mx_status = mxi_u500_command( u500, board_number, "AB" );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_msleep(100);
	}

	/* Wait a second for the aborts to finish. */

	mx_msleep(1000);

	/* Send a fault acknowlege to the U500. */

	wapi_status = WAPIAerFaultAck();

	if ( wapi_status != 0 ) {
		return mxi_u500_error( wapi_status, fname,
	    "The attempt to perform a fault acknowledge for U500 '%s' failed.",
			record->name );
	}

	return mx_status;
}

/* === Functions specific to this driver. === */

MX_EXPORT mx_status_type
mxi_u500_command( MX_U500 *u500, int board_number, char *command )
{
	static const char fname[] = "mxi_u500_command()";

	AERERR_CODE wapi_status;
	mx_status_type mx_status;

	/* If all we wanted to do was to change boards, then return now. */

	if ( command == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}

#if MXI_U500_DEBUG
	MX_DEBUG(-2,("%s: sending '%s' to u500 '%s', board %d",
		fname, command, u500->record->name, board_number));
#endif

	/* If necessary, change the board number. */

	if ( board_number != u500->current_board_number ) {
		mx_status = mxi_u500_select_board( u500, board_number );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Now send the command. */

	wapi_status = WAPIAerSend( command );

	if ( wapi_status != 0 ) {
		return mxi_u500_error( wapi_status, fname,
		"Error sending command '%s' to U500 '%s'",
			command, u500->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_u500_select_board( MX_U500 *u500, int board_number )
{
#if 0	/* FIXME: Board selection is bypassed for now. */

	static const char fname[] = "mxi_u500_select_board()";

	AERERR_CODE wapi_status;
	char buffer[100];

	sprintf( buffer, "BO %d", board_number - 1 );

#if MXI_U500_DEBUG
	MX_DEBUG(-2,("%s: sending '%s' to u500 '%s', board %d",
		fname, buffer, u500->record->name, board_number));
#endif

	wapi_status = WAPIAerSend( buffer );

	if ( wapi_status != 0 ) {
		return mxi_u500_error( wapi_status, fname,
			"Attempt to change to board %d for record '%s' failed.",
				board_number, u500->record->name );
	}
#endif

	u500->current_board_number = board_number;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_u500_set_program_number( MX_U500 *u500, int program_number )
{
	static const char fname[] = "mxi_u500_set_program_number()";

	AERERR_CODE wapi_status;

#if 0
	/* FIXME!!! - WAPIAerSetProgNum() does not seem to exist!!! */

	wapi_status = WAPIAerSetProgNum( program_number );

	if ( wapi_status != 0 ) {
		return mxi_u500_program_error( wapi_status, fname,
	"The attempt to set the program number to %d for U500 '%s' failed.",
					program_number, u500->record->name );
	}
#else
	mx_warning(
	"Attempt to invoke nonexistant function WAPIAerSetProgNum( u500, %d )",
		program_number );
#endif

	u500->program_number = program_number;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_u500_error( long wapi_status, const char *fname, char *format, ... )
{
	char buffer[2000];
	LPSTR wapi_error_message;
	va_list args;

	va_start(args, format);
	vsprintf(buffer, format, args);
	va_end(args);

#if 0
	wapi_error_message = NULL;

	WAPIErrorToAscii( wapi_status, wapi_error_message );

	return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"%s.  WAPI status = %ld, WAPI error = '%s'",
		buffer, wapi_status, wapi_error_message );
#else
	return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"%s.  WAPI status = %ld", buffer, wapi_status );
#endif
}

MX_EXPORT mx_status_type
mxi_u500_program_error( long wapi_status, const char *fname, char *format, ... )
{
	char error_message[80];
	char buffer[2000];
	va_list args;

	va_start(args, format);
	vsprintf(buffer, format, args);
	va_end(args);

	(void) WAPIAerGetProgErrMsg( buffer );

	return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"%s.  WAPI status = %ld, program error = '%s'",
		buffer, wapi_status, error_message );
};

MX_EXPORT mx_status_type
mxi_u500_read_parameter( MX_U500 *u500,
			int parameter_number,
			double *parameter_value )
{
	static const char fname[] = "mxi_u500_read_parameter()";

	SHORT wapi_status;
	CHAR parameter_string_value[100];
	int num_items;

	wapi_status = WAPIAerReadParam( (SHORT) parameter_number,
					parameter_string_value );

	if ( wapi_status < 0 ) {
		return mxi_u500_error( wapi_status, fname,
			"Attempt to read parameter %d from "
			"U500 controller '%s' failed.",
				parameter_number,
				u500->record->name );
	}

	num_items = sscanf( parameter_string_value, "%lg", parameter_value );

	if ( num_items != 1 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Unable to find a parameter value in the string returned "
		"for U500 controller '%s' for parameter %d",
			u500->record->name, parameter_number );
	}

#if MXI_U500_DEBUG
	MX_DEBUG(-2,("%s: read parameter %d for U500 '%s', value = %g",
		fname, parameter_number, u500->record->name, *parameter_value));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_u500_special_processing_setup( MX_RECORD *record )
{
	static const char fname[] = "mxi_u500_special_processing_setup()";

	MX_RECORD_FIELD *record_field;
	MX_RECORD_FIELD *record_field_array;
	long i;

#if U500_DEBUG
	MX_DEBUG(-2,("%s invoked.", fname));
#endif

	record_field_array = record->record_field_array;

	for ( i = 0; i < record->num_record_fields; i++ ) {

		record_field = &record_field_array[i];

		switch( record_field->label_value ) {
		case MXLV_U500_PROGRAM_NUMBER:
		case MXLV_U500_LOAD_PROGRAM:
		case MXLV_U500_UNLOAD_PROGRAM:
		case MXLV_U500_RUN_PROGRAM:
		case MXLV_U500_STOP_PROGRAM:
		case MXLV_U500_FAULT_ACKNOWLEDGE:
			record_field->process_function
					    = mxi_u500_process_function;
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
mxi_u500_process_function( void *record_ptr,
			void *record_field_ptr, int operation )
{
	static const char fname[] = "mxi_u500_process_function()";

	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
	MX_U500 *u500 = NULL;
	AERERR_CODE wapi_status;
	mx_status_type mx_status;

	record = (MX_RECORD *) record_ptr;
	record_field = (MX_RECORD_FIELD *) record_field_ptr;
	u500 = (MX_U500 *) record->record_type_struct;

	mx_status = MX_SUCCESSFUL_RESULT;

	switch( operation ) {
	case MX_PROCESS_GET:
		switch( record_field->label_value ) {
		case MXLV_U500_PROGRAM_NUMBER:
		case MXLV_U500_LOAD_PROGRAM:
		case MXLV_U500_UNLOAD_PROGRAM:
		case MXLV_U500_RUN_PROGRAM:
		case MXLV_U500_FAULT_ACKNOWLEDGE:
			break;
		case MXLV_U500_PROGRAM_NAME:
			wapi_status = WAPIAerGetProgName( u500->program_name );

			if ( wapi_status != 0 ) {
				return mxi_u500_error( wapi_status, fname,
	"The attempt to read the current program name for U500 '%s' failed.",
					record->name );
			}
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
		case MXLV_U500_PROGRAM_NAME:
			return mx_error( MXE_UNSUPPORTED, fname,
	"Setting the program name for U500 '%s' is unsupported.  If you are "
	"trying to upload a program, use 'load_program' instead.",
				record->name );
			break;
		case MXLV_U500_PROGRAM_NUMBER:
			mx_status = mxi_u500_set_program_number( u500,
							u500->program_number );
			break;
		case MXLV_U500_LOAD_PROGRAM:
			wapi_status = 
			    WAPIAerLoadProgram( u500->load_program, "", "" );

			if ( wapi_status != 0 ) {
				return mxi_u500_program_error(
					wapi_status, fname,
	"The attempt to load program '%s' for U500 '%s' failed.",
					u500->load_program, record->name );
			}
			break;
		case MXLV_U500_UNLOAD_PROGRAM:
			if ( (u500->unload_program >= 1)
			  || (u500->unload_program <= 4) )
			{
				mx_status = mxi_u500_set_program_number( u500,
							u500->unload_program );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
			}

			wapi_status = WAPIAerUnloadProgram();

			if ( wapi_status != 0 ) {
				return mxi_u500_program_error(
					wapi_status, fname,
	"The attempt to unload program %d for U500 '%s' failed.",
					u500->program_number, record->name );
			}
			break;
		case MXLV_U500_RUN_PROGRAM:
			if ( (u500->run_program >= 1)
			  || (u500->run_program <= 4) )
			{
				mx_status = mxi_u500_set_program_number( u500,
							u500->run_program );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
			}

#if 0
	/* FIXME!!! - WAPIAerRunProgram() does not seem to exist!!! */

			wapi_status = WAPIAerRunProgram();

			if ( wapi_status != 0 ) {
				return mxi_u500_program_error(
					wapi_status, fname,
	"The attempt to run program %d for U500 '%s' failed.",
					u500->program_number, record->name );
			}
#else
			mx_warning(
		"Attempt to invoke nonexistant function WAPIAerRunProgram()" );
#endif
			break;
		case MXLV_U500_STOP_PROGRAM:
#if 0
	/* FIXME!!! - WAPIAerStopProgram() does not seem to exist!!! */

			wapi_status = WAPIAerStopProgram( u500->stop_program );

			if ( wapi_status != 0 ) {
				return mxi_u500_program_error(
					wapi_status, fname,
	"The attempt to stop program %d for U500 '%s' failed.",
					u500->stop_program, record->name );
			}
#else
			mx_warning(
	"Attempt to invoke nonexistant function WAPIAerStopProgram( %d )",
				u500->stop_program );
#endif
			break;
		case MXLV_U500_FAULT_ACKNOWLEDGE:
			wapi_status = WAPIAerFaultAck();

			if ( wapi_status != 0 ) {
				return mxi_u500_error( wapi_status, fname,
		"The attempt to acknowledge a fault for U500 '%s' failed.",
					record->name );
			}
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

#endif /* HAVE_U500 */

