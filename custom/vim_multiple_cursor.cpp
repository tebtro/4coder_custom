#define USE_MULTIPLE_CURSORS 0

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
    vim_move_down(app);
    global_multiple_cursors[global_multiple_cursor_count++] = view_get_cursor_pos(app, view_id);
    
    view_set_cursor(app, view_id, seek_pos(global_multiple_cursors[0]));
    
    global_changed_multiple_cursor_count = true;
}

CUSTOM_COMMAND_SIG(vim_remove_multiple_cursor_up) {
    if (global_multiple_cursor_count == 1)  return;
    --global_multiple_cursor_count;
    
    global_changed_multiple_cursor_count = true;
}

