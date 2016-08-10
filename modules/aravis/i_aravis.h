/*
 * Name:    i_aravis.h
 *
 * Purpose: Header for the Aravis camera interface.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_ARAVIS_H__
#define __I_ARAVIS_H__

/* Aravis include file */

#include "arv.h"

/* Flag bits for the 'aravis_flags' field. */

#define MXF_ARAVIS_SHOW_CAMERA_LIST		0x1

/*----*/

#define MXU_ARAVIS_DEVICE_ID_LENGTH		80
#define MXU_ARAVIS_NAME_LENGTH			80

typedef struct {
	MX_RECORD *record;

	unsigned long aravis_flags;

	unsigned long num_devices;
	MX_RECORD **device_record_array;

} MX_ARAVIS;

#define MXI_ARAVIS_STANDARD_FIELDS \
  {-1, -1, "aravis_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ARAVIS, aravis_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "num_devices", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ARAVIS, num_devices), \
	{0}, NULL, MXFF_READ_ONLY }

/* The following data structures must be exported as C symbols. */

MX_API mx_status_type mxi_aravis_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxi_aravis_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_aravis_open( MX_RECORD *record );
MX_API mx_status_type mxi_aravis_close( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_aravis_record_function_list;

extern long mxi_aravis_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_aravis_rfield_def_ptr;

#endif /* __I_ARAVIS_H__ */
