//
// @note Draw vertical line range highlight
//
// @note: https://github.com/ryanfleury/4coder_fleury
//        4coder_fleury_cursor.cpp
//
function void
vim_draw_vertical_line_highlight_range(Application_Links *app, View_ID view_id, Text_Layout_ID text_layout_id, Range_i64 range, ARGB_Color argb_color, f32 width_multiplier = 0.1f) {
    f32 roundness = 2.0f;
    
    Rect_f32 view_rect = view_get_screen_rect(app, view_id);
    Rect_f32 clip = draw_set_clip(app, view_rect);
    
    Rect_f32 min_rect = text_layout_character_on_screen(app, text_layout_id, range.min);
    Rect_f32 max_rect = text_layout_character_on_screen(app, text_layout_id, range.max);
    
    f32 lower_bound_y;
    f32 upper_bound_y;
    if(min_rect.y0 < max_rect.y0) {
        lower_bound_y = min_rect.y0;
        upper_bound_y = max_rect.y1;
    }
    else {
        lower_bound_y = max_rect.y0;
        upper_bound_y = min_rect.y1;
    }
    
    Face_ID face_id = get_face_id(app, 0);
    Face_Metrics metrics = get_face_metrics(app, face_id);
    f32 width = metrics.normal_advance * width_multiplier;
    
    Rect_f32 left_rect  = Rf32(view_rect.x0, lower_bound_y, view_rect.x0 + width, upper_bound_y);
    Rect_f32 right_rect = Rf32(view_rect.x1 - width, lower_bound_y, view_rect.x1, upper_bound_y);
    if(min_rect.y0 < max_rect.y0) {
        left_rect.y0 += metrics.line_height;
        right_rect.y1 -= metrics.line_height;
    }
    else {
        left_rect.y1  -= metrics.line_height;
        right_rect.y0 += metrics.line_height;
    }
    
#if 0
    // @note: Fill rect
    if(min_rect.y0 < max_rect.y0) {
        right_rect = min_rect;
        right_rect.x1 = view_rect.x1;
        
        left_rect = max_rect;
        left_rect.x0 = view_rect.x0;
        
        
        Rect_f32 center_rect;
        center_rect.x0 = view_rect.x0;
        center_rect.x1 = view_rect.x1;
        center_rect.y0 = min_rect.y1;
        center_rect.y1 = max_rect.y0;
        draw_rectangle(app, center_rect, roundness, argb_color);
    }
    else {
        Rect_f32 center_rect;
        center_rect.x0 = min_rect.x0;
        center_rect.x1 = max_rect.x1;
        center_rect.y0 = min_rect.y1;
        center_rect.y1 = max_rect.y0;
        draw_rectangle(app, center_rect, roundness, argb_color);
    }
#endif
    
#if 0
    // @note: Draw additional horizontal outline
    f32 height = metrics.line_height * 0.06f;
    
    Rect_f32 top_rect1;
    top_rect1.x0 = view_rect.x0;
    top_rect1.x1 = min_rect.x0;
    top_rect1.y0 = min_rect.y1;
    top_rect1.y1 = top_rect1.y0 + height;
    
    Rect_f32 bottom_rect1;
    bottom_rect1.x0 = max_rect.x1;
    bottom_rect1.x1 = view_rect.x1;
    bottom_rect1.y1 = max_rect.y0;
    bottom_rect1.y0 = bottom_rect1.y1 - height;
    
    draw_rectangle(app, top_rect1, roundness, argb_color);
    draw_rectangle(app, bottom_rect1, roundness, argb_color);
    
    Rect_f32 top_rect2;
    top_rect2.x0 = min_rect.x1;
    top_rect2.x1 = view_rect.x1;
    top_rect2.y0 = min_rect.y0;
    top_rect2.y1 = top_rect2.y0 - height;
    
    Rect_f32 bottom_rect2;
    bottom_rect2.x0 = view_rect.x0;
    bottom_rect2.x1 = max_rect.x0;
    bottom_rect2.y1 = max_rect.y1;
    bottom_rect2.y0 = bottom_rect2.y1 + height;
    
    draw_rectangle(app, top_rect2, roundness, argb_color);
    draw_rectangle(app, bottom_rect2, roundness, argb_color);
#endif
    
    draw_rectangle(app, left_rect,  roundness, argb_color);
    draw_rectangle(app, right_rect, roundness, argb_color);
    
    draw_set_clip(app, clip);
}

function void
vim_draw_vertical_line_highlight_range(Application_Links *app, View_ID view_id, Text_Layout_ID text_layout_id, Range_i64 range, FColor color, f32 width_multiplier = 0.05f) {
    ARGB_Color argb = fcolor_resolve(color);
    vim_draw_vertical_line_highlight_range(app, view_id, text_layout_id, range, argb, width_multiplier);
}

//
// @note Draw cursor mark
//
function void
vim_draw_cursor_mark(Application_Links *app, View_ID view_id, b32 is_active_view, Buffer_ID buffer_id,
                     Text_Layout_ID text_layout_id, f32 cursor_roundness, f32 mark_outline_thickness) {
    Managed_Scope view_scope = view_get_managed_scope(app, view_id);
    Vim_View_State *vim_state = scope_attachment(app, view_scope, view_vim_state_id, Vim_View_State);
    b32 is_mode_insert = (vim_state->mode == vim_mode_insert);
    
    // @note draw cursor mark
    i64 cursor_pos = view_get_cursor_pos(app, view_id);
    i64 mark_pos = view_get_mark_pos(app, view_id);
    b32 cursor_before_mark = (cursor_pos <= mark_pos);
    // @note cursor
    if (is_active_view && is_mode_insert) {
        draw_character_i_bar(app, text_layout_id, cursor_pos, fcolor_id(defcolor_cursor));
    }
    else if (is_active_view) {
        draw_character_block(app, text_layout_id, cursor_pos, cursor_roundness, fcolor_id(defcolor_cursor));
        paint_text_color_pos(app, text_layout_id, cursor_pos, fcolor_id(defcolor_at_cursor));
    }
    else {
        draw_character_wire_frame(app, text_layout_id, cursor_pos, cursor_roundness, mark_outline_thickness, fcolor_id(defcolor_cursor));
    }
    // @note mark
    if (!is_mode_insert) {
        draw_character_wire_frame(app, text_layout_id, mark_pos, cursor_roundness, mark_outline_thickness, fcolor_id(defcolor_mark));
    }
}


//
// @note: Shooth cursor and mark
//
// Source: https://github.com/ryanfleury/4coder_fleury
//

static void
vim_do_the_cursor_interpolation(Application_Links *app, Frame_Info frame_info,
                                Rect_f32 *rect, Rect_f32 *last_rect, Rect_f32 target) {
    *last_rect = *rect;
    
    float x_change = target.x0 - rect->x0;
    float y_change = target.y0 - rect->y0;
    
    float cursor_size_x = (target.x1 - target.x0);
    float cursor_size_y = (target.y1 - target.y0) * (1 + fabsf(y_change) / 80.f);
    
    b32 should_animate_cursor = !global_battery_saver;
    if (should_animate_cursor) {
        if (fabs(x_change) > 1.f || fabs(y_change) > 1.f) {
            animate_in_n_milliseconds(app, 0);
        }
    }
    else {
        *rect = *last_rect = target;
        cursor_size_y = target.y1 - target.y0;
    }
    
    if (should_animate_cursor) {
        rect->x0 += (x_change) * frame_info.animation_dt * 40.f;
        rect->y0 += (y_change) * frame_info.animation_dt * 40.f;
        rect->x1 = rect->x0 + cursor_size_x;
        rect->y1 = rect->y0 + cursor_size_y;
    }
    
    if (target.y0 > last_rect->y0) {
        if (rect->y0 < last_rect->y0) {
            rect->y0 = last_rect->y0;
        }
    }
    else {
        if (rect->y1 > last_rect->y1) {
            rect->y1 = last_rect->y1;
        }
    }
}

function void
vim_draw_smooth_cursor_mark(Application_Links *app, View_ID view_id, b32 is_active_view, Buffer_ID buffer_id,
                            Text_Layout_ID text_layout_id, f32 cursor_roundness, f32 mark_outline_thickness, Frame_Info frame_info) {
    Managed_Scope view_scope = view_get_managed_scope(app, view_id);
    Vim_View_State *vim_state = scope_attachment(app, view_scope, view_vim_state_id, Vim_View_State);
    b32 is_mode_insert = (vim_state->mode == vim_mode_insert);
    
    i64 cursor_pos = view_get_cursor_pos(app, view_id);
    i64 mark_pos = view_get_mark_pos(app, view_id);
    b32 cursor_before_mark = (cursor_pos <= mark_pos);
    
    //
    // @note: Do the interpolation
    //
    Rect_f32 view_rect = view_get_screen_rect(app, view_id);
    Rect_f32 clip = draw_set_clip(app, view_rect);
    Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
    
    if (is_active_view) {
        //~ Cursor
        Rect_f32 target_cursor = text_layout_character_on_screen(app, text_layout_id, cursor_pos);
        
        if (cursor_pos < visible_range.start || cursor_pos > visible_range.end) {
            f32 width = target_cursor.x1 - target_cursor.x0;
            target_cursor.x0 = view_rect.x0;
            target_cursor.x1 = target_cursor.x0 + width;
        }
        
        vim_do_the_cursor_interpolation(app, frame_info, &global_cursor_rect,
                                        &global_last_cursor_rect, target_cursor);
        
        //~ Mark
        Rect_f32 target_mark = text_layout_character_on_screen(app, text_layout_id, mark_pos);
        
        if (mark_pos > visible_range.end) {
            target_mark.x0 = 0;
            target_mark.y0 = view_rect.y1;
            target_mark.y1 = view_rect.y1;
        }
        
        if (mark_pos < visible_range.start || mark_pos > visible_range.end) {
            f32 width = target_mark.x1 - target_mark.x0;
            target_mark.x0 = view_rect.x0;
            target_mark.x1 = target_mark.x0 + width;
        }
        
        vim_do_the_cursor_interpolation(app, frame_info, &global_mark_rect, &global_last_mark_rect,
                                        target_mark);
    }
    //~
    
    // @note Draw cursor
    if (is_active_view && is_mode_insert) {
        //draw_character_i_bar(app, text_layout_id, cursor_pos, fcolor_id(defcolor_cursor));
        global_cursor_rect.x1 = global_cursor_rect.x0 + 1.f;
        draw_rectangle(app, global_cursor_rect, 0.f, fcolor_resolve(fcolor_id(defcolor_cursor)));
    }
    else if (is_active_view) {
        //draw_character_block(app, text_layout_id, cursor_pos, cursor_roundness, fcolor_id(defcolor_cursor));
        draw_rectangle(app, global_cursor_rect, cursor_roundness, fcolor_resolve(fcolor_id(defcolor_cursor)));
        
        paint_text_color_pos(app, text_layout_id, cursor_pos, fcolor_id(defcolor_at_cursor));
    }
    else {
        //draw_character_wire_frame(app, text_layout_id, cursor_pos, cursor_roundness, mark_outline_thickness, fcolor_id(defcolor_cursor));
        draw_rectangle_outline(app, global_cursor_rect, cursor_roundness, mark_outline_thickness, fcolor_resolve(fcolor_id(defcolor_cursor)));
    }
    
    // @note Draw mark
    if (!is_mode_insert) {
        //draw_character_wire_frame(app, text_layout_id, mark_pos, cursor_roundness, mark_outline_thickness, fcolor_id(defcolor_mark));
        draw_rectangle_outline(app, global_mark_rect, cursor_roundness, mark_outline_thickness, fcolor_resolve(fcolor_id(defcolor_mark)));
    }
    
    draw_set_clip(app, clip);
}


//
// @note Draw visual mode highlight
//
function void
vim_draw_highlight(Application_Links *app, View_ID view_id, b32 is_active_view, Buffer_ID buffer_id,
                   Text_Layout_ID text_layout_id, f32 cursor_roundness, f32 mark_outline_thickness) {
    Managed_Scope view_scope = view_get_managed_scope(app, view_id);
    Vim_View_State *vim_state = scope_attachment(app, view_scope, view_vim_state_id, Vim_View_State);
    
    // @note draw highlight
    Range_i64 range = vim_state->selection_range;
    if (vim_state->mode == vim_mode_visual) {
        draw_character_block(app, text_layout_id, range, cursor_roundness, fcolor_id(defcolor_highlight));
    }
    else if (vim_state->mode == vim_mode_visual_line) {
        // @note get_line_range_from_pos_range takes a range that is NOT one_past_last.
        Range_i64 line_range = get_line_range_from_pos_range(app, buffer_id, Ii64(range.start, range.one_past_last - 1));
        draw_line_highlight(app, text_layout_id, line_range, fcolor_id(defcolor_highlight));
    }
    if (vim_state->mode == vim_mode_visual || vim_state->mode == vim_mode_visual_line) {
        // @note Don't paint the text, so that the token coloring is still visible
        // paint_text_color_fcolor(app, text_layout_id, range, fcolor_id(defcolor_at_highlight));
    }
}

//
// @note Draw visual mode whitespaces
//
inline void
vim_draw_character_pos_circle(Application_Links *app, Face_ID face_id, Buffer_ID buffer_id, Text_Layout_ID text_layout_id, i64 pos, ARGB_Color argb) {
    Face_Metrics face_metrics = get_face_metrics(app, face_id);
    f32 roundness = (face_metrics.normal_advance*0.5f)*1.1f;
    
    Rect_f32 char_rect = text_layout_character_on_screen(app, text_layout_id, pos);
    
    f32 width = char_rect.x1 - char_rect.x0;
    f32 height = char_rect.y1 - char_rect.y0;
    f32 offset = (height-width) * 0.5f;
    char_rect.y0 += offset;
    char_rect.y1 -= offset;
    
    f32 scaling = width * 0.1f;
    char_rect.y0 += scaling;
    char_rect.y1 -= scaling;
    char_rect.x0 += scaling;
    char_rect.x1 -= scaling;
    
    i64 line_end_pos = get_line_end_pos_from_pos(app, buffer_id, pos);
    if (pos < line_end_pos-1) {
        draw_rectangle(app, char_rect, roundness, argb);
    }
    else {
        f32 thickness = (width * 0.25f);
        draw_rectangle_outline(app, char_rect, roundness, thickness, argb);
    }
    
    /*
    draw_rectangle(Application_Links* app, Rect_f32 rect, f32 roundness, ARGB_Color color);
    draw_rectangle_outline(Application_Links* app, Rect_f32 rect, f32 roundness, f32 thickness, ARGB_Color color);
    */
    // paint_text_color(app, text_layout_id, range, argb);
}

inline void
vim_draw_whitespaces_in_range(Application_Links *app, View_ID view_id, Face_ID face_id, Buffer_ID buffer_id, Text_Layout_ID text_layout_id, Token_Array *token_array, Range_i64 range, ARGB_Color argb) {
    if (token_array == 0 || token_array->tokens == 0)  return;
    
    i64 first_index = token_index_from_pos(token_array, range.first);
    Token_Iterator_Array it = token_iterator_index(0, token_array, first_index);
    
    Scratch_Block scratch(app);
    for (;;) {
        Temp_Memory_Block temp(scratch);
        Token *token = token_it_read(&it);
        if (!token || token->pos >= range.one_past_last) {
            break;
        }
        
        String_Const_u8 lexeme = {};
        if (token->kind == TokenBaseKind_Whitespace ||
            token->kind == TokenBaseKind_Comment    ||
            token->kind == TokenBaseKind_LiteralString) {
            // lexeme = push_token_lexeme(app, scratch, (Buffer_ID)it.user_id, token);
            Range_i64 token_range = Ii64_size(token->pos, token->size);
            lexeme = push_buffer_range(app, scratch, buffer_id, token_range);
            for (i64 pos = token_range.start; pos < token_range.one_past_last; ++pos) {
                if (pos < range.first)  continue;
                if (pos >= range.one_past_last)  continue;
                if (lexeme.str) {
                    i64 i = pos - token_range.start;
                    if (lexeme.str[i] != ' ' && lexeme.str[i] != '\n')  continue;
                }
                vim_draw_character_pos_circle(app, face_id, buffer_id, text_layout_id, pos, argb);
            }
        }
        
        if (!token_it_inc_all(&it)) {
            break;
        }
    }
}

inline void
vim_draw_visual_mode_whitespaces(Application_Links *app, View_ID view_id, Face_ID face_id, Buffer_ID buffer_id, Text_Layout_ID text_layout_id, Token_Array *token_array, ARGB_Color argb) {
    if (token_array == 0 || token_array->tokens == 0)  return;
    
    Managed_Scope view_scope = view_get_managed_scope(app, view_id);
    Vim_View_State *vim_state = scope_attachment(app, view_scope, view_vim_state_id, Vim_View_State);
    if (vim_state->mode != vim_mode_visual && vim_state->mode != vim_mode_visual_line)  return;
    
    // @note draw highlight
    Range_i64 range = vim_state->selection_range;
    // Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
    vim_draw_whitespaces_in_range(app, view_id, face_id, buffer_id, text_layout_id, token_array, range, argb);
}

//
// @note Draw file bar
//
function void
vim_draw_file_bar(Application_Links *app, View_ID view_id, b32 is_active, Buffer_ID buffer_id, Face_ID face_id, Rect_f32 bar) {
    Scratch_Block scratch(app);
    
    FColor bar_color = fcolor_zero();
    if (is_active) {
        bar_color = fcolor_id(defcolor_bar_active);
    }
    else {
        bar_color = fcolor_id(defcolor_bar);
    }
    draw_rectangle_fcolor(app, bar, 0.f, bar_color);
    
    FColor base_color = fcolor_id(defcolor_base);
    FColor pop2_color = fcolor_id(defcolor_pop2);
    
    i64 cursor_position = view_get_cursor_pos(app, view_id);
    Buffer_Cursor cursor = view_compute_cursor(app, view_id, seek_pos(cursor_position));
    
    Fancy_Line list = {};
    String_Const_u8 unique_name = push_buffer_unique_name(app, scratch, buffer_id);
    push_fancy_string(scratch, &list, base_color, unique_name);
    push_fancy_stringf(scratch, &list, base_color, " - Row: %3.lld Col: %3.lld -", cursor.line, cursor.col);
    
    Managed_Scope buffer_scope = buffer_get_managed_scope(app, buffer_id);
    Line_Ending_Kind *eol_setting = scope_attachment(app, buffer_scope, buffer_eol_setting, Line_Ending_Kind);
    switch (*eol_setting){
        case LineEndingKind_Binary:
        {
            push_fancy_string(scratch, &list, base_color, string_u8_litexpr(" bin"));
        }break;
        
        case LineEndingKind_LF:
        {
            push_fancy_string(scratch, &list, base_color, string_u8_litexpr(" lf"));
        }break;
        
        case LineEndingKind_CRLF:
        {
            push_fancy_string(scratch, &list, base_color, string_u8_litexpr(" crlf"));
        }break;
    }
    
    Managed_Scope view_scope = view_get_managed_scope(app, view_id);
    Vim_View_State *vim_state = scope_attachment(app, view_scope, view_vim_state_id, Vim_View_State);
    // @note: Draw major mode
    if (0) {
        switch (vim_state->mode) {
            case vim_mode_normal: {
                push_fancy_string(scratch, &list, base_color, string_u8_litexpr(" <N>"));
            } break;
            case vim_mode_insert: {
                push_fancy_string(scratch, &list, base_color, string_u8_litexpr(" <I>"));
            } break;
            case vim_mode_replace: {
                push_fancy_string(scratch, &list, base_color, string_u8_litexpr(" <R>"));
            } break;
            case vim_mode_visual: {
                push_fancy_string(scratch, &list, base_color, string_u8_litexpr(" <v>"));
            } break;
            case vim_mode_visual_line: {
                push_fancy_string(scratch, &list, base_color, string_u8_litexpr(" <V>"));
            } break;
        }
    }
    else {
        switch (vim_state->mode) {
            case vim_mode_normal: {
                push_fancy_string(scratch, &list, base_color, string_u8_litexpr(" --Normal--"));
            } break;
            case vim_mode_insert: {
                push_fancy_string(scratch, &list, base_color, string_u8_litexpr(" --Insert--"));
            } break;
            case vim_mode_replace: {
                push_fancy_string(scratch, &list, base_color, string_u8_litexpr(" --Replace--"));
            } break;
            case vim_mode_visual: {
                push_fancy_string(scratch, &list, base_color, string_u8_litexpr(" --Visual--"));
            } break;
            case vim_mode_visual_line: {
                push_fancy_string(scratch, &list, base_color, string_u8_litexpr(" --Visual-Line--"));
            } break;
        }
    }
    
    {
        Dirty_State dirty = buffer_get_dirty_state(app, buffer_id);
        u8 space[3];
        String_u8 str = Su8(space, 0, 3);
        if (dirty != 0){
            string_append(&str, string_u8_litexpr(" "));
        }
        if (HasFlag(dirty, DirtyState_UnsavedChanges)){
            string_append(&str, string_u8_litexpr("*"));
        }
        if (HasFlag(dirty, DirtyState_UnloadedChanges)){
            string_append(&str, string_u8_litexpr("!"));
        }
        push_fancy_string(scratch, &list, pop2_color, str.string);
    }
    
    Vec2_f32 p = bar.p0 + V2f32(2.f, 2.f);
    draw_fancy_line(app, face_id, fcolor_zero(), &list, p);
}

