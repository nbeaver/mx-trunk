/*
 * Name:    d_soft_doutput.h
 *
 * Purpose: Header file for MX soft digital output devices.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SOFT_DOUTPUT_H__
#define __D_SOFT_DOUTPUT_H__

#include "mx_digital_output.h"

typedef struct {
	int dummy;
} MX_SOFT_DOUTPUT;

/* Define all of the interface functions. */

MX_API mx_status_type mxd_soft_doutput_initialize_type( long type );
MX_API mx_status_type mxd_soft_doutput_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_soft_doutput_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_soft_doutput_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_soft_doutput_print_structure( FILE *file,
							MX_RECORD *record );
MX_API mx_status_type mxd_soft_doutput_read_parms_from_hardware(
							MX_RECORD *record );
MX_API mx_status_type mxd_soft_doutput_write_parms_to_hardware(
							MX_RECORD *record );
MX_API mx_status_type mxd_soft_doutput_open( MX_RECORD *record );
MX_API mx_status_type mxd_soft_doutput_close( MX_RECORD *record );

MX_API mx_status_type mxd_soft_doutput_read( MX_DIGITAL_OUTPUT *doutput );
MX_API mx_status_type mxd_soft_doutput_write( MX_DIGITAL_OUTPUT *doutput );
MX_API mx_status_type mxd_soft_doutput_pulse( MX_DIGITAL_OUTPUT *doutput );

extern MX_RECORD_FUNCTION_LIST mxd_soft_doutput_record_function_list;
extern MX_DIGITAL_OUTPUT_FUNCTION_LIST
			mxd_soft_doutput_digital_output_function_list;

extern long mxd_soft_doutput_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_soft_doutput_rfield_def_ptr;

#endif /* __D_SOFT_DOUTPUT_H__ */

