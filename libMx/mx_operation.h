/*
 * Name:    mx_operation.h
 *
 * Purpose: Header file for operation superclass support.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 2013-2014 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef _MX_OPERATION_H_
#define _MX_OPERATION_H_

#include "mx_util.h"
#include "mx_record.h"

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

/* Values for operation_flags. */

#define MXF_OP_AUTOSTART	0x1

/*---*/

typedef struct {
	MX_RECORD *record;	/* Pointer to this operation's MX_RECORD. */

	unsigned long operation_flags;

	unsigned long status;

	mx_bool_type start;
	mx_bool_type stop;

	mx_bool_type operation_running;
} MX_OPERATION;

typedef struct {
	mx_status_type ( *get_status ) ( MX_OPERATION * );
	mx_status_type ( *start ) ( MX_OPERATION * );
	mx_status_type ( *stop ) ( MX_OPERATION * );
} MX_OPERATION_FUNCTION_LIST;

/*---*/

MX_API mx_status_type mx_operation_get_status( MX_RECORD *op_record,
						unsigned long *op_status );

MX_API mx_status_type mx_operation_start( MX_RECORD *op_record );

MX_API mx_status_type mx_operation_stop( MX_RECORD *op_record );

#define MXLV_OP_STATUS		37001
#define MXLV_OP_START		37002
#define MXLV_OP_STOP		37003


#define MX_OPERATION_STANDARD_FIELDS \
  {-1, -1, "operation_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof( MX_OPERATION, operation_flags ), \
	{0}, NULL, MXFF_IN_DESCRIPTION }, \
  \
  {MXLV_OP_STATUS, -1, "status", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof( MX_OPERATION, status ), \
	{0}, NULL, MXFF_IN_SUMMARY }, \
  \
  {MXLV_OP_START, -1, "start", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof( MX_OPERATION, start ), \
	{0}, NULL, 0 }, \
  \
  {MXLV_OP_STOP, -1, "stop", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_SUPERCLASS_STRUCT, offsetof( MX_OPERATION, stop ), \
	{0}, NULL, 0 }

#ifdef __cplusplus
}
#endif

#endif /* _MX_OPERATION_H_ */

