/*
 * Name:    v_rdi_mbc_save_frame.h
 *
 * Purpose: MX variable header for a driver for the MBC beamline that
 *          overrides the default save_averaged_correction_frame function
 *          for the specified area detector.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __V_RDI_MBC_SAVE_FRAME_H__
#define __V_RDI_MBC_SAVE_FRAME_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *area_detector_record;
	char dark_current_directory_name[ MXU_FILENAME_LENGTH+1 ];

} MX_RDI_MBC_SAVE_FRAME;


#define MXV_RDI_MBC_SAVE_FRAME_STANDARD_FIELDS \
  {-1, -1, "area_detector_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_RDI_MBC_SAVE_FRAME, area_detector_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "dark_current_directory_name", MXFT_STRING, \
					NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_RDI_MBC_SAVE_FRAME, dark_current_directory_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

MX_API_PRIVATE mx_status_type mxv_rdi_mbc_save_frame_create_record_structures(
							MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxv_rdi_mbc_save_frame_open( MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxv_rdi_mbc_save_frame_send_variable(
							MX_VARIABLE *variable );
MX_API_PRIVATE mx_status_type mxv_rdi_mbc_save_frame_receive_variable(
							MX_VARIABLE *variable );

extern MX_RECORD_FUNCTION_LIST mxv_rdi_mbc_save_frame_record_function_list;
extern MX_VARIABLE_FUNCTION_LIST mxv_rdi_mbc_save_frame_variable_function_list;

extern long mxv_rdi_mbc_save_frame_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxv_rdi_mbc_save_frame_rfield_def_ptr;

#endif /* __V_RDI_MBC_SAVE_FRAME_H__ */

