/*
 * Name:    i_dg645.h
 *
 * Purpose: Header file for Stanford Research Systems DG645 Digital
 *          Delay Generators.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2017-2018 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_DG645_H__
#define __I_DG645_H__

/* Values for the 'dg645_flags' field. */

#define MXF_DG645_DEBUG		0x1

/*---*/

#define MXU_DG645_STRING_LENGTH		20

#define MXU_DG645_NUM_OUTPUTS		5

/* FIXME: The trigger settings in mx_image.h should be moved into a
 * different header so that we don't end up defining them again here.
 */

#define MXF_DG645_INTERNAL_TRIGGER	0x1
#define MXF_DG645_EXTERNAL_TRIGGER	0x2
#define MXF_DG645_LINE_TRIGGER		0x10

typedef struct {
	MX_RECORD *record;

	MX_INTERFACE dg645_interface;
	unsigned long dg645_flags;
	long instrument_settings;
	double output_voltage[MXU_DG645_NUM_OUTPUTS];

	unsigned long serial_number;
	unsigned long firmware_version;

	mx_bool_type save_settings;
	mx_bool_type recall_settings;

	double trigger_level;
	double trigger_rate;
	unsigned long trigger_source;

	unsigned long trigger_type;
	long trigger_direction;
	mx_bool_type single_shot;

	unsigned long last_error;

	unsigned long event_status_register;
	unsigned long instrument_status_register;
	unsigned long operation_complete;
	unsigned long status_byte;

	mx_bool_type status_update_succeeded;
} MX_DG645;


#define MXLV_DG645_OUTPUT_VOLTAGE		83000
#define MXLV_DG645_SAVE_SETTINGS		83001
#define MXLV_DG645_RECALL_SETTINGS		83002
#define MXLV_DG645_TRIGGER_LEVEL		83003
#define MXLV_DG645_TRIGGER_RATE			83004
#define MXLV_DG645_TRIGGER_SOURCE		83005
#define MXLV_DG645_STATUS			83006

#define MXI_DG645_STANDARD_FIELDS \
  {-1, -1, "dg645_interface", MXFT_INTERFACE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DG645, dg645_interface), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "dg645_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DG645, dg645_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "instrument_settings", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DG645, instrument_settings), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {MXLV_DG645_OUTPUT_VOLTAGE, -1, "output_voltage", \
		MXFT_DOUBLE, NULL, 1, {MXU_DG645_NUM_OUTPUTS}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DG645, output_voltage), \
	{sizeof(double)}, NULL, 0}, \
  \
  {-1, -1, "serial_number", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DG645, serial_number), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "firmware_version", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DG645, firmware_version), \
	{0}, NULL, 0}, \
  \
  {MXLV_DG645_SAVE_SETTINGS, -1, "save_settings", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DG645, save_settings), \
	{0}, NULL, 0}, \
  \
  {MXLV_DG645_RECALL_SETTINGS, -1, "recall_settings", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DG645, recall_settings), \
	{0}, NULL, 0}, \
  \
  {MXLV_DG645_TRIGGER_LEVEL, -1, "trigger_level", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DG645, trigger_level), \
	{0}, NULL, 0}, \
  \
  {MXLV_DG645_TRIGGER_RATE, -1, "trigger_rate", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DG645, trigger_rate), \
	{0}, NULL, 0}, \
  \
  {MXLV_DG645_TRIGGER_SOURCE, -1, "trigger_source", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DG645, trigger_source), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "trigger_type", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DG645, trigger_type), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "trigger_direction", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DG645, trigger_direction), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "single_shot", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DG645, single_shot), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "last_error", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DG645, last_error), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "event_status_register", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DG645, event_status_register), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "instrument_status_register", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DG645, instrument_status_register), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "operation_complete", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DG645, operation_complete), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "status_byte", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DG645, status_byte), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {MXLV_DG645_STATUS, -1, "status_update_succeeded", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_DG645, status_update_succeeded), \
	{0}, NULL, MXFF_READ_ONLY}
	

MX_API mx_status_type mxi_dg645_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxi_dg645_open( MX_RECORD *record );
MX_API mx_status_type mxi_dg645_special_processing_setup( MX_RECORD *record );

MX_API mx_status_type mxi_dg645_command( MX_DG645 *dg645,
					char *command,
					char *response,
					size_t max_response_length,
					unsigned long dg645_flags );

MX_API mx_status_type mxi_dg645_compute_delay_between_channels(
					MX_DG645 *dg645,
					unsigned long original_channel,
					unsigned long requested_channel,
					double *requested_delay,
					unsigned long *adjacent_channel,
					double *adjacent_delay );

MX_API mx_status_type mxi_dg645_get_status( MX_DG645 *dg645 );

MX_API mx_status_type mxi_dg645_setup_pulser_trigger_mode( MX_DG645 *dg645,
							long new_trigger_mode );

MX_API mx_status_type mxi_dg645_interpret_trigger_source( MX_DG645 *dg645 );

extern MX_RECORD_FUNCTION_LIST mxi_dg645_record_function_list;

extern long mxi_dg645_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_dg645_rfield_def_ptr;

#endif /* __I_DG645_H__ */

