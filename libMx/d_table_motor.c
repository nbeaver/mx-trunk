/*
 * Name:    d_table_motor.c 
 *
 * Purpose: MX motor driver for table pseudomotors.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2000-2001, 2003, 2006, 2010-2011, 2013, 2020
 *    Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_driver.h"
#include "mx_motor.h"
#include "mx_table.h"
#include "d_table_motor.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_table_motor_record_function_list = {
	NULL,
	mxd_table_motor_create_record_structures,
	mxd_table_motor_finish_record_initialization,
	NULL,
	mxd_table_motor_print_motor_structure,
	NULL,
	NULL,
	NULL,
	mxd_table_motor_resynchronize
};

MX_MOTOR_FUNCTION_LIST mxd_table_motor_motor_function_list = {
	mxd_table_motor_motor_is_busy,
	mxd_table_motor_move_absolute,
	mxd_table_motor_get_position,
	mxd_table_motor_set_position,
	mxd_table_motor_soft_abort,
	mxd_table_motor_immediate_abort,
	mxd_table_motor_positive_limit_hit,
	mxd_table_motor_negative_limit_hit,
	NULL,
	NULL,
	mx_motor_default_get_parameter_handler,
	mx_motor_default_set_parameter_handler
};

/* Soft motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_table_motor_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_TABLE_MOTOR_STANDARD_FIELDS
};

long mxd_table_motor_num_record_fields
		= sizeof( mxd_table_motor_record_field_defaults )
			/ sizeof( mxd_table_motor_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_table_motor_rfield_def_ptr
			= &mxd_table_motor_record_field_defaults[0];

/*=======================================================================*/

/* This function is a utility function to consolidate all of the
 * pointer mangling that often has to happen at the beginning of an 
 * mxd_table_motor_...  function.  The parameter 'calling_fname'
 * is passed so that error messages will appear with the name of
 * the calling function.
 */

static mx_status_type
mxd_table_motor_get_pointers( MX_MOTOR *motor,
			MX_TABLE_MOTOR **table_motor,
			const char *calling_fname )
{
	static const char fname[] = "mxd_table_motor_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The motor pointer passed by '%s' was NULL",
			calling_fname );
	}

	if ( motor->record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_RECORD pointer for motor pointer passed by '%s' is NULL.",
			calling_fname );
	}

	if ( motor->record->mx_type != MXT_MTR_TABLE ) {
		return mx_error( MXE_TYPE_MISMATCH, fname,
	"The motor '%s' passed by '%s' is not a table motor.  "
	"(superclass = %ld, class = %ld, type = %ld)",
			motor->record->name, calling_fname,
			motor->record->mx_superclass,
			motor->record->mx_class,
			motor->record->mx_type );
	}

	if ( table_motor != (MX_TABLE_MOTOR **) NULL ) {

		*table_motor = (MX_TABLE_MOTOR *)
				(motor->record->record_type_struct);

		if ( *table_motor == (MX_TABLE_MOTOR *) NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
"The MX_TABLE_MOTOR pointer for motor record '%s' passed by '%s' is NULL",
				motor->record->name, calling_fname );
		}
	}

	if ( (*table_motor)->table_record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
	"The MX server record pointer for MX table motor '%s' is NULL.",
			motor->record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

/*=======================================================================*/

MX_EXPORT mx_status_type
mxd_table_motor_create_record_structures( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_table_motor_create_record_structures()";

	MX_MOTOR *motor;
	MX_TABLE_MOTOR *table_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	table_motor = (MX_TABLE_MOTOR *) malloc( sizeof(MX_TABLE_MOTOR) );

	if ( table_motor == (MX_TABLE_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_TABLE_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = table_motor;
	record->class_specific_function_list
				= &mxd_table_motor_motor_function_list;

	motor->record = record;

	/* A table motor is treated as an analog motor. */

	motor->subclass = MXC_MTR_ANALOG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_table_motor_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[] =
		"mxd_table_motor_finish_record_initialization()";

	MX_MOTOR *motor;
	MX_TABLE_MOTOR *table_motor;
	mx_status_type mx_status;

	if ( record == NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_RECORD pointer passed is NULL." );
	}

	mx_status = mx_motor_finish_record_initialization( record );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor = (MX_MOTOR *) record->record_class_struct;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	motor->motor_flags |= MXF_MTR_IS_PSEUDOMOTOR;
	motor->motor_flags |= MXF_MTR_CANNOT_QUICK_SCAN;

	table_motor = (MX_TABLE_MOTOR *) record->record_type_struct;

	if ( table_motor == (MX_TABLE_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"MX_TABLE_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	/* Does the table_record field point to a real table record? */

	if ( table_motor->table_record->mx_class != MXC_TABLE ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Record '%s' used by table motor '%s' is not a table record.",
			table_motor->table_record->name,
			record->name );
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_table_motor_print_motor_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] = "mxd_table_motor_print_motor_structure()";

	MX_MOTOR *motor;
	MX_TABLE_MOTOR *table_motor;
	double position, move_deadband;
	mx_status_type mx_status;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_MOTOR pointer for record '%s' is NULL.",
			record->name);
	}

	table_motor = (MX_TABLE_MOTOR *) (record->record_type_struct);

	if ( table_motor == (MX_TABLE_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_TABLE_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);
	fprintf(file, "  Motor type     = TABLE_MOTOR.\n\n");

	fprintf(file, "  name           = %s\n", record->name);
	fprintf(file, "  table record   = %s\n",
					table_motor->table_record->name );
	fprintf(file, "  axis id        = %ld\n",
					table_motor->axis_id );

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "  position       = %g %s (%g)\n",
		motor->position, motor->units, motor->raw_position.analog );

	fprintf(file, "  scale          = %g %s per step.\n",
		motor->scale, motor->units );

	fprintf(file, "  offset         = %g %s.\n",
		motor->offset, motor->units );

	fprintf(file, "  backlash       = %g %s (%g)\n",
		motor->backlash_correction, motor->units,
		motor->raw_backlash_correction.analog );

	fprintf(file, "  negative_limit = %g %s (%g)\n",
		motor->negative_limit, motor->units,
		motor->raw_negative_limit.analog );

	fprintf(file, "  positive_limit = %g %s (%g)\n",
		motor->positive_limit, motor->units,
		motor->raw_positive_limit.analog );

	move_deadband = motor->scale * motor->raw_move_deadband.analog;

	fprintf(file, "  move deadband  = %g %s (%g)\n\n",
		move_deadband, motor->units,
		motor->raw_move_deadband.analog );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_table_motor_resynchronize( MX_RECORD *record )
{
	static const char fname[] = "mxd_table_motor_resynchronize()";

	MX_MOTOR *motor;
	MX_TABLE_MOTOR *table_motor;
	mx_status_type mx_status;

	motor = (MX_MOTOR *) record->record_class_struct;

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
			"MX_MOTOR pointer for record '%s' is NULL.",
			record->name );
	}

	mx_status = mxd_table_motor_get_pointers(motor, &table_motor, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_resynchronize_record( table_motor->table_record );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_table_motor_motor_is_busy( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_table_motor_motor_is_busy()";

	MX_TABLE_MOTOR *table_motor;
	mx_bool_type busy;
	mx_status_type mx_status;

	mx_status = mxd_table_motor_get_pointers(motor, &table_motor, fname);

	if ( mx_status.code != MXE_SUCCESS ) {
		motor->busy = FALSE;

		return mx_status;
	}

	mx_status = mx_table_is_busy( table_motor->table_record,
				table_motor->axis_id, &busy );

	motor->busy = busy;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_table_motor_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_table_motor_move_absolute()";

	MX_TABLE_MOTOR *table_motor;
	double new_destination;
	mx_status_type mx_status;

	mx_status = mxd_table_motor_get_pointers(motor, &table_motor, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	new_destination = motor->raw_destination.analog;

	mx_status = mx_table_move_absolute( table_motor->table_record,
					table_motor->axis_id,
					new_destination );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_table_motor_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_table_motor_get_position()";

	MX_TABLE_MOTOR *table_motor;
	double position;
	mx_status_type mx_status;

	mx_status = mxd_table_motor_get_pointers(motor, &table_motor, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_table_get_position( table_motor->table_record,
					table_motor->axis_id,
					&position );

	motor->raw_position.analog = position;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_table_motor_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_table_motor_set_position()";

	MX_TABLE_MOTOR *table_motor;
	double new_set_position;
	mx_status_type mx_status;

	mx_status = mxd_table_motor_get_pointers(motor, &table_motor, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	new_set_position = motor->raw_set_position.analog;

	mx_status = mx_table_set_position( table_motor->table_record,
					table_motor->axis_id,
					new_set_position );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_table_motor_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_table_motor_soft_abort()";

	MX_TABLE_MOTOR *table_motor;
	mx_status_type mx_status;

	mx_status = mxd_table_motor_get_pointers(motor, &table_motor, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_table_soft_abort( table_motor->table_record,
					table_motor->axis_id );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_table_motor_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_table_motor_immediate_abort()";

	MX_TABLE_MOTOR *table_motor;
	mx_status_type mx_status;

	mx_status = mxd_table_motor_get_pointers(motor, &table_motor, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_table_immediate_abort( table_motor->table_record,
					table_motor->axis_id );

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_table_motor_positive_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_table_motor_positive_limit_hit()";

	MX_TABLE_MOTOR *table_motor;
	mx_bool_type limit_hit;
	mx_status_type mx_status;

	mx_status = mxd_table_motor_get_pointers(motor, &table_motor, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_table_positive_limit_hit( table_motor->table_record,
						table_motor->axis_id,
						&limit_hit );

	motor->positive_limit_hit = limit_hit;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_table_motor_negative_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_table_motor_negative_limit_hit()";

	MX_TABLE_MOTOR *table_motor;
	mx_bool_type limit_hit;
	mx_status_type mx_status;

	mx_status = mxd_table_motor_get_pointers(motor, &table_motor, fname);

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_table_negative_limit_hit( table_motor->table_record,
						table_motor->axis_id,
						&limit_hit );

	motor->negative_limit_hit = limit_hit;

	return mx_status;
}

