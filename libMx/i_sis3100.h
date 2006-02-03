/*
 * Name:    i_sis3100.h
 *
 * Purpose: Header for the SIS1100/3100 PCI-to-VME bus interface.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2002, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_SIS3100_H__
#define __I_SIS3100_H__

typedef struct {
	int  file_descriptor;

	char filename[ MXU_FILENAME_LENGTH+1 ];
	int  crate_number;

	unsigned long version_register;
} MX_SIS3100;

MX_API mx_status_type mxi_sis3100_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxi_sis3100_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_sis3100_print_structure( FILE *file,
							MX_RECORD *record );
MX_API mx_status_type mxi_sis3100_open( MX_RECORD *record );
MX_API mx_status_type mxi_sis3100_close( MX_RECORD *record );
MX_API mx_status_type mxi_sis3100_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxi_sis3100_input( MX_VME *vme );
MX_API mx_status_type mxi_sis3100_output( MX_VME *vme );
MX_API mx_status_type mxi_sis3100_multi_input( MX_VME *vme );
MX_API mx_status_type mxi_sis3100_multi_output( MX_VME *vme );
MX_API mx_status_type mxi_sis3100_get_parameter( MX_VME *vme );
MX_API mx_status_type mxi_sis3100_set_parameter( MX_VME *vme );

extern MX_RECORD_FUNCTION_LIST mxi_sis3100_record_function_list;
extern MX_VME_FUNCTION_LIST mxi_sis3100_vme_function_list;

extern mx_length_type mxi_sis3100_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_sis3100_rfield_def_ptr;

#define MXI_SIS3100_STANDARD_FIELDS \
  {-1, -1, "filename", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_SIS3100, filename ), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "crate_number", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_SIS3100, crate_number ), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "version_register", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_SIS3100, version_register ), \
	{0}, NULL, 0}

#endif /* __I_SIS3100_H__ */

