/*
 * Name:    d_sis3801.h
 *
 * Purpose: Header for MX MCS driver for the Struck SIS 3801.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2001-2002, 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SIS3801_H__
#define __D_SIS3801_H__

#define MX_SIS3801_NUM_COUNTERS		32

#define MX_SIS3801_10MHZ_INTERNAL_CLOCK	(1.0e7)		/* 10 MHz */

/* SIS3801 register addresses. */

#define MX_SIS3801_STATUS_REG				0x000
#define MX_SIS3801_CONTROL_REG				0x000
#define MX_SIS3801_MODULE_ID_IRQ_CONTROL_REG		0x004
#define MX_SIS3801_COPY_DISABLE_REG			0x00C
#define MX_SIS3801_WRITE_TO_FIFO			0x010
#define MX_SIS3801_CLEAR_FIFO_REG			0x020
#define MX_SIS3801_SEND_NEXT_CLOCK			0x024
#define MX_SIS3801_ENABLE_NEXT_CLOCK_LOGIC		0x028
#define MX_SIS3801_DISABLE_NEXT_CLOCK_LOGIC		0x02C
#define MX_SIS3801_ENABLE_REFERENCE_PULSE_CHANNEL_1	0x050
#define MX_SIS3801_DISABLE_REFERENCE_PULSE_CHANNEL_1	0x054
#define MX_SIS3801_RESET_REG				0x060
#define MX_SIS3801_TEST_PULSE				0x068
#define MX_SIS3801_PRESCALE_FACTOR_REG			0x080
#define MX_SIS3801_READ_FIFO				0x100

/* SIS3801 status register bits. */

#define MXF_SIS3801_FIFO_FLAG_EMPTY			0x00000100
#define MXF_SIS3801_FIFO_FLAG_FULL			0x00001000

/* SIS3801 control register bits. */

#define MXF_SIS3801_ENABLE_10MHZ_TO_LNE_PRESCALER	0x00000040
#define MXF_SIS3801_DISABLE_10MHZ_TO_LNE_PRESCALER	0x00004000

#define MXF_SIS3801_ENABLE_LNE_PRESCALER		0x00000080
#define MXF_SIS3801_DISABLE_LNE_PRESCALER		0x00008000

#define MXF_SIS3801_ENABLE_EXTERNAL_NEXT		0x00010000
#define MXF_SIS3801_DISABLE_EXTERNAL_NEXT		0x01000000

#define MXF_SIS3801_SET_SOFTWARE_DISABLE_COUNTING_BIT	0x00080000
#define MXF_SIS3801_CLEAR_SOFTWARE_DISABLE_COUNTING_BIT	0x08000000

/* SIS3801 flag values. */

#define MXF_SIS3801_USE_REFERENCE_PULSER		0x2

typedef struct {
	MX_RECORD *record;

	MX_RECORD *vme_record;
	char address_mode_name[ MXU_VME_ADDRESS_MODE_LENGTH + 1 ];
	unsigned long crate_number;
	unsigned long address_mode;
	unsigned long base_address;

	unsigned long sis3801_flags;
	unsigned long control_input_mode;

	unsigned long module_id;
	unsigned long firmware_version;

	unsigned long maximum_prescale_factor;

	int fifo_size_in_kwords;		/* A word is 16 bits long */
	int counts_available_in_fifo;

	MX_CLOCK_TICK finish_time;
} MX_SIS3801;

#define MXD_SIS3801_STANDARD_FIELDS \
  {-1, -1, "vme_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3801, vme_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "address_mode_name", MXFT_STRING, NULL, \
				1, {MXU_VME_ADDRESS_MODE_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3801, address_mode_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "crate_number", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3801, crate_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "base_address", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3801, base_address), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "sis3801_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3801, sis3801_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "control_input_mode", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3801, control_input_mode), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "module_id", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3801, module_id), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "firmware_version", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3801, firmware_version), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "maximum_prescale_factor", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3801, maximum_prescale_factor), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "fifo_size_in_kwords", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3801, fifo_size_in_kwords), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "counts_available_in_fifo", MXFT_INT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3801, counts_available_in_fifo), \
	{0}, NULL, MXFF_READ_ONLY }

MX_API mx_status_type mxd_sis3801_initialize_type( long type );
MX_API mx_status_type mxd_sis3801_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_sis3801_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_sis3801_open( MX_RECORD *record );
MX_API mx_status_type mxd_sis3801_close( MX_RECORD *record );

MX_API mx_status_type mxd_sis3801_start( MX_MCS *mcs );
MX_API mx_status_type mxd_sis3801_stop( MX_MCS *mcs );
MX_API mx_status_type mxd_sis3801_clear( MX_MCS *mcs );
MX_API mx_status_type mxd_sis3801_busy( MX_MCS *mcs );
MX_API mx_status_type mxd_sis3801_read_all( MX_MCS *mcs );
MX_API mx_status_type mxd_sis3801_read_scaler( MX_MCS *mcs );
MX_API mx_status_type mxd_sis3801_read_measurement( MX_MCS *mcs );
MX_API mx_status_type mxd_sis3801_get_parameter( MX_MCS *mcs );
MX_API mx_status_type mxd_sis3801_set_parameter( MX_MCS *mcs );

extern MX_RECORD_FUNCTION_LIST mxd_sis3801_record_function_list;
extern MX_MCS_FUNCTION_LIST mxd_sis3801_mcs_function_list;

extern mx_length_type mxd_sis3801_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_sis3801_rfield_def_ptr;

#endif /* __D_SIS3801_H__ */
