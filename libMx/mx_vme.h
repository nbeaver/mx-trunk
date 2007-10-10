/*
 * Name:    mx_vme.h
 *
 * Purpose: Header file for VME interfaces.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2001, 2003, 2006-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_VME_H__
#define __MX_VME_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

/* Addressing modes */

#define MXF_VME_A16	16
#define MXF_VME_A24	24
#define MXF_VME_A32	32

#define MXF_VME_A16_NAME  "A16"
#define MXF_VME_A24_NAME  "A24"
#define MXF_VME_A32_NAME  "A32"

#define MXU_VME_ADDRESS_MODE_LENGTH  4

/* Data sizes */

#define MXF_VME_D8	8
#define MXF_VME_D16	16
#define MXF_VME_D32	32

#define MXF_VME_D8_NAME   "D8"
#define MXF_VME_D16_NAME  "D16"
#define MXF_VME_D32_NAME  "D32"

#define MXU_VME_DATA_SIZE_LENGTH     4

/* Values for the 'vme_flags' field. */

#define MXF_VME_IGNORE_BUS_ERRORS	0x1
#define MXF_VME_SINGLE_STEP		0x2

typedef struct {
	MX_RECORD *record;

	unsigned long crate;
	unsigned long address;
	unsigned long address_mode;
	unsigned long data_size;

	unsigned long num_values;
	unsigned long read_address_increment;
	unsigned long write_address_increment;

	void *data_pointer;

	int parameter_type;

	unsigned long vme_flags;
} MX_VME;

#define MXLV_VME_READ_ADDRESS_INCREMENT		1001
#define MXLV_VME_WRITE_ADDRESS_INCREMENT	1002
#define MXLV_VME_FLAGS				1003

#define MX_VME_STANDARD_FIELDS \
  {MXLV_VME_FLAGS, -1, "vme_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_VME, vme_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

typedef struct {
	mx_status_type ( *input ) ( MX_VME *vme );
	mx_status_type ( *output ) ( MX_VME *vme );
	mx_status_type ( *multi_input ) ( MX_VME *vme );
	mx_status_type ( *multi_output ) ( MX_VME *vme );
	mx_status_type ( *get_parameter ) ( MX_VME *vme );
	mx_status_type ( *set_parameter ) ( MX_VME *vme );

} MX_VME_FUNCTION_LIST;

MX_API mx_status_type mx_vme_finish_record_initialization( MX_RECORD *record );

MX_API mx_status_type mx_vme_in8( MX_RECORD *record,
					unsigned long crate,
					unsigned long address_mode,
					unsigned long address,
					uint8_t *value );

MX_API mx_status_type mx_vme_in16( MX_RECORD *record,
					unsigned long crate,
					unsigned long address_mode,
					unsigned long address,
					uint16_t *value );

MX_API mx_status_type mx_vme_in32( MX_RECORD *record,
					unsigned long crate,
					unsigned long address_mode,
					unsigned long address,
					uint32_t *value );

MX_API mx_status_type mx_vme_out8( MX_RECORD *record,
					unsigned long crate,
					unsigned long address_mode,
					unsigned long address,
					uint8_t value );

MX_API mx_status_type mx_vme_out16( MX_RECORD *record,
					unsigned long crate,
					unsigned long address_mode,
					unsigned long address,
					uint16_t value );

MX_API mx_status_type mx_vme_out32( MX_RECORD *record,
					unsigned long crate,
					unsigned long address_mode,
					unsigned long address,
					uint32_t value );

MX_API mx_status_type mx_vme_multi_in8( MX_RECORD *record,
					unsigned long crate,
					unsigned long address_mode,
					unsigned long address_increment,
					unsigned long address,
					unsigned long num_values,
					uint8_t *value_array );

MX_API mx_status_type mx_vme_multi_in16( MX_RECORD *record,
					unsigned long crate,
					unsigned long address_mode,
					unsigned long address_increment,
					unsigned long address,
					unsigned long num_values,
					uint16_t *value_array );

MX_API mx_status_type mx_vme_multi_in32( MX_RECORD *record,
					unsigned long crate,
					unsigned long address_mode,
					unsigned long address_increment,
					unsigned long address,
					unsigned long num_values,
					uint32_t *value_array );

MX_API mx_status_type mx_vme_multi_out8( MX_RECORD *record,
					unsigned long crate,
					unsigned long address_mode,
					unsigned long address_increment,
					unsigned long address,
					unsigned long num_values,
					uint8_t *value_array );

MX_API mx_status_type mx_vme_multi_out16( MX_RECORD *record,
					unsigned long crate,
					unsigned long address_mode,
					unsigned long address_increment,
					unsigned long address,
					unsigned long num_values,
					uint16_t *value_array );

MX_API mx_status_type mx_vme_multi_out32( MX_RECORD *record,
					unsigned long crate,
					unsigned long address_mode,
					unsigned long address_increment,
					unsigned long address,
					unsigned long num_values,
					uint32_t *value_array );

MX_API mx_status_type mx_vme_get_read_address_increment( MX_RECORD *record,
					unsigned long crate,
					unsigned long *address_increment );

MX_API mx_status_type mx_vme_set_read_address_increment( MX_RECORD *record,
					unsigned long crate,
					unsigned long address_increment );

MX_API mx_status_type mx_vme_get_write_address_increment( MX_RECORD *record,
					unsigned long crate,
					unsigned long *address_increment );

MX_API mx_status_type mx_vme_set_write_address_increment( MX_RECORD *record,
					unsigned long crate,
					unsigned long address_increment );

MX_API mx_status_type mx_vme_parse_address_mode( const char *mode_name,
						unsigned long *address_mode );

MX_API mx_status_type mx_vme_parse_data_size( const char *size_name,
						unsigned long *data_size );

/* ============== Interface function prototypes ============= */

#ifdef __cplusplus
}
#endif

#endif /* __MX_VME_H__ */
