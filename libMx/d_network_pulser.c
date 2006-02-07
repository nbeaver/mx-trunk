/*
 * Name:    d_network_pulser.c
 *
 * Purpose: MX network pulse generator driver.
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

#include <stdio.h>
#include <stdlib.h>
#include <float.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_record.h"
#include "mx_net.h"
#include "mx_pulse_generator.h"

#include "d_network_pulser.h"

MX_RECORD_FUNCTION_LIST mxd_network_pulser_record_function_list = {
	NULL,
	mxd_network_pulser_create_record_structures,
	mxd_network_pulser_finish_record_initialization
};

MX_PULSE_GENERATOR_FUNCTION_LIST
			mxd_network_pulser_pulse_generator_function_list = {
	mxd_network_pulser_busy,
	mxd_network_pulser_start,
	mxd_network_pulser_stop,
	mxd_network_pulser_get_parameter,
	mxd_network_pulser_set_parameter
};

MX_RECORD_FIELD_DEFAULTS mxd_network_pulser_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_PULSE_GENERATOR_STANDARD_FIELDS,
	MXD_NETWORK_PULSER_STANDARD_FIELDS
};

mx_length_type mxd_network_pulser_num_record_fields
		= sizeof( mxd_network_pulser_record_field_defaults )
			/ sizeof( mxd_network_pulser_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_network_pulser_rfield_def_ptr
			= &mxd_network_pulser_record_field_defaults[0];

/* --- */

static mx_status_type
mxd_network_pulser_get_pointers( MX_PULSE_GENERATOR *pulse_generator,
			MX_NETWORK_PULSER **network_pulser,
			const char *calling_fname )
{
	static const char fname[] = "mxd_network_pulser_get_pointers()";

	MX_RECORD *network_pulser_record;

	if ( pulse_generator == (MX_PULSE_GENERATOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_PULSE_GENERATOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	network_pulser_record = pulse_generator->record;

	if ( network_pulser_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_RECORD pointer for the MX_PULSE_GENERATOR pointer "
		"passed by '%s' is NULL.", calling_fname );
	}

	if ( network_pulser == (MX_NETWORK_PULSER **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_NETWORK_PULSER pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*network_pulser = (MX_NETWORK_PULSER *)
				network_pulser_record->record_type_struct;

	return MX_SUCCESSFUL_RESULT;
}

/* ====== */

MX_EXPORT mx_status_type
mxd_network_pulser_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_network_pulser_create_record_structures()";

	MX_PULSE_GENERATOR *pulse_generator;
	MX_NETWORK_PULSER *network_pulser;

	pulse_generator = (MX_PULSE_GENERATOR *)
				malloc( sizeof(MX_PULSE_GENERATOR) );

	if ( pulse_generator == (MX_PULSE_GENERATOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannot allocate memory for a MX_PULSE_GENERATOR structure "
		"for record '%s'.", record->name );
	}

	network_pulser = (MX_NETWORK_PULSER *)
				malloc( sizeof(MX_NETWORK_PULSER) );

	if ( network_pulser == (MX_NETWORK_PULSER *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Cannott allocate memory for a MX_NETWORK_PULSER structure"
		"for record '%s'.", record->name );
	}

	record->record_class_struct = pulse_generator;
	record->record_type_struct = network_pulser;

	record->class_specific_function_list =
			&mxd_network_pulser_pulse_generator_function_list;

	pulse_generator->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_pulser_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_network_pulser_finish_record_initialization()";

	MX_PULSE_GENERATOR *pulse_generator;
	MX_NETWORK_PULSER *network_pulser;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	pulse_generator = (MX_PULSE_GENERATOR *) record->record_class_struct;

	mx_status = mxd_network_pulser_get_pointers(
				pulse_generator, &network_pulser, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_network_field_init( &(network_pulser->busy_nf),
		network_pulser->server_record,
		"%s.busy", network_pulser->remote_record_name );

	mx_network_field_init( &(network_pulser->mode_nf),
		network_pulser->server_record,
		"%s.mode", network_pulser->remote_record_name );

	mx_network_field_init( &(network_pulser->num_pulses_nf),
		network_pulser->server_record,
		"%s.num_pulses", network_pulser->remote_record_name );

	mx_network_field_init( &(network_pulser->pulse_delay_nf),
		network_pulser->server_record,
		"%s.pulse_delay", network_pulser->remote_record_name );

	mx_network_field_init( &(network_pulser->pulse_period_nf),
		network_pulser->server_record,
		"%s.pulse_period", network_pulser->remote_record_name );

	mx_network_field_init( &(network_pulser->pulse_width_nf),
		network_pulser->server_record,
		"%s.pulse_width", network_pulser->remote_record_name );

	mx_network_field_init( &(network_pulser->start_nf),
		network_pulser->server_record,
		"%s.start", network_pulser->remote_record_name );

	mx_network_field_init( &(network_pulser->stop_nf),
		network_pulser->server_record,
		"%s.stop", network_pulser->remote_record_name );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_pulser_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_network_pulser_print_structure()";

	MX_PULSE_GENERATOR *pulse_generator;
	MX_NETWORK_PULSER *network_pulser;
	double pulse_period, pulse_width, pulse_delay;
	mx_length_type num_pulses;
	int32_t pulse_mode;
	mx_bool_type busy;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_RECORD pointer passed was NULL." );
	}

	pulse_generator = (MX_PULSE_GENERATOR *) record->record_class_struct;

	mx_status = mxd_network_pulser_get_pointers( pulse_generator,
						&network_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "PULSE GENERATOR parameters for record '%s':\n",
				record->name );
	fprintf(file, "  Pulse generator type = network_pulser\n\n" );
	fprintf(file, "  name                 = %s\n", record->name );

	mx_status = mx_pulse_generator_get_pulse_period(record, &pulse_period);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "  pulse_period         = %g sec\n", pulse_period );

	mx_status = mx_pulse_generator_get_pulse_width( record, &pulse_width );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "  pulse_width          = %g sec\n", pulse_width );

	mx_status = mx_pulse_generator_get_num_pulses( record, &num_pulses);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "  num_pulses           = %lu\n",
					(unsigned long) num_pulses );

	mx_status = mx_pulse_generator_get_pulse_delay( record, &pulse_delay );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "  pulse_delay          = %g sec\n", pulse_delay );

	mx_status = mx_pulse_generator_get_mode( record, &pulse_mode);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "  mode                 = %d\n", pulse_mode );

	mx_status = mx_pulse_generator_is_busy( record, &busy);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "  busy                 = %d\n", busy );

	return MX_SUCCESSFUL_RESULT;
}

/*-------------------------------------------------------------------------*/

MX_EXPORT mx_status_type
mxd_network_pulser_busy( MX_PULSE_GENERATOR *pulse_generator )
{
	static const char fname[] = "mxd_network_pulser_busy()";

	MX_NETWORK_PULSER *network_pulser;
	mx_bool_type busy;
	mx_status_type mx_status;

	mx_status = mxd_network_pulser_get_pointers( pulse_generator,
						&network_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s'.",
			fname, pulse_generator->record->name));

	mx_status = mx_get( &(network_pulser->busy_nf), MXFT_BOOL, &busy );

	pulse_generator->busy = busy;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_pulser_start( MX_PULSE_GENERATOR *pulse_generator )
{
	static const char fname[] = "mxd_network_pulser_start()";

	MX_NETWORK_PULSER *network_pulser;
	mx_bool_type start;
	mx_status_type mx_status;

	mx_status = mxd_network_pulser_get_pointers( pulse_generator,
						&network_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s'.",
			fname, pulse_generator->record->name));

	start = TRUE;

	mx_status = mx_put( &(network_pulser->start_nf), MXFT_BOOL, &start );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_pulser_stop( MX_PULSE_GENERATOR *pulse_generator )
{
	static const char fname[] = "mxd_network_pulser_stop()";

	MX_NETWORK_PULSER *network_pulser;
	mx_bool_type stop;
	mx_status_type mx_status;

	mx_status = mxd_network_pulser_get_pointers( pulse_generator,
						&network_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s invoked for record '%s'.",
			fname, pulse_generator->record->name));

	stop = TRUE;

	mx_status = mx_put( &(network_pulser->stop_nf), MXFT_BOOL, &stop );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_pulser_get_parameter( MX_PULSE_GENERATOR *pulse_generator )
{
	static const char fname[] = "mxd_network_pulser_get_parameter()";

	MX_NETWORK_PULSER *network_pulser;
	mx_status_type mx_status;

	mx_status = mxd_network_pulser_get_pointers( pulse_generator,
						&network_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,
	("%s invoked for PULSE_GENERATOR '%s', parameter type '%s' (%d)",
		fname, pulse_generator->record->name,
		mx_get_field_label_string( pulse_generator->record,
					pulse_generator->parameter_type ),
		pulse_generator->parameter_type));

	switch( pulse_generator->parameter_type ) {
	case MXLV_PGN_NUM_PULSES:
		mx_status = mx_get( &(network_pulser->num_pulses_nf),
				MXFT_UINT32, &(pulse_generator->num_pulses) );

		break;
	case MXLV_PGN_PULSE_WIDTH:
		mx_status = mx_get( &(network_pulser->pulse_width_nf),
				MXFT_DOUBLE, &(pulse_generator->pulse_width) );

		break;
	case MXLV_PGN_PULSE_DELAY:
		mx_status = mx_get( &(network_pulser->pulse_delay_nf),
				MXFT_DOUBLE, &(pulse_generator->pulse_delay) );

		break;
	case MXLV_PGN_MODE:
		mx_status = mx_get( &(network_pulser->mode_nf),
					MXFT_INT32, &(pulse_generator->mode) );

		break;
	case MXLV_PGN_PULSE_PERIOD:
		mx_status = mx_get( &(network_pulser->pulse_period_nf),
				MXFT_DOUBLE, &(pulse_generator->pulse_period) );

		break;
	default:
		return mx_pulse_generator_default_get_parameter_handler(
							pulse_generator );
		break;
	}
	MX_DEBUG( 2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_pulser_set_parameter( MX_PULSE_GENERATOR *pulse_generator )
{
	static const char fname[] = "mxd_network_pulser_set_parameter()";

	MX_NETWORK_PULSER *network_pulser;
	mx_status_type mx_status;

	mx_status = mxd_network_pulser_get_pointers( pulse_generator,
						&network_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,
	("%s invoked for PULSE_GENERATOR '%s', parameter type '%s' (%d)",
		fname, pulse_generator->record->name,
		mx_get_field_label_string( pulse_generator->record,
					pulse_generator->parameter_type ),
		pulse_generator->parameter_type));

	switch( pulse_generator->parameter_type ) {
	case MXLV_PGN_NUM_PULSES:
		mx_status = mx_put( &(network_pulser->num_pulses_nf),
				MXFT_UINT32, &(pulse_generator->num_pulses) );

		break;
	case MXLV_PGN_PULSE_WIDTH:
		mx_status = mx_put( &(network_pulser->pulse_width_nf),
				MXFT_DOUBLE, &(pulse_generator->pulse_width) );

		break;
	case MXLV_PGN_PULSE_DELAY:
		mx_status = mx_put( &(network_pulser->pulse_delay_nf),
				MXFT_DOUBLE, &(pulse_generator->pulse_delay) );

		break;
	case MXLV_PGN_MODE:
		mx_status = mx_put( &(network_pulser->mode_nf),
					MXFT_INT32, &(pulse_generator->mode) );

		break;
	case MXLV_PGN_PULSE_PERIOD:
		mx_status = mx_put( &(network_pulser->pulse_period_nf),
				MXFT_DOUBLE, &(pulse_generator->pulse_period) );

		break;
	default:
		return mx_pulse_generator_default_set_parameter_handler(
							pulse_generator );
		break;
	}
	MX_DEBUG( 2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

