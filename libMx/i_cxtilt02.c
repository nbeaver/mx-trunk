/*
 * Name:    i_cxtilt02.c
 *
 * Purpose: MX driver for Crossbow Technology's CXTILT02 series of digital
 *          inclinometers.
 *
 * Author:  William Lavender
 *
 * WARNING: See the comments at the start of the open() routine below for
 *          information about problems with this unit.  (W. Lavender)
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2003-2004, 2006, 2010, 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXI_CXTILT02_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_ascii.h"
#include "mx_rs232.h"
#include "i_cxtilt02.h"

MX_RECORD_FUNCTION_LIST mxi_cxtilt02_record_function_list = {
	NULL,
	mxi_cxtilt02_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxi_cxtilt02_open
};

MX_RECORD_FIELD_DEFAULTS mxi_cxtilt02_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXI_CXTILT02_STANDARD_FIELDS
};

long mxi_cxtilt02_num_record_fields
		= sizeof( mxi_cxtilt02_record_field_defaults )
			/ sizeof( mxi_cxtilt02_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxi_cxtilt02_rfield_def_ptr
			= &mxi_cxtilt02_record_field_defaults[0];

/*==========================*/

MX_EXPORT mx_status_type
mxi_cxtilt02_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxi_cxtilt02_create_record_structures()";

	MX_CXTILT02 *cxtilt02;

	/* Allocate memory for the necessary structures. */

	cxtilt02 = (MX_CXTILT02 *) malloc( sizeof(MX_CXTILT02) );

	if ( cxtilt02 == (MX_CXTILT02 *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
			"Can't allocate memory for MX_CXTILT02 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_type_struct = cxtilt02;

	record->record_class_struct = NULL;
	record->class_specific_function_list = NULL;

	cxtilt02->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_cxtilt02_open( MX_RECORD *record )
{
#if 1
	/* WARNING: The body of this open function is currently disabled.
	 * The reason for this is that sending the 'R' and 'N' command to
	 * MR-CAT's tilt meter seemed to cause the meter to stop working
	 * correctly.  However, reading the angle with the 'G' command
	 * _did_ seem to work, so the entire body of the open routine
	 * is currently ifdef-ed out.
	 *
	 * The downside of this is that you are stuck with the power-on
	 * resolution of the tilt meter.  The upside is that the tilt
	 * meter works at all.
	 *
	 * (William Lavender)
	 */

	return MX_SUCCESSFUL_RESULT;

#else /* DISABLED CODE. */

	static const char fname[] = "mxi_cxtilt02_open()";

	MX_CXTILT02 *cxtilt02;
	char command[20];
	char response[20];
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL.");
	}

	MX_DEBUG(-2, ("%s invoked for record '%s'.", fname, record->name ));

	cxtilt02 = (MX_CXTILT02 *) (record->record_type_struct);

	if ( cxtilt02 == (MX_CXTILT02 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_CXTILT02 pointer for record '%s' is NULL.", record->name);
	}

	/* Reset the CXTILT02 firmware to its default settings. */

	mx_status = mxi_cxtilt02_command( cxtilt02, "R", response, 1,
						10.0, MXI_CXTILT02_DEBUG );

#if MXI_CXTILT02_DEBUG
	MX_DEBUG(-2,("%s: 'R' command mx_status.code = %ld",
		fname, mx_status.code));
#endif

	if ( ( mx_status.code != MXE_SUCCESS )
	  && ( mx_status.code != MXE_TIMED_OUT ) )
		return mx_status;

	/* The response to this should be an 'H' character. */

	if ( ( response[0] != 'H' ) && ( response[0] != MX_CTRL_H ) ) {
		(void) mx_error( MXE_INTERFACE_IO_ERROR, fname,
	"Did not see the expected 'H' response to the CXTILT02 firmware "
	"reset command 'R' for inclinometer '%s'.  response[0] = %#x (%c).",
			record->name, response[0], response[0] );
#if MXI_CXTILT02_DEBUG
	} else {
		MX_DEBUG(-2,("%s: 'H' response seen from '%s' for 'R' command.",
			fname, record->name));
#endif
	}

	/* Set the resolution level. */

	if ( ( cxtilt02->resolution_level < 0 )
	  || ( cxtilt02->resolution_level > 9 ) )
	{
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The requested resolution level %ld for "
			"CXTILT02 inclinometer '%s' is outside the allowed "
			"range of 0 to 9.", cxtilt02->resolution_level,
			record->name );
	}

	snprintf( command, sizeof(command), "N%ld", cxtilt02->resolution_level);

	mx_status = mxi_cxtilt02_command( cxtilt02, command, NULL, 0,
						1.0, MXI_CXTILT02_DEBUG );

	return mx_status;

#endif /* DISABLED CODE. */
}

MX_EXPORT mx_status_type
mxi_cxtilt02_command( MX_CXTILT02 *cxtilt02,
			char *command,
			char *response,
			size_t num_bytes_expected,
			double timeout_in_seconds,
			int debug_flag )
{
	static const char fname[] = "mxi_cxtilt02_command()";

	size_t command_length, num_bytes_read;
	int i;
	mx_status_type mx_status;

	if ( cxtilt02 == (MX_CXTILT02 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"NULL MX_CXTILT02 pointer passed." );
	}
	if ( command == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"NULL command buffer pointer passed." );
	}

	if ( debug_flag ) {
		MX_DEBUG(-2, ("%s: sending '%s' to '%s'.",
			fname, command, cxtilt02->record->name));
	}

	/* Discard any existing characters in the input buffer. */

	mx_status = mx_rs232_discard_unread_input( cxtilt02->rs232_record,
							debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Send the command string. */

	command_length = strlen( command );

	for ( i = 0; i < command_length; i++ ) {
		mx_status = mx_rs232_putchar( cxtilt02->rs232_record,
						command[i], 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( debug_flag ) {
			MX_DEBUG(-2,("%s: sent command[%d] = %#x (%c)", fname,
				i, (unsigned int) command[i], command[i]));
		}
	}

	/* Read the response, if any. */

	num_bytes_read = 0;

	if ( response != NULL ) {
		for ( i = 0; i < num_bytes_expected; i++ ) {

			mx_status = mx_rs232_getchar_with_timeout(
						cxtilt02->rs232_record,
						&(response[i]), 0,
						timeout_in_seconds );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			if ( debug_flag ) {
				MX_DEBUG(-2,
			("%s: received from '%s', response[%d] = %#x (%c)",
				fname, cxtilt02->record->name,
				i, (unsigned int) response[i], response[i]));
			}

			num_bytes_read++;
		}

		if ( debug_flag ) {
			MX_DEBUG(-2,("%s: num_bytes_read = %ld",
				fname, (long)num_bytes_read));
		}

		if ( num_bytes_read != num_bytes_expected ) {
			return mx_error( MXE_INTERFACE_IO_ERROR, fname,
		"Fewer bytes (%ld) were read from CXTILT02 controller '%s' "
		"in response to command '%s' than were expected (%ld).",
				(long) num_bytes_read, cxtilt02->record->name,
				command, (long) num_bytes_expected );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxi_cxtilt02_read_angles( MX_CXTILT02 *cxtilt02 )
{
	static const char fname[] = "mxi_cxtilt02_read_angles()";

	char response[20];
	unsigned char header, checksum, local_checksum;
	unsigned char pitch_msb, pitch_lsb, roll_msb, roll_lsb;
	mx_status_type mx_status;

	if ( cxtilt02 == (MX_CXTILT02 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"NULL MX_CXTILT02 pointer passed." );
	}

	/* Send the Get Angle Data Packet command. */

	mx_status = mxi_cxtilt02_command( cxtilt02, "G", response, 6,
						5.0, MXI_CXTILT02_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	header    = response[0];
	pitch_msb = response[1];
	pitch_lsb = response[2];
	roll_msb  = response[3];
	roll_lsb  = response[4];
	checksum  = response[5];

#if MXI_CXTILT02_DEBUG		
	MX_DEBUG(-2,("%s: header = %#x", fname, (unsigned int) header));
	MX_DEBUG(-2,("%s: pitch_msb = %#x, pitch_lsb = %#x", fname,
		(unsigned int) pitch_msb, (unsigned int) pitch_lsb));
	MX_DEBUG(-2,("%s: roll_msb = %#x, roll_lsb = %#x", fname,
		(unsigned int) roll_msb, (unsigned int) roll_lsb));
#endif

	if ( header != 0xff ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"Did not see expected header byte 0xff in the response "
			"to the command 'G' sent to inclinometer '%s'.  "
			"Response = %#x %#x %#x %#x %#x %#x",
				cxtilt02->record->name,
				(unsigned int) header,
				(unsigned int) pitch_msb,
				(unsigned int) pitch_lsb,
				(unsigned int) roll_msb,
				(unsigned int) roll_lsb,
				(unsigned int) checksum);
	}

	local_checksum = 0xff & ( pitch_msb + pitch_lsb + roll_msb + roll_lsb );

#if MXI_CXTILT02_DEBUG		
	MX_DEBUG(-2,("%s: checksum = %#x, local_checksum = %#x", fname,
		(unsigned int) checksum, (unsigned int) local_checksum));
#endif

	if ( local_checksum != checksum ) {
		return mx_error( MXE_INTERFACE_IO_ERROR, fname,
			"The computed and received checksums for the "
			"response to the command 'G' by inclinometer '%s' "
			"do not match.  Computed checksum = %#x, "
			"received_checksum = %#x.  "
			"Response = %#x %#x %#x %#x %#x %#x",
				cxtilt02->record->name,
				(unsigned int) local_checksum,
				(unsigned int) checksum,
				(unsigned int) header,
				(unsigned int) pitch_msb,
				(unsigned int) pitch_lsb,
				(unsigned int) roll_msb,
				(unsigned int) roll_lsb,
				(unsigned int) checksum);
	}

	cxtilt02->raw_pitch = 0xffff & ( (short) pitch_lsb
					+ 256 * (short) pitch_msb );

	cxtilt02->raw_roll = 0xffff & ( (short) roll_lsb
					+ 256 * (short) roll_msb );

#if MXI_CXTILT02_DEBUG		
	MX_DEBUG(-2,("%s: raw_pitch = %#x, raw_roll = %#x", fname,
		(unsigned int) cxtilt02->raw_pitch,
		(unsigned int) cxtilt02->raw_roll));
#endif

	return MX_SUCCESSFUL_RESULT;
}

