/*
 * Name:    d_sercat_als_robot.h
 *
 * Purpose: Header for SER-CAT's version of the ALS sample changing robot.
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

#ifndef __D_SERCAT_ALS_ROBOT_H__
#define __D_SERCAT_ALS_ROBOT_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *dewar_positioner_record;
	MX_RECORD *horizontal_slide_record;
	MX_RECORD *vertical_slide_record;
	MX_RECORD *sample_rotation_record;
	MX_RECORD *small_slide_record;
	MX_RECORD *gripper_record;

	unsigned long cooldown_delay_ms;	/* in milliseconds */
	unsigned long relay_delay_ms;		/* in milliseconds */
} MX_SERCAT_ALS_ROBOT;

#define MXD_SERCAT_ALS_ROBOT_STANDARD_FIELDS \
  {-1, -1, "dewar_positioner_record", MXFT_RECORD, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, \
                offsetof(MX_SERCAT_ALS_ROBOT, dewar_positioner_record), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "horizontal_slide_record", MXFT_RECORD, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, \
                offsetof(MX_SERCAT_ALS_ROBOT, horizontal_slide_record), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "vertical_slide_record", MXFT_RECORD, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, \
                offsetof(MX_SERCAT_ALS_ROBOT, vertical_slide_record), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "sample_rotation_record", MXFT_RECORD, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, \
                offsetof(MX_SERCAT_ALS_ROBOT, sample_rotation_record), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "small_slide_record", MXFT_RECORD, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, \
                offsetof(MX_SERCAT_ALS_ROBOT, small_slide_record), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "gripper_record", MXFT_RECORD, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_SERCAT_ALS_ROBOT, gripper_record), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "cooldown_delay_ms", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SERCAT_ALS_ROBOT, cooldown_delay_ms), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "relay_delay_ms", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SERCAT_ALS_ROBOT, relay_delay_ms), \
        {0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

MX_API mx_status_type mxd_sercat_als_robot_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_sercat_als_robot_open( MX_RECORD *record );


MX_API mx_status_type mxd_sercat_als_robot_initialize(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_sercat_als_robot_shutdown(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_sercat_als_robot_mount_sample(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_sercat_als_robot_unmount_sample(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_sercat_als_robot_grab_sample(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_sercat_als_robot_ungrab_sample(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_sercat_als_robot_select_sample_holder(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_sercat_als_robot_unselect_sample_holder(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_sercat_als_robot_soft_abort(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_sercat_als_robot_immediate_abort(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_sercat_als_robot_idle(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_sercat_als_robot_reset(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_sercat_als_robot_get_status(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_sercat_als_robot_get_parameter(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_sercat_als_robot_set_parameter(
						MX_SAMPLE_CHANGER *changer );

extern MX_RECORD_FUNCTION_LIST mxd_sercat_als_robot_record_function_list;

extern MX_SAMPLE_CHANGER_FUNCTION_LIST
			mxd_sercat_als_robot_sample_changer_function_list;

extern long mxd_sercat_als_robot_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_sercat_als_robot_rfield_def_ptr;

#endif /* __D_SERCAT_ALS_ROBOT_H__ */
