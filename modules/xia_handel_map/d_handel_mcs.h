/*
 * Name:    d_handel_mcs.h 
 *
 * Purpose: Header file for XIA Handel mapping support.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2019 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_HANDEL_MCS_H__
#define __D_HANDEL_MCS_H__

/* Handel MCS flag values. */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *mca_record;
	unsigned long handel_mcs_flags;
} MX_HANDEL_MCS;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_handel_mcs_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxd_handel_mcs_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_handel_mcs_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_handel_mcs_open( MX_RECORD *record );

MX_API mx_status_type mxd_handel_mcs_arm( MX_MCS *mcs );
MX_API mx_status_type mxd_handel_mcs_stop( MX_MCS *mcs );
MX_API mx_status_type mxd_handel_mcs_clear( MX_MCS *mcs );
MX_API mx_status_type mxd_handel_mcs_busy( MX_MCS *mcs );
MX_API mx_status_type mxd_handel_mcs_read_all( MX_MCS *mcs );
MX_API mx_status_type mxd_handel_mcs_read_scaler( MX_MCS *mcs );
MX_API mx_status_type mxd_handel_mcs_read_measurement( MX_MCS *mcs );
MX_API mx_status_type mxd_handel_mcs_get_parameter( MX_MCS *mcs );
MX_API mx_status_type mxd_handel_mcs_set_parameter( MX_MCS *mcs );

extern MX_RECORD_FUNCTION_LIST mxd_handel_mcs_record_function_list;
extern MX_MCS_FUNCTION_LIST mxd_handel_mcs_mcs_function_list;

extern long mxd_handel_mcs_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_handel_mcs_rfield_def_ptr;

#define MXD_HANDEL_MCS_STANDARD_FIELDS \
  {-1, -1, "mca_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_HANDEL_MCS, mca_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "handel_mcs_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_HANDEL_MCS, handel_mcs_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION }

#endif /* __D_HANDEL_MCS_H__ */

