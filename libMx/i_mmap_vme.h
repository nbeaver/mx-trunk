/*
 * Name:    i_mmap_vme.h
 *
 * Purpose: Header for VME bus access via mmap().  This driver is intended
 *          for use with a VME crate where the VME address space may be
 *          accessed indirectly via mmap().
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2003 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_MMAP_VME_H__
#define __I_MMAP_VME_H__

typedef struct {
	MX_RECORD *record;

	unsigned long mmap_vme_flags;
	unsigned long page_size;

	int file_descriptor;

	unsigned long num_a16_pages;
	unsigned long num_a24_pages;
	unsigned long num_a32_pages;

	size_t a16_page_size;
	size_t a24_page_size;
	size_t a32_page_size;

	void **a16_page_table;
	void **a24_page_table;
	void **a32_page_table;
} MX_MMAP_VME;

MX_API mx_status_type mxi_mmap_vme_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_mmap_vme_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_mmap_vme_print_structure( FILE *file,
							MX_RECORD *record );
MX_API mx_status_type mxi_mmap_vme_open( MX_RECORD *record );
MX_API mx_status_type mxi_mmap_vme_close( MX_RECORD *record );
MX_API mx_status_type mxi_mmap_vme_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxi_mmap_vme_input( MX_VME *vme );
MX_API mx_status_type mxi_mmap_vme_output( MX_VME *vme );
MX_API mx_status_type mxi_mmap_vme_multi_input( MX_VME *vme );
MX_API mx_status_type mxi_mmap_vme_multi_output( MX_VME *vme );
MX_API mx_status_type mxi_mmap_vme_get_parameter( MX_VME *vme );
MX_API mx_status_type mxi_mmap_vme_set_parameter( MX_VME *vme );

extern MX_RECORD_FUNCTION_LIST mxi_mmap_vme_record_function_list;
extern MX_VME_FUNCTION_LIST mxi_mmap_vme_vme_function_list;

extern long mxi_mmap_vme_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_mmap_vme_rfield_def_ptr;

#define MXI_MMAP_VME_STANDARD_FIELDS \
  {-1, -1, "mmap_vme_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_MMAP_VME, mmap_vme_flags ), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "page_size", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_MMAP_VME, page_size ), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

#endif /* __I_MMAP_VME_H__ */

