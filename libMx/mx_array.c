/*
 * Name:    mx_array.c
 *
 * Purpose: MX array handling functions.
 *
 * Author:  William Lavender
 *
 *------------------------------------------------------------------------
 *
 * Copyright 1999, 2001, 2003-2006 Illinois Institute of Technology
 *
 * See the file "LICENSE" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 *
 */

#include <stdio.h>
#include <stdlib.h>

#include "mx_util.h"
#include "mx_record.h"
#include "mx_osdef.h"
#include "mx_stdint.h"
#include "mx_socket.h"
#include "mx_array.h"

#if HAVE_XDR
#  if defined(OS_WIN32) || defined(OS_DJGPP)
#     include "../tools/xdr/src/xdr.h"

#  elif defined(OS_VMS)
#     include <tcpip$rpcxdr.h>

#  else
#     if defined(__BORLANDC__)
         typedef long long int		int64_t;
         typedef unsigned long long int	uint64_t;
#     endif

#     include <limits.h>
#     include <rpc/types.h>
#     include <rpc/xdr.h>
#  endif
#endif

MX_EXPORT void *
mx_allocate_array( long num_dimensions,
		long *dimension_array,
		size_t *data_element_size_array )
{
	static const char fname[] = "mx_allocate_array()";

	void *array_pointer;
	char *array_element_pointer;
	void *subarray_pointer;
	long array_size;
	long i, j;
	long current_dimension_element_size;
	mx_status_type mx_status;

	if ( num_dimensions <= 0 ) {
		mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal number of dimensions = %ld", num_dimensions );

		return NULL;
	}

	if ( dimension_array == (long *) NULL ) {
		mx_error( MXE_NULL_ARGUMENT, fname,
		"dimension_array pointer passed is NULL." );

		return NULL;
	}

	if ( data_element_size_array == (size_t *) NULL ) {
		mx_error( MXE_NULL_ARGUMENT, fname,
		"data_element_size_array pointer passed is NULL." );

		return NULL;
	}

	MX_DEBUG( 8,("%s: num_dimensions = %ld", fname, num_dimensions));

	for ( i = 0; i < num_dimensions; i++ ) {
		MX_DEBUG( 8,(
			"%s:   dimension[%ld] = %ld, element size[%ld] = %ld",
			fname, i, dimension_array[i],
			i, (long) data_element_size_array[i]));
	}

	current_dimension_element_size
			= (long) (data_element_size_array[num_dimensions - 1]);

	array_size = dimension_array[0] * current_dimension_element_size;

	MX_DEBUG( 8,("%s: array_size = %ld", fname, array_size));

	if ( array_size <= 0 ) {
		array_pointer = NULL;
	} else {
#if 1
		array_pointer = (char **) malloc( array_size );
#else
		array_pointer = (char **) calloc( array_size, 1 );
#endif
	}

	MX_DEBUG( 8,("%s: array_pointer = %p", fname, array_pointer));

	if ( array_pointer == NULL ) {
		(void) mx_error( MXE_OUT_OF_MEMORY, fname,
	"Ran out of memory trying to allocate %ld elements of size %ld.",
			dimension_array[0],
			(long) current_dimension_element_size );
		return NULL;
	}

	if ( num_dimensions > 1 ) {
		for ( i = 0; i < dimension_array[0]; i++ ) {
			array_element_pointer = (char *)array_pointer
					+ i * current_dimension_element_size;

			subarray_pointer = mx_allocate_array(
				num_dimensions - 1,
				&dimension_array[1],
				data_element_size_array );

			MX_DEBUG( 8,
		("%s: array_element_pointer = %p, subarray_pointer = %p",
			fname, array_element_pointer, subarray_pointer));

			/* If the allocation of this part of the array
			 * failed, attempt to free all the parts of
			 * the array that we have already allocated.
			 */

			if ( subarray_pointer == NULL ) {
				for ( j = 0; j < i; j++ ) {
					array_element_pointer
						= (char *)array_pointer
					+ j * current_dimension_element_size;

					subarray_pointer =
				mx_read_void_pointer_from_memory_location(
						array_element_pointer );

					mx_status = mx_free_array(
						subarray_pointer,
						num_dimensions - 1,
						&dimension_array[1],
						data_element_size_array );

					/* If we can't even free memory,
					 * then there's not much else we
					 * can do but give up.
					 */

					if ( mx_status.code != MXE_SUCCESS )
						return NULL;
				}

				return NULL;
			}
			mx_write_void_pointer_to_memory_location(
				array_element_pointer, subarray_pointer );
		}
	}

	return array_pointer;
}

MX_EXPORT mx_status_type
mx_free_array( void *array_pointer,
		long num_dimensions,
		long *dimension_array,
		size_t *data_element_size_array )
{
	static const char fname[] = "mx_free_array()";

	char *array_element_pointer;
	void *subarray_pointer;
	long i;
	size_t current_dimension_element_size;
	mx_status_type mx_status;

	if ( array_pointer == NULL ) {
		mx_error( MXE_NULL_ARGUMENT, fname,
		"array_pointer passed is NULL." );
	}

	if ( num_dimensions <= 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"Illegal number of dimensions = %ld", num_dimensions );
	}

	if ( dimension_array == (long *) NULL ) {
		mx_error( MXE_NULL_ARGUMENT, fname,
		"dimension_array pointer passed is NULL." );
	}

	if ( data_element_size_array == (size_t *) NULL ) {
		mx_error( MXE_NULL_ARGUMENT, fname,
		"data_element_size_array pointer passed is NULL." );
	}

	MX_DEBUG( 8,("%s: num_dimensions = %ld", fname, num_dimensions));

	MX_DEBUG( 8,("%s: array_pointer = %p", fname, array_pointer));

	for ( i = 0; i < num_dimensions; i++ ) {
		MX_DEBUG( 8,(
			"%s:   dimension[%ld] = %ld, element size[%ld] = %ld",
			fname, i, dimension_array[i],
			i, (long) data_element_size_array[i]));
	}

	current_dimension_element_size
				= data_element_size_array[num_dimensions - 1];

	if ( num_dimensions == 1 ) {
		if ( array_pointer == NULL ) {
			return mx_error( MXE_CORRUPT_DATA_STRUCTURE, fname,
				"array_pointer at address %p is NULL.",
				&array_pointer );
		}

		/* Finally, let's free the memory pointed to by the pointer. */

		MX_DEBUG( 8,( "%s: About to free pointer array_pointer = %p",
			fname, array_pointer ));

		free( array_pointer );

		/* Set the pointer to NULL, so that we will know that it
		 * contains an invalid pointer value.
		 */

		array_pointer = NULL;

		/* free() is defined to have a void return value in ANSI C,
		 * so there really isn't any way to confirm that things
		 * worked correctly.
		 */
	} else {
		for ( i = 0; i < dimension_array[0]; i++ ) {
			array_element_pointer = (char *)array_pointer
					+ i * current_dimension_element_size;

			if ( array_element_pointer == NULL ) {
				return mx_error(
					MXE_CORRUPT_DATA_STRUCTURE, fname, 
				"array_element_pointer at address %p is NULL.",
					&array_element_pointer );
			}

			subarray_pointer
				= mx_read_void_pointer_from_memory_location(
						array_element_pointer );

			if ( subarray_pointer == NULL ) {
				return mx_error(
					MXE_CORRUPT_DATA_STRUCTURE, fname, 
				"subarray_pointer at address %p is NULL.",
					&subarray_pointer );
			}

			MX_DEBUG( 8,
		("%s: array_element_pointer = %p, subarray_pointer = %p",
			fname, array_element_pointer, subarray_pointer ));

			mx_status = mx_free_array( subarray_pointer,
				num_dimensions - 1, &dimension_array[1],
				data_element_size_array );

			/*If freeing the subarray didn't work, then give up.*/

			if ( mx_status.code != MXE_SUCCESS )
				return mx_status;
		}
	}

	return MX_SUCCESSFUL_RESULT;
}

/* The purpose of the following function is to interpret the value
 * stored at a memory location pointed to by a void * as if it were
 * itself an address.
 */

MX_EXPORT void *
mx_read_void_pointer_from_memory_location( void *ptr )
{
	/* As far as I know, the only assumption I am making here is
	 * that the internal representation of a void * is the same
	 * as the internal representation of a void **.  This is
	 * reasonably safe (I think).
	 */

	union {
		void *ptr_to_void;
		void **ptr_to_ptr_to_void;
	} u;

	void **result;

	u.ptr_to_void = ptr;

	result = *( u.ptr_to_ptr_to_void );

	MX_DEBUG( 9, ("readvp: read pointer %p from memory location %p",
				result, ptr));

	return result;
}

/* Similarly, the following function writes an address value "ptr"
 * to a memory location pointed to by the void pointer "memory_location".
 */

MX_EXPORT void
mx_write_void_pointer_to_memory_location( void *memory_location, void *ptr )
{
	union {
		void *ptr_to_void;
		void **ptr_to_ptr_to_void;
	} u;

	u.ptr_to_void = memory_location;

	*( u.ptr_to_ptr_to_void ) = ptr;

	MX_DEBUG( 9, ("writevp: wrote pointer %p to memory location %p",
					ptr, memory_location));

	return;
}

static size_t
mxp_scalar_element_size( long mx_datatype ) {
	size_t element_size;

	switch( mx_datatype ) {
	case MXFT_INT8:       element_size = sizeof(int8_t);            break;
	case MXFT_UINT8:      element_size = sizeof(uint8_t);           break;
	case MXFT_INT16:      element_size = sizeof(int16_t);           break;
	case MXFT_UINT16:     element_size = sizeof(uint16_t);          break;
	case MXFT_INT32:      element_size = sizeof(int32_t);           break;
	case MXFT_UINT32:     element_size = sizeof(uint32_t);          break;

	case MXFT_FLOAT:      element_size = sizeof(float);             break;
	case MXFT_DOUBLE:     element_size = sizeof(double);            break;

	case MXFT_HEX:        element_size = sizeof(uint32_t);          break;
	case MXFT_CHAR:       element_size = sizeof(char);              break;

	case MXFT_INT64:      element_size = sizeof(int64_t);           break;
	case MXFT_UINT64:     element_size = sizeof(uint64_t);          break;

	case MXFT_RECORD:     element_size = MXU_RECORD_NAME_LENGTH;    break;
	case MXFT_RECORDTYPE: element_size = MXU_DRIVER_NAME_LENGTH;    break;
	case MXFT_INTERFACE:  element_size = MXU_INTERFACE_NAME_LENGTH; break;

	case MXFT_OLD_LONG:   element_size = sizeof(int32_t);           break;
	case MXFT_OLD_ULONG:  element_size = sizeof(uint32_t);          break;
	default:              element_size = 0;                         break;
	}

	return element_size;
}

MX_EXPORT mx_status_type
mx_copy_array_to_buffer( void *array_pointer,
		int array_is_dynamically_allocated,
		long mx_datatype, long num_dimensions,
		long *dimension_array, size_t *data_element_size_array,
		void *destination_buffer, size_t destination_buffer_length,
		size_t *num_bytes_copied )
{
	static const char fname[] = "mx_copy_array_to_buffer()";

	MX_RECORD *mx_record;
	MX_DRIVER *mx_driver;
	MX_INTERFACE *mx_interface;
	char *array_row_pointer, *destination_pointer;
	size_t bytes_to_copy, array_size, subarray_size, buffer_left;
	long i, n;
	int num_subarray_elements;
	int return_structure_name, structure_name_length;
	mx_status_type mx_status;

	if (( array_pointer == NULL ) || ( dimension_array == NULL )
	 || (data_element_size_array == NULL) || (destination_buffer == NULL))
	{
		return mx_error( MXE_NULL_ARGUMENT, fname,
	    "One or more of the pointers passed to this function were NULL." );
	}

	if ( num_bytes_copied != NULL ) {
		*num_bytes_copied = 0;
	}

	if ( num_dimensions < 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The number of dimensions passed (%ld) is a negative number.",
			num_dimensions );
	}

	/* For datatypes that correspond to structures, we report the name
	 * of the structure, for example record->name, rather than something
	 * like a pointer to the data structure
	 */

	switch( mx_datatype ) {
	case MXFT_RECORD:
		structure_name_length = MXU_RECORD_NAME_LENGTH + 1;
		return_structure_name = TRUE;
		break;
	case MXFT_RECORDTYPE:
		structure_name_length = MXU_DRIVER_NAME_LENGTH + 1;
		return_structure_name = TRUE;
		break;
	case MXFT_INTERFACE:
		structure_name_length = MXU_INTERFACE_NAME_LENGTH + 1;
		return_structure_name = TRUE;
		break;
	default:
		structure_name_length = 0;
		return_structure_name = FALSE;
		break;
	}

#if 0
	MX_DEBUG(-2,("%s: *** array_pointer = %p", fname, array_pointer));
	MX_DEBUG(-2,("%s: mx_datatype = %ld", fname, mx_datatype));
	MX_DEBUG(-2,("%s: num_dimensions = %ld", fname, num_dimensions));
	MX_DEBUG(-2,("%s: dimension_array = %p", fname, dimension_array));
	MX_DEBUG(-2,("%s: data_element_size_array = %p",
					fname, data_element_size_array));
	MX_DEBUG(-2,("%s: destination_buffer = %p", fname, destination_buffer));
#endif

	if ( num_dimensions == 0 ) {

		/* Handling scalars takes a bit more effort. */

		bytes_to_copy = mxp_scalar_element_size( mx_datatype );

		if ( bytes_to_copy > destination_buffer_length ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The scaler of size %ld bytes is too large "
			"to fit into the destination buffer of %ld bytes.",
				(long) bytes_to_copy,
				(long) destination_buffer_length );
		}

#if 0
		MX_DEBUG(-2,
	("%s: num_dimensions = 0, mx_datatype = %ld, bytes_to_copy = %d",
			fname, mx_datatype, bytes_to_copy));
#endif

		switch( mx_datatype ) {
		case MXFT_STRING:
			return mx_error( MXE_UNSUPPORTED, fname,
				"MX does not support 0-dimensional strings." );
			break;
		case MXFT_INT8:
		case MXFT_UINT8:
		case MXFT_INT16:
		case MXFT_UINT16:
		case MXFT_INT32:
		case MXFT_UINT32:
		case MXFT_FLOAT:
		case MXFT_DOUBLE:
		case MXFT_HEX:
		case MXFT_CHAR:
		case MXFT_OLD_LONG:
		case MXFT_OLD_ULONG:
			memcpy( destination_buffer,
					array_pointer, bytes_to_copy );
			break;

		case MXFT_RECORD:
			mx_record = (MX_RECORD *) array_pointer;

			destination_pointer = (char *) destination_buffer;

			strlcpy( destination_pointer, mx_record->name,
					MXU_RECORD_NAME_LENGTH );
			break;
		case MXFT_RECORDTYPE:
			mx_driver = (MX_DRIVER *) array_pointer;

			destination_pointer = (char *) destination_buffer;

			strlcpy( destination_pointer, mx_driver->name,
					MXU_DRIVER_NAME_LENGTH );
			break;
		case MXFT_INTERFACE:
			mx_interface = (MX_INTERFACE *) array_pointer;

			destination_pointer = (char *) destination_buffer;

			strlcpy( destination_pointer,
					mx_interface->address_name,
					MXU_INTERFACE_ADDRESS_NAME_LENGTH );
			break;

		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Array copy for data type %ld is not supported.",
				mx_datatype );
			break;
		}

		if ( num_bytes_copied != NULL ) {
			*num_bytes_copied = bytes_to_copy;
		}

		return MX_SUCCESSFUL_RESULT;
	}

	/* NOTE: 1-dimensional arrays of structures are reported as if they
	 * were 2-dimensional arrays of strings, where the strings are the
	 * 'names' of the structures, for example record->name.  Thus, the
	 * ( num_dimensions == 1 ) case below should never be invoked for
	 * complex structures.
	 */

	if ( ( num_dimensions == 1 ) && ( return_structure_name == FALSE ) ) {

		bytes_to_copy = dimension_array[0] * data_element_size_array[0];

		if ( mx_datatype == MXFT_STRING ) {
			bytes_to_copy++;
		}

		if ( bytes_to_copy > destination_buffer_length ) {
			return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The 1-dimensional array of size %ld bytes is "
			"too large to fit into the destination buffer "
			"of %ld bytes.",
				(long) bytes_to_copy,
				(long) destination_buffer_length );
		}

		if ( mx_datatype == MXFT_STRING ) {
			strlcpy( (char *) destination_buffer,
					array_pointer, bytes_to_copy );
		} else {
			memcpy( destination_buffer,
					array_pointer, bytes_to_copy );
		}

		if ( num_bytes_copied != NULL ) {
			*num_bytes_copied = bytes_to_copy;
		}

		return MX_SUCCESSFUL_RESULT;
	}

	/* If we get here, either 'num_dimensions > 1' or this array is 
	 * a structure array for which we report the name instead.
	 */

	num_subarray_elements = 1;

	for ( i = 1; i < num_dimensions; i++ ) {
		num_subarray_elements *= ( dimension_array[i] );
	}

	if ( return_structure_name ) {
		/* This is a structure array. */

		subarray_size = num_subarray_elements * structure_name_length;
	} else {
		/* num_dimensions > 1 */

		subarray_size = num_subarray_elements
					* data_element_size_array[0];
	}

	array_size = subarray_size * dimension_array[0];

#if 0
	switch( mx_datatype ) {
	default:
		MX_DEBUG(-2,(
			"%s: mx_datatype = %ld, num_subarray_elements = %d",
			fname, mx_datatype, num_subarray_elements));
		MX_DEBUG(-2,("%s: subarray_size = %ld, array_size = %ld",
			fname, (long) subarray_size, (long) array_size));
	}
#endif

	if ( array_size > destination_buffer_length ) {
		return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The %ld-dimensional array of size %ld bytes is "
			"too large to fit into the destination buffer "
			"of %ld bytes.",
				num_dimensions,
				(long) array_size,
				(long) destination_buffer_length );
	}

	for ( n = 0; n < dimension_array[0]; n++ ) {

		array_row_pointer = (char *) array_pointer
			+ n * data_element_size_array[num_dimensions - 1];

		if ( array_is_dynamically_allocated ) {
			array_row_pointer = (char *)
				mx_read_void_pointer_from_memory_location(
					array_row_pointer );
		}

		destination_pointer = (char *) destination_buffer
			+ n * subarray_size;

		buffer_left = destination_buffer_length - n * subarray_size;

		mx_status = mx_copy_array_to_buffer( array_row_pointer,
				array_is_dynamically_allocated,
				mx_datatype, num_dimensions - 1,
				&dimension_array[1], data_element_size_array,
				destination_pointer, buffer_left,
				NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	if ( num_bytes_copied != NULL ) {
		*num_bytes_copied = array_size;
	}

	return MX_SUCCESSFUL_RESULT;
}

MX_EXPORT mx_status_type
mx_copy_buffer_to_array( void *source_buffer, size_t source_buffer_length,
		void *array_pointer,
		int array_is_dynamically_allocated,
		long mx_datatype, long num_dimensions,
		long *dimension_array, size_t *data_element_size_array,
		size_t *num_bytes_copied )
{
	static const char fname[] = "mx_copy_buffer_to_array()";

	char *array_row_pointer, *source_pointer;
	size_t bytes_to_copy, array_size, subarray_size, buffer_left;
	long i, n;
	int num_subarray_elements;
	mx_status_type mx_status;

	if (( array_pointer == NULL ) || ( dimension_array == NULL )
	 || ( data_element_size_array == NULL ) || ( source_buffer == NULL ))
	{
		return mx_error( MXE_NULL_ARGUMENT, fname,
	    "One or more of the pointers passed to this function were NULL." );
	}

	/* Disallow writes to complex field types since correctly supporting
	 * writes to these datatypes would be very difficult, especially for
	 * MXFT_RECORDTYPE, and not worth the effort required to implement
	 * such a thing.
	 */

	switch( mx_datatype ) {
	case MXFT_RECORD:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Writing to an MXFT_RECORD field is not allowed." );
		break;
	case MXFT_RECORDTYPE:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Writing to an MXFT_RECORDTYPE field is not allowed." );
		break;
	case MXFT_INTERFACE:
		return mx_error( MXE_UNSUPPORTED, fname,
			"Writing to an MXFT_INTERFACE field is not allowed." );
		break;
	}

	if ( num_bytes_copied != NULL ) {
		*num_bytes_copied = 0;
	}

	if ( num_dimensions < 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The number of dimensions passed (%ld) is a negative number.",
			num_dimensions );
	}

#if 0
	MX_DEBUG(-2,("%s: *** array_pointer = %p", fname, array_pointer));
	MX_DEBUG(-2,("%s: mx_datatype = %ld", fname, mx_datatype));
	MX_DEBUG(-2,("%s: num_dimensions = %ld", fname, num_dimensions));
	MX_DEBUG(-2,("%s: dimension_array = %p", fname, dimension_array));
	MX_DEBUG(-2,("%s: data_element_size_array = %p",
					fname, data_element_size_array));
	MX_DEBUG(-2,("%s: source_buffer = %p", fname, source_buffer));
#endif

	if ( num_dimensions == 0 ) {

		/* Handling scalars takes a bit more effort. */

		bytes_to_copy = mxp_scalar_element_size( mx_datatype );

		if ( bytes_to_copy > source_buffer_length ) {
			return mx_error( MXE_UNEXPECTED_END_OF_DATA, fname,
			"The size (%ld bytes) of the scalar value for this "
			"array is larger than the source buffer length "
			"of %ld bytes.",
				(long) bytes_to_copy,
				(long) source_buffer_length );
		}

#if 0
		MX_DEBUG(-2,
	("%s: num_dimensions = 0, mx_datatype = %ld, bytes_to_copy = %d",
			fname, mx_datatype, bytes_to_copy));
#endif

		switch( mx_datatype ) {
		case MXFT_STRING:
			return mx_error( MXE_UNSUPPORTED, fname,
				"MX does not support 0-dimensional strings." );
			break;
		case MXFT_INT8:
		case MXFT_UINT8:
		case MXFT_INT16:
		case MXFT_UINT16:
		case MXFT_INT32:
		case MXFT_UINT32:
		case MXFT_FLOAT:
		case MXFT_DOUBLE:
		case MXFT_HEX:
		case MXFT_CHAR:
		case MXFT_OLD_LONG:
		case MXFT_OLD_ULONG:
			memcpy( array_pointer, source_buffer, bytes_to_copy );
			break;

		default:
			return mx_error( MXE_UNSUPPORTED, fname,
			"Array copy for data type %ld is not supported.",
				mx_datatype );
			break;
		}

		if ( num_bytes_copied != NULL ) {
			*num_bytes_copied = bytes_to_copy;
		}

		return MX_SUCCESSFUL_RESULT;
	}

	if ( num_dimensions == 1 ) {

		array_size = dimension_array[0] * data_element_size_array[0];

		bytes_to_copy = source_buffer_length;

		if ( mx_datatype == MXFT_STRING ) {
			bytes_to_copy++;
		}

		if ( bytes_to_copy > array_size ) {
			bytes_to_copy = array_size;
		}

		if ( mx_datatype == MXFT_STRING ) {
			strlcpy( (char *) array_pointer,
					source_buffer, bytes_to_copy );
		} else {
			memcpy( array_pointer, source_buffer, bytes_to_copy );
		}

		if ( num_bytes_copied != NULL ) {
			*num_bytes_copied = bytes_to_copy;
		}

		return MX_SUCCESSFUL_RESULT;
	}

	/* num_dimensions > 1 */

	num_subarray_elements = 1;

	for ( i = 1; i < num_dimensions; i++ ) {
		num_subarray_elements *= ( dimension_array[i] );
	}

	subarray_size = num_subarray_elements * data_element_size_array[0];

	array_size = subarray_size * dimension_array[0];

	if ( array_size > source_buffer_length ) {
		return mx_error( MXE_UNEXPECTED_END_OF_DATA, fname,
			"The %ld-dimensional of size %ld bytes is larger "
			"than the source buffer of %ld bytes.",
				num_dimensions,
				(long) array_size,
				(long) source_buffer_length );
	}

	for ( n = 0; n < dimension_array[0]; n++ ) {

		array_row_pointer = (char *) array_pointer
			+ n * data_element_size_array[num_dimensions - 1];

		if ( array_is_dynamically_allocated ) {
			array_row_pointer = (char *)
				mx_read_void_pointer_from_memory_location(
					array_row_pointer );
		}

		source_pointer = (char *) source_buffer
			+ n * subarray_size;

		buffer_left = source_buffer_length - n * subarray_size;

		mx_status = mx_copy_buffer_to_array(
				source_pointer, buffer_left,
				array_row_pointer,
				array_is_dynamically_allocated,
				mx_datatype, num_dimensions - 1,
				&dimension_array[1], data_element_size_array,
				NULL );

		if ( mx_status.code != MXE_SUCCESS )
			return mx_status;
	}

	if ( num_bytes_copied != NULL ) {
		*num_bytes_copied = array_size;
	}

	return MX_SUCCESSFUL_RESULT;
}

#if HAVE_XDR

static size_t
mxp_xdr_scalar_element_size( long mx_datatype ) {
	size_t element_size, native_size;

	switch( mx_datatype ) {
	case MXFT_STRING:
		element_size = 1;
		break;
	case MXFT_INT8:
	case MXFT_UINT8:
	case MXFT_INT16:
	case MXFT_UINT16:
	case MXFT_INT32:
	case MXFT_UINT32:
	case MXFT_HEX:
	case MXFT_CHAR:
	case MXFT_OLD_LONG:
	case MXFT_OLD_ULONG:
	case MXFT_FLOAT:
		element_size = 4;
		break;
	case MXFT_INT64:
	case MXFT_UINT64:
	case MXFT_DOUBLE:
		element_size = 8;
		break;
	case MXFT_RECORD:
	case MXFT_RECORDTYPE:
	case MXFT_INTERFACE:
		native_size = mxp_scalar_element_size( mx_datatype );

		element_size = 4 * (native_size / 4);

		if ( ( native_size % 4 ) != 0 ) {
			element_size += 4;
		}
		break;
	default:
		element_size = 0;
		break;
	}

	return element_size;
}

MX_EXPORT mx_status_type
mx_xdr_data_transfer( int direction, void *array_pointer,
		int array_is_dynamically_allocated,
		long mx_datatype, long num_dimensions,
		long *dimension_array, size_t *data_element_size_array,
		void *xdr_buffer, size_t xdr_buffer_length,
		size_t *num_bytes_copied )
{
	static const char fname[] = "mx_xdr_data_transfer()";

	XDR xdrs;
	MX_RECORD *mx_record;
	MX_DRIVER *mx_driver;
	MX_INTERFACE *mx_interface;
	char *array_row_pointer, *xdr_ptr, *ptr;
	size_t subarray_size_product;
	size_t first_dimension_size, last_dimension_size;
	size_t last_dimension_native_size_in_bytes;
	size_t last_dimension_xdr_size_in_bytes;
	size_t native_data_size, native_array_size, native_subarray_size;
	size_t xdr_data_size, xdr_array_size, xdr_subarray_size;
	size_t buffer_left, remainder;
	unsigned long xdr_buffer_address, address_remainder;
	long i, n;
	int xdr_status;
	unsigned int num_array_elements;
	int return_structure_name, structure_name_length;
	enum xdr_op operation;
	mx_status_type mx_status;

	if (( array_pointer == NULL ) || ( dimension_array == NULL )
	 || (data_element_size_array == NULL) || (xdr_buffer == NULL))
	{
		return mx_error( MXE_NULL_ARGUMENT, fname,
	    "One or more of the pointers passed to this function were NULL." );
	}

	/* Verify that the xdr_buffer pointer is correctly aligned
	 * on a 4 byte address.
	 */

	xdr_buffer_address = (unsigned long) xdr_buffer;

	address_remainder = xdr_buffer_address % 4;

	if ( address_remainder != 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The 'xdr_buffer' pointer %p passed to this routine is not "
		"correctly aligned.  XDR buffer pointers must have an address "
		"that is a multiple of 4 bytes, since misaligned buffer "
		"addresses can cause segmentation faults on for some "
		"computer CPU types such as SPARC and MIPS.  "
		"For safety's sake, we do not allow misaligned buffer "
		"addresses to be used.",
			xdr_buffer );
	}

	if ( direction == MX_XDR_ENCODE ) {
		operation = XDR_ENCODE;
	} else {
		operation = XDR_DECODE;
	}

	if ( num_bytes_copied != NULL ) {
		*num_bytes_copied = 0;
	}

	if ( num_dimensions < 0 ) {
		return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
		"The number of dimensions passed (%ld) is a negative number.",
			num_dimensions );
	}

	/* For datatypes that correspond to structures, we report the name
	 * of the structure, for example record->name, rather than something
	 * like a pointer to the data structure
	 */

	switch( mx_datatype ) {
	case MXFT_RECORD:
		if ( direction == MX_XDR_DECODE ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"Writing to an MXFT_RECORD field is not allowed." );
		}

		structure_name_length = MXU_RECORD_NAME_LENGTH + 1;
		return_structure_name = TRUE;
		break;
	case MXFT_RECORDTYPE:
		if ( direction == MX_XDR_DECODE ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"Writing to an MXFT_RECORDTYPE field is not allowed." );
		}

		structure_name_length = MXU_DRIVER_NAME_LENGTH + 1;
		return_structure_name = TRUE;
		break;
	case MXFT_INTERFACE:
		if ( direction == MX_XDR_DECODE ) {
			return mx_error( MXE_UNSUPPORTED, fname,
			"Writing to an MXFT_INTERFACE field is not allowed." );
		}

		structure_name_length = MXU_INTERFACE_NAME_LENGTH + 1;
		return_structure_name = TRUE;
		break;
	default:
		structure_name_length = 0;
		return_structure_name = FALSE;
		break;
	}

#if 0
	MX_DEBUG(-2,("%s: *** direction = %d", fname, direction));
	MX_DEBUG(-2,("%s: *** array_pointer = %p", fname, array_pointer));
	MX_DEBUG(-2,("%s: mx_datatype = %ld", fname, mx_datatype));
	MX_DEBUG(-2,("%s: num_dimensions = %ld", fname, num_dimensions));
	MX_DEBUG(-2,("%s: dimension_array = %p", fname, dimension_array));
	MX_DEBUG(-2,("%s: data_element_size_array = %p",
					fname, data_element_size_array));
	MX_DEBUG(-2,("%s: xdr_buffer = %p", fname, xdr_buffer));
	MX_DEBUG(-2,("%s: xdr_buffer_length = %ld",
					fname, (long)xdr_buffer_length));
#endif

	if ( num_dimensions == 0 ) {

		/* Handling scalars takes a bit more effort. */

		native_data_size = mxp_scalar_element_size( mx_datatype );

		xdr_data_size = mxp_xdr_scalar_element_size( mx_datatype );

		if ( direction == MX_XDR_ENCODE ) {
			if ( xdr_data_size > xdr_buffer_length ) {
				return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The XDR scalar of size %ld bytes is too large "
			"to fit into the destination buffer of %ld bytes.",
					(long) xdr_data_size,
					(long) xdr_buffer_length );
			}
		} else {
			if ( xdr_buffer_length > xdr_data_size ) {
				return mx_error( MXE_WOULD_EXCEED_LIMIT, fname,
			"The source buffer of %ld bytes is too big to fit "
			"into the XDR scalar of size %ld bytes.",
					(long) xdr_buffer_length,
					(long) xdr_data_size );
			}
		}

		xdrmem_create( &xdrs,
				xdr_buffer,
				xdr_buffer_length,
				operation );

		switch( mx_datatype ) {
		case MXFT_STRING:
			xdr_destroy( &xdrs );

			return mx_error( MXE_UNSUPPORTED, fname,
				"MX does not support 0-dimensional strings." );
			break;
		case MXFT_INT8:
			xdr_status = xdr_int8_t( &xdrs, array_pointer );
			break;
		case MXFT_UINT8:
			xdr_status = xdr_uint8_t( &xdrs, array_pointer );
			break;
		case MXFT_INT16:
			xdr_status = xdr_int16_t( &xdrs, array_pointer );
			break;
		case MXFT_UINT16:
			xdr_status = xdr_uint16_t( &xdrs, array_pointer );
			break;
		case MXFT_INT32:
			xdr_status = xdr_int32_t( &xdrs, array_pointer );
			break;
		case MXFT_HEX:
		case MXFT_UINT32:
			xdr_status = xdr_uint32_t( &xdrs, array_pointer );
			break;
		case MXFT_INT64:
			xdr_status = xdr_int64_t( &xdrs, array_pointer );
			break;
		case MXFT_UINT64:
			xdr_status = xdr_uint64_t( &xdrs, array_pointer );
			break;
		case MXFT_FLOAT:
			xdr_status = xdr_float( &xdrs, array_pointer );
			break;
		case MXFT_DOUBLE:
			xdr_status = xdr_double( &xdrs, array_pointer );
			break;
		case MXFT_CHAR:
			xdr_status = xdr_char( &xdrs, array_pointer );
			break;
		case MXFT_OLD_LONG:
			xdr_status = xdr_long( &xdrs, array_pointer );
			break;
		case MXFT_OLD_ULONG:
			xdr_status = xdr_u_long( &xdrs, array_pointer );
			break;

		case MXFT_RECORD:
			mx_record = (MX_RECORD *) array_pointer;

			ptr = mx_record->name;

			xdr_status = xdr_string( &xdrs, &ptr,
					mxp_scalar_element_size( mx_datatype ));
			break;
		case MXFT_RECORDTYPE:
			mx_driver = (MX_DRIVER *) array_pointer;

			ptr = mx_driver->name;

			xdr_status = xdr_string( &xdrs, &ptr,
					mxp_scalar_element_size( mx_datatype ));
			break;
		case MXFT_INTERFACE:
			mx_interface = (MX_INTERFACE *) array_pointer;

			ptr = mx_interface->address_name;

			xdr_status = xdr_string( &xdrs, &ptr,
					mxp_scalar_element_size( mx_datatype ));
			break;

		default:
			xdr_destroy( &xdrs );

			return mx_error( MXE_UNSUPPORTED, fname,
			"Array copy for data type %ld is not supported.",
				mx_datatype );
			break;
		}

		xdr_destroy( &xdrs );

		if ( xdr_status == 1 ) {
			if ( num_bytes_copied != NULL ) {
				*num_bytes_copied = xdr_data_size;
			}

			return MX_SUCCESSFUL_RESULT;
		} else {
			if ( num_bytes_copied != NULL ) {
				*num_bytes_copied = 0;
			}

			return mx_error( MXE_FUNCTION_FAILED, fname,
				"XDR scalar data copy of datatype %ld failed.",
					mx_datatype );
		}
	}

#define XDR_DO_ARRAY(xdr_filter) \
		xdr_status = xdr_array( &xdrs, &ptr, \
					&num_array_elements, \
					num_array_elements, \
					data_element_size_array[0], \
					(xdrproc_t) (xdr_filter) )

	/* NOTE: 1-dimensional arrays of structures are reported as if they
	 * were 2-dimensional arrays of strings, where the strings are the
	 * 'names' of the structures, for example record->name.  Thus, the
	 * ( num_dimensions == 1 ) case below should never be invoked for
	 * complex structures.
	 */

	if ( ( num_dimensions == 1 ) && ( return_structure_name == FALSE ) ) {

		native_array_size = dimension_array[0]
					* data_element_size_array[0];

		xdr_array_size = dimension_array[0] 
				* mxp_xdr_scalar_element_size( mx_datatype );

		if ( mx_datatype == MXFT_STRING ) {
			/* Add one byte for the string terminator. */

			native_array_size++;

			num_array_elements = dimension_array[0] + 1;
		} else {
			num_array_elements = dimension_array[0];
		}

		xdr_array_size += 4;   /* Add space for the length field. */

		/* Round up xdr_array_size to the next multiple of 4. */

		remainder = xdr_array_size % 4;

		if ( remainder != 0 ) {
			xdr_array_size = 4 * ( 1 + xdr_array_size / 4 );
		}

		if ( direction == MX_XDR_ENCODE ) {
			if ( xdr_array_size > xdr_buffer_length ) {
				mx_warning(
			"The 1-dimensional XDR array of size %ld bytes was "
			"truncated to fit into the buffer length of %ld bytes.",
					(long) xdr_array_size,
					(long) xdr_buffer_length );

				xdr_array_size = xdr_buffer_length;
			}
		}

		xdrmem_create( &xdrs,
				xdr_buffer,
				xdr_buffer_length,
				operation );

		ptr = (char *) array_pointer;

		if ( mx_datatype == MXFT_STRING ) {
			xdr_status = xdr_string( &xdrs, &ptr,
						native_array_size );
		} else {
			switch( mx_datatype ) {
			case MXFT_INT8:      XDR_DO_ARRAY(xdr_int8_t);   break;
			case MXFT_UINT8:     XDR_DO_ARRAY(xdr_uint8_t);  break;
			case MXFT_INT16:     XDR_DO_ARRAY(xdr_int16_t);  break;
			case MXFT_UINT16:    XDR_DO_ARRAY(xdr_uint16_t); break;
			case MXFT_INT32:     XDR_DO_ARRAY(xdr_int32_t);  break;
			case MXFT_UINT32:    XDR_DO_ARRAY(xdr_uint32_t); break;
			case MXFT_INT64:     XDR_DO_ARRAY(xdr_int64_t);  break;
			case MXFT_UINT64:    XDR_DO_ARRAY(xdr_uint64_t); break;
			case MXFT_FLOAT:     XDR_DO_ARRAY(xdr_float);    break;
			case MXFT_DOUBLE:    XDR_DO_ARRAY(xdr_double);   break;
			case MXFT_HEX:       XDR_DO_ARRAY(xdr_uint32_t); break;
			case MXFT_CHAR:      XDR_DO_ARRAY(xdr_char);     break;
			case MXFT_OLD_LONG:  XDR_DO_ARRAY(xdr_long);     break;
			case MXFT_OLD_ULONG: XDR_DO_ARRAY(xdr_u_long);   break;
			default:
				xdr_destroy( &xdrs );

				return mx_error( MXE_ILLEGAL_ARGUMENT, fname,
	"Illegal MX datatype %ld for %ld-dimensional XDR data transfer.",
					mx_datatype, num_dimensions );
				break;
			}

		}

		xdr_destroy( &xdrs );

		if ( xdr_status == 1 ) {
			if ( num_bytes_copied != NULL ) {
				*num_bytes_copied = xdr_array_size;
			}

			return MX_SUCCESSFUL_RESULT;
		} else {
			if ( num_bytes_copied != NULL ) {
				*num_bytes_copied = 0;
			}

			return mx_error( MXE_FUNCTION_FAILED, fname,
			"XDR 1-dimensional data copy of datatype %ld failed.",
					mx_datatype );
		}
	}

	/* num_dimensions > 1 */

	/* If we get here, either 'num_dimensions > 1' or this array is
	 * a structure array for which we report the name instead.
	 */

	/* Compute some shared values used to calculate both the
	 * native and XDR sizes of the array and subarrays.
	 *
	 * Note that we stop one step short of the last dimension.
	 * This is due to the fact that for the XDR size calculation
	 * we must adjust the size of the last dimension to take into
	 * account the 4 bytes necessary for the length field and the
	 * necessity to round the length up to an even multiple of
	 * 4 bytes.
	 */

	subarray_size_product = 1;

	for ( i = 1; i < num_dimensions - 1; i++ ) {
		subarray_size_product *= ( dimension_array[i] );
	}

	first_dimension_size = dimension_array[0];
	last_dimension_size  = dimension_array[num_dimensions - 1];

	/* Compute the native and XDR sizes of the array and subarrays. */

	if ( return_structure_name ) {
		/* This is a structure array. */

		last_dimension_native_size_in_bytes = 
					mxp_scalar_element_size(mx_datatype);

		last_dimension_xdr_size_in_bytes =
				mxp_xdr_scalar_element_size( mx_datatype );
	} else {
		/* num_dimensions > 1 */

		last_dimension_native_size_in_bytes = last_dimension_size
						* data_element_size_array[0];

		last_dimension_xdr_size_in_bytes = last_dimension_size
				* mxp_xdr_scalar_element_size( mx_datatype );
	
	}

	native_subarray_size = subarray_size_product
				* last_dimension_native_size_in_bytes;

	native_array_size = native_subarray_size * first_dimension_size;

	/* Add 4 for the length field. */

	last_dimension_xdr_size_in_bytes += 4;

	/* Round up the size to the next multiple of 4. */

	remainder = last_dimension_xdr_size_in_bytes % 4;

	if ( remainder != 0 ) {
		last_dimension_xdr_size_in_bytes =
			4 * ( 1 + last_dimension_xdr_size_in_bytes / 4 );
	}

	xdr_subarray_size = subarray_size_product
				* last_dimension_xdr_size_in_bytes;

	xdr_array_size = xdr_subarray_size * first_dimension_size;

#if 0
	MX_DEBUG(-2,("%s: last_dimension_size = %ld",
			fname, (long) last_dimension_size));

	MX_DEBUG(-2,("%s: last_dimension_native_size_in_bytes = %ld",
			fname, (long) last_dimension_native_size_in_bytes));

	MX_DEBUG(-2,("%s: last_dimension_xdr_size_in_bytes = %ld",
			fname, (long) last_dimension_xdr_size_in_bytes));

	MX_DEBUG(-2,("%s: xdr_subarray_size = %ld, xdr_array_size = %ld",
			fname, (long) xdr_subarray_size, (long)xdr_array_size));
#endif

	if ( direction == MX_XDR_ENCODE ) {
		if ( xdr_array_size > xdr_buffer_length ) {
			mx_warning(
			"The %ld-dimensional XDR array of size %ld bytes was "
			"truncated to %ld bytes.",
				num_dimensions,
				(long) xdr_array_size,
				(long) xdr_buffer_length );

			xdr_array_size = xdr_buffer_length;
		}
	}

	xdrmem_create( &xdrs,
			xdr_buffer,
			xdr_buffer_length,
			operation );

	for ( n = 0; n < dimension_array[0]; n++ ) {

		array_row_pointer = (char *) array_pointer
			    + n * data_element_size_array[num_dimensions - 1];

		if ( array_is_dynamically_allocated ) {
			array_row_pointer = (char *)
				mx_read_void_pointer_from_memory_location(
					array_row_pointer );
		}

		xdr_ptr = (char *) xdr_buffer + n * xdr_subarray_size;

		buffer_left = xdr_buffer_length - n * xdr_subarray_size;

		mx_status = mx_xdr_data_transfer( direction,
				array_row_pointer,
				array_is_dynamically_allocated,
				mx_datatype, num_dimensions - 1,
				&dimension_array[1], data_element_size_array,
				xdr_ptr, buffer_left,
				NULL );

		if ( mx_status.code != MXE_SUCCESS ) {
			xdr_destroy( &xdrs );

			return mx_status;
		}
	}

	if ( num_bytes_copied != NULL ) {
		*num_bytes_copied = xdr_array_size;
	}

	return MX_SUCCESSFUL_RESULT;
}

#endif /* HAVE_XDR */

/* mx_convert_and_copy_array() is designed to copy from one MX-compatible
 * array to another MX-compatible array and convert from one datatype to
 * another datatype along the way.
 *
 * FIXME - Very little of this function has been implemented as of yet.
 *         The only supported cases at the moment are 0-dimensional to
 *         1-dimensional and 1-dimensional to 0-dimensional.  Only ints
 *         and doubles are currently supported.
 *
 *         At the moment, only enough of this function is implemented as
 *         is needed to support the 'record_field_motor' driver.
 */

MX_EXPORT mx_status_type
mx_convert_and_copy_array(
	void *source_array_pointer,
	long source_datatype,
	long source_num_dimensions,
	long *source_dimension_array,
	size_t *source_data_element_size_array,
	void *destination_array_pointer,
	long destination_datatype,
	long destination_num_dimensions,
	long *destination_dimension_array,
	size_t *destination_data_element_size_array )
{
	static const char fname[] = "mx_convert_and_copy_array()";

	int32_t int32_source_value;
	int32_t *int32_dest_ptr;
	double double_source_value;
	double *double_dest_ptr;

	if ( ( source_num_dimensions == 0 )
	  && ( destination_num_dimensions == 1 ) )
	{
		/* This case is supported. */
	} else
	if ( ( source_num_dimensions == 1 )
	  && ( destination_num_dimensions == 0 ) )
	{
		/* This case is also supported. */
	} else {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		"Copying from a %ld-dimensional array to a %ld-dimensional "
		"array is not yet implemented.",
			source_num_dimensions, destination_num_dimensions );
	}

	if ( source_datatype == MXFT_INT32 ) {
		int32_source_value = *((int32_t *) source_array_pointer);

		if ( destination_datatype == MXFT_INT32 ) {
			int32_dest_ptr = (int32_t *) destination_array_pointer;

			*int32_dest_ptr = int32_source_value;
		} else
		if ( destination_datatype == MXFT_DOUBLE ) {
			double_dest_ptr = (double *) destination_array_pointer;

			*double_dest_ptr = (double) int32_source_value;
		} else {
			return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		    "MX datatype %ld is not yet implemented for this function.",
				destination_datatype );
		}
	} else
	if ( source_datatype == MXFT_DOUBLE ) {
		double_source_value = *((double *) source_array_pointer);

		if ( destination_datatype == MXFT_INT32 ) {
			int32_dest_ptr = (int32_t *) destination_array_pointer;

			*int32_dest_ptr = mx_round( double_source_value );
		} else
		if ( destination_datatype == MXFT_DOUBLE ) {
			double_dest_ptr = (double *) destination_array_pointer;

			*double_dest_ptr = double_source_value;
		} else {
			return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		    "MX datatype %ld is not yet implemented for this function.",
				destination_datatype );
		}
	} else {
		return mx_error( MXE_NOT_YET_IMPLEMENTED, fname,
		    "MX datatype %ld is not yet implemented for this function.",
			destination_datatype );
	}

	return MX_SUCCESSFUL_RESULT;
}

