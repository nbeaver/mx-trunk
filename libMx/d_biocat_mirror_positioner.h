/*
 * Name:    d_biocat_mirror_positioner.h
 *
 * Purpose: Header file for MX driver that coordinates the moves of
 *          the BioCAT (APS 18-ID) mirror angle, downstream angle,
 *          collimator and table positions.
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_BIOCAT_MIRROR_POSITIONER_H__
#define __D_BIOCAT_MIRROR_POSITIONER_H__

typedef struct {
	MX_RECORD *record;

	long num_motors;
	MX_RECORD **motor_record_array;
	double *motor_scale_array;
} MX_BIOCAT_MIRROR_POSITIONER;

MX_API mx_status_type mxd_biocat_mirror_pos_initialize_driver(
							MX_DRIVER *driver );
MX_API mx_status_type mxd_biocat_mirror_pos_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_biocat_mirror_pos_open( MX_RECORD *record );

MX_API mx_status_type mxd_biocat_mirror_pos_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_biocat_mirror_pos_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_biocat_mirror_pos_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_biocat_mirror_pos_get_status( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_biocat_mirror_pos_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_biocat_mirror_pos_motor_function_list;

extern long mxd_biocat_mirror_pos_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_biocat_mirror_pos_rfield_def_ptr;

#define MXD_BIOCAT_MIRROR_POSITIONER_STANDARD_FIELDS \
  {-1, -1, "num_motors", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BIOCAT_MIRROR_POSITIONER, num_motors),\
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "motor_record_array", MXFT_RECORD, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof( MX_BIOCAT_MIRROR_POSITIONER, motor_record_array ), \
	{sizeof(MX_RECORD *)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_VARARGS) }, \
  \
  {-1, -1, "motor_scale_array", MXFT_DOUBLE, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof( MX_BIOCAT_MIRROR_POSITIONER, motor_scale_array ), \
	{sizeof(double)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_VARARGS) }


#endif /* __D_BIOCAT_MIRROR_POSITIONER_H__ */
