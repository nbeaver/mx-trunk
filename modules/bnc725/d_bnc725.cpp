/*
 * Name:    d_bnc725.c
 *
 * Purpose: MX pulse generator driver for a single output channel of
 *          the vendor-provided Win32 C++ library for the BNC725 
 *          digital delay generator.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2010-2011, 2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_BNC725_DEBUG	FALSE

#include <stdio.h>
#include <ctype.h>
#include <windows.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_pulse_generator.h"

#include "i_bnc725.h"
#include "d_bnc725.h"

MX_RECORD_FUNCTION_LIST mxd_bnc725_record_function_list = {
	NULL,
	mxd_bnc725_create_record_structures,
	NULL,
	NULL,
	NULL,
	mxd_bnc725_open
};

MX_PULSE_GENERATOR_FUNCTION_LIST mxd_bnc725_pulse_generator_function_list = {
	mxd_bnc725_busy,
	mxd_bnc725_start,
	mxd_bnc725_stop,
	mxd_bnc725_get_parameter,
	mxd_bnc725_set_parameter
};

MX_RECORD_FIELD_DEFAULTS mxd_bnc725_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_PULSE_GENERATOR_STANDARD_FIELDS,
	MXD_BNC725_STANDARD_FIELDS
};

long mxd_bnc725_num_record_fields
		= sizeof( mxd_bnc725_record_field_defaults )
			/ sizeof( mxd_bnc725_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_bnc725_rfield_def_ptr
			= &mxd_bnc725_record_field_defaults[0];

/* --- */

static mx_status_type
mxd_bnc725_get_pointers( MX_PULSE_GENERATOR *pulse_generator,
			MX_BNC725_CHANNEL **bnc725_channel,
			MX_BNC725 **bnc725,
			const char *calling_fname )
{
	static const char fname[] = "mxd_bnc725_get_pointers()";

	MX_RECORD *bnc725_channel_record, *bnc725_lib_record;
	MX_BNC725_CHANNEL *local_bnc725_channel;

	if ( pulse_generator == (MX_PULSE_GENERATOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PULSE_GENERATOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	bnc725_channel_record = pulse_generator->record;

	if ( bnc725_channel_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for the MX_PULSE_GENERATOR pointer "
		"passed by '%s' is NULL.", calling_fname );
	}

	local_bnc725_channel = (MX_BNC725_CHANNEL *)
				bnc725_channel_record->record_type_struct;

	if ( bnc725_channel != (MX_BNC725_CHANNEL **) NULL ) {
		*bnc725_channel = local_bnc725_lib_channel;
	}

	if ( bnc725 != (MX_BNC725 **) NULL ) {
		bnc725_record = local_bnc725_lib_channel->bnc725_lib_record;

		if ( bnc725_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The bnc725_record pointer for "
			"'bnc725_channel' record '%s' "
			"passed by '%s' is NULL.",
				bnc725_channel_record->name,
				calling_fname );
		}

		*bnc725 = (MX_BNC725 *)
				bnc725_record->record_type_struct;

		if ( *bnc725 == (MX_BNC725 *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_BNC725 pointer for bnc725 record '%s' "
			"used by 'bnc725_channel' record '%s' "
			"passed by '%s' is NULL.",
				bnc725_record->name,
				bnc725_channel_record->name,
				calling_fname );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* ====== */

MX_EXPORT mx_status_type
mxd_bnc725_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_bnc725_create_record_structures()";

	MX_PULSE_GENERATOR *pulse_generator;
	MX_BNC725_CHANNEL *bnc725_channel = NULL;

	pulse_generator = (MX_PULSE_GENERATOR *)
				malloc( sizeof(MX_PULSE_GENERATOR) );

	if ( pulse_generator == (MX_PULSE_GENERATOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for a MX_PULSE_GENERATOR structure "
		"for record '%s'.", record->name );
	}

	bnc725_channel = (MX_BNC725_CHANNEL *)
				malloc( sizeof(MX_BNC725_CHANNEL) );

	if ( bnc725_channel == (MX_BNC725_CHANNEL *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannott allocate memory for a MX_BNC725_CHANNEL structure"
		"for record '%s'.", record->name );
	}

	record->record_class_struct = pulse_generator;
	record->record_type_struct = bnc725_channel;

	record->class_specific_function_list =
				&mxd_bnc725_pulse_generator_function_list;

	pulse_generator->record = record;
	bnc725_channel->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bnc725_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_bnc725_open()";

	MX_PULSE_GENERATOR *pulse_generator;
	MX_BNC725_CHANNEL *bnc725_channel = NULL;
	MX_BNC725 *bnc725 = NULL;
	int c;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	pulse_generator = (MX_PULSE_GENERATOR *) record->record_class_struct;

	mx_status = mxd_bnc725_get_pointers( pulse_generator,
				&bnc725_channel, &bnc725_lib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BNC725_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.", fname, record->name));
#endif

	/* Force the channel name to be upper case. */

	c = bnc725_channel->channel_name;

	if ( islower(c) ) {
		bnc725_channel->channel_name = (char) toupper(c);
	}

	/* Are we configured for a legal channel name? */

	if ( ( bnc725_channel->channel_name < 'A' )
	  || ( bnc725_channel->channel_name > 'H' ) )
	{
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Channel name '%c' for 'bnc725_channel' '%s' is illegal.  "
	"The allowed range of channel names for 'bnc725' record '%s' "
	"is from 'A' to 'H'.",
			bnc725_channel->channel_name, record->name,
			bnc725->record->name );
	}

	/* Setup the vendor channel structure. */

	bnc725_channel->channel =
	    bnc725->port->GetChannel( bnc725_lib_channel->channel_name );

	if ( bnc725_channel->channel == NULL ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
		"The attempt to get the CChannel object for channel '%c' "
		"of BNC725 channel '%s' failed.",
			bnc725_channel->channel_name,
			record->name );
	}

	/* Initialize the pulse period, width, etc. */

	pulse_generator->pulse_period = 0.0;
	pulse_generator->pulse_width = 0.0;
	pulse_generator->num_pulses = 0;
	pulse_generator->pulse_delay = 0.0;
	pulse_generator->mode = 0;

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_bnc725_busy( MX_PULSE_GENERATOR *pulse_generator )
{
	static const char fname[] = "mxd_bnc725_busy()";

	MX_BNC725_CHANNEL *bnc725_channel = NULL;
	MX_BNC725 *bnc725 = NULL;
	mx_status_type mx_status;

	mx_status = mxd_bnc725_get_pointers( pulse_generator,
				&bnc725_channel, &bnc725_lib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bnc725_start( MX_PULSE_GENERATOR *pulse_generator )
{
	static const char fname[] = "mxd_bnc725_start()";

	MX_BNC725_CHANNEL *bnc725_channel = NULL;
	MX_BNC725 *bnc725 = NULL;
	int bnc_status;
	mx_status_type mx_status;

	mx_status = mxd_bnc725_get_pointers( pulse_generator,
				&bnc725_channel, &bnc725_lib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	pulse_generator->busy = TRUE;

	if ( pulse_generator->num_pulses == 1 ) {

		/* Use BNC725 Delayed Pulse After Trigger mode. */

		CDelayDurate *dd = new CDelayDurate();

		dd->m_DlyProp.m_dDelay = 1000.0 * pulse_generator->pulse_delay;

		dd->m_DlyProp.m_dDuration =
					1000.0 * pulse_generator->pulse_width;

		CTimingMode *old_mode =
			bnc725_channel->channel->SetTimingMode( dd );

		if ( old_mode != NULL ) {
			delete old_mode;
		}
	} else
	if ( pulse_generator->num_pulses == MXF_PGN_FOREVER ) {

		double high_duration, low_duration;

		/* Use Clocked Pulse Stream mode. */

		CClock *clock = new CClock();

		high_duration = pulse_generator->pulse_width;

		switch( pulse_generator->mode ) {
		case MXF_PGN_PULSE:
			low_duration =
				pulse_generator->pulse_period - high_duration;
			break;

		case MXF_PGN_SQUARE_WAVE:
			low_duration = high_duration;

			pulse_generator->pulse_period =
				high_duration + low_duration;
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Unsupported pulse mode %ld requested for "
			"Clocked Pulse Stream mode of BNC725 channel '%s'.",
				pulse_generator->mode,
				pulse_generator->record->name );
			break;
		}

		clock->m_ClockProp.m_dHighDur = high_duration;
		clock->m_ClockProp.m_dLowDur  = low_duration;

		CTimingMode *old_mode =
			bnc725_channel->channel->SetTimingMode( clock );

		if ( old_mode != NULL ) {
			delete old_mode;
		}

	} else {
		return mx_error( MXE_UNSUPPORTED, fname,
		"Unsupported number of pulses %ld requested for "
		"BNC725 channel '%s'.  The supported numbers are "
		"1 (single pulse) and 0 (infinite pulse train).",
			pulse_generator->num_pulses,
			pulse_generator->record->name );
	}

	bnc_status = bnc725_channel->channel->CheckParameters();

	if ( bnc_status == 0 ) {
		return mx_error(MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested delay and/or duration times for "
		"BNC725 channel '%s' are invalid.",
			pulse_generator->record->name );
	}

	/* Send the new settings to the BNC725. */

	bnc_status = bnc725_channel->channel->CmdSetTimingMode();

	if ( bnc_status == 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to set new delay and duration settings for "
		"BNC725 channel '%s' failed.",
			pulse_generator->record->name );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_bnc725_stop( MX_PULSE_GENERATOR *pulse_generator )
{
	static const char fname[] = "mxd_bnc725_stop()";

	MX_BNC725_CHANNEL *bnc725_channel = NULL;
	MX_BNC725 *bnc725 = NULL;
	int bnc_status;
	mx_status_type mx_status;

	mx_status = mxd_bnc725_get_pointers( pulse_generator,
				&bnc725_channel, &bnc725_lib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	pulse_generator->busy = FALSE;

	/* Switch to Fixed Output mode and set the output to low. */

	CNull *fixed = new CNull();

	fixed->m_FixedProp.m_bHigh = 0;

	CTimingMode *old_mode =
		bnc725_channel->channel->SetTimingMode( fixed );

	if ( old_mode != NULL ) {
		delete old_mode;
	}

	bnc_status = bnc725_channel->channel->CheckParameters();

	if ( bnc_status == 0 ) {
		return mx_error(MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested delay and/or duration times for "
		"BNC725 channel '%s' are invalid.",
			pulse_generator->record->name );
	}

	/* Send the new settings to the BNC725. */

	bnc_status = bnc725_channel->channel->CmdSetTimingMode();

	if ( bnc_status == 0 ) {
		return mx_error( MXE_DEVICE_IO_ERROR, fname,
		"The attempt to set new delay and duration settings for "
		"BNC725 channel '%s' failed.",
			pulse_generator->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bnc725_get_parameter( MX_PULSE_GENERATOR *pulse_generator )
{
	static const char fname[] = "mxd_bnc725_get_parameter()";

	MX_BNC725_CHANNEL *bnc725_channel = NULL;
	MX_BNC725 *bnc725 = NULL;
	mx_status_type mx_status;

	mx_status = mxd_bnc725_get_pointers( pulse_generator,
				&bnc725_channel, &bnc725_lib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BNC725_DEBUG
	MX_DEBUG(-2,
	("%s invoked for PULSE_GENERATOR '%s', parameter type '%s' (%d)",
		fname, pulse_generator->record->name,
		mx_get_field_label_string( pulse_generator->record,
					pulse_generator->parameter_type ),
		pulse_generator->parameter_type));
#endif

	switch( pulse_generator->parameter_type ) {
	case MXLV_PGN_NUM_PULSES:
	case MXLV_PGN_PULSE_WIDTH:
	case MXLV_PGN_PULSE_DELAY:
	case MXLV_PGN_MODE:
	case MXLV_PGN_PULSE_PERIOD:
		/* Just report back previously set values. */

		break;
	default:
		return mx_pulse_generator_default_get_parameter_handler(
							pulse_generator );
	}

#if MXD_BNC725_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_bnc725_set_parameter( MX_PULSE_GENERATOR *pulse_generator )
{
	static const char fname[] = "mxd_bnc725_set_parameter()";

	MX_BNC725_CHANNEL *bnc725_channel = NULL;
	MX_BNC725 *bnc725 = NULL;
	mx_status_type mx_status;

	mx_status = mxd_bnc725_get_pointers( pulse_generator,
				&bnc725_channel, &bnc725_lib, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_BNC725_DEBUG
	MX_DEBUG(-2,
	("%s invoked for PULSE_GENERATOR '%s', parameter type '%s' (%d)",
		fname, pulse_generator->record->name,
		mx_get_field_label_string( pulse_generator->record,
					pulse_generator->parameter_type ),
		pulse_generator->parameter_type));
#endif

	switch( pulse_generator->parameter_type ) {
	case MXLV_PGN_NUM_PULSES:
		break;

	case MXLV_PGN_PULSE_DELAY:
		break;

	case MXLV_PGN_MODE:
		switch( pulse_generator->mode ) {
		case MXF_PGN_PULSE:
			break;
		case MXF_PGN_SQUARE_WAVE:
			break;
		default:
			return mx_error( MXE_UNSUPPORTED, fname,
		"Pulse mode %ld is not supported for pulse generator '%s'.  "
		"Only pulse mode (1) and square wave mode (2) are supported.",
				pulse_generator->mode,
				pulse_generator->record->name );
		}

		break;

	case MXLV_PGN_PULSE_WIDTH:
		break;

	case MXLV_PGN_PULSE_PERIOD:
		break;

	default:
		return mx_pulse_generator_default_set_parameter_handler(
							pulse_generator );
	}

#if MXD_BNC725_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

