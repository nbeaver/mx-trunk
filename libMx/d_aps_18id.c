/*
 * Name:    d_aps_18id.c 
 *
 * Purpose: MX motor driver to control the BIOCAT Sector 18-ID monochromator
 *          at the Advanced Photon Source.  This is a pseudomotor that 
 *          pretends to be the monochromator 'Energy' axis.  The value
 *          written to this pseudo motor is used to control the real 'Bragg'
 *          axis and any other MX devices that are tied into this pseudomotor.
 *
 *          This is a modification of Bill's d_s18id.c in order to move the
 *          monochromator with the Piezo motors.  The options provided here 
 *          include that we could also move the Bragg angle without TrolleyX,
 *          or TrolleyY, or both.
 *                            - Shengke Wang
 *
 * Authors: William Lavender and Shengke Wang
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2003, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <time.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_constants.h"
#include "mx_variable.h"
#include "mx_array.h"
#include "mx_motor.h"
#include "d_aps_18id.h"
#include "mx_scaler.h"
#include "mx_timer.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_aps_18id_motor_record_function_list = {
	mxd_aps_18id_motor_initialize_type,
	mxd_aps_18id_motor_create_record_structures,
	mxd_aps_18id_motor_finish_record_initialization,
	mxd_aps_18id_motor_delete_record,
	mxd_aps_18id_motor_print_motor_structure,
	mxd_aps_18id_motor_read_parms_from_hardware,
	mxd_aps_18id_motor_write_parms_to_hardware,
	mxd_aps_18id_motor_open,
	mxd_aps_18id_motor_close
};

MX_MOTOR_FUNCTION_LIST mxd_aps_18id_motor_motor_function_list = {
	mxd_aps_18id_motor_motor_is_busy,
	mxd_aps_18id_motor_move_absolute,
	mxd_aps_18id_motor_get_position,
	mxd_aps_18id_motor_set_position,
	mxd_aps_18id_motor_soft_abort,
	mxd_aps_18id_motor_immediate_abort,
	mxd_aps_18id_motor_positive_limit_hit,
	mxd_aps_18id_motor_negative_limit_hit,
	mxd_aps_18id_motor_find_home_position,
	mxd_aps_18id_motor_constant_velocity_move,
	mxd_aps_18id_motor_get_parameter,
	mxd_aps_18id_motor_set_parameter
};

/* === */

/* APS_18ID motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_aps_18id_motor_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_APS_18ID_MOTOR_STANDARD_FIELDS
};

mx_length_type mxd_aps_18id_motor_num_record_fields
		= sizeof( mxd_aps_18id_motor_record_field_defaults )
			/ sizeof( mxd_aps_18id_motor_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_aps_18id_motor_rfield_def_ptr
			= &mxd_aps_18id_motor_record_field_defaults[0];

/* === */

static mx_status_type
mxd_aps_18id_motor_get_pointers( MX_MOTOR *motor,
				MX_APS_18ID_MOTOR **aps_18id_motor,
				MX_RECORD **bragg_motor_record,
				MX_RECORD **tune_motor_record )
{
	static const char fname[] = "mxd_aps_18id_motor_find_home_position()";

	MX_APS_18ID_MOTOR *aps_18id_motor_ptr;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_MOTOR pointer passed was NULL.");
	}

	aps_18id_motor_ptr = (MX_APS_18ID_MOTOR *)
				(motor->record->record_type_struct);

	if ( aps_18id_motor_ptr == (MX_APS_18ID_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		    "MX_APS_18ID_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	if ( aps_18id_motor != NULL ) {
		*aps_18id_motor = aps_18id_motor_ptr;
	}

	if ( bragg_motor_record != NULL ) {
		*bragg_motor_record = aps_18id_motor_ptr->bragg_motor_record;

		if ( *bragg_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"bragg_motor_record pointer for record '%s' is NULL.",
			motor->record->name );
		}
	}

	if ( tune_motor_record != NULL ) {
		*tune_motor_record = aps_18id_motor_ptr->tune_motor_record;

		if ( *tune_motor_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"tune_motor_record pointer for record '%s' is NULL.",
				motor->record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_aps_18id_motor_initialize_type( long type )
{
		return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aps_18id_motor_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_aps_18id_motor_create_record_structures()";

	MX_MOTOR *motor;
	MX_APS_18ID_MOTOR *aps_18id_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	aps_18id_motor = (MX_APS_18ID_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( aps_18id_motor == (MX_APS_18ID_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_APS_18ID_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = aps_18id_motor;
	record->class_specific_function_list
				= &mxd_aps_18id_motor_motor_function_list;

	motor->record = record;

	/* The aps_18id pseudomotor is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aps_18id_motor_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] ="mxd_aps_18id_motor_finish_record_initialization()";

	MX_MOTOR *motor;
	MX_APS_18ID_MOTOR *aps_18id_motor;

	mx_status_type mx_status;

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor = (MX_MOTOR *) record->record_class_struct;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	motor->motor_flags |= MXF_MTR_IS_PSEUDOMOTOR;
	motor->motor_flags |= MXF_MTR_PSEUDOMOTOR_RECURSION_IS_NOT_NECESSARY;

	aps_18id_motor = (MX_APS_18ID_MOTOR *) record->record_type_struct;

	if ( aps_18id_motor == (MX_APS_18ID_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_APS_18ID_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	motor->real_motor_record = aps_18id_motor->bragg_motor_record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aps_18id_motor_delete_record( MX_RECORD *record )
{
	if ( record == NULL ) {
		return MX_SUCCESSFUL_RESULT;
	}
	if ( record->record_type_struct != NULL ) {
		free( record->record_type_struct );

		record->record_type_struct = NULL;
	}
	if ( record->record_class_struct != NULL ) {
		free( record->record_class_struct );

		record->record_class_struct = NULL;
	}
	return MX_SUCCESSFUL_RESULT;
}



MX_EXPORT mx_status_type
mxd_aps_18id_motor_print_motor_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_aps_18id_motor_print_motor_structure()";

	MX_MOTOR *motor;
	MX_APS_18ID_MOTOR *aps_18id_motor;
	double position;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_RECORD pointer passed is NULL." );
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MOTOR pointer for record '%s' is NULL.", record->name );
	}

	aps_18id_motor = (MX_APS_18ID_MOTOR *) (record->record_type_struct);

	if ( aps_18id_motor == (MX_APS_18ID_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_APS_18ID_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);
	fprintf(file, "  Motor type                     = APS_18ID_MOTOR.\n\n");

	fprintf(file, "  name                           = %s\n", record->name);
	fprintf(file, "  bragg motor                    = %s\n",
				aps_18id_motor->bragg_motor_record->name);
	fprintf(file, "  trolleyY motor                 = %s\n",
				aps_18id_motor->trolleyY_motor_record->name);
	fprintf(file, "  trolleyY linked                = %s\n",
				aps_18id_motor->trolleyY_linked_record->name);
	fprintf(file, "  trolleyX motor                 = %s\n",
				aps_18id_motor->trolleyX_motor_record->name);
	fprintf(file, "  trolleyX linked                = %s\n",
				aps_18id_motor->trolleyX_linked_record->name);
	fprintf(file, "  trolley setup                  = %s\n",
				aps_18id_motor->trolley_setup_record->name);
	fprintf(file, "  gap energy motor               = %s\n",
				aps_18id_motor->gap_energy_motor_record->name);
	fprintf(file, "  gap linked                     = %s\n",
				aps_18id_motor->gap_linked_record->name);
	fprintf(file, "  gap offset                     = %s\n",
				aps_18id_motor->gap_offset_record->name);
	fprintf(file, "  gap harmonic                   = %s\n",
				aps_18id_motor->gap_harmonic_record->name);
	fprintf(file, "  tune motor                     = %s\n",
				aps_18id_motor->tune_motor_record->name);
	fprintf(file, "  tune linked record             = %s\n",
				aps_18id_motor->tune_linked_record->name);
	fprintf(file, "  tune parameters record         = %s\n",
				aps_18id_motor->tune_parameters_record->name);
	fprintf(file, "  piezo_left motor               = %s\n",
				aps_18id_motor->piezo_left_motor_record->name);
	fprintf(file, "  piezo_right motor              = %s\n",
				aps_18id_motor->piezo_right_motor_record->name);
	fprintf(file, "  piezo enable record            = %s\n",
				aps_18id_motor->piezo_enable_record->name);
	fprintf(file, "  piezo parameters record        = %s\n",
				aps_18id_motor->piezo_parameters_record->name);
	fprintf(file, "  d spacing                      = %s\n",
				aps_18id_motor->d_spacing_record->name);

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS ) {
		return mx_error( MXE_FUNCTION_FAILED, fname,
			"Unable to read position of motor '%s'",
			record->name );
	}

	fprintf(file, "  position                       = %g %s\n",
		position, motor->units);
	fprintf(file, "  bragg scale                    = %g %s "
					"per unscaled theta unit.\n",
		motor->scale, motor->units);
	fprintf(file, "  bragg offset                   = %g %s.\n",
		motor->offset, motor->units);
	fprintf(file, "  backlash                       = %g %s.\n",
		motor->raw_backlash_correction.analog, motor->units);
	fprintf(file, "  negative limit                 = %g %s.\n",
		motor->raw_negative_limit.analog, motor->units);
	fprintf(file, "  positive limit                 = %g %s.\n\n",
	    motor->raw_positive_limit.analog, motor->units);

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aps_18id_motor_read_parms_from_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aps_18id_motor_write_parms_to_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aps_18id_motor_open( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aps_18id_motor_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aps_18id_motor_motor_is_busy( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_aps_18id_motor_motor_is_busy()";

	MX_APS_18ID_MOTOR *aps_18id_motor;
	MX_RECORD *bragg_motor_record;
	MX_RECORD *piezo_left_motor_record;
	MX_RECORD *tune_motor_record;
	MX_RECORD *gap_energy_motor_record;
	int busy;
	int32_t gap_linked;
	mx_status_type mx_status;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MOTOR pointer passed NULL.");
	}

	aps_18id_motor = (MX_APS_18ID_MOTOR *)
				(motor->record->record_type_struct);

	if ( aps_18id_motor == (MX_APS_18ID_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_APS_18ID_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	/* Check to see if bragg is busy. */

	bragg_motor_record = aps_18id_motor->bragg_motor_record;

	if ( bragg_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"bragg_motor_record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	mx_status = mx_motor_is_busy( bragg_motor_record, &busy );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: bragg_motor_record  busy = %d", fname, busy));

	if ( busy ) {
		motor->busy = TRUE;

		return MX_SUCCESSFUL_RESULT;
	}

	/* If bragg was not busy, check to see if tune is busy
		(trolleyX and Y motors use the same busy signals. */

	tune_motor_record = aps_18id_motor->tune_motor_record;

	if ( tune_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"tune_motor_record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	mx_status = mx_motor_is_busy( tune_motor_record, &busy );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: tune_motor_record  busy = %d", fname, busy));

	if ( busy ) {
		motor->busy = TRUE;

		return MX_SUCCESSFUL_RESULT;
	}

	/* If bragg and tune were not busy, check to see if piezo is busy. */

	piezo_left_motor_record = aps_18id_motor->piezo_left_motor_record;

	if ( piezo_left_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"piezo_left_motor_record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	mx_status = mx_motor_is_busy( piezo_left_motor_record, &busy );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: piezo_left_motor_record  busy = %d", fname, busy));

	if ( busy ) {
		motor->busy = TRUE;

		return MX_SUCCESSFUL_RESULT;
	}

	/* If bragg, piezo and tune were not busy, check to see if the gap motor
	 * is busy.
	 */

	/* First check to see whether or not the gap is supposed to be
	 * linked to theta motions.
	 */

	mx_status = mx_get_int32_variable_by_name( bragg_motor_record->list_head,
				aps_18id_motor->gap_linked_record->name,
				&gap_linked );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( gap_linked != 0 ) {
		gap_energy_motor_record =
			aps_18id_motor->gap_energy_motor_record;

		if ( gap_energy_motor_record == (MX_RECORD *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"gap_energy_motor_record pointer for record '%s' is NULL.",
				motor->record->name );
		}

		mx_status = mx_motor_is_busy( gap_energy_motor_record, &busy );

		MX_DEBUG( 2,("%s: tune_motor_record  busy = %d", fname, busy));
	}

	motor->busy = busy;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_aps_18id_motor_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_aps_18id_motor_move_absolute()";

	MX_APS_18ID_MOTOR *aps_18id_motor;
	MX_RECORD *list_head;
	int num_motors;
	MX_RECORD *motor_record_array[MXF_APS_18ID_NUM_REAL_MOTOR_RECORDS];
	double position_array[MXF_APS_18ID_NUM_REAL_MOTOR_RECORDS];
	MX_RECORD *trolleyY_motor_record;
	MX_RECORD *piezo_left_motor_record;
	MX_RECORD *piezo_right_motor_record;
	MX_RECORD *tune_motor_record;
	MX_RECORD *bpm_top_scaler_record;
	MX_RECORD *bpm_bottom_scaler_record;
	MX_RECORD *timer_record;
	double tune_old_position;
	double piezo_left_old_position, piezo_right_old_position, piezo_scale;
	int bragg_motor_index, trolleyY_motor_index, trolleyX_motor_index;
	int tune_motor_index, gap_energy_motor_index;
	MX_RECORD_FIELD *value_field;
	double *parameter_array;
	int32_t num_parameters;
	double bragg, trolleyY, trolleyX, tune, piezo_left, piezo_right;
	double angle_in_radians, beam_height_offset;
	int32_t piezo_enable, trolleyY_linked, trolleyX_linked;
	int32_t tune_linked, gap_linked, gap_harmonic;
	double d_spacing, gap_offset, mono_energy, new_gap_energy;
	double settling_time, integration_time, remaining_time;
	int32_t bpm_top_value, bpm_bottom_value;
	double ruler;
	mx_status_type mx_status;
	int busy;
	
	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MOTOR pointer passed NULL.");
	}

	aps_18id_motor = (MX_APS_18ID_MOTOR *)
				(motor->record->record_type_struct);

	if ( aps_18id_motor == (MX_APS_18ID_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_APS_18ID_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	list_head = motor->record->list_head;

	/* The following NULL assignments keep gcc happy. */

	trolleyY_motor_record = NULL;
	tune_motor_record = NULL;
	
	num_motors = 5;
	bragg_motor_index = 0;
	trolleyY_motor_index = 1;
	trolleyX_motor_index = 2;
	gap_energy_motor_index = 3;
	tune_motor_index = 4;
	
	/* First bragg motor which is the first indexed motor   */
	
	motor_record_array[ bragg_motor_index ] 
					= aps_18id_motor->bragg_motor_record;
					
	if (motor_record_array[ bragg_motor_index ] == (MX_RECORD *)NULL){
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"bragg motor record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	/* TrolleyY is the next motor to check since it is almost always
	 * moved together with bragg during a scan.  
	 */

	mx_status = mx_get_int32_variable_by_name( list_head,
				aps_18id_motor->trolleyY_linked_record->name,
				&trolleyY_linked );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( trolleyY_linked == FALSE) {
		num_motors--;
		trolleyX_motor_index--;
		gap_energy_motor_index--;
		tune_motor_index--;
	} else {
		motor_record_array[ trolleyY_motor_index ] 
			= aps_18id_motor->trolleyY_motor_record;
						
		if ( motor_record_array[ trolleyY_motor_index ]
			== (MX_RECORD *)NULL )
		{
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Trolley Y  motor record pointer for record '%s' is NULL.",
				motor->record->name );
		}
	}
		
	 /* Next we do TrolleyX  */

	mx_status = mx_get_int32_variable_by_name( list_head,
				aps_18id_motor->trolleyX_linked_record->name,
				&trolleyX_linked );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( trolleyX_linked == FALSE) {
		num_motors--;
		gap_energy_motor_index--;
		tune_motor_index--;
	} else {
		motor_record_array[ trolleyX_motor_index ] 
				= aps_18id_motor->trolleyX_motor_record;
						
		if ( motor_record_array[ trolleyX_motor_index ]
			== (MX_RECORD *)NULL )
		{
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"Trolley X motor record pointer for record '%s' is NULL.",
				motor->record->name );
		}
	}

	/*  Now we do gap  */

	mx_status = mx_get_int32_variable_by_name( list_head,
				aps_18id_motor->gap_linked_record->name,
				&gap_linked );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;


	if ( gap_linked == FALSE ) {
		num_motors--;
		tune_motor_index--;
	} else {
		motor_record_array[ gap_energy_motor_index ] 
			= aps_18id_motor->gap_energy_motor_record;
						
		if ( motor_record_array[ gap_energy_motor_index ]
			== (MX_RECORD *)NULL)
		{
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"gap energy motor record pointer for record '%s' is NULL.",
				motor->record->name );
		}
	}

	/* Next to last is tune  */

	mx_status = mx_get_int32_variable_by_name( list_head,
			aps_18id_motor->tune_linked_record->name,
			&tune_linked );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( tune_linked == FALSE ) {
		num_motors--;
	} else {
		motor_record_array[ tune_motor_index ]
			= aps_18id_motor->tune_motor_record;

		if ( motor_record_array[ tune_motor_index ]
			== (MX_RECORD *) NULL)
		{
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"tune motor record pointer for record '%s' is NULL.",
				motor->record->name );
		}
	}
	
	/* piezo motors are the last  */

	mx_status = mx_get_int32_variable_by_name( list_head,
			aps_18id_motor->piezo_enable_record->name,
			&piezo_enable );

	piezo_left_motor_record = aps_18id_motor->piezo_left_motor_record;

	if ( piezo_left_motor_record == (MX_RECORD *) NULL) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"piezo_left motor record pointer for record '%s' is NULL.",
			motor->record->name );
	}
	
	piezo_right_motor_record = aps_18id_motor->piezo_right_motor_record;

	if ( piezo_right_motor_record == (MX_RECORD *) NULL) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"piezo_right motor record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	/***************** Evaluate new position for bragg ****************/

	bragg = motor->raw_destination.analog;

	/* We need to convert the requested bragg value into 
	 * units of electron volts. Remember that bragg is in angles.  
	 */

	mx_status = mx_get_double_variable_by_name( list_head,
			aps_18id_motor->d_spacing_record->name,
			&d_spacing );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	angle_in_radians = bragg * (aps_18id_motor->angle_scale);

	mono_energy = MX_HC / ( 2.0 * d_spacing * sin(angle_in_radians));

#if 0
	MX_DEBUG( 2,
		("%s: bragg = %g, angle_in_radians = %g, angle_scale = %g",
		fname, bragg, angle_in_radians, aps_18id_motor->angle_scale));
	MX_DEBUG( 2,
		("%s: d_spacing = %g, MX_HC = %g, mono_energy = %g",
		fname, d_spacing, MX_HC, mono_energy));
	MX_DEBUG( 2,("Press any key to continue..."));
	(void) fgetc(stdin);
#endif

	position_array[ bragg_motor_index ] = bragg;

	/*************** Evaluate new positions for trolley Y ****************/
	
	beam_height_offset = 35.0;    /* beam height change after the mono */
	
	/* Get the fitting function type. */
	
	/* Here we use the same tune parameters for trolley Y feedforward */

	/* Get the tune motion parameters.  They should be in a 
	 * one dimensional array of doubles.
	 */

	mx_status = mx_find_record_field( aps_18id_motor->tune_parameters_record,
				"value", &value_field );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	num_parameters = value_field->dimension[0];

	parameter_array = mx_get_field_value_pointer( value_field );

	/* Evaluate the function using the values in 'parameter_array'. */

	if ( num_parameters < 4 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"A cubic polynomial fit requires 4 parameters.  "
			"'%s' has only %ld parameters.",
				aps_18id_motor->tune_parameters_record->name,
				(long) num_parameters );
	}

	trolleyY = parameter_array[0] + mono_energy * (
			parameter_array[1] + mono_energy * (
			parameter_array[2] + mono_energy * 
			parameter_array[3] ));
			
	position_array[ trolleyY_motor_index ] = trolleyY;

	/*************** Evaluate new positions for trolley X ****************/

	trolleyX = ( beam_height_offset/2 ) / sin( angle_in_radians );
	position_array[ trolleyX_motor_index ] = trolleyX;

	/***************** Evaluate new position for tune ****************/

	if ( tune_linked ) {

		/* Get the fitting function type. */

		mx_status = mx_get_int32_variable_by_name( list_head,
				aps_18id_motor->tune_linked_record->name,
				&tune_linked );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Get the tune motion parameters.  They should be in a 
		 * one dimensional array of doubles.
		 */

		mx_status = mx_find_record_field(
				aps_18id_motor->tune_parameters_record,
					"value", &value_field );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_parameters = value_field->dimension[0];

		parameter_array = (double *)
				mx_get_field_value_pointer( value_field );

		/* Get the current position of tune. */

		tune_motor_record = motor_record_array[ tune_motor_index ];

		mx_status = mx_motor_get_position(
				tune_motor_record, &tune_old_position );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Evaluate the function using the values in 'parameter_array'.
		 */

		if ( num_parameters < 4 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"A cubic polynomial fit requires 4 parameters.  "
			"'%s' has only %ld parameters.",
				aps_18id_motor->tune_parameters_record->name,
				(long) num_parameters );
		}
		tune = parameter_array[0] + mono_energy * (
			parameter_array[1] + mono_energy * (
			parameter_array[2] + mono_energy * 
			parameter_array[3] ));
			
		position_array[ tune_motor_index ] = tune;
	}


	/******** Evaluate new position for insertion device gap **********/

	/* If the gap _is_ supposed to be linked to all the other motions,
	 * then read in the rest of the gap parameters.
	 */

	if ( gap_linked ) {
		/* Get the requested offset for the undulator gap. */

		mx_status = mx_get_double_variable_by_name( list_head,
				aps_18id_motor->gap_offset_record->name,
				&gap_offset );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		new_gap_energy = mono_energy + gap_offset;

		/* Find out which harmonic of the undulator radiation
		 * that we want to use.
		 */

		mx_status = mx_get_int32_variable_by_name( list_head,
				aps_18id_motor->gap_harmonic_record->name,
				&gap_harmonic );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		if ( gap_harmonic <= 0 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Requested gap harmonic value = %ld is illegal.  "
			"The gap harmonic must be greater than zero.",
				(long) gap_harmonic );
		}

		new_gap_energy /= (double) gap_harmonic;

		position_array[ gap_energy_motor_index ] = new_gap_energy;
	}

	/********************** Now move the motors. **********************/

	mx_status = mx_motor_array_move_absolute( num_motors,
		motor_record_array, position_array, 0 );
		
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;
		
	/* making sure trolley motors are moved too */

	mx_status = mx_set_int32_variable(
			aps_18id_motor->trolley_setup_record, 1L );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* The last thing we need to do is to feed back using piezo/tune
	 * if needed
	 */
	
	if ( piezo_enable == 1 || piezo_enable == 2 || piezo_enable == 3 ) {

		/* get the piezo fit parameters which should be in a one
		 * dimensional array of doubles.
		 */
	
		mx_status = mx_find_record_field(
				aps_18id_motor->piezo_parameters_record,
					"value", &value_field );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		num_parameters = value_field->dimension[0];

		parameter_array = (double *)
				mx_get_field_value_pointer( value_field );

		/* Get the current position of piezo. */

		mx_status = mx_motor_get_position( piezo_left_motor_record,
						&piezo_left_old_position );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_motor_get_position( piezo_right_motor_record,
						&piezo_right_old_position );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		/* Evaluate the function using the values in 'parameter_array'.
		 */

		if ( num_parameters < 4 ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"A cubic polynomial fit requires 4 parameters.  "
			"'%s' has only %ld parameters.",
				aps_18id_motor->piezo_parameters_record->name,
				(long) num_parameters );
		}
		piezo_scale = parameter_array[0] + mono_energy * (
			parameter_array[1] + mono_energy * (
			parameter_array[2] + mono_energy * 
			parameter_array[3] ));
	
		/*  We need to read the BPMs to determine the piezo value */
		
		bpm_top_scaler_record = aps_18id_motor->bpm_top_scaler_record;
		
		if ( bpm_top_scaler_record == (MX_RECORD *) NULL) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"bpm_top scaler record pointer for record '%s' is NULL.",
				motor->record->name );
		}

		bpm_bottom_scaler_record
				= aps_18id_motor->bpm_bottom_scaler_record;
		
		if ( bpm_bottom_scaler_record == (MX_RECORD *) NULL) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"bpm_bottom scaler record pointer for record '%s' is NULL.",
				motor->record->name );
		}
		
		timer_record = aps_18id_motor->timer_record;
		
		if ( timer_record == (MX_RECORD *) NULL) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"Timer record pointer for record '%s' is NULL.",
				motor->record->name );
		}
		
		/* Wait for the settling time  */
		
		settling_time = 0.01;
		mx_status = mx_timer_start( timer_record, settling_time );
		
		/* Wait for the settling time to be over */
	
		busy = TRUE;
		
		while ( busy ) {
			if ( mx_user_requested_interrupt() ) {
				(void) mx_timer_stop( timer_record,
							&remaining_time);

				return mx_error( MXE_INTERRUPTED, fname,
				"Measurement time was interrupted. ");
			}
			
			mx_status = mx_timer_is_busy( timer_record, &busy );
			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
				
			mx_msleep(1);	/* Wait at least 1 millisecond. */
		}
		
		/* Start the measurement  */
	
		integration_time = 1.0;

		mx_status = mx_timer_start( timer_record, integration_time );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		
		/* Wait for the measurement to finish  */
	
		busy = TRUE;

		while ( busy ) {
			if ( mx_user_requested_interrupt() ) {
				(void) mx_timer_stop( timer_record,
						&remaining_time);

				return mx_error( MXE_INTERRUPTED, fname,
				"Measurement time was interrupted. ");
			}
			
			mx_status = mx_timer_is_busy( timer_record, &busy );
			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
				
			mx_msleep(1);	/* Wait at least 1 millisecond. */
		}
		
		/* Read the scalers  */
		
		mx_status = mx_scaler_read( bpm_top_scaler_record,
						&bpm_top_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		
		mx_status = mx_scaler_read( bpm_bottom_scaler_record,
						&bpm_bottom_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
		
		ruler = ( bpm_top_value - bpm_bottom_value )
				/ (  bpm_top_value + bpm_bottom_value );

		if ( piezo_enable == 1 ){
		
			piezo_left = ruler * piezo_scale
						+ piezo_left_old_position;

			piezo_right = ruler * piezo_scale
						+ piezo_right_old_position;
		
			mx_status = mx_motor_move_absolute(
					piezo_left_motor_record,
					piezo_left, 0 );
		
			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		
			mx_status = mx_motor_move_absolute(
					piezo_right_motor_record,
					piezo_right, 0 );
		} else
		if ( piezo_enable == 2 ) {	
		
			/* tune is used to feedback beam height   */		

			if ( tune_linked == 0 ) {
			
			    tune_motor_record =
					aps_18id_motor->tune_motor_record;

			    if ( tune_motor_record == (MX_RECORD *) NULL ) {
				return mx_error( MXE_CORRUPT_DATA_STRUCTURE,
						fname,
			"tune motor record pointer for record '%s' is NULL.",
					motor->record->name );
			    }
			}

			mx_status = mx_motor_get_position( tune_motor_record,
							&tune_old_position );

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			tune = ruler * piezo_scale + tune_old_position;

			mx_status = mx_motor_move_absolute( tune_motor_record,
								tune, 0);
		} else {
			/* We use Trolley Y to feedback the height of the beam.
			 */
			
			trolleyY_motor_record
				= aps_18id_motor->trolleyY_motor_record;

			if (trolleyY_motor_record == (MX_RECORD *) NULL) {
				return mx_error( MXE_CORRUPT_DATA_STRUCTURE,
						fname,
			"tune motor record pointer for record '%s' is NULL.",
					motor->record->name );
			}

			mx_status = mx_motor_get_position( trolleyY_motor_record,
							&trolleyY);

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;

			trolleyY = ruler * piezo_scale
				/ ( 2 * cos( angle_in_radians )) + trolleyY;
		}

		mx_status = mx_motor_move_absolute( trolleyY_motor_record,
							trolleyY, 0 );
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_aps_18id_motor_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_aps_18id_motor_get_position()";

	MX_APS_18ID_MOTOR *aps_18id_motor;
	MX_RECORD *bragg_motor_record;
	double bragg;
	mx_status_type mx_status;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_MOTOR pointer passed NULL.");
	}

	aps_18id_motor = (MX_APS_18ID_MOTOR *)
				(motor->record->record_type_struct);

	if ( aps_18id_motor == (MX_APS_18ID_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_APS_18ID_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	bragg_motor_record = aps_18id_motor->bragg_motor_record;

	if ( bragg_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"bragg_motor_record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	mx_status = mx_motor_get_position( bragg_motor_record, &bragg );
	
	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	/* Since bragg pseudo motor uses the real Bragg motor, both use
	 * degrees as units, there is no conversion needed.
	 */

	motor->raw_position.analog = bragg;

	MX_DEBUG( 2,("%s: bragg angle = %g", fname, bragg));

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aps_18id_motor_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_aps_18id_motor_set_position()";

	MX_APS_18ID_MOTOR *aps_18id_motor;
	MX_RECORD *bragg_motor_record;
	double bragg;
	mx_status_type mx_status;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"MX_MOTOR pointer passed NULL." );
	}

	aps_18id_motor = (MX_APS_18ID_MOTOR *)
				(motor->record->record_type_struct);

	if ( aps_18id_motor == (MX_APS_18ID_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_APS_18ID_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	bragg_motor_record = aps_18id_motor->bragg_motor_record;

	if ( bragg_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"Dependent MX_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	bragg = motor->raw_set_position.analog;

	/* Since both bragg pseudo motor and real Bragg motor use degrees 
	* no convertion needed here.
	*/
	 			
	mx_status = mx_motor_set_position( bragg_motor_record, bragg );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_aps_18id_motor_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_aps_18id_motor_soft_abort()";

	MX_APS_18ID_MOTOR *aps_18id_motor;
	MX_RECORD *bragg_motor_record;
	MX_RECORD *trolleyY_motor_record;
	MX_RECORD *trolleyX_motor_record;
	MX_RECORD *tune_motor_record;
	MX_RECORD *gap_energy_motor_record;
	mx_status_type mx_status;
	mx_status_type mx_status_array[5];
	int i;
	int32_t gap_linked;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MOTOR pointer passed NULL.");
	}

	aps_18id_motor = (MX_APS_18ID_MOTOR *)
				(motor->record->record_type_struct);

	if ( aps_18id_motor == (MX_APS_18ID_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_APS_18ID_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	bragg_motor_record = aps_18id_motor->bragg_motor_record;

	if ( bragg_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"bragg_motor_record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	trolleyY_motor_record = aps_18id_motor->trolleyY_motor_record;

	if ( trolleyY_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"trolleyY_motor_record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	trolleyX_motor_record = aps_18id_motor->trolleyX_motor_record;

	if ( trolleyX_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"trolleyX_motor_record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	tune_motor_record = aps_18id_motor->tune_motor_record;

	if ( tune_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"tune_motor_record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	gap_energy_motor_record = aps_18id_motor->gap_energy_motor_record;

	if ( gap_energy_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"gap_energy_motor_record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	mx_status = mx_get_int32_variable_by_name( motor->record->list_head,
				aps_18id_motor->gap_linked_record->name,
				&gap_linked );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status_array[0] = mx_motor_soft_abort( bragg_motor_record );

	mx_status_array[1] = mx_motor_soft_abort( trolleyY_motor_record );

	mx_status_array[2] = mx_motor_soft_abort( trolleyX_motor_record );

	mx_status_array[3] = mx_motor_soft_abort( tune_motor_record );

	if ( gap_linked ) {
		mx_status_array[4]
			= mx_motor_soft_abort( gap_energy_motor_record );
	} else {
		mx_status_array[4] = MX_SUCCESSFUL_RESULT;
	}

	for ( i = 1; i < 5; i++ ) {
		if ( mx_status_array[i].code != MXE_SUCCESS ) 
			return mx_status_array[i];
	}
	return mx_status_array[4];
}

MX_EXPORT mx_status_type
mxd_aps_18id_motor_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_aps_18id_motor_immediate_abort()";

	MX_APS_18ID_MOTOR *aps_18id_motor;
	MX_RECORD *bragg_motor_record;
	MX_RECORD *trolleyY_motor_record;
	MX_RECORD *trolleyX_motor_record;
	MX_RECORD *tune_motor_record;
	MX_RECORD *gap_energy_motor_record;
	mx_status_type mx_status[5];
	int i;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MOTOR pointer passed NULL.");
	}

	aps_18id_motor = (MX_APS_18ID_MOTOR *)
				(motor->record->record_type_struct);

	if ( aps_18id_motor == (MX_APS_18ID_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_APS_18ID_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	bragg_motor_record = aps_18id_motor->bragg_motor_record;

	if ( bragg_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"bragg_motor_record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	trolleyY_motor_record = aps_18id_motor->trolleyY_motor_record;

	if ( trolleyY_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"trolleyY_motor_record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	trolleyX_motor_record = aps_18id_motor->trolleyX_motor_record;

	if ( trolleyX_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"trolleyX_motor_record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	tune_motor_record = aps_18id_motor->tune_motor_record;

	if ( tune_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"tune_motor_record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	gap_energy_motor_record = aps_18id_motor->gap_energy_motor_record;

	if ( gap_energy_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"gap_energy_motor_record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	mx_status[0] = mx_motor_immediate_abort( bragg_motor_record );

	mx_status[1] = mx_motor_immediate_abort( trolleyY_motor_record );

	mx_status[2] = mx_motor_immediate_abort( trolleyX_motor_record );

	mx_status[3] = mx_motor_immediate_abort( tune_motor_record );

	mx_status[4] = mx_motor_immediate_abort( gap_energy_motor_record );


	for ( i = 1; i < 5; i++ ) {
		if ( mx_status[i].code != MXE_SUCCESS ) 
			return mx_status[i];
	}
	return mx_status[4];
}

MX_EXPORT mx_status_type
mxd_aps_18id_motor_positive_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_aps_18id_motor_positive_limit_hit()";

	MX_APS_18ID_MOTOR *aps_18id_motor;
	MX_RECORD *bragg_motor_record;
	MX_RECORD *trolleyY_motor_record;
	MX_RECORD *trolleyX_motor_record;
	MX_RECORD *tune_motor_record;
	MX_RECORD *piezo_left_motor_record;
	MX_RECORD *piezo_right_motor_record;
	int limit_hit;
	mx_status_type mx_status;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MOTOR pointer passed NULL.");
	}

	aps_18id_motor = (MX_APS_18ID_MOTOR *)
				(motor->record->record_type_struct);

	if ( aps_18id_motor == (MX_APS_18ID_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_APS_18ID_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	bragg_motor_record = aps_18id_motor->bragg_motor_record;

	if ( bragg_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"bragg_motor_record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	trolleyY_motor_record = aps_18id_motor->trolleyY_motor_record;

	if ( trolleyY_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"trolleyY_motor_record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	trolleyX_motor_record = aps_18id_motor->trolleyX_motor_record;

	if ( trolleyX_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"trolleyX_motor_record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	tune_motor_record = aps_18id_motor->tune_motor_record;

	if ( tune_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"tune_motor_record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	piezo_left_motor_record = aps_18id_motor->piezo_left_motor_record;

	if ( piezo_left_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"piezo_left_motor_record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	piezo_right_motor_record = aps_18id_motor->piezo_right_motor_record;

	if ( piezo_right_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"piezo_right_motor_record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	/* Check bragg motor   */
	
	mx_status = mx_motor_positive_limit_hit(
				bragg_motor_record, &limit_hit );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( limit_hit ) {
		motor->positive_limit_hit = TRUE;

		return MX_SUCCESSFUL_RESULT;
	} 
	
	/* Check trolleyY motor   */
	
	mx_status = mx_motor_positive_limit_hit(
				trolleyY_motor_record, &limit_hit );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( limit_hit ) {
		motor->positive_limit_hit = TRUE;

		return MX_SUCCESSFUL_RESULT;
	} 
	
	/* Check trolleyX motor   */
	
	mx_status = mx_motor_positive_limit_hit(
				trolleyX_motor_record, &limit_hit );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( limit_hit ) {
		motor->positive_limit_hit = TRUE;

		return MX_SUCCESSFUL_RESULT;
	} 
	
	/* Check tune motor   */
	
	mx_status = mx_motor_positive_limit_hit(
				tune_motor_record, &limit_hit );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( limit_hit ) {
		motor->positive_limit_hit = TRUE;

		return MX_SUCCESSFUL_RESULT;
	}

	/* Check piezo motors   */
	
	mx_status = mx_motor_positive_limit_hit(
				piezo_left_motor_record, &limit_hit );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( limit_hit ) {
		motor->positive_limit_hit = TRUE;

		return MX_SUCCESSFUL_RESULT;
	}

	mx_status = mx_motor_positive_limit_hit(
				piezo_right_motor_record, &limit_hit );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( limit_hit ) {
		motor->positive_limit_hit = TRUE;

		return MX_SUCCESSFUL_RESULT;
	}

	motor->positive_limit_hit = limit_hit;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aps_18id_motor_negative_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_aps_18id_motor_negative_limit_hit()";

	MX_APS_18ID_MOTOR *aps_18id_motor;
	MX_RECORD *bragg_motor_record;
	MX_RECORD *trolleyY_motor_record;
	MX_RECORD *trolleyX_motor_record;
	MX_RECORD *tune_motor_record;
	MX_RECORD *piezo_left_motor_record;
	MX_RECORD *piezo_right_motor_record;
	int limit_hit;
	mx_status_type mx_status;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MOTOR pointer passed NULL.");
	}

	aps_18id_motor = (MX_APS_18ID_MOTOR *)
				(motor->record->record_type_struct);

	if ( aps_18id_motor == (MX_APS_18ID_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_APS_18ID_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	bragg_motor_record = aps_18id_motor->bragg_motor_record;

	if ( bragg_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"bragg_motor_record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	trolleyY_motor_record = aps_18id_motor->trolleyY_motor_record;

	if ( trolleyY_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"trolleyY_motor_record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	trolleyX_motor_record = aps_18id_motor->trolleyX_motor_record;

	if ( trolleyX_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"trolleyX_motor_record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	tune_motor_record = aps_18id_motor->tune_motor_record;

	if ( tune_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"tune_motor_record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	piezo_left_motor_record = aps_18id_motor->piezo_left_motor_record;

	if ( piezo_left_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"piezo_left_motor_record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	piezo_right_motor_record = aps_18id_motor->piezo_right_motor_record;

	if ( piezo_right_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"piezo_right_motor_record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	mx_status = mx_motor_negative_limit_hit( bragg_motor_record,
						&limit_hit );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( limit_hit ) {
		motor->negative_limit_hit = TRUE;

		return MX_SUCCESSFUL_RESULT;
	}
	
		/* Check trolleyY motor   */
	
	mx_status = mx_motor_negative_limit_hit( trolleyY_motor_record,
						&limit_hit );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( limit_hit ) {
		motor->negative_limit_hit = TRUE;

		return MX_SUCCESSFUL_RESULT;
	} 
	
	/* Check trolleyX motor   */
	
	mx_status = mx_motor_negative_limit_hit( trolleyX_motor_record,
						&limit_hit );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( limit_hit ) {
		motor->negative_limit_hit = TRUE;

		return MX_SUCCESSFUL_RESULT;
	} 
	
	/* Check tune motor   */
	
	mx_status = mx_motor_negative_limit_hit( tune_motor_record,
						&limit_hit );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( limit_hit ) {
		motor->negative_limit_hit = TRUE;

		return MX_SUCCESSFUL_RESULT;
	}

	/* Check piezo motors   */
	
	mx_status = mx_motor_negative_limit_hit( piezo_left_motor_record,
						&limit_hit );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( limit_hit ) {
		motor->negative_limit_hit = TRUE;

		return MX_SUCCESSFUL_RESULT;
	}

	mx_status = mx_motor_negative_limit_hit( piezo_right_motor_record,
						&limit_hit );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( limit_hit ) {
		motor->negative_limit_hit = TRUE;

		return MX_SUCCESSFUL_RESULT;
	}

	motor->negative_limit_hit = limit_hit;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aps_18id_motor_find_home_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_aps_18id_motor_find_home_position()";

	MX_APS_18ID_MOTOR *aps_18id_motor;
	MX_RECORD *bragg_motor_record;
	MX_RECORD *tune_motor_record;
	int direction;
	mx_status_type mx_status;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MOTOR pointer passed NULL.");
	}

	aps_18id_motor = (MX_APS_18ID_MOTOR *)
				(motor->record->record_type_struct);

	if ( aps_18id_motor == (MX_APS_18ID_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_APS_18ID_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}

	bragg_motor_record = aps_18id_motor->bragg_motor_record;

	if ( bragg_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"bragg_motor_record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	tune_motor_record = aps_18id_motor->tune_motor_record;

	if ( tune_motor_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"tune_motor_record pointer for record '%s' is NULL.",
			motor->record->name );
	}

	direction = motor->home_search;

	mx_status = mx_motor_find_home_position( bragg_motor_record,
							direction );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_find_home_position( tune_motor_record,
							- direction );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_aps_18id_motor_constant_velocity_move( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_aps_18id_motor_constant_velocity_move()";

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
	"Constant velocity moves are not yet implemented for motor '%s'.",
		motor->record->name );
}

MX_EXPORT mx_status_type
mxd_aps_18id_motor_get_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_aps_18id_motor_get_parameter()";

	MX_APS_18ID_MOTOR *aps_18id_motor;
	MX_RECORD *bragg_motor_record;
	double double_value;
	double start_position, end_position;
	double real_start_position, real_end_position;
	mx_status_type mx_status;

	mx_status = mxd_aps_18id_motor_get_pointers( motor, &aps_18id_motor,
						&bragg_motor_record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		mx_status = mx_motor_get_speed( bragg_motor_record,
							&double_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->raw_speed = double_value;
		break;

	case MXLV_MTR_BASE_SPEED:
		mx_status = mx_motor_get_base_speed( bragg_motor_record,
							&double_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->raw_base_speed = double_value;
		break;

	case MXLV_MTR_MAXIMUM_SPEED:
		mx_status = mx_motor_get_maximum_speed( bragg_motor_record,
							&double_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->raw_maximum_speed = double_value;
		break;

	case MXLV_MTR_ACCELERATION_TIME:
		mx_status = mx_motor_get_acceleration_time(
						bragg_motor_record,
						&double_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->acceleration_time = double_value;
		break;

	case MXLV_MTR_ACCELERATION_DISTANCE:
		mx_status = mx_motor_get_acceleration_distance(
						bragg_motor_record,
						&double_value );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->acceleration_distance = double_value;
		break;

	case MXLV_MTR_COMPUTE_EXTENDED_SCAN_RANGE:
		start_position = motor->raw_compute_extended_scan_range[0];
		end_position = motor->raw_compute_extended_scan_range[1];

		mx_status = mx_motor_compute_extended_scan_range(
						bragg_motor_record,
						start_position,
						end_position,
						&real_start_position,
						&real_end_position );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		motor->raw_compute_extended_scan_range[2] = real_start_position;
		motor->raw_compute_extended_scan_range[3] = real_end_position;
		break;

	case MXLV_MTR_COMPUTE_PSEUDOMOTOR_POSITION:
		motor->compute_pseudomotor_position[1]
				= motor->compute_pseudomotor_position[0];
		break;

	case MXLV_MTR_COMPUTE_REAL_POSITION:
		motor->compute_real_position[1]
				= motor->compute_real_position[0];
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Parameter type '%s' (%d) is not supported "
			"by the APS_18ID driver for motor '%s'.",

			mx_get_field_label_string( motor->record,
						motor->parameter_type),
			motor->parameter_type,
			motor->record->name );
		break;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aps_18id_motor_set_parameter( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_aps_18id_motor_set_parameter()";

	MX_APS_18ID_MOTOR *aps_18id_motor;
	MX_RECORD *bragg_motor_record;
	double start_position, end_position, time_for_move;
	mx_status_type mx_status;

	mx_status = mxd_aps_18id_motor_get_pointers( motor, &aps_18id_motor,
						&bragg_motor_record, NULL );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	switch( motor->parameter_type ) {
	case MXLV_MTR_SPEED:
		mx_status = mx_motor_set_speed( bragg_motor_record,
						motor->raw_speed );
		break;

	case MXLV_MTR_BASE_SPEED:
		mx_status = mx_motor_set_base_speed( bragg_motor_record,
						motor->raw_base_speed );
		break;

	case MXLV_MTR_MAXIMUM_SPEED:
		mx_status = mx_motor_set_maximum_speed( bragg_motor_record,
						motor->raw_maximum_speed );
		break;

	case MXLV_MTR_SAVE_SPEED:
		mx_status = mx_motor_save_speed( bragg_motor_record );
		break;

	case MXLV_MTR_RESTORE_SPEED:
		mx_status = mx_motor_restore_speed( bragg_motor_record );
		break;

	case MXLV_MTR_SPEED_CHOICE_PARAMETERS:
		start_position = motor->raw_speed_choice_parameters[0];
		end_position = motor->raw_speed_choice_parameters[1];
		time_for_move = motor->raw_speed_choice_parameters[2];

		mx_status = mx_motor_set_speed_between_positions(
						bragg_motor_record,
						start_position,
						end_position,
						time_for_move );
		break;

	default:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Parameter type '%s' (%d) is not supported "
			"by the APS_18ID driver for motor '%s'.",

			mx_get_field_label_string( motor->record,
						motor->parameter_type),
			motor->parameter_type,
			motor->record->name );
		break;
	}

	return mx_status;
}

