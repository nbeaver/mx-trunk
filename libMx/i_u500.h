/*
 * Name:   i_u500.h
 *
 * Purpose: Header for MX driver for Aerotech Unidex 500 controllers.
 *
 * Author:  William Lavender
 *
 *----------------------------------------------------------------------------
 *
 * Copyright 2004, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_U500_H__
#define __I_U500_H__

#if defined(HAVE_U500) && defined(OS_WIN32)
  /* Ignore old Win16 style defines. */
#  define far
#  define _huge
  /* This is needed for the Aerotech header files to correctly use _stdcall. */
#  ifndef WIN32
#     define WIN32
#  endif
#endif

/* Define the data structures used by a U500 controller. */

#define MX_U500_MAX_MOTORS	4

/* U500 board type */

#define MXT_U500_UNKNOWN	0xfff
#define MXT_U500_BASE		0x0
#define MXT_U500_PLUS		0x1
#define MXT_U500_ULTRA		0x2

#define MXT_U500_ISA		0x1000
#define MXT_U500_PCI		0x2000

#define MXU_U500_PROGRAM_NAME_LENGTH	80

typedef struct {
	MX_RECORD *record;
	mx_length_type num_boards;
	int32_t *board_type;
	char **firmware_filename;
	char **parameter_filename;
	char **calibration_filename;
	char **pso_firmware_filename;

	int16_t qlib_version[2];
	int16_t drv_version[2];
	int16_t wapi_version[2];

	char    program_name[ MXU_U500_PROGRAM_NAME_LENGTH+1 ];
	int32_t program_number;
	char    load_program[ MXU_FILENAME_LENGTH+1 ];
	int32_t unload_program;
	int32_t run_program;
	int32_t stop_program;
	int32_t fault_acknowledge;

	int32_t current_board_number;
} MX_U500;

#define MXLV_U500_PROGRAM_NAME		8000
#define MXLV_U500_PROGRAM_NUMBER	8001
#define MXLV_U500_LOAD_PROGRAM		8002
#define MXLV_U500_UNLOAD_PROGRAM	8003
#define MXLV_U500_RUN_PROGRAM		8004
#define MXLV_U500_STOP_PROGRAM		8005
#define MXLV_U500_FAULT_ACKNOWLEDGE	8006

#define MXI_U500_STANDARD_FIELDS \
  {-1, -1, "num_boards", MXFT_LENGTH, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_U500, num_boards), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "board_type", MXFT_INT32, NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_U500, board_type), \
	{sizeof(int)}, NULL, MXFF_VARARGS}, \
  \
  {-1, -1, "firmware_filename", MXFT_STRING, NULL, \
			2, {MXU_VARARGS_LENGTH, MXU_FILENAME_LENGTH+1}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_U500, firmware_filename), \
	{sizeof(char), sizeof(char *)}, NULL, \
		(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS)}, \
  \
  {-1, -1, "parameter_filename", MXFT_STRING, NULL, \
			2, {MXU_VARARGS_LENGTH, MXU_FILENAME_LENGTH+1}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_U500, parameter_filename), \
	{sizeof(char), sizeof(char *)}, NULL, \
		(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS)}, \
  \
  {-1, -1, "calibration_filename", MXFT_STRING, NULL, \
			2, {MXU_VARARGS_LENGTH, MXU_FILENAME_LENGTH+1}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_U500, calibration_filename), \
	{sizeof(char), sizeof(char *)}, NULL, \
		(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS)}, \
  \
  {-1, -1, "pso_firmware_filename", MXFT_STRING, NULL, \
			2, {MXU_VARARGS_LENGTH, MXU_FILENAME_LENGTH+1}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_U500, pso_firmware_filename), \
	{sizeof(char), sizeof(char *)}, NULL, \
		(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY | MXFF_VARARGS)}, \
  \
  {-1, -1, "qlib_version", MXFT_INT16, NULL, 1, {2}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_U500, qlib_version), \
	{sizeof(short)}, NULL, 0}, \
  \
  {-1, -1, "drv_version", MXFT_INT16, NULL, 1, {2}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_U500, drv_version), \
	{sizeof(short)}, NULL, 0}, \
  \
  {-1, -1, "wapi_version", MXFT_INT16, NULL, 1, {2}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_U500, wapi_version), \
	{sizeof(short)}, NULL, 0}, \
  \
  {MXLV_U500_PROGRAM_NAME, -1, "program_name", MXFT_STRING, NULL, \
				1, {MXU_U500_PROGRAM_NAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_U500, program_name), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_U500_PROGRAM_NUMBER, -1, "program_number", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_U500, program_number), \
	{0}, NULL, 0}, \
  \
  {MXLV_U500_LOAD_PROGRAM, -1, "load_program", MXFT_STRING, NULL, \
				1, {MXU_FILENAME_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_U500, load_program), \
	{sizeof(char)}, NULL, 0}, \
  \
  {MXLV_U500_UNLOAD_PROGRAM, -1, "unload_program", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_U500, unload_program), \
	{0}, NULL, 0}, \
  \
  {MXLV_U500_RUN_PROGRAM, -1, "run_program", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_U500, run_program), \
	{0}, NULL, 0}, \
  \
  {MXLV_U500_STOP_PROGRAM, -1, "stop_program", MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_U500, stop_program), \
	{0}, NULL, 0}, \
  \
  {MXLV_U500_FAULT_ACKNOWLEDGE, -1, "fault_acknowledge", \
					MXFT_INT32, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_U500, fault_acknowledge), \
	{0}, NULL, 0}

MX_API mx_status_type mxi_u500_initialize_type( long record_type );
MX_API mx_status_type mxi_u500_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxi_u500_open( MX_RECORD *record );
MX_API mx_status_type mxi_u500_close( MX_RECORD *record );
MX_API mx_status_type mxi_u500_special_processing_setup( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_u500_record_function_list;

extern mx_length_type mxi_u500_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_u500_rfield_def_ptr;

/* === Driver specific functions === */

MX_API mx_status_type mxi_u500_command( MX_U500*u500,
					int board_number,
					char *command );

MX_API mx_status_type mxi_u500_select_board( MX_U500 *u500,
					int board_number );

MX_API mx_status_type mxi_u500_set_program_number( MX_U500 *u500,
					int program_number );

MX_API mx_status_type mxi_u500_error( long wapi_status,
					const char *fname,
					char *format, ... );

MX_API mx_status_type mxi_u500_program_error( long wapi_status,
					const char *fname,
					char *format, ... );

MX_API mx_status_type mxi_u500_read_parameter( MX_U500 *u500,
					int parameter_number,
					double *parameter_value );

#endif /* __I_U500_H__ */

