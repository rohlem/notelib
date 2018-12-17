#ifndef NOTELIB_INTERNAL_TRACK_H_
#define NOTELIB_INTERNAL_TRACK_H_

#include "../notelib.h"
#include "alignment_utilities.h"
#include "circular_buffer.h"
#include "internals_fwd.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

//#define NOTELIB_TRACK_GET_TEMPO_INTERVAL_DISABLE_ARGUMENT_CHECKING

//#define NOTELIB_NO_IMMEDIATE_TRACK

#ifndef NOTELIB_NO_IMMEDIATE_TRACK
//note: only ever used for internal identification
#define NOTELIB_IMMEDIATE_TRACK_INDEX NOTELIB_TRACK_UINT_MAX
#endif//#ifndef NOTELIB_NO_IMMEDIATE_TRACK

#define NOTELIB_TRACK_ALIGNMENT\
	ALIGNAS_MAX3\
	(struct circular_buffer,\
	 struct circular_buffer_liberal_reader_unsynchronized,\
	 struct circular_buffer_liberal_reader_unsynchronized*)

struct notelib_track_data{
	//NOT ALIGNED because the alignment requirement is only necessary at the top-level entity (here struct notelib_track and struct notelib_track_immediate); any intermediate alignment requirement might/would introduce padding.)
	uint32_t initialized_channel_buffer_size;
	//struct circular_buffer command_queue; //cannot declare struct with flexible array member as member of another struct (not even if it's the last member) according to language spec
	//union of struct circular_buffer_liberal_reader_unsynchronized inline_initialized_channel_buffer and circular_buffer_liberal_reader_unsynchronized* external_initialized_channel_buffer ; size in no way enforced
};

struct notelib_track{
	NOTELIB_TRACK_ALIGNMENT
	notelib_position tempo_ceil_interval;
	notelib_position position;
	notelib_sample_uint tempo_ceil_interval_samples;
	notelib_sample_uint position_sample_offset;
	struct notelib_track_data data;
};

#ifndef NOTELIB_NO_IMMEDIATE_TRACK
struct notelib_track_immediate{ //has/requires less state than regular tracks
	NOTELIB_TRACK_ALIGNMENT
	struct notelib_track_data data;
};
#endif//#ifndef NOTELIB_NO_IMMEDIATE_TRACK

bool notelib_track_is_disabled (const struct notelib_track* track);
void notelib_track_disable(struct notelib_track* track);
enum notelib_status notelib_track_regular_data_setup
(struct notelib_internals* internals, struct notelib_track* track_ptr, uint32_t initialized_channel_buffer_size);
enum notelib_status notelib_track_immediate_data_setup
(struct notelib_internals* internals, struct notelib_track_immediate* track_ptr, uint32_t initialized_channel_buffer_size);
//expects initialized_channel_buffer_size to already be initialized
enum notelib_status notelib_track_data_setup
(struct notelib_track_data* track_data,
 bool is_initialized_channel_buffer_inline,
 struct circular_buffer_liberal_reader_unsynchronized* inline_initialized_channel_buffer,
 struct circular_buffer_liberal_reader_unsynchronized** external_initialized_channel_buffer_ptr_ptr,
 size_t command_size, size_t sizeof_command_queue, struct circular_buffer* command_queue);
void notelib_track_base_cleanup(struct notelib_track* track);

notelib_sample_uint notelib_track_get_tempo_interval(const struct notelib_track* track, notelib_position first_position, notelib_position last_position);

notelib_position notelib_track_get_position_change(const struct notelib_track* track, notelib_position starting_position, notelib_sample_uint sample_interval);

static const size_t notelib_track_offsetof_command_queue = NOTELIB_INTERNAL_ALIGN_TO_NEXT_ALIGNOF(sizeof(struct notelib_track), struct circular_buffer);
#ifndef NOTELIB_NO_IMMEDIATE_TRACK
static const size_t notelib_track_immediate_offsetof_command_queue = NOTELIB_INTERNAL_ALIGN_TO_NEXT_ALIGNOF(sizeof(struct notelib_track_immediate), struct circular_buffer);
#endif//#ifndef NOTELIB_NO_IMMEDIATE_TRACK

struct circular_buffer*  notelib_track_get_command_queue(struct notelib_track* track_ptr);
#ifndef NOTELIB_NO_IMMEDIATE_TRACK
struct circular_buffer*  notelib_track_immediate_get_command_queue(struct notelib_track_immediate* track_ptr);
#endif//#ifndef NOTELIB_NO_IMMEDIATE_TRACK

struct circular_buffer_liberal_reader_unsynchronized*  notelib_track_get_inline_initialized_channel_buffer(struct notelib_track* track_ptr, uint16_t queued_command_count);
#ifndef NOTELIB_NO_IMMEDIATE_TRACK
struct circular_buffer_liberal_reader_unsynchronized*  notelib_track_immediate_get_inline_initialized_channel_buffer(struct notelib_track_immediate* track_ptr, uint16_t queued_command_count);
#endif//#ifndef NOTELIB_NO_IMMEDIATE_TRACK

struct circular_buffer_liberal_reader_unsynchronized** notelib_track_get_external_initialized_channel_buffer_ptr_ptr(struct notelib_track* track_ptr, uint16_t queued_command_count);
struct circular_buffer_liberal_reader_unsynchronized*  notelib_track_get_external_initialized_channel_buffer_ptr    (struct notelib_track* track_ptr, uint16_t queued_command_count);
#ifndef NOTELIB_NO_IMMEDIATE_TRACK
struct circular_buffer_liberal_reader_unsynchronized** notelib_track_immediate_get_external_initialized_channel_buffer_ptr_ptr(struct notelib_track_immediate* track_ptr, uint16_t queued_command_count);
struct circular_buffer_liberal_reader_unsynchronized*  notelib_track_immediate_get_external_initialized_channel_buffer_ptr    (struct notelib_track_immediate* track_ptr, uint16_t queued_command_count);
#endif//#ifndef NOTELIB_NO_IMMEDIATE_TRACK

struct circular_buffer_liberal_reader_unsynchronized* notelib_track_get_initialized_channel_buffer_ptr(const struct notelib_internals* internals, struct notelib_track* track_ptr);
#ifndef NOTELIB_NO_IMMEDIATE_TRACK
struct circular_buffer_liberal_reader_unsynchronized* notelib_track_immediate_get_initialized_channel_buffer_ptr(const struct notelib_internals* internals, struct notelib_track_immediate* track_ptr);
#endif//#ifndef NOTELIB_NO_IMMEDIATE_TRACK

#endif//#ifndef NOTELIB_INTERNAL_TRACK_H_
