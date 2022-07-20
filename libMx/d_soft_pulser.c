/*
 * Name:    d_soft_pulser.c
 *
 * Purpose: MX driver for a software-emulated MX pulse generator.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2022 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_SOFT_PULSER_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_pulse_generator.h"
#include "d_soft_pulser.h"

/* Initialize the pulse generator driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_soft_pulser_record_function_list = {
	NULL,
	mxd_soft_pulser_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_soft_pulser_open
};

MX_PULSE_GENERATOR_FUNCTION_LIST mxd_soft_pulser_pulse_generator_function_list
= {
	mxd_soft_pulser_is_busy,
	mxd_soft_pulser_arm,
	mxd_soft_pulser_trigger,
	mxd_soft_pulser_stop,
	NULL,
	mxd_soft_pulser_get_parameter,
	mxd_soft_pulser_set_parameter
};

/* MX digital output pulser data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_soft_pulser_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_PULSE_GENERATOR_STANDARD_FIELDS,
	MXD_SOFT_PULSER_STANDARD_FIELDS
};

long mxd_soft_pulser_num_record_fields
		= sizeof( mxd_soft_pulser_record_field_defaults )
		  / sizeof( mxd_soft_pulser_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_soft_pulser_rfield_def_ptr
			= &mxd_soft_pulser_record_field_defaults[0];

/*=======================================================================*/

static mx_status_type
mxd_soft_pulser_get_pointers( MX_PULSE_GENERATOR *pulser,
			MX_SOFT_PULSER **soft_pulser,
			const char *calling_fname )
{
	static const char fname[] = "mxd_soft_pulser_get_pointers()";

	MX_SOFT_PULSER *soft_pulser_ptr;

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

	soft_pulser_ptr = (MX_SOFT_PULSER *)
					pulser->record->record_type_struct;

	if ( soft_pulser_ptr == (MX_SOFT_PULSER *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_SOFT_PULSER pointer for pulse generator "
			"record '%s' passed by '%s' is NULL",
				pulser->record->name, calling_fname );
	}

	if ( soft_pulser != (MX_SOFT_PULSER **) NULL ) {
		*soft_pulser = soft_pulser_ptr;
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_soft_pulser_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_soft_pulser_create_record_structures()";

	MX_PULSE_GENERATOR *pulser;
	MX_SOFT_PULSER *soft_pulser;

	/* Allocate memory for the necessary structures. */

	pulser = (MX_PULSE_GENERATOR *) malloc( sizeof(MX_PULSE_GENERATOR) );

	if ( pulser == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_PULSE_GENERATOR structure." );
	}

	soft_pulser = (MX_SOFT_PULSER *)
				malloc( sizeof(MX_SOFT_PULSER) );

	if ( soft_pulser == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for MX_SOFT_PULSER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = pulser;
	record->record_type_struct = soft_pulser;
	record->class_specific_function_list
			= &mxd_soft_pulser_pulse_generator_function_list;

	pulser->record = record;
	soft_pulser->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_pulser_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_soft_pulser_open()";

	MX_PULSE_GENERATOR *pulser;
	MX_SOFT_PULSER *soft_pulser = NULL;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	pulser = (MX_PULSE_GENERATOR *) record->record_class_struct;

	mx_status = mxd_soft_pulser_get_pointers( pulser,
						&soft_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_pulser_is_busy( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_soft_pulser_is_busy()";

	MX_SOFT_PULSER *soft_pulser = NULL;
	mx_status_type mx_status;

	mx_status = mxd_soft_pulser_get_pointers( pulser, &soft_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	pulser->busy = FALSE;

	pulser->status = 0;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_pulser_arm( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_soft_pulser_arm()";

	MX_SOFT_PULSER *soft_pulser = NULL;
	mx_status_type mx_status;

	mx_status = mxd_soft_pulser_get_pointers( pulser, &soft_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_PULSER_DEBUG
	MX_DEBUG(-2,("%s: Pulse generator '%s' starting, "
		"pulse_width = %f, pulse_period = %f, num_pulses = %ld",
			fname, pulser->record->name,
			pulser->pulse_width,
			pulser->pulse_period,
			pulser->num_pulses));
#endif

	/* Initialize the internal state of the pulse generator. */

	pulser->busy = TRUE;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_pulser_trigger( MX_PULSE_GENERATOR *pulser )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_pulser_stop( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_soft_pulser_stop()";

	MX_SOFT_PULSER *soft_pulser = NULL;
	mx_status_type mx_status;

	mx_status = mxd_soft_pulser_get_pointers( pulser,
						&soft_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_PULSER_DEBUG
	MX_DEBUG(-2,("%s: Stopping pulse generator '%s'.",
		fname, pulser->record->name ));
#endif

	/* Update the internal state of the pulse generator. */

	pulser->busy = FALSE;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_soft_pulser_get_parameter( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_soft_pulser_get_parameter()";

	MX_SOFT_PULSER *soft_pulser = NULL;
	mx_status_type mx_status;

	mx_status = mxd_soft_pulser_get_pointers( pulser,
						&soft_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_PULSER_DEBUG
	MX_DEBUG(-2,
	("%s invoked for PULSE_GENERATOR '%s', parameter type '%s' (%ld)",
		fname, pulser->record->name,
		mx_get_field_label_string( pulser->record,
					pulser->parameter_type ),
		pulser->parameter_type));
#endif

	switch( pulser->parameter_type ) {
	case MXLV_PGN_NUM_PULSES:
		break;

	case MXLV_PGN_PULSE_WIDTH:
		break;

	case MXLV_PGN_PULSE_DELAY:
		break;

	case MXLV_PGN_FUNCTION_MODE:
		switch( pulser->function_mode ) {
		case MXF_PGN_PULSE:
			break;
		case MXF_PGN_SQUARE_WAVE:
			pulser->pulse_width = 0.5 * pulser->pulse_period;
			break;
		default:
			mx_status = mx_error(
			    MXE_HARDWARE_CONFIGURATION_ERROR, fname,
			"Illegal function mode %ld configured for pulser '%s'.",
			    pulser->function_mode, pulser->record->name );

			pulser->function_mode = -1;
			break;
		}
		break;

	case MXLV_PGN_TRIGGER_MODE:
		pulser->trigger_mode = MXF_DEV_INTERNAL_TRIGGER;
		break;

	case MXLV_PGN_PULSE_PERIOD:
		break;

	default:
		mx_status =
		    mx_pulse_generator_default_get_parameter_handler( pulser );
	}

#if MXD_SOFT_PULSER_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_soft_pulser_set_parameter( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_soft_pulser_set_parameter()";

	MX_SOFT_PULSER *soft_pulser = NULL;
	mx_status_type mx_status;

	mx_status = mxd_soft_pulser_get_pointers( pulser,
						&soft_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_SOFT_PULSER_DEBUG
	MX_DEBUG(-2,
	("%s invoked for PULSE_GENERATOR '%s', parameter type '%s' (%ld)",
		fname, pulser->record->name,
		mx_get_field_label_string( pulser->record,
					pulser->parameter_type ),
		pulser->parameter_type));
#endif

	switch( pulser->parameter_type ) {
	case MXLV_PGN_NUM_PULSES:
		break;

	case MXLV_PGN_PULSE_WIDTH:
		break;

	case MXLV_PGN_PULSE_DELAY:
		break;

	case MXLV_PGN_FUNCTION_MODE:
		switch( pulser->function_mode ) {
		case MXF_PGN_PULSE:
		case MXF_PGN_SQUARE_WAVE:
			break;
		default:
			mx_status = mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		    "Illegal function mode %ld requested for pulser '%s'.  "
		    "The allowed modes are 'pulse' (1) or 'square wave' (2).",
				pulser->function_mode, pulser->record->name );
		}
		break;

	case MXLV_PGN_TRIGGER_MODE:
		pulser->trigger_mode = MXF_DEV_INTERNAL_TRIGGER;
		break;

	case MXLV_PGN_PULSE_PERIOD:
		break;

	default:
		mx_status =
		    mx_pulse_generator_default_set_parameter_handler( pulser );
	}

#if MXD_SOFT_PULSER_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

