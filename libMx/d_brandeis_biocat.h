/*
 * Name:    d_brandeis_biocat.h
 *
 * Purpose: MX driver header for the Brandeis detector at the BioCAT sector
 *          of the Advanced Photon Source.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __D_BRANDEIS_BIOCAT_H__
#define __D_BRANDEIS_BIOCAT_H__

typedef struct {
	MX_RECORD *record;

	long dummy;
} MX_BRANDEIS_BIOCAT;

#define MXD_BRANDEIS_BIOCAT_STANDARD_FIELDS \
  {-1, -1, "dummy", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_TYPE_STRUCT, offsetof(MX_BRANDEIS_BIOCAT, dummy), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY)}

MX_API mx_status_type mxd_brandeis_biocat_initialize_type( long record_type );
MX_API mx_status_type mxd_brandeis_biocat_create_record_structures(
							MX_RECORD *record );
MX_API mx_status_type mxd_brandeis_biocat_finish_record_initialization(
							MX_RECORD *record );
MX_API mx_status_type mxd_brandeis_biocat_open( MX_RECORD *record );
MX_API mx_status_type mxd_brandeis_biocat_close( MX_RECORD *record );
MX_API mx_status_type mxd_brandeis_biocat_resynchronize( MX_RECORD *record );

MX_API mx_status_type mxd_brandeis_biocat_arm( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_brandeis_biocat_trigger( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_brandeis_biocat_stop( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_brandeis_biocat_abort( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_brandeis_biocat_get_extended_status(
						MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_brandeis_biocat_readout_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_brandeis_biocat_correct_frame( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_brandeis_biocat_get_parameter( MX_AREA_DETECTOR *ad );
MX_API mx_status_type mxd_brandeis_biocat_set_parameter( MX_AREA_DETECTOR *ad );

extern MX_RECORD_FUNCTION_LIST mxd_brandeis_biocat_record_function_list;
extern MX_AREA_DETECTOR_FUNCTION_LIST mxd_brandeis_biocat_ad_function_list;

extern long mxd_brandeis_biocat_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxd_brandeis_biocat_rfield_def_ptr;

#endif /* __D_BRANDEIS_BIOCAT_H__ */

