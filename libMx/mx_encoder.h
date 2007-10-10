/*
 * Name:    mx_encoder.h
 *
 * Purpose: MX system header file for encoders.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_ENCODER_H__
#define __MX_ENCODER_H__

#include "mx_record.h"

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	MX_RECORD *record;
	int encoder_type;
	int overflow_set;
	int underflow_set;
	long value;
} MX_ENCODER;

#define MX_ENCODER_STANDARD_FIELDS \
  {-1, -1, "value", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_ENCODER, value), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

/* Encoder type */

#define MXT_ENC_ABSOLUTE_ENCODER	1
#define MXT_ENC_INCREMENTAL_ENCODER	2

/*
 * The structure type MX_ENCODER_FUNCTION_LIST contains a list of pointers
 * to functions that vary from encoder type to encoder type.
 */

typedef struct {
	mx_status_type ( *get_overflow_status ) ( MX_ENCODER *encoder );
	mx_status_type ( *reset_overflow_status ) ( MX_ENCODER *encoder );
	mx_status_type ( *read ) ( MX_ENCODER *encoder );
	mx_status_type ( *write ) ( MX_ENCODER *encoder );
} MX_ENCODER_FUNCTION_LIST;

MX_API mx_status_type mx_encoder_get_pointers( MX_RECORD *record,
					MX_ENCODER **encoder,
					MX_ENCODER_FUNCTION_LIST **fl_ptr,
					const char *calling_fname );

MX_API mx_status_type mx_encoder_get_overflow_status( MX_RECORD *record,
				int *underflow_set, int *overflow_set );
MX_API mx_status_type mx_encoder_reset_overflow_status( MX_RECORD *record );
MX_API mx_status_type mx_encoder_read( MX_RECORD *record, long *value );
MX_API mx_status_type mx_encoder_write( MX_RECORD *record, long value );

#ifdef __cplusplus
}
#endif

#endif /* __MX_ENCODER_H__ */
