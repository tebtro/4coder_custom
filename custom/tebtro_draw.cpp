//
// @note Comment notation test
//
// @todo(tebtro): Hello todo!
// @note(tebtro): Hello note!
// @study(tebtro): Hello note!
// @copynpaste(Robert Ortner): Hello other notations.
// @any_text_is_valid(But whitespaces are only allowed between parens): Hello Sailor
// @also(supports (nested parens) ...): Hello Sailor
// @@ smthg
// :this_is_a_correlation_tag
// :also_with_parens(tebtro): Hello Sailor!
/*
@todo(Robert Ortner
  2020.01.03
18:37):
Hello Sailor
*/
// @note@todo@study@@:correlation_tag@@@study@todo@note:correlation_tag
//
//
//
// @note Should not be highlighted:
//
// 12:30 Some::Namespace
// ::not_valid
// D:\Programme\4coder\custom
// https://google.at
// somename@gmail.com
//
//
struct Tebtro_Comment_Highlight_Pair {
    String_Const_u8 needle;
    ARGB_Color color;
    b32 do_whitespace_search = true;
};
inline void
tebtro_draw_comment_highlights(Application_Links *app, Buffer_ID buffer_id, Text_Layout_ID text_layout_id, Token_Array *token_array) {
    if (token_array == 0 || token_array->tokens == 0)  return;
    
    Tebtro_Comment_Highlight_Pair pairs[] = {
        // tags
        { string_u8_litexpr("@note"),  0xFF00FF00 },
        { string_u8_litexpr("@todo"),  0xFFFF0000 },
        { string_u8_litexpr("@study"), 0xFFFFA500 },
        { string_u8_litexpr("@"),      0xFFFFFF00 }, // every tag with an @
        { string_u8_litexpr(":"),      0xFF00FFFF }, // correlation tag
        
        // User name
        { global_config.user_name,             0xFFE84188, false },
        { string_u8_litexpr("Robert Ortner"),  0xFFE84188, false },
    };
    i32 pair_count = ArrayCount(pairs);
    
    Scratch_Block scratch(app);
    Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
    i64 first_index = token_index_from_pos(token_array, visible_range.first);
    Token_Iterator_Array it = token_iterator_index(buffer_id, token_array, first_index);
    for (;;) {
        Temp_Memory_Block temp(scratch);
        Token *token = token_it_read(&it);
        if (token->pos >= visible_range.one_past_last) {
            break;
        }
        String_Const_u8 tail = {};
        if (token_it_check_and_get_lexeme(app, scratch, &it, TokenBaseKind_Comment, &tail)) {
            for (i64 index = token->pos; tail.size > 0; tail = string_skip(tail, 1), index += 1) {
                Tebtro_Comment_Highlight_Pair *pair = pairs;
                for (i32 pair_index = 0; pair_index < pair_count; ++pair_index, ++pair) {
                    u64 needle_size = pair->needle.size;
                    if (needle_size == 0) {
                        continue;
                    }
                    String_Const_u8 prefix = string_prefix(tail, needle_size);
                    if (string_match(prefix, pair->needle)) {
                        u64 start = index;
                        tail = string_skip(tail, needle_size - 1);
                        index += needle_size - 1;
                        
                        u64 end = start + needle_size;
                        
                        b32 do_whitespace_search = pair->do_whitespace_search;
                        if (needle_size == 1) {
                            // @note Correlation tag specific
                            if (character_is_base10(tail.str[1]) || tail.str[1] == ':' || *(tail.str - 1) == ':') {
                                do_whitespace_search = false;
                            }
                        }
                        if (do_whitespace_search) {
                            int depth = 0;
                            int i = 0;
                            for (; i < tail.size; ++i) {
                                if (tail.str[i] == '(') {
                                    for (; i < tail.size; ++i) {
                                        if (tail.str[i] == '(')  ++depth;
                                        if (tail.str[i] == ')')  --depth;
                                        if (depth == 0)  break;
                                    }
                                }
                                
                                if (tail.str[i] == '.' || tail.str[i] == '/' || tail.str[i] == '\\') { 
                                    goto break_needle_matching; 
                                }
                                if (character_is_whitespace(tail.str[i]))  break;
                            }
                            if (depth >= 0)  end += i - 1;
                        }
                        
                        Range_i64 range = Ii64(start, end);
                        if (range_size(range) < 2)  continue;
                        paint_text_color(app, text_layout_id, range, pair->color);
                        
                        break_needle_matching:;
                        break;
                    }
                }
            }
        }
        if (!token_it_inc_non_whitespace(&it)) {
            break;
        }
    }
}

// 
// @note Cpp token colorizing
// 

function void
tebtro_draw_cpp_token_colors(Application_Links *app, Text_Layout_ID text_layout_id, Buffer_ID buffer, Token_Array *array) {
    Scratch_Block scratch(app);
    
    Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
    i64 first_index = token_index_from_pos(array, visible_range.first);
    Token_Iterator_Array it = token_iterator_index(0, array, first_index);
    for (;;) {
        Token *token = token_it_read(&it);
        if (token->pos >= visible_range.one_past_last) {
            break;
        }
        FColor color = get_token_color_cpp(*token);
        ARGB_Color argb = fcolor_resolve(color);
        if (token->kind == TokenBaseKind_Keyword) {
            String_Const_u8 lexeme = push_token_lexeme(app, scratch, buffer, token);            
            if (string_match(lexeme, SCu8("void")) ||
                string_match(lexeme, SCu8("char")) ||
                string_match(lexeme, SCu8("short")) ||
                string_match(lexeme, SCu8("int")) ||
                string_match(lexeme, SCu8("long")) ||
                string_match(lexeme, SCu8("float")) ||
                string_match(lexeme, SCu8("double"))) {
                // type color
                argb = 0xFFBAA227;
            }
        }
        paint_text_color(app, text_layout_id, Ii64_size(token->pos, token->size), argb);
        if (!token_it_inc_all(&it)) {
            break;
        }
    }
}

function void
tebtro_draw_cpp_identifier_colors(Application_Links *app, Text_Layout_ID text_layout_id, Buffer_ID buffer, Token_Array *array) {
    ProfileScope(app, "draw_cpp_identifier_colors");
    Scratch_Block scratch(app);
    
    Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
    i64 first_index = token_index_from_pos(array, visible_range.first);
    Token_Iterator_Array it = token_iterator_index(0, array, first_index);
    
    code_index_lock();
    for (;;) {
        Token *token = token_it_read(&it);
        if (token->pos >= visible_range.one_past_last){
            break;
        }
        if (token->kind == TokenBaseKind_Identifier) {
            // @note Default is like for all other identifiers, so all other variable names.
            ARGB_Color argb = fcolor_resolve(fcolor_id(defcolor_text_default));
            
            // @note lookup identifier
            String_Const_u8 lexeme = push_token_lexeme(app, scratch, buffer, token);            
            Identifier_Node *node = get_global_identifier(lexeme);
            if (node != 0) {
                switch (node->note_kind) {
                    case CodeIndexNote_Type: {
                        // argb = 0xFFFF0000;
                        // j: argb = 0xFF90EE90;
                        // c: argb = 0xFFA08C54;
                        argb = 0xFFBAA227;
                    } break;
                    case CodeIndexNote_Function: {
                        // argb = 0xFF00FF00;
                        // j: argb = 0xFFFFFFFF;
                        // c: argb = 0xFF915849;
                        argb = 0xFF915849;
                    } break;
                    case CodeIndexNote_Macro: {
                        // argb = 0xFF0000FF;
                        // j: argb = 0xFFC8D4EC;
                        // c: argb = 0xFF4D716B;
                        argb = 0xFF388AD9;
                    } break;
                }
            }
            
            paint_text_color(app, text_layout_id, Ii64_size(token->pos, token->size), argb);
        }
        if (!token_it_inc_all(&it)){
            break;
        }
    }
    code_index_unlock();
}


// 
// @note Highlight token or (in comments) word under cursor.
// 
inline void
tebtro_draw_token_under_cursor_highlight(Application_Links *app, Text_Layout_ID text_layout_id, Buffer_ID buffer_id, Token_Array *token_array, i64 cursor_pos, f32 cursor_roundness) {
    if (token_array == 0 || token_array->tokens == 0)  return;
    
    Token_Iterator_Array it = token_iterator_pos(0, token_array, cursor_pos);
    Token *token = token_it_read(&it);
    if (token == 0)  return;
    if (token->kind == TokenBaseKind_Whitespace || token->kind == TokenBaseKind_EOF)  return;
    
    Range_i64 range = {};
    if (token->kind == TokenBaseKind_Comment || token->kind == TokenBaseKind_LiteralString) {
        Scratch_Block scratch(app);
        // @copynpaset :token_or_word_range
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
    
    
    // @note Draw background
    {
        ARGB_Color argb = 0xFF1A2898; // 0xFF1A30AC; // 0xFF1A303C; // 0xFF1C3039;
        draw_character_block(app, text_layout_id, range, cursor_roundness, argb);
    }
    
    // @note Draw foreground
#if 0
    {
        FColor color = get_token_color_cpp(*token);
        ARGB_Color argb = fcolor_resolve(color); // 0xFFBDB8A4; // 0xFFFF0000;
        ARGB_Color blend_argb = 0xFFFFFFFF; // 0xFFFFFFFF;
        argb = argb_color_blend(argb, 0.4f, blend_argb);
        paint_text_color(app, text_layout_id, range, argb);
    }
#endif
}

//
// @note Draw highlight range
//
inline b32
tebtro_draw_highlight_range(Application_Links *app, View_ID view_id, Buffer_ID buffer, Text_Layout_ID text_layout_id, f32 roundness,
                            ARGB_Color argb_highlight, ARGB_Color argb_at_highlight) {
    b32 has_highlight_range = false;
    Managed_Scope scope = view_get_managed_scope(app, view_id);
    Buffer_ID *highlight_buffer = scope_attachment(app, scope, view_highlight_buffer, Buffer_ID);
    if (*highlight_buffer != 0) {
        if (*highlight_buffer != buffer) {
            view_disable_highlight_range(app, view_id);
        }
        else
        {
            has_highlight_range = true;
            Managed_Object *highlight = scope_attachment(app, scope, view_highlight_range, Managed_Object);
            Marker marker_range[2];
            if (managed_object_load_data(app, *highlight, 0, 2, marker_range)) {
                Range_i64 range = Ii64(marker_range[0].pos, marker_range[1].pos);
                draw_character_block(app, text_layout_id, range, roundness, argb_highlight);
                paint_text_color(app, text_layout_id, range, argb_at_highlight);
            }
        }
    }
    return has_highlight_range;
}

//
// @note Draw whitespaces
//
inline void
tebtro_draw_whitespaces(Application_Links *app, Face_ID face_id, Buffer_ID buffer_id, Text_Layout_ID text_layout_id, Token_Array *token_array, ARGB_Color argb) {
    if (token_array == 0 || token_array->tokens == 0)  return;
    
    Face_Metrics face_metrics = get_face_metrics(app, face_id);
    f32 roundness = (face_metrics.normal_advance*0.5f)*1.1f;
    
    Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
    i64 first_index = token_index_from_pos(token_array, visible_range.first);
    Token_Iterator_Array it = token_iterator_index(0, token_array, first_index);
    
    Scratch_Block scratch(app);
    for (;;) {
        Temp_Memory_Block temp(scratch);
        Token *token = token_it_read(&it);
        if (token->pos >= visible_range.one_past_last) {
            break;
        }
        
        String_Const_u8 lexeme = {};
        if (token_it_check_and_get_lexeme(app, scratch, &it, TokenBaseKind_Whitespace, &lexeme)) {
            Range_i64 token_range = Ii64_size(token->pos, token->size);
            
            for (i64 pos = token_range.start; pos < token_range.one_past_last; ++pos) {
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
                
                i64 line_number = get_line_number_from_pos(app, buffer_id, pos);
                i64 line_end_pos = get_line_end_pos(app, buffer_id, line_number);
                if (pos != line_end_pos) {
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
        }
        
        if (!token_it_inc_all(&it)) {
            break;
        }
    }
}

//
// @note Draw brace highlight
//
inline void
tebtro_draw_brace_highlight(Application_Links *app, Buffer_ID buffer_id, Text_Layout_ID text_layout_id, i64 pos, ARGB_Color *colors, i32 color_count) {
    Scratch_Block scratch(app);
    Range_i64_Array ranges = get_enclosure_ranges(app, scratch, buffer_id, pos, RangeHighlightKind_CharacterHighlight);
    draw_enclosures(app, text_layout_id, buffer_id, pos, FindNest_Scope, RangeHighlightKind_CharacterHighlight, 0, 0, colors, color_count);
}


//
// @note Draw scope brace annotations
// 
// @note: https://github.com/ryanfleury/4coder_fleury
//        4coder_fleury_brace.cpp
//
// @todo @cleanup
// @todo Maybe only render line at the scope close brace when the open scope brace is not visible.
//       And the vertical only when scope open and close brace are not visible.
// @todo @robustness hard coded character width/height values ??
// Vec2_f32 close_scope_pos = { close_scope_rect.x0 + 12, close_scope_rect.y0 };
// @todo Smaller font size, centered in buffer font line height.
// @todo The string start pos and the scope start pos should be treated differently, because sometimes the string is rendering over the function name if theres the return type on the line before.
// 
inline void
tebtro_draw_scope_close_brace_annotations(Application_Links *app, View_ID view_id, Rect_f32 view_rect, Buffer_ID buffer_id, Text_Layout_ID text_layout_id, Face_ID face_id, i64 cursor_pos, ARGB_Color *colors, i32 color_count) {
    Face_Metrics face_metrics = get_face_metrics(app, face_id);
    i64 pos = cursor_pos;
    Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
    Token_Array token_array = get_token_array_from_buffer(app, buffer_id);
    int color_index = 0;
    
    if (token_array.tokens != 0) {
        Token_Iterator_Array it = token_iterator_pos(0, &token_array, pos);
        Token *token = token_it_read(&it);
        
        if (token != 0 && token->kind == TokenBaseKind_ScopeOpen) {
            pos = token->pos + token->size;
        }
        else if (token_it_dec_all(&it)) {
            token = token_it_read(&it);
            if (token->kind == TokenBaseKind_ScopeClose && pos == token->pos + token->size) {
                pos = token->pos;
            }
        }
    }
    
    f32 x_position = view_get_screen_rect(app, view_id).x0 + 4 - view_get_buffer_scroll(app, view_id).position.pixel_shift.x;
    
    Scratch_Block scratch(app);
    Range_i64_Array ranges = get_enclosure_ranges(app, scratch, buffer_id, pos, RangeHighlightKind_CharacterHighlight);
    
    for (i32 range_index = ranges.count - 1; range_index >= 0; range_index -= 1) {
        Range_i64 range = ranges.ranges[range_index];
        
#if 0
        if (range.start >= visible_range.start) {
            continue;
        }
#endif
        
        i64 visual_range_end = get_line_end_pos(app, buffer_id, get_line_number_from_pos(app, buffer_id, range.end));
        // --visual_range_end; // Comment out to don't include the newline character.
        Rect_f32 close_scope_rect = text_layout_character_on_screen(app, text_layout_id, visual_range_end);
        Vec2_f32 close_scope_pos = { close_scope_rect.x0 + face_metrics.normal_advance, close_scope_rect.y0 };
        
        // NOTE(rjf): Find token set before this scope begins.
        Token *start_token = 0;
        i64 token_count = 0;
        {
            Token_Iterator_Array it = token_iterator_pos(0, &token_array, range.start-1);
            int paren_nest = 0;
            int function_keyword_count = 0;
            
            for (;;) {
                Token *token = token_it_read(&it);
                if (!token_it_dec_non_whitespace(&it)) {
                    break;
                }
                if (!token)  break;
                token_count += 1;
                
                if (!start_token) {
                    if (token->kind == TokenBaseKind_ParentheticalClose) {
                        ++paren_nest;
                    }
                    else if (token->kind == TokenBaseKind_ParentheticalOpen) {
                        --paren_nest;
                    }
                    else if (paren_nest <= 0) {
                        if (token->kind == TokenBaseKind_ScopeClose) {
                            break;
                        }
                        else if (token->kind == TokenBaseKind_StatementClose) {
                            if (token->sub_kind == TokenCppKind_Semicolon) {
                                break;
                            }
                        }
                        else if (token->kind == TokenBaseKind_Identifier || token->kind == TokenBaseKind_Keyword || token->kind == TokenBaseKind_Comment) {
                            start_token = token;
                            // break;
                        }
                    }
                }
                else {
                    if (start_token != 0) {
                        if (function_keyword_count >= 2) {
                            break;
                        }
                        if (token->kind == TokenBaseKind_Identifier || token->kind == TokenBaseKind_Keyword) {
                            start_token = token;
                            ++function_keyword_count;
                        }
                        else {
                            break;
                        }
                    }
                }
            }
        }
        if (!start_token)  continue;
        Token *end_token = 0;
        {
            Token_Iterator_Array it = token_iterator_pos(0, &token_array, start_token->pos);
            int paren_nest = 0;
            
            for (;;) {
                Token *token = token_it_read(&it);
                if (!token_it_inc_non_whitespace(&it)) {
                    break;
                }
                
                if (token) {
                    token_count += 1;
                    
                    if (token->kind == TokenBaseKind_ParentheticalClose) {
                        --paren_nest;
                    }
                    else if (token->kind == TokenBaseKind_ParentheticalOpen) {
                        ++paren_nest;
                    }
                    else if (paren_nest == 0 && token->kind == TokenBaseKind_ScopeOpen) {
                        end_token = token;
                        break;
                    }
                }
                else {
                    break;
                }
            }
        }
        if (!end_token)  continue;
        
#if 0
        u32 color = finalize_color(defcolor_comment, 0);
        color &= 0x00ffffff;
        color |= 0x80000000;
        u32 color_identifier = 0xA000FF00;
#else
        u32 color = colors[color_index % color_count];
        ++color_index;
        color &= 0x00ffffff;
        color |= 0x80000000;
        u32 color_identifier = color;
        color_identifier &= 0x00ffffff;
        color_identifier |= 0xC0000000;
#endif
        
        // @note Draw arrow
        String_Const_u8 arrow_string = string_u8_litexpr("<- ");
        draw_string(app, face_id, arrow_string, close_scope_pos, color_identifier);
        close_scope_pos.x += (arrow_string.size * face_metrics.normal_advance);
        String_Const_u8 raw_line = push_buffer_range(app, scratch, buffer_id,
                                                     Ii64(start_token->pos, (end_token->pos + end_token->size)));
        String_Const_u8 start_line = {0};
        start_line.str = push_array(scratch, u8, raw_line.size);
        u8 *src_char = &raw_line.str[0];
        u8 *dest_char = &start_line.str[0];
        int start_line_size = 0;
        int ci = 0;
        b32 first_whitespace = true;
        for (; ci < raw_line.size; ++ci) {
            if (character_is_whitespace(*src_char)) {
                if (first_whitespace) {
                    *dest_char = ' ';
                    ++dest_char;
                    ++start_line_size;
                    first_whitespace = false;
                }
            }
            else {
                first_whitespace = true;
                *dest_char = *src_char;
                ++dest_char;
                ++start_line_size;
            }
            ++src_char;
        }
        start_line.size = start_line_size;
        
        // @note Remove space and { at the end
        start_line.size -= 1;
        if (start_line.str[start_line.size-1] == ' ') {
            start_line.size -= 1;
        }
        
        i64 first_open_paren_index = 0;
        for (int i = 0; i < start_line.size; ++i) {
            if (start_line.str[i] == '(')  first_open_paren_index = i;
        }
        
        // @note Draw string at scope close
        if (close_scope_pos.x > 0.0f && close_scope_pos.y > 0.0f) {
            String_Const_u8 string_identifier = start_line;
            string_identifier.size = first_open_paren_index;
            start_line.str  += first_open_paren_index;
            start_line.size -= first_open_paren_index;
            
            draw_string(app, face_id, string_identifier, close_scope_pos, color_identifier);
            close_scope_pos.x += string_identifier.size * face_metrics.normal_advance;
            draw_string(app, face_id, start_line, close_scope_pos, color);
            
            start_line.str  -= first_open_paren_index;
            start_line.size += first_open_paren_index;
        }
        
        // @note Draw string at scope vertical line
        {
            // @note Prevent the flashing of the string when moving the cursor up past the visible range start.
            i64 visible_range_start_line_number = get_line_number_from_pos(app, buffer_id, visible_range.start);
            i64 scope_range_end_line_number = get_line_number_from_pos(app, buffer_id, range.end);
            if (scope_range_end_line_number <= visible_range_start_line_number)  continue;
            
            i64 visible_range_end_line_number = get_line_number_from_pos(app, buffer_id, visible_range.end);
            i64 scope_range_start_line_number = get_line_number_from_pos(app, buffer_id, range.start);
            if (scope_range_start_line_number > visible_range_end_line_number)  continue;
        }
        b32 is_out_of_bounds = false;
        Rect_f32 start_token_rect = text_layout_character_on_screen(app, text_layout_id, start_token->pos);
        f32 scope_start_y = start_token_rect.y1;
        if (scope_start_y <= 0.0f) {
            scope_start_y = view_rect.y0;
            is_out_of_bounds = true;
        }
        f32 scope_end_y = close_scope_rect.y0;
        // @note View are checking for 0 here, but this also happens when the close brace is at the first line of the view.
        //       This then causes a flashing of the rendered string at the center of the screen,
        //       when moving the cursor up several lines before the first line.
        if (scope_end_y <= 0.0f || scope_end_y > view_rect.y1) {
            scope_end_y = view_rect.y1;
            is_out_of_bounds = true;
        }
        f32 scope_size_y = scope_end_y - scope_start_y;
        if (scope_size_y < face_metrics.normal_advance)  continue;
        
        f32 scope_x = x_position;
        x_position += face_metrics.space_advance * 4;
        Vec2_f32 point = {
            scope_x + face_metrics.line_height, // + face_metrics.descent,
            scope_start_y + ((scope_size_y) * 0.5f)
        };
        if (scope_size_y < (start_line.size * face_metrics.normal_advance)) {
            i64 possible_count = (i64)(scope_size_y / face_metrics.normal_advance);
            start_line.size = possible_count;
        }
        point.y -= (start_line.size*0.5f) * face_metrics.normal_advance;
        
        if (first_open_paren_index > (i64)start_line.size)  first_open_paren_index = start_line.size;
        String_Const_u8 string_identifier = start_line;
        string_identifier.size = first_open_paren_index;
        start_line.str  += first_open_paren_index;
        start_line.size -= first_open_paren_index;
        
        if (!is_out_of_bounds)  continue;
#if 1
        u32 flags = 1; // @note Rotate 90 degree clockwise
        Vec2_f32 delta = {0,1};
#else
        u32 flags = ; // @todo Flag to rotate -90 degree?
        Vec2_f32 delta = {1,0}; // {0,-1};
        point.y += start_line.size * face_metrics.normal_advance;
#endif
        draw_string_oriented(app, face_id, color_identifier, string_identifier, point, flags, delta);
        point.y += string_identifier.size * face_metrics.normal_advance;
        draw_string_oriented(app, face_id, color, start_line, point, flags, delta);
    }
}

//
// @note Draw scope vertical lines
// 
// @note: https://github.com/ryanfleury/4coder_fleury
//        4coder_fleury_brace.cpp
inline void
tebtro_draw_vertical_lines_scope_highlight(Application_Links *app, Buffer_ID buffer_id, View_ID view,
                                           Text_Layout_ID text_layout_id, Rect_f32 view_rect, i64 pos, ARGB_Color *colors, i32 color_count) {
    Face_ID face_id = get_face_id(app, buffer_id);
    Token_Array token_array = get_token_array_from_buffer(app, buffer_id);
    Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
    
    if (token_array.tokens != 0) {
        Token_Iterator_Array it = token_iterator_pos(0, &token_array, pos);
        Token *token = token_it_read(&it);
        if (token != 0 && token->kind == TokenBaseKind_ScopeOpen) {
            pos = token->pos + token->size;
        }
        else {
            if (token_it_dec_all(&it)) {
                token = token_it_read(&it);
                
                if (token->kind == TokenBaseKind_ScopeClose && pos == token->pos + token->size) {
                    pos = token->pos;
                }
            }
        }
    }
    
    Face_Metrics metrics = get_face_metrics(app, face_id);
    
    Scratch_Block scratch(app);
    Range_i64_Array ranges = get_enclosure_ranges(app, scratch, buffer_id, pos, RangeHighlightKind_CharacterHighlight);
    float x_position = view_get_screen_rect(app, view).x0 + 4 - view_get_buffer_scroll(app, view).position.pixel_shift.x;
    
    
    int color_index = 0;
    
    for (i32 i = ranges.count - 1; i >= 0; i -= 1) {
        Range_i64 range = ranges.ranges[i];
        
        Rect_f32 range_start_rect = text_layout_character_on_screen(app, text_layout_id, range.start);
        Rect_f32 range_end_rect = text_layout_character_on_screen(app, text_layout_id, range.end);
        
        float y_start = 0;
        float y_end = view_rect.y1;
        
#if 1 // INCLUDE BRACE LINE
        if (range.start >= visible_range.start) {
            y_start = range_start_rect.y0 + (metrics.line_height * 0.5f);
        }
        if (range.end <= visible_range.end) {
            y_end = range_end_rect.y0 + (metrics.line_height * 0.5f);
        }
#else
        if (range.start >= visible_range.start) {
            y_start = range_start_rect.y0 + metrics.line_height;
        }
        if (range.end <= visible_range.end) {
            y_end = range_end_rect.y0;
        }
#endif
        
        Rect_f32 line_rect = {0};
        line_rect.x0 = x_position;
        line_rect.x1 = x_position+1;
        line_rect.y0 = y_start;
        line_rect.y1 = y_end;
        
#if 0
        u32 color = finalize_color(defcolor_comment, 0);
#else
        u32 color = colors[color_index % color_count];
        ++color_index;
#endif
        color &= 0x00ffffff;
        color |= 0x40000000; // 0x60000000;
        draw_rectangle(app, line_rect, 0.5f, color);
        
        x_position += metrics.space_advance * 4;
    }
}

//
// @note Draw error highlight line annotations
//
// @note: https://github.com/ryanfleury/4coder_fleury
//        4coder_fleury.cpp
// 
// @todo Maybe we don't want the text at the beginning: "error Cxxxx: "
//        
inline void
tebtro_draw_error_annotations(Application_Links *app, Buffer_ID buffer_id, Text_Layout_ID text_layout_id, Face_ID face_id, Buffer_ID jump_buffer_id) {
    Heap *heap = &global_heap;
    Scratch_Block scratch(app);
    Locked_Jump_State jump_state = get_locked_jump_state(app, heap);
    
    // Face_ID face = global_small_code_face;
    Face_Metrics metrics = get_face_metrics(app, face_id);
    
    if(jump_buffer_id != 0 && jump_state.view != 0)
    {
        Managed_Scope buffer_scopes[2] =
        {
            buffer_get_managed_scope(app, jump_buffer_id),
            buffer_get_managed_scope(app, buffer_id),
        };
        
        Managed_Scope comp_scope = get_managed_scope_with_multiple_dependencies(app, buffer_scopes, ArrayCount(buffer_scopes));
        Managed_Object *buffer_markers_object = scope_attachment(app, comp_scope, sticky_jump_marker_handle, Managed_Object);
        
        // NOTE(rjf): Get buffer markers (locations where jumps point at).
        i32 buffer_marker_count = 0;
        Marker *buffer_markers = 0;
        {
            buffer_marker_count = managed_object_get_item_count(app, *buffer_markers_object);
            buffer_markers = push_array(scratch, Marker, buffer_marker_count);
            managed_object_load_data(app, *buffer_markers_object, 0, buffer_marker_count, buffer_markers);
        }
        
        i64 last_line = -1;
        
        for(i32 i = 0; i < buffer_marker_count; i += 1)
        {
            i64 jump_line_number = get_line_from_list(app, jump_state.list, i);
            i64 code_line_number = get_line_number_from_pos(app, buffer_id, buffer_markers[i].pos);
            
            if(code_line_number != last_line)
            {
                
                String_Const_u8 jump_line = push_buffer_line(app, scratch, jump_buffer_id, jump_line_number);
                
                // NOTE(rjf): Remove file part of jump line.
                {
                    u64 index = string_find_first(jump_line, string_u8_litexpr("error"), StringMatch_CaseInsensitive);
                    if(index == jump_line.size)
                    {
                        index = string_find_first(jump_line, string_u8_litexpr("warning"), StringMatch_CaseInsensitive);
                        if(index == jump_line.size)
                        {
                            index = 0;
                        }
                    }
                    jump_line.str += index;
                    jump_line.size -= index;
                }
                
                // NOTE(rjf): Render annotation.
                {
                    Rect_f32 region = text_layout_region(app, text_layout_id);
                    Range_i64 line_range = Ii64(code_line_number);
                    
                    Range_f32 y1 = text_layout_line_on_screen(app, text_layout_id, line_range.min);
                    Range_f32 y2 = text_layout_line_on_screen(app, text_layout_id, line_range.max);
                    Range_f32 y = range_union(y1, y2);
                    
                    i64 line_end_pos = get_line_end_pos(app, buffer_id, code_line_number);
                    Rect_f32 line_end_rect = text_layout_character_on_screen(app, text_layout_id, line_end_pos);
                    f32 x0 = line_end_rect.x1;
                    f32 string_x0 = (region.x1 - metrics.max_advance*jump_line.size - (y.max-y.min)/2 - metrics.line_height/2);
                    if (string_x0 > x0)  x0 = string_x0;
                    
                    if(range_size(y) > 0.f) {
                        draw_string(app, face_id, jump_line,
                                    V2f32(x0,
                                          y.min + (y.max-y.min)/2 - metrics.line_height/2),
                                    0xffff0000);
                    }
                }
            }
            
            last_line = code_line_number;
        }
    }
}

//
// @note Draw comment divider horizontal lines
// 
// @note: https://github.com/ryanfleury/4coder_fleury
//        4coder_fleury_divider_comments.cpp
// 

inline void
tebtro_draw_divider_line_at_pos(Application_Links *app, Face_ID face_id, Rect_f32 view_rect, Text_Layout_ID text_layout_id, i64 pos, b32 y_at_line_top = true, b32 x_at_pos = true) {
    Face_Metrics face_metrics = get_face_metrics(app, face_id);
    f32 line_height = face_metrics.line_height * 0.08f;
    f32 line_offset = line_height;
    
    Rect_f32 comment_first_char_rect = text_layout_character_on_screen(app, text_layout_id, pos);
    Rect_f32 rect = {0};
    // @note LINE START AT COMMENT START
    if (x_at_pos) {
        rect.x0 = comment_first_char_rect.x0;
    }
    else {
        rect.x0 = view_rect.x0;
    }
    rect.x1 = view_rect.x1;
    // @note LINE AT TOP
    if (y_at_line_top) {
        rect.y0 = comment_first_char_rect.y0 + line_offset;
        rect.y1 = comment_first_char_rect.y0 + line_offset - line_height;
    }
    else {
        rect.y0 = comment_first_char_rect.y1 + line_offset;
        rect.y1 = comment_first_char_rect.y1 + line_offset + line_height;
    }
    f32 roundness = 4.f;
    draw_rectangle(app, rect, roundness, fcolor_resolve(fcolor_id(defcolor_comment)));
}

// @note Main section divider comment
inline void
tebtro_draw_main_section_divider_comments(Application_Links *app, View_ID view_id, Face_ID face_id, Buffer_ID buffer_id,
                                          Rect_f32 view_rect, Text_Layout_ID text_layout_id) {
    String_Const_u8 divider_comment_signifier = SCu8("// @note");
    
    Face_Metrics face_metrics = get_face_metrics(app, face_id);
    f32 line_height = face_metrics.line_height * 0.08f;
    f32 line_offset = line_height;
    
    Token_Array token_array = get_token_array_from_buffer(app, buffer_id);
    Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
    Scratch_Block scratch(app);
    
    if (token_array.tokens == 0)  return;
    i64 first_index = token_index_from_pos(&token_array, visible_range.first);
    Token_Iterator_Array it = token_iterator_index(0, &token_array, first_index);
    
    Token *token = 0;
    for (;;) {
        token = token_it_read(&it);
        if (token->pos >= visible_range.one_past_last || !token || !token_it_inc_non_whitespace(&it)) {
            break;
        }
        if (token->kind != TokenBaseKind_Comment)  continue;
        if (token->size < (i64)divider_comment_signifier.size)  continue;
        
        String_Const_u8 token_string = push_buffer_range(app, scratch, buffer_id,
                                                         Ii64_size(token->pos, divider_comment_signifier.size));
        
        if (string_match(token_string, divider_comment_signifier)) {
            i64 line_number = get_line_number_from_pos(app, buffer_id, token->pos);
            // @note Check if line before main section is a blank/empty line
            
            if (!line_is_blank(app, buffer_id, line_number - 2)) {
                continue;
            }
            
#if 0
            i64 prev_line_start_pos = get_line_start_pos(app, buffer_id, line_number - 1);
            i64 next_line_start_pos = get_line_start_pos(app, buffer_id, line_number + 1);
            i64 prev_index = token_index_from_pos(&token_array, prev_line_start_pos);
            i64 next_index = token_index_from_pos(&token_array, next_line_start_pos);
#else
            i64 prev_text_start_pos = get_pos_past_lead_whitespace_from_line_number(app, buffer_id, line_number - 1);
            i64 next_text_start_pos = get_pos_past_lead_whitespace_from_line_number(app, buffer_id, line_number + 1);
            // if (!c_line_comment_starts_at_position(app, buffer_id, prev_text_start_pos))  continue;
            // if (!c_line_comment_starts_at_position(app, buffer_id, next_text_start_pos))  continue;
            
            i64 prev_index = token_index_from_pos(&token_array, prev_text_start_pos);
            i64 next_index = token_index_from_pos(&token_array, next_text_start_pos);
#endif
            
            
            Token_Iterator_Array prev_it = token_iterator_index(0, &token_array, prev_index);
            Token_Iterator_Array next_it = token_iterator_index(0, &token_array, next_index);
            Token *prev_token = token_it_read(&prev_it);
            Token *next_token = token_it_read(&next_it);
            if (prev_token->kind != TokenBaseKind_Comment)  continue;
            if (next_token->kind != TokenBaseKind_Comment)  continue;
            if (prev_token->size > 3)  continue;
            if (next_token->size > 3)  continue;
            
            // @note Draw divider line
            tebtro_draw_divider_line_at_pos(app, face_id, view_rect, text_layout_id, prev_token->pos);
        }
        
    }
}

// @note Simple divider comment
inline void
tebtro_draw_divider_comments(Application_Links *app, View_ID view_id, Face_ID face_id, Buffer_ID buffer_id,
                             Rect_f32 view_rect, Text_Layout_ID text_layout_id) {
    String_Const_u8 divider_comment_signifier = SCu8("//~");
    
    Token_Array token_array = get_token_array_from_buffer(app, buffer_id);
    Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
    Scratch_Block scratch(app);
    
    if (token_array.tokens == 0)  return;
    i64 first_index = token_index_from_pos(&token_array, visible_range.first);
    Token_Iterator_Array it = token_iterator_index(0, &token_array, first_index);
    
    Token *token = 0;
    for (;;) {
        token = token_it_read(&it);
        if (token->pos >= visible_range.one_past_last || !token || !token_it_inc_non_whitespace(&it)) {
            break;
        }
        if (token->kind != TokenBaseKind_Comment)  continue;
        if (token->size < (i64)divider_comment_signifier.size)  continue;
        
        String_Const_u8 token_string = push_buffer_range(app, scratch, buffer_id,
                                                         Ii64_size(token->pos, divider_comment_signifier.size));
        
        if (string_match(token_string, divider_comment_signifier)) {
            // @note Draw divider line
            tebtro_draw_divider_line_at_pos(app, face_id, view_rect, text_layout_id, token->pos);
        }
    }
}


