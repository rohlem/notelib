#ifndef NOTELIB_INTERNAL_CHANNEL_H_
#define NOTELIB_INTERNAL_CHANNEL_H_

#include "../notelib.h"
#include "alignment_utilities.h"

#include <stdalign.h>

#define NOTELIB_INSTRUMENT_STATE_DATA_ALIGNMENT alignof(max_align_t)

struct notelib_channel{
	notelib_note_id_uint current_note_id;
	alignas(NOTELIB_INSTRUMENT_STATE_DATA_ALIGNMENT) unsigned char data[];
};

enum {NOTELIB_CHANNEL_OFFSETOF_DATA = NOTELIB_INTERNAL_ALIGN_TO_NEXT(sizeof(struct notelib_channel), NOTELIB_INSTRUMENT_STATE_DATA_ALIGNMENT)};

#define NOTELIB_CHANNEL_SIZEOF_SINGLE(CHANNEL_DATA_SIZE) NOTELIB_INTERNAL_ALIGN_TO_NEXT_ALIGNOF(NOTELIB_CHANNEL_OFFSETOF_DATA + CHANNEL_DATA_SIZE, struct notelib_channel)

#endif//#ifndef NOTELIB_INTERNAL_CHANNEL_H_
