/*
 * Name:    v_rdi_mbc_pathname_builder.h
 *
 * Purpose: MX variable header for a driver that constructs and takes apart
 *          the filenames used at the MBC beamline.
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

#ifndef __V_RDI_MBC_PATHNAME_BUILDER_H__
#define __V_RDI_MBC_PATHNAME_BUILDER_H__

typedef struct {
	MX_RECORD *record;

	char destination_field_name[MXU_RECORD_FIELD_NAME_LENGTH+1];

	unsigned long num_source_fields;
	char **source_field_name_array;

	MX_RECORD *destination_record;
	MX_RECORD_FIELD *destination_field;

	MX_RECORD_FIELD **source_field_array;

	MX_RECORD_FIELD *internal_value_field;

	char *destination_string_ptr;
	size_t destination_string_length;

	char *internal_string_ptr;
	size_t internal_string_length;

} MX_RDI_MBC_PATHNAME_BUILDER;


#define MXV_RDI_MBC_PATHNAME_BUILDER_STANDARD_FIELDS \
  {-1, -1, "destination_field_name", MXFT_STRING, \
			NULL, 1, {MXU_RECORD_FIELD_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_RDI_MBC_PATHNAME_BUILDER, destination_field_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "num_source_fields", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_RDI_MBC_PATHNAME_BUILDER, num_source_fields), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "source_field_name_array", MXFT_STRING, NULL, \
		2, {MXU_VARARGS_LENGTH, MXU_RECORD_FIELD_NAME_LENGTH+1}, \
	MXF_REC_TYPE_STRUCT, \
	    offsetof(MX_RDI_MBC_PATHNAME_BUILDER, source_field_name_array), \
	{sizeof(char), sizeof(char *)}, NULL, MXFF_IN_DESCRIPTION }

MX_API_PRIVATE mx_status_type
		mxv_rdi_mbc_pathname_builder_initialize_driver(
							MX_DRIVER *driver );

MX_API_PRIVATE mx_status_type
		mxv_rdi_mbc_pathname_builder_create_record_structures(
							MX_RECORD *record );
MX_API_PRIVATE mx_status_type
		mxv_rdi_mbc_pathname_builder_finish_record_initialization(
							MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxv_rdi_mbc_pathname_builder_send_variable(
							MX_VARIABLE *variable );
MX_API_PRIVATE mx_status_type mxv_rdi_mbc_pathname_builder_receive_variable(
							MX_VARIABLE *variable );

extern MX_RECORD_FUNCTION_LIST
			mxv_rdi_mbc_pathname_builder_record_function_list;
extern MX_VARIABLE_FUNCTION_LIST
			mxv_rdi_mbc_pathname_builder_variable_function_list;

extern long mxv_rdi_mbc_pathname_builder_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxv_rdi_mbc_pathname_builder_rfield_def_ptr;

#endif /* __V_RDI_MBC_PATHNAME_BUILDER_H__ */

