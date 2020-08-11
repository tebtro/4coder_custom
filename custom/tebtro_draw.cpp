
//
// @note https://github.com/ryanbyczek/4coder-Customization
//        4coder_ryanb.cpp
//
function u32
tebtro_calculate_color_brightness(u32 color) {
    u32 r = ((color >> 16) & 0xFF);
    u32 g = ((color >> 8 ) & 0xFF);
    u32 b = ((color >> 0 ) & 0xFF);
    
    f32 brightness = sqrtf((r * r * 0.241f) + (g * g * 0.691f) + (b * b * 0.068f));
    
    return (u32)(brightness);
}
function void
tebtro_draw_hex_color_preview(Application_Links* app, Buffer_ID buffer, Text_Layout_ID text_layout_id, i64 pos) {
    ProfileScope(app, "ryanb draw hex color preview");
    
    Scratch_Block scratch(app);
    
    Range_i64 range = enclose_pos_alpha_numeric(app, buffer, pos);
    String_Const_u8 token = push_buffer_range(app, scratch, buffer, range);
    if (token.size == 10) {
        if (token.str[0] == '0' && (token.str[1] == 'x' || token.str[1] == 'X')) {
            b32 is_hex = true;
            for (u32 i = 0; (i < 8) && is_hex; ++i) {
                char c = token.str[i + 2];
                is_hex = ((c >= 'A' && c <= 'F') || (c >= 'a' && c <= 'f') || (c >= '0' && c <= '9'));
            }
            
            if (is_hex) {
                String_Const_u8 hex = string_substring(token, Ii64_size(2, 8));
                
                ARGB_Color hex_color = (u32)string_to_integer(hex, 16);
                draw_character_block(app, text_layout_id, Ii64_size(range.min, 10), 2.0f, hex_color);
                
                ARGB_Color textColor = tebtro_calculate_color_brightness(hex_color) < 128 ? 0xFFFFFFFF : 0xFF000000;
                paint_text_color(app, text_layout_id, range, textColor);
            }
        }
    }
}

//
// @note Background and margin
//

// @todo Display red margin color if the buffer is read-only
// @todo Hover margin color

function Rect_f32
tebtro_draw_background_and_margin(Application_Links *app, View_ID view_id, Buffer_ID buffer_id, b32 is_active_view, b32 keyboard_macro_is_recording) {
    i32 back_sub_id = 0;
    
    Buffer_ID comp_buffer_id = get_comp_buffer(app);
    if (buffer_id == comp_buffer_id) {
        back_sub_id = 1;
    }
    
    FColor back_color = fcolor_id(defcolor_back, back_sub_id);
    FColor margin_color = get_panel_margin_color((is_active_view) ? UIHighlight_Active : UIHighlight_None);
    
    if (is_active_view && keyboard_macro_is_recording) {
        margin_color = fcolor_id(defcolor_margin_keyboard_macro_is_recording);
    }
    
    ARGB_Color margin_argb = fcolor_resolve(margin_color);
    ARGB_Color back_argb = fcolor_resolve(back_color);
    
    return(draw_background_and_margin(app, view_id, margin_argb, back_argb));
}


//
// @note Comment notation test :comment_notations
//
// @todo(tebtro): Hello todo!
// @note(tebtro): Hello note!
// @note(rortner): Hello note!
// @note(Robert Ortner): Hello note!
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
        { string_u8_litexpr("@note"),  finalize_color(defcolor_comment_pop, COMMENT_POP_note)            },
        { string_u8_litexpr("@todo"),  finalize_color(defcolor_comment_pop, COMMENT_POP_todo)            },
        { string_u8_litexpr("@study"), finalize_color(defcolor_comment_pop, COMMENT_POP_study)           },
        { string_u8_litexpr("@"),      finalize_color(defcolor_comment_pop, COMMENT_POP_tag)             },
        { string_u8_litexpr(":"),      finalize_color(defcolor_comment_pop, COMMENT_POP_correlation_tag) },
        
        // User name
        { global_config.user_name,             finalize_color(defcolor_comment_pop, COMMENT_POP_username), false },
        { string_u8_litexpr("rortner"),        finalize_color(defcolor_comment_pop, COMMENT_POP_username), false },
        { string_u8_litexpr("Robert Ortner"),  finalize_color(defcolor_comment_pop, COMMENT_POP_username), false },
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

// @todo Use this and move the keyword stuff to here.
function FColor
tebtro_get_token_color_cpp(Application_Links *app, Buffer_ID buffer_id, Token token) {
    Scratch_Block scratch(app);
    
    Managed_ID color = defcolor_text_default;
    switch (token.kind){
        case TokenBaseKind_Preprocessor: {
            color = defcolor_preproc;
        } break;
        case TokenBaseKind_Keyword: {
            color = defcolor_keyword;
            
            String_Const_u8 lexeme = push_token_lexeme(app, scratch, buffer_id, &token);
            if (string_match(lexeme, SCu8("void"))  ||
                string_match(lexeme, SCu8("bool"))  ||
                string_match(lexeme, SCu8("char"))  ||
                string_match(lexeme, SCu8("short")) ||
                string_match(lexeme, SCu8("int"))   ||
                string_match(lexeme, SCu8("long"))  ||
                string_match(lexeme, SCu8("float")) ||
                string_match(lexeme, SCu8("double"))) {
                color = defcolor_type;
            }
        } break;
        case TokenBaseKind_Operator: {
            color = defcolor_operator;
            
            if (token.sub_kind == TokenCppKind_BrackOp || token.sub_kind == TokenCppKind_BrackCl) {
                color = defcolor_square_bracket;
            }
#if 0
            else if (token->sub_kind == TokenCppKind_Compare ||
                     token->sub_kind == TokenCppKind_Eq      ||
                     token->sub_kind == TokenCppKind_EqEq) {
                argb = 0xFFF2EBD3;
            }
            else if (token->sub_kind == TokenCppKind_Minus ||
                     token->sub_kind == TokenCppKind_MinusMinus) {
                argb = 0xFFF2EBD3;
            }
#endif
        } break;
        case TokenBaseKind_Comment: {
            color = defcolor_comment;
        } break;
        case TokenBaseKind_LiteralString: {
            color = defcolor_str_constant;
        } break;
        case TokenBaseKind_LiteralInteger: {
            color = defcolor_int_constant;
        } break;
        case TokenBaseKind_LiteralFloat: {
            color = defcolor_float_constant;
        } break;
        
        default: {
            switch (token.sub_kind) {
                case TokenCppKind_LiteralTrue:
                case TokenCppKind_LiteralFalse: {
                    color = defcolor_bool_constant;
                } break;
                case TokenCppKind_LiteralCharacter:
                case TokenCppKind_LiteralCharacterWide:
                case TokenCppKind_LiteralCharacterUTF8:
                case TokenCppKind_LiteralCharacterUTF16:
                case TokenCppKind_LiteralCharacterUTF32: {
                    color = defcolor_char_constant;
                } break;
                case TokenCppKind_PPIncludeFile: {
                    color = defcolor_include;
                } break;
            }
        } break;
    }
    return(fcolor_id(color));
}

//
// @note: _underscore_text_
//        -strikethrough-text-
//        *bold_text*
//        /italic-text/
//
inline void
tebtro_draw_one_comment_font_styles(Application_Links *app, Text_Layout_ID text_layout_id, Buffer_ID buffer_id, Token *token, ARGB_Color argb_color_foreground, Face_ID underlined_face_id, Face_ID strikethrough_face_id, Face_ID bold_face_id, Face_ID italic_face_id) {
    if (!token)  return;
    Scratch_Block scratch(app);
    
    Face_ID buffer_face_id = get_face_id(app, buffer_id);
    Face_Metrics face_metrics = get_face_metrics(app, buffer_face_id);
    Face_ID face_id = 0;
    
    String_Const_u8 lexeme = push_token_lexeme(app, scratch, buffer_id, token);
    
    b32 found = false;
    b32 underlined = false;
    b32 strikethrough = false;
    
    int string_start_index = 0;
    int string_end_index = 0;
    int i = 1;
    for (;i+1 < lexeme.size; ++i) {
        u8 *c = lexeme.str + i;
        if (character_is_whitespace(*(c - 1)) && *c == '_') {
            string_start_index = i;
            while (i < lexeme.size && character_is_alpha_numeric(*c)) {
                ++i;
                c = lexeme.str + i;
            }
            if (*(c - 1) == '_') {
                string_end_index = i;
                found = true;
                
                underlined = true;
                face_id = underlined_face_id;
            }
        }
        else if (character_is_whitespace(*(c - 1)) && *c == '-') {
            string_start_index = i;
            while (i < lexeme.size && (character_is_alpha_numeric(*c) || *c == '-')) {
                ++i;
                c = lexeme.str + i;
            }
            if (*(c - 1) == '-') {
                string_end_index = i;
                found = true;
                
                strikethrough = true;
                face_id = strikethrough_face_id;
            }
        }
        else if (character_is_whitespace(*(c - 1)) && *c == '*') {
            string_start_index = i;
            while (i < lexeme.size && (character_is_alpha_numeric(*c) || *c == '-' || *c == '*')) {
                ++i;
                c = lexeme.str + i;
            }
            if (*(c - 1) == '*') {
                string_end_index = i;
                found = true;
                
                face_id = bold_face_id;
            }
        }
        else if (character_is_whitespace(*(c - 1)) && *c == '/') {
            string_start_index = i;
            while (i < lexeme.size && (character_is_alpha_numeric(*c) || *c == '-' || *c == '/')) {
                ++i;
                c = lexeme.str + i;
            }
            if (*(c - 1) == '/') {
                string_end_index = i;
                found = true;
                
                face_id = italic_face_id;
            }
        }
        
        if (!found)  continue;
        
        Rect_f32 first_char_rect = text_layout_character_on_screen(app, text_layout_id, token->pos + string_start_index);
        Rect_f32 last_char_rect = text_layout_character_on_screen(app, text_layout_id, token->pos + string_end_index - 1);
        
        String_Const_u8 substring;
        substring.str = lexeme.str + string_start_index;
        substring.size = (string_end_index - string_start_index);
        if (substring.size <= 2)  goto end_found;
        
        if (face_id != 0) {
            Vec2_f32 point;
            point.x = first_char_rect.x0;
            point.y = first_char_rect.y0;
            
#if 1
            // @note: Hide actual comment text
            Rect_f32 rect;
            rect.x0 = point.x;
            rect.x1 = last_char_rect.x1;
            rect.y0 = point.y;
            rect.y1 = first_char_rect.y1;
            draw_rectangle(app, rect, 0.0f, fcolor_resolve(fcolor_id(defcolor_back)));
#else
            // @note We are first putting the actual text on the screen so we cant do that here.
            paint_text_color(app, text_layout_id, Ii64(token->pos + string_start_index, token->pos + string_end_index - 1), fcolor_resolve(fcolor_id(defcolor_back)));
#endif
            
#if 1
            draw_string(app, face_id, substring, point, argb_color_foreground);
#else
            Fancy_String *fancy_string = push_fancy_string(scratch, 0, face_id, fcolor_argb(argb_color_foreground), substring);
            draw_fancy_string(app, fancy_string, point);
#endif
        }
        else if (underlined || strikethrough) {
            f32 height = face_metrics.line_height * 0.06f;
            Rect_f32 rect = {};
            
            if (underlined) {
                f32 offset = face_metrics.line_height * 0.105f;
                rect.y1 = first_char_rect.y1 - offset;
                rect.y0 = rect.y1 - height;
                rect.x0 = first_char_rect.x0;
                rect.x1 = last_char_rect.x1;
            }
            else if (strikethrough) {
                height *= 1.3f;
                f32 offset = face_metrics.line_height * 0.5f;
                rect.y0 = first_char_rect.y0 + offset - (height * 0.25f);
                rect.y1 = rect.y0 + height;
                rect.x0 = first_char_rect.x0;
                rect.x1 = last_char_rect.x1;
            }
            
            draw_rectangle(app, rect, 0.0f, argb_color_foreground);
            underlined = false;
            strikethrough = false;
        }
        
        end_found:;
        i = string_end_index;
        string_start_index = 0;
        string_end_index = 0;
        found = false;
    }
}
function void
tebtro_draw_comment_font_styles(Application_Links *app, Text_Layout_ID text_layout_id, Buffer_ID buffer_id, Token_Array *array, Face_ID underlined_face_id, Face_ID strikethrough_face_id, Face_ID bold_face_id, Face_ID italic_face_id) {
    Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
    i64 first_index = token_index_from_pos(array, visible_range.first);
    Token_Iterator_Array it = token_iterator_index(0, array, first_index);
    for (;;) {
        Token *token = token_it_read(&it);
        if (token && token->pos >= visible_range.one_past_last) {
            break;
        }
        if (token && token->kind == TokenBaseKind_Comment) {
            ARGB_Color argb_color = fcolor_resolve(fcolor_id(defcolor_comment));
            tebtro_draw_one_comment_font_styles(app, text_layout_id, buffer_id, token, argb_color, underlined_face_id, strikethrough_face_id, bold_face_id, italic_face_id);
        }
        if (!token_it_inc_all(&it)) {
            break;
        }
    }
}

function void
tebtro_draw_cpp_token_colors__only_comments(Application_Links *app, Text_Layout_ID text_layout_id, Buffer_ID buffer, Token_Array *array) {
    Scratch_Block scratch(app);
    
    Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
    i64 first_index = token_index_from_pos(array, visible_range.first);
    Token_Iterator_Array it = token_iterator_index(0, array, first_index);
    for (;;) {
        Token *token = token_it_read(&it);
        if (token->pos >= visible_range.one_past_last) {
            break;
        }
        FColor color = fcolor_id(defcolor_text_default);
        if (token->kind == TokenBaseKind_Comment) {
            color = fcolor_id(defcolor_comment);
        }
        ARGB_Color argb = fcolor_resolve(color);
        paint_text_color(app, text_layout_id, Ii64_size(token->pos, token->size), argb);
        if (!token_it_inc_all(&it)) {
            break;
        }
    }
}

function void
tebtro_draw_cpp_token_colors(Application_Links *app, Text_Layout_ID text_layout_id, Buffer_ID buffer_id, Token_Array *array) {
    Scratch_Block scratch(app);
    
    Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
    i64 first_index = token_index_from_pos(array, visible_range.first);
    Token_Iterator_Array it = token_iterator_index(0, array, first_index);
    for (;;) {
        Token *token = token_it_read(&it);
        if (token->pos >= visible_range.one_past_last) {
            break;
        }
        FColor color = tebtro_get_token_color_cpp(app, buffer_id, *token);
        ARGB_Color argb = fcolor_resolve(color);
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
    
    for (;;) {
        Token *token = token_it_read(&it);
        if (token->pos >= visible_range.one_past_last){
            break;
        }
        if (token->kind == TokenBaseKind_Identifier) {
            // @note Default is like for all other identifiers, so all other variable names.
            Managed_ID color = defcolor_text_default;
            
            // @note lookup identifier
            String_Const_u8 lexeme = push_token_lexeme(app, scratch, buffer, token);
            u64 hash = djb2(lexeme);
            
            b32 found = false;
            for (Buffer_ID buffer_it = get_buffer_next(app, 0, Access_Always);
                 (!found && buffer_it != 0);
                 buffer_it = get_buffer_next(app, buffer_it, Access_Always)) {
                Managed_Scope scope = buffer_get_managed_scope(app, buffer_it);
                Code_Index_Identifier_Hash_Table *identifier_table = scope_attachment(app, scope, attachment_code_index_identifier_table, Code_Index_Identifier_Hash_Table);
                if (!identifier_table)  continue;
                if (identifier_table->count == 0)  continue;
                Code_Index_Identifier_Node *node = code_index_identifier_table_lookup(identifier_table, hash);
                if (node != 0) {
                    switch (node->note_kind) {
                        case CodeIndexNote_Type: {
                            color = defcolor_type;
                            found = true;
                        } break;
                        case CodeIndexNote_Function: {
                            color = defcolor_type_function;
                            found = true;
                        } break;
                        case CodeIndexNote_Macro: {
                            color = defcolor_type_macro;
                            found = true;
                        } break;
                    }
                }
            }
            
            // @todo @cleanup auto should just be added to the lexer as a keyword.
            if (string_match(lexeme, SCu8("auto"))) {
                color = defcolor_keyword;
            }
            
            ARGB_Color argb = fcolor_resolve(fcolor_id(color));
            paint_text_color(app, text_layout_id, Ii64_size(token->pos, token->size), argb);
        }
        if (!token_it_inc_all(&it)){
            break;
        }
    }
}


//
// @note Highlight token or (in comments) word under cursor.
//
inline void
tebtro_draw_token_under_cursor_highlight(Application_Links *app, Text_Layout_ID text_layout_id, Buffer_ID buffer_id, Token_Array *token_array, i64 cursor_pos, f32 cursor_roundness) {
    Range_i64 range = get_token_or_word_under_cursor_range(app, buffer_id, cursor_pos, token_array);
    
    // @note Draw background
    {
        // 0xFF1A2898; // 0xFF1A30AC; // 0xFF1A303C; // 0xFF1C3039;
        ARGB_Color argb = fcolor_resolve(fcolor_id(defcolor_highlight_token_under_cursor));
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
tebtro_draw_search_highlight(Application_Links *app, View_ID view_id, Buffer_ID buffer_id, Text_Layout_ID text_layout_id, f32 roundness,
                             ARGB_Color argb_highlight, ARGB_Color argb_at_highlight, ARGB_Color argb_match, ARGB_Color argb_at_match) {
    Scratch_Block scratch(app);
    
    b32 has_highlight_range = false;
    Managed_Scope scope = view_get_managed_scope(app, view_id);
    Buffer_ID *highlight_buffer = scope_attachment(app, scope, view_highlight_buffer, Buffer_ID);
    if (*highlight_buffer != 0) {
        if (*highlight_buffer != buffer_id) {
            view_disable_highlight_range(app, view_id);
        }
        else {
            has_highlight_range = true;
            Managed_Object *highlight = scope_attachment(app, scope, view_highlight_range, Managed_Object);
            Marker marker_range[2];
            if (managed_object_load_data(app, *highlight, 0, 2, marker_range)) {
                Range_i64 range = Ii64(marker_range[0].pos, marker_range[1].pos);
                
                // @note Draw all search items in visible range
                Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
                String_Const_u8 needle = push_buffer_range(app, scratch, buffer_id, range);
                String_Match_List matches = buffer_find_all_matches(app, scratch, buffer_id, 0, visible_range, needle, &character_predicate_alpha_numeric_underscore_utf8, Scan_Forward);
                if (global_search_highlight_case_sensitive) {
                    String_Match_Flag must_have_flags = 0;
                    String_Match_Flag must_not_have_flags = 0;
                    
                    must_have_flags     = StringMatch_CaseSensitive;
                    // must_not_have_flags = StringMatch_CaseInsensitive;
                    // StringMatch_LeftSideSloppy | StringMatch_RightSideSloppy
                    string_match_list_filter_flags(&matches, must_have_flags, must_not_have_flags);
                }
                
                String_Match *match = matches.first;
                for (int i = 0; i < matches.count; ++i) {
                    Range_i64 match_range = match->range;
                    
                    draw_character_block(app, text_layout_id, match_range, roundness, argb_match);
                    paint_text_color(app, text_layout_id, match_range, argb_at_match);
                    
                    match = match->next;
                }
                
                // @note Draw active search item
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
function void
tebtro_draw_enclosures(Application_Links *app, Text_Layout_ID text_layout_id, Buffer_ID buffer,
                       i64 pos, u32 flags, Range_Highlight_Kind kind,
                       ARGB_Color *back_colors, i32 back_count,
                       ARGB_Color *fore_colors, i32 fore_count) {
    Scratch_Block scratch(app);
    Range_i64_Array ranges = get_enclosure_ranges(app, scratch, buffer, pos, flags);
    
    for (i32 i = ranges.count - 1; i >= 0; i -= 1){
        Range_i64 range = ranges.ranges[i];
        if (kind == RangeHighlightKind_LineHighlight){
            Range_i64 r[2] = {};
            if (i > 0){
                Range_i64 inner_range = ranges.ranges[i - 1];
                Range_i64 lines = get_line_range_from_pos_range(app, buffer, range);
                Range_i64 inner_lines = get_line_range_from_pos_range(app, buffer, inner_range);
                inner_lines.min = clamp_bot(lines.min, inner_lines.min);
                inner_lines.max = clamp_top(inner_lines.max, lines.max);
                inner_lines.min -= 1;
                inner_lines.max += 1;
                if (lines.min <= inner_lines.min){
                    r[0] = Ii64(lines.min, inner_lines.min);
                }
                if (inner_lines.max <= lines.max){
                    r[1] = Ii64(inner_lines.max, lines.max);
                }
            }
            else{
                r[0] = get_line_range_from_pos_range(app, buffer, range);
            }
            for (i32 j = 0; j < 2; j += 1){
                if (r[j].min == 0){
                    continue;
                }
                Range_i64 line_range = r[j];
                if (back_colors != 0){
                    i32 back_index = i%back_count;
                    draw_line_highlight(app, text_layout_id, line_range, back_colors[back_index]);
                }
                if (fore_colors != 0){
                    i32 fore_index = i%fore_count;
                    Range_i64 pos_range = get_pos_range_from_line_range(app, buffer, line_range);
                    paint_text_color(app, text_layout_id, pos_range, fore_colors[fore_index]);
                }
            }
        }
        else{
            if (back_colors != 0){
                i32 back_index = i%back_count;
                draw_character_block(app, text_layout_id, range.min, 0.f, back_colors[back_index]);
                draw_character_block(app, text_layout_id, range.max - 1, 0.f, back_colors[back_index]);
            }
            if (fore_colors != 0){
                i32 fore_index = i%fore_count;
                paint_text_color_pos(app, text_layout_id, range.min, fore_colors[fore_index]);
                paint_text_color_pos(app, text_layout_id, range.max - 1, fore_colors[fore_index]);
            }
        }
    }
}

inline void
tebtro_draw_brace_highlight(Application_Links *app, Buffer_ID buffer_id, Text_Layout_ID text_layout_id, i64 pos, ARGB_Color *colors, i32 color_count) {
    Scratch_Block scratch(app);
    Range_i64_Array ranges = get_enclosure_ranges(app, scratch, buffer_id, pos, RangeHighlightKind_CharacterHighlight);
#if USE_RANGE_COLOR_START_DEFAULT
    draw_enclosures(app, text_layout_id, buffer_id, pos, FindNest_Scope, RangeHighlightKind_CharacterHighlight, 0, 0, colors, color_count);
#else
    tebtro_draw_enclosures(app, text_layout_id, buffer_id, pos, FindNest_Scope, RangeHighlightKind_CharacterHighlight, 0, 0, colors, color_count);
#endif
}

function void
tebtro_draw_paren_highlight(Application_Links *app, Buffer_ID buffer, Text_Layout_ID text_layout_id,
                            i64 pos, ARGB_Color *colors, i32 color_count){
    Token_Array token_array = get_token_array_from_buffer(app, buffer);
    if (token_array.tokens != 0){
        Token_Iterator_Array it = token_iterator_pos(0, &token_array, pos);
        Token *token = token_it_read(&it);
        if (token != 0 && token->kind == TokenBaseKind_ParentheticalOpen){
            pos = token->pos + token->size;
        }
        else{
            if (token_it_dec_all(&it)){
                token = token_it_read(&it);
                if (token->kind == TokenBaseKind_ParentheticalClose &&
                    pos == token->pos + token->size){
                    pos = token->pos;
                }
            }
        }
    }
    tebtro_draw_enclosures(app, text_layout_id, buffer,
                           pos, FindNest_Paren, RangeHighlightKind_CharacterHighlight,
                           0, 0, colors, color_count);
}

function void
tebtro_draw_scope_highlight(Application_Links *app, Buffer_ID buffer, Text_Layout_ID text_layout_id,
                            i64 pos, ARGB_Color *colors, i32 color_count){
    tebtro_draw_enclosures(app, text_layout_id, buffer,
                           pos, FindNest_Scope, RangeHighlightKind_LineHighlight,
                           colors, color_count, 0, 0);
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
#if USE_RANGE_COLOR_START_DEFAULT
        u32 color = colors[color_index % color_count];
#else
        u32 color = colors[i % color_count];
#endif
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


