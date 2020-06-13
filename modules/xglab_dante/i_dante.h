/*
 * Name:   i_dante.h
 *
 * Purpose: Interface driver header for the XGLab DANTE MCA.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2020 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_DANTE_H__
#define __I_DANTE_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MXU_DANTE_MAX_VERSION_LENGTH		20

#define MXU_DANTE_MAX_IDENTIFIER_LENGTH		16

/* The following flags are used by the 'dante_flags' field. */

#define MXF_DANTE_SHOW_DEVICES			0x1

/* Define the data structures used by the Dante driver. */

typedef struct {
	MX_RECORD *record;
	unsigned long max_boards_per_chain;
	unsigned long dante_flags;

	char dante_version[MXU_DANTE_MAX_VERSION_LENGTH+1];
	unsigned long num_master_devices;

	unsigned long num_mcas;
	MX_RECORD **mca_record_array;

	unsigned long *num_boards_for_chain;
	char ***board_identifier;
} MX_DANTE;

#define MXI_DANTE_STANDARD_FIELDS \
  {-1, -1, "dante_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DANTE, dante_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "max_boards_per_chain", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DANTE, max_boards_per_chain), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "dante_version", MXFT_STRING, NULL, \
				1, {MXU_DANTE_MAX_VERSION_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DANTE, dante_version), \
	{sizeof(char)}, NULL, 0 }, \
  \
  {-1, -1, "num_mcas", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DANTE, num_mcas), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {-1, -1, "num_master_devices", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DANTE, num_master_devices), \
	{0}, NULL, 0 }, \

MX_API mx_status_type mxi_dante_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxi_dante_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_dante_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_dante_open( MX_RECORD *record );
MX_API mx_status_type mxi_dante_close( MX_RECORD *record );
MX_API mx_status_type mxi_dante_special_processing_setup(
							MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_dante_record_function_list;

extern long mxi_dante_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_dante_rfield_def_ptr;

#ifdef __cplusplus
}
#endif

/* === Driver specific functions === */

#endif /* __I_DANTE_H__ */
