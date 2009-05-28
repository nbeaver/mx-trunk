/*
 * Name:    i_epics_vme.h
 *
 * Purpose: Header for VME access via the EPICS VME record written
 *          by Mark Rivers of the University of Chicago.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2001-2003, 2009 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_EPICS_VME_H__
#define __I_EPICS_VME_H__

#define MXF_EPICS_VME_INPUT	0
#define MXF_EPICS_VME_OUTPUT	1

typedef struct {
	MX_RECORD *record;

	char epics_record_name[ MXU_EPICS_PVNAME_LENGTH+1 ];

	unsigned long old_address;
	unsigned long old_address_mode;
	unsigned long old_data_size;

	unsigned long old_num_values;
	unsigned long old_address_increment;

	int old_direction;

	int32_t max_epics_values;
	int32_t *temp_buffer;
	unsigned char *status_array;

	MX_EPICS_PV addr_pv;
	MX_EPICS_PV ainc_pv;
	MX_EPICS_PV amod_pv;
	MX_EPICS_PV dsiz_pv;
	MX_EPICS_PV nuse_pv;
	MX_EPICS_PV proc_pv;
	MX_EPICS_PV rdwt_pv;
	MX_EPICS_PV sarr_pv;
	MX_EPICS_PV val_pv;
} MX_EPICS_VME;

MX_API mx_status_type mxi_epics_vme_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_epics_vme_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_epics_vme_delete_record( MX_RECORD *record );
MX_API mx_status_type mxi_epics_vme_print_structure( FILE *file,
							MX_RECORD *record );
MX_API mx_status_type mxi_epics_vme_open( MX_RECORD *record );
MX_API mx_status_type mxi_epics_vme_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxi_epics_vme_input( MX_VME *vme );
MX_API mx_status_type mxi_epics_vme_output( MX_VME *vme );
MX_API mx_status_type mxi_epics_vme_multi_input( MX_VME *vme );
MX_API mx_status_type mxi_epics_vme_multi_output( MX_VME *vme );
MX_API mx_status_type mxi_epics_vme_get_parameter( MX_VME *vme );
MX_API mx_status_type mxi_epics_vme_set_parameter( MX_VME *vme );

extern MX_RECORD_FUNCTION_LIST mxi_epics_vme_record_function_list;
extern MX_VME_FUNCTION_LIST mxi_epics_vme_vme_function_list;

extern long mxi_epics_vme_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_epics_vme_rfield_def_ptr;

#define MXI_EPICS_VME_STANDARD_FIELDS \
  {-1, -1, "epics_record_name", MXFT_STRING, \
		NULL, 1, {MXU_EPICS_PVNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_EPICS_VME, epics_record_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __I_EPICS_VME_H__ */

