/*
 * Name:    i_pmc_mcapi.h
 *
 * Purpose: Header for MX driver for New Focus pmc_mcapi controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2004, 2010 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_PMC_MCAPI_H__
#define __I_PMC_MCAPI_H__

#define MX_MAX_PMC_MCAPI_AXES	8

#define MXU_PMC_MCAPI_MAX_COMMAND_LENGTH	255

typedef struct {
	MX_RECORD *record;
	short controller_id;
	char savefile_name[MXU_FILENAME_LENGTH+1];
	char startup_file_name[MXU_FILENAME_LENGTH+1];

	char command[MXU_PMC_MCAPI_MAX_COMMAND_LENGTH+1];
	char response[MXU_PMC_MCAPI_MAX_COMMAND_LENGTH+1];
	char download_file[MXU_FILENAME_LENGTH+1];

#if defined(__MCAPI_H__) || defined(_INC_MCAPI)

	/* Mcapi.h has been included. */

	HCTRLR controller_handle;
#endif
} MX_PMC_MCAPI;


#define MXLV_PMC_MCAPI_COMMAND			7001
#define MXLV_PMC_MCAPI_RESPONSE			7002
#define MXLV_PMC_MCAPI_COMMAND_WITH_RESPONSE	7003

#define MXLV_PMC_MCAPI_DOWNLOAD_FILE		7010

#define MXI_PMC_MCAPI_STANDARD_FIELDS \
  {-1, -1, "controller_id", MXFT_SHORT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PMC_MCAPI, controller_id), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "savefile_name", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PMC_MCAPI, savefile_name), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "startup_file_name", MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PMC_MCAPI, startup_file_name), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {MXLV_PMC_MCAPI_COMMAND, -1, "command", MXFT_STRING,\
				NULL, 1, {MXU_PMC_MCAPI_MAX_COMMAND_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PMC_MCAPI, command), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_PMC_MCAPI_RESPONSE, -1, "response", MXFT_STRING, \
				NULL, 1, {MXU_PMC_MCAPI_MAX_COMMAND_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PMC_MCAPI, response), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_PMC_MCAPI_COMMAND_WITH_RESPONSE, -1, \
  			"command_with_response", MXFT_STRING,\
			NULL, 1, {MXU_PMC_MCAPI_MAX_COMMAND_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PMC_MCAPI, command), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_PMC_MCAPI_DOWNLOAD_FILE, -1, "download_file", MXFT_STRING, \
			NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_PMC_MCAPI, download_file), \
	{sizeof(char)}, NULL, 0}

MX_API mx_status_type mxi_pmc_mcapi_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_pmc_mcapi_open( MX_RECORD *record );
MX_API mx_status_type mxi_pmc_mcapi_close( MX_RECORD *record );
MX_API mx_status_type mxi_pmc_mcapi_resynchronize( MX_RECORD *record );
MX_API mx_status_type mxi_pmc_mcapi_special_processing_setup(
						MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_pmc_mcapi_record_function_list;

extern long mxi_pmc_mcapi_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_pmc_mcapi_rfield_def_ptr;

/* === Driver specific functions === */

MX_API mx_status_type mxi_pmc_mcapi_command(
		MX_PMC_MCAPI *pmc_mcapi,
		char *command,
		char *response, size_t max_response_length,
		int debug_flag );

MX_API mx_status_type mxi_pmc_mcapi_download_file(
		MX_PMC_MCAPI *pmc_mcapi,
		char *filename );

MX_API void mxi_pmc_mcapi_translate_error(
		long mcapi_error_code,
		char *buffer,
		long buffer_length );

#endif /* __I_PMC_MCAPI_H__ */

