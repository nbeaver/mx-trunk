/*
 * Name:    i_sim900.h
 *
 * Purpose: Header file for the Stanford Research Systems SIM900 mainframe.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_SIM900_H__
#define __I_SIM900_H__

/* Values for 'sim900_flags' */

#define MXF_SIM900_DEBUG			0x1

typedef struct {
	MX_RECORD *record;

	MX_INTERFACE sim900_interface;
	unsigned long sim900_flags;
	double version;
} MX_SIM900;

#define MXI_SIM900_STANDARD_FIELDS \
  {-1, -1, "sim900_interface", MXFT_INTERFACE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIM900, sim900_interface), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "sim900_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIM900, sim900_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

MX_API mx_status_type mxi_sim900_create_record_structures( MX_RECORD *record );

MX_API mx_status_type mxi_sim900_open( MX_RECORD *record );

MX_API mx_status_type mxi_sim900_command( MX_SIM900 *sim900,
					char *command,
					char *response,
					size_t max_response_length,
					unsigned long sim900_flags );

MX_API mx_status_type mxi_sim900_device_clear( MX_SIM900 *sim900,
					unsigned long sim900_flags );

extern MX_RECORD_FUNCTION_LIST mxi_sim900_record_function_list;

extern long mxi_sim900_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_sim900_rfield_def_ptr;

#endif /* __I_SIM900_H__ */

