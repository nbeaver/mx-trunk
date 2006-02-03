/*
 * Name:   i_uglide.h
 *
 * Purpose: Header for MX driver for interface to the BCW u-GLIDE
 *          micropositioning stage from Oceaneering Space Systems.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2002, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_UGLIDE_H__
#define __I_UGLIDE_H__

/* Values for last_response_type.  last_response_type stores the character
 * after the '#' character in the most recent response line read from
 * the controller.
 */

#define MXF_UGLIDE_MOVE_COMPLETE		'e'
#define MXF_UGLIDE_WARNING_MESSAGE		'm'
#define MXF_UGLIDE_OK				'o'
#define MXF_UGLIDE_MOVE_IN_PROGRESS		's'
#define MXF_UGLIDE_MODE_REPORT			't'
#define MXF_UGLIDE_POSITION_REPORT		'x'

/* Values for uglide_flags. */

#define MXF_UGLIDE_USE_RELATIVE_MODE		0x1
#define MXF_UGLIDE_BYPASS_HOME_SEARCH_ON_BOOT	0x2

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;
	unsigned long uglide_flags;

	char last_response_code;

	int x_position;
	int y_position;

	MX_RECORD *x_motor_record;
	MX_RECORD *y_motor_record;
} MX_UGLIDE;

#define MXI_UGLIDE_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_UGLIDE, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "uglide_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_UGLIDE, uglide_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxi_uglide_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxi_uglide_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_uglide_open( MX_RECORD *record );
MX_API mx_status_type mxi_uglide_resynchronize( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_uglide_record_function_list;

extern mx_length_type mxi_uglide_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_uglide_rfield_def_ptr;

/* === Driver specific functions === */

MX_API mx_status_type mxi_uglide_command( MX_UGLIDE *uglide,
						char *command,
						int debug_flag );

MX_API mx_status_type mxi_uglide_getline( MX_UGLIDE *uglide,
						char *response,
						size_t max_response_length,
						int debug_flag );

#endif /* __I_UGLIDE_H__ */
