/*
 * Name:    i_vxworks_vme.h
 *
 * Purpose: Header for VME bus access for a VxWorks controlled VME crate.
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

#ifndef __I_VXWORKS_VME_H__
#define __I_VXWORKS_VME_H__

typedef struct {
	unsigned long vxworks_vme_flags;
} MX_VXWORKS_VME;

MX_API mx_status_type mxi_vxworks_vme_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_vxworks_vme_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_vxworks_vme_print_structure( FILE *file,
							MX_RECORD *record );
MX_API mx_status_type mxi_vxworks_vme_open( MX_RECORD *record );
MX_API mx_status_type mxi_vxworks_vme_close( MX_RECORD *record );
MX_API mx_status_type mxi_vxworks_vme_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxi_vxworks_vme_input( MX_VME *vme );
MX_API mx_status_type mxi_vxworks_vme_output( MX_VME *vme );
MX_API mx_status_type mxi_vxworks_vme_multi_input( MX_VME *vme );
MX_API mx_status_type mxi_vxworks_vme_multi_output( MX_VME *vme );
MX_API mx_status_type mxi_vxworks_vme_get_parameter( MX_VME *vme );
MX_API mx_status_type mxi_vxworks_vme_set_parameter( MX_VME *vme );

extern MX_RECORD_FUNCTION_LIST mxi_vxworks_vme_record_function_list;
extern MX_VME_FUNCTION_LIST mxi_vxworks_vme_vme_function_list;

extern long mxi_vxworks_vme_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_vxworks_vme_rfield_def_ptr;

#define MXI_VXWORKS_VME_STANDARD_FIELDS \
  {-1, -1, "vxworks_vme_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_VXWORKS_VME, vxworks_vme_flags ), \
	{0}, NULL, 0}

#endif /* __I_VXWORKS_VME_H__ */

