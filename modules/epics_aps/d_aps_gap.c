/*
 * Name:    d_aps_gap.c 
 *
 * Purpose: MX motor driver for controlling an Advanced Photon Source
 *          undulator gap.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 1999-2003, 2005-2006, 2008-2009, 2011, 2014-2015
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_epics.h"
#include "mx_motor.h"
#include "d_aps_gap.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_aps_gap_record_function_list = {
	NULL,
	mxd_aps_gap_create_record_structures,
	mxd_aps_gap_finish_record_initialization,
	NULL,
	mxd_aps_gap_print_structure
};

MX_MOTOR_FUNCTION_LIST mxd_aps_gap_motor_function_list = {
	NULL,
	mxd_aps_gap_move_absolute,
	mxd_aps_gap_get_position,
	NULL,
	mxd_aps_gap_soft_abort,
	NULL,
	NULL,
	NULL,
	NULL,
	NULL,
	mx_motor_default_get_parameter_handler,
	mxd_aps_gap_set_parameter,
	NULL,
	mxd_aps_gap_get_status
};

/* APS gap motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_aps_gap_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_APS_GAP_STANDARD_FIELDS
};

long mxd_aps_gap_num_record_fields
		= sizeof( mxd_aps_gap_record_field_defaults )
			/ sizeof( mxd_aps_gap_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_aps_gap_record_field_def_ptr
			= &mxd_aps_gap_record_field_defaults[0];

/* A private function for the use of the driver. */

static mx_status_type
mxd_aps_gap_get_pointers( MX_MOTOR *motor,
			MX_APS_GAP **aps_gap,
			const char *calling_fname )
{
	static const char fname[] = "mxd_aps_gap_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( aps_gap == (MX_APS_GAP **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_APS_GAP pointer passed by '%s' was NULL.",
			calling_fname );
	}

	if ( motor->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
    "The MX_RECORD pointer for the MX_MOTOR pointer passed by '%s' is NULL.",
			calling_fname );
	}

	*aps_gap = (MX_APS_GAP *) motor->record->record_type_struct;

	if ( *aps_gap == (MX_APS_GAP *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_APS_GAP pointer for record '%s' is NULL.",
			motor->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_aps_gap_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_aps_gap_create_record_structures()";

	MX_MOTOR *motor;
	MX_APS_GAP *aps_gap;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	aps_gap = (MX_APS_GAP *) malloc( sizeof(MX_APS_GAP) );

	if ( aps_gap == (MX_APS_GAP *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_APS_GAP structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = aps_gap;
	record->class_specific_function_list
				= &mxd_aps_gap_motor_function_list;

	motor->record = record;

	/* An APS gap motor is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aps_gap_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] = "mxd_aps_gap_finish_record_initialization()";

	MX_APS_GAP *aps_gap;
	mx_status_type mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	strlcpy( record->network_type_name, "epics",
				MXU_NETWORK_TYPE_NAME_LENGTH );

	aps_gap = (MX_APS_GAP *) (record->record_type_struct);

	if ( aps_gap == (MX_APS_GAP *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_APS_GAP pointer for record '%s' is NULL.",
			record->name );
	}

	if ( aps_gap->sector_number < 1 || aps_gap->sector_number > 35 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal APS sector number %ld.  Allowed range is 1 to 35.",
			aps_gap->sector_number );
	}

	/* Initialize EPICS PV data structures used by this record. */

	switch( aps_gap->motor_subtype ) {
	case MXT_APS_GAP_MM:
		mx_epics_pvname_init( &(aps_gap->position_pv),
			"ID%02dds:Gap.VAL", aps_gap->sector_number );

		mx_epics_pvname_init( &(aps_gap->destination_pv),
			"ID%02dds:GapSet.VAL", aps_gap->sector_number );
		break;

	case (-MXT_APS_GAP_MM):
		mx_epics_pvname_init( &(aps_gap->position_pv),
			"ID%02dus:Gap.VAL", aps_gap->sector_number );

		mx_epics_pvname_init( &(aps_gap->destination_pv),
			"ID%02dus:GapSet.VAL", aps_gap->sector_number );
		break;

	case MXT_APS_GAP_KEV:
		mx_epics_pvname_init( &(aps_gap->position_pv),
			"ID%02dds:Energy.VAL", aps_gap->sector_number );

		mx_epics_pvname_init( &(aps_gap->destination_pv),
			"ID%02dds:EnergySet.VAL", aps_gap->sector_number);
		break;

	case (-MXT_APS_GAP_KEV):
		mx_epics_pvname_init( &(aps_gap->position_pv),
			"ID%02dus:Energy.VAL", aps_gap->sector_number );

		mx_epics_pvname_init( &(aps_gap->destination_pv),
			"ID%02dus:EnergySet.VAL", aps_gap->sector_number);
		break;

	case MXT_APS_TAPER_MM:
		mx_epics_pvname_init( &(aps_gap->position_pv),
			"ID%02dds:TaperGap.VAL", aps_gap->sector_number );

		mx_epics_pvname_init( &(aps_gap->destination_pv),
			"ID%02dds:TaperGapSet.VAL", aps_gap->sector_number );
		break;

	case (-MXT_APS_TAPER_MM):
		mx_epics_pvname_init( &(aps_gap->position_pv),
			"ID%02dus:TaperGap.VAL", aps_gap->sector_number );

		mx_epics_pvname_init( &(aps_gap->destination_pv),
			"ID%02dus:TaperGapSet.VAL", aps_gap->sector_number );
		break;

	case MXT_APS_TAPER_KEV:
		mx_epics_pvname_init( &(aps_gap->position_pv),
			"ID%02dds:TaperEnergy.VAL", aps_gap->sector_number );

		mx_epics_pvname_init( &(aps_gap->destination_pv),
			"ID%02dds:TaperEnergySet.VAL", aps_gap->sector_number);
		break;

	case (-MXT_APS_TAPER_KEV):
		mx_epics_pvname_init( &(aps_gap->position_pv),
			"ID%02dus:TaperEnergy.VAL", aps_gap->sector_number );

		mx_epics_pvname_init( &(aps_gap->destination_pv),
			"ID%02dus:TaperEnergySet.VAL", aps_gap->sector_number);
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal APS gap motor subtype %ld.", aps_gap->motor_subtype );
		break;
	}

	mx_epics_pvname_init( &(aps_gap->busy_pv),
			"ID%02d:Busy.VAL", aps_gap->sector_number );

	mx_epics_pvname_init( &(aps_gap->access_security_pv),
			"ID%02d:AccessSecurity.VAL", aps_gap->sector_number );

	mx_epics_pvname_init( &(aps_gap->start_pv),
			"ID%02d:Start.VAL", aps_gap->sector_number );

	mx_epics_pvname_init( &(aps_gap->stop_pv),
			"ID%02d:Stop.VAL", aps_gap->sector_number );

	/* The following is used by the 'epics_scaler_mce' driver. */

	aps_gap->epics_position_pv_ptr = (char *) &(aps_gap->position_pv);

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aps_gap_print_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_aps_gap_print_structure()";

	MX_MOTOR *motor;
	MX_APS_GAP *aps_gap = NULL;
	char raw_units[4];
	double position, move_deadband;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_aps_gap_get_pointers( motor, &aps_gap, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);
	fprintf(file, "  Motor type      = APS_GAP.\n\n");

	fprintf(file, "  name            = %s\n", record->name);
	fprintf(file, "  sector number   = %ld\n", aps_gap->sector_number);
	fprintf(file, "  motor subtype   = %ld\n", aps_gap->motor_subtype);

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of motor '%s'",
			record->name );
	}

	switch( aps_gap->motor_subtype ) {
	case MXT_APS_GAP_MM:
	case MXT_APS_TAPER_MM:
		strlcpy( raw_units, "mm", sizeof(raw_units) );
		break;
	case MXT_APS_GAP_KEV:
	case MXT_APS_TAPER_KEV:
		strlcpy( raw_units, "keV", sizeof(raw_units) );
		break;
	default:
		strlcpy( raw_units, "??", sizeof(raw_units) );
		break;
	}

	fprintf(file, "  position        = %g %s  (%g %s)\n",
		motor->position, motor->units,
		motor->raw_position.analog, raw_units );
	fprintf(file, "  scale           = %g %s per %s.\n",
		motor->scale, motor->units, raw_units );
	fprintf(file, "  offset          = %g %s.\n",
		motor->offset, motor->units);

	fprintf(file, "  backlash        = %g %s  (%g %s).\n",
		motor->backlash_correction, motor->units,
		motor->raw_backlash_correction.analog, raw_units );
	fprintf(file, "  negative limit  = %g %s  (%g %s).\n",
		motor->negative_limit, motor->units,
		motor->raw_negative_limit.analog, raw_units );
	fprintf(file, "  positive limit  = %g %s  (%g %s).\n",
		motor->positive_limit, motor->units,
		motor->raw_positive_limit.analog, raw_units );

	move_deadband = motor->scale * motor->raw_move_deadband.analog;

	fprintf(file, "  move deadband   = %g %s  (%g %s).\n\n",
		move_deadband, motor->units,
		motor->raw_move_deadband.analog, raw_units );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aps_gap_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_aps_gap_move_absolute()";

	MX_APS_GAP *aps_gap = NULL;
	double new_destination;
	int sector;
	int32_t access_mode, move_command;
	mx_status_type mx_status;

	mx_status = mxd_aps_gap_get_pointers( motor, &aps_gap, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	sector = aps_gap->sector_number;

	new_destination = motor->raw_destination.analog;

	/* Do we have permission to change the undulator gap right now? */

	mx_status = mx_caget( &(aps_gap->access_security_pv),
				MX_CA_LONG, 1, &access_mode );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( access_mode != 0 ) {
		return mx_error( MXE_PERMISSION_DENIED, fname,
"The sector %d insertion device is not in user control mode right now.",
			sector );
	}

	/* Send the new destination value. */

	mx_status = mx_caput( &(aps_gap->destination_pv),
				MX_CA_DOUBLE, 1, &new_destination );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Finally, tell the undulator to start moving. */

	move_command = 1;

	mx_status = mx_caput( &(aps_gap->start_pv),
				MX_CA_LONG, 1, &move_command );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_aps_gap_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_aps_gap_get_position()";

	MX_APS_GAP *aps_gap = NULL;
	double raw_position;
	mx_status_type mx_status;

	mx_status = mxd_aps_gap_get_pointers( motor, &aps_gap, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_caget( &(aps_gap->position_pv),
				MX_CA_DOUBLE, 1, &raw_position );

	motor->raw_position.analog = raw_position;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aps_gap_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_aps_gap_soft_abort()";

	MX_APS_GAP *aps_gap = NULL;
	short stop_field;
	int32_t access_mode;
	mx_status_type mx_status;

	mx_status = mxd_aps_gap_get_pointers( motor, &aps_gap, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Do we have permission to write to the stop field right now? */

	mx_status = mx_caget( &(aps_gap->access_security_pv),
				MX_CA_LONG, 1, &access_mode );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( access_mode != 0 ) {
		/* If we do not have permission to stop an undulator motion,
		 * then we just return without generating an error.
		 */

		return MX_SUCCESSFUL_RESULT;
	}

	/* Stop the gap change. */

	stop_field = 1;		/* Stop */

	mx_status = mx_caput( &(aps_gap->stop_pv),
				MX_CA_SHORT, 1, &stop_field );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_aps_gap_set_parameter( MX_MOTOR *motor )
{
	switch( motor->parameter_type ) {
	case MXLV_MTR_SAVE_SPEED:
	case MXLV_MTR_RESTORE_SPEED:
		/* Treat these calls as no-ops, since we cannot directly
		 * control the speed of the APS undulator gaps.
		 */

		break;
	default:
		return mx_motor_default_set_parameter_handler( motor );
		break;
	}
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aps_gap_get_status( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_aps_gap_get_status()";

	MX_APS_GAP *aps_gap = NULL;
	short busy_record_value;
	mx_status_type mx_status;

	mx_status = mxd_aps_gap_get_pointers( motor, &aps_gap, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->busy = FALSE;

	mx_status = mx_caget( &(aps_gap->busy_pv), 
				MX_CA_SHORT, 1, &busy_record_value );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( busy_record_value == 0 ) {
		motor->status = 0;
	} else {
		motor->status = MXSF_MTR_IS_BUSY;
	}

	return MX_SUCCESSFUL_RESULT;
}

