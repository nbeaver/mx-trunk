/* 
 * Name:    d_epics_mca.h
 *
 * Purpose: Header file for EPICS-controlled multichannel analyzers.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2002-2003 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_EPICS_MCA_H__
#define __D_EPICS_MCA_H__

#define MXD_NMCA_TICKS_PER_SECOND	(50.0)

typedef struct {
	char epics_record_name[ MXU_EPICS_PVNAME_LENGTH+1 ];

	MX_EPICS_PV acqg_pv;
	MX_EPICS_PV act_pv;
	MX_EPICS_PV eltm_pv;
	MX_EPICS_PV eras_pv;
	MX_EPICS_PV ertm_pv;
	MX_EPICS_PV nuse_pv;
	MX_EPICS_PV pct_pv;
	MX_EPICS_PV pcth_pv;
	MX_EPICS_PV pctl_pv;
	MX_EPICS_PV pltm_pv;
	MX_EPICS_PV proc_pv;
	MX_EPICS_PV prtm_pv;
	MX_EPICS_PV stop_pv;
	MX_EPICS_PV strt_pv;
	MX_EPICS_PV val_pv;

	long num_roi_pvs;
	MX_EPICS_PV *roi_low_pv_array;
	MX_EPICS_PV *roi_high_pv_array;
	MX_EPICS_PV *roi_integral_pv_array;
	MX_EPICS_PV *roi_background_pv_array;
} MX_EPICS_MCA;

#define MXD_EPICS_MCA_STANDARD_FIELDS \
  {-1, -1, "epics_record_name", MXFT_STRING, \
		NULL, 1, {MXU_EPICS_PVNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPICS_MCA, epics_record_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

MX_API mx_status_type mxd_epics_mca_initialize_type( long type );
MX_API mx_status_type mxd_epics_mca_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_epics_mca_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_epics_mca_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_epics_mca_print_structure( FILE *file,
							MX_RECORD *record );
MX_API mx_status_type mxd_epics_mca_open( MX_RECORD *record );

MX_API mx_status_type mxd_epics_mca_start( MX_MCA *mca );
MX_API mx_status_type mxd_epics_mca_stop( MX_MCA *mca );
MX_API mx_status_type mxd_epics_mca_read( MX_MCA *mca );
MX_API mx_status_type mxd_epics_mca_clear( MX_MCA *mca );
MX_API mx_status_type mxd_epics_mca_busy( MX_MCA *mca );
MX_API mx_status_type mxd_epics_mca_get_parameter( MX_MCA *mca );
MX_API mx_status_type mxd_epics_mca_set_parameter( MX_MCA *mca );

extern MX_RECORD_FUNCTION_LIST mxd_epics_mca_record_function_list;
extern MX_MCA_FUNCTION_LIST mxd_epics_mca_mca_function_list;

extern long mxd_epics_mca_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_epics_mca_rfield_def_ptr;

#endif /* __D_EPICS_MCA_H__ */

