/*
 * Name:    i_sr630.h
 *
 * Purpose: Header file for MX driver for the Stanford Research Systems
 *          model SR630 16 channel thermocouple reader.
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

#ifndef __I_SR630_H__
#define __I_SR630_H__

typedef struct {
	MX_RECORD *record;

	MX_INTERFACE port_interface;
} MX_SR630;

MX_API mx_status_type mxi_sr630_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxi_sr630_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_sr630_open( MX_RECORD *record );
MX_API mx_status_type mxi_sr630_close( MX_RECORD *record );

MX_API mx_status_type mxi_sr630_command( MX_SR630 *sr630,
                        	char *command,
	                        char *response, int response_buffer_length,
				int debug_flag );

extern MX_RECORD_FUNCTION_LIST mxi_sr630_record_function_list;

extern long mxi_sr630_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_sr630_rfield_def_ptr;

#define MXI_SR630_STANDARD_FIELDS \
  {-1, -1, "port_interface", MXFT_INTERFACE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SR630, port_interface), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

#endif /* __I_SR630_H__ */
