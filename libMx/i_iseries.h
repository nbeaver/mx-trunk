/*
 * Name:    i_iseries.h
 *
 * Purpose: Header file for the MX interface driver for iSeries
 *          temperature and process controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_ISERIES_H__
#define __I_ISERIES_H__

#define MXU_ISERIES_BUS_ADDRESS_NAME_LENGTH	4
#define MXU_ISERIES_COMMAND_LENGTH		4

typedef struct {
	MX_RECORD *record;

	MX_RECORD *bus_record;
	char bus_address_name[ MXU_ISERIES_BUS_ADDRESS_NAME_LENGTH+1 ];

	long bus_address;
	char recognition_character;

	int echo_on;
	double software_version;
} MX_ISERIES;

#define MXI_ISERIES_STANDARD_FIELDS \
  {-1, -1, "bus_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ISERIES, bus_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "bus_address_name", MXFT_STRING, \
			NULL, 1, {MXU_ISERIES_BUS_ADDRESS_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ISERIES, bus_address_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxi_iseries_create_record_structures( MX_RECORD *record );

MX_API mx_status_type mxi_iseries_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxi_iseries_open( MX_RECORD *record );

MX_API mx_status_type mxi_iseries_command( MX_ISERIES *iseries,
					char command_prefix,
					unsigned long command_index,
					int num_command_bytes,
					double command_value,
					double *response_value,
					int precision,
					int debug_flag );

extern MX_RECORD_FUNCTION_LIST mxi_iseries_record_function_list;

extern long mxi_iseries_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_iseries_rfield_def_ptr;

#endif /* __I_ISERIES_H__ */

