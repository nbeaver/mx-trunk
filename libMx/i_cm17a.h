/*
 * Name:    i_cm17a.h
 *
 * Purpose: Header file for the MX interface driver for X10 Firecracker (CM17A)
 *          home automation controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_CM17A_H__
#define __I_CM17A_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;

	int rts_bit;
	int dtr_bit;
} MX_CM17A;

#define MXI_CM17A_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_CM17A, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxi_cm17a_create_record_structures( MX_RECORD *record );

MX_API mx_status_type mxi_cm17a_open( MX_RECORD *record );

MX_API mx_status_type mxi_cm17a_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxi_cm17a_reset( MX_CM17A *cm17a, int debug_flag );

MX_API mx_status_type mxi_cm17a_standby( MX_CM17A *cm17a, int debug_flag );

MX_API mx_status_type mxi_cm17a_command( MX_CM17A *cm17a,
					mx_uint16_type command,
					int debug_flag );

extern MX_RECORD_FUNCTION_LIST mxi_cm17a_record_function_list;

extern long mxi_cm17a_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_cm17a_rfield_def_ptr;

#endif /* __I_CM17A_H__ */

