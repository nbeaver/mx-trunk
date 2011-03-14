/*
 * Name:    i_daqmx_base.h
 *
 * Purpose: Header for the National Instruments DAQmx Base system.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2011 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_DAQMX_BASE_H__
#define __I_DAQMX_BASE_H__

/*----*/

#include "NIDAQmxBase.h"	/* Vendor include file. */

/*----*/

#define MXU_DAQMX_BASE_CHANNEL_NAME_LENGTH	40

typedef struct {
	MX_RECORD *record;

	long dummy;

} MX_DAQMX_BASE;

#define MXI_DAQMX_BASE_STANDARD_FIELDS \
  {-1, -1, "dummy", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DAQMX_BASE, dummy), \
	{0}, NULL, MXFF_IN_DESCRIPTION }

/*-----*/

MX_API mx_status_type
mxi_daqmx_base_shutdown_task( MX_RECORD *record, TaskHandle task_handle );

/*-----*/

MX_API mx_status_type mxi_daqmx_base_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxi_daqmx_base_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxi_daqmx_base_open( MX_RECORD *record );

MX_API mx_status_type mxi_daqmx_base_close( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_daqmx_base_record_function_list;

extern long mxi_daqmx_base_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_daqmx_base_rfield_def_ptr;

#endif /* __I_DAQMX_BASE_H__ */
