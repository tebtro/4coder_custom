#ifdef FLEURY_CALC
#include <stdlib.h>
#include <string.h>
#include "4coder_fleury_utilities.cpp"
#include "4coder_fleury_ubiquitous.cpp"
#include "4coder_fleury_plot.cpp"
#include "4coder_fleury_calc.cpp"
#endif



CUSTOM_COMMAND_SIG(tebtro_startup)
CUSTOM_DOC("Tebtro custom command for responding to a startup event")
{
    ProfileScope(app, "tebtro_startup");
    User_Input input = get_current_input(app);
    if (!match_core_code(&input, CoreCode_Startup))  return;
    
    String_Const_u8_Array file_names = input.event.core.file_names;
    load_themes_default_folder(app);
    default_4coder_initialize(app, file_names);
    default_4coder_side_by_side_panels(app, file_names);
    Buffer_ID scratch_buffer_id = get_buffer_by_name(app, string_u8_litexpr("*scratch*"), Access_Always);
    if (scratch_buffer_id != 0) {
        i64 size = buffer_get_size(app, scratch_buffer_id);
        if (size == 0){
            String_Const_u8 message = SCu8("Unfortunately, there’s a radio connected to my brain.");
            buffer_replace_range(app, scratch_buffer_id, Ii64(0, size), message);
        }
    }
    if (global_config.automatically_load_project){
        load_project(app);
    }
    
    vim_setup_mode_and_chord_color_tables(app);
    
    // @note Setup fonts
    {
        // @todo Fonts are not updated when changing the font size with the mouse wheel.
        
        Scratch_Block scratch(app);
        String_Const_u8 bin_path = system_get_path(scratch, SystemPath_Binary);
        Face_Description default_face_desc = get_face_description(app, get_face_id(app, 0));
        
        Face_Description desc = {0};
        desc.font.file_name =  push_u8_stringf(scratch, "%.*sfonts/SourceCodePro-Regular.ttf", string_expand(bin_path));
        desc.parameters.pt_size = default_face_desc.parameters.pt_size;
        desc.parameters.underline = 1;
        global_underlined_face_id = try_create_new_face(app, &desc);
        
        desc.font.file_name =  push_u8_stringf(scratch, "%.*sfonts/SourceCodePro-Bold.ttf", string_expand(bin_path));
        desc.parameters.underline = 0;
        desc.parameters.bold = 1;
        global_bold_face_id = try_create_new_face(app, &desc);
        
        // desc.font.file_name =  push_u8_stringf(scratch, "%.*sfonts/SourceCodePro-Light.ttf", string_expand(bin_path));
        desc.font.file_name =  push_u8_stringf(scratch, "%.*sfonts/Consolas-Italic.ttf", string_expand(bin_path));
        desc.parameters.bold = 0;
        desc.parameters.italic = 1;
        global_italic_face_id = try_create_new_face(app, &desc);
    }
    
#if CALC_PLOT
    // @note: Open calc buffer
    Buffer_ID calc_buffer = create_buffer(app, string_u8_litexpr("*calc*"), BufferCreate_NeverAttachToFile | BufferCreate_AlwaysNew);
    buffer_set_setting(app, calc_buffer, BufferSetting_Unimportant, true);
    buffer_set_setting(app, calc_buffer, BufferSetting_Unkillable, true);
#endif
    
#if !defined(BUILD_DEBUG)
    system_set_fullscreen(true);
#endif
}

CUSTOM_COMMAND_SIG(tebtro_view_input_handler)
CUSTOM_DOC("Input consumption loop for default view behavior")
{
    Thread_Context *tctx = get_thread_context(app);
    Scratch_Block scratch(tctx);
    
    //
    // @note View initialization, begin_view
    //
    {
        View_ID view_id = get_this_ctx_view(app, Access_Always);
        String_Const_u8 name = push_u8_stringf(scratch, "view %d", view_id);
        
        Profile_Global_List *list = get_core_profile_list(app);
        ProfileThreadName(tctx, list, name);
        
        View_Context ctx = view_current_context(app, view_id);
        ctx.mapping = &framework_mapping;
        ctx.map_id = mapid_global;
        view_alter_context(app, view_id, &ctx);
        
        Managed_Scope view_scope = view_get_managed_scope(app, view_id);
        Vim_View_State *vim_state = scope_attachment(app, view_scope, view_vim_state_id, Vim_View_State);
        if (vim_state->execute_command_count == 0) { // :vim_view_state_is_initialized
            *vim_state = {};
            
            vim_state->chord_bar.prompt = string_u8_litexpr("");
            vim_state->chord_bar.string = SCu8(vim_state->chord_bar_string_space, (u64)0);
            vim_state->chord_bar.string_capacity = sizeof(vim_state->chord_bar_string_space);
            
            // vim_state->is_initialized = true;
        }
    }
    
    //
    // @note Handle input
    //
    for (;;) {
        // NOTE(allen): Get the binding from the buffer's current map
        User_Input input = get_next_input(app, EventPropertyGroup_Any, 0);
        ProfileScopeNamed(app, "before view input", view_input_profile);
        if (input.abort) {
            break;
        }
        
        Event_Property event_properties = get_event_properties(&input.event);
        
        // :suppress_mouse
        if ((event_properties & EventPropertyGroup_AnyMouseEvent) != 0) {
            set_mouse_suppression(false);
            vim_global_mouse_last_event_time = 1.0f; // in seconds
        }
        if (suppressing_mouse && (event_properties & EventPropertyGroup_AnyMouseEvent) != 0) {
            continue;
        }
        
#if 0
        if ((event_properties & EventPropertyGroup_AnyKeyboardEvent) != 0) {
            int break_here = 0;
            break_here = 0;
        }
#endif
        
        View_ID view_id = get_this_ctx_view(app, Access_Always);
        Managed_Scope view_scope = view_get_managed_scope(app, view_id);
        Command_Map_ID *map_id_ptr = scope_attachment(app, view_scope, view_map_id, Command_Map_ID);
        if (*map_id_ptr == 0) {
            *map_id_ptr = mapid_vim_mode_normal;
        }
        Command_Map_ID map_id = *map_id_ptr;
        
        Command_Binding binding = map_get_binding_recursive(&framework_mapping, map_id, &input.event);
        
        if (binding.custom == 0) {
            // NOTE(allen): we don't have anything to do with this input,
            // leave it marked unhandled so that if there's a follow up
            // event it is not blocked.
            leave_current_input_unhandled(app);
        }
        else {
            // NOTE(allen): before the command is called do some book keeping
            Rewrite_Type *next_rewrite =
                scope_attachment(app, view_scope, view_next_rewrite_loc, Rewrite_Type);
            *next_rewrite = Rewrite_None;
            if (fcoder_mode == FCoderMode_NotepadLike){
                for (View_ID view_it = get_view_next(app, 0, Access_Always);
                     view_it != 0;
                     view_it = get_view_next(app, view_it, Access_Always)){
                    Managed_Scope scope_it = view_get_managed_scope(app, view_it);
                    b32 *snap_mark_to_cursor =
                        scope_attachment(app, scope_it, view_snap_mark_to_cursor,
                                         b32);
                    *snap_mark_to_cursor = true;
                }
            }
            
            ProfileCloseNow(view_input_profile);
            
            // @note: Call the command
#if USE_MULTIPLE_CURSORS
            vim_multiple_cursor_handle_input_call_command(app, view_id, binding);
#else
            binding.custom(app);
#endif
            
            // NOTE(allen): after the command is called do some book keeping
            ProfileScope(app, "after view input");
            
            next_rewrite = scope_attachment(app, view_scope, view_next_rewrite_loc, Rewrite_Type);
            if (next_rewrite != 0) {
                Rewrite_Type *rewrite =
                    scope_attachment(app, view_scope, view_rewrite_loc, Rewrite_Type);
                *rewrite = *next_rewrite;
                if (fcoder_mode == FCoderMode_NotepadLike) {
                    for (View_ID view_it = get_view_next(app, 0, Access_Always);
                         view_it != 0;
                         view_it = get_view_next(app, view_it, Access_Always)) {
                        Managed_Scope scope_it = view_get_managed_scope(app, view_it);
                        b32 *snap_mark_to_cursor =
                            scope_attachment(app, scope_it, view_snap_mark_to_cursor, b32);
                        if (*snap_mark_to_cursor) {
                            i64 pos = view_get_cursor_pos(app, view_it);
                            view_set_mark(app, view_it, seek_pos(pos));
                        }
                    }
                }
            }
        }
    }
}

BUFFER_HOOK_SIG(tebtro_begin_buffer) {
    ProfileScope(app, "begin buffer");
    
    Managed_Scope buffer_scope = buffer_get_managed_scope(app, buffer_id);
    Scratch_Block scratch(app);
    
    b32 treat_as_code = false;
    String_Const_u8 file_name = push_buffer_file_name(app, scratch, buffer_id);
    if (file_name.size > 0){
        String_Const_u8 ext = string_file_extension(file_name);
        b32 file_is_c = false;
        b32 file_is_c_like = false;
        
        // @note Check if treat file as code
        // @note And check if file is c or c like
        String_Const_u8_Array extensions = global_config.code_exts;
        for (i32 i = 0; i < extensions.count; ++i) {
            if (string_match(ext, extensions.strings[i])) {
                
                if (string_match(ext, string_u8_litexpr("cpp")) ||
                    string_match(ext, string_u8_litexpr("h"))   ||
                    string_match(ext, string_u8_litexpr("c"))   ||
                    string_match(ext, string_u8_litexpr("hpp")) ||
                    string_match(ext, string_u8_litexpr("cc"))  ) {
                    treat_as_code = true;
                    file_is_c = file_is_c_like = true;
                }
                else if (string_match(ext, string_u8_litexpr("jai"))  ||
                         string_match(ext, string_u8_litexpr("rs"))   ||
                         string_match(ext, string_u8_litexpr("cs"))   ||
                         string_match(ext, string_u8_litexpr("java")) ) {
                    file_is_c_like = true;
                }
                
                break;
            }
        }
        
        b32 *file_is_c_ptr      = scope_attachment(app, buffer_scope, buffer_file_is_c,      b32);
        b32 *file_is_c_like_ptr = scope_attachment(app, buffer_scope, buffer_file_is_c_like, b32);
        *file_is_c_ptr      = file_is_c;
        *file_is_c_like_ptr = file_is_c_like;
    }
    
#if 0
    Command_Map_ID map_id = (treat_as_code)?(mapid_code):(mapid_file);
    Command_Map_ID *map_id_ptr = scope_attachment(app, buffer_scope, buffer_map_id, Command_Map_ID);
    *map_id_ptr = map_id;
#endif
    
    Line_Ending_Kind setting = guess_line_ending_kind_from_buffer(app, buffer_id);
    Line_Ending_Kind *eol_setting = scope_attachment(app, buffer_scope, buffer_eol_setting, Line_Ending_Kind);
    *eol_setting = setting;
    
    // NOTE(allen): Decide buffer settings
    b32 wrap_lines = true;
    b32 use_virtual_whitespace = false;
    b32 use_lexer = false;
    if (treat_as_code){
        wrap_lines = global_config.enable_code_wrapping;
        use_virtual_whitespace = global_config.enable_virtual_whitespace;
        use_lexer = true;
    }
    
    String_Const_u8 buffer_name = push_buffer_base_name(app, scratch, buffer_id);
    if (string_match(buffer_name, string_u8_litexpr("*compilation*"))) {
        wrap_lines = false;
    }
    
    if (use_lexer) {
        ProfileBlock(app, "begin buffer kick off lexer");
        Async_Task *lex_task_ptr = scope_attachment(app, buffer_scope, buffer_lex_task, Async_Task);
        *lex_task_ptr = async_task_no_dep(&global_async_system, do_full_lex_async, make_data_struct(&buffer_id));
    }
    
    {
        b32 *wrap_lines_ptr = scope_attachment(app, buffer_scope, buffer_wrap_lines, b32);
        *wrap_lines_ptr = wrap_lines;
    }
    
    if (use_virtual_whitespace) {
        if (use_lexer) {
            buffer_set_layout(app, buffer_id, layout_virt_indent_index_generic);
        }
        else{
            buffer_set_layout(app, buffer_id, layout_virt_indent_literal_generic);
        }
    }
    else{
        buffer_set_layout(app, buffer_id, layout_generic);
    }
#if 0
    b32 wrap_anywhere = true;
    if (wrap_anywhere) {
        buffer_set_layout(app, buffer_id, layout_wrap_anywhere);
    }
#endif
    
    // no meaning for return
    return(0);
}

function void
tebtro_render_buffer(Application_Links *app, View_ID view_id, Face_ID face_id, Buffer_ID buffer_id, Text_Layout_ID text_layout_id, Rect_f32 view_region, Frame_Info frame_info, Vim_View_State *vim_state) {
    ProfileScope(app, "render buffer");
    
    View_ID active_view = get_active_view(app, Access_Always);
    b32 is_active_view = (active_view == view_id);
    Rect_f32 prev_clip = draw_set_clip(app, view_region);
    
    i64 cursor_pos = view_correct_cursor(app, view_id);
    i64 mark_pos   = view_correct_mark(app, view_id);
    
    // NOTE(allen): Cursor shape
    Face_Metrics face_metrics = get_face_metrics(app, face_id);
    f32 cursor_roundness = (face_metrics.normal_advance*0.5f)*0.9f;
    f32 mark_thickness = 2.f;
    
    Color_Array colors_back_cycle = finalize_color_array(defcolor_back_cycle);
    ARGB_Color  colors_back_cycle_adjusted[] = {0x0800A100, 0x0800AAFF, 0x0CFF7F00, 0x0CFF0000}; // blue value adjusted
    ARGB_Color  colors_back_cycle_brighter[] = {0xCF00FF00, 0xFF00AAFF, 0xEFFF7F00, 0xEFFF0000};
    Color_Array colors_text_cycle = finalize_color_array(defcolor_text_cycle);
    
    Token_Array token_array = get_token_array_from_buffer(app, buffer_id);
    
    // @note: Scope highlight
    if (global_config.use_scope_highlight) {
#if USE_RANGE_COLOR_START_DEFAULT
        draw_scope_highlight(app, buffer_id, text_layout_id, cursor_pos, colors_back_cycle.vals, colors_back_cycle.count);
#else
        tebtro_draw_scope_highlight(app, buffer_id, text_layout_id, cursor_pos, colors_back_cycle.vals, colors_back_cycle.count);
#endif
    }
    
    // @note Line highlight
    if (global_config.highlight_line_at_cursor && is_active_view) {
        i64 line_number = get_line_number_from_pos(app, buffer_id, cursor_pos);
        draw_line_highlight(app, text_layout_id, line_number, fcolor_id(defcolor_highlight_cursor_line));
    }
    
    // @note Draw whitespaces
    b64 show_whitespace = false;
    view_get_setting(app, view_id, ViewSetting_ShowWhitespace, &show_whitespace);
    if (show_whitespace) {
        // ARGB_Color argb = 0x0BFFFFFF; // 0xFF4B0082;
        ARGB_Color argb = fcolor_resolve(fcolor_id(defcolor_highlight));
        tebtro_draw_whitespaces(app, face_id, buffer_id, text_layout_id, &token_array, argb);
    }
    
    // @note Token colorizing
    if (token_array.tokens != 0) {
#if 1
        tebtro_draw_cpp_token_colors(app, text_layout_id, buffer_id, &token_array);
        tebtro_draw_cpp_identifier_colors(app, text_layout_id, buffer_id, &token_array);
#else
        tebtro_draw_cpp_token_colors__only_comments(app, text_layout_id, buffer_id, &token_array);
#endif
        
        // @note: Token under cursor highlight
        if (is_active_view) {
            tebtro_draw_token_under_cursor_highlight(app, text_layout_id, buffer_id, &token_array, cursor_pos, cursor_roundness);
        }
        
        // @note Scan for TODOs and NOTEs
        if (global_config.use_comment_keyword) {
#if 0
            Comment_Highlight_Pair pairs[] = {
                {string_u8_litexpr("NOTE"), finalize_color(defcolor_comment_pop, 0)},
                {string_u8_litexpr("TODO"), finalize_color(defcolor_comment_pop, 1)},
            };
            draw_comment_highlights(app, buffer_id, text_layout_id, &token_array, pairs, ArrayCount(pairs));
#else
            tebtro_draw_comment_highlights(app, buffer_id, text_layout_id, &token_array);
#endif
        }
    }
    else {
        Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
        paint_text_color_fcolor(app, text_layout_id, visible_range, fcolor_id(defcolor_text_default));
    }
    
    // @note Vim :view_changed_flash_line
    if (is_active_view && vim_global_view_changed_time > 0.0f) {
        i64 line_number = get_line_number_from_pos(app, buffer_id, cursor_pos);
        draw_line_highlight(app, text_layout_id, line_number, 0x5FD90BCC);
    }
    
    if (global_config.use_error_highlight || global_config.use_jump_highlight) {
        Buffer_ID compilation_buffer = get_comp_buffer(app);
        
        if (global_config.use_error_highlight) {
            // @note: Error highlight
            draw_jump_highlights(app, buffer_id, text_layout_id, compilation_buffer, fcolor_id(defcolor_highlight_junk));
            
            // @note: Draw error annotations
            tebtro_draw_error_annotations(app, buffer_id, text_layout_id, face_id, compilation_buffer);
        }
        
        // @note: Jump highlight
        if (global_config.use_jump_highlight) {
            Buffer_ID jump_buffer = get_locked_jump_buffer(app);
            if (jump_buffer != compilation_buffer) {
                draw_jump_highlights(app, buffer_id, text_layout_id, jump_buffer, fcolor_id(defcolor_highlight_white));
            }
        }
    }
    
    // @note: Color parens
    if (global_config.use_paren_helper) {
#if USE_RANGE_COLOR_START_DEFAULT
        draw_paren_highlight(app, buffer_id, text_layout_id, cursor_pos, colors_text_cycle.vals, colors_text_cycle.count);
#else
        tebtro_draw_paren_highlight(app, buffer_id, text_layout_id, cursor_pos, colors_text_cycle.vals, colors_text_cycle.count);
#endif
    }
    // @note: Color braces
    {
        tebtro_draw_brace_highlight(app, buffer_id, text_layout_id, cursor_pos, colors_back_cycle_brighter, ArrayCount(colors_back_cycle_brighter));
    }
    
    // @note Vim visual highlight
    vim_draw_highlight(app, view_id, is_active_view, buffer_id, text_layout_id, cursor_roundness, mark_thickness);
    
    // @note: Search highlight
    {
#if 0
        ARGB_Color argb_highlight = fcolor_resolve(fcolor_id(defcolor_highlight));
        ARGB_Color argb_at_highlight = fcolor_resolve(fcolor_id(defcolor_at_highlight));
#else
        ARGB_Color argb_highlight = 0xFFDDEE00;
        ARGB_Color argb_at_highlight = 0xFFFF44DD;
#endif
        b32 has_highlight_range = tebtro_draw_search_highlight(app, view_id, buffer_id, text_layout_id, cursor_roundness,
                                                               argb_highlight, argb_at_highlight);
    }
    
    // @note: Scope vertical line highlight
    {
        tebtro_draw_vertical_lines_scope_highlight(app, buffer_id, view_id, text_layout_id, view_region, cursor_pos, (ARGB_Color *)&colors_back_cycle_adjusted, ArrayCount(colors_back_cycle_adjusted));
    }
    
    // @note: Hex color preview
    tebtro_draw_hex_color_preview(app, buffer_id, text_layout_id, cursor_pos);
    
    // @note Vertical line highlight range
    if (is_active_view) {
        ARGB_Color argb_cursor_mark_range = 0x77CCCCCC; // 0xFF010808
        vim_draw_vertical_line_highlight_range(app, view_id, text_layout_id, Ii64(cursor_pos, mark_pos), argb_cursor_mark_range);
    }
    // @note Vim cursor and mark
    vim_draw_cursor_mark(app, view_id, is_active_view, buffer_id, text_layout_id, cursor_roundness, mark_thickness);
    // @note Vim visual mode draw selection whitespaces
    {
        ARGB_Color argb = 0xAF004FCF;
        vim_draw_visual_mode_whitespaces(app, view_id, face_id, buffer_id, text_layout_id, &token_array, argb);
    }
    
    // @note: Divider comments
    {
        tebtro_draw_main_section_divider_comments(app, view_id, face_id, buffer_id, view_region, text_layout_id);
        tebtro_draw_divider_comments(app, view_id, face_id, buffer_id, view_region, text_layout_id);
    }
    
    // NOTE(allen): Fade ranges
    paint_fade_ranges(app, text_layout_id, buffer_id, view_id);
    
    // @note: Put the actual text on the actual screen
    draw_text_layout_default(app, text_layout_id);
    
    // @note: Render comment font styles
    {
        Face_ID underlined_face_id = 0;
        Face_ID strikethrough_face_id = 0;
        Face_ID bold_face_id = global_bold_face_id;
        Face_ID italic_face_id = global_italic_face_id;
        tebtro_draw_comment_font_styles(app, text_layout_id, buffer_id, &token_array, underlined_face_id, strikethrough_face_id, bold_face_id, italic_face_id);
    }
    
    // @note: Scope brace annotations
    {
        // (ARGB_Color *)&colors_back_cycle_adjusted, ArrayCount(colors_back_cycle_adjusted)
        u32 color = 0xFF808080;
        int color_count = 1;
        tebtro_draw_scope_close_brace_annotations(app, view_id, view_region, buffer_id, text_layout_id, face_id, cursor_pos, (ARGB_Color *)&color, color_count);
    }
    
    // @note :avy_search
    {
        ARGB_Color argb_background = 0xFFFFFF00;
        ARGB_Color argb_foreground = 0xFF000000;
        avy_render(app, view_id, buffer_id, text_layout_id, face_id, view_region, cursor_roundness, argb_background, argb_foreground);
    }
    
#if CALC_PLOT
    // @note: Interpret buffer as calc code, if it's the calc buffer.
    {
        Buffer_ID calc_buffer_id = get_buffer_by_name(app, string_u8_litexpr("*calc*"), AccessFlag_Read);
        if(calc_buffer_id == buffer_id) {
            Fleury4RenderCalcBuffer(app, buffer_id, view_id, text_layout_id, frame_info);
        }
    }
    // @note: Render calc comments
    {
        Fleury4RenderCalcComments(app, buffer_id, view_id, text_layout_id, frame_info);
    }
#endif
    
    // @note: Function parameter helper
    if (vim_state->mode == vim_mode_insert && global_show_function_helper) {
        vim_render_function_helper(app, view_id, buffer_id, text_layout_id, cursor_pos);
    }
    
#if CODE_PEEK
    // @note: Code peek
    if (is_active_view) {
        global_last_cursor_rect = text_layout_character_on_screen(app, text_layout_id, cursor_pos);
        Fleury4RenderCodePeek(app, view_id, face_id, text_layout_id, buffer_id, frame_info);
    }
#endif
    
    draw_set_clip(app, prev_clip);
}

function void
tebtro_render_caller(Application_Links *app, Frame_Info frame_info, View_ID view_id){
    ProfileScope(app, "render caller");
    View_ID active_view_id = get_active_view(app, Access_Always);
    b32 is_active_view = (view_id == active_view_id);
#if 0
    Color_Table temp_color_table;
    if (is_active_view) {
        Managed_Scope view_scope = view_get_managed_scope(app, view_id);
        Vim_View_State *vim_state_ptr = scope_attachment(app, view_scope, view_vim_state_id, Vim_View_State);
        temp_color_table = active_color_table;
        if (vim_state_ptr) {
            active_color_table = default_color_table; // @todo vim mode specific color_table
        }
    }
    defer {
        if (is_active_view)  active_color_table = temp_color_table;
    };
#endif
#if 1
    Color_Table temp_color_table = active_color_table;
    Managed_Scope view_scope = view_get_managed_scope(app, view_id);
    Vim_View_State *vim_state_ptr = scope_attachment(app, view_scope, view_vim_state_id, Vim_View_State);
    if (vim_state_ptr != 0 && vim_state_ptr->color_table_ptr != 0) {
        active_color_table = *vim_state_ptr->color_table_ptr;
    }
    else {
        active_color_table = vim_global_state.color_tables.default;
    }
    
#if 0
    defer {
        active_color_table = temp_color_table;
    };
#endif
#endif
    
    Buffer_ID buffer_id = view_get_buffer(app, view_id, Access_Always);
    
    Rect_f32 region = tebtro_draw_background_and_margin(app, view_id, buffer_id, is_active_view, global_keyboard_macro_is_recording);
    Rect_f32 prev_clip = draw_set_clip(app, region);
    
    Face_ID face_id = get_face_id(app, buffer_id);
    Face_Metrics face_metrics = get_face_metrics(app, face_id);
    f32 line_height = face_metrics.line_height;
    f32 digit_advance = face_metrics.decimal_digit_advance;
    
#define BARS_ON_TOP false
    
    // NOTE(allen): query bars
    {
        Query_Bar *space[32];
        Query_Bar_Ptr_Array query_bars = {};
        query_bars.ptrs = space;
        if (get_active_query_bars(app, view_id, ArrayCount(space), &query_bars)) {
            for (i32 i = 0; i < query_bars.count; i += 1){
#if BARS_ON_TOP
                Rect_f32_Pair pair = layout_query_bar_on_top(region, line_height, 1);
                draw_query_bar(app, query_bars.ptrs[i], face_id, pair.min);
                region = pair.max;
#else
                Rect_f32_Pair pair = layout_query_bar_on_bot(region, line_height, 1);
                draw_query_bar(app, query_bars.ptrs[i], face_id, pair.max);
                region = pair.min;
#endif
            }
        }
    }
    
    // NOTE(allen): file bar
    b64 showing_file_bar = false;
    if (view_get_setting(app, view_id, ViewSetting_ShowFileBar, &showing_file_bar) && showing_file_bar) {
#if BARS_ON_TOP
        Rect_f32_Pair pair = layout_file_bar_on_top(region, line_height);
        vim_draw_file_bar(app, view_id, is_active_view, buffer_id, face_id, pair.min);
        region = pair.max;
#else
        Rect_f32_Pair pair = layout_file_bar_on_bot(region, line_height);
        vim_draw_file_bar(app, view_id, is_active_view, buffer_id, face_id, pair.max);
        region = pair.min;
#endif
    }
    
    Buffer_Scroll scroll = view_get_buffer_scroll(app, view_id);
    
    Buffer_Point_Delta_Result delta = delta_apply(app, view_id,
                                                  frame_info.animation_dt, scroll);
    if (!block_match_struct(&scroll.position, &delta.point)) {
        block_copy_struct(&scroll.position, &delta.point);
        view_set_buffer_scroll(app, view_id, scroll, SetBufferScroll_NoCursorChange);
    }
    if (delta.still_animating) {
        animate_in_n_milliseconds(app, 0);
    }
    
    // NOTE(allen): FPS hud
    if (show_fps_hud) {
        Rect_f32_Pair pair = layout_fps_hud_on_bottom(region, line_height);
        draw_fps_hud(app, frame_info, face_id, pair.max);
        region = pair.min;
        animate_in_n_milliseconds(app, 1000);
    }
    
    // NOTE(allen): layout line numbers
    Rect_f32 line_number_rect = {};
    if (global_config.show_line_number_margins) {
        Rect_f32_Pair pair = layout_line_number_margin(app, buffer_id, region, digit_advance);
        line_number_rect = pair.min;
        region = pair.max;
    }
    
    // @note Vim :view_changed_flash_line
    if (is_active_view && vim_global_view_changed_time > 0.0f) {
        vim_global_view_changed_time -= frame_info.literal_dt;
        animate_in_n_milliseconds(app, (u32)(vim_global_view_changed_time * 1000.0f));
    }
    
    // NOTE(allen): begin buffer render
    Buffer_Point buffer_point = scroll.position;
    Text_Layout_ID text_layout_id = text_layout_create(app, buffer_id, region, buffer_point);
    
    // NOTE(allen): draw line numbers
    if (global_config.show_line_number_margins) {
#if 0
        draw_line_number_margin(app, view_id, buffer_id, face_id, text_layout_id, line_number_rect);
#else
        // @note relative
        draw_line_number_relative_margin(app, view_id, buffer_id, face_id, text_layout_id, line_number_rect);
#endif
    }
    
    // NOTE(allen): draw the buffer
    tebtro_render_buffer(app, view_id, face_id, buffer_id, text_layout_id, region, frame_info, vim_state_ptr);
    
    text_layout_free(app, text_layout_id);
    draw_set_clip(app, prev_clip);
}


function void
tebtro_tick(Application_Links *app, Frame_Info frame_info){
    // :suppress_mouse
    // @note Vim suppress mouse if not moving
    if (vim_global_enable_mouse_suppression) {
        if (vim_global_mouse_last_event_time > 0.0f) {
            vim_global_mouse_last_event_time -= frame_info.literal_dt;
            animate_in_n_milliseconds(app, (u32)(vim_global_mouse_last_event_time * 1000.0f));
        }
        if (vim_global_mouse_last_event_time <= 0.0f) {
            set_mouse_suppression(true);
        }
    }
    
#if CALC_PLOT
    // @note: Update calc (once per frame).
    {
        static i32 last_frame_index = -1;
        if(last_frame_index != frame_info.index)
        {
            CalcUpdateOncePerFrame(frame_info);
        }
        last_frame_index = frame_info.index;
    }
#endif
    
    
    Scratch_Block scratch(app);
    
    for (Buffer_Modified_Node *node = global_buffer_modified_set.first;
         node != 0;
         node = node->next){
        Temp_Memory_Block temp(scratch);
        Buffer_ID buffer_id = node->buffer;
        
        Managed_Scope scope = buffer_get_managed_scope(app, buffer_id);
        
        String_Const_u8 contents = push_whole_buffer(app, scratch, buffer_id);
        Token_Array *tokens_ptr = scope_attachment(app, scope, attachment_tokens, Token_Array);
        if (tokens_ptr == 0){
            continue;
        }
        if (tokens_ptr->count == 0){
            continue;
        }
        Token_Array tokens = *tokens_ptr;
        
        Arena arena = make_arena_system(KB(16));
        Code_Index_File *index = push_array_zero(&arena, Code_Index_File, 1);
        index->buffer = buffer_id;
        
        Generic_Parse_State state = {};
        generic_parse_init(app, &arena, contents, &tokens, &state);
        // TODO(allen): Actually determine this in a fair way.
        // Maybe switch to an enum.
        state.do_cpp_parse = true;
        generic_parse_full_input_breaks(index, &state, max_i32);
        Code_Index_Identifier_Hash_Table *identifier_table = scope_attachment(app, scope, attachment_code_index_identifier_table, Code_Index_Identifier_Hash_Table);
        *identifier_table = code_index_identifier_table_from_array(app, buffer_id, state.arena, index->note_array);
        
        code_index_lock();
        code_index_set_file(buffer_id, arena, index);
        code_index_unlock();
        buffer_clear_layout_cache(app, buffer_id);
    }
    
    buffer_modified_set_clear();
    
    if (tick_all_fade_ranges(frame_info.animation_dt)) {
        animate_in_n_milliseconds(app, 0);
    }
}

BUFFER_HOOK_SIG(tebtro_file_save) {
    // buffer_id
    ProfileScope(app, "default file save");
    
    // @note Clean all lines
    tebtro_clean_all_lines(app, buffer_id);
    
    // @note Auto indent
    b32 is_virtual = global_config.enable_virtual_whitespace;
    if (global_config.automatically_indent_text_on_save && is_virtual) {
        auto_indent_buffer(app, buffer_id, buffer_range(app, buffer_id));
    }
    
    // @note eol nixify
    Managed_Scope scope = buffer_get_managed_scope(app, buffer_id);
    Line_Ending_Kind *eol_setting = scope_attachment(app, scope, buffer_eol_setting, Line_Ending_Kind);
    *eol_setting = LineEndingKind_LF;
    rewrite_lines_to_lf(app, buffer_id);
    
    // no meaning for return
    return(0);
}



internal void
tebtro_set_all_hooks(Application_Links *app){
    set_custom_hook(app, HookID_BufferViewerUpdate, default_view_adjust);
    
    set_custom_hook(app, HookID_ViewEventHandler, tebtro_view_input_handler);
    set_custom_hook(app, HookID_Tick, tebtro_tick);
    set_custom_hook(app, HookID_RenderCaller, tebtro_render_caller);
#if 0
    set_custom_hook(app, HookID_DeltaRule, original_delta);
    set_custom_hook_memory_size(app, HookID_DeltaRule,
                                delta_ctx_size(original_delta_memory_size));
#else
    set_custom_hook(app, HookID_DeltaRule, fixed_time_cubic_delta);
    set_custom_hook_memory_size(app, HookID_DeltaRule,
                                delta_ctx_size(fixed_time_cubic_delta_memory_size));
#endif
    set_custom_hook(app, HookID_BufferNameResolver, default_buffer_name_resolution);
    
    set_custom_hook(app, HookID_BeginBuffer, tebtro_begin_buffer);
    set_custom_hook(app, HookID_EndBuffer, end_buffer_close_jump_list);
    set_custom_hook(app, HookID_NewFile, vim_new_file);
    set_custom_hook(app, HookID_SaveFile, tebtro_file_save);
    set_custom_hook(app, HookID_BufferEditRange, default_buffer_edit_range);
    set_custom_hook(app, HookID_BufferRegion, default_buffer_region);
    
    set_custom_hook(app, HookID_Layout, layout_unwrapped);
    //set_custom_hook(app, HookID_Layout, layout_wrap_anywhere);
    //set_custom_hook(app, HookID_Layout, layout_wrap_whitespace);
    //set_custom_hook(app, HookID_Layout, layout_virt_indent_unwrapped);
    //set_custom_hook(app, HookID_Layout, layout_unwrapped_small_blank_lines);
}

