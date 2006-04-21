/*
 * Name:    d_picomotor.h
 *
 * Purpose: Header file for MX drivers for New Focus Picomotor controllers.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------------
 *
 * Copyright 2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_PICOMOTOR_H__
#define __D_PICOMOTOR_H__

/* Values for the driver_type variable. */

#define MXF_PICOMOTOR_UNKNOWN_DRIVER		0
#define MXF_PICOMOTOR_8751_DRIVER		8751
#define MXF_PICOMOTOR_8753_DRIVER		8753

/* Values for the flags variable. */

#define MXF_PICOMOTOR_HOME_TO_LIMIT_SWITCH	0x1

typedef struct {
	MX_RECORD *picomotor_controller_record;
	char driver_name[MXU_PICOMOTOR_DRIVER_NAME_LENGTH+1];
	long motor_number;
	unsigned long flags;

	/* The following is only used by the Model 8753 which
	 * does not report absolute positions.
	 */

	long position_at_start_of_last_move;
} MX_PICOMOTOR;

MX_API mx_status_type mxd_picomotor_create_record_structures(
						MX_RECORD *record );
MX_API mx_status_type mxd_picomotor_finish_record_initialization(
						MX_RECORD *record );
MX_API mx_status_type mxd_picomotor_print_structure( FILE *file,
						MX_RECORD *record );
MX_API mx_status_type mxd_picomotor_open( MX_RECORD *record );
MX_API mx_status_type mxd_picomotor_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxd_picomotor_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_picomotor_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_picomotor_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_picomotor_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_picomotor_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_picomotor_find_home_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_picomotor_constant_velocity_move( MX_MOTOR *motor );
MX_API mx_status_type mxd_picomotor_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_picomotor_set_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_picomotor_simultaneous_start( long num_motor_records,
						MX_RECORD **motor_record_array,
						double *position_array,
						unsigned long flags );
MX_API mx_status_type mxd_picomotor_get_status( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_picomotor_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_picomotor_motor_function_list;

extern long mxd_picomotor_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_picomotor_rfield_def_ptr;

#define MXD_PICOMOTOR_STANDARD_FIELDS \
  {-1, -1, "picomotor_controller_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_PICOMOTOR, picomotor_controller_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "driver_name", MXFT_STRING, NULL, \
	  			1, {MXU_PICOMOTOR_DRIVER_NAME_LENGTH},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_PICOMOTOR, driver_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "motor_number", MXFT_LONG, NULL, 0, {0},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_PICOMOTOR, motor_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PICOMOTOR, flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION }

#endif /* __D_PICOMOTOR_H__ */
