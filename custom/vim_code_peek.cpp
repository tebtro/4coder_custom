//
// @note: Code Peek
//
// @note: https://github.com/ryanfleury/4coder_fleury
//        4coder_fleury_code_peek.cpp
//
static void Fleury4OpenCodePeek(Application_Links *app, String_Const_u8 base_needle, String_Match_Flag must_have_flags, String_Match_Flag must_not_have_flags);
static void Fleury4CloseCodePeek(void);
static void Fleury4NextCodePeek(void);
static void Fleury4CodePeekGo(Application_Links *app);

struct CodePeekMatch
{
    Tiny_Jump jump;
    Code_Index_Note_Kind kind;
};

static Rect_f32 global_last_cursor_rect;

static b32 global_code_peek_open = 0;
static int global_code_peek_match_count = 0;
static CodePeekMatch global_code_peek_matches[16] = {0};
static int global_code_peek_selected_index = -1;
static f32 global_code_peek_open_transition = 0.f;
static Buffer_ID global_code_peek_token_buffer;
static Range_i64 global_code_peek_token_range;

static String_Const_u8_Array
Fleury4MakeTypeSearchList(Application_Links *app, Arena *arena, String_Const_u8 base_needle)
{
    String_Const_u8_Array result = {0};
    if(base_needle.size > 0)
    {
        result.count = 9;
        result.vals = push_array(arena, String_Const_u8, result.count);
        i32 i = 0;
        result.vals[i++] = (push_u8_stringf(arena, "struct %.*s{"  , string_expand(base_needle)));
        result.vals[i++] = (push_u8_stringf(arena, "struct %.*s\n{", string_expand(base_needle)));
        result.vals[i++] = (push_u8_stringf(arena, "struct %.*s {" , string_expand(base_needle)));
        result.vals[i++] = (push_u8_stringf(arena, "union %.*s{"   , string_expand(base_needle)));
        result.vals[i++] = (push_u8_stringf(arena, "union %.*s\n{" , string_expand(base_needle)));
        result.vals[i++] = (push_u8_stringf(arena, "union %.*s {"  , string_expand(base_needle)));
        result.vals[i++] = (push_u8_stringf(arena, "enum %.*s{"    , string_expand(base_needle)));
        result.vals[i++] = (push_u8_stringf(arena, "enum %.*s\n{"  , string_expand(base_needle)));
        result.vals[i++] = (push_u8_stringf(arena, "enum %.*s {"   , string_expand(base_needle)));
        Assert(i == result.count);
    }
    return(result);
}


static void
Fleury4OpenCodePeek(Application_Links *app, String_Const_u8 base_needle,
                    String_Match_Flag must_have_flags, String_Match_Flag must_not_have_flags)
{
    Scratch_Block scratch(app);
    
    global_code_peek_match_count = 0;
    global_code_peek_open_transition = 0.f;
    
    // NOTE(rjf): Add matches from definitions.
    {
        code_index_lock();
        
        for(Buffer_ID buffer = get_buffer_next(app, 0, Access_Always);
            buffer != 0; buffer = get_buffer_next(app, buffer, Access_Always))
        {
            Code_Index_File *file = code_index_get_file(buffer);
            if(file != 0)
            {
                for(i32 i = 0; i < file->note_array.count; i += 1)
                {
                    Code_Index_Note *note = file->note_array.ptrs[i];
                    
                    if(string_match(note->text, base_needle))
                    {
                        Tiny_Jump jump = {0};
                        jump.buffer = buffer;
                        jump.pos = note->pos.first;
                        
                        global_code_peek_matches[global_code_peek_match_count].jump = jump;
                        global_code_peek_matches[global_code_peek_match_count].kind = note->note_kind;
                        ++global_code_peek_match_count;
                        
                        if(global_code_peek_match_count >= sizeof(global_code_peek_matches)/sizeof(global_code_peek_matches[0]))
                        {
                            goto end_definition_lookup;
                        }
                    }
                }
            }
        }
        
        end_definition_lookup:;
        
        code_index_unlock();
    }
    
    // NOTE(rjf): Do a regular search if this isn't a definition.
    if(global_code_peek_match_count == 0)
    {
        
        String_Const_u8_Array type_array = Fleury4MakeTypeSearchList(app, scratch, base_needle);
        String_Match_List matches = find_all_matches_all_buffers(app, scratch, type_array, must_have_flags, must_not_have_flags);
        string_match_list_filter_remove_buffer_predicate(app, &matches, buffer_has_name_with_star);
        
        for(String_Match *match = matches.first; match; match = match->next)
        {
            Tiny_Jump jump = {0};
            jump.buffer = match->buffer;
            jump.pos = match->range.first;
            
            global_code_peek_matches[global_code_peek_match_count].jump = jump;
            global_code_peek_matches[global_code_peek_match_count].kind = -1;
            ++global_code_peek_match_count;
            
            if(global_code_peek_match_count >= sizeof(global_code_peek_matches)/sizeof(global_code_peek_matches[0]))
            {
                break;
            }
        }
        
        if(global_code_peek_match_count == 0)
        {
            matches = find_all_matches_all_buffers(app, scratch, base_needle, must_have_flags, must_not_have_flags);
            
            for(String_Match *match = matches.first; match; match = match->next)
            {
                Tiny_Jump jump = {0};
                jump.buffer = match->buffer;
                jump.pos = match->range.first;
                
                global_code_peek_matches[global_code_peek_match_count].jump = jump;
                global_code_peek_matches[global_code_peek_match_count].kind = -1;
                ++global_code_peek_match_count;
                
                if(global_code_peek_match_count >= sizeof(global_code_peek_matches)/sizeof(global_code_peek_matches[0]))
                {
                    break;
                }
            }
        }
        
    }
    
    if(global_code_peek_match_count > 0)
    {
        global_code_peek_selected_index = 0;
        global_code_peek_open = 1;
    }
    else
    {
        global_code_peek_selected_index = -1;
        global_code_peek_open = 0;
    }
}

static void
Fleury4CloseCodePeek(void)
{
    global_code_peek_open = 0;
}

static void
Fleury4NextCodePeek(void)
{
    if(++global_code_peek_selected_index >= global_code_peek_match_count)
    {
        global_code_peek_selected_index = 0;
    }
    
    if(global_code_peek_selected_index >= global_code_peek_match_count)
    {
        global_code_peek_selected_index = -1;
        global_code_peek_open = 0;
    }
}

static void
Fleury4CodePeekGo(Application_Links *app, b32 same_panel)
{
    if(global_code_peek_selected_index >= 0 && global_code_peek_selected_index < global_code_peek_match_count &&
       global_code_peek_match_count > 0)
    {
        View_ID view = get_active_view(app, Access_Always);
        CodePeekMatch *match = &global_code_peek_matches[global_code_peek_selected_index];
        
        if(!same_panel)
        {
            view = get_next_view_looped_primary_panels(app, view, Access_Always);
        }
        
        view_set_buffer(app, view, match->jump.buffer, 0);
        i64 line_number = get_line_number_from_pos(app, match->jump.buffer, match->jump.pos);
        Buffer_Scroll scroll = view_get_buffer_scroll(app, view);
        scroll.position.line_number = scroll.target.line_number = line_number;
        view_set_buffer_scroll(app, view, scroll, SetBufferScroll_SnapCursorIntoView);
        Fleury4CloseCodePeek();
    }
}

static ARGB_Color
ARGBFromID(Managed_ID id)
{
    return fcolor_resolve(fcolor_id(id));
}
static ARGB_Color
Fleury4GetCTokenColor(Token token)
{
    ARGB_Color color = ARGBFromID(defcolor_text_default);
    
    switch(token.kind)
    {
        case TokenBaseKind_Preprocessor:     color = ARGBFromID(defcolor_preproc); break;
        case TokenBaseKind_Keyword:          color = ARGBFromID(defcolor_keyword); break;
        case TokenBaseKind_Comment:          color = ARGBFromID(defcolor_comment); break;
        case TokenBaseKind_LiteralString:    color = ARGBFromID(defcolor_str_constant); break;
        case TokenBaseKind_LiteralInteger:   color = ARGBFromID(defcolor_int_constant); break;
        case TokenBaseKind_LiteralFloat:     color = ARGBFromID(defcolor_float_constant); break;
        case TokenBaseKind_Operator:         color = ARGBFromID(defcolor_preproc); break;
        
        case TokenBaseKind_ScopeOpen:
        case TokenBaseKind_ScopeClose:
        case TokenBaseKind_ParentheticalOpen:
        case TokenBaseKind_ParentheticalClose:
        case TokenBaseKind_StatementClose:
        {
            u32 r = (color & 0x00ff0000) >> 16;
            u32 g = (color & 0x0000ff00) >>  8;
            u32 b = (color & 0x000000ff) >>  0;
            
            if(true) // global_dark_mode)
            {
                r = (r * 3) / 5;
                g = (g * 3) / 5;
                b = (b * 3) / 5;
            }
            else
            {
                r = (r * 4) / 3;
                g = (g * 4) / 3;
                b = (b * 4) / 3;
            }
            
            color = 0xff000000 | (r << 16) | (g << 8) | (b << 0);
            
            break;
        }
        
        default:
        {
            switch(token.sub_kind)
            {
                case TokenCppKind_LiteralTrue:
                case TokenCppKind_LiteralFalse:
                {
                    color = ARGBFromID(defcolor_bool_constant);
                    break;
                }
                case TokenCppKind_LiteralCharacter:
                case TokenCppKind_LiteralCharacterWide:
                case TokenCppKind_LiteralCharacterUTF8:
                case TokenCppKind_LiteralCharacterUTF16:
                case TokenCppKind_LiteralCharacterUTF32:
                {
                    color = ARGBFromID(defcolor_char_constant);
                    break;
                }
                case TokenCppKind_PPIncludeFile:
                {
                    color = ARGBFromID(defcolor_include);
                    break;
                }
            }
            break;
        }
    }
    
    return color;
}

static void
Fleury4DrawCTokenColors(Application_Links *app, Text_Layout_ID text_layout_id, Token_Array *array)
{
    Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
    i64 first_index = token_index_from_pos(array, visible_range.first);
    Token_Iterator_Array it = token_iterator_index(0, array, first_index);
    
    for(;;)
    {
        Token *token = token_it_read(&it);
        if(!token || token->pos >= visible_range.one_past_last)
        {
            break;
        }
        ARGB_Color argb = Fleury4GetCTokenColor(*token);
        
        if(token->kind == TokenBaseKind_Identifier && token_it_inc_all(&it))
        {
            Token *second_token = token_it_read(&it);
            token_it_dec_all(&it);
            
            // NOTE(rjf): Function or macro?
            if(second_token && second_token->kind == TokenBaseKind_ParentheticalOpen &&
               second_token->sub_kind == TokenCppKind_ParenOp)
            {
                argb = fcolor_resolve(fcolor_id(defcolor_pop1));
            }
            
            // NOTE(rjf): Is this a type?
            else
            {
                // TODO(rjf): When we can look up into the code index by table,
                // let's totally do that here. Otherwise this is way too slow.
#if 0
                Buffer_ID buffer = text_layout_get_buffer(app, text_layout_id);
                if(buffer)
                {
                    Code_Index_Note *note = 0;
                    
                    // NOTE(rjf): Look up token.
                    {
                        ProfileScope(app, "[Fleury] Code Index Token Look-Up");
                        Code_Index_Note *note = Fleury4LookUpTokenInCodeIndex(app, buffer, *token);
                    }
                    
                    if(note && note->note_kind == CodeIndexNote_Type)
                    {
                        argb = fcolor_resolve(fcolor_id(defcolor_pop2));
                    }
                }
#endif
            }
        }
        
        paint_text_color(app, text_layout_id, Ii64_size(token->pos, token->size), argb);
        if(!token_it_inc_all(&it))
        {
            break;
        }
    }
}

static void
Fleury4DrawTooltipRect(Application_Links *app, Rect_f32 rect)
{
    ARGB_Color background_color = fcolor_resolve(fcolor_id(defcolor_back));
    ARGB_Color border_color = fcolor_resolve(fcolor_id(defcolor_margin_active));
    
    background_color &= 0x00ffffff;
    background_color |= 0xd0000000;
    
    border_color &= 0x00ffffff;
    border_color |= 0xd0000000;
    
    draw_rectangle(app, rect, 4.f, background_color);
    draw_rectangle_outline(app, rect, 4.f, 3.f, border_color);
}

static void
Fleury4RenderCodePeek(Application_Links *app, View_ID view_id, Text_Layout_ID text_layout_id, Face_ID face_id,
                      Buffer_ID buffer, Frame_Info frame_info)
{
    Scratch_Block scratch(app);
    
    if(global_code_peek_open &&
       global_code_peek_selected_index >= 0 &&
       global_code_peek_selected_index < global_code_peek_match_count)
    {
        CodePeekMatch *match = &global_code_peek_matches[global_code_peek_selected_index];
        
        global_code_peek_open_transition += (1.f - global_code_peek_open_transition) * frame_info.animation_dt * 14.f;
        if(fabs(global_code_peek_open_transition - 1.f) > 0.005f)
        {
            animate_in_n_milliseconds(app, 0);
        }
        
        Rect_f32 rect = {0};
        rect.x0 = (float)((int)global_last_cursor_rect.x0 + 16);
        rect.y0 = (float)((int)global_last_cursor_rect.y0 + 16);
        rect.x1 = (float)((int)rect.x0 + 800);
        rect.y1 = (float)((int)rect.y0 + 600*global_code_peek_open_transition);
        
        if(rect.x1 > view_get_screen_rect(app, view_id).x1)
        {
            f32 difference = rect.x1 - view_get_screen_rect(app, view_id).x1;
            rect.x0 -= difference;
            rect.x1 -= difference;
        }
        
        Fleury4DrawTooltipRect(app, rect);
        
        //Face_Metrics metrics = get_face_metrics(app, face_id);
        
        if(rect.y1 - rect.y0 > 60.f)
        {
            // NOTE(rjf): Draw title.
            {
                Rect_f32 prev_clip = draw_set_clip(app, rect);
                
                char *match_type_string = "Text";
                
                switch(match->kind)
                {
                    case CodeIndexNote_Type: match_type_string = "Type"; break;
                    case CodeIndexNote_Function: match_type_string = "Function"; break;
                    case CodeIndexNote_Macro: match_type_string = "Macro"; break;
                    default: break;
                }
                
                String_Const_u8 file_name = push_buffer_unique_name(app, scratch, match->jump.buffer);
                String_Const_u8 title_string = push_u8_stringf(scratch, "%s - %.*s (%i of %i)", match_type_string,
                                                               string_expand(file_name),
                                                               global_code_peek_selected_index + 1,
                                                               global_code_peek_match_count);
                draw_string(app, face_id, title_string, V2f32(rect.x0 + 4, rect.y0 + 4),
                            fcolor_resolve(fcolor_id(defcolor_comment)));
                
                draw_set_clip(app, prev_clip);
            }
            
            // NOTE(rjf): Draw code.
            {
                rect.x0 += 30;
                rect.y0 += 30;
                rect.x1 -= 30;
                rect.y1 -= 30;
                
                Buffer_Point buffer_point =
                {
                    get_line_number_from_pos(app, match->jump.buffer, match->jump.pos),
                    0,
                };
                Text_Layout_ID text_layout_id = text_layout_create(app, match->jump.buffer, rect, buffer_point);
                
                Rect_f32 prev_clip = draw_set_clip(app, rect);
                {
                    Token_Array token_array = get_token_array_from_buffer(app, match->jump.buffer);
                    if(token_array.tokens != 0)
                    {
                        Fleury4DrawCTokenColors(app, text_layout_id, &token_array);
                    }
                    else
                    {
                        Range_i64 visible_range = Ii64(match->jump.pos, match->jump.pos + 1);
                        paint_text_color_fcolor(app, text_layout_id, visible_range, fcolor_id(defcolor_text_default));
                    }
                    
                    draw_text_layout_default(app, text_layout_id);
                }
                draw_set_clip(app, prev_clip);
                text_layout_free(app, text_layout_id);
            }
        }
    }
    else
    {
        global_code_peek_open_transition = 0.f;
    }
    
}

CUSTOM_COMMAND_SIG(fleury_code_peek)
CUSTOM_DOC("Opens code peek.")
{
    View_ID view = get_active_view(app, Access_ReadWriteVisible);
    i64 pos = view_get_cursor_pos(app, view);
    if(global_code_peek_open && pos >= global_code_peek_token_range.start &&
       pos <= global_code_peek_token_range.end)
    {
        Fleury4NextCodePeek();
    }
    else
    {
        Buffer_ID buffer = view_get_buffer(app, view, Access_Always);
        Scratch_Block scratch(app);
        Range_i64 range = enclose_pos_alpha_numeric_underscore(app, buffer, pos);
        global_code_peek_token_range = range;
        global_code_peek_token_buffer = buffer;
        String_Const_u8 base_needle = push_token_or_word_under_active_cursor(app, scratch);
        Fleury4OpenCodePeek(app, base_needle, StringMatch_CaseSensitive, StringMatch_LeftSideSloppy | StringMatch_RightSideSloppy);
    }
}

CUSTOM_COMMAND_SIG(fleury_close_code_peek)
CUSTOM_DOC("Closes code peek.")
{
    if(global_code_peek_open)
    {
        Fleury4CloseCodePeek();
    }
    else
    {
        leave_current_input_unhandled(app);
    }
}

CUSTOM_COMMAND_SIG(fleury_code_peek_go)
CUSTOM_DOC("Goes to the active code peek.")
{
    Fleury4CodePeekGo(app, 0);
}

CUSTOM_COMMAND_SIG(fleury_code_peek_go_same_panel)
CUSTOM_DOC("Goes to the active code peek.")
{
    Fleury4CodePeekGo(app, 1);
}
