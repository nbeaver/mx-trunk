/*
 * Name:    d_soft_area_detector.h
 *
 * Purpose: MX driver header for a software emulated area detector.  It uses
 *          an MX video input record as the source of the frame images.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2006-2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SOFT_AREA_DETECTOR_H__
#define __D_SOFT_AREA_DETECTOR_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *video_input_record;
	unsigned long initial_trigger_mode;

	MX_CLOCK_TICK start_time;
	mx_bool_type sequence_in_progress;
} MX_SOFT_AREA_DETECTOR;


#define MXD_SOFT_AREA_DETECTOR_STANDARD_FIELDS \
  {-1, -1, "video_input_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_SOFT_AREA_DETECTOR, video_input_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "initial_trigger_mode", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_SOFT_AREA_DETECTOR, initial_trigger_mode), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}


MX_API mx_status_type mxd_soft_area_detector_initialize_type(long record_type);
MX_API mx_status_type mxd_soft_area_detector_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_soft_area_detector_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_soft_area_detector_open( MX_RECORD *record );
MX_API mx_status_type mxd_soft_area_detector_close( MX_RECORD *record );

MX_API mx_status_type mxd_soft_area_detector_arm( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_soft_area_detector_trigger( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_soft_area_detector_stop( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_soft_area_detector_abort( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_soft_area_detector_get_extended_status(
						MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_soft_area_detector_readout_frame(
						MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_soft_area_detector_get_parameter(
						MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_soft_area_detector_set_parameter(
						MX_AREA_DETECTOR *ad );

extern MX_RECORD_FUNCTION_LIST mxd_soft_area_detector_record_function_list;
extern MX_AREA_DETECTOR_FUNCTION_LIST mxd_soft_area_detector_ad_function_list;

extern long mxd_soft_area_detector_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_soft_area_detector_rfield_def_ptr;

#endif /* __D_SOFT_AREA_DETECTOR_H__ */

