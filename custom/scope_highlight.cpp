//
// @important: MODULE from
// https://4coder.handmade.network/wiki/7384-%5Bmodule%5D_scope_highlight
//
// @note(tebtro) Modifies:
// Don't draw range if start and end are on the same line. Otherwise there are visual bugs when the line wraps.
// Added option to include brace lines in drawing.
// Vertical line width depends on font_metric.normal_advance.
//


/*
# Description

This module provides scope highlighting by drawing colored lines or blocks aligned to the
scope indentation of the scopes surrounding the cursor.

# Version

This was tested with 4coder 4.1.6.

# Setup

Make sure that 'use_scope_highlight' is set to true in 'config.4coder'.

You need to #include this file in your custom layer ('4coder_default_bindings.cpp' by
default) below the line:

'''c
#include "4coder_default_include.cpp"
'''

You need to call the function 'scope_highlight_draw' from 'default_render_buffer' in
'4coder_default_hooks.cpp' instead of 'draw_scope_highlight'. The function has a slightly
different signature: it doesn't take 'colors' and 'color_count' as arguments, instead it
takes the view id and a boolean to choose between line and block mode.

The call should look like this:

'''c
scope_highlight_draw( app, buffer, text_layout_id, cursor_pos, view_id, false );
'''

You'll need to insert the following function prototype above the declaration of
'default_render_buffer':

'''c
function void scope_highlight_draw( Application_Links *app, Buffer_ID buffer, Text_Layout_ID text_layout_id, i64 pos, View_ID view, b32 block_mode );
'''

The line highlight, by default will be drawn on top of the scope highlight. To change that
you can move the code that draws the line highlight above the code that draws the scope
highlight in 'default_render_buffer'. Note that this works great in 'line mode', but in
'block mode' it will make the line highlight invisible, since the blocks will be drawn on
top of it.

Add those lines in your theme file to change the colors.

'''c
scope_highlight_line_cycle = { 0xfff048be, 0xfff07a48, 0xffbef048, 0xff48f07a, 0xff48bef0, 0xff7a48f0 };
scope_highlight_block_cycle = { 0x3ff048be, 0x3ff07a48, 0x3fbef048, 0x3f48f07a, 0x3f48bef0, 0x3f7a48f0 };
'''

# Issues

If all scopes are colored colored purple, make sure the theme file is loaded. The full
theme file name needs to be set in 'config.4coder', which by default isn't the case. Use :

'''c
default_theme_name = "theme-4coder";
'''

instead of

'''c
default_theme_name = "4coder";
'''

# Details

## Prefix

This module uses 'scope_highlight' as a prefix for its types and functions.

## Ids

This module declares the following managed ids:

In the 'colors' group:
- 'scope_highlight_line_cycle';
- 'scope_highlight_block_cycle';

## Modified files

This module needs modifications in the following files to work:
- '4coder_default_bindings.h' or your custom layer file;
- '4coder_default_hooks.h';

## Global variables

This module doesn't use any global variables.

## Types

This module doesn't declare any types.

## Functions

This module declare the following function:
- 'scope_highlight_draw';

## Scope attachment

This module doesn't declare any scope attachment.
*/

CUSTOM_ID( colors, scope_highlight_line_cycle );
CUSTOM_ID( colors, scope_highlight_block_cycle );

function void scope_highlight_draw( Application_Links *app, Buffer_ID buffer, Text_Layout_ID text_layout_id, i64 pos, View_ID view, b32 block_mode ) {
    // @note(tebtro)
    b32 include_brace_lines = true;
    
    ProfileScope( app, "scope_highlight" );
    
    Scratch_Block scratch( app );
    
    ProfileBlockNamed( app, "get enclosure ranges", scope_highlight_get_enclosure_range );
    Range_i64_Array ranges = get_enclosure_ranges( app, scratch, buffer, pos, FindNest_Scope );
    ProfileCloseNow( scope_highlight_get_enclosure_range );
    
    if ( ranges.count ) {
        
        Face_ID face_id = get_face_id( app, buffer );
        Face_Metrics metrics = get_face_metrics( app, face_id );
        Rect_f32 region = text_layout_region( app, text_layout_id );
        Range_i64 visible_range = text_layout_get_visible_range( app, text_layout_id );
        
        i64 indent_space_count = global_config.indent_width;
        
        if ( global_config.enable_virtual_whitespace ) {
            indent_space_count = global_config.virtual_whitespace_regular_indent;
        }
        
        Buffer_Scroll scroll = view_get_buffer_scroll( app, view );
        f32 x_start = region.x0 - scroll.position.pixel_shift.x;
        
        Color_Array colors = finalize_color_array( scope_highlight_line_cycle );
        i32 color_index = 0;
        
        if ( block_mode ) {
            colors = finalize_color_array( scope_highlight_block_cycle );
        }
        
        ProfileScope( app, "Ranges loop" );
        
        for ( i32 i = ranges.count - 1; i >= 0; i -= 1 ) {
            
            Range_i64 range = ranges.ranges[ i ];
            
            i64 start = range.min;
            i64 end = range.max - 1;
            
            i64 start_line = get_line_number_from_pos( app, buffer, start );
            i64 end_line = get_line_number_from_pos( app, buffer, end );
            
            Rect_f32 rect = { 0 };
            
            // @note(tebtro): Don't draw if on the same line.
            b32 draw = true;
            
            if ( start_line == end_line ) {
                
                /*
                                Rect_f32 min_rect = text_layout_character_on_screen( app, text_layout_id, start );
                                Rect_f32 max_rect = text_layout_character_on_screen( app, text_layout_id, end );
                                rect = Rf32( min_rect.x1, min_rect.y1 + 1, max_rect.x0, min_rect.y1 + 2 );
                */
                
                draw = false;
                
            } else {
                
                i64 space_count = indent_space_count * ( ranges.count - 1 - i );
                
                /* +1 to avoid having the first line against the panel border. */
                rect.x0 = x_start + 1 + space_count * metrics.space_advance;
                
                if ( block_mode ) {
                    rect.x1 = region.x1;
                } else {
                    rect.x1 = rect.x0 + (metrics.normal_advance * 0.1f); // @note(tebtro) + 1;
                }
                
                if ( range.max - 1 > visible_range.min && range.min < visible_range.max ) {
                    
                    // i64 unwrapped_start = get_start_of_textual_line_pos_from_pos( app, view, buffer, range.min );
                    
                    /* Get the start position of the textual line. */
                    i64 new_pos = get_line_side_pos_from_pos( app, buffer, range.min, Side_Min );
                    Buffer_Cursor cursor = view_compute_cursor( app, view, seek_pos( new_pos ) );
                    Vec2_f32 p = view_relative_xy_of_pos( app, view, cursor.line, cursor.pos );
                    i64 unwrapped_start = view_pos_at_relative_xy( app, view, cursor.line, p );
                    
                    if ( unwrapped_start < visible_range.min ) {
                        unwrapped_start = range.min;
                    }
                    
                    if ( unwrapped_start < visible_range.min ) {
                        rect.y0 = region.y0;
                    } else  {
                        Rect_f32 r = text_layout_character_on_screen( app, text_layout_id, unwrapped_start );
                        rect.y0 = (include_brace_lines) ? r.y0 : r.y1;
                    }
                    
                    if ( range.max - 1 > visible_range.max ) {
                        rect.y1 = region.y1;
                    } else  {
                        Rect_f32 r = text_layout_character_on_screen( app, text_layout_id, range.max - 1 );
                        rect.y1 = (include_brace_lines) ? r.y1 : r.y0;
                    }
                }
            }
            
            // @note(tebtro): Draw the line a bit further in the y-directions.
            if ( !block_mode ) {
                if (include_brace_lines) {
                    rect.y0 += metrics.line_height * 0.5f;
                    rect.y1 -= metrics.line_height * 0.4f;
                }
                else {
                    rect.y0 -= metrics.line_height * 0.5f;
                    rect.y1 += metrics.line_height * 0.6f;
                }
            }
            
            if ( draw ) {
                if ( rect.x0 < rect.x1 && rect.y0 < rect.y1 ) {
                    draw_rectangle( app, rect, 0, colors.vals[ color_index % colors.count ] );
                }
            }
            
            color_index += 1;
        }
    }
}
