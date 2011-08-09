/*
 * Name:    d_epics_ccd.h
 *
 * Purpose: MX driver header for the EPICS CCD record.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2009-2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_EPICS_CCD_H__
#define __D_EPICS_CCD_H__

typedef struct {
	MX_RECORD *record;

	char epics_record_name[ MXU_EPICS_PVNAME_LENGTH+1 ];

	MX_EPICS_PV abort_pv;
	MX_EPICS_PV acquire_poll_pv;
	MX_EPICS_PV binx_pv;
	MX_EPICS_PV biny_pv;
	MX_EPICS_PV seconds_pv;

	mx_bool_type acquisition_in_progress;
} MX_EPICS_CCD;

#define MXD_EPICS_CCD_STANDARD_FIELDS \
  {-1, -1, "epics_record_name", MXFT_STRING, \
				NULL, 1, {MXU_EPICS_PVNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPICS_CCD, epics_record_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxd_epics_ccd_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxd_epics_ccd_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_epics_ccd_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_epics_ccd_open( MX_RECORD *record );

MX_API mx_status_type mxd_epics_ccd_arm( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_epics_ccd_trigger( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_epics_ccd_abort( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_epics_ccd_get_extended_status(
						MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_epics_ccd_readout_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_epics_ccd_correct_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_epics_ccd_get_parameter( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_epics_ccd_set_parameter( MX_AREA_DETECTOR *ad );

extern MX_RECORD_FUNCTION_LIST mxd_epics_ccd_record_function_list;
extern MX_AREA_DETECTOR_FUNCTION_LIST mxd_epics_ccd_ad_function_list;

extern long mxd_epics_ccd_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_epics_ccd_rfield_def_ptr;

#endif /* __D_EPICS_CCD_H__ */

