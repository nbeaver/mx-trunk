/*
 * Name:    d_sis3801_pulser.h
 *
 * Purpose: Header for an MX pulse generator driver for the Struck SIS 3801.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2002, 2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_SIS3801_PULSER_H__
#define __D_SIS3801_PULSER_H__

/* A variety of SIS3801 specific macros are defined in "d_sis3801.h", so we
 * include that here.
 */

#include "d_sis3801.h"

/* Now define the pulse generator specific structures. */

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

	MX_CLOCK_TICK finish_time;
} MX_SIS3801_PULSER;

#define MXD_SIS3801_PULSER_STANDARD_FIELDS \
  {-1, -1, "vme_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3801_PULSER, vme_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "address_mode_name", MXFT_STRING, NULL, \
				1, {MXU_VME_ADDRESS_MODE_LENGTH}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3801_PULSER, address_mode_name), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "crate_number", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3801_PULSER, crate_number), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "base_address", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3801_PULSER, base_address), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "sis3801_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3801_PULSER, sis3801_flags), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "control_input_mode", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3801_PULSER, control_input_mode), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "module_id", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3801_PULSER, module_id), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "firmware_version", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_SIS3801_PULSER, firmware_version), \
	{0}, NULL, MXFF_READ_ONLY }, \
  \
  {-1, -1, "maximum_prescale_factor", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, \
		offsetof(MX_SIS3801_PULSER, maximum_prescale_factor), \
	{0}, NULL, MXFF_READ_ONLY }

MX_API mx_status_type mxd_sis3801_pulser_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_sis3801_pulser_open( MX_RECORD *record );

MX_API mx_status_type mxd_sis3801_pulser_busy(
					MX_PULSE_GENERATOR *pulse_generator );
MX_API mx_status_type mxd_sis3801_pulser_start(
					MX_PULSE_GENERATOR *pulse_generator );
MX_API mx_status_type mxd_sis3801_pulser_stop(
					MX_PULSE_GENERATOR *pulse_generator );
MX_API mx_status_type mxd_sis3801_pulser_get_parameter(
					MX_PULSE_GENERATOR *pulse_generator );
MX_API mx_status_type mxd_sis3801_pulser_set_parameter(
					MX_PULSE_GENERATOR *pulse_generator );

extern MX_RECORD_FUNCTION_LIST mxd_sis3801_pulser_record_function_list;
extern MX_PULSE_GENERATOR_FUNCTION_LIST 
		mxd_sis3801_pulser_pulse_generator_function_list;

extern mx_length_type mxd_sis3801_pulser_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_sis3801_pulser_rfield_def_ptr;

#endif /* __D_SIS3801_PULSER_H__ */
