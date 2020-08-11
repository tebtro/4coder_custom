//
// @note(tebtro) Modifies:
// Added indentation centering x:  float vertical_scope_annotation_x_centering = 0.1f;
//


/*
# Description

This module helps to figure out which scope is enclosing the current code line by
displaying the line preceding the start of the scope vertically in the margin.

# Version

This was tested with 4coder 4.1.6.

# Setup

You need to #include this file in your custom layer ('4coder_default_bindings.cpp' by
default) below the line:

'''c
#include "4coder_default_include.cpp"
'''

You need to call the function 'vertical_scope_annotation_draw' from the render caller hook
('HookID_RenderCaller'). By default this is the 'default_render_buffer' function in
'4coder_default_hooks.cpp'.

If you're using the default hook, you will need to insert the following function prototype
above the declaration of 'default_render_buffer'.

'''c
function void vertical_scope_annotation_draw( Application_Links *app, View_ID view, Buffer_ID buffer, Text_Layout_ID text_layout_id, u32 flags );
'''

You'll also need to either move the declaration of the 'vertical_scope_annotation_flag_t'
enum before the declaration of 'default_render_buffer' or hardcoding the values instead of
using the enum names. The values are:

'''c
vertical_scope_annotation_flag_top_to_bottom = 0;
vertical_scope_annotation_flag_bottom_to_top = 1;
vertical_scope_annotation_flag_highlight = 2;
vertical_scope_annotation_flag_highlight_case_label = 4;
vertical_scope_annotation_flag_no_comment = 8;
vertical_scope_annotation_flag_only_last_case = 16;
'''

If you're using a custom render caller hook, make sure you include the file before the
definition of your render caller hook.

The call should look like this:

'''c
vertical_scope_annotation_draw( app, view_id, buffer, text_layout_id, flags );
'''

Where flags is a combination of any of the 'vertical_scope_annotation_flag_*' values.

- 'flag_top_top_bottom': display the text from the top towards the bottom (the baseline is
on the left);
- 'flag_bottom_to_top': display the text from the bottom towards the top (the baseline is
on the right);
- 'flag_highlight': display part of the text (keyword, identifier, label... depending on
the line) in a different color;
- 'flag_highlight_case_label': highlight case labels instead of the 'case' keyword;
- 'flag_no_comment': don't use comment in the text;
- 'flag_only_last_case': if the text contains several case labels (fallthrough), display
only the last one.

Colors can be configured by adding the following lines in your theme file.

'''c
vertical_scope_annotation_text = defcolor_comment;
vertical_scope_annotation_highlight = defcolor_text_default;
'''

# Issues

If the text is purple, make sure the theme file is loaded.

The full theme file name needs to be set in 'config.4coder', which by default isn't the
case. Use :

'''c
default_theme_name = "theme-4coder";
'''

instead of

'''c
default_theme_name = "4coder";
'''

# Details

## Prefix

This module uses 'vertical_scope_annotation' as a prefix for its types and functions.

## Ids

This module declares the following managed ids:

In the 'colors' group:
- 'vertical_scope_annotation_text';
- 'vertical_scope_annotation_highlight'.

## Modified files

This module needs modifications in the following files to work:
- '4coder_default_bindings.h' or your custom layer file;
- '4coder_default_hooks.h'.

## Global variables

This module doesn't use any global variables.

## Types

This module defines the following types:
- 'vertical_scope_annotation_flag_t';
- 'vertical_scope_annotation_start_t'.

## Functions

This module declare the following functions:
- 'vertical_scope_annotation_draw';
- 'vertical_scope_annotation_get_text';
- 'vertical_scope_annotation_get_start'.

## Scope attachment

This module doesn't declare any scope attachment.
*/

CUSTOM_ID( colors, vertical_scope_annotation_text );
CUSTOM_ID( colors, vertical_scope_annotation_highlight );

typedef enum vertical_scope_annotation_flag_t {
    vertical_scope_annotation_flag_top_to_bottom = 0,
    vertical_scope_annotation_flag_bottom_to_top = 1,
    vertical_scope_annotation_flag_highlight = 1 << 1,
    vertical_scope_annotation_flag_highlight_case_label = 1 << 2,
    vertical_scope_annotation_flag_no_comment = 1 << 3,
    vertical_scope_annotation_flag_only_last_case = 1 << 4,
    
    /* NOTE: Do not use this. */
    vertical_scope_annotation_orientation_mask = vertical_scope_annotation_flag_bottom_to_top,
    
} vertical_scope_annotation_flag_t;

typedef struct vertical_scope_annotation_start_t {
    
    Token_Iterator_Array it;
    b32 contains_if;
    b32 contains_else;
    b32 is_valid;
    
} vertical_scope_annotation_start_t;

function vertical_scope_annotation_start_t vertical_scope_annotation_get_start( Token_Array* tokens, i64 position, u32 flags ) {
    
    /* NOTE: The annotation_start_t struct iterator will point to the token preceding the one to use for the string. */
    vertical_scope_annotation_start_t annotation = { 0 };
    annotation.it = token_iterator_pos( 0, tokens, position );
    i64 parenthesis = 0;
    
    b32 contains_keyword = false;
    b32 contains_identifier = false;
    b32 contains_comment = false;
    b32 stop = false;
    
    while ( !stop && token_it_dec_non_whitespace( &annotation.it ) ) {
        
        Token *token = token_it_read( &annotation.it );
        
        if ( token->kind == TokenBaseKind_ParentheticalClose ) {
            parenthesis++;
        } else if ( token->kind == TokenBaseKind_ParentheticalOpen ) {
            parenthesis--;
        } else if ( parenthesis == 0 ) {
            
            switch ( token->kind ) {
                
                case TokenBaseKind_Identifier: {
                    
                    if ( token->flags == TokenBaseFlag_PreprocessorBody ) {
                        stop = true;
                    } else {
                        contains_identifier = true;
                    }
                } break;
                
                case TokenBaseKind_Preprocessor:
                case TokenBaseKind_LiteralInteger:
                case TokenBaseKind_LiteralFloat:
                case TokenBaseKind_LiteralString:
                case TokenBaseKind_ScopeOpen:
                case TokenBaseKind_ScopeClose: {
                    stop = true;
                } break;
                
                case TokenBaseKind_StatementClose: {
                    if ( token->sub_kind != TokenCppKind_Colon ) {
                        stop = true;
                    }
                } break;
                
                case TokenBaseKind_Comment: {
                    
                    if ( !( flags & vertical_scope_annotation_flag_no_comment ) ) {
                        
                        if ( !contains_keyword ) {
                            token_it_dec_non_whitespace( &annotation.it );
                            contains_comment = true;
                        };
                        stop = true;
                    }
                } break;
                
                case TokenBaseKind_Keyword: {
                    
                    if ( token->sub_kind == TokenCppKind_Else ) {
                        annotation.contains_else = true;
                    } else if ( token->sub_kind == TokenCppKind_If ) {
                        annotation.contains_if = true;
                    } else if ( token->sub_kind == TokenCppKind_Case && ( flags & vertical_scope_annotation_flag_only_last_case ) ) {
                        token_it_dec_non_whitespace( &annotation.it );
                        stop = true;
                    }
                    
                    contains_keyword = true;
                    
                } break;
            }
        }
    }
    
    annotation.is_valid = contains_keyword || contains_identifier || contains_comment;
    
    return annotation;
}

function i64 vertical_scope_annotation_get_text( Application_Links* app, Buffer_ID buffer, Token_Array* tokens, i64 position, u32 flags, String_u8* out, Range_i64* highlight_range ) {
    
    ProfileScope( app, "vertical scope annotation get_text" );
    
    Scratch_Block scratch( app );
    
    i64 first_character_position = 0;
    highlight_range->min = highlight_range->max = -1;
    
    vertical_scope_annotation_start_t annotation = vertical_scope_annotation_get_start( tokens, position, flags );
    b32 stop_after_new_line = false;
    b32 contains_else_of = false;
    
    if ( annotation.is_valid && annotation.contains_else && !annotation.contains_if ) {
        
        /* NOTE: if we are on "else", not "else if" we need to seek the if to get the condition. */
        annotation.is_valid = false;
        string_append( out, string_u8_litexpr( "else of " ) );
        contains_else_of = true;
        
        Token* token = token_it_read( &annotation.it );
        
        if ( token->kind == TokenBaseKind_ScopeClose ) {
            
            i64 if_scope_open = -1;
            b32 found = find_nest_side( app, buffer, token->pos - 1, FindNest_Balanced | FindNest_Scope, Scan_Backward, NestDelim_Open, &if_scope_open );
            
            if ( found ) {
                annotation = vertical_scope_annotation_get_start( tokens, if_scope_open, flags );
            }
            
        } else {
            
            /* NOTE: "if" without scope bracket, search backward for the first "if". */
            stop_after_new_line = true;
            
            while ( token_it_dec_non_whitespace( &annotation.it ) ) {
                token = token_it_read( &annotation.it );
                if ( token->kind == TokenBaseKind_Keyword && token->sub_kind == TokenCppKind_If ) {
                    /* NOTE: There could be a potential "else if" here so we need to call get_annotation_start again. */
                    annotation = vertical_scope_annotation_get_start( tokens, token->pos + token->size, flags );
                    break;
                }
            }
        }
    }
    
    if ( annotation.is_valid ) {
        
        Assert( !annotation.contains_else || annotation.contains_if );
        
        b32 has_token = token_it_inc_non_whitespace( &annotation.it );
        b32 stop = false;
        i64 parenthesis = 0;
        first_character_position = annotation.it.ptr->pos;
        b32 find_highlight = true;
        
        while ( !stop && has_token ) {
            
            Token* token = token_it_read( &annotation.it );
            
            if ( token->kind == TokenBaseKind_ParentheticalOpen ) {
                parenthesis++;
            } else if ( token->kind == TokenBaseKind_ParentheticalClose ) {
                parenthesis--;
            } else if ( parenthesis == 0 ) {
                
                switch ( token->kind ) {
                    
                    case TokenBaseKind_ScopeOpen: {
                        stop = true;
                    } break;
                    
                    case TokenBaseKind_Whitespace: {
                        
                        /* NOTE: "if" without scope bracket. */
                        if ( stop_after_new_line ) {
                            u8 c = buffer_get_char( app, buffer, token->pos );
                            
                            if ( c == '\n' || c == '\r' ) {
                                stop = true;
                            }
                        }
                        
                    } break;
                }
            }
            
            if ( !stop ) {
                
                /* NOTE: a single whitespace token groups concecutive whitespaces and we want to only print one white space and no new lines.*/
                if ( token->kind == TokenBaseKind_Whitespace ) {
                    string_append( out, string_u8_litexpr( " " ) );
                } else {
                    
                    if ( token->kind != TokenBaseKind_Comment || !( flags & vertical_scope_annotation_flag_no_comment ) ) {
                        
                        /* NOTE: Should I highlight "else" in "else if" ? */
                        if ( find_highlight && ( token->kind == TokenBaseKind_Keyword || token->kind == TokenBaseKind_Identifier ) ) {
                            highlight_range->min = out->size;
                            highlight_range->max = out->size + token->size;
                        }
                        
                        b32 case_condition = false;
                        
                        if ( flags & vertical_scope_annotation_flag_highlight_case_label ) {
                            case_condition = ( token->kind == TokenBaseKind_StatementClose && token->sub_kind == TokenCppKind_Colon );
                        } else {
                            case_condition = ( token->kind == TokenBaseKind_Keyword && token->sub_kind == TokenCppKind_Case );
                        }
                        
                        if ( token->kind == TokenBaseKind_ParentheticalOpen || case_condition ) {
                            find_highlight = false;
                        }
                        
                        String_Const_u8 temp = push_token_lexeme( app, scratch, buffer, token );
                        string_append( out, temp );
                    }
                }
                
                has_token = token_it_inc_all( &annotation.it );
            }
        }
        
        if ( find_highlight ) {
            highlight_range->min = highlight_range->max = -1;
        } else if ( contains_else_of ) {
            highlight_range->min =  0;
        }
    }
    
    return first_character_position;
}

function void vertical_scope_annotation_draw( Application_Links *app, View_ID view, Buffer_ID buffer, Text_Layout_ID text_layout_id, u32 flags ) {
    
    ProfileScope( app, "vertical scope annotation" );
    
    Range_i64 visible_range = text_layout_get_visible_range( app, text_layout_id );
    Token_Array tokens = get_token_array_from_buffer( app, buffer );
    
    Face_ID face_id = get_face_id( app, buffer );
    Face_Metrics metrics = get_face_metrics( app, face_id );
    Rect_f32 region = text_layout_region( app, text_layout_id );
    
    Buffer_Scroll scroll = view_get_buffer_scroll( app, view );
    float x_start = region.x0 - scroll.position.pixel_shift.x;
    
    i64 indent_space_count = global_config.indent_width;
    
    if ( global_config.enable_virtual_whitespace ) {
        indent_space_count = global_config.virtual_whitespace_regular_indent;
    }
    
    Scratch_Block scratch( app );
    
    /* NOTE: Get the end of the textual line. */
    i64 new_pos = get_line_side_pos_from_pos( app, buffer, visible_range.min, Side_Max );
    Buffer_Cursor cursor = view_compute_cursor( app, view, seek_pos( new_pos ) );
    Vec2_f32 p = view_relative_xy_of_pos( app, view, cursor.line, cursor.pos );
    i64 end_of_line = view_pos_at_relative_xy( app, view, cursor.line, p );
    
    ProfileBlockNamed( app, "vertical scope annotation get_enclosure_ranges", get_enclosure_ranges_block );
    Range_i64_Array ranges = get_enclosure_ranges( app, scratch, buffer, end_of_line, RangeHighlightKind_CharacterHighlight );
    ProfileCloseNow( get_enclosure_ranges_block );
    
    ProfileBlockNamed( app, "vertical scope annotation ranges loop", ranges_loop );
    
    for ( i32 i = ranges.count - 1; i >= 0; i-- ) {
        
        Range_i64 range = ranges.ranges[ i ];
        
        u8 string_buffer[ 1024 ] = { 0 };
        String_u8 annotation = Su8( string_buffer, 0, ArrayCount( string_buffer ) );
        Range_i64 highlight = { -1, -1 };
        i64 first_character_position = vertical_scope_annotation_get_text( app, buffer, &tokens, range.min, flags, &annotation, &highlight );
        Range_i64 scope_close_highlight = highlight;
        
        String_Const_u8 scope_close_annotation = SCu8(annotation.str, annotation.size);
        
        b32 draw = false;
        f32 region_top_offset = 0;
        
        if ( first_character_position >= visible_range.min ) {
            /* NOTE: Is the text only partially visible vertically ? */
            Rect_f32 rect = text_layout_character_on_screen( app, text_layout_id, first_character_position );
            
            if ( rect.y1 < region.y0 + metrics.text_height ) {
                rect = text_layout_character_on_screen( app, text_layout_id, range.min );
                region_top_offset = rect.y1 - region.y0;
                draw = true;
            }
        } else {
            
            /* NOTE: If an "if else" has the "else" in view but the "if" isn't, we need to adjust the region to avoid overlapping annotation with code. */
            if ( range.min >= visible_range.min ) {
                Rect_f32 rect = text_layout_character_on_screen( app, text_layout_id, range.min );
                region_top_offset = rect.y1 - region.y0;
            }
            
            draw = true;
        }
        
        if ( draw ) {
            
            ProfileScope( app, "vertical scope annotation draw" );
            
            b32 override_next_range = false;
            Range_i64 override_range = { 0 };
            
            if ( highlight.min > -1 ) {
                
                ProfileScope( app, "vertical scope annotation find else range of if" );
                
                /* NOTE simon: When there is an "if else" and the if is outside the view we want show the annotation for the else even if it's in view. */
                u64 keyword_size = annotation.size - highlight.min;
                b32 is_if = false;
                
                if ( keyword_size >= 2 ) {
                    is_if = ( annotation.str[ highlight.min ] == 'i' && annotation.str[ highlight.min + 1 ] == 'f' );
                    
                    if ( keyword_size > 2 ) {
                        is_if = is_if && ( annotation.str[ highlight.min + 2 ] == ' ' || annotation.str[ highlight.min + 2 ] == '(' );
                    }
                }
                
                if ( is_if ) {
                    
                    Token_Iterator_Array it = token_iterator_pos( 0, &tokens, range.max );
                    token_it_inc_non_whitespace( &it );
                    Token* token = token_it_read( &it );
                    
                    if ( token->kind == TokenBaseKind_Keyword && token->sub_kind == TokenCppKind_Else ) {
                        
                        token_it_inc_non_whitespace( &it );
                        token = token_it_read( &it );
                        
                        if ( token->kind == TokenBaseKind_ScopeOpen ) {
                            override_range.start = token->pos;
                            override_next_range = find_nest_side( app, buffer, token->pos + 1, FindNest_Scope | FindNest_Balanced | FindNest_EndOfToken, Scan_Forward, NestDelim_Close, &override_range.end );
                            if ( override_range.start >= visible_range.end || override_range.end <= visible_range.start ) {
                                override_next_range = false;
                            }
                        }
                    }
                }
            }
            
            float y0 = region.y0 + region_top_offset;
            float y1 = region.y1;
            
            if ( range.max - 1 < visible_range.max ) {
                ProfileScope( app, "vertical scope annotation text_layout_character_on_screen" );
                Rect_f32 rect = text_layout_character_on_screen( app, text_layout_id, range.end );
                y1 = rect.y0;
            }
            
            f32 max_size = y1 - y0;
            
            if ( max_size < 0 ) {
                max_size = 0;
            }
            
            ProfileBlockNamed( app, "vertical scope annotation get_string_advance", get_string_advance_block );
            
            f32 size = get_string_advance( app, face_id, SCu8( annotation.str, annotation.size ) );
#if 0
            while ( size > max_size && annotation.size > 1 ) {
                annotation.size -= 1;
                size = get_string_advance( app, face_id, SCu8( annotation.str, annotation.size ) );
            }
#else
            if ( size > max_size ) {
                
                f32 percentage = max_size / size;
                u64 string_size = ( u64 ) ( ( f32 ) annotation.size * percentage );
                
                size = get_string_advance( app, face_id, SCu8( annotation.str, string_size ) );
                
                if ( size > max_size ) {
                    while ( size > max_size && string_size > 1 ) {
                        string_size--;
                        size = get_string_advance( app, face_id, SCu8( annotation.str, string_size ) );
                    }
                } else {
                    f32 previous_size = size;
                    while ( size < max_size && string_size < annotation.size ) {
                        string_size++;
                        previous_size = size;
                        size = get_string_advance( app, face_id, SCu8( annotation.str, string_size ) );
                    }
                    
                    size = previous_size;
                    string_size--;
                }
                
                annotation.size = string_size;
            }
#endif
            ProfileCloseNow( get_string_advance_block );
            
            if ( size <= max_size ) {
                {
                    ProfileBlockNamed( app, "vertical scope annotation draw preparation", draw_prep );
                    
                    if ( highlight.min > ( i64 ) annotation.size - 1 ) {
                        highlight.min = -1;
                    }
                    
                    if ( highlight.max > ( i64 ) annotation.size ) {
                        highlight.max = annotation.size;
                    }
                    
                    Vec2_f32 position = { 0, 0 };
                    Vec2_f32 delta = { 0, 0 };
                    
                    u32 orientation = flags & vertical_scope_annotation_orientation_mask;
                    
                    // @note(tebtro)
                    float vertical_scope_annotation_x_centering = 0.1f;
                    
                    if ( orientation == vertical_scope_annotation_flag_top_to_bottom ) {
                        
                        float y = y0 + ( max_size - size ) * 0.5f;
                        float x_centering = ( ( indent_space_count * metrics.space_advance ) - metrics.line_height ) * vertical_scope_annotation_x_centering;
                        i64 space_count = indent_space_count * ( ranges.count - 1 - i );
                        float x = x_start + space_count * metrics.space_advance + metrics.line_height + x_centering;
                        position = { x, y };
                        delta = V2f32( 0.0f, 1.0f );
                        
                    } else {
                        
                        float y = y1 - ( max_size - size ) * 0.5f;
                        float x_centering = ( ( indent_space_count * metrics.space_advance ) - metrics.line_height ) * (1.0f - vertical_scope_annotation_x_centering);
                        i64 space_count = indent_space_count * ( ranges.count - 1 - i );
                        float x = x_start + space_count * metrics.space_advance + x_centering;
                        position = { x, y };
                        delta = V2f32( 0.0f, -1.0f );
                    }
                    
                    String_Const_u8 str_1, str_2, str_3;
                    
                    if ( highlight.min >= 0 && ( flags & vertical_scope_annotation_flag_highlight ) ) {
                        str_1 = SCu8( annotation.str, highlight.min );
                        str_2 = SCu8( str_1.str + str_1.size, highlight.max - highlight.min );
                        str_3 = SCu8( str_2.str + str_2.size, annotation.size - ( str_1.size + str_2.size ) );
                    } else {
                        str_1 = SCu8( annotation.str, annotation.size );
                        str_2 = str_3 = SCu8( ( u8* ) 0, ( u64 ) 0 );
                    }
                    
                    ProfileCloseNow( draw_prep );
                    
                    ProfileBlockNamed( app, "vertical scope annotation draw strings", draw_strings );
                    
                    draw_string_oriented( app, face_id, finalize_color( vertical_scope_annotation_text, 0 ), str_1, position, 0, delta );
                    
                    if ( str_2.size ) {
                        f32 str_size = get_string_advance( app, face_id, str_1 );
                        position.y += delta.y * str_size;
                        draw_string_oriented( app, face_id, finalize_color( vertical_scope_annotation_highlight, 0 ), str_2, position, 0, delta );
                    }
                    
                    if ( str_3.size ) {
                        f32 str_size = get_string_advance( app, face_id, str_2 );
                        position.y += delta.y * str_size;
                        draw_string_oriented( app, face_id, finalize_color( vertical_scope_annotation_text, 0 ), str_3, position, 0, delta );
                    }
                    
                    ProfileCloseNow( draw_strings );
                }
                
                // @note(tebtro): Added draw horizontal annotation at scope close
                {
                    ProfileBlockNamed( app, "scope close annotation draw preparation", draw_scope_close_prep );
                    
                    if ( scope_close_highlight.min > ( i64 ) scope_close_annotation.size - 1 ) {
                        scope_close_highlight.min = -1;
                    }
                    
                    if ( scope_close_highlight.max > ( i64 ) scope_close_annotation.size ) {
                        scope_close_highlight.max = scope_close_annotation.size;
                    }
                    
                    String_Const_u8 str_1, str_2, str_3;
                    
                    if ( scope_close_highlight.min >= 0 && ( flags & vertical_scope_annotation_flag_highlight ) ) {
                        str_1 = SCu8( scope_close_annotation.str, scope_close_highlight.min );
                        str_2 = SCu8( str_1.str + str_1.size, scope_close_highlight.max - scope_close_highlight.min );
                        str_3 = SCu8( str_2.str + str_2.size, scope_close_annotation.size - ( str_1.size + str_2.size ) );
                    } else {
                        str_1 = SCu8( scope_close_annotation.str, scope_close_annotation.size );
                        str_2 = str_3 = SCu8( ( u8* ) 0, ( u64 ) 0 );
                    }
                    
                    ProfileCloseNow( draw_scope_close_prep );
                    
                    ProfileBlockNamed(app, "scope close annotation draw strings", draw_scope_close_strings);
                    
                    i64 close_line_end = get_line_end_pos(app, buffer, get_line_number_from_pos(app, buffer, range.end));
                    if (close_line_end <= visible_range.max) {
                        Rect_f32 close_scope_rect = text_layout_character_on_screen( app, text_layout_id, close_line_end);
                        Vec2_f32 close_scope_pos  = { close_scope_rect.x1, close_scope_rect.y0 };
                        
                        String_Const_u8 arrow_string = string_u8_litexpr("<- ");
                        draw_string(app, face_id, arrow_string, close_scope_pos, finalize_color(vertical_scope_annotation_text, 0));
                        close_scope_pos.x += get_string_advance(app, face_id, arrow_string);
                        
                        draw_string(app, face_id, str_1, close_scope_pos, finalize_color(vertical_scope_annotation_text, 0));
                        
                        if (str_2.size) {
                            close_scope_pos.x += get_string_advance(app, face_id, str_1);
                            draw_string(app, face_id, str_2, close_scope_pos, finalize_color(vertical_scope_annotation_highlight, 0));
                        }
                        
                        if (str_3.size) {
                            close_scope_pos.x += get_string_advance(app, face_id, str_2);
                            draw_string(app, face_id, str_3, close_scope_pos, finalize_color(vertical_scope_annotation_text, 0));
                        }
                    }
                    
                    ProfileCloseNow(draw_scope_close_strings);
                }
            }
            
            if ( override_next_range ) {
                ranges.ranges[ i ] = override_range;
                i++;
            }
        }
    }
    
    ProfileCloseNow( ranges_loop );
}
