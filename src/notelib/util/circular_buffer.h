#ifndef NOTELIB_UTIL_CIRCULAR_BUFFER_H_
#define NOTELIB_UTIL_CIRCULAR_BUFFER_H_

#include "circular_buffer_interface.h"

#include "../internal/circular_buffer.h"

#include <stdalign.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifndef NOTELIB_UTIL_CIRCULAR_BUFFER_DATA_ALIGNMENT
#define NOTELIB_UTIL_CIRCULAR_BUFFER_DATA_ALIGNMENT alignof(max_align_t)
#endif//#ifndef NOTELIB_UTIL_CIRCULAR_BUFFER_DATA_ALIGNMENT

///These are two thread-safe lock-free queue-like data structures.
///Please note: Due to a current design flaw/oddity (it's not that big of a deal) they require one buffer element, and so don't support taking advantage of all their data space ("empty" and "full" states were not differentiated).

//Data size is always an integer multiple of element_size and only one element can be read and written at a time => guarantees contiguous memory.
struct circular_buffer{
	//customization idea: "perfect solution" algorithm (halving max element_count) by "collaborative" "everything written" bit of XORed top bits of both positions
	_Atomic uint16_t reader_position;
	_Atomic uint16_t writer_position;
	uint16_t  element_size;
	uint16_t element_count;
	alignas(NOTELIB_UTIL_CIRCULAR_BUFFER_DATA_ALIGNMENT)
	unsigned char data[];
};

enum {circular_buffer_header_size =
	offsetof(struct circular_buffer, data)};

//Data size is arbitrary; reading always happens by memcpy-ing into a caller-provided buffer.
struct circular_buffer_liberal_reader_unsynchronized{
	_Atomic uint32_t reader_position;
	uint32_t writer_position;
	uint32_t data_size;
	unsigned char data[];
};

enum {circular_buffer_liberal_reader_unsynchronized_header_size =
	offsetof(struct circular_buffer_liberal_reader_unsynchronized, data)};

#endif//#ifndef NOTELIB_UTIL_CIRCULAR_BUFFER_H_
