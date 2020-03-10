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


//
// @note: Search highlight
//
global b32 global_search_highlight_case_sensitive = false;


#define VIM
CUSTOM_COMMAND_SIG(vim_enter_mode_normal);


#define VIM_NTIMES_CUSTOM_COMMAND_SIG(name) void name(struct Application_Links *app, View_ID view_id, Buffer_ID buffer_id, Managed_Scope view_scope, Managed_Scope buffer_scope, Vim_View_State *vim_state)

template <VIM_NTIMES_CUSTOM_COMMAND_SIG(command), b32 one_past_last = false, b32 move_right_if_mode_insert = false>
CUSTOM_COMMAND_SIG(vim_command_execute_ntimes);


#define VIM_H
#endif
