/*
 * Name:    d_soft_amplifier.h
 *
 * Purpose: Header file for MX driver for a software emulated amplifiers.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999, 2001 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SOFT_AMPLIFIER_H__
#define __D_SOFT_AMPLIFIER_H__

#include "mx_amplifier.h"

typedef struct {
	int dummy_variable;
} MX_SOFT_AMPLIFIER;

MX_API mx_status_type mxd_soft_amplifier_initialize_type( long type );
MX_API mx_status_type mxd_soft_amplifier_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_soft_amplifier_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_soft_amplifier_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_soft_amplifier_read_parms_from_hardware(
							MX_RECORD *record );
MX_API mx_status_type mxd_soft_amplifier_write_parms_to_hardware(
							MX_RECORD *record );
MX_API mx_status_type mxd_soft_amplifier_open( MX_RECORD *record );
MX_API mx_status_type mxd_soft_amplifier_close( MX_RECORD *record );

MX_API mx_status_type mxd_soft_amplifier_get_gain( MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_soft_amplifier_set_gain( MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_soft_amplifier_get_offset( MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_soft_amplifier_set_offset( MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_soft_amplifier_get_time_constant(
						MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_soft_amplifier_set_time_constant(
						MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_soft_amplifier_get_parameter(
						MX_AMPLIFIER *amplifier );

extern MX_RECORD_FUNCTION_LIST mxd_soft_amplifier_record_function_list;
extern MX_AMPLIFIER_FUNCTION_LIST mxd_soft_amplifier_amplifier_function_list;

extern long mxd_soft_amplifier_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_soft_amplifier_rfield_def_ptr;

#endif /* __D_SOFT_AMPLIFIER_H__ */
