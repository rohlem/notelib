#ifndef NOTELIB_INTERNAL_COMMAND_H_
#define NOTELIB_INTERNAL_COMMAND_H_

#include "../notelib.h"

struct notelib_command{
	enum notelib_command_type {
		notelib_command_type_note,

		notelib_command_type_reset,
		notelib_command_type_set_tempo,

		notelib_command_type_trigger
	} type;
	union{
		struct notelib_command_note{
			notelib_instrument_uint instrument_index;
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
	notelib_position position;
};

#endif//#ifndef NOTELIB_INTERNAL_COMMAND_H_
