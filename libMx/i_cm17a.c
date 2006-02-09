/*
 * Name:    i_cm17a.c
 *
 * Purpose: MX interface driver for X10 Firecracker (CM17A) home automation
 *          controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_CM17A_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_stdint.h"
#include "mx_rs232.h"
#include "i_cm17a.h"

MX_RECORD_FUNCTION_LIST mxi_cm17a_record_function_list = {
	NULL,
	mxi_cm17a_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxi_cm17a_open
};

MX_RECORD_FIELD_DEFAULTS mxi_cm17a_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_CM17A_STANDARD_FIELDS
};

mx_length_type mxi_cm17a_num_record_fields
		= sizeof( mxi_cm17a_record_field_defaults )
			/ sizeof( mxi_cm17a_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_cm17a_rfield_def_ptr
			= &mxi_cm17a_record_field_defaults[0];

MX_EXPORT mx_status_type
mxi_cm17a_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_cm17a_create_record_structures()";

	MX_CM17A *cm17a;

	/* Allocate memory for the necessary structures. */

	cm17a = (MX_CM17A *) malloc( sizeof(MX_CM17A) );

	if ( cm17a == (MX_CM17A *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_CM17A structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = NULL;
	record->record_type_struct = cm17a;

	record->record_function_list = &mxi_cm17a_record_function_list;
	record->superclass_specific_function_list = NULL;
	record->class_specific_function_list = NULL;

	cm17a->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_cm17a_open( MX_RECORD *record )
{
	static const char fname[] = "mxi_cm17a_open()";

	MX_CM17A *cm17a;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	cm17a = (MX_CM17A *) record->record_type_struct;

	if ( cm17a == (MX_CM17A *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_CM17A pointer for record '%s' is NULL.", record->name);
	}

	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name ));

	/* Attempt to put the CM17A controller into standby state. */

	mx_status = mxi_cm17a_standby( cm17a, MXI_CM17A_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_cm17a_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxi_cm17a_resynchronize()";

	MX_CM17A *cm17a;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	cm17a = (MX_CM17A *) record->record_type_struct;

	if ( cm17a == (MX_CM17A *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_CM17A pointer for record '%s' is NULL.", record->name);
	}

	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name ));

	/* Reset the CM17A controller. */

	mx_status = mxi_cm17a_reset( cm17a, MXI_CM17A_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxi_cm17a_reset( MX_CM17A *cm17a,
			int debug_flag )
{
	static const char fname[] = "mxi_cm17a_reset()";

	mx_status_type mx_status;

	if ( cm17a == (MX_CM17A *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_CM17A pointer passed was NULL." );
	}

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: resetting '%s'",
			fname, cm17a->record->name ));
	}

	/* Reset the controller by turning off both the RTS and DTR lines. */

	mx_status = mx_rs232_set_signal_bit( cm17a->rs232_record,
					MXF_232_REQUEST_TO_SEND, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	cm17a->rts_bit = 0;

	mx_status = mx_rs232_set_signal_bit( cm17a->rs232_record,
					MXF_232_DATA_TERMINAL_READY, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	cm17a->dtr_bit = 0;

	/* Wait a moment for the controller to power off. */

	mx_sleep(1);

	/* Turn the controller back on by turning on both RTS and DTR. */

	mx_status = mx_rs232_set_signal_bit( cm17a->rs232_record,
					MXF_232_REQUEST_TO_SEND, 1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	cm17a->rts_bit = 1;

	mx_status = mx_rs232_set_signal_bit( cm17a->rs232_record,
					MXF_232_DATA_TERMINAL_READY, 1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	cm17a->dtr_bit = 1;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_cm17a_standby( MX_CM17A *cm17a,
			int debug_flag )
{
	static const char fname[] = "mxi_cm17a_standby()";

	mx_status_type mx_status;

	if ( cm17a == (MX_CM17A *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_CM17A pointer passed was NULL." );
	}

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: putting '%s' in standby.",
			fname, cm17a->record->name ));
	}

	/* Turn the controller on by turning on both RTS and DTR. */

	mx_status = mx_rs232_set_signal_bit( cm17a->rs232_record,
					MXF_232_REQUEST_TO_SEND, 1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	cm17a->rts_bit = 1;

	mx_status = mx_rs232_set_signal_bit( cm17a->rs232_record,
					MXF_232_DATA_TERMINAL_READY, 1 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	cm17a->dtr_bit = 1;

	return MX_SUCCESSFUL_RESULT;
}

#define MXI_CM17A_NUM_COMMAND_BYTES	5

MX_EXPORT mx_status_type
mxi_cm17a_command( MX_CM17A *cm17a,
			uint16_t command,
			int debug_flag )
{
	static const char fname[] = "mxi_cm17a_command()";

	uint8_t command_array[MXI_CM17A_NUM_COMMAND_BYTES];
	uint8_t bit, mask;
	int i, j;
	mx_status_type mx_status;

	if ( cm17a == (MX_CM17A *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_CM17A pointer passed was NULL." );
	}

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: sending %#x to '%s'",
			fname, command, cm17a->record->name ));
	}

	/* The header and footer are always the same. */

	command_array[0] = 0xD5;
	command_array[1] = 0xAA;

	command_array[4] = 0xAD;

	/* The command passed to us goes in the middle. */

	command_array[2] = (uint8_t) (( command >> 8 ) & 0xff);
	command_array[3] = (uint8_t) (command & 0xff);

	/* Now send the command. */

	for ( i = 0; i < MXI_CM17A_NUM_COMMAND_BYTES; i++ ) {
		/* Send out high order bits before low order bits. */

		for ( j = 0; j < 8; j++ ) {
			mask = 1 << ( 7 - j );

			bit = (command_array[i]) & mask;

			if ( bit ) {
				/* Case '1': Turn on RTS, then turn off DTR. */

				mx_status = mx_rs232_set_signal_bit(
					cm17a->rs232_record,
					MXF_232_REQUEST_TO_SEND, 1 );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

				mx_status = mx_rs232_set_signal_bit(
					cm17a->rs232_record,
					MXF_232_DATA_TERMINAL_READY, 0 );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
			} else {
				/* Case '0': Turn on DTR, then turn off RTS. */

				mx_status = mx_rs232_set_signal_bit(
					cm17a->rs232_record,
					MXF_232_DATA_TERMINAL_READY, 1 );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;

				mx_status = mx_rs232_set_signal_bit(
					cm17a->rs232_record,
					MXF_232_REQUEST_TO_SEND, 0 );

				if ( mx_status.code != MXE_SUCCESS )
					return mx_status;
			}

			/* We must wait for at least 500 microseconds
			 * for the CM17A to notice the change.
			 */

			mx_usleep(500);
		}
	}

	/* Leave the system in standby mode. */

	mx_status = mxi_cm17a_standby( cm17a, debug_flag );

	return mx_status;
}

