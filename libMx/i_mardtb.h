/*
 * Name:    i_mardtb.h
 *
 * Purpose: Header for the MX interface driver for the MarUSA Desktop Beamline
 *          goniostat controller.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2002, 2004-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_MARDTB_H__
#define __I_MARDTB_H__

#define MXU_MARDTB_USERNAME_LENGTH	8	/* This is a guess. */

#define MX_MARDTB_NUM_CHARS_LIMIT	500

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;
	char username[ MXU_MARDTB_USERNAME_LENGTH + 1 ];
	unsigned long mardtb_flags;

	MX_RECORD *currently_active_record;
	unsigned long last_clock_value;
} MX_MARDTB;

/* Define flag values for the 'mardtb_flags' field. */

#define MXF_MARDTB_FORCE_STATUS_UPDATE		0x1
#define MXF_MARDTB_CONDITIONAL_STATUS_UPDATE	0x2
#define MXF_MARDTB_THREE_PARAMETER_STATUS_DUMP	0x1000

/* Define known parameter numbers */

#define MXF_MARDTB_CLOCK_PARAMETER		138

#define MXI_MARDTB_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MARDTB, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "username", MXFT_STRING, NULL, 1, {MXU_MARDTB_USERNAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MARDTB, username), \
	{sizeof(char)}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "mardtb_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_MARDTB, mardtb_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

MX_API mx_status_type mxi_mardtb_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxi_mardtb_open( MX_RECORD *record );

MX_API mx_status_type mxi_mardtb_command( MX_MARDTB *mardtb, char *command,
				char *response, size_t response_buffer_length,
				int debug_flag );

MX_API mx_status_type mxi_mardtb_check_for_move_in_progress( MX_MARDTB *mardtb,
							int *move_in_progress );

MX_API mx_status_type mxi_mardtb_force_status_update( MX_MARDTB *mardtb );

MX_API mx_status_type mxi_mardtb_raw_read_status_parameter( MX_MARDTB *mardtb,
						int parameter_number,
						unsigned long *parameter_value);

MX_API mx_status_type mxi_mardtb_read_status_parameter( MX_MARDTB *mardtb,
						int parameter_number,
						unsigned long *parameter_value);

extern MX_RECORD_FUNCTION_LIST mxi_mardtb_record_function_list;

extern mx_length_type mxi_mardtb_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_mardtb_rfield_def_ptr;

#endif /* __I_MARDTB_H__ */
