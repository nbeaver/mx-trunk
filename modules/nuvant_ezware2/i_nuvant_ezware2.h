/*
 * Name:    i_nuvant_ezware2.h
 *
 * Purpose: Header for NuVant EZstat controller driver via EZWare2.DLL.
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

#ifndef __I_NUVANT_EZWARE2_H__
#define __I_NUVANT_EZWARE2_H__

#define MXF_NUVANT_EZWARE2_POTENTIOSTAT_MODE	0
#define MXF_NUVANT_EZWARE2_GALVANOSTAT_MODE	1

typedef struct {
	MX_RECORD *record;

} MX_NUVANT_EZWARE2;

#define MXI_NUVANT_EZWARE2_STANDARD_FIELDS 

MX_API mx_status_type mxi_nuvant_ezware2_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_nuvant_ezware2_open( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_nuvant_ezware2_record_function_list;

extern long mxi_nuvant_ezware2_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_nuvant_ezware2_rfield_def_ptr;

/*---*/

#endif /* __I_NUVANT_EZWARE2_H__ */

