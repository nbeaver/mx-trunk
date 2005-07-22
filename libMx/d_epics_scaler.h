/*
 * Name:    d_epics_scaler.h
 *
 * Purpose: Header file for MX scaler driver to control EPICS scalers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 1999, 2001-2003 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_EPICS_SCALER_H__
#define __D_EPICS_SCALER_H__

/* ===== EPICS scaler data structure ===== */

typedef struct {
	char epics_record_name[ MXU_EPICS_PVNAME_LENGTH+1 ];
	int scaler_number;
	double epics_record_version;

	MX_EPICS_PV cnt_pv;
	MX_EPICS_PV dark_pv;
	MX_EPICS_PV mode_pv;
	MX_EPICS_PV pr_pv;
	MX_EPICS_PV s_pv;
	MX_EPICS_PV sd_pv;
	MX_EPICS_PV vers_pv;

	long num_epics_counters;
	MX_EPICS_PV *gate_control_pv_array;
} MX_EPICS_SCALER;

#define MXD_EPICS_SCALER_STANDARD_FIELDS \
  {-1, -1, "epics_record_name", MXFT_STRING, \
		NULL, 1, {MXU_EPICS_PVNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPICS_SCALER, epics_record_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "scaler_number", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPICS_SCALER, scaler_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "epics_record_version", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPICS_SCALER, epics_record_version), \
	{0}, NULL, 0}

/* Define all of the interface functions. */

MX_API mx_status_type mxd_epics_scaler_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_epics_scaler_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_epics_scaler_print_structure( FILE *file,
							MX_RECORD *record );
MX_API mx_status_type mxd_epics_scaler_open( MX_RECORD *record );

MX_API mx_status_type mxd_epics_scaler_read( MX_SCALER *scaler );
MX_API mx_status_type mxd_epics_scaler_read_raw( MX_SCALER *scaler );
MX_API mx_status_type mxd_epics_scaler_is_busy( MX_SCALER *scaler );
MX_API mx_status_type mxd_epics_scaler_start( MX_SCALER *scaler );
MX_API mx_status_type mxd_epics_scaler_stop( MX_SCALER *scaler );
MX_API mx_status_type mxd_epics_scaler_get_parameter( MX_SCALER *scaler );
MX_API mx_status_type mxd_epics_scaler_set_parameter( MX_SCALER *scaler );
MX_API mx_status_type mxd_epics_scaler_set_modes_of_associated_counters(
							MX_SCALER *scaler );

extern MX_RECORD_FUNCTION_LIST mxd_epics_scaler_record_function_list;
extern MX_SCALER_FUNCTION_LIST mxd_epics_scaler_scaler_function_list;

extern long mxd_epics_scaler_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_epics_scaler_rfield_def_ptr;

#endif /* __D_EPICS_SCALER_H__ */

