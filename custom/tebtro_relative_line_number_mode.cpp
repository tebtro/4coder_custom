//
// @note Usage:
//
// In default_render_caller or your custom version.
// Call layout_relative_line_number_margin instead of layout_line_number_margin
// and  draw_relative_line_number_margin   instead of draw_line_number_margin
//
// Or use the follwing code so that you can toggle relative line numbers.
#if 0
function void
tebtro_render_caller(Application_Links *app, Frame_Info frame_info, View_ID view_id) {
    //...
    //...
    //...
    
    // @note: Layout line numbers
    Rect_f32 line_number_rect = {};
    if (global_config.show_line_number_margins) {
        Rect_f32_Pair pair;
        if (global_use_relative_line_number_mode) {
            pair = layout_relative_line_number_margin(app, view_id, buffer_id, region, digit_advance);
        }
        else {
            pair = layout_line_number_margin(app, buffer_id, region, digit_advance);
        }
        line_number_rect = pair.min;
        region = pair.max;
    }
    
    // @note Vim :view_changed_flash_line
    if (is_active_view && vim_global_view_changed_time > 0.0f) {
        vim_global_view_changed_time -= frame_info.literal_dt;
        animate_in_n_milliseconds(app, (u32)(vim_global_view_changed_time * 1000.0f));
    }
    
    // @note: begin buffer render
    Buffer_Point buffer_point = scroll.position;
    Text_Layout_ID text_layout_id = text_layout_create(app, buffer_id, region, buffer_point);
    
    // @note: draw line numbers
    if (global_config.show_line_number_margins) {
        if (global_use_relative_line_number_mode) {
            // @note relative
            draw_relative_line_number_margin(app, view_id, buffer_id, face_id, text_layout_id, line_number_rect);
        }
        else {
            draw_line_number_margin(app, view_id, buffer_id, face_id, text_layout_id, line_number_rect);
        }
    }
    
    // @note: draw the buffer
    tebtro_render_buffer(app, view_id, face_id, buffer_id, text_layout_id, region, frame_info, vim_state_ptr);
    
    text_layout_free(app, text_layout_id);
    draw_set_clip(app, prev_clip);
}
#endif


//
// @note Commands
//

global b32 global_use_relative_line_number_mode = true;

CUSTOM_COMMAND_SIG(toggle_relative_line_number_mode)
CUSTOM_DOC("Toggle relative line number mode.") {
    global_use_relative_line_number_mode = !global_use_relative_line_number_mode;
}


//
// @note Draw line numbers
//
function Rect_f32_Pair
layout_relative_line_number_margin(Application_Links *app, View_ID view_id, Buffer_ID buffer_id, Rect_f32 rect, f32 digit_advance) {
    i64 line_count_digit_count;
    {
        // @note: We need to guess a bit here, because this functions creates the maring rect,
        //        which is returned and used to create the actual text layout,
        //        which we would need to get real numbers.
        i64 current_line_number = get_line_number_from_pos(app, buffer_id, view_get_cursor_pos(app, view_id));
#if 0
        line_count_digit_count = digit_count_from_integer(current_line_number, 10);
        // @note: Guessing that you probably won't fit more than 99 lines on the screen.
        if (line_count_digit_count < 2)  line_count_digit_count = 2;
#else
        // @note: Calculate the line counts with face_mecric.line_height
        Face_ID face_id = get_face_id(app, buffer_id);
        Face_Metrics face_metrics = get_face_metrics(app, face_id);
        f32 line_height = face_metrics.line_height;
        
        i64 cursor_pos = view_get_cursor_pos(app, view_id);
        Buffer_Cursor cursor = view_compute_cursor(app, view_id, seek_pos(cursor_pos));
        Vec2_f32 p = view_relative_xy_of_pos(app, view_id, cursor.line, cursor.pos);
        
        i64 line_count;
        {
            f32 top_height = (p.y - rect.y0);
            f32 bottom_height = (rect.y1 - p.y);
            
            i64 top_line_count = (i64)(top_height / line_height);
            i64 bottom_line_count = (i64)(bottom_height / line_height);
            
            line_count = Max(top_line_count, bottom_line_count);
        }
        
        line_count = Max(line_count, current_line_number);
        line_count_digit_count = digit_count_from_integer(line_count, 10);
#endif
    }
    return(layout_line_number_margin(rect, digit_advance, line_count_digit_count));
}

function void
draw_relative_line_number_margin(Application_Links *app, View_ID view_id, Buffer_ID buffer_id, Face_ID face_id, Text_Layout_ID text_layout_id, Rect_f32 margin) {
    ProfileScope(app, "draw line number margin");
    
    Scratch_Block scratch(app);
    FColor line_color = fcolor_id(defcolor_line_numbers_text);
    
    Rect_f32 prev_clip = draw_set_clip(app, margin);
    draw_rectangle_fcolor(app, margin, 0.f, fcolor_id(defcolor_line_numbers_back));
    
    Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
    i64 line_count = buffer_get_line_count(app, buffer_id);
    
    Buffer_Cursor cursor = view_compute_cursor(app, view_id, seek_pos(visible_range.first));
    i64 line_number = cursor.line;
    
    Buffer_Cursor cursor_opl = view_compute_cursor(app, view_id, seek_pos(visible_range.one_past_last));
    i64 one_past_last_line_number = cursor_opl.line + 1;
    
    i64 current_line_number = get_line_number_from_pos(app, buffer_id, view_get_cursor_pos(app, view_id));
    i64 line_count_digit_count;
    {
        i64 offset_start = current_line_number - line_number;
        i64 offset_end   = one_past_last_line_number - 1 - current_line_number - 1;
        i64 highest = Max(current_line_number, Max(offset_start, offset_end));
        line_count_digit_count = digit_count_from_integer(highest, 10);
    }
    
    for (;line_number < one_past_last_line_number && line_number < line_count;
         line_number += 1) {
        Temp_Memory_Block temp(scratch);
        
        Range_f32 line_y = text_layout_line_on_screen(app, text_layout_id, line_number);
        Vec2_f32 p = V2f32(margin.x0, line_y.min);
        
        i64 line_number_relative = current_line_number - line_number;
        if (line_number_relative < 0)  line_number_relative = -line_number_relative;
        if (line_number == current_line_number)  line_number_relative = current_line_number;
        
        Fancy_String *string = push_fancy_stringf(scratch, 0, line_color,
                                                  "%*lld",
                                                  line_count_digit_count,
                                                  line_number_relative);
        draw_fancy_string(app, face_id, fcolor_zero(), string, p);
    }
    
    draw_set_clip(app, prev_clip);
}
