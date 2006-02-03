/*
 * Name:    i_pfcu.h
 *
 * Purpose: Header file for the MX interface driver for the XIA PFCU
 *          RS-232 interface to the PF4 and PF2S2 filter and shutter
 *          controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_PFCU_H__
#define __I_PFCU_H__

/* Values for the 'pfcu_flags' field. */

#define MXF_PFCU_ENABLE_SHUTTER		0x1
#define MXF_PFCU_LOCK_FRONT_PANEL	0x2

/* Module number 0xffffffff is used for module id PFCUALL */

#define MX_PFCU_PFCUALL			(0xffffffff)

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;
	unsigned long pfcu_flags;

	int exposure_in_progress;
} MX_PFCU;

#define MXI_PFCU_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PFCU, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "pfcu_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PFCU, pfcu_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \

MX_API mx_status_type mxi_pfcu_create_record_structures( MX_RECORD *record );

MX_API mx_status_type mxi_pfcu_open( MX_RECORD *record );

MX_API mx_status_type mxi_pfcu_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxi_pfcu_command( MX_PFCU *pfcu,
					unsigned long module_number,
					char *command,
					char *response,
					size_t response_buffer_length,
					int debug_flag );

extern MX_RECORD_FUNCTION_LIST mxi_pfcu_record_function_list;

extern mx_length_type mxi_pfcu_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_pfcu_rfield_def_ptr;

#endif /* __I_PFCU_H__ */

