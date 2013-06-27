/*
 * Name:    v_rdi_mbc_filename.h
 *
 * Purpose: MX variable header for custom filename drivers for RDI detectors
 *          used by the Molecular Biology Consortium.
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

#ifndef __V_RDI_MBC_FILENAME_H__
#define __V_RDI_MBC_FILENAME_H__

#define MXU_RDI_MBC_FILENAME_TYPE_LENGTH	20

typedef struct {
	MX_RECORD *record;

	MX_RECORD *parent_variable_record;
	char rdi_mbc_filename_type_name[MXU_RDI_MBC_FILENAME_TYPE_LENGTH+1];

	unsigned long rdi_mbc_filename_type;
} MX_RDI_MBC_FILENAME;


#define MXV_RDI_MBC_FILENAME_STANDARD_FIELDS \
  {-1, -1, "parent_variable_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
	    offsetof(MX_RDI_MBC_FILENAME, parent_variable_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "rdi_mbc_filename_type_name", MXFT_STRING, \
			NULL, 1, {MXU_RDI_MBC_FILENAME_TYPE_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
	    offsetof(MX_RDI_MBC_FILENAME, rdi_mbc_filename_type_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

MX_API_PRIVATE mx_status_type mxv_rdi_mbc_filename_create_record_structures(
							MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxv_rdi_mbc_filename_send_variable(
							MX_VARIABLE *variable );
MX_API_PRIVATE mx_status_type mxv_rdi_mbc_filename_receive_variable(
							MX_VARIABLE *variable );

extern MX_RECORD_FUNCTION_LIST mxv_rdi_mbc_filename_record_function_list;
extern MX_VARIABLE_FUNCTION_LIST mxv_rdi_mbc_filename_variable_function_list;

extern long mxv_rdi_mbc_filename_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxv_rdi_mbc_filename_rfield_def_ptr;

#endif /* __V_RDI_MBC_FILENAME_H__ */

