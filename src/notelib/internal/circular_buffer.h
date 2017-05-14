#ifndef NOTELIB_INTERNAL_CIRCULAR_BUFFER_H_
#define NOTELIB_INTERNAL_CIRCULAR_BUFFER_H_

#include "command.h"

#ifdef __cplusplus
extern "C" {
#endif//#ifdef __cplusplus

#define NOTELIB_UTIL_CIRCULAR_BUFFER_DATA_ALIGNMENT alignof(struct notelib_command)

#include "../util/circular_buffer.h"

//#undef NOTELIB_UTIL_CIRCULAR_BUFFER_DATA_ALIGNMENT

#ifdef __cplusplus
}
#endif//#ifdef __cplusplus

#endif//#ifndef NOTELIB_INTERNAL_CIRCULAR_BUFFER_H_
