#ifndef NOTELIB_UTIL_CIRCULAR_BUFFER_H_
#define NOTELIB_UTIL_CIRCULAR_BUFFER_H_

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

//Data size is always an integer multiple of element_size; only one element can be read and written at a time.
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

size_t circular_buffer_sizeof(uint16_t element_size, uint16_t element_count);
size_t circular_buffer_dynamic_sizeof(const struct circular_buffer* object);

struct circular_buffer* circular_buffer_construct
(void* position, uint32_t space, uint16_t element_size);

bool        circular_buffer_read
(      struct circular_buffer* object,       void* dest);
const void* circular_buffer_direct_read_commence
(const struct circular_buffer* object);
void        circular_buffer_direct_read_commit
(      struct circular_buffer* object);
bool        circular_buffer_write
(      struct circular_buffer* object, const void* src);

//Data size is arbitrary; reading always happens by memcpy-ing into a caller-provided buffer.
struct circular_buffer_liberal_reader_unsynchronized{
	_Atomic uint32_t reader_position;
	uint32_t writer_position;
	uint32_t data_size;
	unsigned char data[];
};

enum {circular_buffer_liberal_reader_unsynchronized_header_size =
	offsetof(struct circular_buffer_liberal_reader_unsynchronized, data)};

size_t circular_buffer_liberal_reader_unsynchronized_sizeof
(uint32_t data_size);
size_t circular_buffer_liberal_reader_unsynchronized_dynamic_sizeof
(const struct circular_buffer_liberal_reader_unsynchronized* object);

struct circular_buffer_liberal_reader_unsynchronized*
circular_buffer_liberal_reader_unsynchronized_construct
(void* position, size_t space, uint32_t data_size);

void circular_buffer_liberal_reader_unsynchronized_read
(struct circular_buffer_liberal_reader_unsynchronized* object,
 uint32_t size,   void* dest);
bool circular_buffer_liberal_reader_unsynchronized_write
(struct circular_buffer_liberal_reader_unsynchronized* object,
 const void* src, uint32_t size);
/*Although unwrite returns a bool it basically only contains a sanity check;
  if it returns false there is an inconsistency in the system
  and it is in what is pretty much undefined state.*/
bool circular_buffer_liberal_reader_unsynchronized_unwrite
(struct circular_buffer_liberal_reader_unsynchronized* object, uint32_t size);

#endif//#ifndef NOTELIB_UTIL_CIRCULAR_BUFFER_H_
