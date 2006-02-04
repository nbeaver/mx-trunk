/*
 * Name:    d_adc_table.c 
 *
 * Purpose: MX driver for the ADC table installed at APS Sector 17.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <math.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "mx_table.h"
#include "d_adc_table.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_adc_table_record_function_list = {
	mxd_adc_table_initialize_type,
	mxd_adc_table_create_record_structures,
	mxd_adc_table_finish_record_initialization,
	mxd_adc_table_delete_record,
	NULL,
	mxd_adc_table_read_parms_from_hardware,
	mxd_adc_table_write_parms_to_hardware,
	mxd_adc_table_open,
	mxd_adc_table_close,
	NULL,
	mxd_adc_table_resynchronize
};

MX_TABLE_FUNCTION_LIST mxd_adc_table_table_function_list = {
	mxd_adc_table_is_busy,
	mxd_adc_table_move_absolute,
	mxd_adc_table_get_position,
	mxd_adc_table_set_position,
	mxd_adc_table_soft_abort,
	mxd_adc_table_immediate_abort,
	mxd_adc_table_positive_limit_hit,
	mxd_adc_table_negative_limit_hit
};

/* Soft motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_adc_table_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MXD_ADC_TABLE_STANDARD_FIELDS
};

mx_length_type mxd_adc_table_num_record_fields
		= sizeof( mxd_adc_table_record_field_defaults )
			/ sizeof( mxd_adc_table_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_adc_table_rfield_def_ptr
			= &mxd_adc_table_record_field_defaults[0];

/*=======================================================================*/

/* This function is a utility function to consolidate all of the
 * pointer mangling that often has to happen at the beginning of an 
 * mxd_adc_table_...  function.  The parameter 'calling_fname'
 * is passed so that error messages will appear with the name of
 * the calling function.
 */

static mx_status_type
mxd_adc_table_get_pointers( MX_TABLE *table,
			MX_ADC_TABLE **adc_table,
			const char *calling_fname )
{
	const char fname[] = "mxd_adc_table_get_pointers()";

	if ( table == (MX_TABLE *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The table pointer passed by '%s' was NULL",
			calling_fname );
	}

	if ( table->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_RECORD pointer for table pointer passed by '%s' is NULL.",
			calling_fname );
	}

	if ( adc_table == (MX_ADC_TABLE **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The adc_table pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*adc_table = (MX_ADC_TABLE *) table->record->record_type_struct;

	if ( *adc_table == (MX_ADC_TABLE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_ADC_TABLE pointer for table record '%s' passed by '%s' is NULL",
			table->record->name, calling_fname );
	}

	return MX_SUCCESSFUL_RESULT;
}

/* The following function fills in subset_array with the list of motor records
 * that are used by the specified axis_id.
 */

static mx_status_type
mxd_adc_table_construct_table_subset( MX_TABLE *table,
				MX_ADC_TABLE *adc_table,
				int *num_motors,
				MX_RECORD **subset_array )
{
	const char fname[] = "mxd_adc_table_construct_table_subset()";

	MX_RECORD **motor_record_array;

	int i;

	*num_motors = 0;

	motor_record_array = adc_table->motor_record_array;

	switch( table->axis_id ) {
	case MXF_TAB_X:
		*num_motors = 1;

		subset_array[0] = motor_record_array[ MXF_ADC_TABLE_X1 ];
		break;

	case MXF_TAB_Y:
	case MXF_TAB_YAW:
		*num_motors = 2;

		subset_array[0] = motor_record_array[ MXF_ADC_TABLE_Y1 ];
		subset_array[1] = motor_record_array[ MXF_ADC_TABLE_Y2 ];
		break;

	case MXF_TAB_Z:
	case MXF_TAB_PITCH:
		*num_motors = 3;

		subset_array[0] = motor_record_array[ MXF_ADC_TABLE_Z1 ];
		subset_array[1] = motor_record_array[ MXF_ADC_TABLE_Z2 ];
		subset_array[2] = motor_record_array[ MXF_ADC_TABLE_Z3 ];
		break;

	case MXF_TAB_ROLL:
		*num_motors = 5;

		subset_array[0] = motor_record_array[ MXF_ADC_TABLE_Z1 ];
		subset_array[1] = motor_record_array[ MXF_ADC_TABLE_Z2 ];
		subset_array[2] = motor_record_array[ MXF_ADC_TABLE_Z3 ];
		subset_array[3] = motor_record_array[ MXF_ADC_TABLE_Y1 ];
		subset_array[4] = motor_record_array[ MXF_ADC_TABLE_Y2 ];
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal axis_id value %d passed for ADC table '%s'.",
			table->axis_id, table->record->name );
	}

	for ( i = *num_motors; i < MX_ADC_TABLE_NUM_MOTORS; i++ ) {
		subset_array[i] = NULL;
	}

	return MX_SUCCESSFUL_RESULT;
}

static double
mxd_adc_table_compute_y_position( MX_ADC_TABLE *adc_table,
				double y1_position, double y2_position )
{
	const char fname[] = "mxd_adc_table_compute_y_position()";

	double c1, c2, y_position;

	/* What we want to see here is the Y position of the rotation center
	 * rather than the Y position of the table, so we use a weighted Y
	 * position.
	 */

	c1 = mx_divide_safely( adc_table->m2 + adc_table->rx,
				adc_table->m1 + adc_table->m2 );

	c2 = mx_divide_safely( adc_table->m1 - adc_table->rx,
				adc_table->m1 + adc_table->m2 );

	y_position = c1 * y1_position + c2 * y2_position;

	MX_DEBUG( 2,("%s: y1_position = %g, y2_position = %g",
			fname, y1_position, y2_position ));
	MX_DEBUG( 2,("%s: c1 = %g, c2 = %g", fname, c1, c2));
	MX_DEBUG( 2,("%s: y_position = %g", fname, y_position));

	return y_position;
}

static double
mxd_adc_table_compute_z_position( MX_ADC_TABLE *adc_table,
				double z1_position, double z2_position,
				double z3_position )
{
	const char fname[] = "mxd_adc_table_compute_z_position()";

	double c1, c2, c3, c_denom, m1m3_ratio, m2m3_ratio, z_position;

	c_denom = adc_table->m1 + adc_table->m2;

	m2m3_ratio = mx_divide_safely( adc_table->m2, adc_table->m3 );

	c1 = mx_divide_safely(
		adc_table->m2 + adc_table->rx - m2m3_ratio * adc_table->ry,
			c_denom );

	m1m3_ratio = mx_divide_safely( adc_table->m1, adc_table->m3 );

	c2 = mx_divide_safely(
		adc_table->m1 - adc_table->rx - m1m3_ratio * adc_table->ry,
			c_denom );

	c3 = mx_divide_safely( adc_table->ry, adc_table->m3 );

	z_position = c1 * z1_position + c2 * z2_position + c3 * z3_position;

	MX_DEBUG( 2,("%s: z1_position = %g, z2_position = %g, z3_position = %g",
		fname, z1_position, z2_position, z3_position));
	MX_DEBUG( 2,("%s: c1 = %g, c2 = %g, c3 = %g", fname, c1, c2, c3));
	MX_DEBUG( 2,("%s: z_position = %g", fname, z_position));

	return z_position;
}

static double
mxd_adc_table_compute_roll_angle( MX_ADC_TABLE *adc_table,
				double z1_position,
				double z2_position,
				double z3_position )
{
	const char fname[] = "mxd_adc_table_compute_roll_angle()";

	double c21, z12, roll_angle;

	c21 = mx_divide_safely( adc_table->m2 + adc_table->rx,
				adc_table->m1 + adc_table->m2 );

	z12 = z1_position + c21 * ( z2_position - z1_position );

	roll_angle = mx_divide_safely( z3_position - z12, adc_table->m3 );

	MX_DEBUG( 2,("%s: z1_position = %g, z2_position = %g, z3_position = %g",
		fname, z1_position, z2_position, z3_position));
	MX_DEBUG( 2,("%s: c21 = %g, z12 = %g", fname, c21, z12));
	MX_DEBUG( 2,("%s: roll_angle = %g", fname, roll_angle));

	return roll_angle;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_adc_table_initialize_type( long type )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_adc_table_create_record_structures( MX_RECORD *record )
{
	const char fname[] = "mxd_adc_table_create_record_structures()";

	MX_TABLE *table;
	MX_ADC_TABLE *adc_table;

	/* Allocate memory for the necessary structures. */

	table = (MX_TABLE *) malloc( sizeof(MX_TABLE) );

	if ( table == (MX_TABLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_TABLE structure." );
	}

	adc_table = (MX_ADC_TABLE *) malloc( sizeof(MX_ADC_TABLE) );

	if ( adc_table == (MX_ADC_TABLE *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_ADC_TABLE structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = table;
	record->record_type_struct = adc_table;
	record->class_specific_function_list
				= &mxd_adc_table_table_function_list;

	table->record = record;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_adc_table_finish_record_initialization( MX_RECORD *record )
{
	const char fname[] = "mxd_adc_table_finish_record_initialization()";

	MX_ADC_TABLE *adc_table;
	MX_RECORD *motor_record;
	int i;

	if ( record == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_RECORD pointer passed is NULL." );
	}

	adc_table = (MX_ADC_TABLE *) record->record_type_struct;

	if ( adc_table == (MX_ADC_TABLE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_ADC_TABLE pointer for record '%s' is NULL.",
			record->name );
	}

	/* Are the records in motor_record_array actually motor records? */

	for ( i = 0; i < MX_ADC_TABLE_NUM_MOTORS; i++ ) {

		motor_record = adc_table->motor_record_array[i];

		if ( motor_record == NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"Element %d of the motor record array for ADC table '%s' is NULL.",
				i+1, record->name );
		}

		if ( motor_record->mx_class != MXC_MOTOR ) {
			return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Record '%s' mentioned in the motor record array for ADC table '%s' "
	"is not a motor record.", motor_record->name, record->name );
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_adc_table_delete_record( MX_RECORD *record )
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
mxd_adc_table_read_parms_from_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_adc_table_write_parms_to_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_adc_table_open( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_adc_table_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_adc_table_resynchronize( MX_RECORD *record )
{
	const char fname[] = "mxd_adc_table_resynchronize()";

	MX_TABLE *table;
	MX_ADC_TABLE *adc_table;
	MX_RECORD *motor_record;
	int i;
	mx_status_type status;

	table = (MX_TABLE *) record->record_class_struct;

	if ( table == (MX_TABLE *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_TABLE pointer for record '%s' is NULL.",
			record->name );
	}

	status = mxd_adc_table_get_pointers(table, &adc_table, fname);

	if ( status.code != MXE_SUCCESS )
		return status;

	for ( i = 0; i < MX_ADC_TABLE_NUM_MOTORS; i++ ) {

		motor_record = adc_table->motor_record_array[i];

		status = mx_resynchronize_record( motor_record );

		if ( status.code != MXE_SUCCESS )
			return status;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_adc_table_is_busy( MX_TABLE *table )
{
	const char fname[] = "mxd_adc_table_is_busy()";

	MX_ADC_TABLE *adc_table;
	MX_RECORD *subset_array[ MX_ADC_TABLE_NUM_MOTORS ];
	MX_RECORD *motor_record;
	int i, busy, num_motors;
	mx_status_type status;

	status = mxd_adc_table_get_pointers(table, &adc_table, fname);

	if ( status.code != MXE_SUCCESS ) {
		table->busy = FALSE;

		return status;
	}

	status = mxd_adc_table_construct_table_subset( table, adc_table,
					&num_motors, subset_array );

	if ( status.code != MXE_SUCCESS )
		return status;

	table->busy = FALSE;

	for ( i = 0; i < num_motors; i++ ) {

		motor_record = subset_array[i];

		status = mx_motor_is_busy( motor_record, &busy );

		if ( status.code != MXE_SUCCESS )
			return status;

		if ( busy ) {
			table->busy = TRUE;

			break;		/* Exit the for() loop. */
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_adc_table_move_absolute( MX_TABLE *table )
{
	const char fname[] = "mxd_adc_table_move_absolute()";

	MX_ADC_TABLE *adc_table;
	MX_RECORD *subset_array[ MX_ADC_TABLE_NUM_MOTORS ];
	MX_RECORD **marray;
	MX_MOTOR *motor;
	int i, num_motors;
	double y1_position, y2_position;
	double z1_position, z2_position, z3_position;
	double rotation_radius;
	double y_position, delta_y;
	double z_position, delta_z;
	double yaw_angle, delta_yaw;
	double roll_angle, delta_roll;
	double pitch_angle, delta_pitch;
	double delta_y1, delta_y2;
	double delta_z1, delta_z2, delta_z3;
	double theta_zero, cos_theta_zero;
	double test_z_position;
	double new_destination;
	double destination_array[ MX_ADC_TABLE_NUM_MOTORS ];
	mx_status_type status;

	status = mxd_adc_table_get_pointers(table, &adc_table, fname);

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mxd_adc_table_construct_table_subset( table, adc_table,
						&num_motors, subset_array );

	if ( status.code != MXE_SUCCESS )
		return status;

	marray = adc_table->motor_record_array;

	new_destination = table->destination;

	MX_DEBUG( 1,("%s: new_destination = %g", fname, new_destination));

	for ( i = 0; i < MX_ADC_TABLE_NUM_MOTORS; i++ ) {
		destination_array[i] = 0.0;
	}

	/* Compute the new destinations. */

	switch( table->axis_id ) {
	case MXF_TAB_X:
		num_motors = 1;

		destination_array[0] = new_destination;
		break;

	case MXF_TAB_Y:
	case MXF_TAB_YAW:
		num_motors = 2;

		status = mx_motor_get_position( marray[ MXF_ADC_TABLE_Y1 ],
							&y1_position );

		if ( status.code != MXE_SUCCESS )
			return status;

		status = mx_motor_get_position( marray[ MXF_ADC_TABLE_Y2 ],
							&y2_position );

		if ( status.code != MXE_SUCCESS )
			return status;

		MX_DEBUG( 1,("%s: y1_position = %g", fname, y1_position));
		MX_DEBUG( 1,("%s: y2_position = %g", fname, y2_position));

		if ( table->axis_id == MXF_TAB_Y ) {
			y_position = mxd_adc_table_compute_y_position(
					adc_table, y1_position, y2_position );

			MX_DEBUG( 1,("%s: y_position = %g",
					fname, y_position));

			delta_y = new_destination - y_position;

			MX_DEBUG( 1,("%s: delta_y = %g", fname, delta_y));

			destination_array[0] = y1_position + delta_y;
			destination_array[1] = y2_position + delta_y;

		} else {	/* MXF_TAB_YAW */

			yaw_angle
				= mx_divide_safely( y1_position - y2_position,
						adc_table->m1 + adc_table->m2 );

			MX_DEBUG( 1,("%s: yaw_angle = %g", fname, yaw_angle));

			delta_yaw = new_destination - yaw_angle;

			MX_DEBUG( 1,("%s: delta_yaw = %g", fname, delta_yaw));

			delta_y = delta_yaw * (adc_table->m1 + adc_table->m2);

			MX_DEBUG( 1,("%s: delta_y = %g", fname, delta_y));

			delta_y1 = delta_y
			    * mx_divide_safely(adc_table->m1 - adc_table->rx,
						adc_table->m1 + adc_table->m2);

			MX_DEBUG( 1,("%s: delta_y1 = %g", fname, delta_y1));

			delta_y2 = delta_y - delta_y1;

			MX_DEBUG( 1,("%s: delta_y2 = %g", fname, delta_y2));

			/* The corrections for y1 and y2 are in opposite
			 * directions, since we want to keep the rotation
			 * center in the same place.  Thus, we subtract
			 * delta_y2 below.
			 */

			destination_array[0] = y1_position + delta_y1;
			destination_array[1] = y2_position - delta_y2;
		}
		break;

	case MXF_TAB_Z:
	case MXF_TAB_PITCH:
		num_motors = 3;

		status = mx_motor_get_position( marray[ MXF_ADC_TABLE_Z1 ],
							&z1_position );

		if ( status.code != MXE_SUCCESS )
			return status;

		status = mx_motor_get_position( marray[ MXF_ADC_TABLE_Z2 ],
							&z2_position );

		if ( status.code != MXE_SUCCESS )
			return status;

		status = mx_motor_get_position( marray[ MXF_ADC_TABLE_Z3 ],
							&z3_position );

		if ( status.code != MXE_SUCCESS )
			return status;

		MX_DEBUG( 1,("%s: z1_position = %g", fname, z1_position));
		MX_DEBUG( 1,("%s: z2_position = %g", fname, z2_position));
		MX_DEBUG( 1,("%s: z3_position = %g", fname, z3_position));

		if ( table->axis_id == MXF_TAB_Z ) {

			z_position = mxd_adc_table_compute_z_position(
					adc_table, z1_position,
					z2_position, z3_position );

			MX_DEBUG( 1,("%s: z_position = %g",
					fname, z_position));

			delta_z = new_destination - z_position;

			MX_DEBUG( 1,("%s: delta_z = %g", fname, delta_z));

			destination_array[0] = z1_position + delta_z;
			destination_array[1] = z2_position + delta_z;
			destination_array[2] = z3_position + delta_z;

		} else {	/* MXF_TAB_PITCH */

			pitch_angle = mx_divide_safely(
						z1_position - z2_position,
						adc_table->m1 + adc_table->m2);

			MX_DEBUG( 1,("%s: pitch_angle = %g",
						fname, pitch_angle));

			delta_pitch = new_destination - pitch_angle;

			MX_DEBUG( 1,("%s: delta_pitch = %g",
						fname, delta_pitch));

			/* Implement the pitch change using weighted changes
			 * in z1 and z2.
			 */

			delta_z1 = adc_table->m1 * delta_pitch;

			MX_DEBUG( 1,("%s: delta_z1 = %g", fname, delta_z1));

			delta_z2 = - adc_table->m2 * delta_pitch;

			MX_DEBUG( 1,("%s: delta_z2 = %g", fname, delta_z2));

			/* How much would the height (Z) of the rotation
			 * center change if we performed these moves?
			 */

			z_position = mxd_adc_table_compute_z_position(
					adc_table, z1_position,
					z2_position, z3_position );

			MX_DEBUG( 1,("%s: z_position = %g", fname, z_position));

			test_z_position = mxd_adc_table_compute_z_position(
					adc_table, z1_position + delta_z1,
					z2_position + delta_z2, z3_position );

			MX_DEBUG( 1,("%s: test_z_position = %g",
					fname, test_z_position));

			delta_z = test_z_position - z_position;

			MX_DEBUG( 1,("%s: delta_z = %g", fname, delta_z));

			/* Since we _actually_ want the height (Z) of the
			 * rotation center to stay constant, we must compute
			 * new values of z1, z2, and z3 that have delta_z
			 * subtracted from all of them.
			 */

			destination_array[0] = z1_position + delta_z1 - delta_z;
			destination_array[1] = z2_position + delta_z2 - delta_z;
			destination_array[2] = z3_position - delta_z;
		}
		break;
	case MXF_TAB_ROLL:
		num_motors = 5;

		status = mx_motor_get_position( marray[ MXF_ADC_TABLE_Z1 ],
							&z1_position );

		if ( status.code != MXE_SUCCESS )
			return status;

		status = mx_motor_get_position( marray[ MXF_ADC_TABLE_Z2 ],
							&z2_position );

		if ( status.code != MXE_SUCCESS )
			return status;

		status = mx_motor_get_position( marray[ MXF_ADC_TABLE_Z3 ],
							&z3_position );

		if ( status.code != MXE_SUCCESS )
			return status;

		status = mx_motor_get_position( marray[ MXF_ADC_TABLE_Y1 ],
							&y1_position );

		if ( status.code != MXE_SUCCESS )
			return status;

		status = mx_motor_get_position( marray[ MXF_ADC_TABLE_Y2 ],
							&y2_position );

		if ( status.code != MXE_SUCCESS )
			return status;

		MX_DEBUG( 1,("%s: z1_position = %g", fname, z1_position));
		MX_DEBUG( 1,("%s: z2_position = %g", fname, z2_position));
		MX_DEBUG( 1,("%s: z3_position = %g", fname, z3_position));
		MX_DEBUG( 1,("%s: y1_position = %g", fname, y1_position));
		MX_DEBUG( 1,("%s: y2_position = %g", fname, y2_position));

		roll_angle = mxd_adc_table_compute_roll_angle(
				adc_table, z1_position,
				z2_position, z3_position );

		MX_DEBUG( 1,("%s: roll_angle = %g", fname, roll_angle));

		delta_roll = new_destination - roll_angle;

		MX_DEBUG( 1,("%s: delta_roll = %g", fname, delta_roll));

		/* Assuming that all of delta_roll was implemented
		 * by a change in z3, how much of a change in z3
		 * would be required?
		 */

		delta_z3 = delta_roll * adc_table->m3;

		MX_DEBUG( 1,("%s: delta_z3 = %g", fname, delta_z3));

		/* How much would the height (Z) of the rotation
		 * center change if we performed this move?
		 */

		z_position = mxd_adc_table_compute_z_position(
				adc_table, z1_position,
				z2_position, z3_position );

		MX_DEBUG( 1,("%s: z_position = %g", fname, z_position));

		test_z_position = mxd_adc_table_compute_z_position(
				adc_table, z1_position,
				z2_position, z3_position + delta_z3 );

		MX_DEBUG( 1,("%s: test_z_position = %g",
					fname, test_z_position));

		delta_z = test_z_position - z_position;

		MX_DEBUG( 1,("%s: delta_z = %g", fname, delta_z));

		/* Since we _actually_ want the height (Z) of the
		 * rotation center to stay constant, we must compute
		 * new values of z1, z2, and z3 that have delta_z
		 * subtracted from all of them.
		 */

		destination_array[0] = z1_position - delta_z;
		destination_array[1] = z2_position - delta_z;
		destination_array[2] = z3_position + delta_z3 - delta_z;

		/* The roll operation also changes the y position of
		 * the rotation center.  We need to compensate for that.
		 *
		 * Note that at present, since the function
		 * mxd_adc_table_compute_y_position() does not have the
		 * z values as arguments, the y position reported to the
		 * user after a roll will be slightly different than it
		 * was before the roll.
		 */

#if 0
		y_position = mxd_adc_table_compute_y_position(
				adc_table, y1_position, y2_position );
#endif

		rotation_radius = sqrt( (adc_table->ry * adc_table->ry)
					+ (adc_table->rz * adc_table->rz) );

		MX_DEBUG( 1,("%s: rotation_radius = %g",
				fname, rotation_radius));

		cos_theta_zero
			= mx_divide_safely( adc_table->ry, rotation_radius );

		MX_DEBUG( 1,("%s: cos_theta_zero = %g",
				fname, cos_theta_zero));

		if ( cos_theta_zero > 1.0 ) {
			cos_theta_zero = 1.0;
		}
		if ( cos_theta_zero < -1.0) {
			cos_theta_zero = -1.0;
		}

		MX_DEBUG( 1,("%s: cos_theta_zero (fixed) = %g",
				fname, cos_theta_zero));

		theta_zero = acos( cos_theta_zero );

		if ( adc_table->rz < 0.0 ) {
			theta_zero = -theta_zero;
		}

		MX_DEBUG( 1,("%s: theta_zero = %.10g", fname, theta_zero));

		delta_y = rotation_radius * delta_roll
				* sin( theta_zero + roll_angle );

		MX_DEBUG( 1,("%s: theta_zero + roll_angle = %.10g",
				fname, theta_zero + roll_angle));

		MX_DEBUG( 1,("%s: delta_y = %g", fname, delta_y));

		destination_array[3] = y1_position - delta_y;
		destination_array[4] = y2_position - delta_y;

		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal axis_id %d was specified.", table->axis_id );
		break;
	}

	/* Check to see if any of the new destinations would exceed
	 * a software limit.
	 */

	for ( i = 0; i < num_motors; i++ ) {

		motor = (MX_MOTOR *) subset_array[i]->record_class_struct;

		if ( destination_array[i] > motor->positive_limit ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested move would require motor '%s' to move to %g %s."
		"  This would exceed %s's positive limit at %g %s.",
				motor->record->name,
				destination_array[i],
				motor->units,
				motor->record->name,
				motor->positive_limit,
				motor->units );
		}
		if ( destination_array[i] < motor->negative_limit ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
		"The requested move would require motor '%s' to move to %g %s."
		"  This would exceed %s's negative limit at %g %s.",
				motor->record->name,
				destination_array[i],
				motor->units,
				motor->record->name,
				motor->negative_limit,
				motor->units );
		}
#if 1
		MX_DEBUG( 1,("%s: motor '%s' moving to %g",
				fname, motor->record->name,
				destination_array[i]));
#endif
	}

	/* Start the move. */
		
	status = mx_motor_array_move_absolute( num_motors, subset_array,
					destination_array, 0 );

	return status;
}

MX_EXPORT mx_status_type
mxd_adc_table_get_position( MX_TABLE *table )
{
	const char fname[] = "mxd_adc_table_get_position()";

	MX_ADC_TABLE *adc_table;
	MX_RECORD **marray;
	double x1_position, y1_position, y2_position;
	double z1_position, z2_position, z3_position;
	mx_status_type status;

	status = mxd_adc_table_get_pointers(table, &adc_table, fname);

	if ( status.code != MXE_SUCCESS )
		return status;

	table->position = 0;

	marray = adc_table->motor_record_array;

	switch( table->axis_id ) {
	case MXF_TAB_X:
		status = mx_motor_get_position( marray[ MXF_ADC_TABLE_X1 ],
							&x1_position );

		if ( status.code != MXE_SUCCESS )
			return status;

		table->position = x1_position;
		break;

	case MXF_TAB_Y:
		status = mx_motor_get_position( marray[ MXF_ADC_TABLE_Y1 ],
							&y1_position );

		if ( status.code != MXE_SUCCESS )
			return status;

		status = mx_motor_get_position( marray[ MXF_ADC_TABLE_Y2 ],
							&y2_position );

		if ( status.code != MXE_SUCCESS )
			return status;

		table->position = mxd_adc_table_compute_y_position( adc_table,
						y1_position, y2_position );
		break;

	case MXF_TAB_Z:
		status = mx_motor_get_position( marray[ MXF_ADC_TABLE_Z1 ],
							&z1_position );

		if ( status.code != MXE_SUCCESS )
			return status;

		status = mx_motor_get_position( marray[ MXF_ADC_TABLE_Z2 ],
							&z2_position );

		if ( status.code != MXE_SUCCESS )
			return status;

		status = mx_motor_get_position( marray[ MXF_ADC_TABLE_Z3 ],
							&z3_position );

		if ( status.code != MXE_SUCCESS )
			return status;

		table->position = mxd_adc_table_compute_z_position( adc_table,
				z1_position, z2_position, z3_position );
		break;

	case MXF_TAB_ROLL:
		status = mx_motor_get_position( marray[ MXF_ADC_TABLE_Z1 ],
							&z1_position );

		if ( status.code != MXE_SUCCESS )
			return status;

		status = mx_motor_get_position( marray[ MXF_ADC_TABLE_Z2 ],
							&z2_position );

		if ( status.code != MXE_SUCCESS )
			return status;

		status = mx_motor_get_position( marray[ MXF_ADC_TABLE_Z3 ],
							&z3_position );

		if ( status.code != MXE_SUCCESS )
			return status;

		table->position = mxd_adc_table_compute_roll_angle( adc_table,
				z1_position, z2_position, z3_position );
		break;

	case MXF_TAB_PITCH:
		status = mx_motor_get_position( marray[ MXF_ADC_TABLE_Z1 ],
							&z1_position );

		if ( status.code != MXE_SUCCESS )
			return status;

		status = mx_motor_get_position( marray[ MXF_ADC_TABLE_Z2 ],
							&z2_position );

		if ( status.code != MXE_SUCCESS )
			return status;

		table->position = mx_divide_safely( z1_position - z2_position,
						adc_table->m1 + adc_table->m2);

		break;

	case MXF_TAB_YAW:
		status = mx_motor_get_position( marray[ MXF_ADC_TABLE_Y1 ],
							&y1_position );

		if ( status.code != MXE_SUCCESS )
			return status;

		status = mx_motor_get_position( marray[ MXF_ADC_TABLE_Y2 ],
							&y2_position );

		if ( status.code != MXE_SUCCESS )
			return status;

		table->position = mx_divide_safely( y1_position - y2_position,
						adc_table->m1 + adc_table->m2);

		break;


	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal axis_id %d was specified.", table->axis_id );
		break;
	}

	return status;
}

MX_EXPORT mx_status_type
mxd_adc_table_set_position( MX_TABLE *table )
{
	const char fname[] = "mxd_adc_table_set_position()";

	MX_ADC_TABLE *adc_table;
	double new_set_position;
	mx_status_type status;

	status = mxd_adc_table_get_pointers(table, &adc_table, fname);

	if ( status.code != MXE_SUCCESS )
		return status;

	new_set_position = table->set_position;

	MX_DEBUG(-2,
		("%s: Would redefine table '%s' axis %d position to %g here.",
		fname, table->record->name,
		table->axis_id, new_set_position));

	return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
	"This function is not yet implemented for table '%s'",
		table->record->name );
}

MX_EXPORT mx_status_type
mxd_adc_table_soft_abort( MX_TABLE *table )
{
	const char fname[] = "mxd_adc_table_soft_abort()";

	MX_ADC_TABLE *adc_table;
	MX_RECORD *subset_array[ MX_ADC_TABLE_NUM_MOTORS ];
	MX_RECORD *motor_record;
	int i, num_motors;
	mx_status_type status, status_to_return;

	status = mxd_adc_table_get_pointers(table, &adc_table, fname);

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mxd_adc_table_construct_table_subset( table, adc_table,
					&num_motors, subset_array );

	if ( status.code != MXE_SUCCESS )
		return status;

	status_to_return = MX_SUCCESSFUL_RESULT;

	for ( i = 0; i < num_motors; i++ ) {

		motor_record = subset_array[i];

		status = mx_motor_soft_abort( motor_record );

		if ( status.code != MXE_SUCCESS )
			status_to_return = status;
	}

	return status_to_return;
}

MX_EXPORT mx_status_type
mxd_adc_table_immediate_abort( MX_TABLE *table )
{
	const char fname[] = "mxd_adc_table_immediate_abort()";

	MX_ADC_TABLE *adc_table;
	MX_RECORD *subset_array[ MX_ADC_TABLE_NUM_MOTORS ];
	MX_RECORD *motor_record;
	int i, num_motors;
	mx_status_type status, status_to_return;

	status = mxd_adc_table_get_pointers(table, &adc_table, fname);

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mxd_adc_table_construct_table_subset( table, adc_table,
					&num_motors, subset_array );

	if ( status.code != MXE_SUCCESS )
		return status;

	status_to_return = MX_SUCCESSFUL_RESULT;

	for ( i = 0; i < num_motors; i++ ) {

		motor_record = subset_array[i];

		status = mx_motor_immediate_abort( motor_record );

		if ( status.code != MXE_SUCCESS )
			status_to_return = status;
	}

	return status_to_return;
}

MX_EXPORT mx_status_type
mxd_adc_table_positive_limit_hit( MX_TABLE *table )
{
	const char fname[] = "mxd_adc_table_positive_limit_hit()";

	MX_ADC_TABLE *adc_table;
	MX_RECORD *subset_array[ MX_ADC_TABLE_NUM_MOTORS ];
	MX_RECORD *motor_record;
	int i, limit_hit, num_motors;
	mx_status_type status;

	status = mxd_adc_table_get_pointers(table, &adc_table, fname);

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mxd_adc_table_construct_table_subset( table, adc_table,
					&num_motors, subset_array );

	if ( status.code != MXE_SUCCESS )
		return status;

	table->positive_limit_hit = FALSE;

	for ( i = 0; i < num_motors; i++ ) {

		motor_record = subset_array[i];

		status = mx_motor_positive_limit_hit(motor_record, &limit_hit);

		if ( status.code != MXE_SUCCESS )
			return status;

		if ( limit_hit ) {
			table->positive_limit_hit = TRUE;

			break;		/* Exit the for() loop. */
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_adc_table_negative_limit_hit( MX_TABLE *table )
{
	const char fname[] = "mxd_adc_table_negative_limit_hit()";

	MX_ADC_TABLE *adc_table;
	MX_RECORD *subset_array[ MX_ADC_TABLE_NUM_MOTORS ];
	MX_RECORD *motor_record;
	int i, limit_hit, num_motors;
	mx_status_type status;

	status = mxd_adc_table_get_pointers(table, &adc_table, fname);

	if ( status.code != MXE_SUCCESS )
		return status;

	status = mxd_adc_table_construct_table_subset( table, adc_table,
					&num_motors, subset_array );

	if ( status.code != MXE_SUCCESS )
		return status;

	table->negative_limit_hit = FALSE;

	for ( i = 0; i < num_motors; i++ ) {

		motor_record = subset_array[i];

		status = mx_motor_negative_limit_hit(motor_record, &limit_hit);

		if ( status.code != MXE_SUCCESS )
			return status;

		if ( limit_hit ) {
			table->negative_limit_hit = TRUE;

			break;		/* Exit the for() loop. */
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

