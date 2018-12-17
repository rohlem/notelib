#ifndef NOTELIB_INTERNAL_COMMAND_HELPERS_H_
#define NOTELIB_INTERNAL_COMMAND_HELPERS_H_

#include "command.h"

#include "circular_buffer.h"
#include "internals.h"

enum notelib_status notelib_construct_command_data_note
(struct notelib_internals* internals,
 notelib_instrument_uint instrument_index, void* trigger_data, struct circular_buffer_liberal_reader_unsynchronized* initialized_channel_buffer,
 enum notelib_command_type* command_type_target, union notelib_command_data* command_data_target, void** channel_data_pre_write_buffer_target, notelib_note_id_uint* note_id_target);
enum notelib_status notelib_cleanup_command_data_note
(struct notelib_internals* internals, notelib_instrument_uint instrument_index, void* channel_data_pre_write_buffer, void* initialized_channel_buffer);

#endif//#ifndef NOTELIB_INTERNAL_COMMAND_HELPERS_H_
