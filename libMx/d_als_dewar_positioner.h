/*
 * Name:    d_als_dewar_positioner.h 
 *
 * Purpose: Header file for MX motor driver to control the rotation and
 *          X-translation stages of an ALS dewar positioner used as part
 *          of the ALS sample changing robot design.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_ALS_DEWAR_POSITIONER_H__
#define __D_ALS_DEWAR_POSITIONER_H__

#include "mx_motor.h"

#define MXF_ALS_DEWAR_POSITIONER_IGNORE_SLIDE_STATUS	0x1

typedef struct {
	MX_RECORD *record;

	MX_RECORD *dewar_rot_record;
	MX_RECORD *dewar_x_record;

	MX_RECORD *vertical_slide_record;
	MX_RECORD *small_slide_record;

	unsigned long als_dewar_positioner_flags;

	double dewar_x_offset;			/* in mm */
	double dewar_rot_offset;		/* in degrees */
	double puck_center_radius;		/* in mm */
	double outer_sample_radius;		/* in mm */
	double inner_sample_radius;		/* in mm */
	double puck_orientation_offset;		/* in degrees */

	double dewar_rot_normal_speed;
	double dewar_x_normal_speed;

	double dewar_rot_home_speed;
	double dewar_x_home_speed;
} MX_ALS_DEWAR_POSITIONER;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_als_dewar_positioner_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_als_dewar_positioner_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_als_dewar_positioner_open( MX_RECORD *record );

MX_API mx_status_type mxd_als_dewar_positioner_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_als_dewar_positioner_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_als_dewar_positioner_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_als_dewar_positioner_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_als_dewar_positioner_immediate_abort(
							MX_MOTOR *motor );
MX_API mx_status_type mxd_als_dewar_positioner_find_home_position(
							MX_MOTOR *motor );
MX_API mx_status_type mxd_als_dewar_positioner_get_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_als_dewar_positioner_set_parameter( MX_MOTOR *motor );
MX_API mx_status_type mxd_als_dewar_positioner_get_status( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_als_dewar_positioner_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_als_dewar_positioner_motor_function_list;

extern long mxd_als_dewar_positioner_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_als_dewar_positioner_rfield_def_ptr;

#define MXD_ALS_DEWAR_POSITIONER_STANDARD_FIELDS \
  {-1, -1, "dewar_rot_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_ALS_DEWAR_POSITIONER, dewar_rot_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "dewar_x_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_ALS_DEWAR_POSITIONER, dewar_x_record),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "vertical_slide_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_ALS_DEWAR_POSITIONER, vertical_slide_record),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "small_slide_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_ALS_DEWAR_POSITIONER, small_slide_record),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "als_dewar_positioner_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_ALS_DEWAR_POSITIONER, als_dewar_positioner_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "dewar_x_offset", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ALS_DEWAR_POSITIONER, dewar_x_offset),\
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "dewar_rot_offset", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_ALS_DEWAR_POSITIONER, dewar_rot_offset), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "puck_center_radius", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_ALS_DEWAR_POSITIONER, puck_center_radius), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "outer_sample_radius", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_ALS_DEWAR_POSITIONER, outer_sample_radius),\
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "inner_sample_radius", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_ALS_DEWAR_POSITIONER, inner_sample_radius),\
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "puck_orientation_offset", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_ALS_DEWAR_POSITIONER, puck_orientation_offset),\
	{0}, NULL, MXFF_IN_DESCRIPTION }

#endif /* __D_ALS_DEWAR_POSITIONER_H__ */
