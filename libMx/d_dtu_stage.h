/*
 * Name:    d_dtu_stage.h
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

#ifndef __D_DTU_STAGE_H__
#define __D_DTU_STAGE_H__

#define MXU_DTU_PSEUDOMOTOR_NAME_LENGTH		20

#define MXT_DTU_STAGE_ALPHA			1
#define MXT_DTU_STAGE_PHI			2

typedef struct {
	MX_RECORD *record;
	char pseudomotor_name[ MXU_DTU_PSEUDOMOTOR_NAME_LENGTH+1 ];
	unsigned long pseudomotor_type;

	MX_RECORD *delta_l_record;

	MX_RECORD *alpha_motor_record;
	MX_RECORD *x_motor_record;
	MX_RECORD *y_motor_record;
	MX_RECORD *phi_motor_record;
} MX_DTU_STAGE;

MX_API mx_status_type mxd_dtu_stage_create_record_structures(MX_RECORD *record);
MX_API mx_status_type mxd_dtu_stage_open( MX_RECORD *record );

MX_API mx_status_type mxd_dtu_stage_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_dtu_stage_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_dtu_stage_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_dtu_stage_get_status( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_dtu_stage_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_dtu_stage_motor_function_list;

extern long mxd_dtu_stage_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_dtu_stage_rfield_def_ptr;

#define MXD_DTU_STAGE_STANDARD_FIELDS \
  {-1, -1, "pseudomotor_name", MXFT_STRING, NULL, \
	  				1, {MXU_DTU_PSEUDOMOTOR_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_DTU_STAGE, pseudomotor_name ), \
	{sizeof(char)}, NULL, \
		(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_READ_ONLY) }, \
  \
  {-1, -1, "pseudomotor_type", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_DTU_STAGE, pseudomotor_type ), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "delta_l_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_DTU_STAGE, delta_l_record ), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "alpha_motor_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_DTU_STAGE, alpha_motor_record ), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "x_motor_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_DTU_STAGE, x_motor_record ), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "y_motor_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_DTU_STAGE, y_motor_record ), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "phi_motor_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_DTU_STAGE, phi_motor_record ), \
	{0}, NULL, MXFF_IN_DESCRIPTION }

#endif /* __D_DTU_STAGE_H__ */
