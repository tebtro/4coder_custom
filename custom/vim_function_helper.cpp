//
// @note Function parameter helper
//
// @note: https://github.com/ryanfleury/4coder_fleury
//        4coder_fleury.cpp
//


// @todo @cleanup
// @todo Find and render several possible overloaded functions.

static void
Fleury4RenderRangeHighlight(Application_Links *app, View_ID view_id, Text_Layout_ID text_layout_id,
                            Range_i64 range)
{
    Rect_f32 range_start_rect = text_layout_character_on_screen(app, text_layout_id, range.start);
    Rect_f32 range_end_rect = text_layout_character_on_screen(app, text_layout_id, range.end-1);
    Rect_f32 total_range_rect = {0};
    total_range_rect.x0 = Min(range_start_rect.x0, range_end_rect.x0);
    total_range_rect.y0 = Min(range_start_rect.y0, range_end_rect.y0);
    total_range_rect.x1 = Max(range_start_rect.x1, range_end_rect.x1);
    total_range_rect.y1 = Max(range_start_rect.y1, range_end_rect.y1);
    
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
    draw_rectangle(app, total_range_rect, 4.f, highlight_color);
}

static void
Fleury4DrawTooltipRect(Application_Links *app, Rect_f32 rect)
{
#if 0
    ARGB_Color background_color = fcolor_resolve(fcolor_id(defcolor_back));
    ARGB_Color border_color = fcolor_resolve(fcolor_id(defcolor_margin_active));
#else
    ARGB_Color background_color = fcolor_resolve(fcolor_id(defcolor_back));
    ARGB_Color border_color = 0xFF5E5B05; // fcolor_resolve(fcolor_id(defcolor_list_item_active));
#endif
    
    background_color &= 0x00ffffff;
    background_color |= 0xd0000000;
    
    border_color &= 0x00ffffff;
    border_color |= 0xd0000000;
    
    draw_rectangle(app, rect, 4.f, background_color);
    draw_rectangle_outline(app, rect, 4.f, 3.f, border_color);
}



static Range_i64_Array
Fleury4GetLeftParens(Application_Links *app, Arena *arena, Buffer_ID buffer, i64 pos, u32 flags)
{
    Range_i64_Array array = {};
    i32 max = 100;
    array.ranges = push_array(arena, Range_i64, max);
    
    for(;;)
    {
        Range_i64 range = {};
        range.max = pos;
        if(find_nest_side(app, buffer, pos - 1, flags | FindNest_Balanced,
                          Scan_Backward, NestDelim_Open, &range.start))
        {
            array.ranges[array.count] = range;
            array.count += 1;
            pos = range.first;
            if (array.count >= max)
            {
                break;
            }
        }
        else
        {
            break;
        }
    }
    return(array);
}

static String_Const_u8
Fleury4CopyStringButOnlyAllowOneSpace(Arena *arena, String_Const_u8 string)
{
    String_Const_u8 result = {0};
    
    u64 space_to_allocate = 0;
    u64 spaces_left_this_gap = 1;
    
    for(u64 i = 0; i < string.size; ++i)
    {
        if(string.str[i] <= 32)
        {
            if(spaces_left_this_gap > 0)
            {
                --spaces_left_this_gap;
                ++space_to_allocate;
            }
        }
        else
        {
            spaces_left_this_gap = 1;
            ++space_to_allocate;
        }
    }
    
    result.data = push_array(arena, u8, space_to_allocate);
    for(u64 i = 0; i < string.size; ++i)
    {
        if(string.str[i] <= 32)
        {
            if(spaces_left_this_gap > 0)
            {
                --spaces_left_this_gap;
                result.str[result.size++] = string.str[i];
            }
        }
        else
        {
            spaces_left_this_gap = 1;
            result.str[result.size++] = string.str[i];
        }
    }
    
    return result;
}

static void
Fleury4RenderFunctionHelper(Application_Links *app, View_ID view_id, Buffer_ID buffer_id, Text_Layout_ID text_layout_id, i64 pos) {
    ProfileScope(app, "Fleury4RenderFunctionHelper");
    
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
        
        Fleury4RenderRangeHighlight(app, view_id, text_layout_id, function_name_range);
        
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
        b32 active_parameter_has_increased_by_one = active_parameter_index == last_active_parameter + 1;
        last_active_parameter = active_parameter_index;
        
        for(Buffer_ID buffer_it = get_buffer_next(app, 0, Access_Always);
            buffer_it != 0; buffer_it = get_buffer_next(app, buffer_it, Access_Always)) {
            Code_Index_File *file = code_index_get_file(buffer_it);
            if(file == 0)  continue;
            
            for(i32 i = 0; i < file->note_array.count; i += 1) {
                Code_Index_Note *note = file->note_array.ptrs[i];
                
                if((note->note_kind != CodeIndexNote_Function &&
                    note->note_kind != CodeIndexNote_Macro) ||
                   !string_match(note->text, function_name))  continue;
                
                Range_i64 function_def_range;
                function_def_range.min = note->pos.min;
                function_def_range.max = note->pos.max;
                
                Range_i64 highlight_parameter_range = {0};
                
                Token_Array find_token_array = get_token_array_from_buffer(app, buffer_it);
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
                
                if(highlight_parameter_range.min > function_def_range.min &&
                   function_def_range.max > highlight_parameter_range.max) {
                    
                    String_Const_u8 function_def = push_buffer_range(app, scratch, buffer_it, function_def_range);
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
                    
#if 0
                    Rect_f32 helper_rect = {
                        global_cursor_position.x + 16,
                        global_cursor_position.y + 16,
                        global_cursor_position.x + 16,
                        global_cursor_position.y + metrics.line_height + 26,
                    };
#else
                    i64 cursor_pos = view_get_cursor_pos(app, view_id);
                    Rect_f32 cursor_rect = text_layout_character_on_screen(app, text_layout_id, cursor_pos);
                    Rect_f32 function_name_rect = text_layout_character_on_screen(app, text_layout_id, function_name_range.start);
                    Rect_f32 target_rect = function_name_rect;
#if 1
                    if (cursor_rect.y1 > function_name_rect.y1) {
                        // target_rect = cursor_rect;
                        target_rect.y0 = cursor_rect.y0;
                        target_rect.y1 = cursor_rect.y1;
                    }
                    // target_rect.y0 += metrics.line_height;
                    // target_rect.y1 += metrics.line_height;
                    
                    {
                        Range_i64 paren_range = {};
                        if (find_nest_side(app, buffer_id, ranges.ranges[range_index].min+1, FindNest_Paren|FindNest_Balanced|FindNest_EndOfToken,
                                           Scan_Forward, NestDelim_Close, &paren_range.end)) {
                            Rect_f32 paren_end_rect = text_layout_character_on_screen(app, text_layout_id, paren_range.end-1);
                            if (paren_end_rect.y1 > cursor_rect.y1) {
                                target_rect.y0 = paren_end_rect.y0;
                                target_rect.y1 = paren_end_rect.y1;
                            }
                        }
                    }
#else
                    target_rect.y0 -= 2.0f*metrics.line_height;
                    target_rect.y1 -= 2.0f*metrics.line_height;
#endif
                    Rect_f32 helper_rect = {
                        target_rect.x0,
                        target_rect.y1,
                        target_rect.x0,
                        target_rect.y1 + metrics.line_height,
                    };
#endif
                    
                    f32 padding = (helper_rect.y1 - helper_rect.y0)/2 -
                        metrics.line_height/2;
                    
                    // NOTE(rjf): Size helper rect by how much text to draw.
                    {
                        helper_rect.x1 += get_string_advance(app, face, highlight_param);
                        helper_rect.x1 += get_string_advance(app, face, pre_highlight_def);
                        helper_rect.x1 += get_string_advance(app, face, post_highlight_def);
                        helper_rect.x1 += 2 * padding;
                    }
                    
                    if(helper_rect.x1 > view_get_screen_rect(app, view_id).x1) {
                        f32 difference = helper_rect.x1 - view_get_screen_rect(app, view_id).x1;
                        helper_rect.x0 -= difference;
                        helper_rect.x1 -= difference;
                    }
                    
#if 1
                    Vec2_f32 text_position = {
                        helper_rect.x0 + padding,
                        helper_rect.y0 + padding,
                    };
                    
                    Fleury4DrawTooltipRect(app, helper_rect);
                    
                    text_position = draw_string(app, face, pre_highlight_def, text_position, finalize_color(defcolor_comment, 0));
                    
                    text_position = draw_string(app, face, highlight_param, text_position, finalize_color(defcolor_comment_pop, 1));
                    text_position = draw_string(app, face, post_highlight_def, text_position, finalize_color(defcolor_comment, 0));
#else
                    Vec2_f32 text_position = {
                        helper_rect.x0,
                        helper_rect.y0,
                    };
                    
                    Fancy_Block block = {};
                    Fancy_Line *line = push_fancy_line(scratch, &block, face);
                    push_fancy_string(scratch, line, fcolor_id(defcolor_comment,     0), pre_highlight_def);
                    push_fancy_string(scratch, line, fcolor_id(defcolor_comment_pop, 1), highlight_param);
                    push_fancy_string(scratch, line, fcolor_id(defcolor_comment,     0), post_highlight_def);
                    Rect_f32 region = view_get_buffer_region(app, view);
                    draw_drop_down(app, face, &block, text_position, region, 1.0f, 0.5f, fcolor_id(defcolor_list_item_active), fcolor_id(defcolor_back));
#endif
                    
                    goto end_lookup;
                }
            }
        }
        end_lookup:;
        break;
    }
}
