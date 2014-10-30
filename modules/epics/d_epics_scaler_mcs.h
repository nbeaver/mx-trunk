/*
 * Name:    d_epics_scaler_mcs.h 
 *
 * Purpose: MX multichannel scaler driver for counter/timers controlled
 *          using the EPICS Scaler record.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_EPICS_SCALER_MCS_H__
#define __D_EPICS_SCALER_MCS_H__

typedef struct {
	MX_RECORD *record;

	char epics_record_name[ MXU_EPICS_PVNAME_LENGTH+1 ];

	MX_EPICS_PV cnt_pv;
	MX_EPICS_PV rate_pv;
	MX_EPICS_PV tp_pv;

	unsigned long num_channels;

	MX_EPICS_PV *s_pv_array;

	MX_RECORD *motor_record;
	MX_EPICS_PV motor_position_pv;
	double *motor_position_array;

	double version;

	unsigned long current_measurement_number;

	MX_CALLBACK_MESSAGE *callback_message;
} MX_EPICS_SCALER_MCS;

MX_API mx_status_type mxd_epics_scaler_mcs_initialize_driver(
							MX_DRIVER *driver );
MX_API mx_status_type mxd_epics_scaler_mcs_create_record_structures(
                                                        MX_RECORD *record );
MX_API mx_status_type mxd_epics_scaler_mcs_finish_record_initialization(
                                                        MX_RECORD *record );
MX_API mx_status_type mxd_epics_scaler_mcs_open( MX_RECORD *record );

MX_API mx_status_type mxd_epics_scaler_mcs_start( MX_MCS *mcs );
MX_API mx_status_type mxd_epics_scaler_mcs_stop( MX_MCS *mcs );
MX_API mx_status_type mxd_epics_scaler_mcs_clear( MX_MCS *mcs );
MX_API mx_status_type mxd_epics_scaler_mcs_busy( MX_MCS *mcs );
MX_API mx_status_type mxd_epics_scaler_mcs_read_all( MX_MCS *mcs );
MX_API mx_status_type mxd_epics_scaler_mcs_read_scaler( MX_MCS *mcs );
MX_API mx_status_type mxd_epics_scaler_mcs_read_measurement( MX_MCS *mcs );
MX_API mx_status_type mxd_epics_scaler_mcs_get_parameter( MX_MCS *mcs );
MX_API mx_status_type mxd_epics_scaler_mcs_set_parameter( MX_MCS *mcs );

extern MX_RECORD_FUNCTION_LIST mxd_epics_scaler_mcs_record_function_list;
extern MX_MCS_FUNCTION_LIST mxd_epics_scaler_mcs_mcs_function_list;

extern long mxd_epics_scaler_mcs_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_epics_scaler_mcs_rfield_def_ptr;

#define MXD_EPICS_SCALER_MCS_STANDARD_FIELDS \
  {-1, -1, "epics_record_name", MXFT_STRING, NULL, 1,{MXU_EPICS_PVNAME_LENGTH},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPICS_SCALER_MCS, epics_record_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __D_EPICS_SCALER_MCS_H__ */

