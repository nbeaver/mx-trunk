/*
 * Name:    d_als_robot_java.h
 *
 * Purpose: Header for the ALS sample changing robot from Tom Earnest's
 *          ALS crystallography beamlines.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2003-2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_ALS_ROBOT_JAVA_H__
#define __D_ALS_ROBOT_JAVA_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;

	unsigned long interaction_id;
} MX_ALS_ROBOT_JAVA;

#define MXD_ALS_ROBOT_JAVA_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ALS_ROBOT_JAVA, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxd_als_robot_java_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_als_robot_java_open( MX_RECORD *record );


MX_API mx_status_type mxd_als_robot_java_initialize(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_als_robot_java_shutdown( MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_als_robot_java_mount_sample(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_als_robot_java_unmount_sample(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_als_robot_java_grab_sample(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_als_robot_java_ungrab_sample(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_als_robot_java_select_sample_holder(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_als_robot_java_unselect_sample_holder(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_als_robot_java_soft_abort(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_als_robot_java_immediate_abort(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_als_robot_java_idle( MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_als_robot_java_reset( MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_als_robot_java_get_status(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_als_robot_java_get_parameter(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_als_robot_java_set_parameter(
						MX_SAMPLE_CHANGER *changer );

MX_API mx_status_type mxd_als_robot_java_command(
				MX_SAMPLE_CHANGER *changer,
				MX_ALS_ROBOT_JAVA *mardtb,
				char *command,
				char *response, size_t response_buffer_length,
				int debug_flag );

extern MX_RECORD_FUNCTION_LIST mxd_als_robot_java_record_function_list;

extern MX_SAMPLE_CHANGER_FUNCTION_LIST
			mxd_als_robot_java_sample_changer_function_list;

extern long mxd_als_robot_java_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_als_robot_java_rfield_def_ptr;

#endif /* __D_ALS_ROBOT_JAVA_H__ */
