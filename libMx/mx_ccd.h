/*
 * Name:    mx_ccd.h
 *
 * Purpose: MX system header file for CCD camera systems.
 *
 * Author:  William Lavender
 *
 * WARNING: The CCD class is obsolete and is scheduled to be replaced
 *          by the new 'area_detector' class.
 *
 *----------------------------------------------------------------------------
 *
 * Copyright 2003, 2006-2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_CCD_H__
#define __MX_CCD_H__

#include "mx_record.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MXU_CCD_HEADER_NAME_LENGTH		40

#define MXSF_CCD_ACQUIRING_FRAME		0x01
#define MXSF_CCD_READING_FRAME			0x02
#define MXSF_CCD_CORRECTING_FRAME		0x04
#define MXSF_CCD_WRITING_FRAME			0x08
#define MXSF_CCD_STOPPING			0x10

#define MXSF_CCD_WAITING_FOR_FINISH_TIME	0x100

#define MXSF_CCD_ERROR				0x8000000

/* Note that MXSF_CCD_ACQUIRING_FRAME is _not_ included in MXSF_CCD_IS_BUSY
 * since often the CCD does not know by itself when the acquisition should
 * be stopped.
 */

#define MXSF_CCD_IS_BUSY \
		( MXSF_CCD_READING_FRAME | MXSF_CCD_CORRECTING_FRAME \
		| MXSF_CCD_WRITING_FRAME | MXSF_CCD_STOPPING \
		| MXSF_CCD_WAITING_FOR_FINISH_TIME )

/* Several commands ask for flag values, so we define here the possible
 * flag values.  At the moment, they are somewhat MarCCD specific, but
 * this will probably change when other CCD brands are supported.
 */

#define MXF_CCD_FROM_RAW_DATA_FRAME	0x1
#define MXF_CCD_FROM_CORRECTED_FRAME	0x2
#define MXF_CCD_FROM_BACKGROUND_FRAME	0x4
#define MXF_CCD_FROM_SCRATCH_FRAME	0x8

#define MXF_CCD_TO_RAW_DATA_FRAME	0x100
#define MXF_CCD_TO_CORRECTED_FRAME	0x200
#define MXF_CCD_TO_BACKGROUND_FRAME	0x400
#define MXF_CCD_TO_SCRATCH_FRAME	0x800

typedef struct {
	MX_RECORD *record;

	long data_frame_size[2];
	long bin_size[2];
	unsigned long status;
	unsigned long ccd_flags;

	long parameter_type;

	double preset_time;

	mx_bool_type stop;
	mx_bool_type readout;
	mx_bool_type dezinger;
	mx_bool_type correct;
	mx_bool_type writefile;
	char writefile_name[MXU_FILENAME_LENGTH + 1];

	char header_variable_name[MXU_CCD_HEADER_NAME_LENGTH + 1];
	char header_variable_contents[MXU_BUFFER_LENGTH + 1];
} MX_CCD;

#define MXLV_CCD_DATA_FRAME_SIZE		17001
#define MXLV_CCD_BIN_SIZE			17002
#define MXLV_CCD_STATUS				17003
#define MXLV_CCD_CCD_FLAGS			17004
#define MXLV_CCD_PRESET_TIME			17005
#define MXLV_CCD_STOP				17006
#define MXLV_CCD_READOUT			17007
#define MXLV_CCD_DEZINGER			17008
#define MXLV_CCD_CORRECT			17009
#define MXLV_CCD_WRITEFILE			17010
#define MXLV_CCD_WRITEFILE_NAME			17011
#define MXLV_CCD_HEADER_VARIABLE_NAME		17012
#define MXLV_CCD_HEADER_VARIABLE_CONTENTS	17013

#define MX_CCD_STANDARD_FIELDS \
  {MXLV_CCD_DATA_FRAME_SIZE, -1, "data_frame_size", MXFT_LONG, NULL, 1, {2}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_CCD, data_frame_size), \
	{sizeof(int)}, NULL, 0 }, \
  \
  {MXLV_CCD_BIN_SIZE, -1, "bin_size", MXFT_LONG, NULL, 1, {2}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_CCD, bin_size), \
	{sizeof(int)}, NULL, 0 }, \
  \
  {MXLV_CCD_STATUS, -1, "status", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_CCD, status), \
	{0}, NULL, 0 }, \
  \
  {MXLV_CCD_CCD_FLAGS, -1, "ccd_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_CCD, ccd_flags), \
	{0}, NULL, 0 }, \
  \
  {MXLV_CCD_PRESET_TIME, -1, "preset_time", MXFT_DOUBLE, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_CCD, preset_time), \
	{0}, NULL, 0 }, \
  \
  {MXLV_CCD_STOP, -1, "stop", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_CCD, stop), \
	{0}, NULL, 0 }, \
  \
  {MXLV_CCD_READOUT, -1, "readout", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_CCD, readout), \
	{0}, NULL, 0 }, \
  \
  {MXLV_CCD_DEZINGER, -1, "dezinger", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_CCD, dezinger), \
	{0}, NULL, 0 }, \
  \
  {MXLV_CCD_CORRECT, -1, "correct", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_CCD, correct), \
	{0}, NULL, 0 }, \
  \
  {MXLV_CCD_WRITEFILE, -1, "writefile", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_CCD, writefile), \
	{0}, NULL, 0 }, \
  \
  {MXLV_CCD_WRITEFILE_NAME, -1, "writefile_name", \
	  		MXFT_STRING, NULL, 1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_CCD, writefile_name), \
	{sizeof(char)}, NULL, 0 }, \
  \
  {MXLV_CCD_HEADER_VARIABLE_NAME, -1, "header_variable_name", \
	  		MXFT_STRING, NULL, 1, {MXU_CCD_HEADER_NAME_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_CCD, header_variable_name), \
	{sizeof(char)}, NULL, 0 }, \
  \
  {MXLV_CCD_HEADER_VARIABLE_CONTENTS, -1, "header_variable_contents", \
	  		MXFT_STRING, NULL, 1, {MXU_BUFFER_LENGTH}, \
	MXF_REC_CLASS_STRUCT, offsetof(MX_CCD, header_variable_contents), \
	{sizeof(char)}, NULL, 0 }

typedef struct {
	mx_status_type ( *start ) ( MX_CCD *ccd );
	mx_status_type ( *stop ) ( MX_CCD *ccd );
	mx_status_type ( *get_status ) ( MX_CCD *ccd );
	mx_status_type ( *readout ) ( MX_CCD *ccd );
	mx_status_type ( *dezinger ) ( MX_CCD *ccd );
	mx_status_type ( *correct ) ( MX_CCD *ccd );
	mx_status_type ( *writefile ) ( MX_CCD *ccd );
	mx_status_type ( *get_parameter ) ( MX_CCD *ccd );
	mx_status_type ( *set_parameter ) ( MX_CCD *ccd );
} MX_CCD_FUNCTION_LIST;

MX_API_PRIVATE mx_status_type mx_ccd_get_pointers(
	MX_RECORD *ccd_record,
	MX_CCD **ccd,
	MX_CCD_FUNCTION_LIST **function_list_ptr,
	const char *calling_fname );

MX_API mx_status_type mx_ccd_start( MX_RECORD *record );

MX_API mx_status_type mx_ccd_start_for_preset_time( MX_RECORD *record,
						double time_in_seconds );

MX_API mx_status_type mx_ccd_stop( MX_RECORD *record );

MX_API mx_status_type mx_ccd_get_status( MX_RECORD *record,
					unsigned long *status );

MX_API mx_status_type mx_ccd_readout( MX_RECORD *record, unsigned long flags );

MX_API mx_status_type mx_ccd_dezinger( MX_RECORD *record, unsigned long flags );

MX_API mx_status_type mx_ccd_correct( MX_RECORD *record );

MX_API mx_status_type mx_ccd_writefile( MX_RECORD *record,
					char *filename,
					unsigned long flags );

MX_API mx_status_type mx_ccd_default_get_parameter_handler( MX_CCD *ccd );

MX_API mx_status_type mx_ccd_default_set_parameter_handler( MX_CCD *ccd );

MX_API mx_status_type mx_ccd_get_data_frame_size( MX_RECORD *record,
						long *x_size,
						long *y_size );

MX_API mx_status_type mx_ccd_get_bin_size( MX_RECORD *record,
						long *x_size,
						long *y_size );

MX_API mx_status_type mx_ccd_set_bin_size( MX_RECORD *record,
						long x_size,
						long y_size );

MX_API mx_status_type mx_ccd_read_header_variable( MX_RECORD *record,
						char *header_variable_name,
						char *header_variable_contents,
						size_t maximum_length );

MX_API mx_status_type mx_ccd_write_header_variable( MX_RECORD *record,
						char *header_variable_name,
						char *header_variable_contents);

#ifdef __cplusplus
}
#endif

#endif /* __MX_CCD_H__ */
