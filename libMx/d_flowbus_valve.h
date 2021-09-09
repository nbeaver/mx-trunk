/*
 * Name:    d_flowbus_valve.h
 *
 * Purpose: MX header file for Flowbus valves.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2021 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_FLOWBUS_VALVE_H__
#define __D_FLOWBUS_VALVE_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *flowbus_record;
	unsigned long node_address;

	/* Theoretically the following parameters are discovered by
	 * talking to the controller at the specified node_address.
	 * But this is not currently implemented.
	 */

	unsigned long process_number;
	unsigned long parameter_number;
	unsigned long parameter_type;
} MX_FLOWBUS_VALVE;

#define MXD_FLOWBUS_VALVE_STANDARD_FIELDS \
  {-1, -1, "flowbus_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FLOWBUS_VALVE, flowbus_record), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "node_address", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FLOWBUS_VALVE, node_address), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_READ_ONLY) }, \
  \
  {-1, -1, "process_number", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FLOWBUS_VALVE, process_number), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "parameter_number", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FLOWBUS_VALVE, parameter_number), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "parameter_type", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_FLOWBUS_VALVE, parameter_type), \
	{0}, NULL, MXFF_READ_ONLY }

/* Define all of the device functions. */

MX_API mx_status_type mxd_flowbus_valve_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_flowbus_valve_open( MX_RECORD *record );

MX_API mx_status_type mxd_flowbus_valve_relay_command( MX_RELAY *relay );

MX_API mx_status_type mxd_flowbus_valve_get_relay_status( MX_RELAY *relay );

extern MX_RECORD_FUNCTION_LIST mxd_flowbus_valve_record_function_list;
extern MX_RELAY_FUNCTION_LIST mxd_flowbus_valve_relay_function_list;

extern long mxd_flowbus_valve_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_flowbus_valve_rfield_def_ptr;

#endif /* __D_FLOWBUS_VALVE_H__ */
