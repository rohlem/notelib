#include "track.h"

#include "alignment_utilities.h"
#include "internals.h"

#include <stdint.h>
#include <stdio.h>

bool notelib_track_is_disabled (const struct notelib_track* track) {return track->tempo_ceil_interval_samples == 0;}
void notelib_track_disable(struct notelib_track* track) {track->tempo_ceil_interval_samples = 0;}

notelib_sample_uint notelib_track_get_tempo_interval(const struct notelib_track* track, notelib_position first_position, notelib_position last_position){
#ifdef NOTELIB_TRACK_GET_TEMPO_INTERVAL_DISABLE_ARGUMENT_CHECKING
	if(first_position == last_position){
		fputs("ASSERTION FAILED: first_position == last_position in notelib_track_get_tempo_interval! This should never happen!", stderr);
		return 0;
	}
#endif//#ifdef NOTELIB_TRACK_GET_TEMPO_INTERVAL_DISABLE_ARGUMENT_CHECKING
	notelib_position tempo_ceil_interval = track->tempo_ceil_interval;
	notelib_position first_position_aligned = first_position    % tempo_ceil_interval;
	notelib_position  last_position_aligned = (last_position-1) % tempo_ceil_interval + 1;
	notelib_sample_uint tempo_ceil_interval_samples = track->tempo_ceil_interval_samples;
	notelib_sample_uint samples_before_first_position = (((uint64_t)tempo_ceil_interval_samples) * first_position_aligned) / tempo_ceil_interval;
	notelib_sample_uint samples_before_last_position  = (((uint64_t)tempo_ceil_interval_samples) *  last_position_aligned) / tempo_ceil_interval;
	return samples_before_last_position - samples_before_first_position;
}

//I'm about 85% sure that this implementation is correct. (Keep in mind that (currently) the "odd" samples are always floored, not rounded to nearest.)
notelib_position notelib_track_get_position_change(const struct notelib_track* track, notelib_position starting_position, notelib_sample_uint sample_interval){
	notelib_position maximally_covered_positions = (((uint64_t)sample_interval)*track->tempo_ceil_interval)/track->tempo_ceil_interval_samples + 1; //It couldn't ever be more than 1 gained... right?
	if(sample_interval < notelib_track_get_tempo_interval(track, starting_position, starting_position + maximally_covered_positions))
		return maximally_covered_positions - 1;
	else
		return maximally_covered_positions;
}

struct circular_buffer*  notelib_track_get_command_queue(struct notelib_track* track_ptr)
	{return
		 NOTELIB_INTERNAL_OFFSET_AND_CAST
		 (track_ptr,
		  notelib_track_offsetof_command_queue,
		  struct circular_buffer*);}
#ifndef NOTELIB_NO_IMMEDIATE_TRACK
struct circular_buffer*  notelib_track_immediate_get_command_queue(struct notelib_track_immediate* track_ptr)
	{return
		 NOTELIB_INTERNAL_OFFSET_AND_CAST
		 (track_ptr,
		  notelib_track_immediate_offsetof_command_queue,
		  struct circular_buffer*);}
#endif//#ifndef NOTELIB_NO_IMMEDIATE_TRACK

struct circular_buffer_liberal_reader_unsynchronized*  notelib_track_get_inline_initialized_channel_buffer(struct notelib_track* track_ptr, uint16_t queued_command_count)
	{return
		 NOTELIB_INTERNAL_OFFSET_AND_CAST
		 (track_ptr,
		  NOTELIB_INTERNAL_ALIGN_TO_NEXT_ALIGNOF(
		   notelib_track_offsetof_command_queue
		   + notelib_internals_sizeof_track_command_queue(queued_command_count),
		  struct circular_buffer_liberal_reader_unsynchronized),
		  struct circular_buffer_liberal_reader_unsynchronized*);}
#ifndef NOTELIB_NO_IMMEDIATE_TRACK
struct circular_buffer_liberal_reader_unsynchronized*  notelib_track_immediate_get_inline_initialized_channel_buffer(struct notelib_track_immediate* track_ptr, uint16_t queued_command_count)
	{return
		 NOTELIB_INTERNAL_OFFSET_AND_CAST
		 (track_ptr,
		  NOTELIB_INTERNAL_ALIGN_TO_NEXT_ALIGNOF(
		   notelib_track_immediate_offsetof_command_queue
		   + notelib_internals_sizeof_track_command_queue(queued_command_count),
		  struct circular_buffer_liberal_reader_unsynchronized),
		  struct circular_buffer_liberal_reader_unsynchronized*);}
#endif//#ifndef NOTELIB_NO_IMMEDIATE_TRACK

struct circular_buffer_liberal_reader_unsynchronized** notelib_track_get_external_initialized_channel_buffer_ptr_ptr(struct notelib_track* track_ptr, uint16_t queued_command_count)
	{return
		 NOTELIB_INTERNAL_OFFSET_AND_CAST
		 (track_ptr,
		  NOTELIB_INTERNAL_ALIGN_TO_NEXT_ALIGNOF(
		   notelib_track_offsetof_command_queue
		   + notelib_internals_sizeof_track_command_queue(queued_command_count),
		  struct circular_buffer_liberal_reader_unsynchronized*),
		  struct circular_buffer_liberal_reader_unsynchronized**);}
struct circular_buffer_liberal_reader_unsynchronized*  notelib_track_get_external_initialized_channel_buffer_ptr    (struct notelib_track* track_ptr, uint16_t queued_command_count)
	{return *notelib_track_get_external_initialized_channel_buffer_ptr_ptr(track_ptr, queued_command_count);}
#ifndef NOTELIB_NO_IMMEDIATE_TRACK
struct circular_buffer_liberal_reader_unsynchronized** notelib_track_immediate_get_external_initialized_channel_buffer_ptr_ptr(struct notelib_track_immediate* track_ptr, uint16_t queued_command_count)
	{return
		 NOTELIB_INTERNAL_OFFSET_AND_CAST
		 (track_ptr,
		  NOTELIB_INTERNAL_ALIGN_TO_NEXT_ALIGNOF(
		   notelib_track_immediate_offsetof_command_queue
		   + notelib_internals_sizeof_track_command_queue(queued_command_count),
		  struct circular_buffer_liberal_reader_unsynchronized*),
		  struct circular_buffer_liberal_reader_unsynchronized**);}
struct circular_buffer_liberal_reader_unsynchronized*  notelib_track_immediate_get_external_initialized_channel_buffer_ptr    (struct notelib_track_immediate* track_ptr, uint16_t queued_command_count)
	{return *notelib_track_immediate_get_external_initialized_channel_buffer_ptr_ptr(track_ptr, queued_command_count);}
#endif//#ifndef NOTELIB_NO_IMMEDIATE_TRACK

struct circular_buffer_liberal_reader_unsynchronized* notelib_track_get_initialized_channel_buffer_ptr(const struct notelib_internals* internals, struct notelib_track* track_ptr){
	if(notelib_track_is_initialized_channel_buffer_internal(internals, track_ptr))
		return notelib_track_get_inline_initialized_channel_buffer      (track_ptr, internals->command_queue_size);
	else
		return notelib_track_get_external_initialized_channel_buffer_ptr(track_ptr, internals->command_queue_size);
}
#ifndef NOTELIB_NO_IMMEDIATE_TRACK
struct circular_buffer_liberal_reader_unsynchronized* notelib_track_immediate_get_initialized_channel_buffer_ptr(const struct notelib_internals* internals, struct notelib_track_immediate* track_ptr){
	if(notelib_track_immediate_is_initialized_channel_buffer_internal(internals, track_ptr))
		return notelib_track_immediate_get_inline_initialized_channel_buffer      (track_ptr, internals->command_queue_size);
	else
		return notelib_track_immediate_get_external_initialized_channel_buffer_ptr(track_ptr, internals->command_queue_size);
}
#endif//#ifndef NOTELIB_NO_IMMEDIATE_TRACK
