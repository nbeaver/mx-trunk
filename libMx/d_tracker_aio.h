/*
 * Name:    d_tracker_aio.h
 *
 * Purpose: Header file for drivers to control Data Track Tracker locations
 *          as analog I/O ports.
 *
 * Author:  William Lavender
 *
 *-------------------------------------------------------------------------
 *
 * Copyright 2004 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_TRACKER_AIO_H__
#define __D_TRACKER_AIO_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;
	int address;
	int location;
} MX_TRACKER_AINPUT;

typedef struct {
	MX_RECORD *record;

	MX_RECORD *rs232_record;
	int address;
	int location;
} MX_TRACKER_AOUTPUT;

#define MXD_TRACKER_AINPUT_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TRACKER_AINPUT, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "address", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TRACKER_AINPUT, address), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "location", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TRACKER_AINPUT, location), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

#define MXD_TRACKER_AOUTPUT_STANDARD_FIELDS \
  {-1, -1, "rs232_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TRACKER_AOUTPUT, rs232_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "address", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TRACKER_AOUTPUT, address), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "location", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_TRACKER_AOUTPUT, location), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

/* Define all of the interface functions. */

/* First the input functions. */

MX_API mx_status_type mxd_tracker_ain_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_tracker_ain_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_tracker_ain_read( MX_ANALOG_INPUT *dinput );

extern MX_RECORD_FUNCTION_LIST mxd_tracker_ain_record_function_list;
extern MX_ANALOG_INPUT_FUNCTION_LIST
				mxd_tracker_ain_analog_input_function_list;

extern long mxd_tracker_ain_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_tracker_ain_rfield_def_ptr;

/* Second the output functions. */

MX_API mx_status_type mxd_tracker_aout_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_tracker_aout_finish_record_initialization(
							MX_RECORD *record );

MX_API mx_status_type mxd_tracker_aout_read( MX_ANALOG_OUTPUT *doutput );
MX_API mx_status_type mxd_tracker_aout_write( MX_ANALOG_OUTPUT *doutput );

extern MX_RECORD_FUNCTION_LIST mxd_tracker_aout_record_function_list;
extern MX_ANALOG_OUTPUT_FUNCTION_LIST
				mxd_tracker_aout_analog_output_function_list;

extern long mxd_tracker_aout_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_tracker_aout_rfield_def_ptr;

/*---*/

MX_API mx_status_type mxd_tracker_command( MX_RECORD *record,
		                        MX_RECORD *rs232_record,
		                        char *command,
		                        char *response,
		                        size_t max_response_length,
		                        int debug_flag );

#endif /* __D_TRACKER_AIO_H__ */

