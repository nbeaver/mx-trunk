/*
 * Name:    d_icplus.c
 *
 * Purpose: MX interface driver for the Oxford Danfysik IC PLUS
 *          and QBPM intensity monitors.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2002-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_ICPLUS_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_ascii.h"
#include "mx_amplifier.h"
#include "mx_rs232.h"

#include "d_icplus.h"

MX_RECORD_FUNCTION_LIST mxd_icplus_record_function_list = {
	NULL,
	mxd_icplus_create_record_structures,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mxd_icplus_open,
	NULL,
	NULL,
	mxd_icplus_resynchronize
};

MX_AMPLIFIER_FUNCTION_LIST mxd_icplus_amplifier_function_list = {
	mxd_icplus_get_gain,
	mxd_icplus_set_gain,
	mxd_icplus_get_offset,
	mxd_icplus_set_offset
};

MX_RECORD_FIELD_DEFAULTS mxd_icplus_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AMPLIFIER_STANDARD_FIELDS,
	MXD_ICPLUS_STANDARD_FIELDS
};

long mxd_icplus_num_record_fields
		= sizeof( mxd_icplus_record_field_defaults )
			/ sizeof( mxd_icplus_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_icplus_rfield_def_ptr
			= &mxd_icplus_record_field_defaults[0];

MX_RECORD_FIELD_DEFAULTS mxd_qbpm_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AMPLIFIER_STANDARD_FIELDS,
	MXD_QBPM_STANDARD_FIELDS
};

long mxd_qbpm_num_record_fields
		= sizeof( mxd_qbpm_record_field_defaults )
			/ sizeof( mxd_qbpm_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_qbpm_rfield_def_ptr
			= &mxd_qbpm_record_field_defaults[0];

/* --- */

static mx_status_type
mxd_icplus_get_pointers( MX_RECORD *icplus_record,
			MX_AMPLIFIER **amplifier,
			MX_ICPLUS **icplus,
			const char *calling_fname )
{
	static const char fname[] = "mxd_icplus_get_pointers()";

	if ( icplus_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The RECORD pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( amplifier != (MX_AMPLIFIER **) NULL ) {
		*amplifier = (MX_AMPLIFIER *)
				icplus_record->record_class_struct;

		if ( *amplifier == (MX_AMPLIFIER *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_AMPLIFIER pointer for record '%s' passed by '%s' is NULL.",
				icplus_record->name, calling_fname );
		}
	}

	if ( icplus != (MX_ICPLUS **) NULL ) {
		*icplus = (MX_ICPLUS *) icplus_record->record_type_struct;

		if ( *icplus == (MX_ICPLUS *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX_ICPLUS pointer for record '%s' passed by '%s' is NULL.",
				icplus_record->name, calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ====== */

MX_EXPORT mx_status_type
mxd_icplus_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_icplus_create_record_structures()";

	MX_AMPLIFIER *amplifier;
	MX_ICPLUS *icplus;

	amplifier = (MX_AMPLIFIER *) malloc( sizeof(MX_AMPLIFIER) );

	if ( amplifier == (MX_AMPLIFIER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_AMPLIFIER structure.");
	}

	icplus = (MX_ICPLUS *) malloc( sizeof(MX_ICPLUS) );

	if ( icplus == (MX_ICPLUS *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_ICPLUS structure." );
	}

	record->record_class_struct = amplifier;
	record->record_type_struct = icplus;

	record->class_specific_function_list
			= &mxd_icplus_amplifier_function_list;

	amplifier->record = record;
	icplus->record = record;

	icplus->discard_echoed_command_line = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_icplus_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_icplus_open()";

	MX_AMPLIFIER *amplifier;
	MX_ICPLUS *icplus;
	MX_RS232 *rs232;
	char command[40];
	char response[80];
	int timed_out;
	unsigned long i, max_attempts, wait_ms, num_input_bytes_available;
	mx_status_type mx_status;

	mx_status = mxd_icplus_get_pointers( record,
					&amplifier, &icplus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_get_pointers( icplus->rs232_record,
						&rs232, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s'.", fname, record->name));

	/* The ICPLUS driver does not use the QBPM flags. */

	if ( record->mx_type == MXT_AMP_ICPLUS ) {
		icplus->qbpm_flags = 0;
	}

	/* See if the serial port is configured correctly. */

	if( record->mx_type == MXT_AMP_ICPLUS ) {
		mx_status = mx_rs232_verify_configuration( icplus->rs232_record,
					9600, 8, 'N', 1, 'N', 0x0a, 0x0a );
	} else {
		mx_status = mx_rs232_verify_configuration( icplus->rs232_record,
					19200, 8, 'N', 1, 'N', 0x0a, 0x0a );
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Throw away any leftover characters. */

	mx_status = mx_rs232_discard_unwritten_output( icplus->rs232_record,
							MXD_ICPLUS_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_discard_unread_input( icplus->rs232_record,
							MXD_ICPLUS_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If the RS-232 port does not have a timeout specified, set
	 * the timeout to 1 second.
	 */

	if ( rs232->timeout < 0.0 ) {
		rs232->timeout = 1.0;

		MX_DEBUG( 2,("%s: forcing the timeout to 1 second.", fname));
	}

	/* See if the IC PLUS is available by trying to read
	 * the input current.
	 */

	if ( icplus->record->mx_type == MXT_AMP_ICPLUS ) {
		sprintf( command, ":READ%d:CURR?", icplus->address );
	} else {
		sprintf( command, ":READ%d:CURR1?", icplus->address );
	}

	wait_ms = 100;
	max_attempts = 5;
	timed_out = FALSE;

	for ( i = 0; i < max_attempts; i++ ) {
		mx_status = mxd_icplus_command( icplus, command,
					response, sizeof response,
					MXD_ICPLUS_DEBUG );

		switch( mx_status.code ) {
		case MXE_SUCCESS:
			timed_out = FALSE;
			break;
		case MXE_NOT_READY:
		case MXE_TIMED_OUT:
			timed_out = TRUE;
			break;
		default:
			return mx_status;
			break;
		}

		if ( timed_out == FALSE )
			break;			/* Exit the for() loop. */

		/* Resynchronize the serial port.  This will cause
		 * the serial port to be closed and then reopened.
		 */

#if MXD_ICPLUS_DEBUG
		MX_DEBUG(-2,("%s: resynchronizing the serial port.", fname));
#endif

		mx_status = mx_resynchronize_record( icplus->rs232_record );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_msleep( wait_ms );
	}

	/* If there are still characters available from the RS-232 port,
	 * then the serial port is echoing back part of the transmitted
	 * command.  This means that the RS-232 cable is incorrectly
	 * wired, but we will attempt to continue anyway.
	 */

	mx_status = mx_rs232_num_input_bytes_available( icplus->rs232_record,
						&num_input_bytes_available );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( num_input_bytes_available > 0 ) {
		icplus->discard_echoed_command_line = TRUE;

		(void) mx_rs232_discard_unread_input( icplus->rs232_record,
						      	FALSE );

		mx_warning(
	"Some or all of the command string transmitted to '%s' device '%s' "
	"was echoed back to the serial port.  This means that the RS-232 "
	"cable is incorrectly wired, but we will attempt to continue by "
	"discarding the echoed characters.  However, this slows down the "
	"driver, so it would be better to fix the wiring.",
			mx_get_driver_name( icplus->record ), record->name );

	}

	/* Set the gain, offset, and peaking time. */

	mx_status = mx_amplifier_set_gain( record, amplifier->gain );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_amplifier_set_offset( record, amplifier->offset );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_amplifier_set_time_constant( record,
						amplifier->time_constant );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If this is a QBPM controller, set the initial averaging. */

	if ( record->mx_type == MXT_AMP_QBPM ) {
		if ( icplus->default_averaging > 100 ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested averaging size of %d for record '%s' is "
		"outside the allowed range of 1 to 100.",
				icplus->default_averaging, record->name );
		} else
		if ( icplus->default_averaging >= 1 ) {
			sprintf( command, ":READ%d:AVGCURR %d",
				icplus->address, icplus->default_averaging );
		} else
		if ( icplus->default_averaging > -1 ) {
			sprintf( command, ":READ%d:SINGLE",
				icplus->address );
		} else
		if ( icplus->default_averaging >= -100 ) {
			sprintf( command, ":READ%d:WDWCURR %d",
				icplus->address, -(icplus->default_averaging) );
		} else {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested moving average size of %d for record '%s' is "
		"outside the allowed range of -1 to -100.",
				icplus->default_averaging, record->name );
		}

		mx_status = mxd_icplus_command( icplus, command, NULL, 0,
							MXD_ICPLUS_DEBUG );
	}

	MX_DEBUG( 2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_icplus_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_icplus_resynchronize()";

	MX_ICPLUS *icplus;
	char command[40];
	mx_status_type mx_status;

	mx_status = mxd_icplus_get_pointers( record, NULL, &icplus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s'.", fname, record->name));

	mx_status = mx_rs232_discard_unwritten_output( icplus->rs232_record,
							MXD_ICPLUS_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_rs232_discard_unread_input( icplus->rs232_record,
							MXD_ICPLUS_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Reset the IC PLUS. */

	sprintf( command, "*RST%d", icplus->address );

	mx_status = mxd_icplus_command( icplus, command,
					NULL, 0, MXD_ICPLUS_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_icplus_get_gain( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_icplus_get_gain()";

	MX_ICPLUS *icplus;
	char command[40];
	char response[40];
	int exponent, num_items;
	mx_status_type mx_status;

	if ( amplifier == ( MX_AMPLIFIER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AMPLIFIER pointer passed is NULL." );
	}

	mx_status = mxd_icplus_get_pointers( amplifier->record,
						NULL, &icplus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, ":CONF%d:CURR:RANG?", icplus->address );

	mx_status = mxd_icplus_command( icplus, command,
					response, sizeof response,
					MXD_ICPLUS_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%d", &(icplus->range) );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unrecognizable response to a Get Input Range command '%s' "
		"by %s record '%s'.  Response = '%s'.",
			command, mx_get_driver_name( icplus->record ),
			amplifier->record->name, response );
	}

	if ( (icplus->qbpm_flags & MXF_QBPM_USE_NEW_GAINS) == 0 ) {
		/* Use power of 10 gains. */

		exponent = 11 - icplus->range;

		amplifier->gain = pow( 10.0, (double) exponent );
	} else {
		/* Use new irregular gain pattern. */

		switch( icplus->range ) {
		case 1:
			amplifier->gain = 1.4e9;
			break;
		case 2:
			amplifier->gain = 7.0e8;
			break;
		case 3:
			amplifier->gain = 3.5e8;
			break;
		case 4:
			amplifier->gain = 7.0e7;
			break;
		case 5:
			amplifier->gain = 7.0e6;
			break;
		case 6:
			amplifier->gain = 7.0e5;
			break;
		default:
			amplifier->gain = -1.0;
			break;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_icplus_set_gain( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_icplus_set_gain()";

	MX_ICPLUS *icplus;
	char command[40];
	int exponent;
	mx_status_type mx_status;

	if ( amplifier == ( MX_AMPLIFIER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AMPLIFIER pointer passed is NULL." );
	}

	mx_status = mxd_icplus_get_pointers( amplifier->record,
						NULL, &icplus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( (icplus->qbpm_flags & MXF_QBPM_USE_NEW_GAINS) == 0 ) {
		/* Use power of 10 gains. */

		exponent = (int) mx_round( log10( amplifier->gain ) );

		if ( ( exponent < 5 ) || ( exponent > 10 ) ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Requested gain %g for amplifier '%s' is outside the "
			"allowed range of 1.0e5 to 1.0e10 volts/ampere.",
				amplifier->gain, amplifier->record->name );
		}

		icplus->range = 11 - exponent;
	} else {
		/* Use new irregular gain pattern. */

		if ( amplifier->gain >= 1.0e9 ) {
			icplus->range = 1;
		} else
		if ( amplifier->gain >= 5.0e8 ) {
			icplus->range = 2;
		} else
		if ( amplifier->gain >= 1.6e8 ) {
			icplus->range = 3;
		} else
		if ( amplifier->gain >= 2.2e7 ) {
			icplus->range = 4;
		} else
		if ( amplifier->gain >= 2.2e6 ) {
			icplus->range = 5;
		} else {
			icplus->range = 6;
		}
	}

	sprintf( command, ":CONF%d:CURR:RANG %d",
				icplus->address, icplus->range );

	mx_status = mxd_icplus_command( icplus, command,
					NULL, 0, MXD_ICPLUS_DEBUG );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_icplus_get_offset( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_icplus_get_offset()";

	MX_ICPLUS *icplus;
	char command[40];
	char response[40];
	int num_items, offset_percentage;
	mx_status_type mx_status;

	if ( amplifier == ( MX_AMPLIFIER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AMPLIFIER pointer passed is NULL." );
	}

	mx_status = mxd_icplus_get_pointers( amplifier->record,
						NULL, &icplus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sprintf( command, ":CONF%d:CURR:OFFS?", icplus->address );

	mx_status = mxd_icplus_command( icplus, command,
					response, sizeof response,
					MXD_ICPLUS_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "%d", &offset_percentage );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unrecognizable response to a Get Input Offset command '%s' "
		"by %s '%s'.  Response = '%s'.",
			command, mx_get_driver_name( icplus->record ),
			amplifier->record->name, response );
	}

	amplifier->offset = 0.1 * (double) offset_percentage;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_icplus_set_offset( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_icplus_set_offset()";

	MX_ICPLUS *icplus;
	char command[40];
	long offset_percentage;
	mx_status_type mx_status;

	if ( amplifier == ( MX_AMPLIFIER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AMPLIFIER pointer passed is NULL." );
	}

	mx_status = mxd_icplus_get_pointers( amplifier->record,
						NULL, &icplus, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	offset_percentage = mx_round( 10.0 * amplifier->offset );

	if ( ( offset_percentage < 0 ) || ( offset_percentage > 99 ) ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Requested amplifier offset %g for amplifier '%s' is outside "
		"the allowed range of 0 to 9.9 volts.",
			amplifier->offset, amplifier->record->name );
	}

	sprintf( command, ":CONF%d:CURR:OFFS %ld",
				icplus->address, offset_percentage );

	mx_status = mxd_icplus_command( icplus, command,
					NULL, 0, MXD_ICPLUS_DEBUG );

	return mx_status;
}

/* === Extra functions for the use of this driver. === */

MX_EXPORT mx_status_type
mxd_icplus_command( MX_ICPLUS *icplus,
			char *command,
			char *response,
			size_t response_buffer_length,
			int debug_flag )
{
	static const char fname[] = "mxd_icplus_command()";

	char c;
	int i, max_attempts;
	unsigned long sleep_ms, num_input_bytes_available;
	mx_status_type mx_status, mx_status2;

	if ( icplus == (MX_ICPLUS *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"NULL MX_ICPLUS pointer passed." );
	}
	if ( command == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"NULL command buffer pointer passed." );
	}

	if ( debug_flag ) {
		MX_DEBUG(-2,("%s: sending '%s' to '%s'",
				fname, command, icplus->record->name ));
	}

	/* Send the command string. */

	mx_status = mx_rs232_putline( icplus->rs232_record, command, NULL, 0 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The IC PLUS always sends an ACK character to acknowledge receipt
	 * of the LF terminator for the command that we just sent.  Even if
	 * we expect no other response, we must still read and discard this
	 * ACK character.
	 */

	mx_status = mx_rs232_getchar( icplus->rs232_record, &c, MXF_232_WAIT );

	if ( mx_status.code == MXE_NOT_READY ) {
		return mx_error( MXE_NOT_READY, fname,
		"No response received from %s amplifier '%s' "
		"for command '%s'.  Are you sure it is plugged in "
		"and turned on?",
			mx_get_driver_name( icplus->record ),
			icplus->record->name, command );
	}

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( c != MX_ACK ) {
		(void) mx_rs232_discard_unread_input( icplus->rs232_record,
							MXD_ICPLUS_DEBUG );

		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Did not receive an ACK acknowledgement character from "
	"%s interface '%s' in response to the command '%s'.  "
	"Instead, saw a %#x (%c) character.",
			mx_get_driver_name( icplus->record ),
			icplus->record->name, command, c, c );
	}

	/* If we expect a response, then read it in. */

	if ( response != NULL ) {
		mx_status = mx_rs232_getline( icplus->rs232_record,
					response, response_buffer_length,
					NULL, 0 );

		if ( debug_flag & (mx_status.code == MXE_SUCCESS) ) {
			MX_DEBUG(-2,("%s: received '%s' from '%s'",
				fname, response, icplus->record->name ));
		}
	} else {
		if ( debug_flag ) {
			MX_DEBUG(-2,("%s complete.", fname));
		}
	}

	/* If the IC PLUS echoes the command line back to us, then we must
	 * throw this away.
	 */

	if ( icplus->discard_echoed_command_line ) {
		max_attempts = 100;
		sleep_ms = 1;

		for ( i = 0; i < max_attempts; i++ ) {
			mx_status2 = mx_rs232_num_input_bytes_available(
						icplus->rs232_record,
						&num_input_bytes_available );

			if ( mx_status2.code != MXE_SUCCESS )
				break;

			if ( num_input_bytes_available > 0 )
				break;

			mx_msleep( sleep_ms );
		}

		if ( i >= max_attempts ) {
			mx_status = mx_error( MXE_TIMED_OUT, fname,
				"Timed out waiting for %s record '%s' to echo "
				"the command line '%s' back to us.",
					mx_get_driver_name( icplus->record ),
					icplus->record->name, command );
		}
		(void) mx_rs232_discard_unread_input(
					icplus->rs232_record, FALSE );
	}

	return mx_status;
}

