/*
 * Name:    i_nuvant_ezstat.h
 *
 * Purpose: Header for NuVant EZstat controller driver.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_NUVANT_EZSTAT_H__
#define __I_NUVANT_EZSTAT_H__

/*---*/

#include "NIDAQmx.h"		/* National Instruments include file. */

/*---*/

#define MXU_NUVANT_DEVICE_NAME_LENGTH	64

/* Values of the 'ezstat_mode' field below. */

#define MXF_NUVANT_EZSTAT_POTENTIOSTAT_MODE	0
#define MXF_NUVANT_EZSTAT_GALVANOSTAT_MODE	1

typedef struct {
	MX_RECORD *record;

	MX_RECORD *ni_daqmx_record;
	char device_name[MXU_NUVANT_DEVICE_NAME_LENGTH+1];

	mx_bool_type show_parameters;

	mx_bool_type cell_on;

	unsigned long ezstat_mode;

	unsigned long potentiostat_binary_range;
	unsigned long galvanostat_binary_range;

	double potentiostat_current_range;
	double galvanostat_current_range;

	double potentiostat_resistance;
	double galvanostat_resistance;

} MX_NUVANT_EZSTAT;

#define MXLV_NUVANT_EZSTAT_SHOW_PARAMETERS	31001

#define MXI_NUVANT_EZSTAT_STANDARD_FIELDS \
  {-1, -1, "ni_daqmx_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NUVANT_EZSTAT, ni_daqmx_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "device_name", MXFT_STRING, NULL, 1,{MXU_NUVANT_DEVICE_NAME_LENGTH},\
	MXF_REC_TYPE_STRUCT, offsetof(MX_NUVANT_EZSTAT, device_name), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {MXLV_NUVANT_EZSTAT_SHOW_PARAMETERS, -1, "show_parameters", \
		MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NUVANT_EZSTAT, show_parameters), \
	{0}, NULL, 0 }

MX_API mx_status_type mxi_nuvant_ezstat_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_nuvant_ezstat_open( MX_RECORD *record );
MX_API mx_status_type mxi_nuvant_ezstat_special_processing_setup(
							MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_nuvant_ezstat_record_function_list;

extern long mxi_nuvant_ezstat_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_nuvant_ezstat_rfield_def_ptr;

/*---*/

MX_API mx_status_type mxi_nuvant_ezstat_create_task( char *task_name,
						TaskHandle *task_handle );

MX_API mx_status_type mxi_nuvant_ezstat_start_task( TaskHandle task_handle );

MX_API mx_status_type mxi_nuvant_ezstat_shutdown_task( TaskHandle task_handle );

/*---*/

MX_API mx_status_type mxi_nuvant_ezstat_read_ai_values(
					MX_NUVANT_EZSTAT *ezstat,
					double *ai_values );

MX_API mx_status_type mxi_nuvant_ezstat_set_binary_range(
					MX_NUVANT_EZSTAT *ezstat,
					unsigned long ezstat_mode,
					unsigned long binary_range );

MX_API mx_status_type mxi_nuvant_ezstat_set_current_range(
					MX_NUVANT_EZSTAT *ezstat,
					unsigned long ezstat_mode,
					double current_range );

#endif /* __I_NUVANT_EZSTAT_H__ */

