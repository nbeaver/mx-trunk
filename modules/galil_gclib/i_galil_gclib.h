/*
 * Name:    i_galil_gclib.h
 *
 * Purpose: Header file for Galil motor controllers via the gclib library.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2020 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_GALIL_GCLIB_H__
#define __I_GALIL_GCLIB_H__

/* Vendor include file. */

#include "gclibo.h"

#define MXU_GALIL_GCLIB_VERSION_LENGTH	40

#define MXU_GALIL_GCLIB_BUFFER_LENGTH	1024

/* Flag bits for the 'galil_gclib_flags' variable. */

#define MXF_GALIL_GCLIB_DEBUG_COMMANDS	0x1

typedef struct {
	MX_RECORD *record;

	char hostname[MXU_HOSTNAME_LENGTH+1];
	unsigned long galil_gclib_flags;

	char controller_type[MXU_GALIL_GCLIB_VERSION_LENGTH+1];
	char firmware_revision[MXU_GALIL_GCLIB_VERSION_LENGTH+1];
	char gclib_version[MXU_GALIL_GCLIB_VERSION_LENGTH+1];

	unsigned long error_code;
	long gclib_status;

	char command[MXU_GALIL_GCLIB_BUFFER_LENGTH+1];
	char response[MXU_GALIL_GCLIB_BUFFER_LENGTH+1];

	char command_file[MXU_FILENAME_LENGTH+1];

	GCon connection;

} MX_GALIL_GCLIB;

#define MXLV_GALIL_GCLIB_ERROR_CODE		0x9201
#define MXLV_GALIL_GCLIB_GCLIB_STATUS		0x9202

#define MXLV_GALIL_GCLIB_COMMAND		0x9301
#define MXLV_GALIL_GCLIB_COMMAND_FILE		0x9302

#define MXI_GALIL_GCLIB_STANDARD_FIELDS \
  {-1, -1, "hostname", MXFT_STRING, NULL, 1, {MXU_HOSTNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_GALIL_GCLIB, hostname), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "galil_gclib_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_GALIL_GCLIB, galil_gclib_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "controller_type", MXFT_STRING, \
				NULL, 1, {MXU_GALIL_GCLIB_VERSION_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_GALIL_GCLIB, controller_type), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "firmware_revision", MXFT_STRING, \
				NULL, 1, {MXU_GALIL_GCLIB_VERSION_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_GALIL_GCLIB, firmware_revision), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "gclib_version", MXFT_STRING, \
				NULL, 1, {MXU_GALIL_GCLIB_VERSION_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_GALIL_GCLIB, gclib_version), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY }, \
  \
  {MXLV_GALIL_GCLIB_ERROR_CODE, -1, "error_code", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_GALIL_GCLIB, error_code), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {MXLV_GALIL_GCLIB_GCLIB_STATUS, -1, "gclib_status", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_GALIL_GCLIB, gclib_status), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {MXLV_GALIL_GCLIB_COMMAND, -1, "command", \
		MXFT_STRING, NULL, 1, {MXU_GALIL_GCLIB_BUFFER_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_GALIL_GCLIB, command), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "response", MXFT_STRING, NULL, 1, {MXU_GALIL_GCLIB_BUFFER_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_GALIL_GCLIB, response), \
	{0}, NULL, 0 }, \
  \
  {MXLV_GALIL_GCLIB_COMMAND_FILE, -1, "command_file", \
		MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_GALIL_GCLIB, command_file), \
	{0}, NULL, 0 }
		

MX_API mx_status_type mxi_galil_gclib_create_record_structures(
						MX_RECORD *record );

MX_API mx_status_type mxi_galil_gclib_finish_record_initialization(
						MX_RECORD *record );

MX_API mx_status_type mxi_galil_gclib_open( MX_RECORD *record );

MX_API mx_status_type mxi_galil_gclib_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxi_galil_gclib_special_processing_setup(
							MX_RECORD *record );

MX_API mx_status_type mxi_galil_gclib_command( MX_GALIL_GCLIB *galil_gclib,
				char *command,
				char *response, size_t response_length );

MX_API mx_status_type mxi_galil_gclib_run_command_file(
						MX_GALIL_GCLIB *galil_gclib,
						char *filename );

extern MX_RECORD_FUNCTION_LIST mxi_galil_gclib_record_function_list;

extern long mxi_galil_gclib_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_galil_gclib_rfield_def_ptr;

#endif /* __I_GALIL_GCLIB_H__ */

