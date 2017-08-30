/*
 * Name:    i_amptek_dp5.h
 *
 * Purpose: Header file for Amptek MCAs that use the DP5 protocol.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2017 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_AMPTEK_DP5_H__
#define __I_AMPTEK_DP5_H__

/* Values for the 'amptek_dp5_flags' field. */

#define MXF_AMPTEK_DP5_DEBUG	0x1

/*---*/

#define MXU_INTERFACE_TYPE_NAME_LENGTH		20
#define MXU_INTERFACE_ARGUMENTS_LENGTH		80

typedef struct {
	MX_RECORD *record;

	char interface_type_name[MXU_INTERFACE_TYPE_NAME_LENGTH+1];
	long interface_type;
	char interface_arguments[MXU_INTERFACE_ARGUMENTS_LENGTH+1];
	unsigned long amptek_dp5_flags;
} MX_AMPTEK_DP5;

#define MXLV_AMPTEK_DP5_FOO		86000

#define MXI_AMPTEK_DP5_STANDARD_FIELDS \
  {-1, -1, "amptek_dp5_interface", MXFT_INTERFACE, NULL, \
	  			1, {MXU_INTERFACE_TYPE_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_AMPTEK_DP5, interface_type_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \

MX_API mx_status_type mxi_amptek_dp5_create_record_structures(
						MX_RECORD *record );
MX_API mx_status_type mxi_amptek_dp5_open( MX_RECORD *record );
MX_API mx_status_type mxi_amptek_dp5_special_processing_setup(
						MX_RECORD *record );

MX_API mx_status_type mxi_amptek_dp5_command( MX_AMPTEK_DP5 *dg645,
					char *command,
					char *response,
					size_t max_response_length,
					unsigned long amptek_dp5_flags );

extern MX_RECORD_FUNCTION_LIST mxi_amptek_dp5_record_function_list;

extern long mxi_amptek_dp5_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_amptek_dp5_rfield_def_ptr;

#endif /* __I_AMPTEK_DP5_H__ */

