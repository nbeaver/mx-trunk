/*
 * Name:    i_rtems_vme.h
 *
 * Purpose: Header for VME bus access under RTEMS.
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

#ifndef __I_RTEMS_VME_H__
#define __I_RTEMS_VME_H__

typedef struct {
	MX_RECORD *record;

	unsigned long rtems_vme_flags;
} MX_RTEMS_VME;

MX_API mx_status_type mxi_rtems_vme_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_rtems_vme_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxi_rtems_vme_input( MX_VME *vme );
MX_API mx_status_type mxi_rtems_vme_output( MX_VME *vme );
MX_API mx_status_type mxi_rtems_vme_multi_input( MX_VME *vme );
MX_API mx_status_type mxi_rtems_vme_multi_output( MX_VME *vme );
MX_API mx_status_type mxi_rtems_vme_get_parameter( MX_VME *vme );
MX_API mx_status_type mxi_rtems_vme_set_parameter( MX_VME *vme );

extern MX_RECORD_FUNCTION_LIST mxi_rtems_vme_record_function_list;
extern MX_VME_FUNCTION_LIST mxi_rtems_vme_vme_function_list;

extern long mxi_rtems_vme_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_rtems_vme_rfield_def_ptr;

#define MXI_RTEMS_VME_STANDARD_FIELDS \
  {-1, -1, "rtems_vme_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_RTEMS_VME, rtems_vme_flags ), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

#endif /* __I_RTEMS_VME_H__ */

