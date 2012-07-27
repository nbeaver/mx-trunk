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

#if !defined(USE_DAQMX_BASE)
#  define USE_DAQMX_BASE	FALSE
#endif

#if USE_DAQMX_BASE
#  define USE_DAQMX		FALSE
#else
#  define USE_DAQMX		TRUE
#endif

/*----*/

#include "mx_list.h"

#if USE_DAQMX_BASE
#  include "i_ni_daqmx_base.h"
#  include "NIDAQmxBase.h"	/* Vendor include file. */
#else
#  include "NIDAQmx.h"		/* Vendor include file. */
#endif

/*----*/

#define MXU_NI_DAQMX_TASK_NAME_LENGTH		40
#define MXU_NI_DAQMX_CHANNEL_NAME_LENGTH	40

typedef struct {
	char task_name[MXU_NI_DAQMX_TASK_NAME_LENGTH+1];
	TaskHandle task_handle;
	long mx_datatype;
	unsigned long num_channels;
	void *channel_buffer;
} MX_NI_DAQMX_TASK;

typedef struct {
	MX_RECORD *record;

	MX_LIST *task_list;
} MX_NI_DAQMX;

#define MXI_NI_DAQMX_STANDARD_FIELDS

/*-----*/

MX_API mx_status_type mxi_ni_daqmx_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxi_ni_daqmx_open( MX_RECORD *record );

MX_API mx_status_type mxi_ni_daqmx_close( MX_RECORD *record );

MX_API mx_status_type mxi_ni_daqmx_finish_delayed_initialization(
							MX_RECORD *record );

/*-----*/

MX_API mx_status_type mxi_ni_daqmx_create_task( MX_NI_DAQMX *ni_daqmx,
						char *task_name,
						MX_NI_DAQMX_TASK **task );

MX_API mx_status_type mxi_ni_daqmx_shutdown_task( MX_NI_DAQMX *ni_daqmx,
						MX_NI_DAQMX_TASK *task );

/*-----*/

MX_API mx_status_type mxi_ni_daqmx_find_task( MX_NI_DAQMX *ni_daqmx,
						char *task_name,
						MX_NI_DAQMX_TASK **task );

MX_API mx_status_type mxi_ni_daqmx_find_task_by_handle( MX_NI_DAQMX *ni_daqmx,
						TaskHandle task_handle,
						MX_NI_DAQMX_TASK **task );

/*-----*/

MX_API mx_status_type mxi_ni_daqmx_find_or_create_task( MX_NI_DAQMX *ni_daqmx,
						char *task_name,
						MX_NI_DAQMX_TASK **task );

/*-----*/

MX_API mx_status_type mxi_ni_daqmx_set_task_datatype( MX_NI_DAQMX_TASK *task,
							long mx_datatype );

/*-----*/

extern MX_RECORD_FUNCTION_LIST mxi_ni_daqmx_record_function_list;

extern long mxi_ni_daqmx_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_ni_daqmx_rfield_def_ptr;

#endif /* __I_NI_DAQMX_H__ */

