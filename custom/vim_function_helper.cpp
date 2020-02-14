//
// @note Draw highlight range
//

function void
vim_draw_highlight_range(Application_Links *app, View_ID view_id, Text_Layout_ID text_layout_id,
                         Range_i64 range, ARGB_Color background_color, ARGB_Color foreground_color, f32 roundness) {
    if (background_color != 0) {
        draw_character_block(app, text_layout_id, range, roundness, background_color);
    }
    if (foreground_color != 0) {
        paint_text_color(app, text_layout_id, range, foreground_color);
    }
}

function void
vim_draw_highlight_range(Application_Links *app, View_ID view_id, Text_Layout_ID text_layout_id,
                         Range_i64 range) {
    Face_ID face_id = get_face_id(app, 0);
    Face_Metrics face_metrics = get_face_metrics(app, face_id);
    f32 roundness = (face_metrics.normal_advance*0.5f)*0.9f;
    
    ARGB_Color background_color = fcolor_resolve(fcolor_id(defcolor_pop2));
    float background_color_r = (float)((background_color & 0x00ff0000) >> 16) / 255.f;
    float background_color_g = (float)((background_color & 0x0000ff00) >>  8) / 255.f;
    float background_color_b = (float)((background_color & 0x000000ff) >>  0) / 255.f;
    background_color_r += (1.f - background_color_r) * 0.5f;
    background_color_g += (1.f - background_color_g) * 0.5f;
    background_color_b += (1.f - background_color_b) * 0.5f;
    ARGB_Color highlight_color = (0x55000000 |
                                  (((u32)(background_color_r * 255.f)) << 16) |
                                  (((u32)(background_color_g * 255.f)) <<  8) |
                                  (((u32)(background_color_b * 255.f)) <<  0));
    vim_draw_highlight_range(app, view_id, text_layout_id, range, highlight_color, 0, roundness);
}


//
// @note Function parameter helper
//
// @note: https://github.com/ryanfleury/4coder_fleury
//        4coder_fleury.cpp
//
// @note(tebtro): - Shows overloaded functions.
//                - Better positioning where it is rendered.

static Range_i64_Array
Fleury4GetLeftParens(Application_Links *app, Arena *arena, Buffer_ID buffer, i64 pos, u32 flags) {
    Range_i64_Array array = {};
    i32 max = 100;
    array.ranges = push_array(arena, Range_i64, max);
    
    for(;;) {
        Range_i64 range = {};
        range.max = pos;
        if(find_nest_side(app, buffer, pos - 1, flags | FindNest_Balanced, Scan_Backward, NestDelim_Open, &range.start)) {
            array.ranges[array.count] = range;
            array.count += 1;
            pos = range.first;
            if (array.count >= max) {
                break;
            }
        }
        else {
            break;
        }
    }
    return array;
}

static String_Const_u8
Fleury4CopyStringButOnlyAllowOneSpace(Arena *arena, String_Const_u8 string) {
    String_Const_u8 result = {0};
    
    u64 space_to_allocate = 0;
    u64 spaces_left_this_gap = 1;
    
    for(u64 i = 0; i < string.size; ++i) {
        if(string.str[i] <= 32) {
            if(spaces_left_this_gap > 0) {
                --spaces_left_this_gap;
                ++space_to_allocate;
            }
        }
        else {
            spaces_left_this_gap = 1;
            ++space_to_allocate;
        }
    }
    
    result.data = push_array(arena, u8, space_to_allocate);
    for(u64 i = 0; i < string.size; ++i) {
        if(string.str[i] <= 32) {
            if(spaces_left_this_gap > 0) {
                --spaces_left_this_gap;
                result.str[result.size++] = string.str[i];
            }
        }
        else {
            spaces_left_this_gap = 1;
            result.str[result.size++] = string.str[i];
        }
    }
    
    return result;
}

static void
vim_render_function_helper(Application_Links *app, View_ID view_id, Buffer_ID buffer_id, Text_Layout_ID text_layout_id, i64 pos) {
    ProfileScope(app, "vim_render_function_helper");
    
    Token_Array token_array = get_token_array_from_buffer(app, buffer_id);
    if (token_array.tokens == 0)  return;
    Token_Iterator_Array it = token_iterator_pos(0, &token_array, pos);
    Token *token = token_it_read(&it);
    
    // @note(tebtro): Clamp/Make sure the cursor pos is inside the paren range and not one past last or something?
    if(token != 0 && token->kind == TokenBaseKind_ParentheticalOpen) {
        pos = token->pos + token->size;
    }
    else {
        if (token_it_dec_all(&it)) {
            token = token_it_read(&it);
            if (token->kind == TokenBaseKind_ParentheticalClose &&
                pos == token->pos + token->size)
            {
                pos = token->pos;
            }
        }
    }
    
    
    Rect_f32 view_rect = view_get_screen_rect(app, view_id);
#if 0
    Face_ID face = global_small_code_face;
#else
    Face_ID face = get_face_id(app, 0);
#endif
    Face_Metrics metrics = get_face_metrics(app, face);
    
    
    Scratch_Block scratch(app);
    Range_i64_Array ranges = Fleury4GetLeftParens(app, scratch, buffer_id, pos, FindNest_Paren);
    
    for(int range_index = 0; range_index < ranges.count; ++range_index) {
        it = token_iterator_pos(0, &token_array, ranges.ranges[range_index].min);
        token_it_dec_non_whitespace(&it);
        token = token_it_read(&it);
        if(token->kind != TokenBaseKind_Identifier)  continue;
        
        Range_i64 function_name_range = Ii64(token->pos, token->pos+token->size);
        String_Const_u8 function_name = push_buffer_range(app, scratch, buffer_id, function_name_range);
        
        vim_draw_highlight_range(app, view_id, text_layout_id, function_name_range);
        
        // NOTE(rjf): Find active parameter.
        int active_parameter_index = 0;
        local_persist int last_active_parameter = -1;
        {
            it = token_iterator_pos(0, &token_array, function_name_range.min);
            int paren_nest = 0;
            for(;token_it_inc_non_whitespace(&it);) {
                token = token_it_read(&it);
                if(token->pos + token->size > pos) {
                    break;
                }
                
                if(token->kind == TokenBaseKind_ParentheticalOpen) {
                    ++paren_nest;
                }
                else if(token->kind == TokenBaseKind_StatementClose) {
                    if(paren_nest == 1) {
                        ++active_parameter_index;
                    }
                }
                else if(token->kind == TokenBaseKind_ParentheticalClose) {
                    if(!--paren_nest) {
                        break;
                    }
                }
            }
        }
        b32 active_parameter_has_increased_by_one = (active_parameter_index == last_active_parameter + 1);
        last_active_parameter = active_parameter_index;
        
        Fancy_Block block = {};
        u64 longest_return_type_size = 0;
        
        for(Buffer_ID buffer_it = get_buffer_next(app, 0, Access_Always);
            buffer_it != 0; buffer_it = get_buffer_next(app, buffer_it, Access_Always)) {
            Code_Index_File *file = code_index_get_file(buffer_it);
            if(file == 0)  continue;
            
            Token_Array find_token_array = get_token_array_from_buffer(app, buffer_it);
            for(i32 i = 0; i < file->note_array.count; i += 1) {
                Code_Index_Note *note = file->note_array.ptrs[i];
                
                if((note->note_kind != CodeIndexNote_Function &&
                    note->note_kind != CodeIndexNote_Macro) ||
                   !string_match(note->text, function_name))  continue;
                
                Range_i64 function_def_range;
                function_def_range.min = note->pos.min;
                function_def_range.max = note->pos.max;
                
                Range_i64 highlight_parameter_range = {0};
                
                it = token_iterator_pos(0, &find_token_array, function_def_range.min);
                
                int paren_nest = 0;
                int param_index = 0;
                for(;token_it_inc_non_whitespace(&it);) {
                    token = token_it_read(&it);
                    if(token->kind == TokenBaseKind_ParentheticalOpen) {
                        if(++paren_nest == 1) {
                            if(active_parameter_index == param_index) {
                                highlight_parameter_range.min = token->pos+1;
                            }
                        }
                    }
                    else if(token->kind == TokenBaseKind_ParentheticalClose) {
                        if(!--paren_nest) {
                            function_def_range.max = token->pos + token->size;
                            if(param_index == active_parameter_index) {
                                highlight_parameter_range.max = token->pos;
                            }
                            break;
                        }
                    }
                    else if(token->kind == TokenBaseKind_StatementClose) {
                        if(param_index == active_parameter_index) {
                            highlight_parameter_range.max = token->pos;
                        }
                        
                        if(paren_nest == 1) {
                            ++param_index;
                        }
                        
                        if(param_index == active_parameter_index) {
                            highlight_parameter_range.min = token->pos+1;
                        }
                    }
                }
                
                // @note Search backward for return type
                // String_Const_u8 return_type_string;
                it = token_iterator_pos(0, &find_token_array, function_def_range.min);
                if (token_it_dec_non_whitespace(&it)) {
                    token = token_it_read(&it);
                    if (token && (token->kind == TokenBaseKind_Keyword ||
                                  token->kind == TokenBaseKind_Identifier)) {
                        if ((u64)token->size+1 > longest_return_type_size) {
                            longest_return_type_size = token->size+1;
                        }
                        function_def_range.min = token->pos;
                        // return_type_string = push_token_lexeme(app, scratch, buffer_id, token);
                    }
                }
                
                if(highlight_parameter_range.min > function_def_range.min &&
                   function_def_range.max > highlight_parameter_range.max) {
                    
#if 1
                    String_Const_u8 function_def = push_buffer_range(app, scratch, buffer_it, function_def_range);
#else
                    // Memory is wrong or something. With string_copy it is better, but parens and comas are still wrong.
                    // String_Const_u8 function_def = push_string_copy(scratch, note->text);
#endif
                    for (int c = 0; c < function_def.size; ++c) {
                        if (function_def.str[c] == '\n') {
                            function_def.str[c] = ' ';
                        }
                    }
                    
                    String_Const_u8 highlight_param_untrimmed = push_buffer_range(app, scratch, buffer_it, highlight_parameter_range);
                    
                    String_Const_u8 pre_highlight_def_untrimmed = {
                        function_def.str,
                        (u64)(highlight_parameter_range.min - function_def_range.min),
                    };
                    
                    String_Const_u8 post_highlight_def_untrimmed = {
                        function_def.str + highlight_parameter_range.max - function_def_range.min,
                        (u64)(function_def_range.max - highlight_parameter_range.max),
                    };
                    
                    String_Const_u8 highlight_param = Fleury4CopyStringButOnlyAllowOneSpace(scratch, highlight_param_untrimmed);
                    String_Const_u8 pre_highlight_def = Fleury4CopyStringButOnlyAllowOneSpace(scratch, pre_highlight_def_untrimmed);
                    String_Const_u8 post_highlight_def = Fleury4CopyStringButOnlyAllowOneSpace(scratch, post_highlight_def_untrimmed);
                    
                    Fancy_Line *line = push_fancy_line(scratch, &block, face);
                    push_fancy_string(scratch, line, fcolor_id(defcolor_comment,     0), pre_highlight_def);
                    push_fancy_string(scratch, line, fcolor_id(defcolor_comment_pop, 1), highlight_param);
                    push_fancy_string(scratch, line, fcolor_id(defcolor_comment,     0), post_highlight_def);
                }
            }
        }
        
        //
        // @note Position and render the fancy string block
        //
        Rect_f32 region = view_get_buffer_region(app, view_id);
        Buffer_Scroll scroll = view_get_buffer_scroll(app, view_id);
        Buffer_Point buffer_point = scroll.position;
        i64 cursor_pos = view_get_cursor_pos(app, view_id);
        Vec2_f32 cursor_point = view_relative_xy_of_pos(app, view_id, buffer_point.line_number, cursor_pos);
        Vec2_f32 function_name_point = view_relative_xy_of_pos(app, view_id, buffer_point.line_number, function_name_range.start);
        Vec2_f32 target_point = function_name_point;
        if (cursor_point.y > function_name_point.y) {
            target_point.y = cursor_point.y;
        }
        {
            Range_i64 paren_range = {};
            if (find_nest_side(app, buffer_id, ranges.ranges[range_index].min+1, FindNest_Paren|FindNest_Balanced|FindNest_EndOfToken, Scan_Forward, NestDelim_Close, &paren_range.end)) {
                Vec2_f32 paren_end_point = view_relative_xy_of_pos(app, view_id, buffer_point.line_number, paren_range.end-1);
                if (paren_end_point.y > cursor_point.y) {
                    target_point.y = paren_end_point.y;
                }
            }
        }
        Vec2_f32 text_position = {
            target_point.x - (longest_return_type_size * metrics.normal_advance),
            target_point.y,
        };
        text_position -= buffer_point.pixel_shift;
        text_position += region.p0;
        
        // Face_Metrics metrics = get_face_metrics(app, face);
        f32 x_padding = metrics.normal_advance;
        f32 x_half_padding = 0.5f*x_padding;
        draw_drop_down(app, face, &block, text_position, region, x_padding, x_half_padding, fcolor_id(defcolor_list_item_active), fcolor_id(defcolor_back));
        
        break;
    }
}
