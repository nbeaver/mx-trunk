/*
 * Name:    d_umx_pulser.c
 *
 * Purpose: MX pulse generator driver for UMX-based microcontrollers.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2019-2021, 2023 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_UMX_PULSER_DEBUG			FALSE

#define MXD_UMX_PULSER_DEBUG_CONF		FALSE

#define MXD_UMX_PULSER_DEBUG_RUNNING		FALSE

#define MXD_UMX_PULSER_DEBUG_SETUP		FALSE

#define MXD_UMX_PULSER_DEBUG_LAST_PULSE_NUMBER	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_ascii.h"
#include "mx_cfn.h"
#include "mx_rs232.h"
#include "mx_pulse_generator.h"
#include "mx_umx.h"
#include "d_umx_pulser.h"

/* Initialize the pulse generator driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_umx_pulser_record_function_list = {
	NULL,
	mxd_umx_pulser_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_umx_pulser_open
};

MX_PULSE_GENERATOR_FUNCTION_LIST mxd_umx_pulser_pulser_function_list = {
	mxd_umx_pulser_is_busy,
	mxd_umx_pulser_arm,
	mxd_umx_pulser_trigger,
	mxd_umx_pulser_stop,
	NULL,
	mxd_umx_pulser_get_parameter,
	mxd_umx_pulser_set_parameter,
	mxd_umx_pulser_setup,
	mxd_umx_pulser_get_status
};

/* MX digital output pulser data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_umx_pulser_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_PULSE_GENERATOR_STANDARD_FIELDS,
	MXD_UMX_PULSER_STANDARD_FIELDS
};

long mxd_umx_pulser_num_record_fields
		= sizeof( mxd_umx_pulser_record_field_defaults )
		  / sizeof( mxd_umx_pulser_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_umx_pulser_rfield_def_ptr
			= &mxd_umx_pulser_record_field_defaults[0];

/*=======================================================================*/

static mx_status_type
mxd_umx_pulser_get_pointers( MX_PULSE_GENERATOR *pulser,
			MX_UMX_PULSER **umx_pulser,
			MX_RECORD **umx_record,
			const char *calling_fname )
{
	static const char fname[] = "mxd_umx_pulser_get_pointers()";

	MX_UMX_PULSER *umx_pulser_ptr = NULL;
	MX_RECORD *umx_record_ptr = NULL;

	if ( pulser == (MX_PULSE_GENERATOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PULSE_GENERATOR pointer passed by '%s' was NULL",
			calling_fname );
	}

	if ( pulser->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"MX_RECORD pointer for timer pointer passed by '%s' is NULL.",
			calling_fname );
	}

	umx_pulser_ptr = (MX_UMX_PULSER *) pulser->record->record_type_struct;

	if ( umx_pulser_ptr == (MX_UMX_PULSER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_UMX_PULSER pointer for pulse generator "
		"record '%s' passed by '%s' is NULL",
			pulser->record->name, calling_fname );
	}

	if ( umx_pulser != (MX_UMX_PULSER **) NULL ) {
		*umx_pulser = umx_pulser_ptr;
	}

	umx_record_ptr = umx_pulser_ptr->umx_record;

	if ( umx_record_ptr == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The umx_record pointer for pulser '%s' is NULL.",
			pulser->record->name );
	}

	if ( umx_record != (MX_RECORD **) NULL ) {
		*umx_record = umx_record_ptr;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_umx_pulser_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_umx_pulser_create_record_structures()";

	MX_PULSE_GENERATOR *pulser;
	MX_UMX_PULSER *umx_pulser;

	/* Allocate memory for the necessary structures. */

	pulser = (MX_PULSE_GENERATOR *) malloc( sizeof(MX_PULSE_GENERATOR) );

	if ( pulser == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_PULSE_GENERATOR structure." );
	}

	umx_pulser = (MX_UMX_PULSER *)
				malloc( sizeof(MX_UMX_PULSER) );

	if ( umx_pulser == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_UMX_PULSER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = pulser;
	record->record_type_struct = umx_pulser;
	record->class_specific_function_list
			= &mxd_umx_pulser_pulser_function_list;

	pulser->record = record;
	umx_pulser->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_umx_pulser_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_umx_pulser_open()";

	MX_PULSE_GENERATOR *pulser;
	MX_UMX_PULSER *umx_pulser = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	pulser = (MX_PULSE_GENERATOR *) record->record_class_struct;

	mx_status = mxd_umx_pulser_get_pointers( pulser,
						&umx_pulser, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	pulser->trigger_mode = MXF_DEV_INTERNAL_TRIGGER;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_umx_pulser_is_busy( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_umx_pulser_is_busy()";

	MX_UMX_PULSER *umx_pulser = NULL;
	mx_status_type mx_status;

	mx_status = mxd_umx_pulser_get_pointers( pulser,
						&umx_pulser, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Get the number of pulses generated so far and the total number of
	 * pulses expected to be generated.
	 */

	mx_status = mx_pulse_generator_get_num_pulses( pulser->record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_pulse_generator_get_last_pulse_number( pulser->record,
									NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_UMX_PULSER_DEBUG_RUNNING
	MX_DEBUG(-2,("%s: num_pulses = %lu, last_pulse_number = %ld",
		fname, pulser->num_pulses, pulser->last_pulse_number));
#endif

	if ( pulser->last_pulse_number < 0 ) {
		pulser->busy = FALSE;
	} else
	if ( pulser->last_pulse_number < pulser->num_pulses ) {
		pulser->busy = TRUE;
	} else {
		pulser->busy = FALSE;
	}

#if MXD_UMX_PULSER_DEBUG_RUNNING
	MX_DEBUG(-2,("%s: pulser '%s', busy = %d",
		fname, pulser->record->name, (int) pulser->busy));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_umx_pulser_arm( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_umx_pulser_arm()";

	MX_UMX_PULSER *umx_pulser = NULL;
	MX_RECORD *umx_record = NULL;
	char command[80];
	char response[200];
	mx_bool_type debug_flag;
	mx_status_type mx_status;

	mx_status = mxd_umx_pulser_get_pointers( pulser,
					&umx_pulser, &umx_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	debug_flag = FALSE;

#if MXD_UMX_PULSER_DEBUG_RUNNING
	MX_DEBUG(-2,("%s: Pulse generator '%s' armed, "
		"pulse_width = %f, pulse_period = %f, num_pulses = %ld",
			fname, pulser->record->name,
			pulser->pulse_width,
			pulser->pulse_period,
			pulser->num_pulses));
#endif
	pulser->busy = FALSE;

	if ( pulser->pulse_width < 0.0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Pulser '%s' is configured for a negative pulse width %g.",
			pulser->record->name, pulser->pulse_width );
	}
	if ( pulser->pulse_period < 0.0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Pulser '%s' is configured for a negative pulse period %g.",
			pulser->record->name, pulser->pulse_period );
	}

	if ( pulser->pulse_period < pulser->pulse_width ) {
		mx_warning( "The pulse period %g for pulser '%s' is less than "
			"the pulse width %g.  The pulse period will be "
			"increased to match the pulse width.",
				pulser->pulse_period,
				pulser->record->name,
				pulser->pulse_width );

		pulser->pulse_period = pulser->pulse_width;
	}

	/* Stop the pulser, just in case it was running. */

	snprintf( command, sizeof(command),
			"PUT %s.stop",
			umx_pulser->pulser_name );

	mx_status = mx_umx_command( umx_record, pulser->record->name,
		fname, command, response, sizeof(response), debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the pulse period. */

	snprintf( command, sizeof(command),
			"PUT %s.pulse_period %f",
			umx_pulser->pulser_name,
			pulser->pulse_period );

	mx_status = mx_umx_command( umx_record, pulser->record->name,
		fname, command, response, sizeof(response), debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the pulse width. */

	snprintf( command, sizeof(command),
			"PUT %s.pulse_width %f",
			umx_pulser->pulser_name,
			pulser->pulse_width );

	mx_status = mx_umx_command( umx_record, pulser->record->name,
		fname, command, response, sizeof(response), debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the pulse delay. */

	snprintf( command, sizeof(command),
			"PUT %s.pulse_delay %f",
			umx_pulser->pulser_name,
			pulser->pulse_delay );

	mx_status = mx_umx_command( umx_record, pulser->record->name,
		fname, command, response, sizeof(response), debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the number of pulses. */

	snprintf( command, sizeof(command),
			"PUT %s.num_pulses %lu",
			umx_pulser->pulser_name,
			pulser->num_pulses );

	mx_status = mx_umx_command( umx_record, pulser->record->name,
		fname, command, response, sizeof(response), debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the function mode. */

	snprintf( command, sizeof(command),
			"PUT %s.function_mode %lu",
			umx_pulser->pulser_name,
			pulser->function_mode );

	mx_status = mx_umx_command( umx_record, pulser->record->name,
		fname, command, response, sizeof(response), debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Set the trigger mode. */

	snprintf( command, sizeof(command),
			"PUT %s.trigger_mode %lu",
			umx_pulser->pulser_name,
			pulser->trigger_mode );

	mx_status = mx_umx_command( umx_record, pulser->record->name,
		fname, command, response, sizeof(response), debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* If we are not in external trigger mode, then we are done. */

#if 0
	if ( (pulser->trigger_mode & MXF_DEV_EXTERNAL_TRIGGER) == 0 ) {
		return MX_SUCCESSFUL_RESULT;
	}
#endif

	/* We are in external trigger mode. */

	/* Arm the pulse generator. */

	snprintf( command, sizeof(command),
		"PUT %s.arm 1",
		umx_pulser->pulser_name );

	mx_status = mx_umx_command( umx_record, pulser->record->name,
		fname, command, response, sizeof(response), debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Mark the pulse generator as busy. */

	pulser->busy = TRUE;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_umx_pulser_trigger( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_umx_pulser_arm()";

	MX_UMX_PULSER *umx_pulser = NULL;
	MX_RECORD *umx_record = NULL;
	char command[80];
	char response[200];
	mx_bool_type debug_flag;
	mx_status_type mx_status;

	mx_status = mxd_umx_pulser_get_pointers( pulser,
					&umx_pulser, &umx_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	debug_flag = FALSE;

#if MXD_UMX_PULSER_DEBUG_RUNNING
	MX_DEBUG(-2,("%s: Pulse generator '%s' triggered, "
			"trigger_mode = %#lx",
			fname, pulser->record->name,
			pulser->trigger_mode ));
#endif

	/*----*/

	if ( (pulser->trigger_mode & MXF_DEV_INTERNAL_TRIGGER) == 0 ) {
		return MX_SUCCESSFUL_RESULT;
	}

	/* We are in internal trigger mode. */

	/* Start the pulse generator. */

	snprintf( command, sizeof(command),
		"PUT %s.trigger 1",
		umx_pulser->pulser_name );

	mx_status = mx_umx_command( umx_record, pulser->record->name,
		fname, command, response, sizeof(response), debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Mark the pulse generator as busy. */

	pulser->busy = TRUE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_umx_pulser_stop( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_umx_pulser_stop()";

	MX_UMX_PULSER *umx_pulser = NULL;
	MX_RECORD *umx_record = NULL;
	char command[80];
	char response[200];
	mx_bool_type debug_flag;
	mx_status_type mx_status;

	mx_status = mxd_umx_pulser_get_pointers( pulser,
					&umx_pulser, &umx_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	debug_flag = FALSE;

#if MXD_UMX_PULSER_DEBUG_RUNNING
	MX_DEBUG(-2,("%s: Stopping pulse generator '%s'.",
		fname, pulser->record->name ));
#endif

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	snprintf( command, sizeof(command),
		"PUT %s.stop 1",
		umx_pulser->pulser_name );

	mx_status = mx_umx_command( umx_record, pulser->record->name,
		fname, command, response, sizeof(response), debug_flag );

	return mx_status;;
}

/*-----*/

MX_EXPORT mx_status_type
mxd_umx_pulser_get_parameter( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_umx_pulser_get_parameter()";

	MX_UMX_PULSER *umx_pulser = NULL;
	MX_RECORD *umx_record = NULL;
	MX_PULSE_GENERATOR_FUNCTION_LIST *pulser_flist = NULL;
	unsigned long flags;
	int num_items;
	char command[80];
	char response[200];
	mx_bool_type debug_flag;
	mx_status_type mx_status;

	debug_flag = FALSE;

	mx_status = mxd_umx_pulser_get_pointers( pulser,
					&umx_pulser, &umx_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_pulse_generator_get_pointers( pulser->record, NULL,
						&pulser_flist, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	flags = umx_pulser->umx_pulser_flags;

	MXW_UNUSED( flags );

#if MXD_UMX_PULSER_DEBUG
	MX_DEBUG(-2,
	("%s invoked for PULSE_GENERATOR '%s', parameter type '%s' (%ld)",
		fname, pulser->record->name,
		mx_get_field_label_string( pulser->record,
					pulser->parameter_type ),
		pulser->parameter_type));
#endif

	switch( pulser->parameter_type ) {
	case MXLV_PGN_NUM_PULSES:
		snprintf( command, sizeof(command),
			"GET %s.num_pulses", umx_pulser->pulser_name );

		mx_status = mx_umx_command( umx_record, pulser->record->name,
		    fname, command, response, sizeof(response), debug_flag );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "$%ld", &(pulser->num_pulses) );

		if ( num_items != 1 ) {
			return mx_error( MXE_PROTOCOL_ERROR, fname,
			"The response '%s' to command 'PNUM' sent to "
			"pulse generator '%s' was not understandable.",
				response, pulser->record->name );
		}
		break;

	case MXLV_PGN_PULSE_WIDTH:
		snprintf( command, sizeof(command),
			"GET %s.pulse_width", umx_pulser->pulser_name );

		mx_status = mx_umx_command( umx_record, pulser->record->name,
		    fname, command, response, sizeof(response), debug_flag );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "$%lg", &(pulser->pulse_width) );

		if ( num_items != 1 ) {
			return mx_error( MXE_PROTOCOL_ERROR, fname,
			"The response '%s' to command 'PWID' sent to "
			"pulse generator '%s' was not understandable.",
				response, pulser->record->name );
		}
		break;

	case MXLV_PGN_PULSE_PERIOD:
		snprintf( command, sizeof(command),
			"GET %s.pulse_period", umx_pulser->pulser_name );

		mx_status = mx_umx_command( umx_record, pulser->record->name,
		    fname, command, response, sizeof(response), debug_flag );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "$%lg", &(pulser->pulse_period) );

		if ( num_items != 1 ) {
			return mx_error( MXE_PROTOCOL_ERROR, fname,
			"The response '%s' to command 'PPER' sent to "
			"pulse generator '%s' was not understandable.",
				response, pulser->record->name );
		}
		break;

	case MXLV_PGN_PULSE_DELAY:
		snprintf( command, sizeof(command),
			"GET %s.pulse_delay", umx_pulser->pulser_name );

		mx_status = mx_umx_command( umx_record, pulser->record->name,
		    fname, command, response, sizeof(response), debug_flag );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "$%lg", &(pulser->pulse_delay) );

		if ( num_items != 1 ) {
			return mx_error( MXE_PROTOCOL_ERROR, fname,
			"The response '%s' to command 'PDLY' sent to "
			"pulse generator '%s' was not understandable.",
				response, pulser->record->name );
		}
		break;

	case MXLV_PGN_FUNCTION_MODE:
		snprintf( command, sizeof(command),
			"GET %s.function_mode", umx_pulser->pulser_name );

		mx_status = mx_umx_command( umx_record, pulser->record->name,
		    fname, command, response, sizeof(response), debug_flag );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "$%ld", &(pulser->function_mode));

		if ( num_items != 1 ) {
			return mx_error( MXE_PROTOCOL_ERROR, fname,
			"The response '%s' to command 'PFMO' sent to "
			"pulse generator '%s' was not understandable.",
				response, pulser->record->name );
		}
		break;

	case MXLV_PGN_LAST_PULSE_NUMBER:
		break;

	default:
		return
		    mx_pulse_generator_default_get_parameter_handler( pulser );

		break;
	}

#if MXD_UMX_PULSER_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_umx_pulser_set_parameter( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_umx_pulser_set_parameter()";

	MX_UMX_PULSER *umx_pulser = NULL;
	MX_RECORD *umx_record = NULL;
	char command[80];
	char response[200];
	mx_bool_type debug_flag;
	mx_status_type mx_status;

	mx_status = mxd_umx_pulser_get_pointers( pulser,
					&umx_pulser, &umx_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_UMX_PULSER_DEBUG
	MX_DEBUG(-2,
	("%s invoked for PULSE_GENERATOR '%s', parameter type '%s' (%ld)",
		fname, pulser->record->name,
		mx_get_field_label_string( pulser->record,
					pulser->parameter_type ),
		pulser->parameter_type));
#endif

	debug_flag = FALSE;

	switch( pulser->parameter_type ) {
	case MXLV_PGN_PULSE_PERIOD:
		snprintf( command, sizeof(command),
			"PUT %s.pulse_period %f",
			umx_pulser->pulser_name,
			pulser->pulse_period );

		mx_status = mx_umx_command( umx_record, pulser->record->name,
		    fname, command, response, sizeof(response), debug_flag );
		break;

	case MXLV_PGN_PULSE_WIDTH:
		snprintf( command, sizeof(command),
			"PUT %s.pulse_width %f",
			umx_pulser->pulser_name,
			pulser->pulse_width );

		mx_status = mx_umx_command( umx_record, pulser->record->name,
		    fname, command, response, sizeof(response), debug_flag );
		break;

	case MXLV_PGN_NUM_PULSES:
		snprintf( command, sizeof(command),
			"PUT %s.num_pulses %ld",
			umx_pulser->pulser_name,
			pulser->num_pulses );

		mx_status = mx_umx_command( umx_record, pulser->record->name,
		    fname, command, response, sizeof(response), debug_flag );
		break;

	case MXLV_PGN_PULSE_DELAY:
		snprintf( command, sizeof(command),
			"PUT %s.pulse_delay %f",
			umx_pulser->pulser_name,
			pulser->pulse_delay );

		mx_status = mx_umx_command( umx_record, pulser->record->name,
		    fname, command, response, sizeof(response), debug_flag );
		break;

	case MXLV_PGN_FUNCTION_MODE:
		snprintf( command, sizeof(command),
			"PUT %s.function_mode %ld",
			umx_pulser->pulser_name,
			pulser->function_mode );

		mx_status = mx_umx_command( umx_record, pulser->record->name,
		    fname, command, response, sizeof(response), debug_flag );
		break;

	case MXLV_PGN_TRIGGER_MODE:
		snprintf( command, sizeof(command),
			"PUT %s.trigger_mode %ld",
			umx_pulser->pulser_name,
			pulser->trigger_mode );

		mx_status = mx_umx_command( umx_record, pulser->record->name,
		    fname, command, response, sizeof(response), debug_flag );
		break;
	default:
		return
		    mx_pulse_generator_default_set_parameter_handler( pulser );

		break;
	}

#if MXD_UMX_PULSER_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_umx_pulser_setup( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_umx_pulser_setup()";

	MX_UMX_PULSER *umx_pulser = NULL;
	mx_status_type mx_status;

	mx_status = mxd_umx_pulser_get_pointers( pulser,
						&umx_pulser, NULL, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 0
	pulser->pulse_period  = pulser->setup[MXSUP_PGN_PULSE_PERIOD];
	pulser->pulse_width   = pulser->setup[MXSUP_PGN_PULSE_WIDTH];
	pulser->num_pulses    = mx_round( pulser->setup[MXSUP_PGN_NUM_PULSES] );
	pulser->pulse_delay   = pulser->setup[MXSUP_PGN_PULSE_DELAY];
	pulser->function_mode = 
			mx_round( pulser->setup[MXSUP_PGN_FUNCTION_MODE] );
	pulser->trigger_mode = 
			mx_round( pulser->setup[MXSUP_PGN_TRIGGER_MODE] );
#else
	mx_status = mx_pulse_generator_update_settings_from_setup( pulser );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;
#endif

#if MXD_UMX_PULSER_DEBUG_SETUP
	MX_DEBUG(-2,("%s: pulser '%s', period = %f, width = %f, "
		"num_pulses = %ld, delay = %f, "
		"function mode = %ld, trigger_mode = %ld",
		fname, pulser->record->name,
		pulser->pulse_period,
		pulser->pulse_width,
		pulser->num_pulses,
		pulser->pulse_delay,
		pulser->function_mode,
		pulser->trigger_mode ));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_umx_pulser_get_status( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_umx_pulser_get_status()";

	MX_UMX_PULSER *umx_pulser = NULL;
	MX_RECORD *umx_record = NULL;
	char command[80];
	char response[200];
	int num_items;
	mx_bool_type debug_flag;
	mx_status_type mx_status;

	mx_status = mxd_umx_pulser_get_pointers( pulser,
					&umx_pulser, &umx_record, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	debug_flag = FALSE;

	snprintf( command, sizeof(command),
		"GET %s.status", umx_pulser->pulser_name );

	mx_status = mx_umx_command( umx_record, pulser->record->name,
		fname, command, response, sizeof(response), debug_flag );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_items = sscanf( response, "$%lu", &(pulser->status) );

	if ( num_items != 1 ) {
		return mx_error( MXE_PROTOCOL_ERROR, fname,
		"The response '%s' to command 'PPER' sent to "
		"pulse generator '%s' was not understandable.",
			response, pulser->record->name );
	}

	return mx_status;
}

