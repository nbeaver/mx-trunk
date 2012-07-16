/*
 * Name:    i_ni_daqmx.h
 *
 * Purpose: Header for the National Instruments DAQmx system.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2011-2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_NI_DAQMX_H__
#define __I_NI_DAQMX_H__

/*----*/


#if defined(USE_DAQMX_BASE)
#  include "i_ni_daqmx_base.h"
#  include "NIDAQmxBase.h"	/* Vendor include file. */
#else
#  include "NIDAQmx.h"		/* Vendor include file. */
#endif

/*----*/

#define MXU_NI_DAQMX_CHANNEL_NAME_LENGTH	40

typedef struct {
	MX_RECORD *record;

} MX_NI_DAQMX;

#define MXI_NI_DAQMX_STANDARD_FIELDS

/*-----*/

MX_API mx_status_type
mxi_ni_daqmx_create_task( MX_RECORD *record, TaskHandle *task_handle );

MX_API mx_status_type
mxi_ni_daqmx_shutdown_task( MX_RECORD *record, TaskHandle task_handle );

/*-----*/

MX_API mx_status_type mxi_ni_daqmx_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxi_ni_daqmx_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxi_ni_daqmx_open( MX_RECORD *record );

MX_API mx_status_type mxi_ni_daqmx_close( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_ni_daqmx_record_function_list;

extern long mxi_ni_daqmx_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_ni_daqmx_rfield_def_ptr;

#endif /* __I_NI_DAQMX_H__ */
