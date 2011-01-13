/*
 * Name:    i_pleora_iport.h
 *
 * Purpose: Header for the Pleora iPORT camera interface.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2010-2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_PLEORA_IPORT_H__
#define __I_PLEORA_IPORT_H__

#ifdef __cplusplus

/* Vendor include files. */

#include "CyDeviceFinder.h"
#include "CyGrabber.h"

/*----*/

typedef struct {
	MX_RECORD *record;

	long max_devices;

	MX_RECORD **device_record_array;

} MX_PLEORA_IPORT;

#define MXI_PLEORA_IPORT_STANDARD_FIELDS \
  {-1, -1, "max_devices", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PLEORA_IPORT, max_devices), \
	{0}, NULL, MXFF_IN_DESCRIPTION }

#endif /* __cplusplus */

#ifdef __cplusplus
extern "C" {
#endif

MX_API mx_status_type mxi_pleora_iport_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_pleora_iport_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_pleora_iport_open( MX_RECORD *record );
MX_API mx_status_type mxi_pleora_iport_close( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_pleora_iport_record_function_list;

extern long mxi_pleora_iport_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_pleora_iport_rfield_def_ptr;

#ifdef __cplusplus
}
#endif

#endif /* __I_PLEORA_IPORT_H__ */
