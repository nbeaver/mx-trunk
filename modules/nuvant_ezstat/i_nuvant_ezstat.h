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

typedef struct {
	MX_RECORD *record;

	MX_RECORD *ni_daqmx_record;

} MX_NUVANT_EZSTAT;

#define MXI_NUVANT_EZSTAT_STANDARD_FIELDS \
  {-1, -1, "ni_daqmx_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NUVANT_EZSTAT, ni_daqmx_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION }

MX_API mx_status_type mxi_nuvant_ezstat_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_nuvant_ezstat_open( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_nuvant_ezstat_record_function_list;

extern long mxi_nuvant_ezstat_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_nuvant_ezstat_rfield_def_ptr;

#endif /* __I_NUVANT_EZSTAT_H__ */

