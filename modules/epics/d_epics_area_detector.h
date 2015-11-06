/*
 * Name:    d_epics_area_detector.h
 *
 * Purpose: MX driver header for the EPICS Area Detector record.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2010, 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_EPICS_AREA_DETECTOR_H__
#define __D_EPICS_AREA_DETECTOR_H__

/* Bit definitions for the epics_area_detector_flags field.
 *
 * The 'raw image file' bits are used if MX can directly
 * read the raw file created by the detector without going
 * through the EPICS ArrayData PV to get the image data.
 *
 * MXF_EPICS_AD_CHANGE_RAW_IMAGE_FILE_PREFIX is used when
 * the raw file is available by either an NFS or SMB mounted
 * remote file system.  In that case, the filename seen by
 * the MX client may be different than the filename seen by
 * the EPICS IOC.
 */

#define MXF_EPICS_AD_RAW_IMAGE_FILE_AVAILABLE		0x1
#define MXF_EPICS_AD_READ_RAW_IMAGE_FILE		0x2
#define MXF_EPICS_AD_CHANGE_RAW_IMAGE_FILE_PREFIX	0x4

#define MXF_EPICS_AD_DO_NOT_ENABLE_ARRAY_CALLBACKS	0x1000

typedef struct {
	char array_port_name[ MXU_EPICS_STRING_LENGTH+1 ];

	MX_EPICS_PV nd_array_port_pv;
	MX_EPICS_PV nd_array_port_rbv_pv;
	MX_EPICS_PV min_x_pv;
	MX_EPICS_PV min_x_rbv_pv;
	MX_EPICS_PV min_y_pv;
	MX_EPICS_PV min_y_rbv_pv;
	MX_EPICS_PV size_x_pv;
	MX_EPICS_PV size_x_rbv_pv;
	MX_EPICS_PV size_y_pv;
	MX_EPICS_PV size_y_rbv_pv;
} MX_EPICS_AREA_DETECTOR_ROI;

typedef struct {
	MX_RECORD *record;

	char prefix_name[ MXU_EPICS_PVNAME_LENGTH+1 ];
	char camera_name[ MXU_EPICS_PVNAME_LENGTH+1 ];
	char image_name[ MXU_EPICS_PVNAME_LENGTH+1 ];

	unsigned long epics_area_detector_flags;

	/* The following names are read back from EPICS. */

	char manufacturer_name[ MXU_EPICS_STRING_LENGTH+1 ];
	char model_name[ MXU_EPICS_STRING_LENGTH+1 ];

	char asyn_port_name[ MXU_EPICS_STRING_LENGTH+1 ];

	/* Does the EPICS driver _really_ implement NumImagesCounter? */

	mx_bool_type num_images_counter_is_implemented;

	/* If not, then we use 'old_total_num_frames' to implement
	 * 'last_frame_number' without NumImagesCounter.
	 */

	unsigned long old_total_num_frames;

	/* The following are used if MX needs to directly process
	 * the detector image frame data, without going through
	 * EPICS for it.  This typically will be for an IOC-based
	 * detector that does not implement the ArrayData PV.
	 *
	 * Note: not yet used.
	 */

	unsigned long raw_data_type;
	unsigned long raw_frame_bytes;
	void *raw_frame_array;

	MX_EPICS_AREA_DETECTOR_ROI *epics_roi_array;

	MX_EPICS_PV acquire_pv;
	MX_EPICS_PV acquire_rbv_pv;
	MX_EPICS_PV acquire_period_pv;
	MX_EPICS_PV acquire_period_rbv_pv;
	MX_EPICS_PV acquire_time_pv;
	MX_EPICS_PV acquire_time_rbv_pv;
	MX_EPICS_PV array_callbacks_pv;
	MX_EPICS_PV array_counter_rbv_pv;
	MX_EPICS_PV array_data_pv;
	MX_EPICS_PV binx_pv;
	MX_EPICS_PV binx_rbv_pv;
	MX_EPICS_PV biny_pv;
	MX_EPICS_PV biny_rbv_pv;
	MX_EPICS_PV detector_state_pv;
	MX_EPICS_PV file_format_pv;
	MX_EPICS_PV file_format_rbv_pv;
	MX_EPICS_PV file_name_pv;
	MX_EPICS_PV file_name_rbv_pv;
	MX_EPICS_PV file_number_pv;
	MX_EPICS_PV file_number_rbv_pv;
	MX_EPICS_PV file_path_pv;
	MX_EPICS_PV file_path_rbv_pv;
	MX_EPICS_PV file_path_exists_rbv_pv;
	MX_EPICS_PV file_template_pv;
	MX_EPICS_PV file_template_rbv_pv;
	MX_EPICS_PV full_file_name_rbv_pv;
	MX_EPICS_PV image_mode_pv;
	MX_EPICS_PV image_mode_rbv_pv;
	MX_EPICS_PV num_acquisitions_pv;
	MX_EPICS_PV num_acquisitions_rbv_pv;
	MX_EPICS_PV num_acquisitions_counter_rbv_pv;
	MX_EPICS_PV num_images_pv;
	MX_EPICS_PV num_images_rbv_pv;
	MX_EPICS_PV num_images_counter_rbv_pv;
	MX_EPICS_PV trigger_mode_pv;
	MX_EPICS_PV trigger_mode_rbv_pv;

	mx_bool_type acquisition_is_starting;

	unsigned long max_array_bytes;
	mx_bool_type use_num_acquisitions;
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
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "epics_area_detector_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_EPICS_AREA_DETECTOR, epics_area_detector_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "manufacturer_name", MXFT_STRING, NULL, \
			1, {MXU_EPICS_STRING_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_EPICS_AREA_DETECTOR, manufacturer_name), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "model_name", MXFT_STRING, NULL, 1, {MXU_EPICS_STRING_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPICS_AREA_DETECTOR, model_name), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "asyn_port_name", MXFT_STRING, NULL, 1, {MXU_EPICS_STRING_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPICS_AREA_DETECTOR, asyn_port_name), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "num_images_counter_is_implemented", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
    offsetof(MX_EPICS_AREA_DETECTOR, num_images_counter_is_implemented),\
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "old_total_num_frames", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_EPICS_AREA_DETECTOR, old_total_num_frames), \
	{0}, NULL, MXFF_READ_ONLY}

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

