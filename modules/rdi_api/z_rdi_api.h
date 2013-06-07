/*
 * Name:    z_rdi_api.h
 *
 * Purpose: Header file for RDI's preferred API for area detectors.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __Z_RDI_API_H__
#define __Z_RDI_API_H__

#define MXU_RDI_API_SCAN_PARAMETERS_LENGTH	(MXU_FILENAME_LENGTH + 40)

typedef struct {
	MX_RECORD *record;

	MX_RECORD *area_detector_record;

	char scan_parameters[MXU_RDI_API_SCAN_PARAMETERS_LENGTH+1];
} MX_RDI_API;

#define MXLV_RDI_API_SCAN_PARAMETERS		110001

#define MXZ_RDI_API_STANDARD_FIELDS \
  {-1, -1, "area_detector_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_RDI_API, area_detector_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {MXLV_RDI_API_SCAN_PARAMETERS, -1, "scan_parameters", \
		MXFT_STRING, NULL, 1, {MXU_RDI_API_SCAN_PARAMETERS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_RDI_API, scan_parameters), \
	{sizeof(char)}, NULL, 0}

MX_API_PRIVATE mx_status_type mxz_rdi_api_create_record_structures(
							MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxz_rdi_api_finish_record_initialization(
							MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxz_rdi_api_open( MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxz_rdi_api_special_processing_setup(
							MX_RECORD *record );

/*---*/

MX_API_PRIVATE mx_status_type mxz_rdi_api_scan_mode( MX_RECORD *record );

extern long mxz_rdi_api_num_record_fields;
extern MX_RECORD_FUNCTION_LIST mxz_rdi_api_record_function_list;
extern MX_RECORD_FIELD_DEFAULTS *mxz_rdi_api_rfield_def_ptr;

#endif /* __Z_RDI_API_H__ */

