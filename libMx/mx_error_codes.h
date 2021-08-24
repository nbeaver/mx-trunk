/*
 * Name:    mx_error_codes.h
 *
 * Purpose: Define standard error codes for MX and related packages.
 *
 * Author:  William Lavender
 *
 *---------------------------------------------------------------------
 *
 * Copyright 1999-2021 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_ERROR_CODES_H__
#define __MX_ERROR_CODES_H__

#ifdef __cplusplus
extern "C" {
#endif

#define MXE_SUCCESS				1000	/* No error. */

#define MXE_NULL_ARGUMENT			1001
#define MXE_ILLEGAL_ARGUMENT			1002
#define MXE_CORRUPT_DATA_STRUCTURE		1003
#define MXE_UNPARSEABLE_STRING			1004
#define MXE_END_OF_DATA				1005
#define MXE_UNEXPECTED_END_OF_DATA		1006
#define MXE_NOT_FOUND				1007
#define MXE_TYPE_MISMATCH			1008
#define MXE_NOT_YET_IMPLEMENTED			1009
#define MXE_UNSUPPORTED				1010
#define MXE_OUT_OF_MEMORY			1011
#define MXE_WOULD_EXCEED_LIMIT			1012
#define MXE_LIMIT_WAS_EXCEEDED			1013
#define MXE_INTERFACE_IO_ERROR			1014
#define MXE_DEVICE_IO_ERROR			1015
#define MXE_FILE_IO_ERROR			1016
#define MXE_TERMINAL_IO_ERROR			1017
#define MXE_IPC_IO_ERROR			1018
#define MXE_IPC_CONNECTION_LOST			1019
#define MXE_NETWORK_IO_ERROR			1020
#define MXE_NETWORK_CONNECTION_LOST		1021
#define MXE_NOT_READY				1022
#define MXE_INTERRUPTED				1023
#define MXE_PAUSE_REQUESTED			1024
#define MXE_INTERFACE_ACTION_FAILED		1025
#define MXE_DEVICE_ACTION_FAILED		1026
#define MXE_FUNCTION_FAILED			1027
#define MXE_CONTROLLER_INTERNAL_ERROR		1028
#define MXE_PERMISSION_DENIED			1029
#define MXE_CLIENT_REQUEST_DENIED		1030
#define MXE_TRY_AGAIN				1031
#define MXE_TIMED_OUT				1032
#define MXE_HARDWARE_CONFIGURATION_ERROR	1033
#define MXE_HARDWARE_FAULT			1034
#define MXE_RECORD_DISABLED_DUE_TO_FAULT	1035
#define MXE_RECORD_DISABLED_BY_USER		1036
#define MXE_INITIALIZATION_ERROR		1037
#define MXE_READ_ONLY				1038
#define MXE_SOFTWARE_CONFIGURATION_ERROR	1039
#define MXE_OPERATING_SYSTEM_ERROR		1040
#define MXE_UNKNOWN_ERROR			1041
#define MXE_NOT_VALID_FOR_CURRENT_STATE		1042
#define MXE_CONFIGURATION_CONFLICT		1043
#define MXE_NOT_AVAILABLE			1044
#define MXE_STOP_REQUESTED			1045
#define MXE_BAD_HANDLE				1046
#define MXE_OBJECT_ABANDONED			1047
#define MXE_MIGHT_CAUSE_DEADLOCK		1048
#define MXE_ALREADY_EXISTS			1049
#define MXE_INVALID_CALLBACK			1050
#define MXE_EARLY_EXIT				1051
#define MXE_PROTOCOL_ERROR			1052
#define MXE_DISK_FULL				1053
#define MXE_DATA_WAS_LOST			1054
#define MXE_NETWORK_CONNECTION_REFUSED		1055
#define MXE_CALLBACK_IN_PROGRESS		1056
#define MXE_ILLEGAL_COMMAND			1057

/* If the error message code is OR-ed with MXE_QUIET, then
 * the error message is not displayed to the user.
 */

#define MXE_QUIET				0x8000

#ifdef __cplusplus
}
#endif

#endif /* __MX_ERROR_CODES_H__ */
