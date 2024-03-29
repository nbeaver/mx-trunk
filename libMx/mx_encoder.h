/*
 * Name:    mx_encoder.h
 *
 * Purpose: MX system header file for encoders.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999-2001, 2007, 2023 Illinois Institute of Technology
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

/* Flag bits for the 'status' field. */

#define MXSF_ENC_MOVING		0x1
#define MXSF_ENC_UNDERFLOW	0x2
#define MXSF_ENC_OVERFLOW	0x4

typedef struct {
	MX_RECORD *record;

	double value;
	double measurement_time;

	long encoder_type;
	mx_bool_type reset;
	unsigned long status;
} MX_ENCODER;

#define MX_ENCODER_STANDARD_FIELDS \
  {-1, -1, "value", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_ENCODER, value), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "measurement_time", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_ENCODER, measurement_time), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "encoder_type", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_ENCODER, encoder_type), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "reset", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_ENCODER, reset), \
	{0}, NULL, 0 }, \
  \
  {-1, -1, "status", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_ENCODER, status), \
	{0}, NULL, 0 }

/* Encoder type */

#define MXT_ENC_ABSOLUTE_ENCODER	1
#define MXT_ENC_INCREMENTAL_ENCODER	2

/*
 * The structure type MX_ENCODER_FUNCTION_LIST contains a list of pointers
 * to functions that vary from encoder type to encoder type.
 */

typedef struct {
	mx_status_type ( *read ) ( MX_ENCODER *encoder );
	mx_status_type ( *write ) ( MX_ENCODER *encoder );
	mx_status_type ( *reset ) ( MX_ENCODER *encoder );
	mx_status_type ( *get_status ) ( MX_ENCODER *encoder );
	mx_status_type ( *get_measurement_time ) ( MX_ENCODER *encoder );
	mx_status_type ( *set_measurement_time ) ( MX_ENCODER *encoder );
} MX_ENCODER_FUNCTION_LIST;

MX_API mx_status_type mx_encoder_get_pointers( MX_RECORD *record,
					MX_ENCODER **encoder,
					MX_ENCODER_FUNCTION_LIST **fl_ptr,
					const char *calling_fname );

MX_API mx_status_type mx_encoder_read( MX_RECORD *record, double *value );
MX_API mx_status_type mx_encoder_write( MX_RECORD *record, double value );
MX_API mx_status_type mx_encoder_reset( MX_RECORD *record );
MX_API mx_status_type mx_encoder_get_status( MX_RECORD *record,
						unsigned long *status );
MX_API mx_status_type mx_encoder_get_measurement_time( MX_RECORD *record,
						double *measurement_time );
MX_API mx_status_type mx_encoder_set_measurement_time( MX_RECORD *record,
						double measurement_time );

#ifdef __cplusplus
}
#endif

#endif /* __MX_ENCODER_H__ */
