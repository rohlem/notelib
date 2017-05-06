#ifndef NOTELIB_INTERNAL_CIRCULAR_BUFFER_H_
#define NOTELIB_INTERNAL_CIRCULAR_BUFFER_H_

#include "command.h"

#define NOTELIB_UTIL_CIRCULAR_BUFFER_DATA_ALIGNMENT alignof(struct notelib_command)

#include "../util/circular_buffer.h"

//#undef NOTELIB_UTIL_CIRCULAR_BUFFER_DATA_ALIGNMENT

#endif//#ifndef NOTELIB_INTERNAL_CIRCULAR_BUFFER_H_
