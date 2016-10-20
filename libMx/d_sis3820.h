/*
 * Name:    d_sis3820.h
 *
 * Purpose: Header for MX MCS driver for the Struck SIS 3820.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2016 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SIS3820_H__
#define __D_SIS3820_H__

#define MX_SIS3820_NUM_COUNTERS		32

#define MX_SIS3820_10MHZ_INTERNAL_CLOCK	(1.0e7)		/* 10 MHz */

/* SIS3820 register addresses. */

#define MX_SIS3820_CONTROL_STATUS_REG			0x000
#define MX_SIS3820_MODID_REG				0x004
#define MX_SIS3820_IRQ_CONFIG_REG			0x008
#define MX_SIS3820_IRQ_CONTROL_REG			0x00C

#define MX_SIS3820_ACQUISITION_PRESET_REG		0x010
#define MX_SIS3820_ACQUISITION_COUNT_REG		0x014
#define MX_SIS3820_LNE_PRESCALE_REG			0x018

#define MX_SIS3820_PRESET_GROUP1_REG			0x020
#define MX_SIS3820_PRESET_GROUP2_REG			0x024
#define MX_SIS3820_PRESET_ENABLE_HIT_REG		0x028

#define MX_SIS3820_CBLT_BROADCAST_SETUP_REG		0x030
#define MX_SIS3820_SDRAM_PAGE_REG			0x034
#define MX_SIS3820_FIFO_WORDCOUNTER_REG			0x038
#define MX_SIS3820_FIFO_WORDCOUNT_THRESHOLD_REG		0x03C

#define MX_SIS3820_HISCAL_START_PRESET_REG		0x040
#define MX_SIS3820_HISCAL_COUNT_REG			0x044
#define MX_SIS3820_HISCAL_LAST_ACQ_COUNT_REG		0x048

#define MX_SIS3820_ENCODER_POSITION_COUNTER_REG		0x050
#define MX_SIS3820_ENCODER_ERROR_COUNTER_REG		0x054
#define MX_SIS3820_CIP_LNE_DELAY_REG			0x058
#define MX_SIS3820_CIP_LNE_WIDTH_REG			0x05C

#define MX_SIS3820_OPERATION_MODE_REG			0x100
#define MX_SIS3820_COPY_DISABLE_REG			0x104
#define MX_SIS3820_LNE_CHANNEL_SELECT_REG		0x108
#define MX_SIS3820_PRESET_CHANNEL_SELECT_REG		0x10C

#define MX_SIS3820_MUX_OUT_CHANNEL_SELECT_REG		0x110

#define MX_SIS3820_COUNTER_INHIBIT_REG			0x200
#define MX_SIS3820_COUNTER_CLEAR_REG			0x204
#define MX_SIS3820_COUNTER_OVERFLOW_REG			0x208

#define MX_SIS3820_CHANNEL_1_17_BITS_33_48_REG		0x210
#define MX_SIS3820_VETO_EXTERNAL_COUNT_INHIBIT_REG	0x214
#define MX_SIS3820_TEST_PULSE_MASK_REG			0x218

#define MX_SIS3820_SDRAM_EEPROM_CTRL_STAT_REG		0x300

#define MX_SIS3820_JTAG_TEST_DATA_IN_REG		0x310
#define MX_SIS3820_JTAG_CONTROL_REG			0x314

#define MX_SIS3820_KEY_RESET_REG			0x400
#define MX_SIS3820_KEY_SDRAM_FIFO_RESET_REG		0x404
#define MX_SIS3820_KEY_TEST_PULS_REG			0x408
#define MX_SIS3820_KEY_COUNTER_CLEAR_REG		0x40C

#define MX_SIS3820_KEY_LNE_PULS_REG			0x410
#define MX_SIS3820_KEY_OPERATION_ARM_REG		0x414
#define MX_SIS3820_KEY_OPERATION_ENABLE_REG		0x418
#define MX_SIS3820_KEY_OPERATION_DISABLE_REG		0x41C

#define MX_SIS3820_KEY_HISCAL_START_PULS_REG		0x420
#define MX_SIS3820_KEY_HISCAL_ENABLE_LNE_ARM_REG	0x424
#define MX_SIS3820_KEY_HISCAL_ENABLE_LNE_ENABLE_REG	0x428
#define MX_SIS3820_KEY_HISCAL_DISABLE_REG		0x42C

#define MX_SIS3820_KEY_COUNTER_SHADOW_CH1_REG		0x800

#define MX_SIS3820_KEY_COUNTER_CH1_REG			0xA00

#define MX_SIS3820_FIFO_BASE_REG			0x800000

#define MX_SIS3820_SDRAM_BASE_REG			0x800000

/* SIS3820 status register bits. */

#define MXF_SIS3820_STAT_USER_LED_ON			0x1

#define MXF_SIS3820_STAT_COUNTER_TEST_25MHZ_ENABLE	0x10
#define MXF_SIS3820_STAT_COUNTER_TEST_MODE_ENABLE	0x20
#define MXF_SIS3820_STAT_REFERENCE_CH1_ENABLE		0x40

#define MXF_SIS3820_STAT_OPERATION_SCALER_ENABLED	0x10000
#define MXF_SIS3820_STAT_OPERATION_MCS_ENABLED		0x40000
#define MXF_SIS3820_STAT_OPERATION_VME_WRITE_ENABLED	0x8000000

/* SIS3820 control register bits. */

#define MXF_SIS3820_CTRL_USER_LED_ON			0x1

#define MXF_SIS3820_CTRL_COUNTER_TEST_25MHZ_ENABLE	0x10
#define MXF_SIS3820_CTRL_COUNTER_TEST_MODE_ENABLE	0x20
#define MXF_SIS3820_CTRL_REFERENCE_CH1_ENABLE		0x40

#define MXF_SIS3820_CTRL_USER_LED_OFF			0x10000

#define MXF_SIS3820_CTRL_COUNTER_TEST_25MHZ_DISABLE	0x100000
#define MXF_SIS3820_CTRL_COUNTER_TEST_MODE_DISABLE	0x200000
#define MXF_SIS3820_CTRL_REFERENCE_CH1_DISABLE		0x400000

/* SIS3820 Preset Enable and Hit register (0x28) (read) */

#define MXF_SIS3820_PRESET_STATUS_ENABLE_GROUP1		0x1
#define MXF_SIS3820_PRESET_REACHED_GROUP1		0x2
#define MXF_SIS3820_PRESET_LNELATCHED_REACHED_GROUP1	0x4

#define MXF_SIS3820_PRESET_STATUS_ENABLE_GROUP2		0x10000
#define MXF_SIS3820_PRESET_REACHED_GROUP2		0x20000
#define MXF_SIS3820_PRESET_LNELATCHED_REACHED_GROUP2	0x40000

/* SIS3820 Preset Enable and Hit register (0x28) (write) */

#define MXF_SIS3820_PRESET_ENABLE_GROUP1		0x1

#define MXF_SIS3820_PRESET_ENABLE_GROUP2		0x10000

/* SIS3820 Preset Enable and Hit register (0x28) (EEPROM support ?) */

#define MXF_SIS3820_SDRAM_EEPROM_SCL			0x1
#define MXF_SIS3820_SDRAM_EEPROM_SDA_OUT		0x2
#define MXF_SIS3820_SDRAM_EEPROM_SDA_OE			0x4

#define MXF_SIS3820_SDRAM_EEPROM_SDA_IN			0x100

/* SIS3820 Acquisition [ Operation Mode register (0x100) ]  */

#define MXF_SIS3820_CLEARING_MODE			0x0
#define MXF_SIS3820_NON_CLEARING_MODE			0x1

#define MXF_SIS3820_MCS_DATA_FORMAT_32BIT		0x0
#define MXF_SIS3820_MCS_DATA_FORMAT_24BIT		0x4
#define MXF_SIS3820_MCS_DATA_FORMAT_16BIT		0x8
#define MXF_SIS3820_MCS_DATA_FORMAT_8BIT		0xC

#define MXF_SIS3820_SCALER_DATA_FORMAT_32BIT		0x0
#define MXF_SIS3820_SCALER_DATA_FORMAT_24BIT		0x4

#define MXF_SIS3820_LNE_SOURCE_VME			0x0
#define MXF_SIS3820_LNE_SOURCE_CONTROL_SIGNAL		0x10
#define MXF_SIS3820_LNE_SOURCE_INTERNAL_10MHZ		0x20
#define MXF_SIS3820_LNE_SOURCE_CHANNEL_N		0x30
#define MXF_SIS3820_LNE_SOURCE_PRESET			0x40

#define MXF_SIS3820_ARM_ENABLE_CONTROL_SIGNAL		0x000
#define MXF_SIS3820_ARM_ENABLE_CHANNEL_N		0x100

#define MXF_SIS3820_FIFO_MODE				0x0000
#define MXF_SIS3820_SDRAM_MODE				0x1000
#define MXF_SIS3820_SDRAM_ADD_MODE			0x2000

#define MXF_SIS3820_HISCAL_START_SOURCE_VME		0x0000
#define MXF_SIS3820_HISCAL_START_SOURCE_EXTERN		0x4000

#define MXF_SIS3820_CONTROL_INPUT_MODE0			0x00000
#define MXF_SIS3820_CONTROL_INPUT_MODE1			0x10000
#define MXF_SIS3820_CONTROL_INPUT_MODE2			0x20000
#define MXF_SIS3820_CONTROL_INPUT_MODE3			0x30000
#define MXF_SIS3820_CONTROL_INPUT_MODE4			0x40000
#define MXF_SIS3820_CONTROL_INPUT_MODE5			0x50000

#define MXF_SIS3820_CONTROL_INPUTS_INVERT		0x80000

#define MXF_SIS3820_CONTROL_OUTPUT_MODE0		0x000000
#define MXF_SIS3820_CONTROL_OUTPUT_MODE1		0x100000

#define MXF_SIS3820_CONTROL_OUTPUTS_INVERT		0x800000

#define MXF_SIS3820_OP_MODE_SCALER			0x00000000
#define MXF_SIS3820_OP_MODE_MULTI_CHANNEL_SCALER	0x20000000
#define MXF_SIS3820_OP_MODE_VME_FIFO_WRITE		0x70000000


/* MX SIS3820 flag values. */

#define MXF_SIS3820_USE_REFERENCE_PULSER		0x2

typedef struct {
	MX_RECORD *record;

	MX_RECORD *vme_record;
	char address_mode_name[ MXU_VME_ADDRESS_MODE_LENGTH + 1 ];
	unsigned long crate_number;
	unsigned long address_mode;
	unsigned long base_address;

	unsigned long sis3820_flags;
	unsigned long control_input_mode;

	unsigned long module_id;
	unsigned long firmware_version;

	unsigned long disabled_interrupts;

	unsigned long maximum_prescale_factor;

	long fifo_size_in_kwords;		/* A word is 16 bits long */
	mx_bool_type counts_available_in_fifo;

	mx_bool_type new_start;

	uint32_t *measurement_buffer;

	mx_bool_type use_callback;
	MX_CALLBACK_MESSAGE *callback_message;
} MX_SIS3820;

#define MXD_SIS3820_STANDARD_FIELDS \
  {-1, -1, "vme_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3820, vme_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "address_mode_name", MXFT_STRING, NULL, \
				1, {MXU_VME_ADDRESS_MODE_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3820, address_mode_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "crate_number", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3820, crate_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "base_address", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3820, base_address), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "sis3820_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3820, sis3820_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "control_input_mode", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3820, control_input_mode), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "module_id", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3820, module_id), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "firmware_version", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3820, firmware_version), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "maximum_prescale_factor", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3820, maximum_prescale_factor), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "fifo_size_in_kwords", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3820, fifo_size_in_kwords), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "counts_available_in_fifo", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3820, counts_available_in_fifo), \
	{0}, NULL, MXFF_READ_ONLY }

MX_API mx_status_type mxd_sis3820_initialize_driver( MX_DRIVER *driver );
MX_API mx_status_type mxd_sis3820_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_sis3820_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_sis3820_open( MX_RECORD *record );
MX_API mx_status_type mxd_sis3820_close( MX_RECORD *record );

MX_API mx_status_type mxd_sis3820_start( MX_MCS *mcs );
MX_API mx_status_type mxd_sis3820_stop( MX_MCS *mcs );
MX_API mx_status_type mxd_sis3820_clear( MX_MCS *mcs );
MX_API mx_status_type mxd_sis3820_busy( MX_MCS *mcs );
MX_API mx_status_type mxd_sis3820_get_parameter( MX_MCS *mcs );
MX_API mx_status_type mxd_sis3820_set_parameter( MX_MCS *mcs );

extern MX_RECORD_FUNCTION_LIST mxd_sis3820_record_function_list;
extern MX_MCS_FUNCTION_LIST mxd_sis3820_mcs_function_list;

extern long mxd_sis3820_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_sis3820_rfield_def_ptr;

#endif /* __D_SIS3820_H__ */
