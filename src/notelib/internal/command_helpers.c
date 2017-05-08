#include "command_helpers.h"

#include "../notelib.h"
#include "alignment_utilities.h"
#include "circular_buffer.h"
#include "instrument.h"

#include <stdlib.h>

enum notelib_status notelib_construct_command_data_note
(struct notelib_internals* internals,
 notelib_instrument_uint instrument_index, void* trigger_data, struct circular_buffer_liberal_reader_unsynchronized* initialized_channel_buffer,
 struct notelib_command_data* command_data_target, void** channel_data_pre_write_buffer_target, notelib_note_id_uint* note_id_target){
	struct notelib_instrument* instrument_ptr = notelib_internals_get_instrument(internals, instrument_index);
	notelib_step_uint step_count = instrument_ptr->step_count;
	if(step_count == 0)
		return notelib_answer_failure_invalid_instrument;

	if(channel_data_pre_write_buffer_target == NULL)
		return notelib_answer_failure_unknown;
	notelib_instrument_state_uint channel_data_size = instrument_ptr->channel_data_size;
	void* channel_data_pre_write_buffer = malloc(channel_data_size);
	if(channel_data_pre_write_buffer == NULL)
		return notelib_answer_failure_bad_alloc;
	struct notelib_processing_step_entry* steps_ptr = notelib_instrument_get_processing_steps(internals, instrument_ptr);
	for(notelib_step_uint i = 0; i < step_count; ++i){
		struct notelib_processing_step_entry* step = steps_ptr + i;
		notelib_processing_step_setup_function setup = step->setup;
		if(setup != NULL)
			setup
			(NOTELIB_INTERNAL_OFFSET_AND_CAST(trigger_data,                  step->trigger_data_offset, void*),
			 NOTELIB_INTERNAL_OFFSET_AND_CAST(channel_data_pre_write_buffer, step->data_offset,         void*));
	}
	if(!circular_buffer_liberal_reader_unsynchronized_write(initialized_channel_buffer, channel_data_pre_write_buffer, channel_data_size)){
		for(notelib_step_uint i = 0; i < step_count; ++i){
			struct notelib_processing_step_entry* step = steps_ptr + i;
			notelib_processing_step_cleanup_function cleanup = step->cleanup;
			if(cleanup != NULL)
				cleanup(NOTELIB_INTERNAL_OFFSET_AND_CAST(channel_data_pre_write_buffer, step->data_offset, void*));
		}
		free(channel_data_pre_write_buffer);
		return notelib_answer_failure_insufficient_initialized_channel_buffer_space;
	}

	*channel_data_pre_write_buffer_target = channel_data_pre_write_buffer;

	command_data_target->type = notelib_command_type_note;
	command_data_target->note.instrument_index = instrument_index;
	notelib_note_id_uint note_id = notelib_internals_get_next_note_id(internals);
	//could split up get and increment of note_id and only increment if command_queue write was successful
	 //+ help performance in the case of error (which is not a priority)
	 //- possibly hurt data access locality? (Can fetch for increment-post-fetch be expected to be optimized out? (and would it even make a difference not to have a read-dependency?) (update: much less likely across function boundaries the way the code turned out))
	command_data_target->note.note_id = note_id;
	if(note_id_target != NULL)
		*note_id_target = note_id;

	return notelib_answer_success;
}

enum notelib_status notelib_cleanup_command_data_note
(struct notelib_internals* internals, notelib_instrument_uint instrument_index, void* channel_data_pre_write_buffer, void* initialized_channel_buffer){
	struct notelib_instrument* instrument_ptr = notelib_internals_get_instrument(internals, instrument_index);
	struct notelib_processing_step_entry* steps_ptr = notelib_instrument_get_processing_steps(internals, instrument_ptr);
	for(notelib_step_uint i = 0; i < instrument_ptr->step_count; ++i){
		struct notelib_processing_step_entry* step = steps_ptr + i;
		notelib_processing_step_cleanup_function cleanup = step->cleanup;
		if(cleanup != NULL)
			cleanup(NOTELIB_INTERNAL_OFFSET_AND_CAST(channel_data_pre_write_buffer, step->data_offset, void*));
	}
	if(!circular_buffer_liberal_reader_unsynchronized_unwrite(initialized_channel_buffer, instrument_ptr->channel_data_size))
		return notelib_status_not_ok;

	return notelib_answer_success;
}
