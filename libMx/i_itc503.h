/*
 * Name:    i_itc503.h
 *
 * Purpose: Header file for Oxford Instruments ITC503 temperature controllers.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2008 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __I_ITC503_H__
#define __I_ITC503_H__

/* Values for 'itc503_flags' below */

#define MXF_ITC503_ENABLE_REMOTE_MODE	0x1
#define MXF_ITC503_UNLOCK		0x2

/* The two lowest order bits in 'itc503_flags' are used to
 * construct a 'Cn' control command.  The 'Cn' determines whether
 * or not the controller is in LOCAL or REMOTE mode and also
 * whether or not the LOC/REM button is locked or active.  The
 * possible values for the 'Cn' command are:
 * 
 * C0 - Local and locked (default state)
 * C1 - Remote and locked (front panel disabled)
 * C2 - Local and unlocked
 * C3 - Remote and unlocked (front panel disabled)
 */

typedef struct {
	MX_RECORD *record;

	MX_RECORD *isobus_record;
	long isobus_address;

	unsigned long itc503_flags;

	long maximum_retries;

	/* The 'label' below is used in messages to the user.  Currently,
	 * it either has the value 'ITC503' or 'Cryojet'.
	 */

	char label[10];
} MX_ITC503;

#define MXI_ITC503_STANDARD_FIELDS \
  {-1, -1, "isobus_record", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_ITC503, isobus_record), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "isobus_address", MXFT_LONG, NULL, 0, {0}, \
  	MXF_REC_TYPE_STRUCT, offsetof(MX_ITC503, isobus_address), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}, \
  \
  {-1, -1, "itc503_flags", MXFT_ULONG, NULL, 0, {0}, \
  	MXF_REC_TYPE_STRUCT, offsetof(MX_ITC503, itc503_flags), \
	{0}, NULL, MXFF_IN_DESCRIPTION}, \
  \
  {-1, -1, "maximum_retries", MXFT_LONG, NULL, 0, {0}, \
  	MXF_REC_TYPE_STRUCT, offsetof(MX_ITC503, maximum_retries), \
	{0}, NULL, MXFF_IN_DESCRIPTION}

MX_API mx_status_type mxi_itc503_create_record_structures( MX_RECORD *record );

MX_API mx_status_type mxi_itc503_open( MX_RECORD *record );

extern MX_RECORD_FUNCTION_LIST mxi_itc503_record_function_list;

extern long mxi_itc503_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxi_itc503_rfield_def_ptr;

#endif /* __I_ITC503_H__ */

