//
// @note Draw vertical line range highlight
//
// @note: https://github.com/ryanfleury/4coder_fleury
//        4coder_fleury_cursor.cpp
//
function void
vim_draw_vertical_line_highlight_range(Application_Links *app, View_ID view_id, Text_Layout_ID text_layout_id, Range_i64 range, ARGB_Color argb_color, f32 width_multiplier = 0.05f) {
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
        draw_rectangle(app, center_rect, 3.0f, argb_color);
    }
    else {
        Rect_f32 center_rect;
        center_rect.x0 = min_rect.x0;
        center_rect.x1 = max_rect.x1;
        center_rect.y0 = min_rect.y1;
        center_rect.y1 = max_rect.y0;
        draw_rectangle(app, center_rect, 3.0f, argb_color);
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
    
    draw_rectangle(app, top_rect1, 3.f, argb_color);
    draw_rectangle(app, bottom_rect1, 3.f, argb_color);
    
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
    
    draw_rectangle(app, top_rect2, 3.f, argb_color);
    draw_rectangle(app, bottom_rect2, 3.f, argb_color);
#endif
    
    draw_rectangle(app, left_rect, 3.f, argb_color);
    draw_rectangle(app, right_rect, 3.f, argb_color);
    
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
    
#if USE_MULTIPLE_CURSORS
    // @note Draw multiple cursors
    vim_draw_multiple_cursors(app, text_layout_id, cursor_roundness, is_mode_insert);
#endif
    
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
draw_file_bar(Application_Links *app, View_ID view_id, b32 is_active, Buffer_ID buffer, Face_ID face_id, Rect_f32 bar) {
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
    String_Const_u8 unique_name = push_buffer_unique_name(app, scratch, buffer);
    push_fancy_string(scratch, &list, base_color, unique_name);
    push_fancy_stringf(scratch, &list, base_color, " - Row: %3.lld Col: %3.lld -", cursor.line, cursor.col);
    
    Managed_Scope scope = buffer_get_managed_scope(app, buffer);
    Line_Ending_Kind *eol_setting = scope_attachment(app, scope, buffer_eol_setting,
                                                     Line_Ending_Kind);
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
    
    {
        Dirty_State dirty = buffer_get_dirty_state(app, buffer);
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

//
// @note Draw line numbers
//
function void
draw_line_number_relative_margin(Application_Links *app, View_ID view_id, Buffer_ID buffer_id, Face_ID face_id, Text_Layout_ID text_layout_id, Rect_f32 margin) {
    Rect_f32 prev_clip = draw_set_clip(app, margin);
    draw_rectangle_fcolor(app, margin, 0.f, fcolor_id(defcolor_line_numbers_back));
    
    Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
    
    FColor line_color = fcolor_id(defcolor_line_numbers_text);
    
    i64 line_count = buffer_get_line_count(app, buffer_id);
    i64 line_count_digit_count = digit_count_from_integer(line_count, 10);
    
    Scratch_Block scratch(app, Scratch_Share);
    
    i64 current_line_number = get_line_number_from_pos(app, buffer_id, view_get_cursor_pos(app, view_id));
    Buffer_Cursor cursor = view_compute_cursor(app, view_id, seek_pos(visible_range.first));
    i64 line_number = cursor.line;
    for (;cursor.pos <= visible_range.one_past_last;){
        if (line_number > line_count){
            break;
        }
        Range_f32 line_y = text_layout_line_on_screen(app, text_layout_id, line_number);
        Vec2_f32 p = V2f32(margin.x0, line_y.min);
        Temp_Memory_Block temp(scratch);
        
        i64 line_number_relative = current_line_number - line_number;
        if (line_number_relative < 0)  line_number_relative = -line_number_relative;
        if (line_number == current_line_number)  line_number_relative = current_line_number;
        
        Fancy_String *string = push_fancy_stringf(scratch, 0, line_color,
                                                  "%*lld",
                                                  line_count_digit_count,
                                                  line_number_relative);
        draw_fancy_string(app, face_id, fcolor_zero(), string, p);
        line_number += 1;
    }
    
    draw_set_clip(app, prev_clip);
}