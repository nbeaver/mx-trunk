/*
 * Name:    v_epics_timeout.h
 *
 * Purpose: Header file for specifying the EPICS PV connection timeout
 *          for MX programs.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------------
 *
 * Copyright 2012 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef _V_EPICS_TIMEOUT_H_
#define _V_EPICS_TIMEOUT_H_

typedef struct {
	MX_RECORD *record;
} MX_EPICS_TIMEOUT;

MX_API_PRIVATE mx_status_type mxv_epics_timeout_create_record_structures(
							MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxv_epics_timeout_open( MX_RECORD *record );

MX_API_PRIVATE mx_status_type mxv_epics_timeout_send_variable(
						MX_VARIABLE *variable );

MX_API_PRIVATE mx_status_type mxv_epics_timeout_receive_variable(
						MX_VARIABLE *variable );

extern MX_RECORD_FUNCTION_LIST mxv_epics_timeout_record_function_list;
extern MX_VARIABLE_FUNCTION_LIST mxv_epics_timeout_variable_function_list;

extern long mxv_epics_timeout_num_record_fields;
extern MX_RECORD_FIELD_DEFAULTS *mxv_epics_timeout_field_def_ptr;

#endif /* _V_EPICS_TIMEOUT_H_ */

