/*
 * Name:    d_bluice_area_detector.h
 *
 * Purpose: MX driver header for Blu-Ice controlled area detectors.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2008, 2010-2011, 2018 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_BLUICE_AREA_DETECTOR_H__
#define __D_BLUICE_AREA_DETECTOR_H__

#include "mx_namefix.h"

/* Flag bits for the 'bluice_flags' field. */

#define MXF_BLUICE_AD_REUSE_DARK	0x1

typedef struct {
	MX_RECORD *record;

	MX_RECORD *bluice_server_record;
	char bluice_dhs_name[MXU_BLUICE_NAME_LENGTH+1];
	unsigned long bluice_flags;

	char datafile_directory[MXU_FILENAME_LENGTH+1];
	char datafile_name[MXU_FILENAME_LENGTH+1];

	MX_RECORD *detector_distance_record;
	char detector_distance_name[MXU_RECORD_NAME_LENGTH+1];

	MX_RECORD *wavelength_record;
	char wavelength_name[MXU_RECORD_NAME_LENGTH+1];

	MX_RECORD *detector_x_record;
	char detector_x_name[MXU_RECORD_NAME_LENGTH+1];

	MX_RECORD *detector_y_record;
	char detector_y_name[MXU_RECORD_NAME_LENGTH+1];

	MX_BLUICE_FOREIGN_DEVICE *collect_operation;
	int last_collect_operation_state;

	MX_BLUICE_FOREIGN_DEVICE *transfer_operation;
	MX_BLUICE_FOREIGN_DEVICE *oscillation_ready_operation;
	MX_BLUICE_FOREIGN_DEVICE *stop_operation;
	MX_BLUICE_FOREIGN_DEVICE *reset_run_operation;

	MX_BLUICE_FOREIGN_DEVICE *last_image_collected_string;

	MX_THREAD *collect_thread;
	char collect_command[5000];

	mx_bool_type initialize_datafile_number;

	char detector_type[MXU_BLUICE_NAME_LENGTH+1];

	char last_datafile_name[MXU_FILENAME_LENGTH+1];
} MX_BLUICE_AREA_DETECTOR;

MX_API mx_status_type mxd_bluice_area_detector_initialize_driver(
							MX_DRIVER *driver );
MX_API mx_status_type mxd_bluice_area_detector_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_bluice_area_detector_open( MX_RECORD *record );
MX_API mx_status_type mxd_bluice_area_detector_finish_delayed_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_bluice_area_detector_arm( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_bluice_area_detector_trigger( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_bluice_area_detector_stop( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_bluice_area_detector_get_extended_status(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_bluice_area_detector_readout_frame(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_bluice_area_detector_transfer_frame(
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
  {-1, -1, "bluice_dhs_name", MXFT_STRING, NULL, 1, {MXU_BLUICE_NAME_LENGTH}, \
  	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_BLUICE_AREA_DETECTOR, bluice_dhs_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "bluice_flags", MXFT_HEX, NULL, 0, {0}, \
  	MXF_REC_TYPE_STRUCT, offsetof(MX_BLUICE_AREA_DETECTOR, bluice_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#define MXD_BLUICE_DHS_AREA_DETECTOR_STANDARD_FIELDS \
  {-1, -1, "bluice_server_record", MXFT_RECORD, NULL, 0, {0}, \
  	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_BLUICE_AREA_DETECTOR, bluice_server_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "bluice_dhs_name", MXFT_STRING, NULL, 1, {MXU_BLUICE_NAME_LENGTH}, \
  	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_BLUICE_AREA_DETECTOR, bluice_dhs_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "bluice_flags", MXFT_HEX, NULL, 0, {0}, \
  	MXF_REC_TYPE_STRUCT, offsetof(MX_BLUICE_AREA_DETECTOR, bluice_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "detector_distance_name", MXFT_STRING, \
  				NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
  	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_BLUICE_AREA_DETECTOR, detector_distance_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "wavelength_name", MXFT_STRING, NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
  	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_BLUICE_AREA_DETECTOR, wavelength_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "detector_x_name", MXFT_STRING, NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
  	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_BLUICE_AREA_DETECTOR, detector_x_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "detector_y_name", MXFT_STRING, NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
  	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_BLUICE_AREA_DETECTOR, detector_y_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

#endif /* __D_BLUICE_AREA_DETECTOR_H__ */

