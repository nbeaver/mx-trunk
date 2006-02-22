/*
 * Name:    i_vxi_memacc.h
 *
 * Purpose: Header for NI-VISA VXI/VME access via the MEMACC resource.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2001 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_VXI_MEMACC_H__
#define __I_VXI_MEMACC_H__

#define MXU_NI_VISA_ERROR_MESSAGE_LENGTH	256

#if ( HAVE_NI_VISA && IS_MX_DRIVER )

typedef struct {
	ViSession session;
	ViChar description[VI_FIND_BUFLEN + 1];

	unsigned long old_read_address_increment;
	unsigned long old_write_address_increment;
} MX_VXI_MEMACC_CRATE;

typedef struct {
	ViSession resource_manager;

	unsigned long num_crates;
	MX_VXI_MEMACC_CRATE *crate_array;

} MX_VXI_MEMACC;

#endif /* HAVE_NI_VISA && IS_MX_DRIVER */

MX_API mx_status_type mxi_vxi_memacc_initialize_type( long type );
MX_API mx_status_type mxi_vxi_memacc_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_vxi_memacc_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_vxi_memacc_delete_record( MX_RECORD *record );
MX_API mx_status_type mxi_vxi_memacc_print_structure( FILE *file,
							MX_RECORD *record );
MX_API mx_status_type mxi_vxi_memacc_read_parms_from_hardware(
							MX_RECORD *record );
MX_API mx_status_type mxi_vxi_memacc_write_parms_to_hardware(
							MX_RECORD *record );
MX_API mx_status_type mxi_vxi_memacc_open( MX_RECORD *record );
MX_API mx_status_type mxi_vxi_memacc_close( MX_RECORD *record );
MX_API mx_status_type mxi_vxi_memacc_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxi_vxi_memacc_input( MX_VME *vme );
MX_API mx_status_type mxi_vxi_memacc_output( MX_VME *vme );
MX_API mx_status_type mxi_vxi_memacc_multi_input( MX_VME *vme );
MX_API mx_status_type mxi_vxi_memacc_multi_output( MX_VME *vme );
MX_API mx_status_type mxi_vxi_memacc_get_parameter( MX_VME *vme );
MX_API mx_status_type mxi_vxi_memacc_set_parameter( MX_VME *vme );

extern MX_RECORD_FUNCTION_LIST mxi_vxi_memacc_record_function_list;
extern MX_VME_FUNCTION_LIST mxi_vxi_memacc_vme_function_list;

extern long mxi_vxi_memacc_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_vxi_memacc_rfield_def_ptr;

#endif /* __I_VXI_MEMACC_H__ */

