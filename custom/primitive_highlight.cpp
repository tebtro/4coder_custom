
//
// @important: MODULE from
// https://4coder.handmade.network/wiki/7370-%5Bmodule%5D_primitive_highlight
//
// @note(tebtro) Modifies:
// Use my custom get_token_color_cpp function.
//


/*
# Description

This module provides text highlighting of types, functions and macros (as of 4coder 4.1.6,
4coder commands are not supported) by coloring them with a user specified color. For an
identifier to be recognized as a type, function or macro, the file declaring it must be
loaded.

# Version

This was tested with 4coder 4.1.6.

# Setup

You need to #include this file in your custom layer ('4coder_default_bindings.cpp' by
default) below the line:

'''c
#include "4coder_default_include.cpp"
'''

You need to call the function 'primitive_highlight_draw_cpp_token_colors' from
'default_render_buffer' in '4coder_default_hooks.cpp' instead of 'draw_cpp_token_colors'. The
function takes one extra argument which is the buffer that is currently being drawn. The
call should look like this:

'''c
primitive_highlight_draw_cpp_token_colors( app, text_layout_id, &token_array, buffer );
'''

You'll need to insert the following function prototype above the declaration of
'default_render_buffer':

'''c
function void primitive_highlight_draw_cpp_token_colors( Application_Links *app, Text_Layout_ID text_layout_id, Token_Array *array, Buffer_ID buffer );
'''

Add those lines in your theme file to change the colors.

'''c
primitive_highlight_type = 0xffaaaaef;
primitive_highlight_function = 0xffbbbbbb;
primitive_highlight_macro = 0xffa46391;
primitive_highlight_4coder_command = 0xffffffff;
'''

You can define the order in which primitive types are chosen in case of collisions (same
identifier names with different types) by modifying the function
'primitive_highlight_get_note' below. Modify the 'type_weight' array at the top of the
function to assign a higher value to the primitive type you want to have a higher
priority. By default macro are chosen first, then types then functions (then commands).

# Performance

Compiling in 'release' mode (passing 'release' as the second argument to 'buildsuper' script)
will greatly improve performances. If you have lots of files opened in 4coder this might
be necessary to maintain a decent frame rate.

Test:
- 4coder 4.1.6 out of the box with only this module modifications;
- Window maximized, 1920 * 1080 resolution;
- Windows 7 64bit in classic mode (compositor is of);
- 'Alt + X' (command_lister) > 'load_project' (to load all 4coder source files, ~150 files);
- 'Alt + X' > profile_enable;
- 'Control + i' (interactive_switch_buffer) > 4coder_draw.cpp;
- Hold 'Control + Down arrow' until the bottom of the file is reached;
- 'Alt + X' > profile_disable;
- 'Alt + X' > profile_inspect;
- Select main;

CPU: Intel Core i7 860 2.8 Ghz
- debug mode worst time 0.0112 ms;
- release mode wost time 0.0045 ms;

Avoiding recreating the 'hashes_notes' array each frame if it's not necessary would save
about half the time (wouldn't help with worst case scenario).

# Issues

If types, functions and macros are colored purple, make sure the theme file is loaded. The
full theme file name needs to be set in 'config.4coder', which by default isn't the case.
Use :

'''c
default_theme_name = "theme-4coder";
'''

instead of

'''c
default_theme_name = "4coder";
'''

# High level overview

At the start of 'primitive_highlight_draw_cpp_token_colors' an array containing the
pointers to 'Code_Index_Note's and hashes of their strings is built, then sorted using
ascending hashes values. 'Code_Index_Note's are information provided by 4coder about types,
functions and macros that are attached to each "code" buffer. Searching each buffer
'Code_Index_Note's is slow, that's why we build a single big array containing all notes
pointers.

When rendering CPP tokens, we use a binary search to find out if an identifier is a type,
function or macro and color it.

# Details

## Prefix

This module uses 'primitive_highlight_' as a prefix for its types and functions.

## Ids

This module declares the following managed ids:

In the 'colors' group:
- 'primitive_highlight_type';
- 'primitive_highlight_function';
- 'primitive_highlight_macro';
- 'primitive_highlight_4coder_command';

## Modified files

This module needs modification in the following files to work:
- '4coder_default_bindings.h' or your custom layer file;
- '4coder_default_hooks.h';

## Global variables

This module doesn't use any global variables.

## Types

This module declare the following types:
- 'primitive_highlight_hashes_notes_t';

## Functions

This module declare the following functions:
- 'primitive_highlight_quick_sort_hashes_notes';
- 'primitive_highlight_create_big_note_array';
- 'primitive_highlight_get_note';
- 'primitive_highlight_draw_cpp_token_colors';

## Scope attachment

This module doesn't declare any scope attachment.
*/

CUSTOM_ID( colors, primitive_highlight_type );
CUSTOM_ID( colors, primitive_highlight_function );
CUSTOM_ID( colors, primitive_highlight_macro );
CUSTOM_ID( colors, primitive_highlight_4coder_command );

/* NOTE: end is the index of the last item, NOT one past the last item. */
function void primitive_highlight_quick_sort_hashes_notes( u64* hashes, Code_Index_Note** notes, u64 start, u64 end ) {
    
    if ( hashes && start < end ) {
        
        u64 pivot_index = ( start + end ) / 2;
        u64 pivot_hash = hashes[ pivot_index ];
        
        u64 i = start, j = end;
        
        while ( 1 ) {
            
            while ( hashes[ i ] < pivot_hash ) {
                i++;
            }
            
            while ( hashes[ j ] > pivot_hash ) {
                j--;
            }
            
            if ( i < j ) {
                
                u64 hash_temp = hashes[ i ];
                hashes[ i ] = hashes[ j ];
                hashes[ j ] = hash_temp;
                
                Code_Index_Note* temp = notes[ i ];
                notes[ i ] = notes[ j ];
                notes[ j ] = temp;
                
                i++;
                j--;
                
            } else {
                
                break;
            }
        }
        
        primitive_highlight_quick_sort_hashes_notes( hashes, notes, start, j );
        primitive_highlight_quick_sort_hashes_notes( hashes, notes, j + 1, end );
    }
}

typedef struct primitive_highlight_hashes_notes_t {
    u64* hashes;
    Code_Index_Note** notes;
    i32 count;
} primitive_highlight_hashes_notes_t;

function primitive_highlight_hashes_notes_t primitive_highlight_create_big_note_array( Application_Links* app, Arena* arena ) {
    
    ProfileScope( app, "create_big_note_array" );
    
    primitive_highlight_hashes_notes_t hashes_notes = { 0 };
    
    Buffer_ID buffer_it = get_buffer_next( app, 0, Access_Always );
    
#if 0
    /* NOTE: locking the index only in this function seems to cost more than locking in the primitive_highlight_draw_cpp_token_colors function. */
    code_index_lock( );
#endif
    
    while ( buffer_it ) {
        
        Code_Index_File *file = code_index_get_file( buffer_it );
        
        if ( file ) {
            hashes_notes.count += file->note_array.count;
        }
        
        buffer_it = get_buffer_next( app, buffer_it, Access_Always );
    }
    
    hashes_notes.hashes = push_array( arena, u64, hashes_notes.count );
    hashes_notes.notes = push_array( arena, Code_Index_Note*, hashes_notes.count );
    
    i32 count = 0;
    
    {
        ProfileScope( app, "create hashes" );
        
        buffer_it = get_buffer_next( app, 0, Access_Always );
        
        while ( buffer_it ) {
            
            Code_Index_File *file = code_index_get_file( buffer_it );
            
            if ( file ) {
                
                for ( i32 i = 0; i < file->note_array.count; i++ ) {
                    hashes_notes.notes[ count ] = file->note_array.ptrs[ i ];
                    hashes_notes.hashes[ count ] = table_hash_u8( hashes_notes.notes[ count ]->text.str, hashes_notes.notes[ count ]->text.size );
                    count++;
                }
            }
            
            buffer_it = get_buffer_next( app, buffer_it, Access_Always );
        }
    }
    
#if 0
    code_index_unlock( );
#endif
    
    if ( count ) {
        ProfileScope( app, "quick_sort_hashes_notes" );
        primitive_highlight_quick_sort_hashes_notes( hashes_notes.hashes, hashes_notes.notes, 0, count - 1 );
    }
    
    return hashes_notes;
}

function Code_Index_Note* primitive_highlight_get_note( Application_Links* app, primitive_highlight_hashes_notes_t* hashes_notes, String_Const_u8 name ) {
    
    u8 type_weight[ 4 ];
    type_weight[ CodeIndexNote_4coderCommand ] = 1;
    type_weight[ CodeIndexNote_Function ] = 2;
    type_weight[ CodeIndexNote_Type ] = 3;
    type_weight[ CodeIndexNote_Macro ] = 4;
    
    ProfileScope( app, "get_note" );
    
    Code_Index_Note* result = 0;
    
    u64 name_hash = table_hash_u8( name.str, name.size );
    
    i32 start = 0;
    i32 end = hashes_notes->count - 1;
    
    while ( start <= end ) {
        
        i32 midle = ( start + end ) / 2;
        
        u64 note_hash = hashes_notes->hashes[ midle ];
        
        if ( name_hash < note_hash ) {
            end = midle - 1;
        } else if ( name_hash > note_hash ) {
            start = midle + 1;
        } else {
            
            ProfileBlockNamed( app, "solve collisions", solve_collisions );
            
            while ( midle - 1 >= start && hashes_notes->hashes[ midle - 1 ] == name_hash ) {
                midle--;
            }
            
            u8 current_weight = 0;
            
            while ( midle <= end && hashes_notes->hashes[ midle ] == name_hash ) {
                
                Code_Index_Note* note = hashes_notes->notes[ midle ];
                // Assert( note->kind < ArrayCount( type_weight ) );
                
                if ( type_weight[ note->note_kind ] > current_weight ) {
                    
                    if ( string_compare( name, note->text ) == 0 ) {
                        
                        current_weight = type_weight[ note->note_kind ];
                        result = note;
                        
                        if ( current_weight == 4 ) {
                            break;
                        }
                    }
                }
                
                midle++;
            }
            
            ProfileCloseNow( solve_collisions );
            
            break;
        }
    }
    
    return result;
}

/* NOTE: This funciton is a modification of 'draw_cpp_token_colors' from '4coder_draw.cpp'. */
function void primitive_highlight_draw_cpp_token_colors( Application_Links *app, Text_Layout_ID text_layout_id, Token_Array *array, Buffer_ID buffer ) {
    
    Range_i64 visible_range = text_layout_get_visible_range( app, text_layout_id );
    i64 first_index = token_index_from_pos( array, visible_range.first );
    Token_Iterator_Array it = token_iterator_index( 0, array, first_index );
    
    /* NOTE: Start modification. */
    Scratch_Block scratch( app );
    
#if 1
    code_index_lock( );
#endif
    
    Temp_Memory notes_temp = begin_temp( scratch );
    primitive_highlight_hashes_notes_t hashes_notes = primitive_highlight_create_big_note_array( app, scratch );
    
    ProfileBlockNamed( app, "token loop", token_loop );
    /* NOTE: End modification. */
    
    for ( ; ; ) {
        
        Token *token = token_it_read( &it );
        
        if ( token->pos >= visible_range.one_past_last ){
            break;
        }
        
        /* NOTE: Start modification. */
        FColor color = fcolor_id( defcolor_text_default );
        b32 colored = false;
        
        if ( token->kind == TokenBaseKind_Identifier ) {
            
            Temp_Memory temp = begin_temp( scratch );
            String_Const_u8 lexeme = push_token_lexeme( app, scratch, buffer, token );
            Code_Index_Note* note = primitive_highlight_get_note( app, &hashes_notes, lexeme );
            end_temp( temp );
            
            if ( note ) {
                
                switch ( note->note_kind ) {
                    case CodeIndexNote_Type: {
                        color = fcolor_id( primitive_highlight_type );
                    } break;
                    
                    case CodeIndexNote_Function: {
                        color = fcolor_id( primitive_highlight_function );
                    } break;
                    
                    case CodeIndexNote_Macro: {
                        color = fcolor_id( primitive_highlight_macro );
                    } break;
                    
                    case CodeIndexNote_4coderCommand: {
                        /* NOTE: 4coder doesn't create those notes as of 4.1.6. */
                        color = fcolor_id( primitive_highlight_4coder_command );
                    } break;
                }
                
                colored = true;
                
#if 1
                if ( note->note_kind == CodeIndexNote_Type ) {
                    
                    Token_Iterator_Array dot_arrow_it = it;
                    
                    if ( token_it_dec_non_whitespace( &dot_arrow_it ) ) {
                        
                        Token* dot_arrow = token_it_read( &dot_arrow_it );
                        
                        if ( dot_arrow->kind == TokenBaseKind_Operator && ( dot_arrow->sub_kind == TokenCppKind_Dot || dot_arrow->sub_kind == TokenCppKind_Arrow ) ) {
                            colored = false;
                        }
                    }
                }
#endif
            }
        }
        
        if( !colored ) {
            //color = get_token_color_cpp( *token );
            color = tebtro_get_token_color_cpp( app, buffer, *token );
        }
        /* NOTE: End modification. */
        
        ARGB_Color argb = fcolor_resolve( color );
        paint_text_color( app, text_layout_id, Ii64_size( token->pos, token->size ), argb );
        
        if ( !token_it_inc_all( &it ) ){
            break;
        }
    }
    
#if 1
    code_index_unlock( );
#endif
    
    /* NOTE: Start modification. */
    ProfileCloseNow( token_loop );
    end_temp( notes_temp );
    /* NOTE: End modification. */
}
