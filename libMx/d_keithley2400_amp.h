/*
 * Name:    d_keithley2400_amp.h
 *
 * Purpose: Header for the MX amplifier driver for controlling the
 *          range of a Keithley 2400 multimeter.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2005-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_KEITHLEY2400_AMP_H__
#define __D_KEITHLEY2400_AMP_H__

typedef struct {
	MX_RECORD *record;

	MX_RECORD *controller_record;
	int32_t io_type;
} MX_KEITHLEY2400_AMP;

/* Values for the 'io_type' field. */

#define MXT_KEITHLEY2400_AMP_INPUT	1
#define MXT_KEITHLEY2400_AMP_OUTPUT	2

#define MXD_KEITHLEY2400_AMP_STANDARD_FIELDS \
  {-1, -1, "controller_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_KEITHLEY2400_AMP, controller_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "io_type", MXFT_INT32, NULL, 0, {0}, \
        MXF_REC_TYPE_STRUCT, offsetof(MX_KEITHLEY2400_AMP, io_type), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxd_keithley2400_amp_create_record_structures(
							MX_RECORD *record );

MX_API mx_status_type mxd_keithley2400_amp_get_gain( MX_AMPLIFIER *amplifier );
MX_API mx_status_type mxd_keithley2400_amp_set_gain( MX_AMPLIFIER *amplifier );

extern MX_RECORD_FUNCTION_LIST mxd_keithley2400_amp_record_function_list;

extern MX_AMPLIFIER_FUNCTION_LIST mxd_keithley2400_amp_amplifier_function_list;

extern mx_length_type mxd_keithley2400_amp_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_keithley2400_amp_rfield_def_ptr;

#endif /* __D_KEITHLEY2400_AMP_H__ */
