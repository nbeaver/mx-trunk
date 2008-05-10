/*
 * Name:    d_bluice_area_detector.h
 *
 * Purpose: MX driver header for Blu-Ice controlled area detectors.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_BLUICE_AREA_DETECTOR_H__
#define __D_BLUICE_AREA_DETECTOR_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *bluice_server_record;
	char bluice_name[MXU_BLUICE_NAME_LENGTH+1];

} MX_BLUICE_AREA_DETECTOR;

MX_API mx_status_type mxd_bluice_area_detector_initialize_type(
							long record_type );
MX_API mx_status_type mxd_bluice_area_detector_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_bluice_area_detector_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_bluice_area_detector_open( MX_RECORD *record );

MX_API mx_status_type mxd_bluice_area_detector_trigger( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_bluice_area_detector_stop( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_bluice_area_detector_abort( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_bluice_area_detector_get_extended_status(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_bluice_area_detector_readout_frame(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_bluice_area_detector_correct_frame(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_bluice_area_detector_get_parameter(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_bluice_area_detector_set_parameter(
							MX_AREA_DETECTOR *ad );

extern MX_RECORD_FUNCTION_LIST mxd_bluice_area_detector_record_function_list;
extern MX_AREA_DETECTOR_FUNCTION_LIST mxd_bluice_area_detector_ad_function_list;

extern long mxd_bluice_dcss_area_detector_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_bluice_dcss_area_detector_rfield_def_ptr;

extern long mxd_bluice_dhs_area_detector_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_bluice_dhs_area_detector_rfield_def_ptr;

#define MXD_BLUICE_DCSS_AREA_DETECTOR_STANDARD_FIELDS \
  {-1, -1, "bluice_server_record", MXFT_RECORD, NULL, 0, {0}, \
  	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_BLUICE_AREA_DETECTOR, bluice_server_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "bluice_name", MXFT_STRING, NULL, 1, {MXU_BLUICE_NAME_LENGTH}, \
  	MXF_REC_TYPE_STRUCT, offsetof(MX_BLUICE_AREA_DETECTOR, bluice_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#define MXD_BLUICE_DHS_AREA_DETECTOR_STANDARD_FIELDS \
  {-1, -1, "bluice_server_record", MXFT_RECORD, NULL, 0, {0}, \
  	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_BLUICE_AREA_DETECTOR, bluice_server_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "bluice_name", MXFT_STRING, NULL, 1, {MXU_BLUICE_NAME_LENGTH}, \
  	MXF_REC_TYPE_STRUCT, offsetof(MX_BLUICE_AREA_DETECTOR, bluice_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_BLUICE_AREA_DETECTOR_H__ */

