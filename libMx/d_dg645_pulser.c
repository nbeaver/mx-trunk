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
 * Copyright 2017 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#define MXD_DG645_PULSER_DEBUG	TRUE

#include <stdio.h>
#include <stdlib.h>

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
	mxd_dg645_pulser_is_busy,
	mxd_dg645_pulser_start,
	mxd_dg645_pulser_stop,
	mxd_dg645_pulser_get_parameter,
	mxd_dg645_pulser_set_parameter
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

	/* Read the pulser parameters for this channel. */

	pulser->parameter_type = MXLV_PGN_PULSE_PERIOD;

	mx_status = mxd_dg645_pulser_get_parameter( pulser );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	pulser->parameter_type = MXLV_PGN_PULSE_WIDTH;

	mx_status = mxd_dg645_pulser_get_parameter( pulser );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	pulser->parameter_type = MXLV_PGN_NUM_PULSES;

	mx_status = mxd_dg645_pulser_get_parameter( pulser );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	pulser->parameter_type = MXLV_PGN_PULSE_DELAY;

	mx_status = mxd_dg645_pulser_get_parameter( pulser );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	pulser->parameter_type = MXLV_PGN_MODE;

	mx_status = mxd_dg645_pulser_get_parameter( pulser );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_dg645_pulser_is_busy( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_dg645_pulser_is_busy()";

	MX_DG645_PULSER *dg645_pulser = NULL;
	MX_DG645 *dg645 = NULL;
	mx_status_type mx_status;

	mx_status = mxd_dg645_pulser_get_pointers( pulser,
					&dg645_pulser, &dg645, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* FIXME: As far as I can tell, the DG645 is always busy unless
	 * it is in a single shot mode and the single shot has already
	 * happened.  I am not yet sure how you detect that.
	 */

	pulser->busy = TRUE;

#if MXD_DG645_PULSER_DEBUG
	MX_DEBUG(-2,("%s: pulser '%s', busy = %d",
		fname, pulser->record->name, (int) pulser->busy));
#endif

	return mx_status;
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

	if ( single_shot_mode ) {
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
	case MXLV_PGN_NUM_PULSES:
		break;

	case MXLV_PGN_PULSE_WIDTH:
		break;

	case MXLV_PGN_PULSE_DELAY:
		pulser->pulse_delay = 0;
		break;

	case MXLV_PGN_MODE:
		pulser->mode = MXF_PGN_SQUARE_WAVE;
		break;

	case MXLV_PGN_PULSE_PERIOD:
		break;

	default:
		return
		    mx_pulse_generator_default_get_parameter_handler( pulser );
	}

#if MXD_DG645_PULSER_DEBUG
	MX_DEBUG(-2,("%s complete.", fname));
#endif

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_dg645_pulser_set_parameter( MX_PULSE_GENERATOR *pulser )
{
	static const char fname[] = "mxd_dg645_pulser_set_parameter()";

	MX_DG645_PULSER *dg645_pulser = NULL;
	MX_DG645 *dg645 = NULL;
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
	case MXLV_PGN_NUM_PULSES:
		break;

	case MXLV_PGN_PULSE_WIDTH:
		break;

	case MXLV_PGN_PULSE_DELAY:
		pulser->pulse_delay = 0;
		break;

	case MXLV_PGN_MODE:
		pulser->mode = MXF_PGN_SQUARE_WAVE;
		break;

	case MXLV_PGN_PULSE_PERIOD:
		break;

	default:
		return
		    mx_pulse_generator_default_set_parameter_handler( pulser );
	}
	MX_DEBUG( 2,("%s complete.", fname));

	return MX_SUCCESSFUL_RESULT;
}

