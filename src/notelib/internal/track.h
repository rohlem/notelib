#ifndef NOTELIB_INTERNAL_TRACK_H_
#define NOTELIB_INTERNAL_TRACK_H_

#include "../notelib.h"
#include "circular_buffer.h"
#include "internals_fwd.h"

#include <stddef.h>
#include <stdint.h>

struct notelib_track{
	ALIGNAS_MAX
	(struct circular_buffer_liberal_reader_unsynchronized,
	 struct circular_buffer_liberal_reader_unsynchronized*)
	notelib_position tempo_ceil_interval;
	notelib_position position;
	notelib_sample_uint tempo_ceil_interval_samples;
	notelib_sample_uint position_sample_offset;
	uint32_t initialized_channel_buffer_size;
	struct circular_buffer command_queue;
	//union of struct circular_buffer_liberal_reader_unsynchronized inline_initialized_channel_buffer and circular_buffer_liberal_reader_unsynchronized* external_initialized_channel_buffer ; size in no way enforced
};

notelib_sample_uint notelib_track_get_tempo_interval(const struct notelib_track* track, notelib_position first_position, notelib_position last_position);

notelib_position notelib_track_get_position_change(const struct notelib_track* track, notelib_position starting_position, notelib_sample_uint sample_interval);

static const size_t notelib_tracks_offsetof_command_queue = offsetof(struct notelib_track, command_queue);

struct circular_buffer_liberal_reader_unsynchronized*  notelib_track_get_inline_initialized_channel_buffer(struct notelib_track* track_ptr, uint16_t queued_command_count);

struct circular_buffer_liberal_reader_unsynchronized** notelib_track_get_external_initialized_channel_buffer_ptr(struct notelib_track* track_ptr, uint16_t queued_command_count);

struct circular_buffer_liberal_reader_unsynchronized* notelib_track_get_initialized_channel_buffer_ptr(const struct notelib_internals* internals, struct notelib_track* track_ptr);

#endif//#ifndef NOTELIB_INTERNAL_TRACK_H_