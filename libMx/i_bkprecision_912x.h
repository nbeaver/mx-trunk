/*
 * Name:    i_bkprecision_912x.h
 *
 * Purpose: Header file for the BK Precision 912x series of programmable
 *          power supplies.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_BKPRECISION_912X_H__
#define __I_BKPRECISION_912X_H__

typedef struct {
	MX_RECORD *record;

	MX_INTERFACE port_interface;

	char model_name[10];
} MX_BKPRECISION_912X;

#define MXI_BKPRECISION_912X_STANDARD_FIELDS \
  {-1, -1, "port_interface", MXFT_INTERFACE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BKPRECISION_912X, port_interface), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxi_bkprecision_912x_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxi_bkprecision_912x_open( MX_RECORD *record );

MX_API mx_status_type mxi_bkprecision_912x_command(
					MX_BKPRECISION_912X *bkprecision_912x,
					char *command,
					char *response,
					size_t response_buffer_length,
					int debug_flag );

extern MX_RECORD_FUNCTION_LIST mxi_bkprecision_912x_record_function_list;

extern long mxi_bkprecision_912x_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_bkprecision_912x_rfield_def_ptr;

#endif /* __I_BKPRECISION_912X_H__ */

