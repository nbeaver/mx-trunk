/*
 * Name:    d_epics_mcs.h 
 *
 * Purpose: Header file for EPICS multichannel support.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999-2003, 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_EPICS_MCS_H__
#define __D_EPICS_MCS_H__

/* EPICS MCS flag values. */

#define MXF_EPICS_MCS_USE_REFERENCE_PULSER		0x2 /*not implemented*/
#define MXF_EPICS_MCS_DO_NOT_SKIP_FIRST_MEASUREMENT	0x4

typedef struct {
	char channel_prefix[ MXU_EPICS_PVNAME_LENGTH+1 ];
	char common_prefix[ MXU_EPICS_PVNAME_LENGTH+1 ];
	unsigned long epics_mcs_flags;

	double epics_record_version;
	long *scaler_value_buffer;

	MX_EPICS_PV acquiring_pv;
	MX_EPICS_PV dwell_pv;
	MX_EPICS_PV erase_pv;
	MX_EPICS_PV nmax_pv;
	MX_EPICS_PV nuse_pv;
	MX_EPICS_PV pltm_pv;
	MX_EPICS_PV read_pv;
	MX_EPICS_PV start_pv;
	MX_EPICS_PV stop_pv;
	MX_EPICS_PV vers_pv;

	long num_scaler_pvs;
	MX_EPICS_PV *dark_pv_array;
	MX_EPICS_PV *read_pv_array;
	MX_EPICS_PV *val_pv_array;
} MX_EPICS_MCS;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_epics_mcs_initialize_type( long type );
MX_API mx_status_type mxd_epics_mcs_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_epics_mcs_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_epics_mcs_open( MX_RECORD *record );

MX_API mx_status_type mxd_epics_mcs_start( MX_MCS *mcs );
MX_API mx_status_type mxd_epics_mcs_stop( MX_MCS *mcs );
MX_API mx_status_type mxd_epics_mcs_clear( MX_MCS *mcs );
MX_API mx_status_type mxd_epics_mcs_busy( MX_MCS *mcs );
MX_API mx_status_type mxd_epics_mcs_read_all( MX_MCS *mcs );
MX_API mx_status_type mxd_epics_mcs_read_scaler( MX_MCS *mcs );
MX_API mx_status_type mxd_epics_mcs_read_measurement( MX_MCS *mcs );
MX_API mx_status_type mxd_epics_mcs_get_parameter( MX_MCS *mcs );
MX_API mx_status_type mxd_epics_mcs_set_parameter( MX_MCS *mcs );

extern MX_RECORD_FUNCTION_LIST mxd_epics_mcs_record_function_list;
extern MX_MCS_FUNCTION_LIST mxd_epics_mcs_mcs_function_list;

extern long mxd_epics_mcs_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_epics_mcs_rfield_def_ptr;

#define MXD_EPICS_MCS_STANDARD_FIELDS \
  {-1, -1, "channel_prefix", MXFT_STRING, NULL, 1, {MXU_EPICS_PVNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPICS_MCS, channel_prefix), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "common_prefix", MXFT_STRING, NULL, 1, {MXU_EPICS_PVNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPICS_MCS, common_prefix), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "epics_mcs_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPICS_MCS, epics_mcs_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "epics_record_version", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPICS_MCS, epics_record_version), \
	{0}, NULL, 0}

#endif /* __D_EPICS_MCS_H__ */

