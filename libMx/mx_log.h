/*
 * Name:     mx_log.h
 *
 * Purpose:  Header file to describe logfile support.
 *
 * Author:   William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 1999-2000, 2007 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_LOG_H__
#define __MX_LOG_H__

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	MX_LIST_HEAD *list_head;

	int  log_type;
	char log_name[ MXU_FILENAME_LENGTH + 1 ];
} MX_LOG;

/* A 1-D string variable record with the following name must exist
 * in the database for logging to start.
 */

#define MX_LOG_RECORD_NAME	"mx_log"

/* Log types */

#define MXLT_FILE	1

MX_API mx_status_type mx_log_open( MX_RECORD *record_list );

MX_API mx_status_type mx_log_close( MX_RECORD *record_list );

MX_API mx_status_type mx_log_message( MX_LOG *log_handler, char *message );

MX_API char *mx_log_timestamp( MX_LOG *log_handler,
				char *buffer, size_t buffer_length );

#ifdef __cplusplus
}
#endif

#endif /* __MX_LOG_H__ */

