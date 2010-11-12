/*
 * Name:    d_epics_ad.h
 *
 * Purpose: MX driver header for the EPICS Area Detector record.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_EPICS_AREA_DETECTOR_H__
#define __D_EPICS_AREA_DETECTOR_H__

typedef struct {
	MX_RECORD *record;

	char epics_prefix[ MXU_EPICS_PVNAME_LENGTH+1 ];

	MX_EPICS_PV abort_pv;
	MX_EPICS_PV acquire_time_pv;
	MX_EPICS_PV acquire_pv;
	MX_EPICS_PV binx_pv;
	MX_EPICS_PV biny_pv;
	MX_EPICS_PV detector_state_pv;

	mx_bool_type acquisition_in_progress;
} MX_EPICS_AREA_DETECTOR;

#define MXD_EPICS_AREA_DETECTOR_STANDARD_FIELDS \
  {-1, -1, "epics_prefix", MXFT_STRING, NULL, 1, {MXU_EPICS_PVNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPICS_AREA_DETECTOR, epics_prefix), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxd_epics_ad_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxd_epics_ad_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_epics_ad_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_epics_ad_open( MX_RECORD *record );

MX_API mx_status_type mxd_epics_ad_arm( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_epics_ad_trigger( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_epics_ad_abort( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_epics_ad_get_extended_status( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_epics_ad_readout_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_epics_ad_correct_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_epics_ad_get_parameter( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_epics_ad_set_parameter( MX_AREA_DETECTOR *ad );

extern MX_RECORD_FUNCTION_LIST mxd_epics_ad_record_function_list;
extern MX_AREA_DETECTOR_FUNCTION_LIST mxd_epics_ad_ad_function_list;

extern long mxd_epics_ad_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_epics_ad_rfield_def_ptr;

#endif /* __D_EPICS_AREA_DETECTOR_H__ */

