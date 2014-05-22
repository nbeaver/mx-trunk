/*
 * Name:    i_newport_xps.h
 *
 * Purpose: Header file for Newport XPS motor controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_NEWPORT_XPS_H__
#define __I_NEWPORT_XPS_H__

#define MXU_NEWPORT_XPS_AUTH_LENGTH	40

#define MXU_NEWPORT_XPS_STATUS_LENGTH	250

typedef struct {
	MX_RECORD *record;

	char hostname[MXU_HOSTNAME_LENGTH+1];
	unsigned long port_number;
	double timeout;
	char username[MXU_NEWPORT_XPS_AUTH_LENGTH+1];
	char password[MXU_NEWPORT_XPS_AUTH_LENGTH+1];

	long socket_id;

	long controller_status;
	char
	  controller_status_string[MXU_NEWPORT_XPS_STATUS_LENGTH+1];

	double elapsed_time;
	char firmware_version[MXU_NEWPORT_XPS_STATUS_LENGTH+1];
	char hardware_time[MXU_NEWPORT_XPS_STATUS_LENGTH+1];
	char library_version[MXU_NEWPORT_XPS_STATUS_LENGTH+1];
} MX_NEWPORT_XPS;

#define MXLV_NEWPORT_XPS_SOCKET_ID			87001
#define MXLV_NEWPORT_XPS_CONTROLLER_STATUS		87002
#define MXLV_NEWPORT_XPS_CONTROLLER_STATUS_STRING	87003
#define MXLV_NEWPORT_XPS_ELAPSED_TIME			87004
#define MXLV_NEWPORT_XPS_FIRMWARE_VERSION		87005
#define MXLV_NEWPORT_XPS_HARDWARE_TIME			87006
#define MXLV_NEWPORT_XPS_LIBRARY_VERSION		87007

#define MXI_NEWPORT_XPS_STANDARD_FIELDS \
  {-1, -1, "hostname", MXFT_STRING, NULL, 1, {MXU_HOSTNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NEWPORT_XPS, hostname), \
	{sizeof(char)}, NULL, \
		(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_READ_ONLY)}, \
  \
  {-1, -1, "port_number", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NEWPORT_XPS, port_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_READ_ONLY)}, \
  \
  {-1, -1, "timeout", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NEWPORT_XPS, timeout), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_READ_ONLY)}, \
  \
  {-1, -1, "username", MXFT_STRING, NULL, 1, {MXU_NEWPORT_XPS_AUTH_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NEWPORT_XPS, username), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_NO_ACCESS) }, \
  \
  {-1, -1, "password", MXFT_STRING, NULL, 1, {MXU_NEWPORT_XPS_AUTH_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NEWPORT_XPS, password), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_NO_ACCESS) }, \
  \
  {MXLV_NEWPORT_XPS_SOCKET_ID, -1, "socket_id", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NEWPORT_XPS, socket_id), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_NEWPORT_XPS_CONTROLLER_STATUS, -1, "controller_status", \
			MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NEWPORT_XPS, controller_status), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_NEWPORT_XPS_CONTROLLER_STATUS_STRING, -1, "controller_status_string",\
	    MXFT_STRING, NULL, 1, {MXU_NEWPORT_XPS_STATUS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, \
			offsetof(MX_NEWPORT_XPS, controller_status_string), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_NEWPORT_XPS_ELAPSED_TIME, -1, "elapsed_time", \
			MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NEWPORT_XPS, elapsed_time), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_NEWPORT_XPS_FIRMWARE_VERSION, -1, "firmware_version",\
	    MXFT_STRING, NULL, 1, {MXU_NEWPORT_XPS_STATUS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NEWPORT_XPS, firmware_version), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_NEWPORT_XPS_HARDWARE_TIME, -1, "hardware_time",\
	    MXFT_STRING, NULL, 1, {MXU_NEWPORT_XPS_STATUS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NEWPORT_XPS, hardware_time), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_NEWPORT_XPS_LIBRARY_VERSION, -1, "library_version",\
	    MXFT_STRING, NULL, 1, {MXU_NEWPORT_XPS_STATUS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_NEWPORT_XPS, library_version), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY}

MX_API mx_status_type mxi_newport_xps_create_record_structures(
						MX_RECORD *record );

MX_API mx_status_type mxi_newport_xps_open( MX_RECORD *record );

MX_API mx_status_type mxi_newport_xps_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxi_newport_xps_special_processing_setup(
							MX_RECORD *record );

MX_API mx_status_type mxi_newport_xps_error( int socket_id,
						char *api_name,
						int error_code );

extern MX_RECORD_FUNCTION_LIST mxi_newport_xps_record_function_list;

extern long mxi_newport_xps_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_newport_xps_rfield_def_ptr;

#endif /* __I_NEWPORT_XPS_H__ */

