#if !defined(VIM_H)


//
// @note attachments
//
CUSTOM_ID(attachment, view_map_id);
CUSTOM_ID(attachment, view_vim_state_id);

CUSTOM_ID(attachment, buffer_file_is_c);
CUSTOM_ID(attachment, buffer_file_is_c_like);

//
// @note Vim map ids
//
CUSTOM_ID(command_map, mapid_vim_unbound);
CUSTOM_ID(command_map, mapid_vim_escape_to_normal_mode);

CUSTOM_ID(command_map, mapid_vim_movements);
CUSTOM_ID(command_map, mapid_vim_mode_normal);
CUSTOM_ID(command_map, mapid_vim_mode_insert);
CUSTOM_ID(command_map, mapid_vim_mode_visual);
CUSTOM_ID(command_map, mapid_vim_mode_replace);

// There are a bunch of different chord "starters" that result in keys having
// different behaviors.
CUSTOM_ID(command_map, mapid_vim_chord_replace_single);
CUSTOM_ID(command_map, mapid_vim_chord_yank);
CUSTOM_ID(command_map, mapid_vim_chord_delete);
CUSTOM_ID(command_map, mapid_vim_chord_change);

CUSTOM_ID(command_map, mapid_vim_chord_format);
CUSTOM_ID(command_map, mapid_vim_chord_indent_left);
CUSTOM_ID(command_map, mapid_vim_chord_indent_right);

CUSTOM_ID(command_map, mapid_vim_chord_move_right_to_found);
CUSTOM_ID(command_map, mapid_vim_chord_move_right_before_found);
CUSTOM_ID(command_map, mapid_vim_chord_move_left_to_found);
CUSTOM_ID(command_map, mapid_vim_chord_move_left_before_found);

CUSTOM_ID(command_map, mapid_vim_chord_mark);
CUSTOM_ID(command_map, mapid_vim_chord_window);
CUSTOM_ID(command_map, mapid_vim_chord_choose_register);

CUSTOM_ID(command_map, mapid_vim_chord_g);
CUSTOM_ID(command_map, mapid_vim_chord_z);


//
// @note vim mode id
//
enum Vim_Mode {
    vim_mode_normal = 1,
    vim_mode_insert,
    vim_mode_replace,
    vim_mode_visual,
    vim_mode_visual_line,
};

enum Vim_Action {
    vim_action_none = 1,
    
    vim_action_delete_range,
    vim_action_change_range,
    vim_action_yank_range,
    vim_action_format_range,
    vim_action_indent_left_range,
    vim_action_indent_right_range,
};

struct Vim_View_State {
    // b32 is_initialized = false;
    int execute_command_count = 1; // :vim_view_state_is_initialized
    int predecimal_count = 0;
    
    // @note The *current* vim mode. If a chord or action is pending, this will dictate
    //       what mode you return to once the action is completed.
    Vim_Mode mode = vim_mode_normal;
    // @note A pending action. Used to keep track of intended edits while in the middle
    //       of chords.
    Vim_Action pending_action;
    
    // @note
    Range_i64 selection_range;
    
    Color_Table *color_table_ptr;
};

struct Vim_Global_State {
    union {
        struct {
            Color_Table default;
            
            // @todo Generate at compiletime for all modes/chords
            Color_Table mode_normal;
            Color_Table mode_insert;
            Color_Table mode_visual;
            Color_Table mode_replace;
            
            Color_Table chord_replace_single;
            Color_Table chord_yank;
            Color_Table chord_delete;
            Color_Table chord_change;
        } color_tables;
        Color_Table color_tables_array[9];
    };
    
    // @todo registers, marks, ...
};

global Vim_Global_State vim_global_state = {};


// :view_changed_flash_line
global f32 vim_global_view_changed_time = 0.0f;

// :suppress_mouse
global f32 vim_global_mouse_last_event_time = 0.0f;
global b32 vim_global_enable_mouse_suppression = true;


// :build_panel
global b32 vim_is_build_panel_hidden = false;
global b32 vim_is_build_panel_expanded = false;


// @note: Search highlight
global b32 global_search_highlight_case_sensitive = false;



//
// @note: Vim helpers
//
#define VIM

#define VIM_GET_VIEW_ID_VIEW_SCOPE_BUFFER_ID_AND_VIM_STATE(app) \
View_ID view_id = get_active_view((app), Access_Always); \
Buffer_ID buffer_id = view_get_buffer((app), view_id, Access_ReadVisible); \
Managed_Scope view_scope = view_get_managed_scope((app), view_id); \
Vim_View_State *vim_state = scope_attachment((app), view_scope, view_vim_state_id, Vim_View_State);

#define VIM_GET_VIEW_ID_SCOPE_AND_VIM_STATE(app) \
View_ID view_id = get_active_view((app), Access_Always); \
Managed_Scope view_scope = view_get_managed_scope((app), view_id); \
Vim_View_State *vim_state = scope_attachment((app), view_scope, view_vim_state_id, Vim_View_State);

#define VIM_GET_VIEW_ID_AND_BUFFER_ID(app) \
View_ID view_id = get_active_view((app), Access_Always); \
Buffer_ID buffer_id = view_get_buffer((app), view_id, Access_ReadVisible);

// @note View for macro
#define for_views(app, it) \
for (View_ID it = get_view_next((app), 0, Access_Always); \
it != 0;                                           \
it = get_view_next((app), it, Access_Always))


CUSTOM_COMMAND_SIG(vim_enter_mode_normal);


//
// @note: Move command helpers
//

#define VIM_NTIMES_CUSTOM_COMMAND_SIG(name) void name(struct Application_Links *app, View_ID view_id, Buffer_ID buffer_id, Managed_Scope view_scope, Managed_Scope buffer_scope, Vim_View_State *vim_state)

// @note execute command n times
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

#define VIM_MOVE_COMMAND(new_name, command_name, ...) \
inline VIM_NTIMES_CUSTOM_COMMAND_SIG(_##new_name) { \
for (int i = 0; i < vim_state->execute_command_count; ++i) { \
command_name(app); \
} \
} \
CUSTOM_COMMAND_SIG(new_name) { \
vim_command_execute_ntimes<_##new_name, __VA_ARGS__>(app); \
}

#define VIM_MOVE_COMMAND_EXECUTE_ONCE(new_name, command_name, ...) \
inline VIM_NTIMES_CUSTOM_COMMAND_SIG(_##new_name) { \
command_name(app); \
} \
CUSTOM_COMMAND_SIG(new_name) { \
vim_command_execute_ntimes<_##new_name, __VA_ARGS__>(app); \
}

#define VIM_MOVE_COMMAND_EXECUTE_NTIMES(new_vim_name, vim_command_name, ...) \
CUSTOM_COMMAND_SIG(new_vim_name) { \
vim_command_execute_ntimes<vim_command_name, __VA_ARGS__>(app); \
}


//
// @note: View command helper
//
template <CUSTOM_COMMAND_SIG(command), b32 active_view_changed_highlight = false>
CUSTOM_COMMAND_SIG(vim_window_command) {
    vim_reset_mode_mapid(app);
    command(app);
    
    // @note Vim :view_changed_flash_line
    if (active_view_changed_highlight) {
        vim_global_view_changed_time = 1.0f;
    }
}

#define VIM_VIEW_COMMAND(new_name, command_name, ...) \
CUSTOM_COMMAND_SIG(new_name) { \
vim_window_command<command_name, __VA_ARGS__>(app); \
}


// @todo: Can we combine the view and chard ones.
#if 0
#define VIM_CHORD_COMMAND(new_name, command_name) \
CUSTOM_COMMAND_SIG(new_name) { \
vim_reset_mode_mapid(app); \
command_name(app); \
}
#else

#define VIM_CHORD_COMMAND(new_name, command_name)  VIM_VIEW_COMMAND(new_name, command_name)

#endif



#define VIM_H
#endif
