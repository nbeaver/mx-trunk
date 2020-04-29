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

#define MXU_GALIL_GCLIB_BUFFER_LENGTH	1024

/* Flag bits for the 'galil_gclib_flags' variable. */

#define MXF_GALIL_GCLIB_DEBUG_COMMANDS	0x1

typedef struct {
	MX_RECORD *record;

	char hostname[MXU_HOSTNAME_LENGTH+1];
	unsigned long galil_gclib_flags;

	char version[MXU_GALIL_GCLIB_BUFFER_LENGTH+1];

	GCon connection;

	long num_motors;
	MX_RECORD **motor_record_array;
} MX_GALIL_GCLIB;

#define MXI_GALIL_GCLIB_STANDARD_FIELDS \
  {-1, -1, "hostname", MXFT_STRING, NULL, 1, {MXU_HOSTNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_GALIL_GCLIB, hostname), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "galil_gclib_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_GALIL_GCLIB, galil_gclib_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "version", MXFT_STRING, NULL, 1, {MXU_GALIL_GCLIB_BUFFER_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_GALIL_GCLIB, version), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY }

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

extern MX_RECORD_FUNCTION_LIST mxi_galil_gclib_record_function_list;

extern long mxi_galil_gclib_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_galil_gclib_rfield_def_ptr;

#endif /* __I_GALIL_GCLIB_H__ */

