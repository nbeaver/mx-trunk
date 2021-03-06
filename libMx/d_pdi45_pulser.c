/*
 * Name:    d_pdi45_pulser.c
 *
 * Purpose: MX pulse generator driver for a single output channel of
 *          the PDI45 controller.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2003, 2006, 2008, 2010, 2015, 2018 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_driver.h"
#include "mx_pulse_generator.h"

#include "i_pdi45.h"
#include "d_pdi45_pulser.h"

MX_RECORD_FUNCTION_LIST mxd_pdi45_pulser_record_function_list = {
	NULL,
	mxd_pdi45_pulser_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_pdi45_pulser_open
};

MX_PULSE_GENERATOR_FUNCTION_LIST
		mxd_pdi45_pulser_pulse_generator_function_list = {
	mxd_pdi45_pulser_is_busy,
	mxd_pdi45_pulser_arm,
	NULL,
	mxd_pdi45_pulser_stop,
	NULL,
	mxd_pdi45_pulser_get_parameter,
	mxd_pdi45_pulser_set_parameter
};

MX_RECORD_FIELD_DEFAULTS mxd_pdi45_pulser_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_PULSE_GENERATOR_STANDARD_FIELDS,
	MXD_PDI45_PULSER_STANDARD_FIELDS
};

long mxd_pdi45_pulser_num_record_fields
		= sizeof( mxd_pdi45_pulser_record_field_defaults )
			/ sizeof( mxd_pdi45_pulser_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_pdi45_pulser_rfield_def_ptr
			= &mxd_pdi45_pulser_record_field_defaults[0];

/* --- */

static mx_status_type
mxd_pdi45_pulser_get_pointers( MX_PULSE_GENERATOR *pulse_generator,
			MX_PDI45_PULSER **pdi45_pulser,
			MX_PDI45 **pdi45,
			const char *calling_fname )
{
	static const char fname[] = "mxd_pdi45_pulser_get_pointers()";

	MX_RECORD *pdi45_pulser_record, *pdi45_record;
	MX_PDI45_PULSER *local_pdi45_pulser;

	if ( pulse_generator == (MX_PULSE_GENERATOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PULSE_GENERATOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	pdi45_pulser_record = pulse_generator->record;

	if ( pdi45_pulser_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for the MX_PULSE_GENERATOR pointer "
		"passed by '%s' is NULL.", calling_fname );
	}

	local_pdi45_pulser = (MX_PDI45_PULSER *)
				pdi45_pulser_record->record_type_struct;

	if ( pdi45_pulser != (MX_PDI45_PULSER **) NULL ) {
		*pdi45_pulser = local_pdi45_pulser;
	}

	if ( pdi45 != (MX_PDI45 **) NULL ) {
		pdi45_record = local_pdi45_pulser->pdi45_record;

		if ( pdi45_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The pdi45_record pointer for 'pdi45_pulser' "
			"record '%s' passed by '%s' is NULL.",
				pdi45_pulser_record->name,
				calling_fname );
		}

		*pdi45 = (MX_PDI45 *) pdi45_record->record_type_struct;

		if ( *pdi45 == (MX_PDI45 *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_PDI45 pointer for pdi45_record '%s' "
			"used by 'pdi45_pulser' record '%s' "
			"passed by '%s' is NULL.",
				pdi45_record->name,
				pdi45_pulser_record->name,
				calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ====== */

MX_EXPORT mx_status_type
mxd_pdi45_pulser_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_pdi45_pulser_create_record_structures()";

	MX_PULSE_GENERATOR *pulse_generator;
	MX_PDI45_PULSER *pdi45_pulser = NULL;

	pulse_generator = (MX_PULSE_GENERATOR *)
				malloc( sizeof(MX_PULSE_GENERATOR) );

	if ( pulse_generator == (MX_PULSE_GENERATOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for a MX_PULSE_GENERATOR structure "
		"for record '%s'.", record->name );
	}

	pdi45_pulser = (MX_PDI45_PULSER *)
				malloc( sizeof(MX_PDI45_PULSER) );

	if ( pdi45_pulser == (MX_PDI45_PULSER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannott allocate memory for a MX_PDI45_PULSER structure"
		"for record '%s'.", record->name );
	}

	record->record_class_struct = pulse_generator;
	record->record_type_struct = pdi45_pulser;

	record->class_specific_function_list =
				&mxd_pdi45_pulser_pulse_generator_function_list;

	pulse_generator->record = record;
	pdi45_pulser->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pdi45_pulser_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_pdi45_pulser_open()";

	MX_PULSE_GENERATOR *pulse_generator;
	MX_PDI45_PULSER *pdi45_pulser = NULL;
	MX_PDI45 *pdi45 = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	pulse_generator = (MX_PULSE_GENERATOR *) record->record_class_struct;

	mx_status = mxd_pdi45_pulser_get_pointers( pulse_generator,
					&pdi45_pulser, &pdi45, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( pdi45_pulser->line_number != 3 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"PDI45 pulser record '%s' is not configured to use digital I/O line 3.",
			record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_pdi45_pulser_is_busy( MX_PULSE_GENERATOR *pulse_generator )
{
	static const char fname[] = "mxd_pdi45_pulser_is_busy()";

	MX_PDI45_PULSER *pdi45_pulser = NULL;
	MX_PDI45 *pdi45 = NULL;
	char command[80];
	char response[80];
	int num_items, pulser_on;
	long hex_value;
	mx_status_type mx_status;

	mx_status = mxd_pdi45_pulser_get_pointers( pulse_generator,
					&pdi45_pulser, &pdi45, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command),
			"00*000%02X", 1 << ( pdi45_pulser->line_number ) );

	mx_status = mxi_pdi45_command( pdi45, command,
					response, sizeof(response) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response + 1, "%1X%4lX", &pulser_on, &hex_value );

	if ( num_items != 2 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unparseable response '%s' to command '%s' for record '%s'.",
			response, command, pulse_generator->record->name );
	}

	MX_DEBUG(-2,("%s: pulser_on = %d, hex_value = %#04lX",
		fname, pulser_on, hex_value ));

	if ( pulser_on ) {
		pulse_generator->busy = TRUE;
	} else {
		pulse_generator->busy = FALSE;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pdi45_pulser_arm( MX_PULSE_GENERATOR *pulse_generator )
{
	static const char fname[] = "mxd_pdi45_pulser_arm()";

	MX_PDI45_PULSER *pdi45_pulser = NULL;
	MX_PDI45 *pdi45 = NULL;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_pdi45_pulser_get_pointers( pulse_generator,
					&pdi45_pulser, &pdi45, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Turn on the pulser. */

	snprintf( command, sizeof(command),
			"00K%02X", 1 << ( pdi45_pulser->line_number ) );

	mx_status = mxi_pdi45_command( pdi45, command, NULL, 0 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pdi45_pulser_stop( MX_PULSE_GENERATOR *pulse_generator )
{
	static const char fname[] = "mxd_pdi45_pulser_stop()";

	MX_PDI45_PULSER *pdi45_pulser = NULL;
	MX_PDI45 *pdi45 = NULL;
	char command[80];
	mx_status_type mx_status;

	mx_status = mxd_pdi45_pulser_get_pointers( pulse_generator,
					&pdi45_pulser, &pdi45, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Turn off the pulser. */

	snprintf( command, sizeof(command),
			"00L%02X", 1 << ( pdi45_pulser->line_number ) );

	mx_status = mxi_pdi45_command( pdi45, command, NULL, 0 );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_pdi45_pulser_get_parameter( MX_PULSE_GENERATOR *pulse_generator )
{
	static const char fname[] = "mxd_pdi45_pulser_get_parameter()";

	MX_PDI45_PULSER *pdi45_pulser = NULL;
	MX_PDI45 *pdi45 = NULL;
	char command[80];
	char response[80];
	int num_items;
	long hex_value;
	mx_status_type mx_status;

	mx_status = mxd_pdi45_pulser_get_pointers( pulse_generator,
					&pdi45_pulser, &pdi45, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,
	("%s invoked for PULSE_GENERATOR '%s', parameter type '%s' (%ld)",
		fname, pulse_generator->record->name,
		mx_get_field_label_string( pulse_generator->record,
					pulse_generator->parameter_type ),
		pulse_generator->parameter_type));

	switch( pulse_generator->parameter_type ) {
	case MXLV_PGN_PULSE_WIDTH:

		snprintf( command, sizeof(command),
			"00*000%02X",
			1 << ( pdi45_pulser->line_number ) );

		mx_status = mxi_pdi45_command( pdi45, command, NULL, 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response + 2, "%4lX", &hex_value );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"Unparseable response '%s' to command '%s' for record '%s'.",
			response, command, pulse_generator->record->name );
		}

		pulse_generator->pulse_period = 2.7e-7 * (double) hex_value;

		break;
	default:
		return mx_pulse_generator_default_get_parameter_handler(
							pulse_generator );
	}
	MX_DEBUG(-2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_pdi45_pulser_set_parameter( MX_PULSE_GENERATOR *pulse_generator )
{
	static const char fname[] = "mxd_pdi45_pulser_set_parameter()";

	MX_PDI45_PULSER *pdi45_pulser = NULL;
	MX_PDI45 *pdi45 = NULL;
	char command[80];
	long hex_value;
	mx_status_type mx_status;

	mx_status = mxd_pdi45_pulser_get_pointers( pulse_generator,
					&pdi45_pulser, &pdi45, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG(-2,
	("%s invoked for PULSE_GENERATOR '%s', parameter type '%s' (%ld)",
		fname, pulse_generator->record->name,
		mx_get_field_label_string( pulse_generator->record,
					pulse_generator->parameter_type ),
		pulse_generator->parameter_type));

	switch( pulse_generator->parameter_type ) {
	case MXLV_PGN_NUM_PULSES:
		break;

	case MXLV_PGN_PULSE_PERIOD:
		break;

	case MXLV_PGN_PULSE_DELAY:
		break;

	case MXLV_PGN_PULSE_WIDTH:
		MX_DEBUG(-2,("%s invoked for %g second pulse width.",
			fname, pulse_generator->pulse_width));

		/* Set the pulse width. */

		hex_value = mx_round( 3703703.7037
				* pulse_generator->pulse_width );

		MX_DEBUG(-2,("%s: pulse width hex_value = %#lx",
			fname, hex_value));

		snprintf( command, sizeof(command),
			"00*100%02X%04lX",
			1 << ( pdi45_pulser->line_number ), hex_value );

		mx_status = mxi_pdi45_command( pdi45, command, NULL, 0 );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		break;
	default:
		return mx_pulse_generator_default_set_parameter_handler(
							pulse_generator );
	}
	MX_DEBUG(-2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

