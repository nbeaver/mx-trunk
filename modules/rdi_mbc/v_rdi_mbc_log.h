/*
 * Name:    v_rdi_mbc_log.h
 *
 * Purpose: MX operation header for a driver for the MBC beamline that
 *          logs certain area detector operations to a log file.
 *          This includes starts, stops, and changes in the status of
 *          the area detector.
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

#ifndef __V_RDI_MBC_LOG_H__
#define __V_RDI_MBC_LOG_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *area_detector_record;
	char log_file_name[MXU_FILENAME_LENGTH+1];

	FILE *log_file;
} MX_RDI_MBC_LOG;


#define MXV_RDI_MBC_LOG_STANDARD_FIELDS \
  {-1, -1, "area_detector_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_RDI_MBC_LOG, area_detector_record), \
	{0}, NULL (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "log_file_name", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_RDI_MBC_LOG, log_file_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

MX_API_PRIVATE mx_status_type mxv_rdi_mbc_log_create_record_structures(
							MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxv_rdi_mbc_log_open( MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxv_rdi_mbc_log_close( MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxv_rdi_mbc_log_get_status(
						MX_OPERATION *operation );
MX_API_PRIVATE mx_status_type mxv_rdi_mbc_log_start(
						MX_OPERATION *operation );
MX_API_PRIVATE mx_status_type mxv_rdi_mbc_log_stop(
						MX_OPERATION *operation );

extern MX_RECORD_FUNCTION_LIST mxv_rdi_mbc_log_record_function_list;
extern MX_OPERATION_FUNCTION_LIST mxv_rdi_mbc_log_operation_function_list;

extern long mxv_rdi_mbc_log_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxv_rdi_mbc_log_rfield_def_ptr;

#endif /* __V_RDI_MBC_LOG_H__ */

