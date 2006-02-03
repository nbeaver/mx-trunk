/* 
 * Name:    d_roentec_rcl_mca.h
 *
 * Purpose: Header file for Roentec MCAs that use the RCL 2.2 command
 *          language.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_ROENTEC_RCL_MCA_H__
#define __D_ROENTEC_RCL_MCA_H__

#define MX_ROENTEC_RCL_MAX_SCAS	8
#define MX_ROENTEC_RCL_MAX_BINS	4096

#define MXU_ROENTEC_RCL_MAX_COMMAND_LENGTH	100

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;

	char command[MXU_ROENTEC_RCL_MAX_COMMAND_LENGTH+1];
	char response[MXU_ROENTEC_RCL_MAX_COMMAND_LENGTH+1];

#if ( MX_WORDSIZE != 32 )
	uint32_t *channel_32bit_array;
#endif

} MX_ROENTEC_RCL_MCA;

#define MXLV_ROENTEC_RCL_COMMAND			7001
#define MXLV_ROENTEC_RCL_RESPONSE		7002
#define MXLV_ROENTEC_RCL_COMMAND_WITH_RESPONSE	7003

#define MXD_ROENTEC_RCL_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof( MX_ROENTEC_RCL_MCA, rs232_record ), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {MXLV_ROENTEC_RCL_COMMAND, -1, "command", MXFT_STRING,\
				NULL, 1, {MXU_ROENTEC_RCL_MAX_COMMAND_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ROENTEC_RCL_MCA, command), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_ROENTEC_RCL_RESPONSE, -1, "response", MXFT_STRING, \
				NULL, 1, {MXU_ROENTEC_RCL_MAX_COMMAND_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ROENTEC_RCL_MCA, response), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_ROENTEC_RCL_COMMAND_WITH_RESPONSE, -1, \
  			"command_with_response", MXFT_STRING,\
			NULL, 1, {MXU_ROENTEC_RCL_MAX_COMMAND_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ROENTEC_RCL_MCA, command), \
	{sizeof(char)}, NULL, 0}

MX_API mx_status_type mxd_roentec_rcl_initialize_type( long type );
MX_API mx_status_type mxd_roentec_rcl_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxd_roentec_rcl_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_roentec_rcl_delete_record( MX_RECORD *record );
MX_API mx_status_type mxd_roentec_rcl_open( MX_RECORD *record );
MX_API mx_status_type mxd_roentec_rcl_resynchronize( MX_RECORD *record );
MX_API mx_status_type mxd_roentec_rcl_special_processing_setup(
							MX_RECORD *record );

MX_API mx_status_type mxd_roentec_rcl_start( MX_MCA *mca );
MX_API mx_status_type mxd_roentec_rcl_stop( MX_MCA *mca );
MX_API mx_status_type mxd_roentec_rcl_read( MX_MCA *mca );
MX_API mx_status_type mxd_roentec_rcl_clear( MX_MCA *mca );
MX_API mx_status_type mxd_roentec_rcl_busy( MX_MCA *mca );
MX_API mx_status_type mxd_roentec_rcl_get_parameter( MX_MCA *mca );
MX_API mx_status_type mxd_roentec_rcl_set_parameter( MX_MCA *mca );

MX_API mx_status_type mxd_roentec_rcl_command(
					MX_ROENTEC_RCL_MCA *roentec_rcl_mca,
					char *command,
					char *response,
					size_t max_response_length,
					int transfer_flags );

MX_API mx_status_type mxd_roentec_rcl_read_32bit_array(
					MX_ROENTEC_RCL_MCA *roentec_rcl_mca,
					size_t num_32bit_values,
					uint32_t *value_array,
					int transfer_flags );

extern MX_RECORD_FUNCTION_LIST mxd_roentec_rcl_record_function_list;
extern MX_MCA_FUNCTION_LIST mxd_roentec_rcl_mca_function_list;

extern mx_length_type mxd_roentec_rcl_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_roentec_rcl_rfield_def_ptr;

#endif /* __D_ROENTEC_RCL_MCA_H__ */
