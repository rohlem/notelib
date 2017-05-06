#ifndef NOTELIB_INTERNAL_COMMAND_H_
#define NOTELIB_INTERNAL_COMMAND_H_

#include "../notelib.h"

struct notelib_command_data{
	enum notelib_command_type {
		notelib_command_type_note,

		notelib_command_type_reset,
		notelib_command_type_set_tempo,

		notelib_command_type_trigger
	} type;
	union{
		struct notelib_command_note{
			notelib_instrument_uint instrument_index;
			notelib_note_id_uint note_id; //possible optimization: put note_id into initialized_channel_buffer (in-place), memcpy state_size instead of data_size (desirable?)
		} note;
		struct notelib_tempo{
			notelib_position position_interval;
			notelib_sample_uint interval;
		} tempo;
		struct notelib_command_trigger{
			notelib_trigger_function trigger_function;
			void* userdata;
		} trigger;
	};
};
struct notelib_command{
	struct notelib_command_data data;
	notelib_position position;
};
#ifndef NOTELIB_NO_IMMEDIATE_TRACK
struct notelib_command_immediate{
	struct notelib_command_data data;
};
#endif//#ifndef NOTELIB_NO_IMMEDIATE_TRACK

#endif//#ifndef NOTELIB_INTERNAL_COMMAND_H_
