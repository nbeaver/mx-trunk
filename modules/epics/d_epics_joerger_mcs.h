/*
 * Name:    d_epics_joerger_mcs.h 
 *
 * Purpose: Header file for Joerger-based EPICS multichannel scaler support.
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

#ifndef __D_EPICS_JOERGER_MCS_H__
#define __D_EPICS_JOERGER_MCS_H__

typedef struct {
	char epics_record_name[ MXU_EPICS_PVNAME_LENGTH+1 ];

	MX_EPICS_PV cnt_pv;
	MX_EPICS_PV freq_pv;
	MX_EPICS_PV pr1_pv;
	MX_EPICS_PV rate_pv;

	unsigned long num_channels;

	MX_EPICS_PV *s_pv_array;
} MX_EPICS_JOERGER_MCS;

MX_API mx_status_type mxd_epics_joerger_mcs_initialize_driver(
							MX_DRIVER *driver );
MX_API mx_status_type mxd_epics_joerger_mcs_create_record_structures(
                                                        MX_RECORD *record );
MX_API mx_status_type mxd_epics_joerger_mcs_finish_record_initialization(
                                                        MX_RECORD *record );
MX_API mx_status_type mxd_epics_joerger_mcs_open( MX_RECORD *record );

MX_API mx_status_type mxd_epics_joerger_mcs_start( MX_MCS *mcs );
MX_API mx_status_type mxd_epics_joerger_mcs_stop( MX_MCS *mcs );
MX_API mx_status_type mxd_epics_joerger_mcs_clear( MX_MCS *mcs );
MX_API mx_status_type mxd_epics_joerger_mcs_busy( MX_MCS *mcs );
MX_API mx_status_type mxd_epics_joerger_mcs_read_all( MX_MCS *mcs );
MX_API mx_status_type mxd_epics_joerger_mcs_read_scaler( MX_MCS *mcs );
MX_API mx_status_type mxd_epics_joerger_mcs_read_measurement( MX_MCS *mcs );
MX_API mx_status_type mxd_epics_joerger_mcs_get_parameter( MX_MCS *mcs );
MX_API mx_status_type mxd_epics_joerger_mcs_set_parameter( MX_MCS *mcs );

extern MX_RECORD_FUNCTION_LIST mxd_epics_joerger_mcs_record_function_list;
extern MX_MCS_FUNCTION_LIST mxd_epics_joerger_mcs_mcs_function_list;

extern long mxd_epics_joerger_mcs_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_epics_joerger_mcs_rfield_def_ptr;

#endif /* __D_EPICS_JOERGER_MCS_H__ */

