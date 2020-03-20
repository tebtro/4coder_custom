#define USE_MULTIPLE_CURSORS 1

// @todo Make it per view

global i64 global_multiple_cursors[1024] = {};
global int global_multiple_cursor_count = 1;

global b32 global_changed_multiple_cursor_count = false;


//
// @note: Handle input for multiple cursors
//
inline void
vim_multiple_cursor_handle_input_call_command(Application_Links *app, View_ID view_id, Command_Binding binding) {
    if (global_multiple_cursor_count == 1) {
        binding.custom(app);
    }
    else {
        Buffer_ID buffer_id = view_get_buffer(app, view_id, Access_Always);
        int cursor_count = global_multiple_cursor_count;
        
        global_multiple_cursors[0] = view_get_cursor_pos(app, view_id);
        b32 do_exit = false;
        for (int cursor_index = 0; !do_exit && (cursor_index < cursor_count); ++cursor_index) {
            i64 start_buffer_size = buffer_get_size(app, buffer_id);
            
            i64 *cursor = global_multiple_cursors + cursor_index;
            view_set_cursor(app, view_id, seek_pos(*cursor));
            
            binding.custom(app);
            if (global_changed_multiple_cursor_count) {
                global_changed_multiple_cursor_count = false;
                do_exit = true;
            }
            
            
            i64 end_buffer_size = buffer_get_size(app, buffer_id);
            i64 offset = 0;
            if (start_buffer_size < end_buffer_size) {
                offset += end_buffer_size - start_buffer_size;
            }
            else {
                offset -= start_buffer_size - end_buffer_size;
            }
            
            for (int i = 0; i < cursor_count; ++i) {
                if (i == cursor_index)  continue;
                i64 *it = global_multiple_cursors + i;
                
                if (*it > *cursor) {
                    *it += offset;
                }
            }
            
            *cursor = view_get_cursor_pos(app, view_id);
        }
        
        view_set_cursor(app, view_id, seek_pos(global_multiple_cursors[0]));
    }
}


//
// @note Draw multiple cursors
//

function void
vim_draw_multiple_cursors(Application_Links *app, Text_Layout_ID text_layout_id, f32 cursor_roundness, b32 is_mode_insert) {
    for (int cursor_index = 1; cursor_index < global_multiple_cursor_count; ++cursor_index) {
        i64 cursor_pos = global_multiple_cursors[cursor_index];
        
        if (is_mode_insert) {
            draw_character_i_bar(app, text_layout_id, cursor_pos, fcolor_id(defcolor_cursor));
        }
        else {
            draw_character_block(app, text_layout_id, cursor_pos, cursor_roundness, fcolor_id(defcolor_cursor));
            paint_text_color_pos(app, text_layout_id, cursor_pos, fcolor_id(defcolor_at_cursor));
        }
    }
}

function void
vim_draw_multiple_cursors(Application_Links *app, View_ID view_id, Text_Layout_ID text_layout_id, f32 cursor_roundness) {
    Managed_Scope view_scope = view_get_managed_scope(app, view_id);
    Vim_View_State *vim_state = scope_attachment(app, view_scope, view_vim_state_id, Vim_View_State);
    
    b32 is_mode_insert = (vim_state->mode == vim_mode_insert);
    
    vim_draw_multiple_cursors(app, text_layout_id, cursor_roundness, is_mode_insert);
}


//
// @note: Commands
//
CUSTOM_COMMAND_SIG(vim_add_multiple_cursor_down) {
    Assert(global_multiple_cursor_count < ArrayCount(global_multiple_cursors));
    
    View_ID view_id = get_active_view(app, Access_Always);
    
    global_multiple_cursors[0] = view_get_cursor_pos(app, view_id);
    view_set_cursor(app, view_id, seek_pos(global_multiple_cursors[global_multiple_cursor_count-1]));
    move_down(app);
    global_multiple_cursors[global_multiple_cursor_count++] = view_get_cursor_pos(app, view_id);
    
    view_set_cursor(app, view_id, seek_pos(global_multiple_cursors[0]));
    
    global_changed_multiple_cursor_count = true;
}

CUSTOM_COMMAND_SIG(vim_remove_multiple_cursor_up) {
    if (global_multiple_cursor_count == 1)  return;
    --global_multiple_cursor_count;
    
    global_changed_multiple_cursor_count = true;
}


function void
vim_multiple_cursor_isearch(Application_Links *app, Scan_Direction start_scan, i64 first_pos, String_Const_u8 query_init) {
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
    
    String_Const_u8 isearch_str = string_u8_litexpr("MultipleCursor-Search: ");
    String_Const_u8 rsearch_str = string_u8_litexpr("Reverse-MultipleCursor-Search: ");
    
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
        scroll_cursor_line(app, 0);
        
        in = get_next_input(app, EventPropertyGroup_AnyKeyboardEvent, EventProperty_Escape|EventProperty_ViewActivation);
        if (in.abort) {
            break;
        }
        Input_Modifier_Set *mods = &in.event.key.modifiers;
        
        String_Const_u8 string = to_writable(&in);
        
        b32 string_change = false;
        if (match_key_code(&in, KeyCode_Return) ||
            match_key_code(&in, KeyCode_Tab)) {
#if 0
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
#endif
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
            else if (match_key_code(&in, KeyCode_PageUp) ||
                     match_key_code(&in, KeyCode_Up) ||
                     (match_key_code(&in, KeyCode_R) && has_modifier(mods, KeyCode_Control)) ||
                     (match_key_code(&in, KeyCode_K) && has_modifier(mods, KeyCode_Alt))) {
                change_scan = Scan_Backward;
                do_scan_action = true;
            }
            else if (match_key_code(&in, KeyCode_Return)) {
                global_multiple_cursors[global_multiple_cursor_count++] = pos;
                change_scan = Scan_Forward;
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
                    if (new_pos < buffer_size) {
                        pos = new_pos;
                        match_size = bar.string.size;
                    }
                }break;
                
                case Scan_Backward:
                {
                    i64 new_pos = 0;
                    seek_string_insensitive_backward(app, buffer, pos + 1, 0, bar.string, &new_pos);
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
                    if (new_pos < buffer_size) {
                        pos = new_pos;
                        match_size = bar.string.size;
                    }
                }break;
                
                case Scan_Backward:
                {
                    i64 new_pos = 0;
                    seek_string_insensitive_backward(app, buffer, pos, 0, bar.string, &new_pos);
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
    
    // :cursor_mark
    view_set_cursor_and_preferred_x(app, view, seek_pos(pos));
}

CUSTOM_COMMAND_SIG(vim_multiple_cursor_search) {
    View_ID view = get_active_view(app, Access_ReadVisible);
    i64 pos = view_get_cursor_pos(app, view);
    vim_multiple_cursor_isearch(app, Scan_Forward, pos, SCu8());
}


