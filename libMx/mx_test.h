/*
 * Name:    mx_test.h
 *
 * Purpose: Defines some routines commonly used for testing MX.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------
 *
 * Copyright 2002, 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_TEST_H__
#define __MX_TEST_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

MX_API mx_status_type mx_test_setup_outputs( MX_RECORD *record_list );

MX_API mx_status_type mx_test_output_on( int output_number );

MX_API mx_status_type mx_test_output_off( int output_number );

MX_API mx_status_type mx_test_pulse( int output_number,
				unsigned long pulse_length_in_microseconds );

MX_API mx_status_type mx_test_dip( int output_number,
				unsigned long dip_length_in_microseconds );

#ifdef __cplusplus
}
#endif

#endif /* __MX_TEST_H__ */
