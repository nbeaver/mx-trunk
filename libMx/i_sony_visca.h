/*
 * Name:    i_sony_visca.h
 *
 * Purpose: MX header file for the Sony VISCA protocol.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2005 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_SONY_VISCA_H__
#define __I_SONY_VISCA_H__

#define MXU_SONY_VISCA_MAX_CMD_LENGTH	14

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;

	int num_cameras;
} MX_SONY_VISCA;

#define MXI_SONY_VISCA_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SONY_VISCA, rs232_record),\
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxi_sony_visca_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxi_sony_visca_open( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_sony_visca_record_function_list;

extern long mxi_sony_visca_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_sony_visca_rfield_def_ptr;

MX_API mx_status_type
mxi_sony_visca_copy( unsigned char *destination,
			unsigned char *source,
			size_t max_length );

MX_API mx_status_type
mxi_sony_visca_value_command( unsigned char *destination,
				size_t max_destination_length,
				size_t prefix_length,
				unsigned char *prefix,
				unsigned long value );

MX_API mx_status_type
mxi_sony_visca_handle_error( MX_SONY_VISCA *sony_visca,
				int error_type,
				char *sent_visca_ascii );

MX_API mx_status_type
mxi_sony_visca_cmd( MX_SONY_VISCA *sony_visca,
			int camera_number,
			unsigned char *command,
			unsigned char *response,
			size_t max_response_length,
			size_t *actual_response_length );

#define mxi_sony_visca_cmd_broadcast( s, c, r, m, a ) \
	mxi_sony_visca_cmd( (s), 8, (c), (r), (m), (a) )

#endif /* __I_SONY_VISCA_H__ */

