/*
 * Name:    i_sis3807.h
 *
 * Purpose: Header for the MX interface driver for the Struck SIS 3807
 *          pulse generator module.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2002, 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_SIS3807_H__
#define __I_SIS3807_H__

#define MX_SIS3807_NUM_PULSE_GENERATORS		4

#define MX_SIS3807_MAXIMUM_PRESET		0xffffff

/* SIS3807 register addresses. */

#define MX_SIS3807_STATUS_REG			0x000
#define MX_SIS3807_CONTROL_REG			0x000
#define MX_SIS3807_MODULE_ID_REG		0x004
#define MX_SIS3807_OUTPUT_INVERT_REG		0x008
#define MX_SIS3807_PULSE_CHANNEL_1		0x020
#define MX_SIS3807_PULSE_CHANNEL_2		0x024
#define MX_SIS3807_PULSE_CHANNEL_3		0x028
#define MX_SIS3807_PULSE_CHANNEL_4		0x02C

#define MX_SIS3807_PRESET_FACTOR_PULSER_1	0x040	/* V1 and V2 */
#define MX_SIS3807_PRESET_FACTOR_PULSER_2	0x044	/* V1 and V2 */
#define MX_SIS3807_PRESET_FACTOR_PULSER_3	0x048	/* V1 and V2 */
#define MX_SIS3807_PRESET_FACTOR_PULSER_4	0x04C	/* V1 and V2 */

#define MX_SIS3807_FREQUENCY_REGISTER		0x040	/* V3 */
#define MX_SIS3807_BURST_REGISTER		0x044	/* V3 */
#define MX_SIS3807_PULSE_WIDTH			0x048	/* V3 */

#define MX_SIS3807_RESET_REG			0x060

/* SIS3807 control register bits. */

#define MXF_SIS3807_GENERAL_ENABLE_BIT				0x2
#define MXF_SIS3807_SET_2_CHANNEL_MODE_BIT			0x4
#define MXF_SIS3807_ENABLE_COMMON_FACTOR_8_PREDIVIDER_BIT	0x8

/* SIS3807 status register bits. */

#define MXF_SIS3807_STATUS_ENABLE_PULSER_1			0x10

typedef struct {
	MX_RECORD *record;

	MX_RECORD *vme_record;
	char address_mode_name[ MXU_VME_ADDRESS_MODE_LENGTH + 1 ];
	unsigned long crate_number;
	unsigned long address_mode;
	unsigned long base_address;

	long          num_channels;
	unsigned long output_invert_register;
	mx_bool_type  enable_factor_8_predivider;

	unsigned long module_id;
	unsigned long firmware_version;

	mx_bool_type burst_register_available;
	mx_bool_type pulse_width_available;
} MX_SIS3807;

#define MXI_SIS3807_STANDARD_FIELDS \
  {-1, -1, "vme_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3807, vme_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "address_mode_name", MXFT_STRING, NULL, \
				1, {MXU_VME_ADDRESS_MODE_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3807, address_mode_name), \
	{sizeof(char)}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "crate_number", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3807, crate_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "base_address", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3807, base_address), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "num_channels", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3807, num_channels), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "output_invert_register", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3807, output_invert_register), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "enable_factor_8_predivider", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3807, enable_factor_8_predivider), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "module_id", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3807, module_id), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "firmware_version", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3807, firmware_version), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "burst_register_available", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3807, burst_register_available), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "pulse_width_available", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3807, pulse_width_available), \
	{0}, NULL, MXFF_READ_ONLY }


MX_API mx_status_type mxi_sis3807_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxi_sis3807_open( MX_RECORD *record );
MX_API mx_status_type mxi_sis3807_close( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_sis3807_record_function_list;

extern long mxi_sis3807_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_sis3807_rfield_def_ptr;

#endif /* __I_SIS3807_H__ */
