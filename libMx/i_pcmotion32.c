/*
 * Name:    i_pcmotion32.c
 *
 * Purpose: MX interface driver for the National Instruments PCMOTION32.DLL
 *          distributed for use with the ValueMotion series of motor
 *          controllers (formerly made by nuLogic).
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>

#include "mxconfig.h"

#if HAVE_PCMOTION32

#ifndef OS_WIN32
#error "This driver is only supported under Win32 (Windows NT/98/95)."
#endif

#include <stdlib.h>

#include <windows.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_generic.h"

#include "i_pcmotion32.h"

#include "pcMotion32.h"

MX_RECORD_FUNCTION_LIST mxi_pcmotion32_record_function_list = {
	NULL,
	mxi_pcmotion32_create_record_structures,
	NULL,
	NULL,
	mxi_pcmotion32_print_structure,
	NULL,
	NULL,
	mxi_pcmotion32_open
};

MX_GENERIC_FUNCTION_LIST mxi_pcmotion32_generic_function_list = {
	NULL
};

MX_RECORD_FIELD_DEFAULTS mxi_pcmotion32_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_PCMOTION32_STANDARD_FIELDS
};

long mxi_pcmotion32_num_record_fields
		= sizeof( mxi_pcmotion32_record_field_defaults )
			/ sizeof( mxi_pcmotion32_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_pcmotion32_rfield_def_ptr
			= &mxi_pcmotion32_record_field_defaults[0];

#define MXI_PCMOTION32_DEBUG	FALSE

static mx_status_type
mxi_pcmotion32_get_pointers( MX_RECORD *record,
			MX_PCMOTION32 **pcmotion32,
			const char *calling_fname )
{
	static const char fname[] = "mxi_pcmotion32_get_pointers()";

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( pcmotion32 == (MX_PCMOTION32 **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_PCMOTION32 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*pcmotion32 = (MX_PCMOTION32 *) (record->record_type_struct);

	if ( *pcmotion32 == (MX_PCMOTION32 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_PCMOTION32 pointer for record '%s' is NULL.",
			record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/* ====== */

MX_EXPORT mx_status_type
mxi_pcmotion32_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_pcmotion32_create_record_structures()";

	MX_GENERIC *generic;
	MX_PCMOTION32 *pcmotion32;
	int i;

	/* Allocate memory for the necessary structures. */

	generic = (MX_GENERIC *) malloc( sizeof(MX_GENERIC) );

	if ( generic == (MX_GENERIC *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_GENERIC structure." );
	}

	pcmotion32 = (MX_PCMOTION32 *) malloc( sizeof(MX_PCMOTION32) );

	if ( pcmotion32 == (MX_PCMOTION32 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_PCMOTION32 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = generic;
	record->record_type_struct = pcmotion32;
	record->class_specific_function_list
				= &mxi_pcmotion32_generic_function_list;

	generic->record = record;
	pcmotion32->record = record;

	for ( i = 0; i < MX_PCMOTION32_NUM_MOTORS; i++ ) {
		pcmotion32->motor_array[i] = NULL;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_pcmotion32_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxi_pcmotion32_print_structure()";

	MX_PCMOTION32 *pcmotion32;
	MX_RECORD *this_record;
	int i;
	mx_status_type mx_status;

	mx_status = mxi_pcmotion32_get_pointers( record, &pcmotion32, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "Record '%s':\n\n", record->name);
	fprintf(file, "  name                       = \"%s\"\n",
						record->name);

	fprintf(file, "  superclass                 = interface\n");
	fprintf(file, "  class                      = generic\n");
	fprintf(file, "  type                       = PCMOTION32\n");
	fprintf(file, "  label                      = \"%s\"\n",
						record->label);

	fprintf(file, "  board ID                   = %d\n",
						pcmotion32->board_id);

	fprintf(file, "  motors in use              = (");

	for ( i = 0; i < MX_PCMOTION32_NUM_MOTORS; i++ ) {

		this_record = pcmotion32->motor_array[i];

		if ( this_record == NULL ) {
			fprintf( file, "NULL" );
		} else {
			fprintf( file, "%s", this_record->name );
		}
		if ( i != (MX_PCMOTION32_NUM_MOTORS - 1) ) {
			fprintf( file, ", " );
		}
	}

	fprintf(file, ")\n");

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_pcmotion32_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_pcmotion32_open()";

	MX_PCMOTION32 *pcmotion32;
	BYTE boardID, board_type;
	WORD limit_switch_polarity;
	WORD enable_limit_switches;
	int status;
	mx_status_type mx_status;

	mx_status = mxi_pcmotion32_get_pointers( record, &pcmotion32, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	boardID = pcmotion32->board_id;

	/* Verify that the board is there by asking for the board type. */

	status = get_board_type( boardID, &board_type );

	if ( status != 0 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Board %d for motor controller '%s' is not available.  "
		"Reason = '%s'",
			(int) boardID, record->name,
			mxi_pcmotion32_strerror( status ) );
	}

	/* Set the polarity of the limit switches and home switches. */

	limit_switch_polarity = (WORD) pcmotion32->limit_switch_polarity;

	status = set_lim_pol( boardID, limit_switch_polarity );

	if ( status != 0 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
"Cannot set limit switch polarity for motor controller '%s'.  Reason = '%s'",
			record->name, mxi_pcmotion32_strerror( status ) );
	}

	/* Enable the limit switches. */

	enable_limit_switches = (WORD) pcmotion32->enable_limit_switches;

	status = enable_limits( boardID, enable_limit_switches );

	if ( status != 0 ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
"Cannot enable/disable the limit switches for motor controller '%s'.  "
"Reason = '%s'", record->name, mxi_pcmotion32_strerror( status ) );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === Driver specific functions === */

static char mxi_pcmotion32_error_message[][80] = {
"Error 0: Successful (no errors)",
"Error 1: Communications error: Ready to receive timeout",
"Error 2: Command error: Command error on current packet",
"Error 3: No data available in the return data buffer",
"Error 4: Communications error: Return packet is not complete",
"Error 5: Board failure",
"Error 6: Application error: Bad axis number passed to driver",
"Error 7: Application error: Command in process bit stuck on",
"Error 8: Command error: Command error occured prior to current packet",
"Error 9: Command error: Unable to clear command error bit",
"Error 10: Application error: Bad command ID passed to driver",
"Error 11: Communications error: Returned packet ID is corrupted",
"Error 12: Applications error: Bad board ID or board ID not configured",
"Error 13: Applications error: Bad word count passed to 'send_command'",
"Error 14: Applications error: Closed loop command only",
"Error 15: Communications error: Unable to flush the data buffer",
"Error 16: Applications error: Servo command only",
"Error 17: Applications error: Stepper command only",
"Error 18: Applications error: Closed loop stepper command only",
"Error 19: Can not find board configuration file",
"Error 20: Steps/rev or lines/rev not configured for given board/axis",
"Error 21: System reset did not occur within allotted time",
"Error 22: Applications error: 4-axis Stepper command only",
"Error 23: 4-axis command issued to a 3-axis board",
"Error 24: Motion command issued to a pcPosition board",
"Error 25: Return data buffer is not empty",
"Error 26: Reserved",
"Error 27: Process timed out waiting to get the mutex object"
};

static int mxi_pcmotion32_num_error_messages
			= sizeof( mxi_pcmotion32_error_message )
				/ sizeof( mxi_pcmotion32_error_message[0] );

MX_EXPORT char *
mxi_pcmotion32_strerror( int status_code )
{
	static char error_string[40];

	if ( (status_code < 0)
	  || (status_code >= mxi_pcmotion32_num_error_messages) )
	{
		sprintf( error_string,
			"The error status code (%d) is outside the normal "
			"range of error codes (0 to 27).  This should not "
			"be able to happen, but it has." );

		return &error_string[0];
	}

	return &mxi_pcmotion32_error_message[ status_code ][0];
}

#endif /* HAVE_PCMOTION32 */

