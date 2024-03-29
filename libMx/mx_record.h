/*
 * Name:    mx_record.h
 *
 * Purpose: Header file to describe generic record support.
 *
 * Author:  William Lavender
 *
 *--------------------------------------------------------------------------
 *
 * Copyright 1999-2010, 2012-2022 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#ifndef __MX_RECORD_H__
#define __MX_RECORD_H__

#include <stddef.h>  /* Should get the definition of offsetof() from here. */

/* Include hardware and operating system dependent definitions and symbols. */

#include "mx_util.h"
#include "mx_stdint.h"
#include "mx_clock_tick.h"

/* Make the header file C++ safe. */

#ifdef __cplusplus
extern "C" {
#endif

/* ------------------------------------------------------------------ */

/* Define hardware and operating system independent preprocessor symbols. */

#define MXU_USERNAME_LENGTH			40
#define MXU_PROGRAM_NAME_LENGTH			40

#define MXU_RECORD_NAME_LENGTH			40
#define MXU_FIELD_NAME_LENGTH			40
#define MXU_RECORD_FIELD_NAME_LENGTH \
			(MXU_RECORD_NAME_LENGTH + MXU_FIELD_NAME_LENGTH + 1)

#define MXU_INTERFACE_ADDRESS_NAME_LENGTH	80
#define MXU_INTERFACE_NAME_LENGTH \
	(MXU_RECORD_NAME_LENGTH + MXU_INTERFACE_ADDRESS_NAME_LENGTH + 1)

#define MXU_LABEL_LENGTH			40
#define MXU_UNITS_NAME_LENGTH			16

#define MXU_NETWORK_TYPE_NAME_LENGTH		40
#define MXU_SCRIPT_TYPE_NAME_LENGTH		40

#define MXU_FIELD_MAX_DIMENSIONS		8
#define MXU_RECORD_DESCRIPTION_LENGTH		2500

#define MXU_VARARGS_LENGTH			0
#define MXU_VARARGS_COOKIE_MULTIPLIER		10000

#define MXU_ACL_DESCRIPTION_LENGTH		40

#define MXU_DEPENDENCY_ARRAY_BLOCK_LENGTH	10

#define MX_RECORD_FIELD_SEPARATORS		" \t\n"
#define MX_ILLEGAL_FILENAME_CHARS		" \t\n\""

/* The following values placed in the 'structure_id' field below
 * describe whether the 'data' pointer in an MX_RECORD_FIELD points
 * to an MX_RECORD, or the 'record_class_struct', or the 'record_type_struct'
 * in an MX_RECORD.
 */

#define MXF_REC_RECORD_STRUCT		1
#define MXF_REC_SUPERCLASS_STRUCT	2
#define MXF_REC_CLASS_STRUCT		4
#define MXF_REC_TYPE_STRUCT		8

/* The following is the list of bitmasks that can be put in the "flags"
 * field of an MX_RECORD_FIELD structure.  These are also used as mask
 * values for mx_print_structure().
 */

#define MXFF_IN_DESCRIPTION		0x1
#define MXFF_IN_SUMMARY			0x2
#define MXFF_READ_ONLY			0x4
#define MXFF_NO_ACCESS			0x8
#define MXFF_VARARGS			0x10
#define MXFF_NO_NEXT_EVENT_TIME_UPDATE	0x20
#define MXFF_POLL			0x40
#define MXFF_NO_PARENT_DEPENDENCY	0x80

/* The following values are used only by mx_print_structure(). */

#define MXFF_UPDATE_ALL			0x40000000
#define MXFF_SHOW_ALL			0x80000000

typedef struct mx_record_field_type {
	long label_value;
	long field_number;
	char *name;
	void *typeinfo;
	long datatype;
	long num_dimensions;
	long *dimension;
	size_t *data_element_size;

	unsigned long num_elements;
	size_t max_bytes;

	void *data_pointer;
	mx_status_type (*process_function) (void *, void *, void *, int);
	long flags;
	long timer_interval;

	double value_change_threshold;
	double last_value;
	mx_bool_type value_has_changed_manual_override;

	mx_status_type (*value_changed_test_function)(
			struct mx_record_field_type *, int, mx_bool_type *);

	void *callback_list;
	void *application_ptr;
	struct mx_record_type *record;
	mx_bool_type active;
} MX_RECORD_FIELD;

typedef struct {
	long label_value;
	long field_number;
	char name[MXU_FIELD_NAME_LENGTH+1];
	long datatype;
	void *typeinfo;
	long num_dimensions;
	long dimension[MXU_FIELD_MAX_DIMENSIONS];
	short structure_id;
	size_t structure_offset;
	size_t data_element_size[MXU_FIELD_MAX_DIMENSIONS];
	mx_status_type (*process_function) (void *, void *, void *, int);
	long flags;
	long timer_interval;
	double value_change_threshold;
	mx_status_type (*value_changed_test_function)(
			struct mx_record_field_type *, int, mx_bool_type *);
} MX_RECORD_FIELD_DEFAULTS;

typedef struct {
	struct mx_record_type *record;
	MX_CLOCK_TICK event_interval;
	MX_CLOCK_TICK next_allowed_event_time;
} MX_EVENT_TIME_MANAGER;

/* An MX_RECORD structure is the top level structure describing records
 * in the MX system.  It contains all the record type independent fields
 * and a pointer to a structure describing record type dependent information.
 */

typedef struct mx_record_type {
	long mx_superclass;	/* MXR_LIST_HEAD, MXR_INTERFACE, etc. */
	long mx_class;		/* MXI_CAMAC, MXC_MOTOR, etc. */
	long mx_type;		/* MXT_MTR_E500, MXT_MTR_PANTHER, etc. */
	char name[MXU_RECORD_NAME_LENGTH+1];
	char label[MXU_LABEL_LENGTH+1];
	void *acl;
	char acl_description[MXU_ACL_DESCRIPTION_LENGTH+1];
	signed long handle;
	long long_precision;
	int precision;
	mx_bool_type resynchronize;
	unsigned long record_flags;
	unsigned long record_processing_flags;

	struct mx_record_type *list_head;
	struct mx_record_type *previous_record;
	struct mx_record_type *next_record;

	void *record_superclass_struct; /* Ptr to MX_SCAN, MX_VARIABLE, etc. */
	void *record_class_struct;	/* Ptr to MX_CAMAC, MX_MOTOR, etc. */
	void *record_type_struct;	/* Ptr to MX_DSP6001, MX_E500, etc. */

	void *record_function_list;	/* Ptr to MX_RECORD_FUNCTION_LIST */
	void *superclass_specific_function_list;
	void *class_specific_function_list;

	long                  num_record_fields;
	MX_RECORD_FIELD       *record_field_array;

	struct mx_record_type *allocated_by;
	long                  num_parent_records;
	struct mx_record_type **parent_record_array;
	long                  num_child_records;
	struct mx_record_type **child_record_array;

	char network_type_name[MXU_NETWORK_TYPE_NAME_LENGTH+1];

	unsigned long script_type;
	char script_type_name[MXU_SCRIPT_TYPE_NAME_LENGTH+1];
	void *script_object;

	MX_EVENT_TIME_MANAGER *event_time_manager;
	void *event_queue;		/* Ptr to MXSRV_QUEUED_EVENT */

	void *application_ptr;

	mx_status_type (*application_destructor)( struct mx_record_type *,
							void * );
	void *application_destructor_args;
} MX_RECORD;

typedef struct {
	MX_RECORD *record;
	MX_RECORD_FIELD *record_field;
} MX_RECORD_FIELD_HANDLER;

/* The current list of script types. */

#define MXSO_NONE		0
#define MXSO_PYTHON		1

#define MXSO_OTHER		0x80000000

/* The following is the current list of record field types. */

#define MXFT_STRING		1	/* C-style null terminated string */
#define MXFT_CHAR		2	/* 8-bit signed integer */
#define MXFT_UCHAR		3	/* 8-bit unsigned integer */
#define MXFT_SHORT		4	/* 16-bit signed integer */
#define MXFT_USHORT		5	/* 16-bit unsigned integer */
#define MXFT_BOOL		6	/* Boolean.  32-bit unsigned integer */

/* When finally implemented, MXFT_ENUM will use a 32-bit unsigned integer
 * to represent the enum value on the network.  However, MX clients will
 * be able to fetch the string representations of the enum values from
 * the server if they want.  This will use an as-yet unspecified type of
 * network message.  The string representations will _not_ be read from
 * a file on the client, since there would be no good way to guarantee
 * that the client file was using the same string names as the server.
 */

#define MXFT_ENUM		7	/* NOT YET IMPLEMENTED! */

/* For local variables on the host, in principle, MXFT_LONG and MXFT_ULONG
 * use the native C size of a long integer.  However, MX has only been
 * implemented on 32-bit and 64-bit computers, so those are the only sizes
 * that have been tested.  But a compiled version of libMx would not fit into
 * the maximum address space of an 8-bit or 16-bit computer anyway, so this
 * is a distinction that does not really matter.
 *
 * For network messages in transit between a client and a server, the
 * MXFT_LONG and MXFT_ULONG data types default to 32-bit integers.  However,
 * if the client and the server are both 64-bit computers that have the
 * same integer endianness, then it is possible to request that MXFT_LONG
 * and MXFT_ULONG use 64-bit integers.  But that feature has not been
 * heavily tested.
 */

#define MXFT_LONG		8	/* 32 or 64-bit signed integer */
#define MXFT_ULONG		9	/* 32 or 64-bit unsigned integer */

/* MX network communication always uses IEEE 754 floating point numbers.
 *
 * A few ancient MX build targets (VAX, etc.) used non-IEEE floating point
 * on the host.  But all modern MX build targets use IEEE floating point 
 * on the host.
 */

#define MXFT_FLOAT		10	/* 32-bit IEEE 754 format */
#define MXFT_DOUBLE		11	/* 64-bit IEEE 754 format */

/* MXFT_HEX is internally the same as an MXFT_ULONG.  But when displayed
 * on the screen, their values are displayed in hexadecimal.
 */

#define MXFT_HEX		12	/* Stored as an MXFT_ULONG. */

/* The following datatypes are used when one needs to directly specify
 * the numerical bits in an integer field.  Mostly used in MX network
 * communication (since MX 2.1.19).  The 64-bit integer types have been
 * available since MX 1.2.0, but the 8, 16, and 32-bit types were only
 * added in MX 2.1.19.
 *
 * Note that for user displays, MXFT_CHAR and MXFT_UCHAR are displayed
 * as ASCII bytes, while MXFT_INT8 and MXFT_UINT8 are displayed as
 * 8-bit integer numbers.  If you want/need UTF-8, then you need to use
 * either an MXFT_STRING or an MXFT_STRING_XDR (for XDR) instead.  Note
 * that Unicode variants other than UTF-8 are not supported by MX,
 * even on Microsoft Windows.
 */

#define MXFT_INT64		14
#define MXFT_UINT64		15
#define MXFT_INT8		16
#define MXFT_UINT8		17
#define MXFT_INT16		18
#define MXFT_UINT16		19
#define MXFT_INT32		20
#define MXFT_UINT32		21

/* MXFT_STRING_XDR is a counted byte string as used by XDR.  The first
 * 4 bytes are a 32-bit length field expressed as a big-endian unsigned
 * integer followed by the bytes of the string.  The string field is not
 * required by the XDR standard to be null terminated.  However when
 * MX 2.1.19 and above receives an MXFT_STRING_XDR field, it adds a null
 * terminator after the specified end of the string data for convenience
 * in manipulating the data in C-code.
 */

#define MXFT_STRING_XDR		22

/* The following datatypes are mainly for internal use by MX.  However, if
 * a network client reads one of these fields, the client will be sent an
 * appropriate MXFT_STRING or MXFT_STRING_XDR instead over the network.
 */

#define MXFT_RECORD		31
#define MXFT_RECORDTYPE		32
#define MXFT_INTERFACE		33
#define MXFT_RECORD_FIELD	34

/* MX_NUM_RECORD_ID_FIELDS is the number of fields at the beginning
 * of a record description needed to unambiguously identify
 * the record type.  For example,  "testmotor device motor pmac_motor".
 */

#define MX_NUM_RECORD_ID_FIELDS	4

/* MX_RECORD_STANDARD_FIELDS are the record fields that should be
 * present in all record types.  They must be located at the beginning
 * of each record field array.
 */

#define MXLV_REC_PRECISION		101
#define MXLV_REC_RESYNCHRONIZE		102
#define MXLV_REC_ALLOCATED_BY		103
#define MXLV_REC_PARENT_RECORD_ARRAY	104
#define MXLV_REC_CHILD_RECORD_ARRAY	105
#define MXLV_REC_NETWORK_TYPE_NAME	106

#define MX_RECORD_STANDARD_FIELDS  \
  {-1, -1, "name", MXFT_STRING, NULL, 1, {MXU_RECORD_NAME_LENGTH}, \
	MXF_REC_RECORD_STRUCT, offsetof(MX_RECORD, name), \
	{sizeof(char)}, NULL, \
	(MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY \
	 	| MXFF_READ_ONLY | MXFF_NO_NEXT_EVENT_TIME_UPDATE) }, \
  \
  {-1, -1, "mx_superclass", MXFT_RECORDTYPE, NULL, 0, {0}, \
	MXF_REC_RECORD_STRUCT, offsetof(MX_RECORD, mx_superclass), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_READ_ONLY \
			| MXFF_NO_NEXT_EVENT_TIME_UPDATE)}, \
  \
  {-1, -1, "mx_class", MXFT_RECORDTYPE, NULL, 0, {0}, \
	MXF_REC_RECORD_STRUCT, offsetof(MX_RECORD, mx_class), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_READ_ONLY \
			| MXFF_NO_NEXT_EVENT_TIME_UPDATE)}, \
  \
  {-1, -1, "mx_type", MXFT_RECORDTYPE, NULL, 0, {0}, \
	MXF_REC_RECORD_STRUCT, offsetof(MX_RECORD, mx_type), \
	{0}, NULL, (MXFF_IN_DESCRIPTION | MXFF_IN_SUMMARY \
			| MXFF_READ_ONLY | MXFF_NO_NEXT_EVENT_TIME_UPDATE)},\
  \
  {-1, -1, "label", MXFT_STRING, NULL, 1, {MXU_LABEL_LENGTH}, \
	MXF_REC_RECORD_STRUCT, offsetof(MX_RECORD, label), \
	{sizeof(char)}, NULL, \
		(MXFF_IN_DESCRIPTION | MXFF_NO_NEXT_EVENT_TIME_UPDATE)}, \
  \
  {-1, -1, "acl_description", MXFT_STRING, NULL, \
					1, {MXU_ACL_DESCRIPTION_LENGTH}, \
	MXF_REC_RECORD_STRUCT, offsetof(MX_RECORD, acl_description), \
	{sizeof(char)}, NULL, \
		(MXFF_IN_DESCRIPTION | MXFF_NO_NEXT_EVENT_TIME_UPDATE)}, \
  \
  {-1, -1, "precision", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_RECORD_STRUCT, offsetof(MX_RECORD, long_precision), \
	{0}, NULL, MXFF_NO_NEXT_EVENT_TIME_UPDATE }, \
  \
  {MXLV_REC_RESYNCHRONIZE, -1, "resynchronize", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_RECORD_STRUCT, offsetof(MX_RECORD, resynchronize), \
	{0}, NULL, 0}, \
  \
  {MXLV_REC_RESYNCHRONIZE, -1, "reset", MXFT_BOOL, NULL, 0, {0}, \
	MXF_REC_RECORD_STRUCT, offsetof(MX_RECORD, resynchronize), \
	{0}, NULL, 0}, \
  \
  {-1, -1, "record_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_RECORD_STRUCT, offsetof(MX_RECORD, record_flags), \
	{0}, NULL, (MXFF_READ_ONLY | MXFF_NO_NEXT_EVENT_TIME_UPDATE) }, \
  \
  {-1, -1, "record_processing_flags", MXFT_HEX, NULL, 0, {0}, \
	MXF_REC_RECORD_STRUCT, offsetof(MX_RECORD, record_processing_flags), \
	{0}, NULL, (MXFF_READ_ONLY | MXFF_NO_NEXT_EVENT_TIME_UPDATE) }, \
  \
  {MXLV_REC_ALLOCATED_BY, -1, "allocated_by", MXFT_RECORD, NULL, 0, {0}, \
	MXF_REC_RECORD_STRUCT, offsetof(MX_RECORD, allocated_by), \
	{0}, NULL, (MXFF_READ_ONLY | MXFF_NO_NEXT_EVENT_TIME_UPDATE) }, \
  \
  {-1, -1, "num_parent_records", MXFT_LONG, NULL, 0, {0},\
	MXF_REC_RECORD_STRUCT, offsetof(MX_RECORD, num_parent_records),\
	{0}, NULL, (MXFF_READ_ONLY | MXFF_NO_NEXT_EVENT_TIME_UPDATE) }, \
  \
  {MXLV_REC_PARENT_RECORD_ARRAY, -1, "parent_record_array", MXFT_RECORD, \
	NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_RECORD_STRUCT, offsetof(MX_RECORD, parent_record_array), \
	{sizeof(MX_RECORD *)}, NULL, \
	    (MXFF_VARARGS | MXFF_READ_ONLY | MXFF_NO_NEXT_EVENT_TIME_UPDATE)}, \
  \
  {-1, -1, "num_child_records", MXFT_LONG, NULL, 0, {0}, \
	MXF_REC_RECORD_STRUCT, offsetof(MX_RECORD, num_child_records),\
	{0}, NULL, (MXFF_READ_ONLY | MXFF_NO_NEXT_EVENT_TIME_UPDATE) }, \
  \
  {MXLV_REC_CHILD_RECORD_ARRAY, -1, "child_record_array", MXFT_RECORD, \
	NULL, 1, {MXU_VARARGS_LENGTH}, \
	MXF_REC_RECORD_STRUCT, offsetof(MX_RECORD, child_record_array), \
	{sizeof(MX_RECORD *)}, NULL, \
	    (MXFF_VARARGS | MXFF_READ_ONLY | MXFF_NO_NEXT_EVENT_TIME_UPDATE)}, \
  \
  {MXLV_REC_NETWORK_TYPE_NAME, -1, "network_type_name", MXFT_STRING, \
		NULL, 1, {MXU_NETWORK_TYPE_NAME_LENGTH}, \
	MXF_REC_RECORD_STRUCT, offsetof(MX_RECORD, network_type_name), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "script_type", MXFT_ULONG, NULL, 0, {0}, \
	MXF_REC_RECORD_STRUCT, offsetof(MX_RECORD, script_type), \
	{0}, NULL, MXFF_READ_ONLY}, \
  \
  {-1, -1, "script_type_name", MXFT_STRING, \
		NULL, 1, {MXU_SCRIPT_TYPE_NAME_LENGTH}, \
	MXF_REC_RECORD_STRUCT, offsetof(MX_RECORD, script_type_name), \
	{sizeof(char)}, NULL, MXFF_READ_ONLY}

/* Definition of bits in the 'record_flags' field of the record. */

#define MXF_REC_INITIALIZED		0x1
#define MXF_REC_ENABLED			0x2
#define MXF_REC_OPEN			0x4
#define MXF_REC_BROKEN			0x8
#define MXF_REC_FAULTED			0x10

#define MXF_REC_CAN_LATCH_VALUE		0x100

/* Definition of bits in the 'record_processing_flags' field of the record. */

#define MXF_PROC_BYPASS_DEFAULT_PROCESSING	0x1

#define MXF_PROC_PROCESSING_IS_INITIALIZED	0x8000

/* Definition of trigger modes for devices that support triggers, such as
 * area detectors, video inputs, pulse generators, multichannel analyzers,
 * multichannel scalers, etc.
 *
 * For some devices, the trigger flags can be used as a bit map, since
 * the device can be waiting for any of several possible kinds of triggers.
 * However, most devices do not do this.
 *
 * No trigger        (0x0) = Ignore any triggers.
 * Internal trigger  (0x1) = Usually a software generated trigger.
 * External trigger  (0x2) = An external voltage/current/whatever pulse.
 * Line trigger      (0x4) = Typically an AC voltage line crossing.
 * Auto trigger      (0x8) - Always trigger.
 * Database trigger (0x10) = Waiting for a change in a database variable.
 * Manual trigger   (0x20) = Waiting for the user to push a button or some such.
 *
 * Some devices can be configured either for edge triggers (0x1000)
 * or level triggers (0x2000).  Typically this will be (|) OR-ed with
 * the other trigger values above.  Trigger low (0x4000) and trigger
 * high (0x8000) refer either to edges or levels.
 *
 * One-shot (0x10000) tells the device to do a single acquisition or trigger.
 * For some devices, the one-shot mode may do something different from what
 * happens for an ordinary internal/external trigger with one pulse.
 */

#define MXF_DEV_NO_TRIGGER		0x0
#define MXF_DEV_INTERNAL_TRIGGER	0x1
#define MXF_DEV_EXTERNAL_TRIGGER	0x2
#define MXF_DEV_LINE_TRIGGER		0x4
#define MXF_DEV_AUTO_TRIGGER		0x8
#define MXF_DEV_DATABASE_TRIGGER	0x10
#define MXF_DEV_MANUAL_TRIGGER		0x20

#define MXF_DEV_EDGE_TRIGGER		0x1000
#define MXF_DEV_LEVEL_TRIGGER		0x2000
#define MXF_DEV_TRIGGER_LOW		0x4000
#define MXF_DEV_TRIGGER_HIGH		0x8000
#define MXF_DEV_ONE_SHOT_TRIGGER	0x10000

/* An MX_RECORD_FUNCTION_LIST structure contains a list of the functions
 * that are the same for all record types.
 */

struct mx_driver_type;

typedef struct {
	mx_status_type ( *initialize_driver )( struct mx_driver_type *driver );
	mx_status_type ( *create_record_structures ) ( MX_RECORD *record );
	mx_status_type ( *finish_record_initialization )( MX_RECORD * );
	mx_status_type ( *delete_record )( MX_RECORD * );
	mx_status_type ( *print_structure )( FILE *file, MX_RECORD *record );
	mx_status_type ( *open )( MX_RECORD *record );
	mx_status_type ( *close )( MX_RECORD *record );
	mx_status_type ( *finish_delayed_initialization )( MX_RECORD *record );
	mx_status_type ( *resynchronize )( MX_RECORD *record );
	mx_status_type ( *special_processing_setup )( MX_RECORD *record );
} MX_RECORD_FUNCTION_LIST;

#define MXU_DRIVER_NAME_LENGTH	32 

typedef struct mx_driver_type {
	char name[MXU_DRIVER_NAME_LENGTH+1];
	long mx_type;
	long mx_class;
	long mx_superclass;
	MX_RECORD_FUNCTION_LIST *record_function_list;
	void *superclass_specific_function_list;
	void *class_specific_function_list;
	long *num_record_fields;
	MX_RECORD_FIELD_DEFAULTS **record_field_defaults_ptr;
	struct mx_driver_type *next_driver;
} MX_DRIVER;

typedef struct {
	char *description;
	char *separators;
	int num_separators;
	char *start_of_trailing_whitespace;
	unsigned long max_string_token_length;
} MX_RECORD_FIELD_PARSE_STATUS;

typedef struct {
	MX_RECORD *record;
	char address_name[MXU_INTERFACE_ADDRESS_NAME_LENGTH+1];
	long address;
} MX_INTERFACE;

typedef struct {
	mx_bool_type add_dependency;
	mx_bool_type dependency_is_to_parent;
} MX_RECORD_ARRAY_DEPENDENCY_STRUCT;

/* MX_LIST_HEAD is a place to put information about the record list
 * as a whole.
 */

#define MXU_REVISION_NAME_LENGTH		80

#define MX_FIXUP_RECORD_ARRAY_BLOCK_SIZE	50

typedef struct {
	MX_RECORD *record;

	mx_bool_type list_is_active;
	mx_bool_type fast_mode;
	mx_bool_type allow_fast_mode;
	mx_bool_type remote_breakpoint_enabled;
	unsigned long network_debug_flags;
	long debug_level;
	char status[ MXU_FIELD_NAME_LENGTH + 1 ];

	unsigned long mx_version;
	uint64_t mx_version_time;
	uint64_t posix_time;
	long mx_revision_number;
	char mx_revision_string[ MXU_REVISION_NAME_LENGTH + 1 ];
	char mx_branch_label[ MXU_REVISION_NAME_LENGTH + 1 ];
	char mx_version_string[ MXU_REVISION_NAME_LENGTH + 1 ];

	unsigned long num_records;

	char report[ MXU_RECORD_NAME_LENGTH + 1 ];
	char report_all[ MXU_RECORD_NAME_LENGTH + 1 ];
	char update_all[ MXU_RECORD_NAME_LENGTH + 1 ];
	char summary[ MXU_RECORD_NAME_LENGTH + 1 ];
	mx_bool_type show_record_list;
	char fielddef[ MXU_FIELD_NAME_LENGTH + 1 ];
	unsigned long show_handle[2];
	mx_bool_type show_callbacks;
	unsigned long show_callback_id;
	mx_bool_type show_socket_handlers;
	unsigned long show_socket_id;
	mx_bool_type short_error_codes;

	unsigned long breakpoint_number;
	mx_bool_type numbered_breakpoint_status;

	mx_bool_type breakpoint;		/* Run mx_breakpoint(). */
	mx_bool_type crash;			/* Intentional crash. */
	unsigned long debugger_pid;
	mx_bool_type show_open_fds;
	mx_bool_type callbacks_enabled;
	char *cflags;
	unsigned long vm_region[2];
	char show_field[ MXU_RECORD_FIELD_NAME_LENGTH + 1 ];

	mx_bool_type is_server;
	void *connection_acl;
	mx_bool_type fixup_records_in_use;
	long num_fixup_records;
	void **fixup_record_array;
	mx_bool_type plotting_enabled;
	long default_precision;
	long default_data_format;
	void *log_handler;
	long server_protocols_active;
	void *handle_table;
	void *application_ptr;

	char hostname[ MXU_HOSTNAME_LENGTH + 1 ];
	char username[ MXU_USERNAME_LENGTH + 1 ];
	char program_name[ MXU_PROGRAM_NAME_LENGTH + 1 ];

	unsigned long num_server_records;
	MX_RECORD **server_record_array;

	void *client_callback_handle_table;
	void *server_callback_handle_table;

	void *master_timer;
	void *callback_timer;

	void *callback_pipe;

	void *poll_callback_message;
	unsigned long num_poll_callbacks;
	double poll_callback_interval;		/* in seconds */

	void *module_list;

	unsigned long socket_multiplexer_type;

	unsigned long max_network_dump_bytes;

	/* Show a list of all threads in this process. */
	mx_bool_type show_thread_list;

	/* For the following two, you give it the thread number shown
	 * by writing to the show_thread_list field above.  Yes, there
	 * can be race conditions, but this is for diagnostics only.
	 */
	unsigned long show_thread_info;
	unsigned long show_thread_stack;

	void *thread_object;
	long thread_stack_signal;

} MX_LIST_HEAD;

/* --- Record list handling functions. --- */

/* The following are flags for mx_create_record_from_description()
 * and mx_read_database_file().
 */

#define MXFC_ALLOW_RECORD_REPLACEMENT	0x1
#define MXFC_ALLOW_SCAN_REPLACEMENT	0x2
#define MXFC_DELETE_BROKEN_RECORDS	0x4

/* The following are flags for mx_initialize_hardware(). */

#define MXF_INITHW_TRACE_OPENS		0x1
#define MXF_INITHW_ABORT_ON_FAULT	0x2

MX_API mx_bool_type mx_verify_driver_type( MX_RECORD *record,
					long mx_superclass,
					long mx_class,
					long mx_type );

MX_API MX_DRIVER *mx_get_driver_by_name( char *name );
MX_API MX_DRIVER *mx_get_driver_by_type( long record_type );

MX_API MX_DRIVER *mx_get_driver_object( MX_RECORD *record );
MX_API MX_DRIVER *mx_get_driver_class_object( MX_RECORD *record );
MX_API MX_DRIVER *mx_get_driver_superclass_object( MX_RECORD *record );

MX_API const char *mx_get_driver_name( MX_RECORD *record );
MX_API const char *mx_get_driver_class_name( MX_RECORD *record );
MX_API const char *mx_get_driver_superclass_name( MX_RECORD *record );

MX_API MX_DRIVER *mx_get_superclass_driver_by_name( char *name );
MX_API MX_DRIVER *mx_get_class_driver_by_name( char *name );

MX_API MX_DRIVER *mx_get_superclass_driver_by_type( long superclass_type );
MX_API MX_DRIVER *mx_get_class_driver_by_type( long class_type );

MX_API mx_status_type mx_add_driver_table( MX_DRIVER *driver_table );

MX_API mx_status_type mx_verify_driver( MX_DRIVER *driver );

MX_API mx_status_type mx_verify_driver_tables( void );

/*---*/

MX_API long  mx_get_parameter_type_from_name( MX_RECORD *record, char *name );
MX_API char *mx_get_parameter_name_from_type( MX_RECORD *record, long type,
						char *name_buffer,
						size_t name_buffer_length );

MX_API MX_RECORD      *mx_create_record( void );
MX_API mx_status_type  mx_delete_record( MX_RECORD *record );

MX_API mx_status_type  mx_insert_before_record( MX_RECORD *old_record,
						MX_RECORD *new_record );
MX_API mx_status_type  mx_insert_after_record( MX_RECORD *old_record,
						MX_RECORD *new_record );

MX_API MX_RECORD      *mx_get_record( MX_RECORD *record_list,
						const char *record_name );

MX_API mx_status_type  mx_default_delete_record_handler( MX_RECORD *record );

MX_API mx_status_type  mx_delete_record_list( MX_RECORD *record_list );

MX_API mx_status_type  mx_delete_record_class(
				MX_RECORD *record_list, long record_classes );

MX_API MX_RECORD      *mx_get_database( void );

MX_API MX_RECORD      *mx_initialize_database( void );

MX_API mx_status_type  mx_initialize_drivers( void );

MX_API mx_status_type  mx_initialize_hardware( MX_RECORD *record_list,
						unsigned long inithw_flags );

MX_API mx_status_type  mx_shutdown_hardware( MX_RECORD *record_list );

MX_API MX_LIST_HEAD   *mx_get_record_list_head_struct( MX_RECORD *record );

MX_API mx_bool_type    mx_database_is_server( MX_RECORD *record );

MX_API mx_bool_type    mx_record_is_valid( MX_RECORD *record );

MX_API mx_status_type  mx_set_event_interval( MX_RECORD *record,
					double event_interval_in_seconds );

/* --- */

MX_API mx_status_type  mx_create_empty_database( MX_RECORD **record_list );

MX_API mx_status_type  mx_read_database_file( MX_RECORD *record_list,
				const char *filename, unsigned long flags );

MX_API mx_status_type  mx_finish_record_initialization( MX_RECORD *record );

MX_API mx_status_type  mx_finish_database_initialization(
				MX_RECORD *record_list );

MX_API mx_status_type  mx_write_database_file( MX_RECORD *record_list,
						const char *filename,
						long num_record_superclasses,
						long *record_superclass_list );

/* Most MX clients and servers should invoke mx_setup_database() to
 * initialize MX at program startup time.
 */

MX_API mx_status_type  mx_setup_database( MX_RECORD **record_list,
					const char *database_filename );

/* --- */

/* These functions can be used to read the database description records
 * from an array instead of a file.
 */

MX_API mx_status_type  mx_setup_database_from_array( MX_RECORD **record_list,
						long num_descriptions,
						char **description_array );

MX_API mx_status_type  mx_read_database_from_array( MX_RECORD *record_list,
						long num_descriptions,
						char **description_array,
						unsigned long flags );
/* --- */

/* These functions return the record list pointer as the function return
 * value.  This is to make it easier for applications like LabVIEW which
 * do not have easy ways of dealing with doubly-indirect C pointers.
 */

MX_API MX_RECORD *mx_setup_database_pointer( const char *database_filename );

MX_API MX_RECORD *mx_setup_database_pointer_from_array(
						long num_descriptions,
						char **description_array );

/* --- */

MX_API mx_status_type  mx_print_structure( FILE *file,
					MX_RECORD *record,
					unsigned long mask );

MX_API mx_status_type  mx_print_summary( FILE *file,
					MX_RECORD *record );

MX_API mx_status_type  mx_print_all_field_attributes( FILE *file,
					MX_RECORD *record );

MX_API mx_status_type  mx_print_field_attributes( FILE *file,
					MX_RECORD_FIELD *field );

MX_API long mx_get_sized_local_datatype( long mx_datatype );

MX_API mx_status_type  mx_compute_field_size( MX_RECORD_FIELD *field );

MX_API mx_status_type  mx_print_field_array( FILE *file,
					MX_RECORD *record,
					MX_RECORD_FIELD *field,
					mx_bool_type verbose_flag );

MX_API mx_status_type  mx_print_field_value( FILE *file,
					MX_RECORD *record,
					MX_RECORD_FIELD *field,
					void *value_ptr,
					mx_bool_type verbose_flag );

MX_API mx_status_type  mx_print_field( FILE *file,
					MX_RECORD *record,
					MX_RECORD_FIELD *field,
					mx_bool_type verbose_flag );

MX_API mx_status_type  mx_open_hardware( MX_RECORD *record );
MX_API mx_status_type  mx_close_hardware( MX_RECORD *record );

MX_API mx_status_type  mx_finish_delayed_initialization( MX_RECORD *record );

MX_API mx_status_type  mx_resynchronize_record( MX_RECORD *record );

/* 'reset' is just a new alias for 'resynchronize'. */

MX_API mx_status_type  mx_reset_record( MX_RECORD *record );

MX_API mx_status_type  mx_update_record_values( MX_RECORD *record );

/* --- */

MX_API mx_status_type  mx_create_record_from_description(
			MX_RECORD *record_list, char *record_description,
			MX_RECORD **created_record, unsigned long flags );

MX_API mx_status_type  mx_create_description_from_record(
			MX_RECORD *record, char *record_description_buffer,
			size_t description_buffer_length );

/* --- */

#define MXF_DATABASE_VALID_DEBUG	0x1

MX_API mx_bool_type    mx_database_is_valid( MX_RECORD *record_list,
						unsigned long flags );

/* --- */

MX_API MX_RECORD_FIELD *mx_get_record_field( MX_RECORD *record,
						const char *field_name );

/* mx_get_record_field_by_name() expects to be provided a record_field_name
 * in the format "record_name.field_name".
 */

MX_API MX_RECORD_FIELD *mx_get_record_field_by_name( MX_RECORD *mx_database,
						const char *record_field_name );

MX_API mx_status_type  mx_find_record_field( MX_RECORD *record,
				const char *name_of_field_to_find,
				MX_RECORD_FIELD **field_that_was_found );

MX_API mx_status_type  mx_construct_ptr_to_field_data(
			MX_RECORD *record,
			MX_RECORD_FIELD_DEFAULTS *record_field_defaults,
			void **field_data_ptr );

MX_API mx_status_type  mx_construct_ptr_to_field_value(
			MX_RECORD *record,
			MX_RECORD_FIELD_DEFAULTS *record_field_defaults,
			void **field_value_ptr );

/* --- */

MX_API_PRIVATE mx_status_type  mx_create_field_from_description(
			MX_RECORD *record, MX_RECORD_FIELD *record_field,
			MX_RECORD_FIELD_PARSE_STATUS *parse_status,
			char *field_description );

MX_API_PRIVATE mx_status_type  mx_create_description_from_field(
			MX_RECORD *record, MX_RECORD_FIELD *record_field,
			char *field_description_buffer,
			size_t field_description_buffer_length );

MX_API_PRIVATE mx_status_type mx_parse_array_description( void *array_ptr,
			long dimension_level,
			MX_RECORD *record,
			MX_RECORD_FIELD *field,
			MX_RECORD_FIELD_PARSE_STATUS *parse_status,
			mx_status_type (*token_parser) ( void *, char *,
				MX_RECORD *, MX_RECORD_FIELD *,
				MX_RECORD_FIELD_PARSE_STATUS * )
			);

MX_API_PRIVATE mx_status_type mx_create_array_description( void *array_ptr,
			long dimension_level,
			char *buffer_ptr,
			size_t max_buffer_length,
			MX_RECORD *record,
			MX_RECORD_FIELD *field,
			mx_status_type (*token_creater) ( void *,
				char *, size_t,
				MX_RECORD *, MX_RECORD_FIELD * )
			);

MX_API_PRIVATE void mx_initialize_parse_status(
				MX_RECORD_FIELD_PARSE_STATUS *parse_status,
				char *description, char *separators );

MX_API_PRIVATE mx_status_type mx_copy_defaults_to_record_field(
			MX_RECORD_FIELD *field,
			MX_RECORD_FIELD_DEFAULTS *field_defaults );

MX_API_PRIVATE mx_status_type  mx_parse_record_fields( MX_RECORD *record,
			MX_RECORD_FIELD_DEFAULTS *record_field_defaults_array,
			MX_RECORD_FIELD_PARSE_STATUS *parse_status );

MX_API_PRIVATE mx_status_type  mx_get_next_record_token(
				MX_RECORD_FIELD_PARSE_STATUS *parse_status,
				char *buffer, size_t buffer_length );

MX_API_PRIVATE mx_status_type mx_get_token_parser( long field_type,
		mx_status_type ( **token_parser ) (
			void *dataptr,
			char *token,
			MX_RECORD *record,
			MX_RECORD_FIELD *record_field,
			MX_RECORD_FIELD_PARSE_STATUS *parse_status )
		);

MX_API_PRIVATE mx_status_type mx_get_token_constructor( long field_type,
		mx_status_type ( **token_constructor ) (
			void *dataptr,
			char *token_buffer,
			size_t token_buffer_length,
			MX_RECORD *record,
			MX_RECORD_FIELD *record_field )
		);

MX_API_PRIVATE long mx_get_max_string_token_length( MX_RECORD_FIELD *field );

/* --- */

MX_API_PRIVATE mx_status_type mx_convert_varargs_cookie_to_value(
				MX_RECORD *record,
				long varargs_cookie,
				long *returned_value );

MX_API_PRIVATE mx_status_type mx_construct_varargs_cookie(
				long field_index,
				long array_in_field_index,
				long *returned_varargs_cookie );

MX_API_PRIVATE mx_status_type mx_replace_varargs_cookies_with_values(
		MX_RECORD *record, long i,
		mx_bool_type allow_forward_references );

/* --- */

MX_API_PRIVATE mx_status_type  mx_find_record_field_defaults(
		MX_DRIVER *driver,
		const char *name_of_field_to_find,
		MX_RECORD_FIELD_DEFAULTS **default_field_that_was_found );

MX_API_PRIVATE mx_status_type  mx_find_record_field_defaults_index(
		MX_DRIVER *driver,
		const char *name_of_field_to_find,
		long *index_of_field_that_was_found );

MX_API long mx_get_datatype_from_datatype_name( const char *datatype_name );

MX_API const char *mx_get_datatype_name_from_datatype( long datatype );

MX_API mx_status_type mx_get_field_by_label_value( MX_RECORD *record,
						long label_value,
						MX_RECORD_FIELD **field );

MX_API const char *mx_get_field_label_string( MX_RECORD *record,
						long label_value );

MX_API const char *mx_get_field_type_string( long field_type );

MX_API void *mx_get_field_value_pointer( MX_RECORD_FIELD *field );

MX_API mx_status_type mx_set_1d_field_array_length( MX_RECORD_FIELD *field,
							unsigned long length );

MX_API mx_status_type mx_set_1d_field_array_length_by_name( MX_RECORD *record,
							char *field_name,
							unsigned long length );

/* --- */

MX_API_PRIVATE mx_status_type  mx_construct_placeholder_record(
			MX_RECORD *referencing_record,
			char *record_name,
			MX_RECORD **placeholder_record );

MX_API_PRIVATE mx_status_type  mx_fixup_placeholder_records(
					MX_RECORD *record_list );

/* --- */

#define mx_num_array_elements( x )	( sizeof(x) / sizeof(x)[0] )

MX_API mx_status_type  mx_get_datatype_sizeof_array( long datatype,
						size_t *sizeof_array,
						size_t max_dimensions );

/* --- */

MX_API_PRIVATE mx_status_type  mx_initialize_temp_record_field(
					MX_RECORD_FIELD *temp_record_field,
					void *typeinfo,  /* MX_DRIVER */
					long datatype,
					long num_dimensions,
					long *dimension,
					size_t *data_element_size,
					void *value_ptr );

/* --- */

/* Typedef for MX_TRAVERSE_FIELD_HANDLER function type.
 * Unfortunately, due to ANSI C restrictions, this typedef
 * cannot be used directly to define a handler function.
 * All it can be used for is to declare arguments to
 * functions like mx_traverse_field() or to manipulate
 * pointers to these functions.  You must define the handler
 * functions themselves in the code in the longwinded way
 * via "mx_status_type foo_fn(...".
 */

typedef mx_status_type MX_TRAVERSE_FIELD_HANDLER(
				MX_RECORD *record,
				MX_RECORD_FIELD *field,
				void *handler_data_ptr,
				long *array_indices,
				void *array_ptr,
				long dimension_level );
				

MX_API mx_status_type  mx_traverse_field( MX_RECORD *record,
					MX_RECORD_FIELD *field,
					MX_TRAVERSE_FIELD_HANDLER *handler_fn,
					void *handler_data_ptr,
					long *array_indices );

MX_API_PRIVATE mx_status_type  mx_traverse_field_array(
					MX_RECORD *record,
					MX_RECORD_FIELD *field,
					MX_TRAVERSE_FIELD_HANDLER *handler_fn,
					void *handler_data_ptr,
					long *array_indices,
					void *array_ptr,
					long dimension_level );

/* --- */

MX_API_PRIVATE mx_status_type  mx_record_array_dependency_handler(
					MX_RECORD *record,
					MX_RECORD_FIELD *record_field,
					void *dependency_struct_ptr,
					long *array_indices,
					void *array_ptr,
					long dimension_level );

MX_API_PRIVATE mx_status_type  mx_add_parent_dependency(
				MX_RECORD *current_record,
				MX_RECORD_FIELD *current_field,
				mx_bool_type add_child_pointer_in_parent,
				MX_RECORD *parent_record );
MX_API_PRIVATE mx_status_type  mx_delete_parent_dependency(
				MX_RECORD *current_record,
				MX_RECORD_FIELD *current_field,
				mx_bool_type delete_child_pointer_in_parent,
				MX_RECORD *parent_record );
MX_API_PRIVATE mx_status_type  mx_add_child_dependency(
				MX_RECORD *current_record,
				MX_RECORD_FIELD *current_field,
				mx_bool_type add_parent_pointer_in_child,
				MX_RECORD *child_record );
MX_API_PRIVATE mx_status_type  mx_delete_child_dependency(
				MX_RECORD *current_record,
				MX_RECORD_FIELD *current_field,
				mx_bool_type delete_parent_pointer_in_child,
				MX_RECORD *child_record );

/* --- */

MX_API mx_status_type  mx_set_list_application_ptr( MX_RECORD *record_list,
						void *application_ptr );
MX_API mx_status_type  mx_set_record_application_ptr( MX_RECORD *record,
						void *application_ptr );
MX_API mx_status_type  mx_set_field_application_ptr(
						MX_RECORD_FIELD *record_field,
						void *application_ptr );

MX_API mx_status_type  mx_get_record_application_destructor(
		MX_RECORD *record,
		mx_status_type (**application_destructor)(MX_RECORD *, void *),
		void **destructor_args );

MX_API mx_status_type  mx_set_record_application_destructor(
		MX_RECORD *record,
		mx_status_type (*application_destructor)(MX_RECORD *, void *),
		void *destructor_args );

/* --- */

MX_API mx_status_type  mx_set_program_name( MX_RECORD *record_list,
						const char *program_name );

MX_API mx_status_type  mx_get_fast_mode( MX_RECORD *record_list,
						mx_bool_type *mode_flag );

MX_API mx_status_type  mx_set_fast_mode( MX_RECORD *record_list,
						mx_bool_type mode_flag );

#ifdef __cplusplus
}
#endif

#endif /* __MX_RECORD_H__ */

