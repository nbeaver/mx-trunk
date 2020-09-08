/*
 * Name:    d_keithley428.c
 *
 * Purpose: MX driver to control Keithley 428 current amplifiers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2002, 2004, 2006, 2008, 2010, 2015, 2017-2018, 2020
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define KEITHLEY428_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_gpib.h"
#include "mx_amplifier.h"
#include "d_keithley428.h"

/* Initialize the amplifier driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_keithley428_record_function_list = {
	NULL,
	mxd_keithley428_create_record_structures,
	mxd_keithley428_finish_record_initialization,
	NULL,
	NULL,
	mxd_keithley428_open,
	mxd_keithley428_close
};

MX_AMPLIFIER_FUNCTION_LIST mxd_keithley428_amplifier_function_list = {
	mxd_keithley428_get_gain,
	mxd_keithley428_set_gain,
	mxd_keithley428_get_offset,
	mxd_keithley428_set_offset,
	mxd_keithley428_get_time_constant,
	mxd_keithley428_set_time_constant,
	mx_amplifier_default_get_parameter_handler,
	mx_amplifier_default_set_parameter_handler
};

/* Keithley 428 amplifier data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_keithley428_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_AMPLIFIER_STANDARD_FIELDS,
	MXD_KEITHLEY428_STANDARD_FIELDS
};

long mxd_keithley428_num_record_fields
		= sizeof( mxd_keithley428_record_field_defaults )
		  / sizeof( mxd_keithley428_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_keithley428_rfield_def_ptr
			= &mxd_keithley428_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_keithley428_get_pointers( MX_AMPLIFIER *amplifier,
				MX_KEITHLEY428 **keithley428,
				MX_INTERFACE **gpib_interface,
				const char *calling_fname )
{
	static const char fname[] = "mxd_keithley428_get_pointers()";

	if ( amplifier == (MX_AMPLIFIER *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_AMPLIFIER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( keithley428 == (MX_KEITHLEY428 **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_KEITHLEY428 pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( gpib_interface == (MX_INTERFACE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_INTERFACE pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*keithley428 = (MX_KEITHLEY428 *)
				(amplifier->record->record_type_struct);

	if ( *keithley428 == (MX_KEITHLEY428 *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_KEITHLEY428 pointer for record '%s' is NULL.",
			amplifier->record->name );
	}

	*gpib_interface = &((*keithley428)->gpib_interface);

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_keithley428_get_machine_status_word( MX_KEITHLEY428 *keithley428,
			char *buffer, int buffer_length, int debug_flag )
{
	static const char fname[] = "mxd_keithley428_get_machine_status_word()";

	mx_status_type mx_status;

	mx_status = mxd_keithley428_command( keithley428, "U0X",
				buffer, buffer_length, debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( strlen( buffer ) < 14 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Machine status word '%s' returned was truncated.",
			buffer );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_keithley428_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_keithley428_create_record_structures()";

	MX_AMPLIFIER *amplifier;
	MX_KEITHLEY428 *keithley428 = NULL;

	/* Allocate memory for the necessary structures. */

	amplifier = (MX_AMPLIFIER *) malloc( sizeof(MX_AMPLIFIER) );

	if ( amplifier == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_AMPLIFIER structure." );
	}

	keithley428 = (MX_KEITHLEY428 *) malloc( sizeof(MX_KEITHLEY428) );

	if ( keithley428 == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_KEITHLEY428 structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = amplifier;
	record->record_type_struct = keithley428;
	record->class_specific_function_list
			= &mxd_keithley428_amplifier_function_list;

	amplifier->record = record;
	keithley428->record = record;

	/* The gain range for a Keithley model 428 is from 1e3 to 1e10. */

	amplifier->gain_range[0] = 1.0e3;
	amplifier->gain_range[1] = 1.0e10;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_keithley428_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[]
		= "mxd_keithley428_finish_record_initialization()";

	MX_KEITHLEY428 *keithley428 = NULL;
	MX_RECORD *gpib_record;
	long gpib_address;

	/* Verify that the gpib_interface field actually corresponds
	 * to a GPIB interface record.
	 */

	keithley428 = (MX_KEITHLEY428 *)( record->record_type_struct );

	gpib_record = keithley428->gpib_interface.record;

	gpib_address = keithley428->gpib_interface.address;

	if ( gpib_record->mx_superclass != MXR_INTERFACE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"'%s' is not an interface record.", gpib_record->name );
	}
	if ( gpib_record->mx_class != MXI_GPIB ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
		"'%s' is not a GPIB record.", gpib_record->name );
	}

	/* Check that the GPIB address is valid. */

	if ( gpib_address < 0 || gpib_address > 30 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"GPIB address %ld for record '%s' is out of allowed range 0-30.",
			gpib_address, record->name );
	}

	keithley428->bypass_get_gain_or_offset = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_keithley428_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_keithley428_open()";

	MX_AMPLIFIER *amplifier;
	MX_KEITHLEY428 *keithley428 = NULL;
	MX_INTERFACE *interface = NULL;
	MX_GPIB *gpib;
	char command[20];
	unsigned long read_terminator;
	unsigned long flags;
	mx_status_type mx_status;

	amplifier = (MX_AMPLIFIER *) (record->record_class_struct);

	if ( amplifier == (MX_AMPLIFIER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_AMPLIFIER pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mxd_keithley428_get_pointers(amplifier,
			&keithley428, &interface, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	gpib = (MX_GPIB *) interface->record->record_class_struct;

	/* For the Keithley 428, we force on EOI and force off EOS. */

	/* Force EOI signalling on. */

	gpib->eoi_mode[ interface->address ] = 1;

	gpib->read_terminator[ interface->address ] = 0x0a;
	gpib->write_terminator[ interface->address ] = 0x0a;

	/*---*/

	flags = keithley428->keithley_flags;

	mx_status = mx_gpib_open_device(interface->record, interface->address);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0
	/* Send a selective device clear command to the Keithley. */

	mx_status = mx_gpib_selective_device_clear(
				interface->record, interface->address );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;
#endif

	/* Set the ASCII terminator for the Keithley. */

	read_terminator = gpib->read_terminator[ interface->address ];

	switch( read_terminator ) {
	case 0x0d0a:
	case 0x0:
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

	mx_status = mxd_keithley428_command( keithley428, command,
					NULL, 0, KEITHLEY428_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( (flags & MXF_KEITHLEY428_BYPASS_STARTUP_SET) == 0 ) {

		/* Turn zero check off. */

		mx_status = mxd_keithley428_command( keithley428, "C0X",
						NULL, 0, KEITHLEY428_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Set the gain, offset, and time constant. */

		keithley428->bypass_get_gain_or_offset = TRUE;

		mx_status = mxd_keithley428_set_gain( amplifier );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		keithley428->bypass_get_gain_or_offset = FALSE;

		/* Given that bypass_get_gain_or_offset was set to TRUE, the
		 * function mxd_keithley_set_gain() will have set the offset
		 * correctly as a side effect, so we don't need to explicitly
		 * set the offset here.
		 */

		mx_status = mxd_keithley428_set_time_constant( amplifier );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Get the firmware version for this Keithley. */

	mx_status = mxd_keithley428_command( keithley428, "U4X",
				keithley428->firmware_version,
				MXU_KEITHLEY428_FIRMWARE_VERSION_LENGTH,
				KEITHLEY428_DEBUG );
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_keithley428_close( MX_RECORD *record )
{
	static const char fname[] = "mxd_keithley428_close()";

	MX_AMPLIFIER *amplifier;
	MX_KEITHLEY428 *keithley428 = NULL;
	MX_INTERFACE *interface = NULL;
	mx_status_type mx_status;

	amplifier = (MX_AMPLIFIER *) (record->record_class_struct);

	if ( amplifier == (MX_AMPLIFIER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_AMPLIFIER pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mxd_keithley428_get_pointers(amplifier,
			&keithley428, &interface, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_keithley428_get_gain( amplifier );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_keithley428_get_offset( amplifier );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_keithley428_get_time_constant( amplifier );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_gpib_close_device( interface->record,
					interface->address );

	return mx_status;
}

#define GAIN_CHAR_OFFSET	24

MX_EXPORT mx_status_type
mxd_keithley428_get_gain( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_keithley428_get_gain()";

	MX_KEITHLEY428 *keithley428 = NULL;
	MX_INTERFACE *interface = NULL;
	char buffer[50];
	int num_items, gain_setting;
	mx_bool_type fast_mode;
	unsigned long flags;
	mx_status_type mx_status;

	mx_status = mxd_keithley428_get_pointers( amplifier,
			&keithley428, &interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If status checking is bypassed, then return without doing anything.*/

	flags = keithley428->keithley_flags;

	if ( flags & MXF_KEITHLEY428_BYPASS_GET_STATUS ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* Skip communicating with the Keithley if the database
	 * is in fast mode.
	 */

	mx_status = mx_get_fast_mode( amplifier->record, &fast_mode );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( fast_mode ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* Get the Keithley status. */

	mx_status = mxd_keithley428_get_machine_status_word( keithley428,
			buffer, sizeof(buffer) - 1, KEITHLEY428_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( &buffer[GAIN_CHAR_OFFSET], "R%dS", &gain_setting );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Incorrectly formatted machine status word.  Contents = '%s'",
			buffer );
	}

	amplifier->gain = pow( 10.0, (double) gain_setting );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_keithley428_set_gain( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_keithley428_set_gain()";

	MX_KEITHLEY428 *keithley428 = NULL;
	MX_INTERFACE *interface = NULL;
	char command[20];
	double requested_gain, rounded_requested_gain, exponent;
	int gain_setting;
	mx_status_type mx_status;

	mx_status = mxd_keithley428_get_pointers( amplifier,
			&keithley428, &interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Force the gain to the nearest allowed value. */

	requested_gain = amplifier->gain;

	exponent = log10( requested_gain );

	gain_setting = (int) mx_round( exponent );

	rounded_requested_gain = pow( 10.0, (double) gain_setting );

	if ( gain_setting < 3 || gain_setting > 10 ) {

		/* Set amplifier->gain to a legal value before returning. */

		amplifier->gain = amplifier->gain_range[0];

		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"Illegal gain setting %g.  Allowed range is 1.0e3 to 1.0e10. "
"Specify gain multipliers like 1.0e4 rather than gain ranges like 4.",
			rounded_requested_gain );
	}

	amplifier->gain = rounded_requested_gain;

	/* Get the old offset value so we can restore it after setting
	 * the gain.  The old value of the offset is saved in the 
	 * structure element amplifier->offset.
	 */

	if ( keithley428->bypass_get_gain_or_offset == FALSE ) {

		mx_status = mxd_keithley428_get_offset( amplifier );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	/* Set the new gain. */

	snprintf( command, sizeof(command), "R%dX", gain_setting );

	mx_status = mxd_keithley428_command( keithley428, command,
						NULL, 0, KEITHLEY428_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Restore the old offset value. */

	mx_status = mxd_keithley428_set_offset( amplifier );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_keithley428_get_offset( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_keithley428_get_offset()";

	MX_KEITHLEY428 *keithley428 = NULL;
	MX_INTERFACE *interface = NULL;
	char response[50];
	double current_suppress_value;
	int num_items;
	unsigned long flags;
	mx_bool_type fast_mode;
	mx_status_type mx_status;

	mx_status = mxd_keithley428_get_pointers( amplifier,
			&keithley428, &interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If status checking is bypassed, then return without doing anything.*/

	flags = keithley428->keithley_flags;

	if ( flags & MXF_KEITHLEY428_BYPASS_GET_STATUS ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* Skip communicating with the Keithley if the database
	 * is in fast mode.
	 */

	mx_status = mx_get_fast_mode( amplifier->record, &fast_mode );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( fast_mode ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* Get the Keithley current suppress setting. */

	mx_status = mxd_keithley428_command( keithley428, "UX",
			response, sizeof(response) - 1, KEITHLEY428_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( &response[4], "%lg", &current_suppress_value );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Current suppression value not found in response '%s'",
			response );
	}

	mx_status = mxd_keithley428_get_gain( amplifier );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	amplifier->offset = - ( current_suppress_value * amplifier->gain );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_keithley428_set_offset( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_keithley428_set_offset()";

	MX_KEITHLEY428 *keithley428 = NULL;
	MX_INTERFACE *interface = NULL;
	char command[40];
	double current_offset;
	mx_status_type mx_status;

	mx_status = mxd_keithley428_get_pointers( amplifier,
			&keithley428, &interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( keithley428->bypass_get_gain_or_offset == FALSE ) {

		mx_status = mxd_keithley428_get_gain( amplifier );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	current_offset = - ( amplifier->offset / amplifier->gain );

	/* Set the Keithley current suppression range to auto-ranging. */

	mx_status = mxd_keithley428_command( keithley428,
					"S,0X", NULL, 0, KEITHLEY428_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the Keithley current suppression value. */

	snprintf( command, sizeof(command), "S%g,X", current_offset );

	mx_status = mxd_keithley428_command( keithley428, command,
						NULL, 0, KEITHLEY428_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Turn on current suppression. */

	mx_status = mxd_keithley428_command( keithley428,
					"N1X", NULL, 0, KEITHLEY428_DEBUG );

	return mx_status;
}

#define FILTER_ENABLE			22
#define TIME_CONSTANT_CHAR_OFFSET	30

MX_EXPORT mx_status_type
mxd_keithley428_get_time_constant( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_keithley428_get_time_constant()";

	MX_KEITHLEY428 *keithley428 = NULL;
	MX_INTERFACE *interface = NULL;
	char buffer[50];
	int num_items, filter_enabled, rise_time_setting;
	unsigned long flags;
	mx_bool_type fast_mode;
	mx_status_type mx_status;

	mx_status = mxd_keithley428_get_pointers( amplifier,
			&keithley428, &interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If status checking is bypassed, then return without doing anything.*/

	flags = keithley428->keithley_flags;

	if ( flags & MXF_KEITHLEY428_BYPASS_GET_STATUS ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* Skip communicating with the Keithley if the database
	 * is in fast mode.
	 */

	mx_status = mx_get_fast_mode( amplifier->record, &fast_mode );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( fast_mode ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* Get the Keithley status. */

	mx_status = mxd_keithley428_get_machine_status_word( keithley428,
			buffer, sizeof(buffer) - 1, KEITHLEY428_DEBUG );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* See if the filter is enabled. */

	num_items = sscanf( &buffer[FILTER_ENABLE], "P%dR", &filter_enabled );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Incorrectly formatted machine status word.  Contents = '%s'",
			buffer );
	}

	/* If the filter is not enabled, set the reported amplifier time
	 * constant to zero.
	 */

	if ( filter_enabled == 0 ) {
		amplifier->time_constant = 0.0;

		return MX_SUCCESSFUL_RESULT;
	}

	/* Get the filter rise time. */

	num_items = sscanf( &buffer[TIME_CONSTANT_CHAR_OFFSET],
						"T%dW", &rise_time_setting );

	if ( num_items != 1 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Incorrectly formatted machine status word.  Contents = '%s'",
			buffer );
	}

	switch( rise_time_setting ) {
	case 0: amplifier->time_constant = 0.00001;
		break;
	case 1: amplifier->time_constant = 0.00003;
		break;
	case 2: amplifier->time_constant = 0.0001;
		break;
	case 3: amplifier->time_constant = 0.0003;
		break;
	case 4: amplifier->time_constant = 0.001;
		break;
	case 5: amplifier->time_constant = 0.003;
		break;
	case 6: amplifier->time_constant = 0.01;
		break;
	case 7: amplifier->time_constant = 0.03;
		break;
	case 8: amplifier->time_constant = 0.1;
		break;
	case 9: amplifier->time_constant = 0.3;
		break;
	default:
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
	"Illegal rise time setting %d found in machine status word '%s'",
			rise_time_setting, buffer );
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_keithley428_set_time_constant( MX_AMPLIFIER *amplifier )
{
	static const char fname[] = "mxd_keithley428_set_time_constant()";

	MX_KEITHLEY428 *keithley428 = NULL;
	MX_INTERFACE *interface = NULL;
	char command[20];
	double time_constant;
	int enable_filter, rise_time_range;
	mx_status_type mx_status;

	mx_status = mxd_keithley428_get_pointers( amplifier,
			&keithley428, &interface, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Figure out which rise time range to use. */

	enable_filter = TRUE;

	time_constant = amplifier->time_constant;

	if ( mx_difference( time_constant, 0.3 ) < 0.5 ) {
		rise_time_range = 9;
	} else
	if ( mx_difference( time_constant, 0.1 ) < 0.5 ) {
		rise_time_range = 8;
	} else
	if ( mx_difference( time_constant, 0.03 ) < 0.5 ) {
		rise_time_range = 7;
	} else
	if ( mx_difference( time_constant, 0.01 ) < 0.5 ) {
		rise_time_range = 6;
	} else
	if ( mx_difference( time_constant, 0.003 ) < 0.5 ) {
		rise_time_range = 5;
	} else
	if ( mx_difference( time_constant, 0.001 ) < 0.5 ) {
		rise_time_range = 4;
	} else
	if ( mx_difference( time_constant, 0.0003 ) < 0.5 ) {
		rise_time_range = 3;
	} else
	if ( mx_difference( time_constant, 0.0001 ) < 0.5 ) {
		rise_time_range = 2;
	} else
	if ( mx_difference( time_constant, 0.00003 ) < 0.5 ) {
		rise_time_range = 1;
	} else
	if ( mx_difference( time_constant, 0.00001 ) < 0.5 ) {
		rise_time_range = 0;
	} else
	if ( mx_difference( time_constant, 0.0 ) < 0.5 ) {
		rise_time_range = -1;
		enable_filter = FALSE;
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
"Illegal Keithley filter rise time %g.  The legal range is 1e-5 to 0.3 sec "
" or 0 to disable the filter.", time_constant );
	}

	/* Set the rise time range. */

	if ( rise_time_range >= 0 ) {
		snprintf( command, sizeof(command),"T%dX", rise_time_range );

		mx_status = mxd_keithley428_command( keithley428, command,
						NULL, 0, KEITHLEY428_DEBUG );

		switch( mx_status.code ) {
		case MXE_SUCCESS:
			break;

		case MXE_ILLEGAL_ARGUMENT:
			/* If the requested rise time was illegal, then
			 * turn the filter off.
			 */

			enable_filter = FALSE;

			mx_warning("Keithley rise time filter disabled.");

			break;
		default:
			return mx_status;
		}
	}

	/* Enable or disable the filter. */

	if ( enable_filter ) {
		mx_status = mxd_keithley428_command( keithley428, "P1X",
						NULL, 0, KEITHLEY428_DEBUG );
	} else {
		mx_status = mxd_keithley428_command( keithley428, "P0X",
						NULL, 0, KEITHLEY428_DEBUG );
	}
	return mx_status;
}

/* === Driver specific commands === */

MX_EXPORT mx_status_type
mxd_keithley428_command( MX_KEITHLEY428 *keithley428, char *command,
		char *response, int response_buffer_length,
		int debug_flag )
{
	static const char fname[] = "mxd_keithley428_command()";

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
	MX_GPIB *gpib = NULL;

	char local_command_buffer[1024];
#if 0
	size_t local_command_length;
	unsigned long ulong_write_terminator;

	mx_bool_type need_write_terminator;
#endif
	
	int local_response_length = 0;
	char *local_response_ptr = NULL;
	char local_response_buffer[1024];
	unsigned long flags;
	mx_status_type mx_status;

	if ( keithley428 == (MX_KEITHLEY428 *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_KEITHLEY428 pointer passed was NULL." );
	}
	if ( command == NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"'command' buffer pointer passed was NULL.  No command sent.");
	}

	interface = &(keithley428->gpib_interface);

	if ( interface == (MX_INTERFACE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_INTERFACE pointer for Keithley '%s' was NULL.",
			keithley428->record->name );
	}

	gpib = (MX_GPIB *) interface->record->record_class_struct;

	if ( gpib == (MX_GPIB *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_GPIB pointer for Keithley '%s' was NULL.",
			keithley428->record->name );
	}

	flags = keithley428->keithley_flags;

	if ( flags & MXF_KEITHLEY428_DEBUG ) {
		debug_flag = TRUE;
	}

	/* Copy the command string to a local buffer and then add
	 * line terminators as needed.
	 */

	strlcpy(local_command_buffer, command, sizeof(local_command_buffer-3));

#if 0
	need_write_terminator = TRUE;

	if ( need_write_terminator ) {
		local_command_length = strlen( local_command_buffer );

		ulong_write_terminator =
			gpib->write_terminator[ interface->address ];

		switch( ulong_write_terminator ) {
		case 0x0d0a:
			local_command_buffer[local_command_length] = 0x0d;
			local_command_buffer[local_command_length+1] = 0x0a;
			local_command_buffer[local_command_length+2] = 0x0;
			break;
		case 0x0a0d:
			local_command_buffer[local_command_length] = 0x0a;
			local_command_buffer[local_command_length+1] = 0x0d;
			local_command_buffer[local_command_length+2] = 0x0;
			break;
		case 0x0d:
			local_command_buffer[local_command_length] = 0x0d;
			local_command_buffer[local_command_length+1] = 0x0;
			break;
		case 0x0a:
			local_command_buffer[local_command_length] = 0x0a;
			local_command_buffer[local_command_length+1] = 0x0;
			break;
		default:
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal command write terminator %#lx requested "
			"for Keithley 428 '%s'.  The allowed values are "
			"0x0d0a, 0x0a0d, 0x0d, or 0x0a.",
				ulong_write_terminator,
				keithley428->record->name );
			break;
		}
	}
#endif

	/* Send the command string. */

	mx_status = mx_gpib_putline( interface->record, interface->address,
				local_command_buffer, NULL, debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The Keithley 428 always sends a response of some sort, but
	 * some of them we do not want.  If the response pointer is set
	 * to NULL, then we redirect the response we are ignoring to
	 * a local buffer.
	 */

	if ( response == NULL ) {
		local_response_ptr = local_response_buffer;
		local_response_length = sizeof(local_response_buffer);
	} else {
		local_response_ptr = response;
		local_response_length = response_buffer_length;
	}

	/* Get the response. */

	mx_status = mx_gpib_getline(interface->record, interface->address,
		local_response_ptr, local_response_length, NULL, debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Do we skip over checking for errors? */

	if ( flags & MXF_KEITHLEY428_BYPASS_ERROR_CHECK ) {
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

	if ( strcmp( error_status, "42800000000000" ) == 0 ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* If the first three characters of the error_status string are
	 * not "428", then the Keithley is probably not connected or turned on.
	 */

	if ( strncmp( error_status, "428", 3 ) != 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Attempt to contact the Keithley 428 at GPIB address %ld "
			"on interface '%s' failed.  Is it turned on?  "
			"error_status = '%s'",
			interface->address,
			interface->record->name,
			error_status );
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

