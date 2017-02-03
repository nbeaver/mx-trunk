/*
 * Name:    d_soft_sample_changer.h
 *
 * Purpose: Header for software emulated sample changers.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2004, 2017 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SOFT_SAMPLE_CHANGER_H__
#define __D_SOFT_SAMPLE_CHANGER_H__

typedef struct {
	MX_RECORD *record;
} MX_SOFT_SAMPLE_CHANGER;

#define MXD_SOFT_SAMPLE_CHANGER_STANDARD_FIELDS

MX_API mx_status_type mxd_soft_sample_changer_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_soft_sample_changer_open( MX_RECORD *record );


MX_API mx_status_type mxd_soft_sample_changer_initialize(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_soft_sample_changer_shutdown(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_soft_sample_changer_mount_sample(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_soft_sample_changer_unmount_sample(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_soft_sample_changer_grab_sample(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_soft_sample_changer_ungrab_sample(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_soft_sample_changer_select_sample_holder(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_soft_sample_changer_unselect_sample_holder(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_soft_sample_changer_soft_abort(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_soft_sample_changer_immediate_abort(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_soft_sample_changer_idle(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_soft_sample_changer_reset_changer(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_soft_sample_changer_get_status(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_soft_sample_changer_get_parameter(
						MX_SAMPLE_CHANGER *changer );
MX_API mx_status_type mxd_soft_sample_changer_set_parameter(
						MX_SAMPLE_CHANGER *changer );

extern MX_RECORD_FUNCTION_LIST mxd_soft_sample_changer_record_function_list;

extern MX_SAMPLE_CHANGER_FUNCTION_LIST
			mxd_soft_sample_changer_sample_changer_function_list;

extern long mxd_soft_sample_changer_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_soft_sample_changer_rfield_def_ptr;

#endif /* __D_SOFT_SAMPLE_CHANGER_H__ */
