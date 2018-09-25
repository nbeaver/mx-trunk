/*
 * Name:    d_dg645_pulser.c
 *
 * Purpose: MX pulse generator driver for the Stanford Research Systems
 *          DG645 Digital Delay Generator.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2017-2018 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_DG645_PULSER_DEBUG	FALSE

#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_process.h"
#include "mx_pulse_generator.h"
#include "i_dg645.h"
#include "d_dg645_pulser.h"

/* Initialize the pulse generator driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_dg645_pulser_record_function_list = {
	NULL,
	mxd_dg645_pulser_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_dg645_pulser_open
};

MX_PULSE_GENERATOR_FUNCTION_LIST mxd_dg645_pulser_pulser_function_list = {
	NULL,
	mxd_dg645_pulser_start,
	mxd_dg645_pulser_stop,
	mxd_dg645_pulser_get_parameter,
	mxd_dg645_pulser_set_parameter,
	NULL,
	mxd_dg645_pulser_get_status
};

/* MX digital output pulser data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_dg645_pulser_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_PULSE_GENERATOR_STANDARD_FIELDS,
	MXD_DG645_PULSER_STANDARD_FIELDS
};

long mxd_dg645_pulser_num_record_fields
		= sizeof( mxd_dg645_pulser_record_field_defaults )
		  / sizeof( mxd_dg645_pulser_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_dg645_pulser_rfield_def_ptr
			= &mxd_dg645_pulser_record_field_defaults[0];

/*=======================================================================*/

static mx_status_type
mxd_dg645_pulser_get_pointers( MX_PULSE_GENERATOR *pulser,
			MX_DG645_PULSER **dg645_pulser,
			MX_DG645 **dg645,
			const char *calling_fname )
{
	static const char fname[] = "mxd_dg645_pulser_get_pointers()";

	MX_DG645_PULSER *dg645_pulser_ptr;

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

	dg645_pulser_ptr = (MX_DG645_PULSER *)
					pulser->record->record_type_struct;

	if ( dg645_pulser_ptr == (MX_DG645_PULSER *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_DG645_PULSER pointer for pulse generator "
			"record '%s' passed by '%s' is NULL",
				pulser->record->name, calling_fname );
	}

	if ( dg645_pulser != (MX_DG645_PULSER **) NULL ) {
		*dg645_pulser = dg645_pulser_ptr;
	}

	if ( dg645 != (MX_DG645 **) NULL ) {
		if ( dg645_pulser_ptr->dg645_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The pointer to the MX_DG645 pointer for "
			"pulse generator '%s' passed by '%s' is NULL.",
				pulser->record->name, calling_fname );
		}

		*dg645 = (MX_DG645 *)
			dg645_pulser_ptr->dg645_record->record_type_struct;

		if ( (*dg645) == (MX_DG645 *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_DG645 pointer for "
			"pulse generator '%s' passed by '%s' is NULL.",
				pulser->record->name, calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_dg645_pulser_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
			"mxd_dg645_pulser_create_record_structures()";

	MX_PULSE_GENERATOR *pulser;
	MX_DG645_PULSER *dg645_pulser;

	/* Allocate memory for the necessary structures. */

	pulser = (MX_PULSE_GENERATOR *) malloc( sizeof(MX_PULSE_GENERATOR) );

	if ( pulser == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_PULSE_GENERATOR structure." );
	}

	dg645_pulser = (MX_DG645_PULSER *)
				malloc( sizeof(MX_DG645_PULSER) );

	if ( dg645_pulser == NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for an MX_DG645_PULSER structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = pulser;
	record->record_type_struct = dg645_pulser;
	record->class_specific_function_list
			= &mxd_dg645_pulser_pulser_function_list;

	pulser->record = record;
	dg645_pulser->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dg645_pulser_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_dg645_pulser_open()";

	MX_PULSE_GENERATOR *pulser;
	MX_DG645_PULSER *dg645_pulser = NULL;
	MX_DG645 *dg645 = NULL;
	size_t i, length;
	int c;
	char *output_name;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	pulser = (MX_PULSE_GENERATOR *) record->record_class_struct;

	mx_status = mxd_dg645_pulser_get_pointers( pulser,
					&dg645_pulser, &dg645, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	output_name = dg645_pulser->output_name;

	length = strlen( output_name );

	if ( length != 2 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Invalid output name '%s'.  "
		"Output names for '%s' should be 2 characters long.",
			output_name, record->name );
	}

	/* Force the output_name field to upper case. */

	for ( i = 0; i < length; i++ ) {
		c = output_name[i];

		if ( islower(c) ) {
			output_name[i] = toupper(c);
		}
	}

	/* Figure out the output number for this channel as used by
	 * delay and output commands.
	 */

	if ( strcmp( "T0", output_name ) == 0 ) {
		dg645_pulser->output_number = 0;
	} else
	if ( strcmp( "AB", output_name ) == 0 ) {
		dg645_pulser->output_number = 1;
	} else
	if ( strcmp( "CD", output_name ) == 0 ) {
		dg645_pulser->output_number = 2;
	} else
	if ( strcmp( "EF", output_name ) == 0 ) {
		dg645_pulser->output_number = 3;
	} else
	if ( strcmp( "GH", output_name ) == 0 ) {
		dg645_pulser->output_number = 4;
	} else {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal output name '%s' requested for pulser '%s'.  "
		"The allowed names are 'T0', 'AB', 'CD', 'EF', and 'GH'.",
			output_name, record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dg645_pulser_start( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_dg645_pulser_start()";

	MX_DG645_PULSER *dg645_pulser = NULL;
	MX_DG645 *dg645 = NULL;
	mx_bool_type single_shot_mode;
	mx_status_type mx_status;

	mx_status = mxd_dg645_pulser_get_pointers( pulser,
					&dg645_pulser, &dg645, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* This routine only actually does something if the trigger source
	 * is using a single shot trigger.
	 */

	/* Are we in a single shot mode? */

	mx_status = mx_process_record_field_by_name( dg645->record,
						"trigger_source",
						MX_PROCESS_GET, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( dg645->trigger_source ) {
	case 0: case 1: case 2: case 6:
		single_shot_mode = FALSE;
		break;
	case 3: case 4: case 5:
		single_shot_mode = TRUE;
		break;
	default:
		return mx_error( MXE_INTERFACE_ACTION_FAILED, fname,
		"DG645 controller '%s' reported that it was using "
		"an unsupported trigger mode %lu, which should _not_ "
		"be able to happen.  The supported modes are from 0 to 6.",
			dg645->record->name,
			dg645->trigger_source );
		break;
	}

#if 0
	if ( single_shot_mode ) {
#else
	if ( 1 ) {
#endif
		mx_status = mxi_dg645_command( dg645, "*TRG",
						NULL, 0, 
						MXD_DG645_PULSER_DEBUG );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_dg645_pulser_stop( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_dg645_pulser_stop()";

	MX_DG645_PULSER *dg645_pulser = NULL;
	MX_DG645 *dg645 = NULL;
	mx_status_type mx_status;

	mx_status = mxd_dg645_pulser_get_pointers( pulser,
					&dg645_pulser, &dg645, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* FIXME: The DG645 does not seem to have a stop as such. 
	 * All it seems you can do is to switch the trigger source
	 * to a single shot trigger.
	 */

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dg645_pulser_get_parameter( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_dg645_pulser_get_parameter()";

	MX_DG645_PULSER *dg645_pulser = NULL;
	MX_DG645 *dg645 = NULL;
	unsigned long starting_channel, ending_channel;
	unsigned long t0_channel;
	long trigger_source;
	char response[80];
	int num_items;
	double trigger_rate;
	mx_status_type mx_status;

	mx_status = mxd_dg645_pulser_get_pointers( pulser,
					&dg645_pulser, &dg645, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_DG645_PULSER_DEBUG
	MX_DEBUG(-2,
	("%s invoked for PULSE_GENERATOR '%s', parameter type '%s' (%ld)",
		fname, pulser->record->name,
		mx_get_field_label_string( pulser->record,
					pulser->parameter_type ),
		pulser->parameter_type));
#endif

	switch( pulser->parameter_type ) {
	case MXLV_PGN_FUNCTION_MODE:
		if ( pulser->function_mode == MXF_PGN_SQUARE_WAVE ) {
			pulser->pulse_width = 0.5 * pulser->pulse_period;
		}
		break;

	case MXLV_PGN_NUM_PULSES:
		mx_status = mx_process_record_field_by_name( dg645->record,
							"trigger_source",
							MX_PROCESS_GET, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( dg645->single_shot ) {
			pulser->num_pulses = 1;
		} else {
			pulser->num_pulses = 0;
		}
		break;

	case MXLV_PGN_PULSE_WIDTH:
		starting_channel = 2 * dg645_pulser->output_number;

		ending_channel = starting_channel + 1;

		mx_status = mxi_dg645_compute_delay_between_channels( dg645,
						ending_channel,
						starting_channel,
						&(pulser->pulse_width),
						NULL, NULL );
		break;

	case MXLV_PGN_PULSE_DELAY:
		starting_channel = 2 * dg645_pulser->output_number;

		t0_channel = 0;

		mx_status = mxi_dg645_compute_delay_between_channels( dg645,
						starting_channel,
						t0_channel,
						&(pulser->pulse_delay),
						NULL, NULL );
		break;

	case MXLV_PGN_PULSE_PERIOD:
		mx_status = mxi_dg645_command( dg645, "TRAT?",
						response, sizeof(response),
						MXD_DG645_PULSER_DEBUG );

		num_items = sscanf( response, "%lg", &trigger_rate );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Did not see the trigger rate in the response '%s' "
			"to command 'TRAT?' for DG645 controller '%s'.",
				response, dg645->record->name );
		}

		pulser->pulse_period = mx_divide_safely( 1.0, trigger_rate );
		break;

	case MXLV_PGN_TRIGGER_MODE:
		mx_status = mxi_dg645_command( dg645, "TSRC?",
					response, sizeof(response),
					MXD_DG645_PULSER_DEBUG );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_items = sscanf( response, "%ld", &trigger_source );

		if ( num_items != 1 ) {
			return mx_error( MXE_DEVICE_IO_ERROR, fname,
			"Did not see the trigger source in the response '%s' "
			"to command 'TSRC?' for DG645 controller '%s'.",
				response, dg645->record->name );
		}
		break;

	default:
		return
		    mx_pulse_generator_default_get_parameter_handler( pulser );
	}

#if MXD_DG645_PULSER_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_dg645_pulser_set_parameter( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_dg645_pulser_set_parameter()";

	MX_DG645_PULSER *dg645_pulser = NULL;
	MX_DG645 *dg645 = NULL;
	char command[80];
	int new_trigger_source;
	unsigned long t0_channel;
	unsigned long starting_channel, ending_channel;
	unsigned long adjacent_channel;
	double adjacent_delay, new_adjacent_delay;
	double existing_delay, delay_difference;
	double existing_width;
	double trigger_rate;
	mx_status_type mx_status;

	mx_status = mxd_dg645_pulser_get_pointers( pulser,
					&dg645_pulser, &dg645, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_DG645_PULSER_DEBUG
	MX_DEBUG(-2,
	("%s invoked for PULSE_GENERATOR '%s', parameter type '%s' (%ld)",
		fname, pulser->record->name,
		mx_get_field_label_string( pulser->record,
					pulser->parameter_type ),
		pulser->parameter_type));
#endif

	switch( pulser->parameter_type ) {
	case MXLV_PGN_FUNCTION_MODE:
		switch( pulser->function_mode ) {
		case MXF_PGN_PULSE:
			break;
		case MXF_PGN_SQUARE_WAVE:
			pulser->pulse_width = 0.5 * pulser->pulse_period;
			break;
		default:
			mx_status = mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Illegal function mode %ld requested for pulser '%s'.",
			    pulser->function_mode, pulser->record->name );

			pulser->function_mode = -1;
			break;
		}
		break;

	case MXLV_PGN_NUM_PULSES:
		/* First, get the existing trigger source, so that we
		 * can modify it to match what we want here.
		 */

		mx_status = mx_process_record_field_by_name( dg645->record,
							"trigger_source",
							MX_PROCESS_GET, NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Now construct the new trigger source value. */

		switch( dg645->trigger_type ) {
		case MXF_DG645_INTERNAL_TRIGGER:
			pulser->trigger_mode = MXF_PGN_INTERNAL_TRIGGER;

			if ( pulser->num_pulses == 1 ) {
				/* Single shot */
				new_trigger_source = 5;
			} else {
				/* Internal */
				new_trigger_source = 1;
				pulser->num_pulses = 0;
			}
			break;
		case MXF_DG645_EXTERNAL_TRIGGER:
			pulser->trigger_mode = MXF_PGN_EXTERNAL_TRIGGER;

			if ( pulser->num_pulses == 1 ) {
				if ( dg645->trigger_direction < 0 ) {
					/* Single shot ext falling edges */
					new_trigger_source = 4;
					dg645->trigger_direction = -1;
				} else {
					/* Single shot ext rising edges */
					new_trigger_source = 3;
					dg645->trigger_direction = 1;
				}
			} else {
				if ( dg645->trigger_direction < 0 ) {
					/* External falling edges */
					new_trigger_source = 2;
					dg645->trigger_direction = -1;
				} else {
					/* External rising edges */
					new_trigger_source = 1;
					dg645->trigger_direction = 1;
				}
				pulser->num_pulses = 0;
			}
			break;
		case MXF_DG645_LINE_TRIGGER:
			/* For 'line trigger', the number of pulses is always
			 * infinite, so we can't change anything and should
			 * not send a command to the DG645.
			 */

			pulser->trigger_mode = 0;
			pulser->num_pulses = 0;
			dg645->trigger_direction = 0;
			dg645->single_shot = FALSE;

			return MX_SUCCESSFUL_RESULT;
			break;
		default:
			return mx_error( MXE_DEVICE_ACTION_FAILED, fname,
			"DG645 controller '%s' used by pulser '%s' is somehow "
			"configured for an illegal trigger type %lu.  "
			"This should never happen.",
				dg645->record->name,
				pulser->record->name,
				dg645->trigger_type );
			break;
		}

		/* Now set the new trigger source. */

		snprintf( command, sizeof(command),
			"TSRC %d", new_trigger_source );

		mx_status = mxi_dg645_command( dg645, command,
					NULL, 0, MXD_DG645_PULSER_DEBUG );
		break;

	case MXLV_PGN_PULSE_WIDTH:
		starting_channel = 2 * dg645_pulser->output_number;

		ending_channel = starting_channel + 1;

		/* We first need to figure out what the preexisting
		 * width is as well as the channel our offset is
		 * relative to.
		 */

		mx_status = mxi_dg645_compute_delay_between_channels( dg645,
						ending_channel,
						starting_channel,
						&existing_width,
						&adjacent_channel,
						&adjacent_delay );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Reprogram the delay to the adjacent channel so that
		 * we get the overall width that we want.
		 */

		delay_difference = pulser->pulse_width - existing_width;

		new_adjacent_delay = adjacent_delay + delay_difference;

		snprintf( command, sizeof(command),
			"DLAY %lu,%lu,%g",
			ending_channel, adjacent_channel,
			new_adjacent_delay );

		mx_status = mxi_dg645_command( dg645, command,
					NULL, 0, MXD_DG645_PULSER_DEBUG );
		break;

	case MXLV_PGN_PULSE_DELAY:
		starting_channel = 2 * dg645_pulser->output_number;

		t0_channel = 0;

		/* We first need to figure out what the preexisting 
		 * delay is as well as the channel our offset is
		 * relative to.
		 */

		mx_status = mxi_dg645_compute_delay_between_channels( dg645,
						starting_channel,
						t0_channel,
						&existing_delay,
						&adjacent_channel,
						&adjacent_delay );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Reprogram the delay to the adjacent channel so that
		 * we get the overall delay that we want.
		 */

		delay_difference = pulser->pulse_delay - existing_delay;

		new_adjacent_delay = adjacent_delay + delay_difference;

		snprintf( command, sizeof(command),
			"DLAY %lu,%lu,%g",
			starting_channel, adjacent_channel,
			new_adjacent_delay );

		mx_status = mxi_dg645_command( dg645, command,
					NULL, 0, MXD_DG645_PULSER_DEBUG );
		break;

	case MXLV_PGN_PULSE_PERIOD:
		trigger_rate = mx_divide_safely( 1.0, pulser->pulse_period );

		snprintf( command, sizeof(command), "TRAT %g", trigger_rate );

		mx_status = mxi_dg645_command( dg645, command,
					NULL, 0, MXD_DG645_PULSER_DEBUG );
		break;

	case MXLV_PGN_TRIGGER_MODE:
		mx_status = mxi_dg645_setup_pulser_trigger_mode( dg645,
						pulser->trigger_mode );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		snprintf( command, sizeof(command),
			"TSRC %lu", dg645->trigger_source );

		mx_status = mxi_dg645_command( dg645, command,
					NULL, 0, MXD_DG645_PULSER_DEBUG );
		break;

	default:
		mx_status = 
		    mx_pulse_generator_default_set_parameter_handler( pulser );
	}
	MX_DEBUG( 2,("%s complete.", fname));

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_dg645_pulser_get_status( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_dg645_pulser_get_status()";

	MX_DG645_PULSER *dg645_pulser = NULL;
	MX_DG645 *dg645 = NULL;
	mx_status_type mx_status;

	mx_status = mxd_dg645_pulser_get_pointers( pulser,
					&dg645_pulser, &dg645, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxi_dg645_get_status( dg645 );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_DG645_PULSER_DEBUG
	MX_DEBUG(-2,("%s: pulser '%s', stb = %#lx, insr = %#lx, esr = %#lx",
		fname, pulser->record->name,
		dg645->status_byte,
		dg645->instrument_status_register,
		dg645->event_status_register));
#endif

	pulser->status = 0;

	if ( dg645->status_byte & 0x2 ) {
		pulser->status |= MXSF_PGN_IS_BUSY;
	}

#if MXD_DG645_PULSER_DEBUG
	MX_DEBUG(-2,("%s: pulser '%s', status = %#lx",
		fname, pulser->record->name, pulser->status));
#endif

	return mx_status;
}

