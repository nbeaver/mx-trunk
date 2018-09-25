/*
 * Name:    d_network_pulser.c
 *
 * Purpose: MX network pulse generator driver.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2002-2007, 2014-2016, 2018 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_NETWORK_PULSER_DEBUG	FALSE

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
	mxd_network_pulser_finish_record_initialization,
	NULL,
	NULL,
	mxd_network_pulser_open
};

MX_PULSE_GENERATOR_FUNCTION_LIST
			mxd_network_pulser_pulse_generator_function_list = {
	mxd_network_pulser_busy,
	mxd_network_pulser_arm,
	mxd_network_pulser_trigger,
	mxd_network_pulser_stop,
	mxd_network_pulser_abort,
	mxd_network_pulser_get_parameter,
	mxd_network_pulser_set_parameter,
	mxd_network_pulser_setup,
	mxd_network_pulser_get_status
};

MX_RECORD_FIELD_DEFAULTS mxd_network_pulser_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_PULSE_GENERATOR_STANDARD_FIELDS,
	MXD_NETWORK_PULSER_STANDARD_FIELDS
};

long mxd_network_pulser_num_record_fields
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
	static const char fname[] =
		"mxd_network_pulser_create_record_structures()";

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

	network_pulser = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_RECORD pointer passed was NULL." );
	}

	pulse_generator = (MX_PULSE_GENERATOR *) record->record_class_struct;

	mx_status = mxd_network_pulser_get_pointers(
				pulse_generator, &network_pulser, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	strlcpy(record->network_type_name, "mx", MXU_NETWORK_TYPE_NAME_LENGTH);

	mx_network_field_init( &(network_pulser->abort_nf),
		network_pulser->server_record,
		"%s.abort", network_pulser->remote_record_name );

	mx_network_field_init( &(network_pulser->arm_nf),
		network_pulser->server_record,
		"%s.arm", network_pulser->remote_record_name );

	mx_network_field_init( &(network_pulser->busy_nf),
		network_pulser->server_record,
		"%s.busy", network_pulser->remote_record_name );

	mx_network_field_init( &(network_pulser->function_mode_nf),
		network_pulser->server_record,
		"%s.function_mode", network_pulser->remote_record_name );

	mx_network_field_init( &(network_pulser->last_pulse_number_nf),
		network_pulser->server_record,
		"%s.last_pulse_number", network_pulser->remote_record_name );

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

	mx_network_field_init( &(network_pulser->setup_nf),
		network_pulser->server_record,
		"%s.setup", network_pulser->remote_record_name );

	mx_network_field_init( &(network_pulser->start_nf),
		network_pulser->server_record,
		"%s.start", network_pulser->remote_record_name );

	mx_network_field_init( &(network_pulser->status_nf),
		network_pulser->server_record,
		"%s.status", network_pulser->remote_record_name );

	mx_network_field_init( &(network_pulser->stop_nf),
		network_pulser->server_record,
		"%s.stop", network_pulser->remote_record_name );

	mx_network_field_init( &(network_pulser->trigger_nf),
		network_pulser->server_record,
		"%s.trigger", network_pulser->remote_record_name );

	mx_network_field_init( &(network_pulser->trigger_mode_nf),
		network_pulser->server_record,
		"%s.trigger_mode", network_pulser->remote_record_name );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_pulser_open( MX_RECORD *record )
{
	static const char fname[] = "mxd_network_pulser_open()";

	MX_PULSE_GENERATOR *pulse_generator;
	MX_NETWORK_PULSER *network_pulser;
	MX_NETWORK_SERVER *network_server;
	MX_PULSE_GENERATOR_FUNCTION_LIST *pg_flist;
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

	if ( network_pulser->server_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The server_record pointer for network pulser '%s' is NULL.",
			record->name );
	}

	network_server = (MX_NETWORK_SERVER *)
			network_pulser->server_record->record_class_struct;

	if ( network_server == (MX_NETWORK_SERVER *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_NETWORK_SERVER pointer for server '%s' "
		"used by network pulser '%s' is NULL.",
			network_pulser->server_record->name,
			record->name );
	}

	/* The network pulse generator 'setup' field did not exist
	 * until MX 2.0.1.  If the remote MX server is running an
	 * older version of MX, then prevent this client from trying
	 * to use the 'setup' field by overwriting the 'setup' method
	 * pointer in the MX_PULSE_GENERATOR_FUNCTION_LIST table.
	 */

#if MXD_NETWORK_PULSER_DEBUG
	MX_DEBUG(-2,("%s: '%s' server version is %lu",
		fname, record->name, network_server->remote_mx_version ));
#endif

	if ( network_server->remote_mx_version < 2000001L ) {

#if MXD_NETWORK_PULSER_DEBUG
		MX_DEBUG(-2,
		("%s: '%s' setup method disabled due to old MX server.",
			fname, record->name ));
#endif

		pg_flist = (MX_PULSE_GENERATOR_FUNCTION_LIST *)
				record->class_specific_function_list;

		if ( pg_flist == (MX_PULSE_GENERATOR_FUNCTION_LIST *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"The MX_PULSE_GENERATOR_FUNCTION_LIST pointer for "
			"network pulser '%s' is NULL.", record->name );
		}

		/* Disable the setup method. */

		pg_flist->setup = NULL;
	}

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

	network_pulser = NULL;

	mx_status = mxd_network_pulser_get_pointers( pulse_generator,
						&network_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_PULSER_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, pulse_generator->record->name));
#endif

	mx_status = mx_get( &(network_pulser->busy_nf), MXFT_BOOL, &busy );

	pulse_generator->busy = busy;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_pulser_arm( MX_PULSE_GENERATOR *pulse_generator )
{
	static const char fname[] = "mxd_network_pulser_arm()";

	MX_NETWORK_PULSER *network_pulser;
	mx_bool_type arm;
	mx_status_type mx_status;

	network_pulser = NULL;

	mx_status = mxd_network_pulser_get_pointers( pulse_generator,
						&network_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_PULSER_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, pulse_generator->record->name));
#endif

	arm = TRUE;

	mx_status = mx_put( &(network_pulser->arm_nf), MXFT_BOOL, &arm );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_pulser_trigger( MX_PULSE_GENERATOR *pulse_generator )
{
	static const char fname[] = "mxd_network_pulser_trigger()";

	MX_NETWORK_PULSER *network_pulser;
	mx_bool_type trigger;
	mx_status_type mx_status;

	network_pulser = NULL;

	mx_status = mxd_network_pulser_get_pointers( pulse_generator,
						&network_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_PULSER_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, pulse_generator->record->name));
#endif

	trigger = TRUE;

	mx_status = mx_put( &(network_pulser->trigger_nf),
			MXFT_BOOL, &trigger );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_pulser_start( MX_PULSE_GENERATOR *pulse_generator )
{
	static const char fname[] = "mxd_network_pulser_start()";

	MX_NETWORK_PULSER *network_pulser;
	mx_bool_type start;
	mx_status_type mx_status;

	network_pulser = NULL;

	mx_status = mxd_network_pulser_get_pointers( pulse_generator,
						&network_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_PULSER_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, pulse_generator->record->name));
#endif

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

	network_pulser = NULL;

	mx_status = mxd_network_pulser_get_pointers( pulse_generator,
						&network_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_PULSER_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, pulse_generator->record->name));
#endif

	stop = TRUE;

	mx_status = mx_put( &(network_pulser->stop_nf), MXFT_BOOL, &stop );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_pulser_abort( MX_PULSE_GENERATOR *pulse_generator )
{
	static const char fname[] = "mxd_network_pulser_abort()";

	MX_NETWORK_PULSER *network_pulser;
	mx_bool_type abort;
	mx_status_type mx_status;

	network_pulser = NULL;

	mx_status = mxd_network_pulser_get_pointers( pulse_generator,
						&network_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_PULSER_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, pulse_generator->record->name));
#endif

	abort = TRUE;

	mx_status = mx_put( &(network_pulser->abort_nf), MXFT_BOOL, &abort );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_pulser_get_parameter( MX_PULSE_GENERATOR *pulse_generator )
{
	static const char fname[] = "mxd_network_pulser_get_parameter()";

	MX_NETWORK_PULSER *network_pulser;
	mx_status_type mx_status;

	network_pulser = NULL;

	mx_status = mxd_network_pulser_get_pointers( pulse_generator,
						&network_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_PULSER_DEBUG
	MX_DEBUG( 2,
	("%s invoked for PULSE_GENERATOR '%s', parameter type '%s' (%ld)",
		fname, pulse_generator->record->name,
		mx_get_field_label_string( pulse_generator->record,
					pulse_generator->parameter_type ),
		pulse_generator->parameter_type));
#endif

	switch( pulse_generator->parameter_type ) {
	case MXLV_PGN_FUNCTION_MODE:
		mx_status = mx_get( &(network_pulser->function_mode_nf),
				MXFT_LONG, &(pulse_generator->function_mode) );

		break;
	case MXLV_PGN_LAST_PULSE_NUMBER:
		mx_status = mx_get( &(network_pulser->last_pulse_number_nf),
			MXFT_LONG, &(pulse_generator->last_pulse_number) );
		break;
	case MXLV_PGN_NUM_PULSES:
		mx_status = mx_get( &(network_pulser->num_pulses_nf),
				MXFT_ULONG, &(pulse_generator->num_pulses) );

		break;
	case MXLV_PGN_PULSE_WIDTH:
		mx_status = mx_get( &(network_pulser->pulse_width_nf),
				MXFT_DOUBLE, &(pulse_generator->pulse_width) );

		break;
	case MXLV_PGN_PULSE_DELAY:
		mx_status = mx_get( &(network_pulser->pulse_delay_nf),
				MXFT_DOUBLE, &(pulse_generator->pulse_delay) );

		break;
	case MXLV_PGN_PULSE_PERIOD:
		mx_status = mx_get( &(network_pulser->pulse_period_nf),
				MXFT_DOUBLE, &(pulse_generator->pulse_period) );

		break;
	case MXLV_PGN_TRIGGER_MODE:
		mx_status = mx_get( &(network_pulser->trigger_mode_nf),
				MXFT_LONG, &(pulse_generator->trigger_mode) );

		break;
	default:
		return mx_pulse_generator_default_get_parameter_handler(
							pulse_generator );
	}

#if MXD_NETWORK_PULSER_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_pulser_set_parameter( MX_PULSE_GENERATOR *pulse_generator )
{
	static const char fname[] = "mxd_network_pulser_set_parameter()";

	MX_NETWORK_PULSER *network_pulser;
	mx_status_type mx_status;

	network_pulser = NULL;

	mx_status = mxd_network_pulser_get_pointers( pulse_generator,
						&network_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_PULSER_DEBUG
	MX_DEBUG(-2,
	("%s invoked for PULSE_GENERATOR '%s', parameter type '%s' (%ld)",
		fname, pulse_generator->record->name,
		mx_get_field_label_string( pulse_generator->record,
					pulse_generator->parameter_type ),
		pulse_generator->parameter_type));
#endif

	switch( pulse_generator->parameter_type ) {
	case MXLV_PGN_FUNCTION_MODE:
		mx_status = mx_put( &(network_pulser->function_mode_nf),
				MXFT_LONG, &(pulse_generator->function_mode) );

		break;
	case MXLV_PGN_NUM_PULSES:
		mx_status = mx_put( &(network_pulser->num_pulses_nf),
				MXFT_ULONG, &(pulse_generator->num_pulses) );

		break;
	case MXLV_PGN_PULSE_WIDTH:
		mx_status = mx_put( &(network_pulser->pulse_width_nf),
				MXFT_DOUBLE, &(pulse_generator->pulse_width) );

		break;
	case MXLV_PGN_PULSE_DELAY:
		mx_status = mx_put( &(network_pulser->pulse_delay_nf),
				MXFT_DOUBLE, &(pulse_generator->pulse_delay) );

		break;
	case MXLV_PGN_PULSE_PERIOD:
		mx_status = mx_put( &(network_pulser->pulse_period_nf),
				MXFT_DOUBLE, &(pulse_generator->pulse_period) );

		break;
	case MXLV_PGN_TRIGGER_MODE:
		mx_status = mx_put( &(network_pulser->trigger_mode_nf),
				MXFT_LONG, &(pulse_generator->trigger_mode) );

		break;
	default:
		return mx_pulse_generator_default_set_parameter_handler(
							pulse_generator );
	}

#if MXD_NETWORK_PULSER_DEBUG
	MX_DEBUG( 2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_network_pulser_setup( MX_PULSE_GENERATOR *pulse_generator )
{
	static const char fname[] = "mxd_network_pulser_setup()";

	MX_NETWORK_PULSER *network_pulser;
	long dimension[1];
	mx_status_type mx_status;

	network_pulser = NULL;

	mx_status = mxd_network_pulser_get_pointers( pulse_generator,
						&network_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_PULSER_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, pulse_generator->record->name));
#endif

	dimension[0] = MXU_PGN_NUM_SETUP_PARAMETERS;

	mx_status = mx_put_array( &(network_pulser->setup_nf),
			MXFT_DOUBLE, 1, dimension, pulse_generator->setup );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_network_pulser_get_status( MX_PULSE_GENERATOR *pulse_generator )
{
	static const char fname[] = "mxd_network_pulser_get_status()";

	MX_NETWORK_PULSER *network_pulser;
	long dimension[1];
	mx_status_type mx_status;

	network_pulser = NULL;

	mx_status = mxd_network_pulser_get_pointers( pulse_generator,
						&network_pulser, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if MXD_NETWORK_PULSER_DEBUG
	MX_DEBUG(-2,("%s invoked for record '%s'.",
			fname, pulse_generator->record->name));
#endif

	dimension[0] = MXU_PGN_NUM_SETUP_PARAMETERS;

	mx_status = mx_put_array( &(network_pulser->status_nf),
		MXFT_DOUBLE, 1, dimension, &(pulse_generator->status) );

	return mx_status;
}

