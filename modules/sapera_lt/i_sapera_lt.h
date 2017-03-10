/*
 * Name:    i_sapera_lt.h
 *
 * Purpose: Header for DALSA's Sapera LT camera interface.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2011, 2016-2017 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_SAPERA_LT_H__
#define __I_SAPERA_LT_H__

#ifdef __cplusplus

/* Vendor include file. */

#include "SapClassBasic.h"

#define MXU_SAPERA_VERSION_STRING_LENGTH	40

/*----*/

/* Values for the 'sapera_flags' field. */

#define MXF_SAPERA_LT_SHOW_VERSION		0x1
#define MXF_SAPERA_LT_SHOW_LICENSE		0x2
#define MXF_SAPERA_LT_SHOW_AVAILABLE_SERVERS	0x4

/* Values for the 'license_type' field. */

#define MXT_SAPERA_LT_LICENSE_RUNTIME		1
#define MXT_SAPERA_LT_LICENSE_EVALUATION	2
#define MXT_SAPERA_LT_LICENSE_FULL_SDK		3

/*----*/

typedef struct {
	MX_RECORD *record;

	long max_devices;
	MX_RECORD **device_record_array;

	char server_name[CORSERVER_MAX_STRLEN+1];
	unsigned long sapera_flags;
	long eval_advance_warning_days;

	long server_index;
	long num_frame_grabbers;
	long num_cameras;

	unsigned long version_major;
	unsigned long version_minor;
	unsigned long version_revision;
	unsigned long version_build;

	char version_string[MXU_SAPERA_VERSION_STRING_LENGTH+1];
	
	long license_type;
	long eval_days_remaining;

} MX_SAPERA_LT;

#define MXI_SAPERA_LT_STANDARD_FIELDS \
  {-1, -1, "max_devices", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SAPERA_LT, max_devices), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "server_name", MXFT_STRING, NULL, 1, {CORSERVER_MAX_STRLEN}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SAPERA_LT, server_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY) }, \
  \
  {-1, -1, "sapera_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SAPERA_LT, sapera_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "eval_advance_warning_days", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_SAPERA_LT, eval_advance_warning_days), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "version_major", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SAPERA_LT, version_major), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "version_minor", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SAPERA_LT, version_minor), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "version_revision", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SAPERA_LT, version_revision), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "version_build", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SAPERA_LT, version_build), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "version_string", MXFT_STRING, NULL, \
				1, {MXU_SAPERA_VERSION_STRING_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SAPERA_LT, version_string), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "license_type", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SAPERA_LT, license_type), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "eval_days_remaining", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SAPERA_LT, eval_days_remaining), \
	{0}, NULL, MXFF_READ_ONLY}

#endif /* __cplusplus */

/* The following data structures must be exported as C symbols. */

#ifdef __cplusplus
extern "C" {
#endif

MX_API mx_status_type mxi_sapera_lt_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_sapera_lt_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_sapera_lt_open( MX_RECORD *record );
MX_API mx_status_type mxi_sapera_lt_close( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_sapera_lt_record_function_list;

extern long mxi_sapera_lt_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_sapera_lt_rfield_def_ptr;

#ifdef __cplusplus
}
#endif

#endif /* __I_SAPERA_LT_H__ */
