/*
 * Name:   i_vsc16.h
 *
 * Purpose: Header for MX driver for Joerger VSC16 and VSC8 counter/timers.
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

#ifndef __I_VSC16_H__
#define __I_VSC16_H__

#define MX_MAX_VSC16_CHANNELS	16

/* Register definitions. */

#define MX_VSC16_RESET_MODULE		0x00
#define MX_VSC16_CONTROL_REGISTER	0x04
#define MX_VSC16_DIRECTION_REGISTER	0x08
#define MX_VSC16_STATUS_ID_REGISTER	0x10
#define MX_VSC16_IRQ_LEVEL_EN_REGISTER	0x14
#define MX_VSC16_IRQ_MASK_REGISTER	0x18
#define MX_VSC16_IRQ_RESET		0x1C
#define MX_VSC16_SERIAL_NUMBER_REGISTER	0x20
#define MX_VSC16_MODULE_TYPE		0x24
#define MX_VSC16_MANUFACTURER_ID	0x28

#define MX_VSC16_READ_BASE		0x80
#define MX_VSC16_READ_RESET_BASE	0xC0
#define MX_VSC16_PRESET_BASE		0xC0

/* Module type flags. */

#define MXF_VSC16_NIM			0x01
#define MXF_VSC16_8_CHANNEL		0x08
#define MXF_VSC16_16_CHANNEL		0x10

typedef struct {
	MX_RECORD *record;

	MX_RECORD *vme_record;
	unsigned long crate_number;
	unsigned long base_address;

	unsigned long num_counters;
	MX_RECORD *counter_record[MX_MAX_VSC16_CHANNELS];

	unsigned char module_type;
	unsigned char serial_number;
} MX_VSC16;

#define MXI_VSC16_STANDARD_FIELDS \
  {-1, -1, "vme_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_VSC16, vme_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "crate_number", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_VSC16, crate_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "base_address", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_VSC16, base_address), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxi_vsc16_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxi_vsc16_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxi_vsc16_open( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_vsc16_record_function_list;

extern mx_length_type mxi_vsc16_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_vsc16_rfield_def_ptr;

/* === Driver specific functions === */

#endif /* __I_VSC16_H__ */
