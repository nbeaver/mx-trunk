/*
 * Name:    v_rdi_mbc_string.h
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

#ifndef __V_RDI_MBC_STRING_H__
#define __V_RDI_MBC_STRING_H__

#define MXU_RDI_MBC_STRING_TYPE_LENGTH	20

#define MXT_RDI_MBC_STRING		1
#define MXT_RDI_MBC_FILENAME		2
#define MXT_RDI_MBC_DATAFILE_PREFIX	3

typedef struct {
	MX_RECORD *record;

	char external_field_name[MXU_RECORD_FIELD_NAME_LENGTH+1];

	MX_RECORD *external_record;
	MX_RECORD_FIELD *external_field;
	MX_RECORD_FIELD *internal_field;

	char *external_string_ptr;
	size_t external_string_length;

	char *internal_string_ptr;
	size_t internal_string_length;

	unsigned long string_type;
} MX_RDI_MBC_STRING;


#define MXV_RDI_MBC_STRING_STANDARD_FIELDS \
  {-1, -1, "external_field_name", MXFT_STRING, \
			NULL, 1, {MXU_RECORD_FIELD_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_RDI_MBC_STRING, external_field_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }

MX_API_PRIVATE mx_status_type mxv_rdi_mbc_string_create_record_structures(
							MX_RECORD *record );
MX_API_PRIVATE mx_status_type mxv_rdi_mbc_string_finish_record_initialization(
							MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxv_rdi_mbc_string_send_variable(
							MX_VARIABLE *variable );
MX_API_PRIVATE mx_status_type mxv_rdi_mbc_string_receive_variable(
							MX_VARIABLE *variable );

extern MX_RECORD_FUNCTION_LIST mxv_rdi_mbc_string_record_function_list;
extern MX_VARIABLE_FUNCTION_LIST mxv_rdi_mbc_string_variable_function_list;

extern long mxv_rdi_mbc_string_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxv_rdi_mbc_string_rfield_def_ptr;

#endif /* __V_RDI_MBC_STRING_H__ */

