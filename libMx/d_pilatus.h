/*
 * Name:    d_pilatus.h
 *
 * Purpose: Header file for Dectris Pilatus detectors.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_PILATUS_H__
#define __D_PILATUS_H__

typedef struct {
	MX_RECORD *record;

} MX_PILATUS;


#define MXD_PILATUS_STANDARD_FIELDS 

#if 0
  {-1, -1, "server_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_PILATUS, server_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "remote_record_name", MXFT_STRING, \
					NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_PILATUS, remote_record_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }
#endif

MX_API mx_status_type mxd_pilatus_initialize_driver(
							MX_DRIVER *driver );
MX_API mx_status_type mxd_pilatus_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_pilatus_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_pilatus_open( MX_RECORD *record );
MX_API mx_status_type mxd_pilatus_close( MX_RECORD *record );
MX_API mx_status_type mxd_pilatus_resynchronize(
							MX_RECORD *record );

MX_API mx_status_type mxd_pilatus_arm( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_trigger( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_stop( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_abort( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_get_last_frame_number(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_get_total_num_frames(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_get_status(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_get_extended_status(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_readout_frame(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_correct_frame(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_transfer_frame(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_load_frame(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_save_frame(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_copy_frame(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_get_roi_frame(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_get_parameter(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_set_parameter(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_measure_correction(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_setup_exposure(
							MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_pilatus_trigger_exposure(
							MX_AREA_DETECTOR *ad );

extern MX_RECORD_FUNCTION_LIST mxd_pilatus_record_function_list;
extern MX_AREA_DETECTOR_FUNCTION_LIST
			mxd_pilatus_ad_function_list;

extern long mxd_pilatus_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_pilatus_rfield_def_ptr;

#endif /* __D_PILATUS_H__ */

