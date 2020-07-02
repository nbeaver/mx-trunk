/*
 * Name:    d_dante_mcs.h 
 *
 * Purpose: Header file for XGLab DANTE mapping support.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2020 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_DANTE_MCS_H__
#define __D_DANTE_MCS_H__

#ifdef __cplusplus
extern "C" {
#endif

/* Handel MCS flag values. */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *mca_record;
	unsigned long dante_mcs_flags;
} MX_DANTE_MCS;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_dante_mcs_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxd_dante_mcs_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_dante_mcs_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_dante_mcs_open( MX_RECORD *record );

MX_API mx_status_type mxd_dante_mcs_arm( MX_MCS *mcs );
MX_API mx_status_type mxd_dante_mcs_trigger( MX_MCS *mcs );
MX_API mx_status_type mxd_dante_mcs_stop( MX_MCS *mcs );
MX_API mx_status_type mxd_dante_mcs_clear( MX_MCS *mcs );
MX_API mx_status_type mxd_dante_mcs_busy( MX_MCS *mcs );
MX_API mx_status_type mxd_dante_mcs_read_all( MX_MCS *mcs );
MX_API mx_status_type mxd_dante_mcs_get_parameter( MX_MCS *mcs );
MX_API mx_status_type mxd_dante_mcs_set_parameter( MX_MCS *mcs );
MX_API mx_status_type mxd_dante_mcs_get_last_measurement_number( MX_MCS *mcs );
MX_API mx_status_type mxd_dante_mcs_get_total_num_measurements( MX_MCS *mcs );

extern MX_RECORD_FUNCTION_LIST mxd_dante_mcs_record_function_list;
extern MX_MCS_FUNCTION_LIST mxd_dante_mcs_mcs_function_list;

extern long mxd_dante_mcs_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_dante_mcs_rfield_def_ptr;

#define MXD_DANTE_MCS_STANDARD_FIELDS \
  {-1, -1, "mca_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DANTE_MCS, mca_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "dante_mcs_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DANTE_MCS, dante_mcs_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION }

#ifdef __cplusplus
}
#endif

#endif /* __D_DANTE_MCS_H__ */

