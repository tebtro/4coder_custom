#include "vim.h"


function void vim_improved_newline(Application_Links *app, b32 below = true, b32 newline_at_sol_eol = true);

// Ugh
template <CUSTOM_COMMAND_SIG(command), b32 view_changed_do_highlight = false> CUSTOM_COMMAND_SIG(vim_window_command);

function void vim_scroll_cursor_line(Application_Links *app, int view_pos, View_ID view_id = -1);
CUSTOM_COMMAND_SIG(vim_scroll_cursor_line_to_view_center);

//
// @note helpers
//
CUSTOM_COMMAND_SIG(nop) {}

inline b32
character_is_newline(char c) {
    b32 result = (c == '\n');
    return result;
}

#define VIM_GET_VIEW_ID_VIEW_SCOPE_BUFFER_ID_AND_VIM_STATE(app) \
View_ID view_id = get_this_ctx_view((app), Access_Always); \
Buffer_ID buffer_id = view_get_buffer((app), view_id, Access_ReadVisible); \
Managed_Scope view_scope = view_get_managed_scope((app), view_id); \
Vim_View_State *vim_state = scope_attachment((app), view_scope, view_vim_state_id, Vim_View_State);

#define VIM_GET_VIEW_ID_SCOPE_AND_VIM_STATE(app) \
View_ID view_id = get_this_ctx_view((app), Access_Always); \
Managed_Scope view_scope = view_get_managed_scope((app), view_id); \
Vim_View_State *vim_state = scope_attachment((app), view_scope, view_vim_state_id, Vim_View_State);

#define VIM_GET_VIEW_ID_AND_BUFFER_ID(app) \
View_ID view_id = get_this_ctx_view((app), Access_Always); \
Buffer_ID buffer_id = view_get_buffer((app), view_id, Access_ReadVisible);

// @note View for macro
#define for_views(app, it) \
for (View_ID it = get_view_next((app), 0, Access_Always); \
it != 0;                                           \
it = get_view_next((app), it, Access_Always))


//
// @note vim initialization
//

function void
vim_setup_mode_and_chord_color_tables(Application_Links *app) {
    Arena *arena = &global_theme_arena;
    Vim_Global_State *state = &vim_global_state;
    
    for (int i = 0; i < ArrayCount(vim_global_state.color_tables_array); ++i) {
        vim_global_state.color_tables_array[i] = make_color_table(app, arena);
        
        for (u32 x = 0; x < active_color_table.count; ++x) {
            vim_global_state.color_tables_array[i].arrays[x] = active_color_table.arrays[x];
        }
    }
    
    Color_Table *table;
    
    // @note mode normal
    table = &state->color_tables.mode_normal;
    
    // @note mode insert
    table = &state->color_tables.mode_insert;
    table->arrays[defcolor_highlight_cursor_line] = make_colors(arena, 0x1100FF00);
    table->arrays[defcolor_highlight_token_under_cursor] = make_colors(arena, 0x331A9828);
    
    // @note mode visual
    table = &state->color_tables.mode_visual;
    table->arrays[defcolor_cursor] = make_colors(arena, 0xFF0DDFFF);
    table->arrays[defcolor_mark]   = make_colors(arena, 0xAA0DDFFF);
    
    // @note mode replace
    table = &state->color_tables.mode_replace;
    table->arrays[defcolor_cursor]    = make_colors(arena, 0xFF9500C2);
    table->arrays[defcolor_at_cursor] = make_colors(arena, 0xFFFFFFFF);
    table->arrays[defcolor_highlight_cursor_line] = make_colors(arena, 0x22D90BCC);
    table->arrays[defcolor_highlight_token_under_cursor] = make_colors(arena, 0x33D90BCC);
    
    // @note chord replace single
    table = &state->color_tables.chord_replace_single;
    table->arrays[defcolor_cursor]    = make_colors(arena, 0xFFD90BCC);
    table->arrays[defcolor_at_cursor] = make_colors(arena, 0xFFFFFFFF);
    
    // @note chord delete
    table = &state->color_tables.chord_delete;
    table->arrays[defcolor_cursor] = make_colors(arena, 0xFFFF0000);
    table->arrays[defcolor_highlight_cursor_line] = make_colors(arena, 0x22FF0000);
    table->arrays[defcolor_highlight_token_under_cursor] = make_colors(arena, 0x44A8281A);
    
    // @note chord change
    table = &state->color_tables.chord_change;
    table->arrays[defcolor_cursor] = make_colors(arena, 0xFFFF9800);
    table->arrays[defcolor_highlight_cursor_line] = make_colors(arena, 0x22E9710C);
    table->arrays[defcolor_highlight_token_under_cursor] = make_colors(arena, 0x29E9710C);
    
    // @note chord yank
    table = &state->color_tables.chord_yank;
    table->arrays[defcolor_cursor] = make_colors(arena, 0xFFFFFF00);
    table->arrays[defcolor_highlight_cursor_line] = make_colors(arena, 0x20CCCC00);
    table->arrays[defcolor_highlight_token_under_cursor] = make_colors(arena, 0x27CCCC00);
}


//
// @note Vim mode and chord
//

function void
vim_set_mode_and_command_map(Application_Links *app,
                             Vim_Mode new_mode, Vim_Action new_action, Command_Map_ID new_map_id, Color_Table *color_table_ptr) {
    VIM_GET_VIEW_ID_SCOPE_AND_VIM_STATE(app);
    
    Command_Map_ID *map_id_ptr = scope_attachment(app, view_scope, view_map_id, Command_Map_ID);
    *map_id_ptr = new_map_id;
    
    if (new_mode != 0) {
        vim_state->mode = new_mode;
        vim_state->color_table_ptr = color_table_ptr;
    }
    if (new_action != 0) {
        vim_state->pending_action = new_action;
        vim_state->color_table_ptr = color_table_ptr;
    }
    if (new_mode == 0 && new_action == 0 && color_table_ptr != 0) {
        vim_state->color_table_ptr = color_table_ptr;
    }
    
#if 1
    if (vim_state->color_table_ptr == 0) {
        vim_state->color_table_ptr = &vim_global_state.color_tables.default;
    }
#endif
}

//
// @note vim mode handling
//

function void
vim_enter_mode(Application_Links *app,
               Vim_Mode new_mode, Command_Map_ID new_map_id, Color_Table *color_table_ptr) {
    vim_set_mode_and_command_map(app, new_mode, vim_action_none, new_map_id, color_table_ptr);
    
#if 0
    // :suppress_mouse
    if (new_mode == vim_mode_normal) { //  || vim_mode_visual || vim_mode_visual_line
        allow_mouse(app);
    }
    else {
        suppress_mouse(app);
    }
#endif
}


// @todo @incomplete
// chord_bar
CUSTOM_COMMAND_SIG(vim_enter_mode_normal) {
    // @todo @incomplete
    vim_enter_mode(app, vim_mode_normal, mapid_vim_mode_normal, &vim_global_state.color_tables.mode_normal);
    
#if VIM_WINDOWS_AUTO_DISABLE_CAPSLOCK
    SHORT capslock_state = GetKeyState(VK_CAPITAL);
    bool capslock_is_toggled = (capslock_state & 1) != 0;
    
    if (capslock_is_toggled) {
        INPUT input;
        
        // Set up a generic keyboard event.
        input.type = INPUT_KEYBOARD;
        input.ki.wScan = 0; // hardware scan code for key
        input.ki.time = 0;
        input.ki.dwExtraInfo = 0;
        
        // Press the "CAPSLOCK" key
        input.ki.wVk = VK_CAPITAL; // virtual-key code for the "CAPSLOCK" key
        input.ki.dwFlags = 0; // 0 for key press
        SendInput(1, &input, sizeof(INPUT));
        
        // Release the "CAPSLOCK" key
        input.ki.dwFlags = KEYEVENTF_KEYUP; // KEYEVENTF_KEYUP for key release
        SendInput(1, &input, sizeof(INPUT));
    }
#endif
}

// @note enter insert mode commands
CUSTOM_COMMAND_SIG(vim_enter_mode_insert) {
    // @todo @incomplete
    // end_visual_selection
    // end_chord_bar
    vim_enter_mode(app, vim_mode_insert, mapid_vim_mode_insert, &vim_global_state.color_tables.mode_insert);
}
CUSTOM_COMMAND_SIG(vim_enter_mode_insert_after) {
    VIM_GET_VIEW_ID_AND_BUFFER_ID(app);
    i64 cursor_pos = view_get_cursor_pos(app, view_id);
    u8 next_char;
    if (buffer_read_range(app, buffer_id, Ii64(cursor_pos, cursor_pos + 1), &next_char)) {
        if (!character_is_newline(next_char)) {
            move_right(app);
        }
    }
    vim_enter_mode_insert(app);
}
CUSTOM_COMMAND_SIG(vim_enter_mode_insert_line_start) {
    seek_pos_of_visual_line(app, Side_Min); // :non_virtual_whitespace textual or visual line
    vim_enter_mode_insert(app);
}
CUSTOM_COMMAND_SIG(vim_enter_mode_insert_line_end) {
    seek_pos_of_visual_line(app, Side_Max); // :non_virtual_whitespace textual or visual line
    vim_enter_mode_insert(app);
}
CUSTOM_COMMAND_SIG(vim_newline_and_enter_mode_insert_after) {
    vim_improved_newline(app, true, false);
    vim_enter_mode_insert(app);
}
CUSTOM_COMMAND_SIG(vim_newline_and_enter_mode_insert_before) {
    vim_improved_newline(app, false, false);
    vim_enter_mode_insert(app);
}

CUSTOM_COMMAND_SIG(vim_enter_mode_replace) {
    // @todo @incomplete
    // clear_register_selection
    vim_enter_mode(app, vim_mode_replace, mapid_vim_mode_replace, &vim_global_state.color_tables.mode_replace);
}

// :vim_visual_range
inline void
vim_update_visual_range(Application_Links *app, View_ID view_id, Vim_View_State *vim_state, b32 reset_mark = false) {
    Buffer_ID buffer_id = view_get_buffer(app, view_id, Access_ReadVisible);
    i64 buffer_size = buffer_get_size(app, buffer_id);
    
    // :cursor_mark
    i64 cursor_pos = view_get_cursor_pos(app, view_id);
    i64 mark_pos;
    if (reset_mark) {
        mark_pos = cursor_pos;
        view_set_mark(app, view_id, seek_pos(mark_pos));
    } else {
        mark_pos = view_get_mark_pos(app, view_id);
    }
    if (cursor_pos >= buffer_size) {
        move_left(app);
        cursor_pos = view_get_cursor_pos(app, view_id);
    }
    if (mark_pos >= buffer_size) {
        mark_pos = buffer_size;
        view_set_mark(app, view_id, seek_pos(mark_pos - 1));
    }
    
    Range_i64 range = Ii64(cursor_pos, mark_pos);
    vim_state->selection_range = Ii64(range.start, range.end + 1);
    
    if (vim_state->selection_range.one_past_last >= buffer_size) {
        vim_state->selection_range.one_past_last = buffer_size;
    }
}
inline void
vim_update_visual_range(Application_Links *app, View_ID view_id, b32 reset_mark = false) {
    Managed_Scope view_scope = view_get_managed_scope((app), view_id);
    Vim_View_State *vim_state = scope_attachment((app), view_scope, view_vim_state_id, Vim_View_State);
    vim_update_visual_range(app, view_id, vim_state, reset_mark);
}
inline void
vim_enter_mode_visual(Application_Links *app, b32 reset_mark = false) {
    VIM_GET_VIEW_ID_SCOPE_AND_VIM_STATE(app);
    vim_update_visual_range(app, view_id, vim_state, reset_mark);
    
    // @todo @incomplete
    // clear_register_selection
    vim_enter_mode(app, vim_mode_visual, mapid_vim_mode_visual, &vim_global_state.color_tables.mode_visual);
}
CUSTOM_COMMAND_SIG(vim_enter_mode_visual) {
    vim_enter_mode_visual(app, true);
}
CUSTOM_COMMAND_SIG(vim_enter_mode_visual_with_cursor_mark_range) {
    vim_enter_mode_visual(app, false);
}

// :vim_visual_range
inline void
vim_update_visual_line_range(Application_Links *app, View_ID view_id, Vim_View_State *vim_state, b32 reset_mark = false) {
    Buffer_ID buffer_id = view_get_buffer(app, view_id, Access_Always);
    i64 buffer_size = buffer_get_size(app, buffer_id);
    
    i64 cursor_pos = view_get_cursor_pos(app, view_id);
    i64 mark_pos;
    if (reset_mark) {
        mark_pos = cursor_pos;
    } else {
        mark_pos = view_get_mark_pos(app, view_id);
    }
    if (cursor_pos >= buffer_size) {
        move_left(app);
        cursor_pos = view_get_cursor_pos(app, view_id);
    }
    
    Range_i64 pos_range = Ii64(cursor_pos, mark_pos);
    Range_i64 line_range = get_line_range_from_pos_range(app, buffer_id, pos_range);
    Range_i64 range = get_pos_range_from_line_range(app, buffer_id, line_range);
    
    vim_state->selection_range = Ii64(range.start, range.end + 1);
    
    if (vim_state->selection_range.one_past_last >= buffer_size) {
        vim_state->selection_range.one_past_last = buffer_size;
    }
    
    // :cursor_mark
    if (cursor_pos <= mark_pos) {
        view_set_mark(app, view_id, seek_pos(vim_state->selection_range.one_past_last - 1));
    }
    else {
        view_set_mark(app, view_id, seek_pos(vim_state->selection_range.start));
    }
}
inline void
vim_enter_mode_visual_line(Application_Links *app, b32 reset_mark = false) {
    VIM_GET_VIEW_ID_SCOPE_AND_VIM_STATE(app);
    vim_update_visual_line_range(app, view_id, vim_state, reset_mark);
    
    // @todo @incomplete
    // clear_register_selection
    vim_enter_mode(app, vim_mode_visual_line, mapid_vim_mode_visual, &vim_global_state.color_tables.mode_visual);
}
CUSTOM_COMMAND_SIG(vim_enter_mode_visual_line) {
    vim_enter_mode_visual_line(app, true);
}
CUSTOM_COMMAND_SIG(vim_enter_mode_visual_line_with_cursor_mark_range) {
    vim_enter_mode_visual_line(app, false);
}

//
// @note Evil escape
//
#define ESCAPE_SEQUENCE_DELAY (130 * 1000)
global u64 vim_mode_escape_sequence_start_time = 0;

// @note mode insert
CUSTOM_COMMAND_SIG(vim_mode_insert_start_escape_sequence) {
    vim_mode_escape_sequence_start_time = system_now_time();
    
    User_Input input = get_current_input(app);
    String_Const_u8 insert = SCu8("j");
    if (has_modifier(&input.event.key.modifiers, KeyCode_Shift)) {
        insert = SCu8("J");
    }
    
    // write_text(app, insert);
    VIM_GET_VIEW_ID_AND_BUFFER_ID(app);
    i64 pos = view_get_cursor_pos(app, view_id);
    b32 edit_success = buffer_replace_range(app, buffer_id, Ii64(pos), insert);
    if (edit_success) {
        view_set_cursor_and_preferred_x(app, view_id, seek_pos(pos + insert.size));
    }
}
CUSTOM_COMMAND_SIG(vim_mode_insert_finish_escape_sequence) {
    if (vim_mode_escape_sequence_start_time) {
        u64 duration_time = system_now_time() - vim_mode_escape_sequence_start_time;
        u64 delay = ESCAPE_SEQUENCE_DELAY;
        if (duration_time <= delay) {
            // undo(app);
            // @note Undo the start sequence "j" and merge that with a previous history record
            backspace_char(app);
            VIM_GET_VIEW_ID_AND_BUFFER_ID(app);
            History_Record_Index last_index = buffer_history_get_current_state_index(app, buffer_id);
            buffer_history_merge_record_range(app, buffer_id, last_index-2, last_index, RecordMergeFlag_StateInRange_MoveStateForward);
            
            vim_enter_mode_normal(app);
            return;
        }
    }
	User_Input input = get_current_input(app);
    String_Const_u8 insert = SCu8("k");
    if (has_modifier(&input.event.key.modifiers, KeyCode_Shift)) {
        insert = SCu8("K");
    }
	write_text(app, insert);
}

// @note mode replace
CUSTOM_COMMAND_SIG(vim_replace_character_and_move_right);
function void vim_replace_character(Application_Links *app, String_Const_u8 replace);
function void vim_replace_character_and_move_right(Application_Links *app, String_Const_u8 replace);

CUSTOM_COMMAND_SIG(vim_mode_replace_start_escape_sequence) {
    vim_mode_escape_sequence_start_time = system_now_time();
    
    String_Const_u8 replace = SCu8("j");
    vim_replace_character_and_move_right(app, replace);
}
CUSTOM_COMMAND_SIG(vim_mode_replace_finish_escape_sequence) {
    if (vim_mode_escape_sequence_start_time) {
        u64 duration_time = system_now_time() - vim_mode_escape_sequence_start_time;
        u64 delay = ESCAPE_SEQUENCE_DELAY;
        if (duration_time <= delay) {
            move_left(app);
            undo(app);
            vim_enter_mode_normal(app);
            return;
        }
    }
    vim_replace_character_and_move_right(app);
}


//
// @note vim chord handling
//

function void
vim_enter_chord(Application_Links *app, Command_Map_ID new_map_id, Color_Table *color_table_ptr) {
    vim_set_mode_and_command_map(app, (Vim_Mode)0, (Vim_Action)0, new_map_id, color_table_ptr);
}
function void
vim_enter_chord(Application_Links *app,
                Vim_Action new_action, Command_Map_ID new_map_id, Color_Table *color_table_ptr) {
    vim_set_mode_and_command_map(app, (Vim_Mode)0, new_action, new_map_id, color_table_ptr);
}


CUSTOM_COMMAND_SIG(vim_enter_chord_replace_single) {
    // @todo @incomplete
    // clear_register_selection
    vim_enter_chord(app, mapid_vim_chord_replace_single, &vim_global_state.color_tables.chord_replace_single);
}

CUSTOM_COMMAND_SIG(vim_enter_chord_delete) {
    // @todo @incomplete
    // chord_bar
    vim_enter_chord(app, vim_action_delete_range, mapid_vim_chord_delete, &vim_global_state.color_tables.chord_delete);
}

CUSTOM_COMMAND_SIG(vim_enter_chord_change) {
    // @todo @incomplete
    // chord_bar
    vim_enter_chord(app, vim_action_change_range, mapid_vim_chord_change, &vim_global_state.color_tables.chord_change);
}

CUSTOM_COMMAND_SIG(vim_enter_chord_yank) {
    // @todo @incomplete
    // chord_bar
    vim_enter_chord(app, vim_action_yank_range, mapid_vim_chord_yank, &vim_global_state.color_tables.chord_yank);
}


CUSTOM_COMMAND_SIG(vim_enter_chord_format) {
    // @todo @incomplete
    // chord_bar
    vim_enter_chord(app, vim_action_format_range, mapid_vim_chord_format, 0);
}

CUSTOM_COMMAND_SIG(vim_enter_chord_indent_right) {
    // @todo @incomplete
    // chord_bar
    vim_enter_chord(app, vim_action_indent_right_range, mapid_vim_chord_indent_right, 0);
}

CUSTOM_COMMAND_SIG(vim_enter_chord_indent_left) {
    // @todo @incomplete
    // chord_bar
    vim_enter_chord(app, vim_action_indent_left_range, mapid_vim_chord_indent_left, 0);
}


CUSTOM_COMMAND_SIG(vim_enter_chord_move_right_to_found) {
    // @todo @incomplete
    // chord_bar
    vim_enter_chord(app, mapid_vim_chord_move_right_to_found, 0);
}
CUSTOM_COMMAND_SIG(vim_enter_chord_move_right_before_found) {
    // @todo @incomplete
    // chord_bar
    vim_enter_chord(app, mapid_vim_chord_move_right_before_found, 0);
}
CUSTOM_COMMAND_SIG(vim_enter_chord_move_left_to_found) {
    // @todo @incomplete
    // chord_bar
    vim_enter_chord(app, mapid_vim_chord_move_left_to_found, 0);
}
CUSTOM_COMMAND_SIG(vim_enter_chord_move_left_before_found) {
    // @todo @incomplete
    // chord_bar
    vim_enter_chord(app, mapid_vim_chord_move_left_before_found, 0);
}


CUSTOM_COMMAND_SIG(vim_enter_chord_g) {
    // @todo @incomplete
    // chord_bar
    vim_enter_chord(app, mapid_vim_chord_g, 0);
}
CUSTOM_COMMAND_SIG(vim_enter_chord_z) {
    // @todo @incomplete
    // chord_bar
    vim_enter_chord(app, mapid_vim_chord_z, 0);
}


CUSTOM_COMMAND_SIG(vim_enter_chord_mark) {
    // @todo @incomplete
    // chord_bar
    vim_enter_chord(app, mapid_vim_chord_mark, 0);
}
CUSTOM_COMMAND_SIG(vim_enter_chord_window) {
    // @todo @incomplete
    // chord_bar
    vim_enter_chord(app, mapid_vim_chord_window, 0);
}
CUSTOM_COMMAND_SIG(vim_enter_chord_choose_register) {
    // @todo @incomplete
    // chord_bar
    vim_enter_chord(app, mapid_vim_chord_choose_register, 0);
}

//
// @note Chord helper
//
template <CUSTOM_COMMAND_SIG(command)>
CUSTOM_COMMAND_SIG(vim_chord_command) {
    command(app);
    vim_enter_mode_normal(app);
}

//
// @note vim handle actions
//
function void
vim_reset_mode_mapid(Application_Links *app) {
    // :change_mode
    VIM_GET_VIEW_ID_SCOPE_AND_VIM_STATE(app);
    switch (vim_state->mode) {
        case vim_mode_normal: {
            vim_enter_mode_normal(app);
        } break;
        case vim_mode_insert: {
            vim_enter_mode_insert(app);
        } break;
        
        // @note Set mapid, but don't change visual selection_range
        case vim_mode_visual: {
            vim_enter_mode(app, vim_mode_visual, mapid_vim_mode_visual, &vim_global_state.color_tables.mode_visual);
        } break;
        case vim_mode_visual_line: {
            vim_enter_mode(app, vim_mode_visual_line, mapid_vim_mode_visual, &vim_global_state.color_tables.mode_visual);
        } break;
    }
}

function void
vim_exec_pending_action(Application_Links *app, Range_i64 range, b32 is_line = false, b32 moved_right_one_past_last = false) {
    ProfileScope(app, "vim_exec_pending_action");
    
    VIM_GET_VIEW_ID_SCOPE_AND_VIM_STATE(app);
    Buffer_ID buffer_id = view_get_buffer(app, view_id, Access_ReadWriteVisible);
    Vim_Action action = vim_state->pending_action;
    // @todo When we need it. Vim_Global_State *vim_global_state = &vim_global_state;
    
    // @note execute pending action
    switch (action) {
        case vim_action_delete_range:
        case vim_action_change_range: {
            // @todo save to vim register
            
            // @note post to clipboard and remove range in buffer
            if (clipboard_post_buffer_range(app, 0, buffer_id, range)) {
                // @note If changing a line, don't remove the newline char!
                if (is_line && action == vim_action_change_range) {
                    --range.one_past_last;
                }
                
                buffer_replace_range(app, buffer_id, range, string_u8_empty);
            }
            
            // :change_mode
            if (action == vim_action_change_range) {
                // :cursor_mark
                // if (!is_line && vim_state->mode != vim_mode_visual && vim_state->mode != vim_mode_visual_line)
                if (moved_right_one_past_last) {
                    move_left(app);
                }
                
                vim_enter_mode_insert(app);
            }
        } break;
        
        case vim_action_yank_range: {
            // @todo save to vim register
            
            clipboard_post_buffer_range(app, 0, buffer_id, range);
        } break;
        
        case vim_action_indent_left_range:  // @todo If not in virtual_whitespace should decrease indentation
        case vim_action_indent_right_range: // @todo If not in virtual_whitespace should increase indentation
        case vim_action_format_range: {
            auto_indent_buffer(app, buffer_id, range);
            move_past_lead_whitespace(app, view_id, buffer_id);
        } break;
    }
    
    // :change_mode
    vim_reset_mode_mapid(app);
}

CUSTOM_COMMAND_SIG(vim_exec_pending_action_on_line_range) {
    VIM_GET_VIEW_ID_AND_BUFFER_ID(app);
    
    i64 cursor_pos = view_get_cursor_pos(app, view_id);
    i64 line_number = get_line_number_from_pos(app, buffer_id, cursor_pos);
    Range_i64 range = get_line_pos_range(app, buffer_id, line_number);
    range.one_past_last = range.end + 1;
    
    vim_exec_pending_action(app, range, true);
}


//
// @note vim movement commands
//

#define VIM_NTIMES_CUSTOM_COMMAND_SIG(name) void name(struct Application_Links *app, View_ID view_id, Buffer_ID buffer_id, Managed_Scope view_scope, Managed_Scope buffer_scope, Vim_View_State *vim_state)

// @note execute ntimes

inline VIM_NTIMES_CUSTOM_COMMAND_SIG(_vim_ntimes_move_up) {
    i64 line_count = buffer_get_line_count(app, buffer_id);
    i64 n = Min(line_count, vim_state->execute_command_count);
    
    // @todo Not moving the exact line amount, because it is including wrapped lines.
    // move_up_textual
    move_vertical_lines(app, view_id, -n);
}
inline VIM_NTIMES_CUSTOM_COMMAND_SIG(_vim_ntimes_move_down) {
    i64 line_count = buffer_get_line_count(app, buffer_id);
    i64 n = Min(line_count, vim_state->execute_command_count);
    
    // @todo Not moving the exact line amount, because it is including wrapped lines.
    // move_down_textual
    move_vertical_lines(app, view_id, n);
}
inline VIM_NTIMES_CUSTOM_COMMAND_SIG(_vim_ntimes_move_left) {
    view_set_cursor_by_character_delta(app, view_id, -vim_state->execute_command_count);
    no_mark_snap_to_cursor_if_shift(app, view_id);
}
inline VIM_NTIMES_CUSTOM_COMMAND_SIG(_vim_ntimes_move_right) {
    view_set_cursor_by_character_delta(app, view_id, vim_state->execute_command_count);
    no_mark_snap_to_cursor_if_shift(app, view_id);
}

inline VIM_NTIMES_CUSTOM_COMMAND_SIG(_vim_ntimes_move_up_by_page) {
    f32 page_jump = get_page_jump(app, view_id) * (f32)vim_state->execute_command_count;
    move_vertical_pixels(app, -page_jump);
}
inline VIM_NTIMES_CUSTOM_COMMAND_SIG(_vim_ntimes_move_down_by_page) {
    f32 page_jump = get_page_jump(app, view_id) * (f32)vim_state->execute_command_count;
    move_vertical_pixels(app, page_jump);
}
inline VIM_NTIMES_CUSTOM_COMMAND_SIG(_vim_ntimes_move_up_by_page_half) {
    f32 page_jump = get_page_jump(app, view_id) * 0.5f * (f32)vim_state->execute_command_count;
    move_vertical_pixels(app, -page_jump);
}
inline VIM_NTIMES_CUSTOM_COMMAND_SIG(_vim_ntimes_move_down_by_page_half) {
    f32 page_jump = get_page_jump(app, view_id) * 0.5f * (f32)vim_state->execute_command_count;
    move_vertical_pixels(app, page_jump);
}

/* @todo :non_virtual_whitespace This option only really changes the behaviour in non_virtual_whitspace_mode.
PositionWithinLine_Start
PositionWithinLine_SkipLeadingWhitespace
PositionWithinLine_End
*/
inline VIM_NTIMES_CUSTOM_COMMAND_SIG(_vim_ntimes_move_up_by_whitespace) {
    for (int i = 0; i < vim_state->execute_command_count; ++i) {
        seek_blank_line(app, Scan_Backward, PositionWithinLine_SkipLeadingWhitespace);
    }
}
inline VIM_NTIMES_CUSTOM_COMMAND_SIG(_vim_ntimes_move_down_by_whitespace) {
    for (int i = 0; i < vim_state->execute_command_count; ++i) {
        seek_blank_line(app, Scan_Forward, PositionWithinLine_SkipLeadingWhitespace);
    }
}

inline void
vim_get_found_input_character_pos(Application_Links *app, View_ID view_id, Buffer_ID buffer_id, Vim_View_State *vim_state, Scan_Direction direction, b32 one_before_found = false) {
    i64 pos = view_get_cursor_pos(app, view_id);
    if (one_before_found) {
        pos += (direction == Scan_Forward) ? 1 : -1;
    }
    
    User_Input user_input = get_current_input(app);
    String_Const_u8 character = to_writable(&user_input);
    Assert(character.size == 1);
    Character_Predicate character_predicate = character_predicate_from_character(character.str[0]);
    
    i64 new_pos;
    String_Match match;
    for (int i = 0; i < vim_state->execute_command_count; ++i) {
        match = buffer_seek_character_class(app, buffer_id, &character_predicate, direction, pos);
        if (match.buffer == 0)  break; // If not found break
        new_pos = match.range.min;
        if (pos == new_pos)  break;
        pos = new_pos;
    }
    
    if (one_before_found) {
        pos += (direction == Scan_Forward) ? -1 : 1;
    }
    view_set_cursor_and_preferred_x(app, view_id, seek_pos(pos));
    no_mark_snap_to_cursor_if_shift(app, view_id);
}
inline VIM_NTIMES_CUSTOM_COMMAND_SIG(_vim_ntimes_move_right_to_found) {
    vim_get_found_input_character_pos(app, view_id, buffer_id, vim_state, Scan_Forward);
}
inline VIM_NTIMES_CUSTOM_COMMAND_SIG(_vim_ntimes_move_right_before_found) {
    vim_get_found_input_character_pos(app, view_id, buffer_id, vim_state, Scan_Forward, true);
}
inline VIM_NTIMES_CUSTOM_COMMAND_SIG(_vim_ntimes_move_left_to_found) {
    vim_get_found_input_character_pos(app, view_id, buffer_id, vim_state, Scan_Backward);
}
inline VIM_NTIMES_CUSTOM_COMMAND_SIG(_vim_ntimes_move_left_before_found) {
    vim_get_found_input_character_pos(app, view_id, buffer_id, vim_state, Scan_Backward, true);
}


// @note Added Side
internal i64
scan(Application_Links *app, Boundary_Function *func, Buffer_ID buffer, Scan_Direction direction, Side side, i64 pos){
    return(func(app, buffer, side, direction, pos));
}
internal i64
scan(Application_Links *app, Boundary_Function_List funcs, Buffer_ID buffer, Scan_Direction direction, Side side, i64 start_pos){
    i64 result = 0;
    if (direction == Scan_Forward){
        i64 size = buffer_get_size(app, buffer);
        result = size + 1;
        for (Boundary_Function_Node *node = funcs.first;
             node != 0;
             node = node->next){
            i64 pos = scan(app, node->func, buffer, direction, side, start_pos);
            result = Min(result, pos);
        }
    }
    else{
        result = -1;
        for (Boundary_Function_Node *node = funcs.first;
             node != 0;
             node = node->next){
            i64 pos = scan(app, node->func, buffer, direction, side, start_pos);
            result = Max(result, pos);
        }
    }
    return(result);
}
function void
current_view_scan_move(Application_Links *app, Scan_Direction direction, Side side, Boundary_Function_List funcs){
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    i64 cursor_pos = view_get_cursor_pos(app, view);
    i64 pos = scan(app, funcs, buffer, direction, side, cursor_pos);
    view_set_cursor_and_preferred_x(app, view, seek_pos(pos));
    no_mark_snap_to_cursor_if_shift(app, view);
}

internal i64
boundary_whitespace(Application_Links *app, Buffer_ID buffer, Side side, Scan_Direction direction, i64 pos){
    return(boundary_predicate(app, buffer, side, direction, pos, &character_predicate_whitespace));
}

inline VIM_NTIMES_CUSTOM_COMMAND_SIG(_vim_ntimes_move_right_word_start) {
    for (int i = 0; i < vim_state->execute_command_count; ++i) {
        Scratch_Block scratch(app);
        current_view_scan_move(app, Scan_Forward, Side_Min, push_boundary_list(scratch, boundary_alpha_numeric, boundary_token));
    }
}
inline VIM_NTIMES_CUSTOM_COMMAND_SIG(_vim_ntimes_move_right_token_start) {
    for (int i = 0; i < vim_state->execute_command_count; ++i) {
        Scratch_Block scratch(app);
        current_view_scan_move(app, Scan_Forward, Side_Min, push_boundary_list(scratch, boundary_token));
    }
}
inline VIM_NTIMES_CUSTOM_COMMAND_SIG(_vim_ntimes_move_right_one_after_whitespace) {
    for (int i = 0; i < vim_state->execute_command_count; ++i) {
        Scratch_Block scratch(app);
        current_view_scan_move(app, Scan_Forward, Side_Max, push_boundary_list(scratch, boundary_whitespace));
    }
}

inline VIM_NTIMES_CUSTOM_COMMAND_SIG(_vim_ntimes_move_right_word_end) {
    for (int i = 0; i < vim_state->execute_command_count; ++i) {
        move_right(app);
        Scratch_Block scratch(app);
        current_view_scan_move(app, Scan_Forward, push_boundary_list(scratch, boundary_alpha_numeric, boundary_token));
        move_left(app);
    }
}
inline VIM_NTIMES_CUSTOM_COMMAND_SIG(_vim_ntimes_move_right_token_end) {
    for (int i = 0; i < vim_state->execute_command_count; ++i) {
        move_right(app);
        move_right_token_boundary(app);
        move_left(app);
    }
}
inline VIM_NTIMES_CUSTOM_COMMAND_SIG(_vim_ntimes_move_right_one_before_whitespace) {
    for (int i = 0; i < vim_state->execute_command_count; ++i) {
        move_right(app);
        move_right_whitespace_boundary(app);
        move_left(app);
    }
}


inline VIM_NTIMES_CUSTOM_COMMAND_SIG(_vim_ntimes_move_left_word_start) {
    for (int i = 0; i < vim_state->execute_command_count; ++i) {
        Scratch_Block scratch(app);
        current_view_scan_move(app, Scan_Backward, push_boundary_list(scratch, boundary_alpha_numeric, boundary_token));
    }
}
inline VIM_NTIMES_CUSTOM_COMMAND_SIG(_vim_ntimes_move_left_token_start) {
    for (int i = 0; i < vim_state->execute_command_count; ++i) {
        move_left_token_boundary(app);
    }
}
inline VIM_NTIMES_CUSTOM_COMMAND_SIG(_vim_ntimes_move_left_one_before_whitespace) {
    for (int i = 0; i < vim_state->execute_command_count; ++i) {
        move_left_whitespace_boundary(app);
    }
}

inline VIM_NTIMES_CUSTOM_COMMAND_SIG(_vim_ntimes_move_left_word_end) {
    for (int i = 0; i < vim_state->execute_command_count; ++i) {
        Scratch_Block scratch(app);
        current_view_scan_move(app, Scan_Backward, Side_Max, push_boundary_list(scratch, boundary_alpha_numeric, boundary_token));
        move_left(app);
    }
}
inline VIM_NTIMES_CUSTOM_COMMAND_SIG(_vim_ntimes_move_left_token_end) {
    for (int i = 0; i < vim_state->execute_command_count; ++i) {
        Scratch_Block scratch(app);
        current_view_scan_move(app, Scan_Backward, Side_Max, push_boundary_list(scratch, boundary_token));
        move_left(app);
    }
}
inline VIM_NTIMES_CUSTOM_COMMAND_SIG(_vim_ntimes_move_left_one_after_whitespace) {
    for (int i = 0; i < vim_state->execute_command_count; ++i) {
        Scratch_Block scratch(app);
        current_view_scan_move(app, Scan_Backward, Side_Max, push_boundary_list(scratch, boundary_non_whitespace));
        move_left(app);
    }
}

// @note execute once

/* @todo :non_virtual_whitespace
seek_pos_of_visual_line
seek_pos_of_textual_line
*/
inline VIM_NTIMES_CUSTOM_COMMAND_SIG(_vim_once_move_to_line_start) {
    seek_pos_of_visual_line(app, Side_Min);
}
inline VIM_NTIMES_CUSTOM_COMMAND_SIG(_vim_once_move_to_line_end) {
    seek_pos_of_visual_line(app, Side_Max);
}

inline VIM_NTIMES_CUSTOM_COMMAND_SIG(_vim_once_move_to_file_start) {
    view_set_cursor_and_preferred_x(app, view_id, seek_pos(0));
    no_mark_snap_to_cursor_if_shift(app, view_id);
}
inline VIM_NTIMES_CUSTOM_COMMAND_SIG(_vim_once_move_to_file_end) {
    i32 size = (i32)buffer_get_size(app, buffer_id);
    view_set_cursor_and_preferred_x(app, view_id, seek_pos(size));
    no_mark_snap_to_cursor_if_shift(app, view_id);
}

inline VIM_NTIMES_CUSTOM_COMMAND_SIG(_vim_once_goto_line) {
    goto_line(app);
    vim_scroll_cursor_line_to_view_center(app);
}


inline VIM_NTIMES_CUSTOM_COMMAND_SIG(_vim_set_mark) {
    set_mark(app);
}
inline VIM_NTIMES_CUSTOM_COMMAND_SIG(_vim_cursor_mark_swap) {
    cursor_mark_swap(app);
}

CUSTOM_COMMAND_SIG(vim_center_all_views) {
    Buffer_ID comp_buffer = get_comp_buffer(app);
    View_ID build_view_id  = get_first_view_with_buffer(app, comp_buffer);
    
    for_views(app, it) {
        if (it == build_view_id) {
            continue;
        }
        
        vim_scroll_cursor_line(app, 0, it);
    }
}

//
// @note compound commands
//

//
// @note execute command n times
//
// Uh, templates ...

template <VIM_NTIMES_CUSTOM_COMMAND_SIG(command), b32 one_past_last = false, b32 move_right_if_mode_insert = false>
CUSTOM_COMMAND_SIG(vim_command_execute_ntimes) {
    ProfileScope(app, "vim_command_execute_ntimes");
    
    VIM_GET_VIEW_ID_SCOPE_AND_VIM_STATE(app);
    Buffer_ID buffer_id = view_get_buffer(app, view_id, Access_Always);
    Managed_Scope buffer_scope = buffer_get_managed_scope(app, buffer_id);
    
    i64 pos_before = view_get_cursor_pos(app, view_id);
    
    command(app, view_id, buffer_id, view_scope, buffer_scope, vim_state);
    
    vim_state->execute_command_count = 1;
    vim_state->predecimal_count = 0;
    
    i64 pos_after = view_get_cursor_pos(app, view_id);
    
    
    Range_i64 diff_range = Ii64(pos_before, pos_after);
    if (one_past_last) {
        i64 buffer_size = buffer_get_size(app, buffer_id);
        if (diff_range.end < buffer_size) {
            diff_range.one_past_last = diff_range.end + 1;
        }
    }
    
    // @note Vim exec pending action
    vim_exec_pending_action(app, diff_range, false, move_right_if_mode_insert);
    
    
    // @note Update :vim_visual_range
    if (vim_state->mode == vim_mode_visual) {
        vim_update_visual_range(app, view_id, vim_state);
    }
    else if (vim_state->mode == vim_mode_visual_line) {
        vim_update_visual_line_range(app, view_id, vim_state);
    }
    else if (vim_state->mode == vim_mode_insert) {
        if (move_right_if_mode_insert) {
            move_right(app);
        }
    }
}

// @note execute ntimes

#define vim_move_up    vim_command_execute_ntimes<_vim_ntimes_move_up>
#define vim_move_down  vim_command_execute_ntimes<_vim_ntimes_move_down>
#define vim_move_left  vim_command_execute_ntimes<_vim_ntimes_move_left>
#define vim_move_right vim_command_execute_ntimes<_vim_ntimes_move_right>

#define vim_move_up_by_page   vim_command_execute_ntimes<_vim_ntimes_move_up_by_page>
#define vim_move_down_by_page vim_command_execute_ntimes<_vim_ntimes_move_down_by_page>
#define vim_move_up_by_page_half   vim_command_execute_ntimes<_vim_ntimes_move_up_by_page_half>
#define vim_move_down_by_page_half vim_command_execute_ntimes<_vim_ntimes_move_down_by_page_half>

#define vim_move_up_by_whitespace   vim_command_execute_ntimes<_vim_ntimes_move_up_by_whitespace>
#define vim_move_down_by_whitespace vim_command_execute_ntimes<_vim_ntimes_move_down_by_whitespace>


#define vim_move_right_word_start  vim_command_execute_ntimes<_vim_ntimes_move_right_word_start>
#define vim_move_right_token_start vim_command_execute_ntimes<_vim_ntimes_move_right_token_start>
#define vim_move_right_one_after_whitespace  vim_command_execute_ntimes<_vim_ntimes_move_right_one_after_whitespace>

#define vim_move_right_word_end  vim_command_execute_ntimes<_vim_ntimes_move_right_word_end, true, true>
#define vim_move_right_token_end vim_command_execute_ntimes<_vim_ntimes_move_right_token_end, true, true>
#define vim_move_right_one_before_whitespace  vim_command_execute_ntimes<_vim_ntimes_move_right_one_before_whitespace, true, true>


#define vim_move_left_word_start  vim_command_execute_ntimes<_vim_ntimes_move_left_word_start>
#define vim_move_left_token_start  vim_command_execute_ntimes<_vim_ntimes_move_left_token_start>
#define vim_move_left_one_before_whitespace  vim_command_execute_ntimes<_vim_ntimes_move_left_one_before_whitespace>

#define vim_move_left_word_end  vim_command_execute_ntimes<_vim_ntimes_move_left_word_end>
#define vim_move_left_token_end vim_command_execute_ntimes<_vim_ntimes_move_left_token_end>
#define vim_move_left_one_after_whitespace  vim_command_execute_ntimes<_vim_ntimes_move_left_one_after_whitespace>

// @note execute once

#define vim_move_to_file_start vim_command_execute_ntimes<_vim_once_move_to_file_start>
#define vim_move_to_file_end   vim_command_execute_ntimes<_vim_once_move_to_file_end>

#define vim_move_to_line_start vim_command_execute_ntimes<_vim_once_move_to_line_start>
#define vim_move_to_line_end   vim_command_execute_ntimes<_vim_once_move_to_line_end>

#define vim_goto_line vim_command_execute_ntimes<_vim_once_goto_line>

#define vim_move_right_to_found     vim_command_execute_ntimes<_vim_ntimes_move_right_to_found, true, true>
#define vim_move_right_before_found vim_command_execute_ntimes<_vim_ntimes_move_right_before_found, true, true>
#define vim_move_left_to_found      vim_command_execute_ntimes<_vim_ntimes_move_left_to_found, true>
#define vim_move_left_before_found  vim_command_execute_ntimes<_vim_ntimes_move_left_before_found, true>

#define vim_set_mark  vim_command_execute_ntimes<_vim_set_mark>
#define vim_cursor_mark_swap  vim_command_execute_ntimes<_vim_cursor_mark_swap>


//
// @note commands to increase the ntimes count
//
template <int x>
CUSTOM_COMMAND_SIG(vim_execute_command_count_add_predecimal) {
    VIM_GET_VIEW_ID_SCOPE_AND_VIM_STATE(app);
    
    if (x == 0 &&
        vim_state->predecimal_count == 0) {
        vim_state->execute_command_count = 1;
        return;
    }
    
    // if (vim_state->predecimal_count >= 3)  return;
    ++vim_state->predecimal_count;
    
    if (vim_state->predecimal_count > 1) {
        vim_state->execute_command_count *= 10;
        vim_state->execute_command_count += x;
    }
    else if (vim_state->predecimal_count == 1) {
        vim_state->execute_command_count = x;
    }
}

#define vim_execute_command_count_add_predecimal_0  vim_execute_command_count_add_predecimal<0>
#define vim_execute_command_count_add_predecimal_1  vim_execute_command_count_add_predecimal<1>
#define vim_execute_command_count_add_predecimal_2  vim_execute_command_count_add_predecimal<2>
#define vim_execute_command_count_add_predecimal_3  vim_execute_command_count_add_predecimal<3>
#define vim_execute_command_count_add_predecimal_4  vim_execute_command_count_add_predecimal<4>
#define vim_execute_command_count_add_predecimal_5  vim_execute_command_count_add_predecimal<5>
#define vim_execute_command_count_add_predecimal_6  vim_execute_command_count_add_predecimal<6>
#define vim_execute_command_count_add_predecimal_7  vim_execute_command_count_add_predecimal<7>
#define vim_execute_command_count_add_predecimal_8  vim_execute_command_count_add_predecimal<8>
#define vim_execute_command_count_add_predecimal_9  vim_execute_command_count_add_predecimal<9>

CUSTOM_COMMAND_SIG(vim_move_to_line_start__or__vim_execute_command_count_add_predecimal_0) {
    VIM_GET_VIEW_ID_SCOPE_AND_VIM_STATE(app);
    
    if (vim_state->predecimal_count == 0) {
        vim_move_to_line_start(app);
    }
    else {
        vim_execute_command_count_add_predecimal_0(app);
    }
}

//
// @note Selection commands
//

inline Range_i64
vim_get_token_or_word_under_cursor_range(Application_Links *app, Buffer_ID buffer_id, i64 cursor_pos, Token *token) {
    Range_i64 range = Ii64(-1, -1);
    
    if (!token) {
        // range = enclose_pos_alpha_numeric_underscore(app, buffer_id, cursor_pos);
        range = enclose_pos_non_whitespace(app, buffer_id, cursor_pos);
    }
    else if (token->kind == TokenBaseKind_Whitespace || token->kind == TokenBaseKind_EOF) {
        return range;
    }
    else if (token->kind == TokenBaseKind_Comment || token->kind == TokenBaseKind_LiteralString) {
        Scratch_Block scratch(app);
        String_Const_u8 lexeme = push_token_lexeme(app, scratch, buffer_id, token);
        i64 i = 0;
        for (i = cursor_pos; i >= token->pos && !character_is_whitespace(lexeme.str[i - token->pos]); --i);
        range.start = i + 1;
        for (i = cursor_pos; i < (token->pos+token->size) && !character_is_whitespace(lexeme.str[i - token->pos]); ++i);
        range.end = i;
        
        if (token->kind == TokenBaseKind_LiteralString) {
            if (range.start == token->pos && range.end != (token->pos+token->size))    ++range.start;
            if (range.end   == (token->pos+token->size) && range.start != token->pos)  --range.end;
        }
    }
    else {
        range = Ii64_size(token->pos, token->size);
    }
    
    return range;
}

CUSTOM_COMMAND_SIG(vim_select_token_or_word_under_cursor) {
    View_ID view_id = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer_id = view_get_buffer(app, view_id, Access_ReadVisible);
    
    i64 cursor_pos = view_get_cursor_pos(app, view_id);
    Token *token = get_token_from_pos(app, buffer_id, cursor_pos);
    
    Range_i64 range = vim_get_token_or_word_under_cursor_range(app, buffer_id, cursor_pos, token);
    range.end = range.one_past_last - 1;
    if (range.start < 0 || range.end < 0)  return;
    
    // :cursor_mark
    i64 new_cursor_pos;
    i64 new_mark_pos;
    i64 start_cursor_diff = Max(range.start, cursor_pos) - Min(range.start, cursor_pos);
    i64 end_cursor_diff = Max(range.end, cursor_pos) - Min(range.end, cursor_pos);
    if (start_cursor_diff <= end_cursor_diff) {
        new_cursor_pos = range.start;
        new_mark_pos = range.end;
    }
    else {
        new_cursor_pos = range.end;
        new_mark_pos = range.start;
    }
    view_set_cursor_and_preferred_x(app, view_id, seek_pos(new_cursor_pos));
    view_set_mark(app, view_id, seek_pos(new_mark_pos));
    
    ++range.one_past_last;
    vim_exec_pending_action(app, range);
}

CUSTOM_COMMAND_SIG(vim_visual_select_token_or_word_under_cursor) {
    VIM_GET_VIEW_ID_SCOPE_AND_VIM_STATE(app);
    if (vim_state->mode != vim_mode_visual) {
        vim_enter_mode_visual(app, false);
    }
    
    vim_select_token_or_word_under_cursor(app);
    vim_update_visual_range(app, view_id);
}

//
// @note action commands
//

#if 0
// :search
#define vim_search          vim_window_command<search>
#define vim_reverse_search  vim_window_command<reverse_search>
#define vim_search_identifier          vim_window_command<search_identifier>
#define vim_reverse_search_identifier  vim_window_command<reverse_search_identifier>
#endif

// :search
function void
vim_isearch_token_or_word(Application_Links *app, Scan_Direction scan) {
    View_ID view_id = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer_id = view_get_buffer(app, view_id, Access_ReadVisible);
    
    i64 cursor_pos = view_get_cursor_pos(app, view_id);
    Token *token = get_token_from_pos(app, buffer_id, cursor_pos);
    
    Range_i64 range = vim_get_token_or_word_under_cursor_range(app, buffer_id, cursor_pos, token);
    if (range.min < 0 || range.max < 0)  return;
    
    Scratch_Block scratch(app);
    String_Const_u8 query = push_buffer_range(app, scratch, buffer_id, range);
    isearch(app, scan, range.first, query);    
}
CUSTOM_COMMAND_SIG(vim_search_token_or_word) {
    vim_isearch_token_or_word(app, Scan_Forward);
}
CUSTOM_COMMAND_SIG(vim_reverse_search_token_or_word) {
    vim_isearch_token_or_word(app, Scan_Backward);
}


CUSTOM_COMMAND_SIG(vim_delete_char) {
    VIM_GET_VIEW_ID_SCOPE_AND_VIM_STATE(app);
    vim_state->pending_action = vim_action_delete_range;
    vim_move_right(app);
}

CUSTOM_COMMAND_SIG(vim_delete_line) {
    VIM_GET_VIEW_ID_SCOPE_AND_VIM_STATE(app);
    vim_state->pending_action = vim_action_delete_range;
    vim_exec_pending_action_on_line_range(app);
}

CUSTOM_COMMAND_SIG(vim_yank_line) {
    VIM_GET_VIEW_ID_SCOPE_AND_VIM_STATE(app);
    vim_state->pending_action = vim_action_yank_range;
    vim_exec_pending_action_on_line_range(app);
}


function void
vim_replace_character(Application_Links *app, String_Const_u8 replace) {
    if (replace.str == 0 || replace.size <= 0)  return;
    
    View_ID view_id = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer_id = view_get_buffer(app, view_id, Access_ReadWriteVisible);
    
    Range_i64 range = {};
    range.start = view_get_cursor_pos(app, view_id);
    range.end = range.start + 1;
    
    buffer_replace_range(app, buffer_id, range, replace);
}
CUSTOM_COMMAND_SIG(vim_replace_character) {
    User_Input user_input = get_current_input(app);
    String_Const_u8 replace = to_writable(&user_input);
    vim_replace_character(app, replace);
}
function void
vim_replace_character_and_move_right(Application_Links *app, String_Const_u8 replace) {
    vim_replace_character(app, replace);
    move_right(app);
}
CUSTOM_COMMAND_SIG(vim_replace_character_and_move_right) {
    vim_replace_character(app);
    move_right(app);
}
CUSTOM_COMMAND_SIG(vim_replace_character_and_enter_mode_normal) {
    vim_replace_character(app);
    vim_enter_mode_normal(app);
}


//
// @note Insert mode specific commands
//

CUSTOM_COMMAND_SIG(vim_write_text_and_maybe_auto_close_and_auto_indent) {
    ProfileScope(app, "write_text_and_maybe_auto_close_and_auto_indent");
    Scratch_Block scratch(app);
    
    User_Input in = get_current_input(app);
    String_Const_u8 input_string = to_writable(&in);
    if (input_string.str == 0 || input_string.size <= 0)  return;
    String_Const_u8 insert_string = SCu8(input_string.str);
    
    View_ID view_id = get_active_view(app, Access_Always);
    i64 cursor_pos = view_get_cursor_pos(app, view_id);
    Buffer_ID buffer_id = view_get_buffer(app, view_id, Access_ReadWriteVisible);
    
    b32 do_auto_indent = false; // @todo :non_virtual_whitespace behaviour?
    b32 check_for_closing_tag = false;
    i64 cursor_offset = 0;
    
    b32 inside_quotes = false; // string and character quotes
    {
        // @copynpaste refactor to local function
        Token *token = get_token_from_pos(app, buffer_id, cursor_pos);
        if (token && token->pos != cursor_pos) {
            if (token->kind == TokenBaseKind_LiteralString) {
                inside_quotes = true;
            }
        }
    }
    // @note: Control characters / Escape sequences
    if (inside_quotes) {
        switch (input_string.str[0]) {
            case '\n': {
                insert_string = SCu8("\\n");
                goto skip_auto_close;
            } break;
            case '\t': {
                insert_string = SCu8("\\t");
                goto skip_auto_close;
            } break;
            case '\v': {
                insert_string = SCu8("\\v");
                goto skip_auto_close;
            } break;
            case '\f': {
                insert_string = SCu8("\\f");
                goto skip_auto_close;
            } break;
            case '\r': {
                insert_string = SCu8("\\r");
                goto skip_auto_close;
            } break;
        }
    }
    else {
        if (input_string.str[0] == '\n') {
            vim_improved_newline(app);
            return;
        }
    }
    
    // :auto_close and check if need :auto_indent
    switch (input_string.str[0]) {
        case '(': {
            insert_string = SCu8("()");
            --cursor_offset;
        } break;
        
        case '[': {
            insert_string = SCu8("[]");
            --cursor_offset;
        } break;
        
        case '{': {
            insert_string = SCu8("{}");
            --cursor_offset;
            
            // :auto_close_semicolon
            // @note Check before the open brace for: struct, enum, union, defer, ...
            Token_Array token_array = get_token_array_from_buffer(app, buffer_id);
            if (token_array.tokens == 0)  break;//case '{':
            Token_Iterator_Array it = token_iterator_pos(0, &token_array, cursor_pos);
            i64 scope_depth = 0;
            i64 paren_depth = 0;
            for (;;) {
                // @note At start of for loop to skip token under cursor!
                // @note dec non whitespace and non comment
                if (!token_it_dec(&it)) {
                    break;
                }
                Token *token = token_it_read(&it);
                if (!token)  break;
                
                switch (token->sub_kind) {
                    case TokenCppKind_BraceCl: {
                        ++scope_depth;
                    } break;
                    case TokenCppKind_BraceOp: {
                        --scope_depth;
                    } break;
                    
                    case TokenCppKind_ParenCl: {
                        ++paren_depth;
                    } break;
                    case TokenCppKind_ParenOp: {
                        --paren_depth;
                    } break;
                }
                if (scope_depth < 0 || paren_depth < 0) {
                    break;
                }
                if (scope_depth == 0 && paren_depth == 0) {
                    if (token->sub_kind == TokenCppKind_Semicolon) {
                        break;
                    }
                    
                    switch (token->sub_kind) {
                        case TokenCppKind_Identifier: {
                            String_Const_u8 lexeme = push_token_lexeme(app, scratch, buffer_id, token);
                            if (string_match(lexeme, SCu8("defer"))) {}
                            else {
                                break;
                            }
                        }
                        case TokenCppKind_Class:
                        case TokenCppKind_Struct:
                        case TokenCppKind_Enum:
                        case TokenCppKind_Union: {
                            insert_string = SCu8("{};");
                            --cursor_offset;
                        } break;
                        
                        case TokenCppKind_Case: {
                            insert_string = SCu8("{} break;");
                            cursor_offset -= 7;
                        } break;
                    }
                }
            }
        } break;
        case '<': {
            insert_string = SCu8("<");
            
            Token_Array token_array = get_token_array_from_buffer(app, buffer_id);
            if (token_array.tokens == 0)  break;
            Token_Iterator_Array it = token_iterator_pos(0, &token_array, cursor_pos);
            if (!token_it_dec_non_whitespace(&it))  break;
            Token *token = token_it_read(&it);
            if (!token)  break;
            if (token->sub_kind == TokenCppKind_PPInclude) {
                insert_string = SCu8("<>");
                --cursor_offset;
            }
        } break;
        case '*': {
            String_Const_u8 next_char = push_buffer_range(app, scratch, buffer_id, Ii64_size(cursor_pos-1, 1));
            if (next_char.str && next_char.str[0] == '/') {
                insert_string = SCu8("**/");
                cursor_offset -= 2;
            }
            else {
                // check_for_closing_tag = true;
                // @todo closing_tag length is greater than 1. We are not doing that, and I don' think we need it for now.
            }
        } break;
        
        case '\'': {
            // @note Minus 1, because the insert pos is like before the actual cursor pos.
            Token *token = get_token_from_pos(app, buffer_id, cursor_pos-1);
            if (token && token->kind == TokenBaseKind_Comment) {
                insert_string = SCu8("'");
                check_for_closing_tag = true;
            }
            else {
                String_Const_u8 prev_char = push_buffer_range(app, scratch, buffer_id, Ii64_size(cursor_pos-1, 1));
                if (prev_char.str && prev_char.str[0] == '\\') {
                    insert_string = SCu8("'");
                    check_for_closing_tag = false;
                }
                else {
                    insert_string = SCu8("''");
                    --cursor_offset;
                    check_for_closing_tag = true;
                }
            }
        } break;
        case '\"': {
            // @note Minus 1, because the insert pos is like before the actual cursor pos.
            Token *token = get_token_from_pos(app, buffer_id, cursor_pos-1);
            if (token && token->kind == TokenBaseKind_Comment) {
                insert_string = SCu8("\"");
                check_for_closing_tag = true;
            }
            else {
                String_Const_u8 prev_char = push_buffer_range(app, scratch, buffer_id, Ii64_size(cursor_pos-1, 1));
                if (prev_char.str && prev_char.str[0] == '\\') {
                    insert_string = SCu8("\"");
                    check_for_closing_tag = false;
                }
                else {
                    insert_string = SCu8("\"\"");
                    --cursor_offset;
                    check_for_closing_tag = true;
                }
            }
        } break;
        
        case ')':
        case ']':
        case '}':
        case ';': {
            check_for_closing_tag = true;
        } break;
        
        default: {
            do_auto_indent = true;
        }
    }
    skip_auto_close:;
    
    // @note Check if we need to write the closing tag
    if (check_for_closing_tag) {
        String_Const_u8 next_char = push_buffer_range(app, scratch, buffer_id, Ii64_size(cursor_pos, 1));
        if (next_char.str && next_char.str[0] == input_string.str[0]) {
            cursor_offset = 1;
            goto skip_write_string;
        }
        // switch (next_char.str[0]) {}
    }
    
    // :auto_indent
    if (do_auto_indent) {
        Range_i64 pos = {};
        pos.min = view_get_cursor_pos(app, view_id);
        write_text(app, insert_string);
        pos.max = view_get_cursor_pos(app, view_id);
        auto_indent_buffer(app, buffer_id, pos, 0);
        move_past_lead_whitespace(app, view_id, buffer_id);
    }
    else {
        write_text(app, insert_string);
    }
    skip_write_string:;
    
    // :cursor_mark
    view_set_cursor_by_character_delta(app, view_id, cursor_offset);
    no_mark_snap_to_cursor_if_shift(app, view_id);
}


//
// @note visual mode specific commands
//

CUSTOM_COMMAND_SIG(vim_visual_mode_delete) {
    VIM_GET_VIEW_ID_SCOPE_AND_VIM_STATE(app);
    vim_state->pending_action = vim_action_delete_range;
    vim_exec_pending_action(app, vim_state->selection_range, (vim_state->mode == vim_mode_visual_line));
    vim_enter_mode_normal(app);
}

CUSTOM_COMMAND_SIG(vim_visual_mode_yank) {
    VIM_GET_VIEW_ID_SCOPE_AND_VIM_STATE(app);
    vim_state->pending_action = vim_action_yank_range;
    vim_exec_pending_action(app, vim_state->selection_range, (vim_state->mode == vim_mode_visual_line));
    vim_enter_mode_normal(app);
}

CUSTOM_COMMAND_SIG(vim_visual_mode_change) {
    VIM_GET_VIEW_ID_SCOPE_AND_VIM_STATE(app);
    vim_state->pending_action = vim_action_change_range;
    vim_exec_pending_action(app, vim_state->selection_range, (vim_state->mode == vim_mode_visual_line));
}


// :search visual selection
function void vim_isearch(Application_Links *app, Scan_Direction start_scan, i64 first_pos, String_Const_u8 query_init, b32 union_visual_selection_range = true);

function void
vim_isearch_visual_selection(Application_Links *app, Scan_Direction scan){
    VIM_GET_VIEW_ID_VIEW_SCOPE_BUFFER_ID_AND_VIM_STATE(app);
    Range_i64 range = vim_state->selection_range;
    
    Scratch_Block scratch(app);
    String_Const_u8 query = push_buffer_range(app, scratch, buffer_id, range);
    vim_isearch(app, scan, range.first, query, false);
}
CUSTOM_COMMAND_SIG(vim_search_visual_selection) {
    vim_isearch_visual_selection(app, Scan_Forward);
}
CUSTOM_COMMAND_SIG(vim_reverse_search_visual_selection) {
    vim_isearch_visual_selection(app, Scan_Backward);
}


// :surround
function void
vim_surround_range_lines_with(Application_Links *app, Range_i64 range, char *begin, char *end) {
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    
    Range_i64 lines = get_line_range_from_pos_range(app, buffer, range);
    range = get_pos_range_from_line_range(app, buffer, lines);
    
    Scratch_Block scratch(app);
    
    b32 min_line_blank = line_is_valid_and_blank(app, buffer, lines.min);
    b32 max_line_blank = line_is_valid_and_blank(app, buffer, lines.max);
    
    if ((lines.min < lines.max) || (!min_line_blank)){
        String_Const_u8 begin_str = {};
        String_Const_u8 end_str = {};
        
        i64 min_adjustment = 0;
        i64 max_adjustment = 0;
        
        if (min_line_blank){
            begin_str = push_u8_stringf(scratch, "\n%s", begin);
            min_adjustment += 1;
        }
        else{
            begin_str = push_u8_stringf(scratch, "%s\n", begin);
        }
        if (max_line_blank){
            end_str = push_u8_stringf(scratch, "%s\n", end);
        }
        else{
            end_str = push_u8_stringf(scratch, "\n%s", end);
            max_adjustment += 1;
        }
        
        max_adjustment += begin_str.size;
        Range_i64 new_pos = Ii64(range.min + min_adjustment, range.max + max_adjustment);
        
        History_Group group = history_group_begin(app, buffer);
        buffer_replace_range(app, buffer, Ii64(range.min), begin_str);
        buffer_replace_range(app, buffer, Ii64(range.max + begin_str.size), end_str);
        history_group_end(group);
        
        set_view_range(app, view, new_pos);
    }
    else{
        String_Const_u8 str = push_u8_stringf(scratch, "%s\n\n%s", begin, end);
        buffer_replace_range(app, buffer, range, str);
        i64 center_pos = range.min + cstring_length(begin) + 1;
        view_set_cursor_and_preferred_x(app, view, seek_pos(center_pos));
        view_set_mark(app, view, seek_pos(center_pos));
    }
}
function void
vim_surround_range_with(Application_Links *app, Range_i64 range, char *begin, char *end) {
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadWriteVisible);
    
    String_Const_u8 begin_str = SCu8(begin);
    String_Const_u8 end_str   = SCu8(end);
    
    Range_i64 new_pos = Ii64(range.min, range.max + (begin_str.size + end_str.size) - 1);
    
    History_Group group = history_group_begin(app, buffer);
    buffer_replace_range(app, buffer, Ii64(range.min), begin_str);
    buffer_replace_range(app, buffer, Ii64(range.max + begin_str.size), end_str);
    history_group_end(group);
    
    set_view_range(app, view, new_pos);
}
inline void
vim_surround_with(Application_Links *app, char *str_begin, char *str_end, b32 is_line = -1) {
    VIM_GET_VIEW_ID_VIEW_SCOPE_BUFFER_ID_AND_VIM_STATE(app);
    Range_i64 range = vim_state->selection_range;
    if (is_line == -1) {
        is_line = (vim_state->mode == vim_mode_visual_line);
    }
    
    if (is_line) {
        --range.one_past_last;
        vim_surround_range_lines_with(app, range, str_begin, str_end);
    }
    else {
        vim_surround_range_with(app, range, str_begin, str_end);
    }
    
    // vim_update_visual_range(app, view_id, vim_state);
    vim_enter_mode_normal(app);
}
function void
vim_range_remove_surrounding(Application_Links *app, Range_i64 range, String_Const_u8 begin, String_Const_u8 end, b32 is_line) {
    VIM_GET_VIEW_ID_AND_BUFFER_ID(app);
    if (is_line) {
        --range.one_past_last;
        range = get_pos_range_from_line_range(app, buffer_id, get_line_range_from_pos_range(app, buffer_id, range));
    }
    
    i64 buffer_size = buffer_get_size(app, buffer_id);
    
    String_Match match_begin = buffer_seek_string(app, buffer_id, begin, Scan_Forward, range.start-1);
    if (match_begin.range.start >= range.end)  return;
    String_Match match_end   = buffer_seek_string(app, buffer_id, end,   Scan_Backward, range.end);
    if (match_end.range.start < range.start)  return;
    
    Range_i64 range_begin = match_begin.range;
    Range_i64 range_end   = match_end.range;
    
    if (range_end.start < range_begin.end)  return;
    
    if (is_line) {
        range_begin = get_pos_range_from_line_range(app, buffer_id, get_line_range_from_pos_range(app, buffer_id, range_begin));
        range_end   = get_pos_range_from_line_range(app, buffer_id, get_line_range_from_pos_range(app, buffer_id, range_end));
        ++range_begin.end;
        ++range_end.end;
    }
    
#if 0
    // Weird bug were it deletes some text outside of the ranges specified.
    
    Batch_Edit batch_begin = {};
    Batch_Edit batch_end   = {};
    
    batch_begin.edit.text = SCu8("");
    batch_begin.edit.range = range_begin;
    batch_begin.next = &batch_end;
    
    batch_end.edit.text = SCu8("");
    batch_end.edit.range = range_end;
    batch_end.next = 0;
    
    buffer_batch_edit(app, buffer_id, &batch_begin);
#else
    
    History_Group group = history_group_begin(app, buffer_id);
    buffer_replace_range(app, buffer_id, range_begin, string_u8_empty);
    i64 range_begin_size = range_size(range_begin);
    buffer_replace_range(app, buffer_id, Ii64(range_end.start - range_begin_size, range_end.end - range_begin_size), string_u8_empty);
    history_group_end(group);
#endif
}
function void
vim_remove_surrounding(Application_Links *app, char *str_begin, char *str_end, b32 is_line = -1) {
    VIM_GET_VIEW_ID_VIEW_SCOPE_BUFFER_ID_AND_VIM_STATE(app);
    Range_i64 range = vim_state->selection_range;
    if (is_line == -1) {
        is_line = (vim_state->mode == vim_mode_visual_line);
    }
    
    String_Const_u8 begin = SCu8(str_begin);
    String_Const_u8 end   = SCu8(str_end);
    vim_range_remove_surrounding(app, range, begin, end, is_line);
    
    // vim_update_visual_range(app, view_id, vim_state);
    vim_enter_mode_normal(app);
}

CUSTOM_COMMAND_SIG(vim_visual_surround_with_parenthesis) {
    vim_surround_with(app, "(", ")", false);
}
CUSTOM_COMMAND_SIG(vim_visual_remove_surrounding_parenthesis) {
    vim_remove_surrounding(app, "(", ")", false);
}

CUSTOM_COMMAND_SIG(vim_visual_surround_with_braces) {
    vim_surround_with(app, "{", "}");
}
CUSTOM_COMMAND_SIG(vim_visual_remove_surrounding_braces) {
    vim_remove_surrounding(app, "{", "}");
}

CUSTOM_COMMAND_SIG(vim_visual_surround_with_brackets) {
    vim_surround_with(app, "[", "]", false);
}
CUSTOM_COMMAND_SIG(vim_visual_remove_surrounding_brackets) {
    vim_remove_surrounding(app, "[", "]", false);
}

CUSTOM_COMMAND_SIG(vim_visual_surround_with_static_if) {
    vim_surround_with(app, "#if 0", "#endif", true);
}
CUSTOM_COMMAND_SIG(vim_visual_remove_surrounding_static_if) {
    vim_remove_surrounding(app, "#if 0", "#endif", true);
}

CUSTOM_COMMAND_SIG(vim_visual_surround_with_double_quotes) {
    vim_surround_with(app, "\"", "\"", false);
}
CUSTOM_COMMAND_SIG(vim_visual_remove_surround_with_double_quotes) {
    vim_remove_surrounding(app, "\"", "\"", false);
}

CUSTOM_COMMAND_SIG(vim_visual_surround_with_single_quotes) {
    vim_surround_with(app, "\'", "\'", false);
}
CUSTOM_COMMAND_SIG(vim_visual_remove_surround_with_single_quotes) {
    vim_remove_surrounding(app, "\'", "\'", false);
}

CUSTOM_COMMAND_SIG(vim_visual_surround_with_comment) {
    vim_surround_with(app, "/*", "*/");
}
CUSTOM_COMMAND_SIG(vim_visual_remove_surround_with_comment) {
    vim_remove_surrounding(app, "/*", "*/");
}
CUSTOM_COMMAND_SIG(vim_visual_comment_line_range) {
    VIM_GET_VIEW_ID_VIEW_SCOPE_BUFFER_ID_AND_VIM_STATE(app);
    Range_i64 pos_range = vim_state->selection_range;
    
    History_Group group = history_group_begin(app, buffer_id);    
    Range_i64 line_range = get_line_range_from_pos_range(app, buffer_id, pos_range);
    for (i64 line = line_range.start; line < line_range.one_past_last; ++line) {
        i64 line_start_pos = get_line_start_pos(app, buffer_id, line);
        b32 has_comment = c_line_comment_starts_at_position(app, buffer_id, line_start_pos);
        if (has_comment)  continue;
        buffer_replace_range(app, buffer_id, Ii64(line_start_pos), string_u8_litexpr("//"));
    }
    history_group_end(group);
    
    // vim_update_visual_range(app, view_id, vim_state);
    vim_enter_mode_normal(app);
}
CUSTOM_COMMAND_SIG(vim_visual_uncomment_line_range) {
    global_history_edit_group_begin(app);
    
    VIM_GET_VIEW_ID_VIEW_SCOPE_BUFFER_ID_AND_VIM_STATE(app);
    Range_i64 pos_range = vim_state->selection_range;
    
    History_Group group = history_group_begin(app, buffer_id);    
    Range_i64 line_range = get_line_range_from_pos_range(app, buffer_id, pos_range);
    for (i64 line = line_range.start; line < line_range.one_past_last; ++line) {
        i64 line_start_pos = get_line_start_pos(app, buffer_id, line);
        b32 has_comment = c_line_comment_starts_at_position(app, buffer_id, line_start_pos);
        if (!has_comment)  continue;
        buffer_replace_range(app, buffer_id, Ii64(line_start_pos, line_start_pos + 2), string_u8_empty);
    }
    history_group_end(group);
    
    // vim_update_visual_range(app, view_id, vim_state);
    vim_enter_mode_normal(app);
    
    global_history_edit_group_end(app);
}

//
// @note paste commands
//

function void
vim_paste(Application_Links *app, b32 paste_after = true) {
    ProfileScope(app, "vim_paste");
    
    i32 count = clipboard_count(app, 0);
    if (count <= 0)  return;
    
    View_ID view_id = get_active_view(app, Access_ReadWriteVisible);
    if_view_has_highlighted_range_delete_range(app, view_id);
    
    Managed_Scope view_scope = view_get_managed_scope(app, view_id);
    Rewrite_Type *next_rewrite = scope_attachment(app, view_scope, view_next_rewrite_loc, Rewrite_Type);
    *next_rewrite = Rewrite_Paste;
    i32 *paste_index = scope_attachment(app, view_scope, view_paste_index_loc, i32);
    *paste_index = 0;
    
    Scratch_Block scratch(app);
    String_Const_u8 string = push_clipboard_index(app, scratch, 0, *paste_index);
    if (string.size <= 0)  return;
    
    // @note Check if there are newline characters in the string
    b32 is_line = false;
    for (u64 i = 0; i < string.size; ++i) {
        if (character_is_newline(string.str[i])) {
            is_line = true;
            break;
        }
    }
    
    i64 pos = view_get_cursor_pos(app, view_id);
    Buffer_ID buffer_id = view_get_buffer(app, view_id, Access_ReadWriteVisible);
    i64 buffer_size = buffer_get_size(app, buffer_id);
    
    if (is_line) {
        i64 line_number = get_line_number_from_pos(app, buffer_id, pos);
        if (paste_after) {
            i64 buffer_line_count = buffer_get_line_count(app, buffer_id);
            if (line_number == buffer_line_count) {
                buffer_replace_range(app, buffer_id, Ii64(buffer_size), string_u8_litexpr("\n"));
            }
            ++line_number;
        }
        pos = get_line_start_pos(app, buffer_id, line_number);
    }
    else if (paste_after) {
        if (pos < buffer_size)  ++pos;
    }
    
    buffer_replace_range(app, buffer_id, Ii64(pos), string);
    
    // :cursor_mark
    // @note paste_after positions
    i64 new_cursor_pos = pos + (i64)string.size - 1;
    i64 new_mark_pos = pos;
    if (!paste_after) {
        // @note paste_before positions
        new_mark_pos = new_cursor_pos;
        new_cursor_pos = pos;
    }
    view_set_cursor_and_preferred_x(app, view_id, seek_pos(new_cursor_pos));
    view_set_mark(app, view_id, seek_pos(new_mark_pos));
    
    // @note Fade pasted text
    ARGB_Color argb_fade = fcolor_resolve(fcolor_id(defcolor_paste));
    view_post_fade(app, view_id, 0.667f, Ii64_size(pos, string.size), argb_fade);
#if 0
    // @todo 4coder 4.0 Do we need to do that?
    if (!is_line) {
        view_post_fade(app, &view, 0.667f, pos, pos + len, paste.color);
    } else {
        view_post_fade(app, &view, 0.667f, seek_line_beginning(app, &buffer, pos), seek_line_beginning(app, &buffer, pos) + len, paste.color);
    }
#endif
}

CUSTOM_COMMAND_SIG(vim_paste_after) {
    vim_paste(app, true);
}
CUSTOM_COMMAND_SIG(vim_paste_after_and_indent) {
    vim_paste_after(app);
    auto_indent_range(app);
}

CUSTOM_COMMAND_SIG(vim_paste_before) {
    vim_paste(app, false);
}
CUSTOM_COMMAND_SIG(vim_paste_before_and_indent) {
    vim_paste_before(app);
    auto_indent_range(app);
}

CUSTOM_COMMAND_SIG(vim_paste_next) {
    ProfileScope(app, "vim_paste_next");
    
    Scratch_Block scratch(app);
    
    i32 count = clipboard_count(app, 0);
    if (count <= 0)  return;
    View_ID view_id = get_active_view(app, Access_ReadWriteVisible);
    Managed_Scope view_scope = view_get_managed_scope(app, view_id);
    no_mark_snap_to_cursor(app, view_scope);
    
    Rewrite_Type *rewrite = scope_attachment(app, view_scope, view_rewrite_loc, Rewrite_Type);
    if (*rewrite != Rewrite_Paste)  return;
    
    Rewrite_Type *next_rewrite = scope_attachment(app, view_scope, view_next_rewrite_loc, Rewrite_Type);
    *next_rewrite = Rewrite_Paste;
    
    i32 *paste_index_ptr = scope_attachment(app, view_scope, view_paste_index_loc, i32);
    i32 paste_index = (*paste_index_ptr) + 1;
    *paste_index_ptr = paste_index;
    
    String_Const_u8 string = push_clipboard_index(app, scratch, 0, paste_index);
    
    Buffer_ID buffer_id = view_get_buffer(app, view_id, Access_ReadWriteVisible);
    
    Range_i64 range = get_view_range(app, view_id);
    range.one_past_last = range.end + 1;
    i64 pos = range.min;
    
    buffer_replace_range(app, buffer_id, range, string);
    view_set_cursor_and_preferred_x(app, view_id, seek_pos(pos + string.size));
    
    // :cursor_mark
    // @todo The cursor mark pos should keep the previous order,
    //       depending on if it was paste_after or paste_before.
    i64 new_cursor_pos = pos + (i64)string.size - 1;
    i64 new_mark_pos = pos;
    view_set_cursor_and_preferred_x(app, view_id, seek_pos(new_cursor_pos));
    view_set_mark(app, view_id, seek_pos(new_mark_pos));
    
    // @note Fade pasted text
    ARGB_Color argb = fcolor_resolve(fcolor_id(defcolor_paste));
    view_post_fade(app, view_id, 0.667f, Ii64_size(pos, string.size), argb);
    
}
CUSTOM_COMMAND_SIG(vim_paste_next_and_indent) {
    vim_paste_next(app);
    auto_indent_range(app);
}

CUSTOM_COMMAND_SIG(vim_visual_paste) {
    ProfileScope(app, "vim_visual_paste");
    
    VIM_GET_VIEW_ID_SCOPE_AND_VIM_STATE(app);
    if (vim_state->mode != vim_mode_visual && vim_state->mode != vim_mode_visual_line)  return;
    
    i32 count = clipboard_count(app, 0);
    if (count <= 0)  return;
    
    i64 cursor_pos = view_get_cursor_pos(app, view_id);
    i64 mark_pos   = view_get_mark_pos(app, view_id);
    
    Rewrite_Type *next_rewrite = scope_attachment(app, view_scope, view_next_rewrite_loc, Rewrite_Type);
    *next_rewrite = Rewrite_Paste;
    i32 *paste_index = scope_attachment(app, view_scope, view_paste_index_loc, i32);
    *paste_index = 0;
    
    Scratch_Block scratch(app);
    String_Const_u8 clipboard_string = push_clipboard_index(app, scratch, 0, *paste_index);
    if (clipboard_string.size <= 0)  return;
    
    Range_i64 range = vim_state->selection_range;
    Buffer_ID buffer_id = view_get_buffer(app, view_id, Access_ReadWriteVisible);
    buffer_replace_range(app, buffer_id, range, clipboard_string);
    
    // @note Fade pasted text
    ARGB_Color argb = fcolor_resolve(fcolor_id(defcolor_paste));
    view_post_fade(app, view_id, 0.667f, range, argb);
    
    // :vim_mode
    vim_enter_mode_normal(app);
    
    // :cursor_mark
    i64 new_cursor_pos = range.start;
    i64 new_mark_pos   = range.start + (i64)clipboard_string.size - 1;
    if (cursor_pos > mark_pos) {
        new_cursor_pos = new_mark_pos;
        new_mark_pos   = range.start;
    }
    view_set_cursor_and_preferred_x(app, view_id, seek_pos(new_cursor_pos));
    view_set_mark(app, view_id, seek_pos(new_mark_pos));
}
CUSTOM_COMMAND_SIG(vim_visual_paste_and_indent) {
    vim_visual_paste(app);
    auto_indent_range(app);
}

//
// @note Improved newline
//
// @todo Arguments as enum?
// @todo Maybe use List_String_Const_u8 and string_list_flatten() if needed.
// @cleanup Some ranges can just be i64
// @todo Multiline comment, maybe insert * at the start, because of 4coder indentation.
function void
vim_improved_newline(Application_Links *app, b32 below/* = true*/, b32 newline_at_cursor/* = true*/) {
    ProfileScope(app, "vim_improved_newline");
    
    VIM_GET_VIEW_ID_AND_BUFFER_ID(app);
    
    // @note Get current line range
    i64 cursor_pos = view_get_cursor_pos(app, view_id);
    i64 line_number = get_line_number_from_pos(app, buffer_id, cursor_pos);
    Range_i64 line_range = get_line_pos_range(app, buffer_id, line_number);
    line_range.one_past_last = line_range.end + 1;
    
    Scratch_Block scratch(app);
    String_Const_u8 line_continuation_string = {0};
    
    // @note Check for comment and surrounding braces
    b32 has_surrounding_braces = false;
    b32 cursor_is_inside_comment = false;
    Managed_Scope buffer_scope = buffer_get_managed_scope(app, buffer_id);
    b32 file_is_c_like = *scope_attachment(app, buffer_scope, buffer_file_is_c_like, b32);
    if (file_is_c_like) {
        // @note Preserve 3 chars, one at the beginning for a newline
        //       and to at the end for a whitespace and newline
        line_continuation_string.str = push_array(scratch, u8, range_size(line_range + 3));
        line_continuation_string.str[0] = '\n';
        ++line_continuation_string.str;
        
        String_Const_u8 line_string = line_continuation_string;
        line_string.size = range_size(line_range);
        buffer_read_range(app, buffer_id, line_range, line_string.str);
        if (line_string.size <= 1) {
            if (line_string.str[0] == '\n') {
                line_continuation_string.size = 1;
            }
            goto end_c_comment;
        }
        
        // @note Get comment range
        b32 line_is_comment = false;
        Range_i64 comment_range = {};
        u8 *c = line_string.str;
        int i = 0;
        for (; i < line_string.size; ++i) {
            if (!character_is_whitespace(*c)) {
                if (*c == '/' && *(c + 1) == '/') {
                    line_is_comment = true;
                    comment_range = Ii64(line_range.start + i, line_range.one_past_last);
                    c += 2;
                }
                break;
            }
            ++c;
        }
        if (!line_is_comment) {
            // @note Check for surrounding braces, if not inside a comment
            i64 braces_pos = cursor_pos - line_range.start - 1;
            if (line_string.str[braces_pos] == '{'  &&
                line_string.str[braces_pos+1] == '}') {
                has_surrounding_braces = true;
            }
            
            goto end_c_comment;
        }
        if (cursor_pos < comment_range.start + 2)  goto end_c_comment;
        cursor_is_inside_comment = true;
        
        // @note Get text range inside comment
        b32 cursor_is_inside_comment_text = false;
        Range_i64 comment_text_range = Ii64(comment_range.start);
        for (i += 2; i < line_string.size; ++i) {
            if (!character_is_whitespace(*c)) {
                comment_text_range = Ii64(line_range.start + i, line_range.one_past_last);
                cursor_is_inside_comment_text = (cursor_pos >= comment_text_range.start);
                break;
            }
            ++c;
        }
        
        // @note Search for colon as new comment_text_range start
        b32 cursor_is_after_comment_text_colon = false;
        for (; i < line_string.size; ++i) {
            if (*c == ':') {
                if (!character_is_whitespace(*(c - 1)) && character_is_whitespace(*(c + 1)) ) {
                    if (cursor_pos >= (line_range.start + i + 1)) {
                        cursor_is_after_comment_text_colon = true;
                        comment_text_range = Ii64(line_range.start + i + 2, line_range.one_past_last);
                    }
                    break;
                }
            }
            ++c;
        }
        
        // @note Get line continuation range and string
        Range_i64 line_continuation_range = {0};
        if (cursor_is_after_comment_text_colon) {
            for (i = (int)comment_range.start + 2; i < comment_text_range.start; ++i) {
                line_string.str[i - line_range.start] = ' ';
            }
            line_continuation_range = Ii64(line_range.start, comment_text_range.start);
        }
        else if (cursor_is_inside_comment_text) {
            line_continuation_range = Ii64(line_range.start, comment_text_range.start);
        }
        else if (cursor_is_inside_comment) {
            // @note Plus 1 adds one space after the double-slash
            line_continuation_range = Ii64(line_range.start, (comment_range.start + 2) + 1);
            line_string.str[(comment_range.start + 2) - line_range.start] = ' ';
        }
        if (cursor_is_inside_comment_text || cursor_is_after_comment_text_colon) {
            if (comment_text_range.start == (comment_range.start + 2)) {
                line_string.str[comment_text_range.start - line_range.start] = ' ';
                ++line_continuation_range.end;
            }
        }
        line_continuation_string = { line_string.str, (u64)range_size(line_continuation_range) };
    }
    end_c_comment:;
    
    if (line_continuation_string.str == 0) {
        line_continuation_string = SCu8("\n");
    }
    else if (line_continuation_string.size == 0) {
        --line_continuation_string.str;
        line_continuation_string.size = 1;
    }
    else if (line_continuation_string.str[line_continuation_string.size - 1] != '\n') {
        line_continuation_string.str[line_continuation_string.size++] = '\n';
    }
    
    // @note Get the insert position
    Range_i64 range = {};
    if (newline_at_cursor) {
        range = Ii64(cursor_pos);
        
        if (cursor_is_inside_comment) {
            --line_continuation_string.str;
            // @note Newline at the end is already removed because of decrementing the pointer.
            // --line_continuation_string.size;
        }
        else {
            if (has_surrounding_braces) {
                line_continuation_string.str[1] = '\n';
                line_continuation_string.size = 2;
            }
        }
    }
    else {
        if (below)  range = Ii64(line_range.one_past_last);
        else  range = Ii64(line_range.start);
    }
    
    buffer_replace_range(app, buffer_id, range, line_continuation_string);
    
    // :cursor_mark
    i64 new_cursor_pos = range.start + line_continuation_string.size;
    if (!newline_at_cursor ||
        (newline_at_cursor && has_surrounding_braces)) {
        --new_cursor_pos;
    }
    view_set_cursor_and_preferred_x(app, view_id, seek_pos(new_cursor_pos));
}

#if 0
CUSTOM_COMMAND_SIG(vim_newline) {
    vim_improved_newline(app);
}
#endif

//
// @note Window/view commands
//
template <CUSTOM_COMMAND_SIG(command), b32 view_changed_to_line_highlight/* = false*/>
CUSTOM_COMMAND_SIG(vim_window_command) {
    vim_enter_mode_normal(app);
    command(app);
    
    // @note Vim :view_changed_flash_line
    if (view_changed_to_line_highlight) {
        vim_global_view_changed_time = 1.0f;
    }
}

function void
vim_scroll_cursor_line(Application_Links *app, int view_pos, View_ID view_id) {
    ProfileScope(app, "vim_scroll_cursor_line");
    
    if (view_id == -1) {
        view_id = get_active_view(app, Access_Always);
    }
    f32 line_height = 0.f;
    if (view_pos != 0) {
        Buffer_ID buffer_id = view_get_buffer(app, view_id, Access_ReadVisible);
        Face_ID face_id = get_face_id(app, buffer_id);
        Face_Metrics metrics = get_face_metrics(app, face_id);
        line_height = metrics.line_height;
    }
    
    Rect_f32 region = view_get_buffer_region(app, view_id);
    i64 pos = view_get_cursor_pos(app, view_id);
    Buffer_Cursor cursor = view_compute_cursor(app, view_id, seek_pos(pos));
    f32 view_height = rect_height(region);
    Buffer_Scroll scroll = view_get_buffer_scroll(app, view_id);
    scroll.target.line_number = cursor.line;
    if (view_pos == 0) {
        scroll.target.pixel_shift.y = -view_height*0.5f;
    }
    else if (view_pos == -1) {
        scroll.target.pixel_shift.y =  -line_height;
    }
    else if (view_pos == 1) {
        scroll.target.pixel_shift.y = -(view_height - 2.0f*line_height);
        // @todo Not quite right when the line to scroll to is a wrapped line!
    }
    
    view_set_buffer_scroll(app, view_id, scroll, SetBufferScroll_SnapCursorIntoView);
}
CUSTOM_COMMAND_SIG(vim_scroll_cursor_line_to_view_center) {
    vim_scroll_cursor_line(app, 0);
    vim_reset_mode_mapid(app);
}
CUSTOM_COMMAND_SIG(vim_scroll_cursor_line_to_view_top) {
    vim_scroll_cursor_line(app, -1);
    vim_reset_mode_mapid(app);
}
CUSTOM_COMMAND_SIG(vim_scroll_cursor_line_to_view_bottom) {
    vim_scroll_cursor_line(app, 1);
    vim_reset_mode_mapid(app);
}


CUSTOM_COMMAND_SIG(_vim_swap_buffers_between_two_views) {
    swap_panels(app);
}

CUSTOM_COMMAND_SIG(_vim_cycle_view_focus) {
    View_ID view_id = get_active_view(app, Access_Always);
    View_ID next_view_id = get_next_view_looped_primary_panels(app, view_id, Access_Always);
    view_set_active(app, next_view_id);
}
CUSTOM_COMMAND_SIG(_vim_rotate_view_buffers) {
    // @todo Rotate all buffers
    _vim_swap_buffers_between_two_views(app);
}

CUSTOM_COMMAND_SIG(_vim_open_view_duplicate_split_vertical) {
    open_panel_vsplit(app);
}
CUSTOM_COMMAND_SIG(_vim_open_view_duplicate_split_horizontal) {
    open_panel_hsplit(app);
}

CUSTOM_COMMAND_SIG(_vim_open_view_split_vertical) {
    View_ID view = get_active_view(app, Access_Always);
    View_ID new_view = open_view(app, view, ViewSplit_Right);
    new_view_settings(app, new_view);
}
CUSTOM_COMMAND_SIG(_vim_open_view_split_horizontal) {
    View_ID view = get_active_view(app, Access_Always);
    View_ID new_view = open_view(app, view, ViewSplit_Bottom);
    new_view_settings(app, new_view);
}

CUSTOM_COMMAND_SIG(_vim_close_view) {
    close_panel(app);
}

// @todo Fix focus commands
CUSTOM_COMMAND_SIG(_vim_focus_view_left) {
    View_ID view_id = get_active_view(app, Access_Always);
    Rect_f32 view_rect = view_get_screen_rect(app, view_id);
    f32 x0 = view_rect.x0;
    f32 y0 = view_rect.y0;
    
    View_ID best_view_id = view_id;
    Rect_f32 best_view_rect = view_rect;
    
    for_views(app, it) {
        if (it == view_id)  continue;
        
        Rect_f32 it_rect = view_get_screen_rect(app, it);
        if (y0 < it_rect.y0 || y0 > it_rect.y1)  continue;
        if (x0 < it_rect.x0)  continue;
        
        if (best_view_id == view_id ||
            it_rect.x0 > best_view_rect.x0) {
            best_view_id   = it;
            best_view_rect = it_rect;
        }
    }
    
    view_set_active(app, best_view_id);
}
CUSTOM_COMMAND_SIG(_vim_focus_view_right) {
    View_ID view_id = get_active_view(app, Access_Always);
    Rect_f32 view_rect = view_get_screen_rect(app, view_id);
    f32 x0 = view_rect.x0;
    f32 y0 = view_rect.y0;
    
    View_ID best_view_id = view_id;
    Rect_f32 best_view_rect = view_rect;
    
    for_views(app, it) {
        if (it == view_id)  continue;
        
        Rect_f32 it_rect = view_get_screen_rect(app, it);
        if (y0 < it_rect.y0 || y0 > it_rect.y1)  continue;
        if (x0 > it_rect.x0)  continue;
        
        if (best_view_id == view_id ||
            it_rect.x0 < best_view_rect.x0) {
            best_view_id   = it;
            best_view_rect = it_rect;
        }
    }
    
    view_set_active(app, best_view_id);
}
CUSTOM_COMMAND_SIG(_vim_focus_view_down) {
    View_ID view_id = get_active_view(app, Access_Always);
    Rect_f32 view_rect = view_get_screen_rect(app, view_id);
    f32 x0 = view_rect.x0;
    f32 y0 = view_rect.y0;
    
    View_ID best_view_id = view_id;
    Rect_f32 best_view_rect = view_rect;
    
    for_views(app, it) {
        if (it == view_id)  continue;
        
        Rect_f32 it_rect = view_get_screen_rect(app, it);
        if (x0 < it_rect.x0 || x0 > it_rect.x1)  continue;
        if (y0 < it_rect.y0)  continue;
        
        if (best_view_id == view_id ||
            it_rect.y0 > best_view_rect.y0) {
            best_view_id   = it;
            best_view_rect = it_rect;
        }
    }
    
    view_set_active(app, best_view_id);
}
CUSTOM_COMMAND_SIG(_vim_focus_view_up) {
    View_ID view_id = get_active_view(app, Access_Always);
    Rect_f32 view_rect = view_get_screen_rect(app, view_id);
    f32 x0 = view_rect.x0;
    f32 y0 = view_rect.y0;
    
    View_ID best_view_id = view_id;
    Rect_f32 best_view_rect = view_rect;
    
    for_views(app, it) {
        if (it == view_id)  continue;
        
        Rect_f32 it_rect = view_get_screen_rect(app, it);
        if (x0 < it_rect.x0 || x0 > it_rect.x1)  continue;
        if (y0 > it_rect.y0)  continue;
        
        if (best_view_id == view_id ||
            it_rect.y0 < best_view_rect.y0) {
            best_view_id   = it;
            best_view_rect = it_rect;
        }
    }
    
    view_set_active(app, best_view_id);
}


#define vim_swap_buffers_between_two_views  vim_window_command<_vim_swap_buffers_between_two_views, true>

#define vim_cycle_view_focus    vim_window_command<_vim_cycle_view_focus, true>
#define vim_rotate_view_buffers vim_window_command<_vim_rotate_view_buffers, true>

#define vim_open_view_duplicate_split_vertical   vim_window_command<_vim_open_view_duplicate_split_vertical, true>
#define vim_open_view_duplicate_split_horizontal vim_window_command<_vim_open_view_duplicate_split_horizontal, true>

#define vim_open_view_split_horizontal vim_window_command<_vim_open_view_split_horizontal, true>
#define vim_open_view_split_vertical   vim_window_command<_vim_open_view_split_vertical, true>

#define vim_close_view  vim_window_command<_vim_close_view, true>

#define vim_focus_view_left  vim_window_command<_vim_focus_view_left, true>
#define vim_focus_view_right vim_window_command<_vim_focus_view_right, true>
#define vim_focus_view_down  vim_window_command<_vim_focus_view_down, true>
#define vim_focus_view_up    vim_window_command<_vim_focus_view_up, true>

//
// @note Face commands / Font commands
//

function void
vim_set_face_size(Application_Links *app, u32 pt_size) {
    View_ID view_id = get_active_view(app, Access_Always);
    Buffer_ID buffer_id = view_get_buffer(app, view_id, Access_Always);
    Face_ID face_id = get_face_id(app, buffer_id);
    Face_Description description = get_face_description(app, face_id);
    description.parameters.pt_size = pt_size;
    try_modify_face(app, face_id, &description);
}

CUSTOM_COMMAND_SIG(vim_reset_face_size) {
    vim_set_face_size(app, global_config.default_font_size);
}

CUSTOM_COMMAND_SIG(vim_set_small_face_size) {
    vim_set_face_size(app, 14);
}

CUSTOM_COMMAND_SIG(vim_set_big_face_size) {
    vim_set_face_size(app, 22);
}

//
// @note File commands
//

#define vim_exit_4coder  vim_window_command<exit_4coder>
#define vim_interactive_open_or_new  vim_window_command<interactive_open_or_new>
#define vim_open_matching_file_cpp  vim_window_command<open_matching_file_cpp>

inline void
vim_save_buffer(Application_Links *app, Scratch_Block *scratch, Buffer_ID buffer_id, String_Const_u8 postfix) {
    Temp_Memory temp = begin_temp(*scratch);
    String_Const_u8 file_name = push_buffer_file_name(app, *scratch, buffer_id);
    if (string_match(string_postfix(file_name, postfix.size), postfix)) {
        buffer_save(app, buffer_id, file_name, 0);
    }
    end_temp(temp);
}

CUSTOM_COMMAND_SIG(vim_save_buffer) {
    VIM_GET_VIEW_ID_AND_BUFFER_ID(app);
    vim_enter_mode_normal(app);
    
    String_Const_u8 postfix = {0};
    Scratch_Block scratch(app);
    
    vim_save_buffer(app, &scratch, buffer_id, postfix);
}

CUSTOM_COMMAND_SIG(vim_save_all_dirty_buffers) {
    vim_enter_mode_normal(app);
    
    ProfileScope(app, "clean and save all dirty buffers");
    String_Const_u8 postfix = {0};
    
    Scratch_Block scratch(app);
    for (Buffer_ID buffer_id = get_buffer_next(app, 0, Access_ReadWriteVisible);
         buffer_id != 0;
         buffer_id = get_buffer_next(app, buffer_id, Access_ReadWriteVisible)) {
        Dirty_State dirty = buffer_get_dirty_state(app, buffer_id);
        if (dirty == DirtyState_UnsavedChanges) {
            vim_save_buffer(app, &scratch, buffer_id, postfix);
        }
    }
}

//
// @note Buffer commands
//

#define vim_interactive_switch_buffer  vim_window_command<interactive_switch_buffer>
#define vim_interactive_kill_buffer    vim_window_command<interactive_kill_buffer>

CUSTOM_COMMAND_SIG(vim_kill_current_buffer) {
    vim_enter_mode_normal(app);
    
    VIM_GET_VIEW_ID_AND_BUFFER_ID(app);
    if (buffer_id == 0)  return;
    try_buffer_kill(app, buffer_id, view_id, 0);
}


//
// @note Project commands (build, debug, ...)
//

function View_ID
vim_open_build_view(Application_Links *app) {
    View_ID result = 0;
    
    Panel_ID root_panel_id = panel_get_root(app);
    
    Dimension split = Dimension_Y;
    Side side = Side_Max;
    
    if (panel_split(app, root_panel_id, split)) {
        Panel_ID new_panel_id = panel_get_child(app, root_panel_id, side);
        if (new_panel_id == 0)  return 0;
        
        result = panel_get_view(app, new_panel_id, Access_Always);
    }
    return result;
}
function View_ID
vim_open_build_footer_panel(Application_Links *app) {
    if (build_footer_panel_view_id != 0)  return build_footer_panel_view_id;
    View_ID view_id = get_active_view(app, Access_Always);
    {
        View_ID special_view = vim_open_build_view(app);
        new_view_settings(app, special_view);
        
        view_set_passive(app, special_view, true);
        view_set_setting(app, special_view, ViewSetting_ShowFileBar, false);
        
        build_footer_panel_view_id = special_view;
    }
    view_set_active(app, view_id);
    return build_footer_panel_view_id;
}

global b32 vim_is_build_panel_hidden = false;
global b32 vim_is_build_panel_expanded = false;

static void
vim_minimize_build_panel(Application_Links *app, View_ID build_view_id, b32 center_view = true) {
#if 0
    view_set_split_proportion(app, build_view_id, 0.1f);
#else
    f32 line_count = 4.f;
    
    Buffer_ID buffer_id = view_get_buffer(app, build_view_id, Access_Always);
    Face_ID face_id = get_face_id(app, buffer_id);
    Face_Metrics metrics = get_face_metrics(app, face_id);
    view_set_split_pixel_size(app, build_view_id, (i32)(metrics.line_height * line_count));
#endif
    
    if (center_view) vim_center_all_views(app);
    
    vim_is_build_panel_expanded = false;
    vim_is_build_panel_hidden = false;
}
static void
vim_maximize_build_panel(Application_Links *app, View_ID build_view_id, b32 center_view = true) {
    view_set_split_proportion(app, build_view_id, 0.8f);
    
    if (center_view)  vim_center_all_views(app);
    
    vim_is_build_panel_expanded = true;
    vim_is_build_panel_hidden = false;
}

static View_ID
vim_get_or_open_build_panel(Application_Links *app) {
    View_ID build_view_id = 0;
    Buffer_ID build_buffer = get_comp_buffer(app);
    if (build_buffer != 0){
        build_view_id = get_first_view_with_buffer(app, build_buffer);
    }
    if (build_view_id == 0){
        build_view_id = vim_open_build_footer_panel(app);
        vim_minimize_build_panel(app, build_view_id);
    }
    return build_view_id;
}

CUSTOM_COMMAND_SIG(vim_toggle_build_panel_height) {
    View_ID build_view_id = vim_get_or_open_build_panel(app);
    
    if (!vim_is_build_panel_expanded) {
        vim_maximize_build_panel(app, build_view_id);
    }
    else {
        vim_minimize_build_panel(app, build_view_id);
    }
}


CUSTOM_COMMAND_SIG(vim_build_in_build_panel) {
    View_ID view_id = get_active_view(app, Access_Always);
    Buffer_ID buffer_id = view_get_buffer(app, view_id, Access_Always);
    
    View_ID build_view_id = vim_get_or_open_build_panel(app);
#if 0
    if (vim_is_build_panel_expanded) {
        vim_maximize_build_panel(app, build_view_id, false);
    }
    else {
        vim_minimize_build_panel(app, build_view_id, false);
    }
#else
    vim_minimize_build_panel(app, build_view_id, false);
    // vim_maximize_build_panel(app, build_view_id, false);
#endif
    
    standard_search_and_build(app, build_view_id, buffer_id);
    set_fancy_compilation_buffer_font(app);
    
    block_zero_struct(&prev_location);
    lock_jump_buffer(app, string_u8_litexpr("*compilation*"));
}
#if 0
CUSTOM_COMMAND_SIG(vim_close_build_panel) {
    close_build_panel(app);
    vim_is_build_panel_expanded = false;
}
#endif
CUSTOM_COMMAND_SIG(vim_toggle_hide_build_panel) {
    View_ID build_view_id = vim_get_or_open_build_panel(app);
    if (!vim_is_build_panel_hidden) {
        view_set_split_proportion(app, build_view_id, 0.0f);
        // vim_is_build_panel_expanded = false;
        vim_center_all_views(app);
        
        vim_is_build_panel_hidden = true;
    } 
    else {
        if (vim_is_build_panel_expanded) {
            vim_maximize_build_panel(app, build_view_id, false);
        }
        else {
            vim_minimize_build_panel(app, build_view_id, false);
        }
    }
}
CUSTOM_COMMAND_SIG(vim_change_to_build_panel) {
    View_ID build_view_id = vim_get_or_open_build_panel(app);
    if (build_view_id == 0)  return;
    
    view_set_active(app, build_view_id);
}


CUSTOM_COMMAND_SIG(vim_clean_save_all_dirty_buffers_and_build) {
    vim_save_all_dirty_buffers(app);
    vim_build_in_build_panel(app);
}

CUSTOM_COMMAND_SIG(vim_save_all_dirty_buffers_and_build) {
    save_all_dirty_buffers(app);
    vim_build_in_build_panel(app);
}

//
// @note Vim statusbar
//

#define VIM_COMMAND_SIG(name)  void name(struct Application_Links *app, \
const String_Const_u8 command, \
const String_Const_u8 arg_str, \
b32 force)
typedef VIM_COMMAND_SIG(Vim_Command_Function);
typedef CUSTOM_COMMAND_SIG(Vim_Custom_Command_Function);

struct Vim_Command_Node {
    Vim_Command_Node *next;
    
    String_Const_u8 command;
    
    b32 is_vim_command;
    union {
        Vim_Command_Function        *vim_command_function;
        Vim_Custom_Command_Function *custom_command_function;
    };
};

struct Vim_Command_List {
    Vim_Command_Node *first;
    Vim_Command_Node *last;
    i32 count;
};

global Arena global_vim_command_arena = {};
global Vim_Command_List global_vim_command_list = {};

inline Vim_Command_Node *
vim_make_command() {
    if (global_vim_command_arena.base_allocator == 0) {
        global_vim_command_arena = make_arena_system();
    }
    
    Vim_Command_Node *node = push_array(&global_vim_command_arena, Vim_Command_Node, 1);
    sll_queue_push(global_vim_command_list.first, global_vim_command_list.last, node);
    ++global_vim_command_list.count;
    
    return node;
}
function void
_vim_define_command(String_Const_u8 command_string, Vim_Command_Function func) {
    Vim_Command_Node *command = vim_make_command();
    command->command = command_string;
    command->is_vim_command = true;
    command->vim_command_function = func;
}
function void
_vim_define_command(String_Const_u8 command_string, Vim_Custom_Command_Function func) {
    Vim_Command_Node *command = vim_make_command();
    command->command = command_string;
    command->is_vim_command = false;
    command->custom_command_function = func;
}

#define vim_define_command(command, func)  _vim_define_command(SCu8(command), func)


CUSTOM_COMMAND_SIG(vim_status_command) {
    vim_enter_mode_normal(app);
    
    
    Query_Bar_Group group(app);
    Query_Bar bar = {};
    if (start_query_bar(app, &bar, 0) == 0) {
        return;
    }
    
    u8 bar_string_space[256];
    bar.string = SCu8(bar_string_space, (u64)0);
    bar.prompt = string_u8_litexpr(":");
    
    User_Input input = {};
    for (;;) {
        input = get_next_input(app, EventPropertyGroup_AnyKeyboardEvent, EventProperty_Escape | EventProperty_ViewActivation);
        if (input.abort)  return;
        
        String_Const_u8 input_string = to_writable(&input);
        
        if (match_key_code(&input, KeyCode_Return)) {
            break;
        }
        else if (match_key_code(&input, KeyCode_Tab)) {
            // @todo auto complete
        }
        else if (input_string.str != 0 && input_string.size > 0) {
            String_u8 bar_string = Su8(bar.string, sizeof(bar_string_space));
            string_append(&bar_string, input_string);
            bar.string = bar_string.string;
        }
        else if (match_key_code(&input, KeyCode_Backspace)) {
            if (is_unmodified_key(&input.event)) {
                bar.string = backspace_utf8(bar.string);
            }
            else if (has_modifier(&input.event.key.modifiers, KeyCode_Control)) {
                if (bar.string.size > 0) {
                    bar.string.size = 0;
                }
            }
        }
        else {
            leave_current_input_unhandled(app);
        }
    }
    
    
    u64 command_offset = 0;
    while (command_offset < bar.string.size && character_is_whitespace(bar.string.str[command_offset])) {
        ++command_offset;
    }
    u64 command_end = command_offset;
    while (command_end < bar.string.size && !character_is_whitespace(bar.string.str[command_end])) {
        ++command_end;
    }
    if (command_offset == command_end)  return;
    
    String_Const_u8 command = string_substring(bar.string, Ii64(command_offset, command_end));
    
    b32 force_command = false;
    if (command.str[command.size - 1] == '!') {
        --command.size;
        force_command = true;
    }
    
    // @note goto line
    if (string_is_integer(bar.string, 10)) {
        u64 line_number = string_to_integer(bar.string, 10);
        View_ID view_id = get_active_view(app, Access_ReadVisible);
        view_set_cursor_and_preferred_x(app, view_id, seek_line_col(line_number, 0));
        return;
    }
    
    // @note find and call command
    for (Vim_Command_Node *node = global_vim_command_list.first; node != 0; node = node->next) {
        if (string_match(node->command, command)) {
            if (node->is_vim_command) {
                // @note command arguments
                u64 args_start = command_end;
                while (args_start < bar.string.size && character_is_whitespace(bar.string.str[args_start])) {
                    ++args_start;
                }
                String_Const_u8 args_string = string_substring(bar.string, Ii64(args_start, (bar.string.size - args_start)));
                
                node->vim_command_function(app, command, args_string, force_command);
            }
            else {
                node->custom_command_function(app);
            }
        }
    }
}


//
// @note Statusbar commands
//

VIM_COMMAND_SIG(vim_exec_regex) {
    // @todo
}
VIM_COMMAND_SIG(vim_move_lines_selection) {
    
}

//
// @note :vim_default_function_overrides
//
// @note You need to #if 0 out the default 4coder implementations to use the ones defined below.
//       Otherwise you will get an error during build that the functions already exist.
//
// For example:
/*
function Lister_Result run_lister(Application_Links *app, Lister *lister);
#define VIM_USE_DEFAULT_RUN_LISTER 0
#if VIM_USE_DEFAULT_RUN_LISTER
function Lister_Result
run_lister(Application_Links *app, Lister *lister) {
...
}
#endif // VIM_USE_DEFAULT_RUN_LISTER
*/

#if !defined(VIM_USE_DEFAULT_RUN_LISTER)
#error "You need to #if 0 out the default 4coder run_lister function in 4coder_lister_base.cpp to use the vim custom one. And define VIM_USE_DEFAULT_RUN_LISTER 0."
#endif
#if !VIM_USE_DEFAULT_RUN_LISTER
function Lister_Result
run_lister(Application_Links *app, Lister *lister) {
    lister->filter_restore_point = begin_temp(lister->arena);
    lister_update_filtered_list(app, lister);
    
    View_ID view = get_this_ctx_view(app, Access_Always);
    View_Context ctx = view_current_context(app, view);
    ctx.render_caller = lister_render;
    ctx.hides_buffer = true;
    View_Context_Block ctx_block(app, view, &ctx);
    
    for (;;) {
        User_Input in = get_next_input(app, EventPropertyGroup_Any, EventProperty_Escape);
        if (has_modifier(&in.event.key.modifiers, KeyCode_Control)) {
            if (in.event.key.code == KeyCode_Space) {
                in.abort = true;
            }
        }
        if (in.abort) {
            block_zero_struct(&lister->out);
            lister->out.canceled = true;
            break;
        }
        
        Lister_Activation_Code result = ListerActivation_Continue;
        b32 handled = true;
        switch (in.event.kind) {
            case InputEventKind_TextInsert:
            {
                if (lister->handlers.write_character != 0) {
                    result = lister->handlers.write_character(app);
                }
            }break;
            
            case InputEventKind_KeyStroke:
            {
                if (has_modifier(&in.event.key.modifiers, KeyCode_Alt)) {
                    if (in.event.key.code == KeyCode_K) {
                        in.event.key.code = KeyCode_Up;
                    }
                    else if (in.event.key.code == KeyCode_J) {
                        in.event.key.code = KeyCode_Down;
                    }
                }
                
                switch (in.event.key.code) {
                    case KeyCode_Return:
                    case KeyCode_Tab:
                    {
                        void *user_data = 0;
                        if (0 <= lister->raw_item_index &&
                            lister->raw_item_index < lister->options.count) {
                            user_data = lister_get_user_data(lister, lister->raw_item_index);
                        }
                        lister_activate(app, lister, user_data, false);
                        result = ListerActivation_Finished;
                    }break;
                    
                    case KeyCode_Backspace:
                    {
                        if (lister->handlers.backspace != 0) {
                            lister->handlers.backspace(app);
                        }
                        else if (lister->handlers.key_stroke != 0) {
                            result = lister->handlers.key_stroke(app);
                        }
                        else{
                            handled = false;
                        }
                    }break;
                    
                    case KeyCode_Up:
                    {
                        if (lister->handlers.navigate != 0) {
                            lister->handlers.navigate(app, view, lister, -1);
                        }
                        else if (lister->handlers.key_stroke != 0) {
                            result = lister->handlers.key_stroke(app);
                        }
                        else{
                            handled = false;
                        }
                    }break;
                    
                    case KeyCode_Down:
                    {
                        if (lister->handlers.navigate != 0) {
                            lister->handlers.navigate(app, view, lister, 1);
                        }
                        else if (lister->handlers.key_stroke != 0) {
                            result = lister->handlers.key_stroke(app);
                        }
                        else{
                            handled = false;
                        }
                    }break;
                    
                    case KeyCode_PageUp:
                    {
                        if (lister->handlers.navigate != 0) {
                            lister->handlers.navigate(app, view, lister,
                                                      -lister->visible_count);
                        }
                        else if (lister->handlers.key_stroke != 0) {
                            result = lister->handlers.key_stroke(app);
                        }
                        else{
                            handled = false;
                        }
                    }break;
                    
                    case KeyCode_PageDown:
                    {
                        if (lister->handlers.navigate != 0) {
                            lister->handlers.navigate(app, view, lister,
                                                      lister->visible_count);
                        }
                        else if (lister->handlers.key_stroke != 0) {
                            result = lister->handlers.key_stroke(app);
                        }
                        else{
                            handled = false;
                        }
                    }break;
                    
                    default:
                    {
                        if (lister->handlers.key_stroke != 0) {
                            result = lister->handlers.key_stroke(app);
                        }
                        else{
                            handled = false;
                        }
                    }break;
                }
            }break;
            
            case InputEventKind_MouseButton:
            {
                switch (in.event.mouse.code) {
                    case MouseCode_Left:
                    {
                        Vec2_f32 p = V2f32(in.event.mouse.p);
                        void *clicked = lister_user_data_at_p(app, view, lister, p);
                        lister->hot_user_data = clicked;
                    }break;
                    
                    default:
                    {
                        handled = false;
                    }break;
                }
            }break;
            
            case InputEventKind_MouseButtonRelease:
            {
                switch (in.event.mouse.code) {
                    case MouseCode_Left:
                    {
                        if (lister->hot_user_data != 0) {
                            Vec2_f32 p = V2f32(in.event.mouse.p);
                            void *clicked = lister_user_data_at_p(app, view, lister, p);
                            if (lister->hot_user_data == clicked) {
                                lister_activate(app, lister, clicked, true);
                                result = ListerActivation_Finished;
                            }
                        }
                        lister->hot_user_data = 0;
                    }break;
                    
                    default:
                    {
                        handled = false;
                    }break;
                }
            }break;
            
            case InputEventKind_MouseWheel:
            {
                Mouse_State mouse = get_mouse_state(app);
                lister->scroll.target.y += mouse.wheel;
                lister_update_filtered_list(app, lister);
            }break;
            
            case InputEventKind_MouseMove:
            {
                lister_update_filtered_list(app, lister);
            }break;
            
            case InputEventKind_Core:
            {
                switch (in.event.core.code) {
                    case CoreCode_Animate:
                    {
                        lister_update_filtered_list(app, lister);
                    }break;
                    
                    default:
                    {
                        handled = false;
                    }break;
                }
            }break;
            
            default:
            {
                handled = false;
            }break;
        }
        
        if (result == ListerActivation_Finished) {
            break;
        }
        
        if (!handled) {
            Mapping *mapping = lister->mapping;
            Command_Map *map = lister->map;
            
            Fallback_Dispatch_Result disp_result =
                fallback_command_dispatch(app, mapping, map, &in);
            if (disp_result.code == FallbackDispatch_DelayedUICall) {
                call_after_ctx_shutdown(app, view, disp_result.func);
                break;
            }
            if (disp_result.code == FallbackDispatch_Unhandled) {
                leave_current_input_unhandled(app);
            }
            else{
                lister_call_refresh_handler(app, lister);
            }
        }
    }
    
    return(lister->out);
}
#endif // !VIM_USE_DEFAULT_RUN_LISTER

#if !defined(VIM_USE_DEFAULT_ISEARCH)
#error "You need to #if 0 out the default 4coder isearch function in 4coder_base_commands.cpp to use the vim custom one. And define VIM_USE_DEFAULT_ISEARCH 0."
#endif
#if !VIM_USE_DEFAULT_ISEARCH
#define ISEARCH_RING_BUFFER 1
function void
vim_isearch(Application_Links *app, Scan_Direction start_scan, i64 first_pos, String_Const_u8 query_init, b32 union_visual_selection_range) {
    View_ID view = get_active_view(app, Access_ReadVisible);
    Buffer_ID buffer = view_get_buffer(app, view, Access_ReadVisible);
    if (!buffer_exists(app, buffer)) {
        return;
    }
    Managed_Scope view_scope = view_get_managed_scope((app), view);
    Vim_View_State *vim_state = scope_attachment((app), view_scope, view_vim_state_id, Vim_View_State);
    
    i64 buffer_size = buffer_get_size(app, buffer);
    
    Query_Bar_Group group(app);
    Query_Bar bar = {};
    if (start_query_bar(app, &bar, 0) == 0) {
        return;
    }
    
    i64 first_mark_pos = view_get_mark_pos(app, view);
    Range_i64 first_selection_range = vim_state->selection_range;
    Scan_Direction scan = start_scan;
    i64 pos = first_pos;
    
    u8 bar_string_space[256];
    bar.string = SCu8(bar_string_space, query_init.size);
    block_copy(bar.string.str, query_init.str, query_init.size);
    
    String_Const_u8 isearch_str = string_u8_litexpr("I-Search: ");
    String_Const_u8 rsearch_str = string_u8_litexpr("Reverse-I-Search: ");
    
    u64 match_size = bar.string.size;
    
    User_Input in = {};
    for (;;) {
        switch (scan) {
            case Scan_Forward:
            {
                bar.prompt = isearch_str;
            }break;
            case Scan_Backward:
            {
                bar.prompt = rsearch_str;
            }break;
        }
        isearch__update_highlight(app, view, Ii64_size(pos, match_size));
        // :cursor_mark
        if (vim_state->mode == vim_mode_visual || vim_state->mode == vim_mode_visual_line) {
            if (union_visual_selection_range) {
                Range_i64 new_range = range_union(Ii64_size(pos, match_size), first_selection_range);
                view_set_mark(app, view, seek_pos(new_range.start));
                view_set_cursor_and_preferred_x(app, view, seek_pos(new_range.one_past_last - 1));
                vim_update_visual_range(app, view);
            }
        }
        vim_scroll_cursor_line(app, 0);
        
        in = get_next_input(app, EventPropertyGroup_AnyKeyboardEvent, EventProperty_Escape|EventProperty_ViewActivation);
        if (in.abort) {
            break;
        }
        Input_Modifier_Set *mods = &in.event.key.modifiers;
        
        String_Const_u8 string = to_writable(&in);
        
        b32 string_change = false;
        if (match_key_code(&in, KeyCode_Return) ||
            match_key_code(&in, KeyCode_Tab)) {
            if (has_modifier(mods, KeyCode_Control)) {
                bar.string.size = cstring_length(previous_isearch_query);
                block_copy(bar.string.str, previous_isearch_query, bar.string.size);
            }
            else{
                u64 size = bar.string.size;
                size = clamp_top(size, sizeof(previous_isearch_query) - 1);
                block_copy(previous_isearch_query, bar.string.str, size);
                previous_isearch_query[size] = 0;
                break;
            }
        }
        else if (string.str != 0 && string.size > 0) {
            String_u8 bar_string = Su8(bar.string, sizeof(bar_string_space));
            string_append(&bar_string, string);
            bar.string = bar_string.string;
            string_change = true;
        }
        else if (match_key_code(&in, KeyCode_Backspace)) {
            if (is_unmodified_key(&in.event)) {
                u64 old_bar_string_size = bar.string.size;
                bar.string = backspace_utf8(bar.string);
                string_change = (bar.string.size < old_bar_string_size);
            }
            else if (has_modifier(&in.event.key.modifiers, KeyCode_Control)) {
                if (bar.string.size > 0) {
                    string_change = true;
                    bar.string.size = 0;
                }
            }
        }
        
        // TODO(allen): how to detect if the input corresponds to
        // a search or rsearch command, a scroll wheel command?
        
        b32 do_scan_action = false;
        b32 do_scroll_wheel = false;
        Scan_Direction change_scan = scan;
        if (!string_change) {
            if (match_key_code(&in, KeyCode_PageDown) ||
                match_key_code(&in, KeyCode_Down)     ||
                (match_key_code(&in, KeyCode_S) && has_modifier(mods, KeyCode_Control)) ||
                (match_key_code(&in, KeyCode_J) && has_modifier(mods, KeyCode_Alt))) {
                change_scan = Scan_Forward;
                do_scan_action = true;
            }
            if (match_key_code(&in, KeyCode_PageUp) ||
                match_key_code(&in, KeyCode_Up) ||
                (match_key_code(&in, KeyCode_R) && has_modifier(mods, KeyCode_Control)) ||
                (match_key_code(&in, KeyCode_K) && has_modifier(mods, KeyCode_Alt))) {
                change_scan = Scan_Backward;
                do_scan_action = true;
            }
            
#if 0
            if (in.command == mouse_wheel_scroll) {
                do_scroll_wheel = true;
            }
#endif
        }
        
        if (string_change) {
            switch (scan) {
                case Scan_Forward:
                {
                    i64 new_pos = 0;
                    seek_string_insensitive_forward(app, buffer, pos - 1, 0, bar.string, &new_pos);
#if ISEARCH_RING_BUFFER
                    if (new_pos >= buffer_size) {
                        pos = 0;
                        seek_string_insensitive_forward(app, buffer, pos - 1, 0, bar.string, &new_pos);
                    }
#endif
                    if (new_pos < buffer_size) {
                        pos = new_pos;
                        match_size = bar.string.size;
                    }
                }break;
                
                case Scan_Backward:
                {
                    i64 new_pos = 0;
                    seek_string_insensitive_backward(app, buffer, pos + 1, 0, bar.string, &new_pos);
#if ISEARCH_RING_BUFFER
                    if (new_pos <= 0) {
                        pos = buffer_size;
                        seek_string_insensitive_backward(app, buffer, pos + 1, 0, bar.string, &new_pos);
                    }
#endif
                    if (new_pos >= 0) {
                        pos = new_pos;
                        match_size = bar.string.size;
                    }
                }break;
            }
        }
        else if (do_scan_action) {
            scan = change_scan;
            switch (scan) {
                case Scan_Forward:
                {
                    i64 new_pos = 0;
                    seek_string_insensitive_forward(app, buffer, pos, 0, bar.string, &new_pos);
#if ISEARCH_RING_BUFFER
                    if (new_pos >= buffer_size) {
                        pos = 0;
                        seek_string_insensitive_forward(app, buffer, pos, 0, bar.string, &new_pos);
                    }
#endif
                    if (new_pos < buffer_size) {
                        pos = new_pos;
                        match_size = bar.string.size;
                    }
                }break;
                
                case Scan_Backward:
                {
                    i64 new_pos = 0;
                    seek_string_insensitive_backward(app, buffer, pos, 0, bar.string, &new_pos);
#if ISEARCH_RING_BUFFER
                    if (new_pos <= 0) {
                        pos = buffer_size;
                        seek_string_insensitive_backward(app, buffer, pos, 0, bar.string, &new_pos);
                    }
#endif
                    if (new_pos >= 0) {
                        pos = new_pos;
                        match_size = bar.string.size;
                    }
                }break;
            }
        }
        else if (do_scroll_wheel) {
            mouse_wheel_scroll(app);
        }
        else{
            leave_current_input_unhandled(app);
        }
    }
    
    view_disable_highlight_range(app, view);
    
    if (in.abort) {
        u64 size = bar.string.size;
        size = clamp_top(size, sizeof(previous_isearch_query) - 1);
        block_copy(previous_isearch_query, bar.string.str, size);
        previous_isearch_query[size] = 0;
        
        // :cursor_mark
        view_set_cursor_and_preferred_x(app, view, seek_pos(first_pos));
        view_set_mark(app, view, seek_pos(first_mark_pos));
        vim_update_visual_range(app, view);
    }
    else {
        // :cursor_mark
        if (vim_state->mode == vim_mode_visual || vim_state->mode == vim_mode_visual_line) {
            if (union_visual_selection_range) {
                Range_i64 new_range = range_union(Ii64_size(pos, match_size), vim_state->selection_range);
                view_set_mark(app, view, seek_pos(new_range.start));
                view_set_cursor_and_preferred_x(app, view, seek_pos(new_range.one_past_last - 1));
                vim_update_visual_range(app, view);
            }
            else {
                view_set_cursor_and_preferred_x(app, view, seek_pos(pos));
                view_set_mark(app, view, seek_pos(pos + match_size - 1));
                vim_update_visual_range(app, view);
                
                vim_enter_mode_normal(app);
            }
        }
        else {
            view_set_cursor_and_preferred_x(app, view, seek_pos(pos));
            view_set_mark(app, view, seek_pos(pos + match_size - 1));
        }
    }
    
}
function void
isearch(Application_Links *app, Scan_Direction start_scan, i64 first_pos, String_Const_u8 query_init) {
    vim_isearch(app, start_scan, first_pos, query_init);
}
#endif // !VIM_USE_DEFAULT_ISEARCH

#if !defined(VIM_USE_DEFAULT_QUERY_REPLACE_BASE)
#error "You need to #if 0 out the default 4coder query_replace_base function in 4coder_base_commands.cpp to use the vim custom one. And define VIM_USE_DEFAULT_QUERY_REPLACE_BASE 0."
#endif
#if !VIM_USE_DEFAULT_QUERY_REPLACE_BASE

function void
query_replace_base(Application_Links *app, View_ID view, Buffer_ID buffer_id, i64 pos, String_Const_u8 r, String_Const_u8 w) {
    i64 new_pos = 0;
    seek_string_forward(app, buffer_id, pos - 1, 0, r, &new_pos);
    
    i64 buffer_size = buffer_get_size(app, buffer_id);
    
    User_Input in = {};
    for (;new_pos < buffer_size;) {
        Range_i64 match = Ii64(new_pos, new_pos + r.size);
        isearch__update_highlight(app, view, match);
        vim_scroll_cursor_line(app, 0);
        
        in = get_next_input(app, EventProperty_AnyKey, EventProperty_MouseButton);
        if (in.abort || match_key_code(&in, KeyCode_Escape) || !is_unmodified_key(&in.event)) {
            break;
        }
        
        if (match_key_code(&in, KeyCode_Y) ||
            match_key_code(&in, KeyCode_Return) ||
            match_key_code(&in, KeyCode_Tab)) {
            buffer_replace_range(app, buffer_id, match, w);
            pos = match.start + w.size;
        }
        else {
            pos = match.max;
        }
        
        seek_string_forward(app, buffer_id, pos, 0, r, &new_pos);
    }
    
    view_disable_highlight_range(app, view);
    
    if (in.abort) {
        return;
    }
    
    view_set_cursor_and_preferred_x(app, view, seek_pos(pos));
}
#endif // !VIM_USE_DEFAULT_QUERY_REPLACE_BASE


#if !defined(VIM_USE_DEFAULT_LISTER__WRITE_CHARACTER__FILE_PATH)
#error "You need to #if 0 out the default 4coder lister__write_character__file_path function in 4coder_lists.cpp to use the vim custom one. And define VIM_USE_DEFAULT_LISTER__WRITE_CHARACTER__FILE_PATH 0."
#endif
#if !VIM_USE_DEFAULT_LISTER__WRITE_CHARACTER__FILE_PATH
function Lister_Activation_Code
lister__write_character__file_path(Application_Links *app){
    Lister_Activation_Code result = ListerActivation_Continue;
    View_ID view = get_this_ctx_view(app, Access_Always);
    Lister *lister = view_get_lister(view);
    if (lister != 0){
        User_Input in = get_current_input(app);
        String_Const_u8 string = to_writable(&in);
        if (string.str != 0 && string.size > 0){
            lister_append_text_field(lister, string);
            if (character_is_slash(string.str[0])) {
                if (string_find_first(string, 0, ':') != 0) {
                    // @note Change drive
                    String_u8 new_hot = lister->text_field;
                    set_hot_directory(app, new_hot.string);
                    // @todo: refresh_handler resets the text_field.
                    lister_call_refresh_handler(app, lister);
                    lister->text_field = new_hot;
                }
                else {
                    // @note Ask to create folder
                    lister->out.text_field = lister->text_field.string;
                    result = ListerActivation_Finished;
                }
            }
            else{
                String_Const_u8 front_name = string_front_of_path(lister->text_field.string);
                lister_set_key(lister, front_name);
            }
            lister->item_index = 0;
            lister_zero_scroll(lister);
            lister_update_filtered_list(app, lister);
        }
    }
    return(result);
}
#endif // !VIM_USE_DEFAULT_LISTER__WRITE_CHARACTER__FILE_PATH