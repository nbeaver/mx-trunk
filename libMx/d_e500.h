/*
 * Name:    d_e500.h 
 *
 * Purpose: Header file for MX motor driver to control E500A CAMAC stepping 
 *          motor controllers.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2006, 2010, 2013 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_E500_H__
#define __D_E500_H__

#include "mx_motor.h"
#include "mx_camac.h"

/* Define all of the MX interface functions. */

MX_API mx_status_type mxd_e500_create_record_structures( MX_RECORD *record );
MX_API mx_status_type mxd_e500_finish_record_initialization(MX_RECORD *record);
MX_API mx_status_type mxd_e500_print_motor_structure(
					FILE *file, MX_RECORD *record );
MX_API mx_status_type mxd_e500_open( MX_RECORD *record );
MX_API mx_status_type mxd_e500_close( MX_RECORD *record );

MX_API mx_status_type mxd_e500_motor_is_busy( MX_MOTOR *motor );
MX_API mx_status_type mxd_e500_move_absolute( MX_MOTOR *motor );
MX_API mx_status_type mxd_e500_get_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_e500_set_position( MX_MOTOR *motor );
MX_API mx_status_type mxd_e500_soft_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_e500_immediate_abort( MX_MOTOR *motor );
MX_API mx_status_type mxd_e500_positive_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_e500_negative_limit_hit( MX_MOTOR *motor );
MX_API mx_status_type mxd_e500_raw_home_command( MX_MOTOR *motor );

extern MX_RECORD_FUNCTION_LIST mxd_e500_record_function_list;
extern MX_MOTOR_FUNCTION_LIST mxd_e500_motor_function_list;

#ifndef MX_MOTORS_PER_E500
#define MX_MOTORS_PER_E500 8	/* Eight motors per E500 module. */
#endif

/* ===== E500 motor data structures ===== */

typedef struct {
	MX_RECORD      *camac_record;
	long           slot;
	long           subaddress;
	unsigned short e500_base_speed;
	unsigned long  e500_slew_speed;
	unsigned short acceleration_time;
	unsigned short correction_limit;
	long           lam_mask;
} MX_E500;

extern long mxd_e500_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_e500_rfield_def_ptr;

#define MXD_E500_STANDARD_FIELDS \
  {-1, -1, "camac_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_E500, camac_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "slot", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_E500, slot), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "subaddress", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_E500, subaddress), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "e500_base_speed", MXFT_USHORT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_E500, e500_base_speed), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "e500_slew_speed", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_E500, e500_slew_speed), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "acceleration_time", MXFT_USHORT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_E500, acceleration_time), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "correction_limit", MXFT_USHORT, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_E500, correction_limit), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "lam_mask", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_E500, lam_mask), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

/* ===== E500 C preprocessor definitions ===== */

/* Define bit masks for the low order byte of the command and status reg. */

/* Bits for reading. */

#define MX_E500_CCW_LIMIT  		0x80
#define MX_E500_CW_LIMIT   		0x40
#define MX_E500_ACCUMULATOR_OVERFLOW	0x20	/* read & write */
#define MX_E500_ACCUMULATOR_UNDERFLOW	0x10	/* read & write */
#define MX_E500_CORRECTION_FAILURE	0x08
#define MX_E500_MOTOR_BUSY		0x04
#define MX_E500_ILLEGAL_INSTRUCTION	0x02
#define MX_E500_LAM_MASK		0x01	/* read & write */

/* Other bits for writing. */

#define MX_E500_IMMEDIATE_ABORT		0x80
#define MX_E500_SOFT_ABORT		0x40
#define MX_E500_BUILD_FILE		0x04
#define MX_E500_MOTOR_START		0x02

/* Bitmask to use when updating the command and status word.
 * This mask preserves the correction limit and the lam mask.
 * The programmed base rate must also be fetched using
 * mx_e500_read_baserate() in order to form a new CSR.
 */

#define MX_E500_UPDATE_MASK		0x00FF01

/* ===== Low level E500 functions ===== */

/* -- There are two commands that affect an E500 module as a whole. -- */

#if 0

MX_API mx_status_type mx_e500_simultaneous_start(
					MX_E500_MODULE *e500_module );

MX_API mx_status_type mx_e500_read_module_id( MX_E500_MODULE *e500_module,
				uint16_t *id, uint8_t *revision );
#endif

/* -- All the rest of the command only affect individual motors. -- */

MX_API mx_status_type mx_e500_preserve_csr_bitmap( MX_E500 *e500,
					int32_t *csr );

MX_API mx_status_type mx_e500_accumulator_overflow( MX_E500 *e500,
					int *overflow);

MX_API mx_status_type mx_e500_accumulator_overflow_reset( MX_E500 *e500 );

MX_API mx_status_type mx_e500_accumulator_underflow( MX_E500 *e500,
					int *underflow );

MX_API mx_status_type mx_e500_accumulator_underflow_reset( MX_E500 *e500 );

MX_API mx_status_type mx_e500_ccw_limit( MX_E500 *e500, int *limit_hit );

MX_API mx_status_type mx_e500_cw_limit( MX_E500 *e500, int *limit_hit );

MX_API mx_status_type mx_e500_build_file( MX_E500 *e500 );

MX_API mx_status_type mx_e500_correction_failure( MX_E500 *e500,
					int *correction_failure );

MX_API mx_status_type mx_e500_illegal_instruction( MX_E500 *e500,
						int *illegal_instruction );

MX_API mx_status_type mx_e500_immediate_abort( MX_E500 *e500 );

MX_API mx_status_type mx_e500_motor_busy( MX_E500 *e500, int *busy );

MX_API mx_status_type mx_e500_motor_start( MX_E500 *e500 );

MX_API mx_status_type mx_e500_soft_abort( MX_E500 *e500 );

MX_API mx_status_type mx_e500_go_to_home( MX_E500 *e500 );

MX_API mx_status_type mx_e500_move_absolute( MX_E500 *e500,
					int32_t steps );

MX_API mx_status_type mx_e500_move_relative( MX_E500 *e500,
					int32_t steps );

MX_API mx_status_type mx_e500_read_absolute_accumulator( MX_E500 *e500,
					int32_t *accumulator );

MX_API mx_status_type mx_e500_read_baserate( MX_E500 *e500,
					uint16_t *actual_baserate );

MX_API mx_status_type mx_e500_read_command_status_reg( MX_E500 *e500,
					int32_t *csr );

MX_API mx_status_type mx_e500_read_correction_limit( MX_E500 *e500 );

MX_API mx_status_type mx_e500_read_lam_mask( MX_E500 *e500 );

MX_API mx_status_type mx_e500_read_lam_status( MX_E500 *e500,
					int *lam_status );

MX_API mx_status_type mx_e500_read_pulse_parameter_reg( MX_E500 *e500 );

MX_API mx_status_type mx_e500_read_relative_magnitude( MX_E500 *e500,
					int32_t *relative_magnitude );

MX_API mx_status_type mx_e500_write_absolute_accumulator( MX_E500 *e500,
					int32_t accumulator );

MX_API mx_status_type mx_e500_write_baserate( MX_E500 *e500 );

MX_API mx_status_type mx_e500_write_command_status_reg( MX_E500 *e500,
					int32_t csr );

MX_API mx_status_type mx_e500_write_correction_limit( MX_E500 *e500 );

MX_API mx_status_type mx_e500_write_lam_mask( MX_E500 *e500 );

MX_API mx_status_type mx_e500_write_pulse_parameter_reg( MX_E500 *e500 );

MX_API mx_status_type mx_e500_write_relative_magnitude( MX_E500 *e500,
					int32_t relative_magnitude );

#endif /* __D_E500_H__ */

