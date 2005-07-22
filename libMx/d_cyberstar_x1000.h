/*
 * Name:    d_cyberstar_x1000.h
 *
 * Purpose: Header file for MX driver for the Oxford Danfysik Cyberstar X1000
 *          scintillation detector and pulse processing system.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2002 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_CYBERSTAR_X1000_H__
#define __D_CYBERSTAR_X1000_H__

#include "mx_sca.h"

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;
	int address;

	double high_voltage;
	double delay;
	int    forced_remote_control;
	int    security_status;

	int    discard_echoed_command_line;
} MX_CYBERSTAR_X1000;

#define MXLV_CYBERSTAR_X1000_HIGH_VOLTAGE		0
#define MXLV_CYBERSTAR_X1000_DELAY			1
#define MXLV_CYBERSTAR_X1000_FORCED_REMOTE_CONTROL	2
#define MXLV_CYBERSTAR_X1000_SECURITY_STATUS		3

MX_API mx_status_type mxd_cyberstar_x1000_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_cyberstar_x1000_open( MX_RECORD *record );
MX_API mx_status_type mxd_cyberstar_x1000_resynchronize( MX_RECORD *record );
MX_API mx_status_type mxd_cyberstar_x1000_special_processing_setup(
							MX_RECORD *record );

MX_API mx_status_type mxd_cyberstar_x1000_get_parameter( MX_SCA *sca );
MX_API mx_status_type mxd_cyberstar_x1000_set_parameter( MX_SCA *sca );

MX_API mx_status_type mxd_cyberstar_x1000_command(
				MX_CYBERSTAR_X1000 *cyberstar_x1000,
				char *command,
				char *response,
				size_t response_buffer_length,
				int debug_flag );

extern MX_RECORD_FUNCTION_LIST mxd_cyberstar_x1000_record_function_list;
extern MX_SCA_FUNCTION_LIST mxd_cyberstar_x1000_sca_function_list;

extern long mxd_cyberstar_x1000_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_cyberstar_x1000_rfield_def_ptr;

#define MXD_CYBERSTAR_X1000_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_CYBERSTAR_X1000, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "address", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_CYBERSTAR_X1000, address), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {MXLV_CYBERSTAR_X1000_HIGH_VOLTAGE, -1, "high_voltage", \
	  					MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_CYBERSTAR_X1000, high_voltage), \
	{0}, NULL, 0}, \
  \
  {MXLV_CYBERSTAR_X1000_DELAY, -1, "delay", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_CYBERSTAR_X1000, delay), \
	{0}, NULL, 0}, \
  \
  {MXLV_CYBERSTAR_X1000_FORCED_REMOTE_CONTROL, -1, \
	  	"forced_remote_control", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_CYBERSTAR_X1000, \
						forced_remote_control), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {MXLV_CYBERSTAR_X1000_SECURITY_STATUS, -1, \
	  	"security_status", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_CYBERSTAR_X1000, security_status), \
	{0}, NULL, 0}

#endif /* __D_CYBERSTAR_X1000_H__ */

