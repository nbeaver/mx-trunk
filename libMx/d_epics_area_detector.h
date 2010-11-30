/*
 * Name:    d_epics_area_detector.h
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

	char prefix_name[ MXU_EPICS_PVNAME_LENGTH+1 ];
	char camera_name[ MXU_EPICS_PVNAME_LENGTH+1 ];
	char image_name[ MXU_EPICS_PVNAME_LENGTH+1 ];

	/* The following name is read back from EPICS. */

	char asyn_port_name[ MXU_EPICS_PVNAME_LENGTH+1 ];

	MX_EPICS_PV acquire_pv;
	MX_EPICS_PV acquire_rbv_pv;
	MX_EPICS_PV acquire_period_pv;
	MX_EPICS_PV acquire_period_rbv_pv;
	MX_EPICS_PV acquire_time_pv;
	MX_EPICS_PV acquire_time_rbv_pv;
	MX_EPICS_PV array_counter_rbv_pv;
	MX_EPICS_PV array_data_pv;
	MX_EPICS_PV binx_pv;
	MX_EPICS_PV binx_rbv_pv;
	MX_EPICS_PV biny_pv;
	MX_EPICS_PV biny_rbv_pv;
	MX_EPICS_PV detector_state_pv;
	MX_EPICS_PV image_mode_pv;
	MX_EPICS_PV image_mode_rbv_pv;
	MX_EPICS_PV num_images_pv;
	MX_EPICS_PV num_images_rbv_pv;
	MX_EPICS_PV trigger_mode_pv;
	MX_EPICS_PV trigger_mode_rbv_pv;

	MX_EPICS_PV mx_next_frame_number_pv;

	mx_bool_type acquisition_is_starting;
	mx_bool_type array_data_available;

	unsigned long max_array_bytes;
} MX_EPICS_AREA_DETECTOR;

#define MXD_EPICS_AREA_DETECTOR_STANDARD_FIELDS \
  {-1, -1, "prefix_name", MXFT_STRING, NULL, 1, {MXU_EPICS_PVNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPICS_AREA_DETECTOR, prefix_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "camera_name", MXFT_STRING, NULL, 1, {MXU_EPICS_PVNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPICS_AREA_DETECTOR, camera_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "image_name", MXFT_STRING, NULL, 1, {MXU_EPICS_PVNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPICS_AREA_DETECTOR, image_name), \
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
MX_API mx_status_type mxd_epics_ad_get_last_frame_number( MX_AREA_DETECTOR *ad);
MX_API mx_status_type mxd_epics_ad_get_total_num_frames( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_epics_ad_get_status( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_epics_ad_readout_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_epics_ad_correct_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_epics_ad_get_parameter( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_epics_ad_set_parameter( MX_AREA_DETECTOR *ad );

extern MX_RECORD_FUNCTION_LIST mxd_epics_ad_record_function_list;
extern MX_AREA_DETECTOR_FUNCTION_LIST mxd_epics_ad_ad_function_list;

extern long mxd_epics_ad_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_epics_ad_rfield_def_ptr;

#endif /* __D_EPICS_AREA_DETECTOR_H__ */

