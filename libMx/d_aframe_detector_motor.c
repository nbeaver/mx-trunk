/*
 * Name:    d_aframe_detector_motor.c 
 *
 * Purpose: MX motor driver for the pseudomotors used by Gerd Rosenbaum's
 *          A-frame CCD detector mount.
 *
 *          The pseudomotors available are:
 *
 *          detector_distance - This is the length of the line perpendicular
 *              to the plane containing the front face of the detector which
 *              passes through the center of rotation of the goniometer head.
 *
 *          detector_horizontal_angle - This is the angle between the line
 *              used by the detector distance and the horizontal plane.
 *
 *          detector_offset - This is the distance between the centerline
 *              of the detector and the line used to define the detector
 *              distance above.
 *
 *          There are three constants that describe the system:
 *
 *          A - This is the perpendicular distance between the two vertical
 *              supports that hold up the detector.
 *
 *          B - This is the distance along the centerline of the detector
 *              from the front face of the detector to the point where
 *              a perpendicular from the downstream detector pivot intersects
 *              this line.
 *
 *          C - This is the separation between the centerline of the detector
 *              and the line defining the detector distance above.
 *
 *          The pseudomotors depend on the positions of three real motors.
 *          These are:
 *
 *          dv_upstream - This motor controls the height of the upstream
 *              vertical detector support.
 *
 *          dv_downstream - This motor controls the height of the downstream
 *              vertical detector support.
 *
 *          dh - This motor controls the horizontal position of the vertical
 *              detector supports.
 *
 *          Confused?  I am planning to write a short document that describes
 *          the definitions of these parameters in more detail and derives
 *          the formulas describing them.  If you are reading this comment
 *          and I have not yet written that document, then pester me until
 *          I do write it.
 *
 * Warning: The detector horizontal angle is expressed internally in radians.
 *          If you want to display the angle in degrees, use the scale field
 *          of the angle pseudomotor to do the conversion.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2002-2003, 2006-2007 Illinois Institute of Technology
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
#include "mx_motor.h"
#include "mx_variable.h"
#include "d_aframe_detector_motor.h"

/* Initialize the motor driver jump table. */

MX_RECORD_FUNCTION_LIST mxd_aframe_det_motor_record_function_list = {
	mxd_aframe_det_motor_initialize_type,
	mxd_aframe_det_motor_create_record_structures,
	mxd_aframe_det_motor_finish_record_initialization,
	mxd_aframe_det_motor_delete_record,
	mxd_aframe_det_motor_print_motor_structure,
	mxd_aframe_det_motor_read_parms_from_hardware,
	mxd_aframe_det_motor_write_parms_to_hardware,
	mxd_aframe_det_motor_open,
	mxd_aframe_det_motor_close,
	NULL,
	mxd_aframe_det_motor_resynchronize
};

MX_MOTOR_FUNCTION_LIST mxd_aframe_det_motor_motor_function_list = {
	mxd_aframe_det_motor_is_busy,
	mxd_aframe_det_motor_move_absolute,
	mxd_aframe_det_motor_get_position,
	mxd_aframe_det_motor_set_position,
	mxd_aframe_det_motor_soft_abort,
	mxd_aframe_det_motor_immediate_abort,
	mxd_aframe_det_motor_positive_limit_hit,
	mxd_aframe_det_motor_negative_limit_hit,
	NULL,
	NULL,
	mxd_aframe_det_motor_get_parameter,
	mxd_aframe_det_motor_set_parameter
};

/* Soft motor data structures. */

MX_RECORD_FIELD_DEFAULTS mxd_aframe_det_motor_record_field_defaults[] = {
	MX_RECORD_STANDARD_FIELDS,
	MX_ANALOG_MOTOR_STANDARD_FIELDS,
	MX_MOTOR_STANDARD_FIELDS,
	MXD_AFRAME_DETECTOR_MOTOR_STANDARD_FIELDS
};

long mxd_aframe_det_motor_num_record_fields
		= sizeof( mxd_aframe_det_motor_record_field_defaults )
			/ sizeof( mxd_aframe_det_motor_record_field_defaults[0] );

MX_RECORD_FIELD_DEFAULTS *mxd_aframe_det_motor_rfield_def_ptr
			= &mxd_aframe_det_motor_record_field_defaults[0];

/* The following functions are declared for internal use by the driver. */

static mx_status_type
mxd_aframe_det_motor_get_real_positions(
			MX_AFRAME_DETECTOR_MOTOR *aframe_detector_motor );

static mx_status_type
mxd_aframe_det_motor_compute_pseudomotor_position(
			MX_AFRAME_DETECTOR_MOTOR *aframe_detector_motor,
			double *position );

static mx_status_type
mxd_aframe_det_motor_compute_new_real_positions(
			MX_AFRAME_DETECTOR_MOTOR *aframe_detector_motor,
			double old_pseudomotor_position,
			double new_pseudomotor_position,
			double *new_dv_downstream,
			double *new_dv_upstream,
			double *new_dh );

/* A private function for the use of the driver. */

static mx_status_type
mxd_aframe_det_motor_get_pointers( MX_MOTOR *motor,
			MX_AFRAME_DETECTOR_MOTOR **aframe_detector_motor,
			const char *calling_fname )
{
	static const char fname[] = "mxd_aframe_det_motor_get_pointers()";

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
			"The MX_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}
	if ( aframe_detector_motor == (MX_AFRAME_DETECTOR_MOTOR **) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"The MX_AFRAME_DETECTOR_MOTOR pointer passed by '%s' was NULL.",
			calling_fname );
	}

	*aframe_detector_motor = (MX_AFRAME_DETECTOR_MOTOR *)
					motor->record->record_type_struct;

	if ( *aframe_detector_motor == (MX_AFRAME_DETECTOR_MOTOR *) NULL ) {
		return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
		"The MX_AFRAME_DETECTOR_MOTOR pointer for record '%s' is NULL.",
			motor->record->name );
	}
	return MX_SUCCESSFUL_RESULT;
}

/* === */

MX_EXPORT mx_status_type
mxd_aframe_det_motor_initialize_type( long type )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aframe_det_motor_create_record_structures( MX_RECORD *record )
{
	static const char fname[] = "mxd_aframe_det_motor_create_record_structures()";

	MX_MOTOR *motor;
	MX_AFRAME_DETECTOR_MOTOR *aframe_detector_motor;

	/* Allocate memory for the necessary structures. */

	motor = (MX_MOTOR *) malloc( sizeof(MX_MOTOR) );

	if ( motor == (MX_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
		"Can't allocate memory for MX_MOTOR structure." );
	}

	aframe_detector_motor = (MX_AFRAME_DETECTOR_MOTOR *)
				malloc( sizeof(MX_AFRAME_DETECTOR_MOTOR) );

	if ( aframe_detector_motor == (MX_AFRAME_DETECTOR_MOTOR *) NULL ) {
		return mx_error( MXE_OUT_OF_MEMORY, fname,
	"Can't allocate memory for MX_AFRAME_DETECTOR_MOTOR structure." );
	}

	/* Now set up the necessary pointers. */

	record->record_class_struct = motor;
	record->record_type_struct = aframe_detector_motor;
	record->class_specific_function_list
				= &mxd_aframe_det_motor_motor_function_list;

	motor->record = record;

	aframe_detector_motor->record = record;

	/* An A-frame detector motor is treated as an analog_motor. */

	motor->subclass = MXC_MTR_ANALOG;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aframe_det_motor_finish_record_initialization( MX_RECORD *record )
{
	static const char fname[]
		= "mxd_aframe_det_motor_finish_record_initialization()";

	MX_MOTOR *motor;
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
	motor->motor_flags |= MXF_MTR_CANNOT_QUICK_SCAN;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aframe_det_motor_delete_record( MX_RECORD *record )
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
mxd_aframe_det_motor_print_motor_structure( FILE *file, MX_RECORD *record )
{
	static const char fname[] =
		"mxd_aframe_det_motor_print_motor_structure()";

	MX_MOTOR *motor;
	MX_AFRAME_DETECTOR_MOTOR *aframe_detector_motor;
	double position, move_deadband;
	mx_status_type mx_status;

	aframe_detector_motor = NULL;

	if ( record == (MX_RECORD *) NULL ) {
		return mx_error( MXE_NULL_ARGUMENT, fname,
		"MX_RECORD pointer passed is NULL.");
	}

	motor = (MX_MOTOR *) (record->record_class_struct);

	mx_status = mxd_aframe_det_motor_get_pointers( motor,
						&aframe_detector_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "MOTOR parameters for motor '%s':\n", record->name);
	fprintf(file, "  Motor type     = AFRAME_DETECTOR_MOTOR.\n\n");

	fprintf(file, "  name           = %s\n", record->name);

	mx_status = mx_motor_get_position( record, &position );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	fprintf(file, "  position       = %.*g %s\n",
		record->precision,
		motor->position, motor->units );

	fprintf(file, "  scale          = %.*g %s per raw unit.\n",
		record->precision,
		motor->scale, motor->units );

	fprintf(file, "  offset         = %.*g %s.\n",
		record->precision,
		motor->offset, motor->units );

	fprintf(file, "  backlash       = %.*g %s\n",
		record->precision,
		motor->backlash_correction, motor->units );

	fprintf(file, "  negative_limit = %.*g %s\n",
		record->precision,
		motor->negative_limit, motor->units );

	fprintf(file, "  positive_limit = %.*g %s\n",
		record->precision,
		motor->positive_limit, motor->units );

	move_deadband = motor->scale * (double) motor->raw_move_deadband.analog;

	fprintf(file, "  move_deadband       = %.*g %s\n\n",
		record->precision,
		move_deadband, motor->units );

	switch( aframe_detector_motor->pseudomotor_type ) {
	case MXT_AFRAME_DETECTOR_DISTANCE:
		fprintf(file,
		"  Pseudomotor type is \"Detector distance\" " );
		break;
	case MXT_AFRAME_DETECTOR_HORIZONTAL_ANGLE:
		fprintf(file,
		"  Pseudomotor type is \"Detector horizontal angle\" " );
		break;
	case MXT_AFRAME_DETECTOR_OFFSET:
		fprintf(file,
		"  Pseudomotor type is \"Detector offset\" " );
		break;
	default:
		fprintf(file,
		      "  Warning: unrecognized pseudomotor_type = %ld\n\n",
			aframe_detector_motor->pseudomotor_type);
		break;
	}

	fprintf(file, "(%ld).\n\n", aframe_detector_motor->pseudomotor_type);

	fprintf(file, "  A record       = '%s'\n",
		aframe_detector_motor->A_record->name );

	fprintf(file, "  B record       = '%s'\n",
		aframe_detector_motor->B_record->name );

	fprintf(file, "  C record       = '%s'\n",
		aframe_detector_motor->C_record->name );

	fprintf(file, "  dv_downstream  = '%s'\n",
		aframe_detector_motor->dv_downstream_record->name );

	fprintf(file, "  dv_upstream    = '%s'\n",
		aframe_detector_motor->dv_upstream_record->name );

	fprintf(file, "  dh             = '%s'\n",
		aframe_detector_motor->dh_record->name );

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aframe_det_motor_read_parms_from_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aframe_det_motor_write_parms_to_hardware( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aframe_det_motor_open( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aframe_det_motor_close( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aframe_det_motor_resynchronize( MX_RECORD *record )
{
	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aframe_det_motor_is_busy( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_aframe_det_motor_is_busy()";

	MX_AFRAME_DETECTOR_MOTOR *aframe_detector_motor;
	mx_bool_type busy;
	mx_status_type mx_status;

	aframe_detector_motor = NULL;

	mx_status = mxd_aframe_det_motor_get_pointers( motor,
						&aframe_detector_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->busy = FALSE;

	/* Check the vertical downstream motor. */

	mx_status = mx_motor_is_busy(
				aframe_detector_motor->dv_downstream_record,
				&busy );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( busy ) {
		motor->busy = TRUE;
		return MX_SUCCESSFUL_RESULT;
	}

	/* Check the vertical upstream motor. */

	mx_status = mx_motor_is_busy(
				aframe_detector_motor->dv_upstream_record,
				&busy );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( busy ) {
		motor->busy = TRUE;
		return MX_SUCCESSFUL_RESULT;
	}

	/* Check the horizontal motor. */

	mx_status = mx_motor_is_busy( aframe_detector_motor->dh_record,
				&busy );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( busy ) {
		motor->busy = TRUE;
		return MX_SUCCESSFUL_RESULT;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_aframe_det_motor_move_absolute( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_aframe_det_motor_move_absolute()";

	MX_AFRAME_DETECTOR_MOTOR *aframe_detector_motor;
	double new_dv_downstream_position, new_dv_upstream_position;
	double new_dh_position;
	double old_raw_position;
	mx_status_type mx_status;

	aframe_detector_motor = NULL;

	mx_status = mxd_aframe_det_motor_get_pointers( motor,
						&aframe_detector_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_aframe_det_motor_get_real_positions(
				aframe_detector_motor );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_aframe_det_motor_compute_pseudomotor_position(
				aframe_detector_motor, &old_raw_position );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_aframe_det_motor_compute_new_real_positions(
				aframe_detector_motor,
				old_raw_position,
				motor->raw_destination.analog,
				&new_dv_downstream_position,
				&new_dv_upstream_position,
				&new_dh_position );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 1
	/* Move to the new positions. */

	mx_status = mx_motor_move_absolute(
			aframe_detector_motor->dv_downstream_record,
			new_dv_downstream_position, MXF_MTR_NOWAIT );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_move_absolute(
			aframe_detector_motor->dv_upstream_record,
			new_dv_upstream_position, MXF_MTR_NOWAIT );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_move_absolute(
			aframe_detector_motor->dh_record,
			new_dh_position, MXF_MTR_NOWAIT );
#endif
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_aframe_det_motor_get_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_aframe_det_motor_get_position()";

	MX_AFRAME_DETECTOR_MOTOR *aframe_detector_motor;
	double raw_position;
	mx_status_type mx_status;

	aframe_detector_motor = NULL;

	mx_status = mxd_aframe_det_motor_get_pointers( motor,
					&aframe_detector_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_aframe_det_motor_get_real_positions(
			aframe_detector_motor );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_aframe_det_motor_compute_pseudomotor_position(
			aframe_detector_motor,
			&raw_position );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->raw_position.analog = raw_position;

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mxd_aframe_det_motor_set_position( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_aframe_det_motor_set_position()";

	MX_AFRAME_DETECTOR_MOTOR *aframe_detector_motor;
	double new_dv_downstream_position, new_dv_upstream_position;
	double new_dh_position;
	double old_raw_position;
	mx_status_type mx_status;

	aframe_detector_motor = NULL;

	mx_status = mxd_aframe_det_motor_get_pointers( motor,
						&aframe_detector_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_aframe_det_motor_get_real_positions(
				aframe_detector_motor );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_aframe_det_motor_compute_pseudomotor_position(
				aframe_detector_motor, &old_raw_position );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mxd_aframe_det_motor_compute_new_real_positions(
				aframe_detector_motor,
				old_raw_position,
				motor->raw_set_position.analog,
				&new_dv_downstream_position,
				&new_dv_upstream_position,
				&new_dh_position );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

#if 1
	/* Set the new positions. */

	mx_status = mx_motor_set_position(
			aframe_detector_motor->dv_downstream_record,
			new_dv_downstream_position );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_set_position(
			aframe_detector_motor->dv_upstream_record,
			new_dv_upstream_position );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	mx_status = mx_motor_set_position(
			aframe_detector_motor->dh_record,
			new_dh_position );
#endif
	return mx_status;
}

MX_EXPORT mx_status_type
mxd_aframe_det_motor_soft_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_aframe_det_motor_soft_abort()";

	MX_AFRAME_DETECTOR_MOTOR *aframe_detector_motor;
	mx_status_type mx_status, abort_status;

	aframe_detector_motor = NULL;

	mx_status = mxd_aframe_det_motor_get_pointers( motor,
						&aframe_detector_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	abort_status = mx_motor_soft_abort(
				aframe_detector_motor->dv_downstream_record );

	if ( abort_status.code != MXE_SUCCESS )
		mx_status = abort_status;

	abort_status = mx_motor_soft_abort(
				aframe_detector_motor->dv_upstream_record );

	if ( abort_status.code != MXE_SUCCESS )
		mx_status = abort_status;

	abort_status = mx_motor_soft_abort( aframe_detector_motor->dh_record );

	if ( abort_status.code != MXE_SUCCESS )
		mx_status = abort_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_aframe_det_motor_immediate_abort( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_aframe_det_motor_immediate_abort()";

	MX_AFRAME_DETECTOR_MOTOR *aframe_detector_motor;
	mx_status_type mx_status, abort_status;

	aframe_detector_motor = NULL;

	mx_status = mxd_aframe_det_motor_get_pointers( motor,
						&aframe_detector_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	abort_status = mx_motor_immediate_abort(
				aframe_detector_motor->dv_downstream_record );

	if ( abort_status.code != MXE_SUCCESS )
		mx_status = abort_status;

	abort_status = mx_motor_immediate_abort(
				aframe_detector_motor->dv_upstream_record );

	if ( abort_status.code != MXE_SUCCESS )
		mx_status = abort_status;

	abort_status = mx_motor_immediate_abort(
				aframe_detector_motor->dh_record );

	if ( abort_status.code != MXE_SUCCESS )
		mx_status = abort_status;

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_aframe_det_motor_positive_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_aframe_det_motor_positive_limit_hit()";

	MX_AFRAME_DETECTOR_MOTOR *aframe_detector_motor;
	mx_bool_type positive_limit_hit;
	mx_status_type mx_status;

	aframe_detector_motor = NULL;

	mx_status = mxd_aframe_det_motor_get_pointers( motor,
						&aframe_detector_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->positive_limit_hit = FALSE;

	/* Check the vertical downstream motor. */

	mx_status = mx_motor_positive_limit_hit(
				aframe_detector_motor->dv_downstream_record,
				&positive_limit_hit );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( positive_limit_hit ) {
		motor->positive_limit_hit = TRUE;
		return MX_SUCCESSFUL_RESULT;
	}

	/* Check the vertical upstream motor. */

	mx_status = mx_motor_positive_limit_hit(
				aframe_detector_motor->dv_upstream_record,
				&positive_limit_hit );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( positive_limit_hit ) {
		motor->positive_limit_hit = TRUE;
		return MX_SUCCESSFUL_RESULT;
	}

	/* Check the horizontal motor. */

	mx_status = mx_motor_positive_limit_hit(
				aframe_detector_motor->dh_record,
				&positive_limit_hit );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( positive_limit_hit ) {
		motor->positive_limit_hit = TRUE;
		return MX_SUCCESSFUL_RESULT;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_aframe_det_motor_negative_limit_hit( MX_MOTOR *motor )
{
	static const char fname[] = "mxd_aframe_det_motor_negative_limit_hit()";

	MX_AFRAME_DETECTOR_MOTOR *aframe_detector_motor;
	mx_bool_type negative_limit_hit;
	mx_status_type mx_status;

	aframe_detector_motor = NULL;

	mx_status = mxd_aframe_det_motor_get_pointers( motor,
						&aframe_detector_motor, fname );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	motor->negative_limit_hit = FALSE;

	/* Check the vertical downstream motor. */

	mx_status = mx_motor_negative_limit_hit(
				aframe_detector_motor->dv_downstream_record,
				&negative_limit_hit );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( negative_limit_hit ) {
		motor->negative_limit_hit = TRUE;
		return MX_SUCCESSFUL_RESULT;
	}

	/* Check the vertical upstream motor. */

	mx_status = mx_motor_negative_limit_hit(
				aframe_detector_motor->dv_upstream_record,
				&negative_limit_hit );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( negative_limit_hit ) {
		motor->negative_limit_hit = TRUE;
		return MX_SUCCESSFUL_RESULT;
	}

	/* Check the horizontal motor. */

	mx_status = mx_motor_negative_limit_hit(
				aframe_detector_motor->dh_record,
				&negative_limit_hit );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( negative_limit_hit ) {
		motor->negative_limit_hit = TRUE;
		return MX_SUCCESSFUL_RESULT;
	}

	return mx_status;
}

MX_EXPORT mx_status_type
mxd_aframe_det_motor_get_parameter( MX_MOTOR *motor )
{
	return mx_motor_default_get_parameter_handler( motor );
}

MX_EXPORT mx_status_type
mxd_aframe_det_motor_set_parameter( MX_MOTOR *motor )
{
	return mx_motor_default_set_parameter_handler( motor );
}

/* === */

static mx_status_type
mxd_aframe_det_motor_get_real_positions(
			MX_AFRAME_DETECTOR_MOTOR *aframe_detector_motor )
{
	static const char fname[] = "mxd_aframe_det_motor_get_real_positions()";

	mx_bool_type fast_mode;
	mx_status_type mx_status;

	mx_status = mx_get_fast_mode( aframe_detector_motor->record,
						&fast_mode );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	if ( fast_mode == FALSE ) {
		mx_status = mx_get_double_variable(
					aframe_detector_motor->A_record,
					&(aframe_detector_motor->A_value) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_get_double_variable(
					aframe_detector_motor->B_record,
					&(aframe_detector_motor->B_value) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;

		mx_status = mx_get_double_variable(
					aframe_detector_motor->C_record,
					&(aframe_detector_motor->C_value) );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	mx_status = mx_motor_get_position(
			aframe_detector_motor->dv_downstream_record,
			&(aframe_detector_motor->dv_downstream_position) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: dv_downstream_position = %g", fname,
			aframe_detector_motor->dv_downstream_position));

	mx_status = mx_motor_get_position(
			aframe_detector_motor->dv_upstream_record,
			&(aframe_detector_motor->dv_upstream_position) );

	if ( mx_status.code != MXE_SUCCESS )
		return mx_status;

	MX_DEBUG( 2,("%s: dv_upstream_position = %g", fname,
			aframe_detector_motor->dv_upstream_position));

	mx_status = mx_motor_get_position(
			aframe_detector_motor->dh_record,
			&(aframe_detector_motor->dh_position) );

	MX_DEBUG( 2,("%s: dh_position = %g", fname,
			aframe_detector_motor->dh_position));

	return mx_status;
}

static mx_status_type
mxd_aframe_det_motor_compute_pseudomotor_position(
			MX_AFRAME_DETECTOR_MOTOR *aframe_detector_motor,
			double *position )
{
	static const char fname[] =
		"mxd_aframe_det_motor_compute_pseudomotor_position()";

	double dv_downstream, dv_upstream, dh;
	double A_value, B_value, C_value;
	double tan_det_hor_angle, det_hor_angle;

	A_value       = aframe_detector_motor->A_value;
	B_value       = aframe_detector_motor->B_value;
	C_value       = aframe_detector_motor->C_value;
	dv_downstream = aframe_detector_motor->dv_downstream_position;
	dv_upstream   = aframe_detector_motor->dv_upstream_position;
	dh            = aframe_detector_motor->dh_position;

	/* All of the pseudomotors need the detector horizontal angle. */

	tan_det_hor_angle = mx_divide_safely( dv_downstream - dv_upstream,
							A_value );
	det_hor_angle = atan( tan_det_hor_angle );

	MX_DEBUG( 2,("%s: det_hor_angle = %g", fname, det_hor_angle ));

	switch( aframe_detector_motor->pseudomotor_type ) {
	case MXT_AFRAME_DETECTOR_HORIZONTAL_ANGLE:
		*position = det_hor_angle;
		break;

	case MXT_AFRAME_DETECTOR_DISTANCE:
		*position = dv_downstream * sin( det_hor_angle )
				+ dh * cos( det_hor_angle ) - B_value;
		break;

	case MXT_AFRAME_DETECTOR_OFFSET:
		*position = dv_downstream * cos( det_hor_angle )
				- dh * sin( det_hor_angle ) - C_value;
		break;

	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
			"Pseudomotor type %ld is not recognized.",
			aframe_detector_motor->pseudomotor_type );
	}

	MX_DEBUG( 2,("%s: pseudomotor_position = %g",
		fname, *position ));

	return MX_SUCCESSFUL_RESULT;
}

static mx_status_type
mxd_aframe_det_motor_compute_new_real_positions(
			MX_AFRAME_DETECTOR_MOTOR *aframe_detector_motor,
			double old_pseudomotor_position,
			double new_pseudomotor_position,
			double *new_dv_downstream,
			double *new_dv_upstream,
			double *new_dh )
{
	static const char fname[] =
		"mxd_aframe_det_motor_compute_new_real_positions()";

	double dv_downstream, dv_upstream, dh, A_value;
	double tan_det_hor_angle, det_hor_angle;
	double distance_change, offset_change, angle_change;

	A_value       = aframe_detector_motor->A_value;
	dv_downstream = aframe_detector_motor->dv_downstream_position;
	dv_upstream   = aframe_detector_motor->dv_upstream_position;
	dh            = aframe_detector_motor->dh_position;

	/* All of the pseudomotors need the detector horizontal angle. */

	tan_det_hor_angle = mx_divide_safely( dv_downstream - dv_upstream,
							A_value );
	det_hor_angle = atan( tan_det_hor_angle );

	MX_DEBUG( 2,("%s: det_hor_angle = %g", fname, det_hor_angle ));

	switch ( aframe_detector_motor->pseudomotor_type ) {
	case MXT_AFRAME_DETECTOR_DISTANCE:

		/* Change the detector distance, holding the detector
		 * horizontal angle and the detector offset constant.
		 */

		distance_change = new_pseudomotor_position
					- old_pseudomotor_position;

		MX_DEBUG( 2,("%s: distance_change = %g",
				fname, distance_change ));

		*new_dv_downstream = dv_downstream
				+ sin( det_hor_angle ) * distance_change;

		*new_dv_upstream = dv_upstream
				+ sin( det_hor_angle ) * distance_change;

		*new_dh = dh + cos( det_hor_angle ) * distance_change;

		break;

	case MXT_AFRAME_DETECTOR_OFFSET:

		/* Change the detector offset, holding the detector
		 * distance and the detector horizontal angle constant.
		 */

		offset_change = new_pseudomotor_position
					- old_pseudomotor_position;

		MX_DEBUG( 2,("%s: offset_change = %g",
				fname, offset_change ));

		*new_dv_downstream = dv_downstream
				+ cos( det_hor_angle ) * offset_change;

		*new_dv_upstream = dv_upstream
				+ cos( det_hor_angle ) * offset_change;

		*new_dh = dh - sin( det_hor_angle ) * offset_change;

		break;

	case MXT_AFRAME_DETECTOR_HORIZONTAL_ANGLE:

		/* Change the detector horizontal angle, holding the
		 * detector distance and the detector offset constant.
		 */

		angle_change = new_pseudomotor_position
					- old_pseudomotor_position;

		MX_DEBUG( 2,("%s: angle_change = %g",
				fname, angle_change ));

		*new_dv_downstream = dv_downstream * cos( angle_change )
					+ dh * sin( angle_change );

		*new_dh = dh * cos( angle_change )
					- dv_downstream * sin( angle_change );

		*new_dv_upstream = dv_upstream
			+ ( *new_dv_downstream - dv_downstream )
			- A_value * ( tan( new_pseudomotor_position )
					- tan( old_pseudomotor_position ) );

		break;
	default:
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Pseudomotor type %ld is not recognized.",
			aframe_detector_motor->pseudomotor_type );
	}

	MX_DEBUG( 2,("%s: *new_dv_downstream = %g", fname, *new_dv_downstream));
	MX_DEBUG( 2,("%s: *new_dv_upstream = %g", fname, *new_dv_upstream));
	MX_DEBUG( 2,("%s: *new_dh = %g", fname, *new_dh ));

	return MX_SUCCESSFUL_RESULT;
}

