/*
 * Name:    d_dtu_tilt.h
 *
 * Purpose: Header file for a tilt angle pseudomotor used at BioCAT by
 *          a general user group from dtu.dk.
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2022 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_DTU_TILT_H__
#define __D_DTU_TILT_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *x_motor_record;
	MX_RECORD *z_motor_record;
	MX_RECORD *dl_offset_motor_record;
	MX_RECORD *phi_motor_record;
} MX_DTU_TILT;

MX_API mx_status_type mxd_dtu_tilt_create_record_structures( MX_RECORD *record);

MX_API mx_status_type mxd_dtu_tilt_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_dtu_tilt_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_dtu_tilt_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_dtu_tilt_get_status( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_dtu_tilt_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_dtu_tilt_motor_function_list;

extern long mxd_dtu_tilt_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_dtu_tilt_rfield_def_ptr;

#define MXD_DTU_TILT_STANDARD_FIELDS \
  {-1, -1, "x_motor_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_DTU_TILT, x_motor_record ), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "z_motor_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_DTU_TILT, z_motor_record ), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "dl_offset_motor_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_DTU_TILT, dl_offset_motor_record ), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "phi_motor_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_DTU_TILT, phi_motor_record ), \
	{0}, NULL, MXFF_IN_DESCRIPTION }

#endif /* __D_DTU_TILT_H__ */
