/*
 * Name:    i_nuvant_ezware2.h
 *
 * Purpose: Header for NuVant EZstat controller driver via EZWare2.DLL.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2014-2015 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_NUVANT_EZWARE2_H__
#define __I_NUVANT_EZWARE2_H__

#define MXF_NUVANT_EZWARE2_POTENTIOSTAT_MODE	0
#define MXF_NUVANT_EZWARE2_GALVANOSTAT_MODE	1

typedef uint8_t LVBoolean;

#include "EZware2.h"

typedef struct {
	MX_RECORD *record;

	long device_number;

	long potentiostat_current_range;
	long galvanostat_current_range;
} MX_NUVANT_EZWARE2;

#define MXI_NUVANT_EZWARE2_STANDARD_FIELDS \
  {-1, -1, "device_number", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NUVANT_EZWARE2, device_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxi_nuvant_ezware2_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_nuvant_ezware2_open( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_nuvant_ezware2_record_function_list;

extern long mxi_nuvant_ezware2_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_nuvant_ezware2_rfield_def_ptr;

/*---*/

extern long mxp_nuvant_ezware2_get_potentiostat_range_from_voltage(
							double voltage );

extern long mxp_nuvant_ezware2_get_galvanostat_range_from_current(
							double current );

/*---*/

#if 0
int32_t __cdecl GetEzWareVersion( void );

int32_t __cdecl SetVoltage( int32_t, double, int32_t );

int32_t __cdecl SetCurrent( int32_t, double, int32_t );
#endif

/*---*/

#endif /* __I_NUVANT_EZWARE2_H__ */

