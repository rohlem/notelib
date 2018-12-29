#ifndef NOTELIB_UTIL_CIRCULAR_BUFFER_INTERFACE_H_
#define NOTELIB_UTIL_CIRCULAR_BUFFER_INTERFACE_H_

/*basically the .h without struct definitions; could technically be named _fwd I guess;
  its main use case is C++ interoperability, which doesn't understand _Atomic and would therefore screw up the structs' definitions*/

#ifdef __cplusplus
extern "C" {
#endif//#ifdef __cplusplus

#include <stdbool.h>
#include <stddef.h>
#include "stdint.h"


//Data size is always an integer multiple of element_size and only one element can be read and written at a time => guarantees contiguous memory.
struct circular_buffer;

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
struct circular_buffer_liberal_reader_unsynchronized;

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

#ifdef __cplusplus
}
#endif//#ifdef __cplusplus

#endif//#ifndef NOTELIB_UTIL_CIRCULAR_BUFFER_INTERFACE_H_
