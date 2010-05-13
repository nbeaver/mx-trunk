/*
 * Name:    d_linkam_t9x_pump.h
 *
 * Purpose: Header file for the pump controller part of the Linkam T9x
 *          series of cooling controllers.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_LINKAM_T9X_PUMP_H__
#define __D_LINKAM_T9X_PUMP_H__

typedef struct {
	MX_RECORD *record;
	MX_RECORD *linkam_t9x_record;
} MX_LINKAM_T9X_PUMP;

#define MXD_LINKAM_T9X_PUMP_STANDARD_FIELDS \
  {-1, -1, "linkam_t9x_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_LINKAM_T9X_PUMP, linkam_t9x_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Define all of the interface functions. */

MX_API mx_status_type mxd_linkam_t9x_pump_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_linkam_t9x_pump_read( MX_ANALOG_OUTPUT *dac );
MX_API mx_status_type mxd_linkam_t9x_pump_write( MX_ANALOG_OUTPUT *dac );

extern MX_RECORD_FUNCTION_LIST mxd_linkam_t9x_pump_record_function_list;
extern MX_ANALOG_OUTPUT_FUNCTION_LIST
			mxd_linkam_t9x_pump_analog_output_function_list;

extern long mxd_linkam_t9x_pump_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_linkam_t9x_pump_rfield_def_ptr;

#endif /* __D_LINKAM_T9X_PUMP_H__ */

