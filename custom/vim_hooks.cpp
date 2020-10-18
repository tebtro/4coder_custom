CUSTOM_COMMAND_SIG(vim_startup)
CUSTOM_DOC("Vim custom command for responding to a startup event")
{
    ProfileScope(app, "vim_startup");
    User_Input input = get_current_input(app);
    if (!match_core_code(&input, CoreCode_Startup))  return;
    
    String_Const_u8_Array file_names = input.event.core.file_names;
    load_themes_default_folder(app);
    default_4coder_initialize(app, file_names);
    default_4coder_side_by_side_panels(app, file_names);
    if (global_config.automatically_load_project){
        load_project(app);
    }
    
    vim_setup_mode_and_chord_color_tables(app);
}

CUSTOM_COMMAND_SIG(vim_view_input_handler)
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
        
        if (suppressing_mouse && (event_properties & EventPropertyGroup_AnyMouseEvent) != 0) {
            continue;
        }
        
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
            
            // NOTE(allen): call the command
            binding.custom(app);
            
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

function void
vim_render_buffer(Application_Links *app, View_ID view_id, Face_ID face_id,
                  Buffer_ID buffer_id, Text_Layout_ID text_layout_id,
                  Rect_f32 rect) {
    ProfileScope(app, "render buffer");
    
    View_ID active_view = get_active_view(app, Access_Always);
    b32 is_active_view = (active_view == view_id);
    Rect_f32 prev_clip = draw_set_clip(app, rect);
    
    // NOTE(allen): Token colorizing
    Token_Array token_array = get_token_array_from_buffer(app, buffer_id);
    if (token_array.tokens != 0) {
        draw_cpp_token_colors(app, text_layout_id, &token_array);
        
        // NOTE(allen): Scan for TODOs and NOTEs
        if (global_config.use_comment_keyword) {
            Comment_Highlight_Pair pairs[] = {
                {string_u8_litexpr("NOTE"), finalize_color(defcolor_comment_pop, 0)},
                {string_u8_litexpr("TODO"), finalize_color(defcolor_comment_pop, 1)},
            };
            draw_comment_highlights(app, buffer_id, text_layout_id,
                                    &token_array, pairs, ArrayCount(pairs));
        }
    }
    else {
        Range_i64 visible_range = text_layout_get_visible_range(app, text_layout_id);
        paint_text_color_fcolor(app, text_layout_id, visible_range, fcolor_id(defcolor_text_default));
    }
    
    i64 cursor_pos = view_correct_cursor(app, view_id);
    view_correct_mark(app, view_id);
    
    // @note: Token under cursor highlight
    // @todo
    
    // NOTE(allen): Scope highlight
    if (global_config.use_scope_highlight) {
        Color_Array colors = finalize_color_array(defcolor_back_cycle);
        draw_scope_highlight(app, buffer_id, text_layout_id, cursor_pos, colors.vals, colors.count);
    }
    
    // NOTE(allen): Line highlight
    if (global_config.highlight_line_at_cursor && is_active_view) {
        i64 line_number = get_line_number_from_pos(app, buffer_id, cursor_pos);
        draw_line_highlight(app, text_layout_id, line_number,
                            fcolor_id(defcolor_highlight_cursor_line));
    }
    
    if (global_config.use_error_highlight || global_config.use_jump_highlight) {
        // NOTE(allen): Error highlight
        String_Const_u8 name = string_u8_litexpr("*compilation*");
        Buffer_ID compilation_buffer = get_buffer_by_name(app, name, Access_Always);
        if (global_config.use_error_highlight) {
            draw_jump_highlights(app, buffer_id, text_layout_id, compilation_buffer,
                                 fcolor_id(defcolor_highlight_junk));
        }
        
        // NOTE(allen): Search highlight
        if (global_config.use_jump_highlight) {
            Buffer_ID jump_buffer = get_locked_jump_buffer(app);
            if (jump_buffer != compilation_buffer) {
                draw_jump_highlights(app, buffer_id, text_layout_id, jump_buffer,
                                     fcolor_id(defcolor_highlight_white));
            }
        }
    }
    
    // NOTE(allen): Color parens
    if (global_config.use_paren_helper) {
        Color_Array colors = finalize_color_array(defcolor_text_cycle);
        draw_paren_highlight(app, buffer_id, text_layout_id, cursor_pos, colors.vals, colors.count);
    }
    
    // NOTE(allen): Cursor shape
    Face_Metrics metrics = get_face_metrics(app, face_id);
    f32 cursor_roundness = (metrics.normal_advance*0.5f)*0.9f;
    f32 mark_thickness = 2.f;
    
    // @note Vim cursor, mark and highlight
    vim_draw_highlight(app, view_id, is_active_view, buffer_id, text_layout_id, cursor_roundness, mark_thickness);
    vim_draw_cursor_mark(app, view_id, is_active_view, buffer_id, text_layout_id, cursor_roundness, mark_thickness);
    
    // NOTE(allen): put the actual text on the actual screen
    draw_text_layout_default(app, text_layout_id);
    
    draw_set_clip(app, prev_clip);
}

function void
vim_render_caller(Application_Links *app, Frame_Info frame_info, View_ID view_id){
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
    
    // @todo hover
    // @copynpaste @note
    // custom/4coder_draw.cpp get_margin_color return lister_item color.
    // But it is used for background margin color and other stuff, which is wrong.
    // Because we have a defcolor_margin.
    ARGB_Color margin_argb = 0;
    if (is_active_view) {
        if (global_keyboard_macro_is_recording) {
            margin_argb = 0xFFDC143C; // crimson red: 0xFFDC143C
        }
        else {
            margin_argb = fcolor_resolve(fcolor_id(defcolor_margin_active));
        }
    }
    else {
        margin_argb = fcolor_resolve(fcolor_id(defcolor_margin));
    }
    Rect_f32 region = draw_background_and_margin(app, view_id, margin_argb, fcolor_resolve(fcolor_id(defcolor_back)));
    Rect_f32 prev_clip = draw_set_clip(app, region);
    
    Buffer_ID buffer = view_get_buffer(app, view_id, Access_Always);
    Face_ID face_id = get_face_id(app, buffer);
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
        vim_draw_file_bar(app, view_id, is_active_view, buffer, face_id, pair.min);
        region = pair.max;
#else
        Rect_f32_Pair pair = layout_file_bar_on_bot(region, line_height);
        vim_draw_file_bar(app, view_id, is_active_view, buffer, face_id, pair.max);
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
        Rect_f32_Pair pair = layout_line_number_margin(app, buffer, region, digit_advance);
        line_number_rect = pair.min;
        region = pair.max;
    }
    
    // NOTE(allen): begin buffer render
    Buffer_Point buffer_point = scroll.position;
    Text_Layout_ID text_layout_id = text_layout_create(app, buffer, region, buffer_point);
    
    // NOTE(allen): draw line numbers
    if (global_config.show_line_number_margins) {
#if 0
        draw_line_number_margin(app, view_id, buffer, face_id, text_layout_id, line_number_rect);
#else
        // @note relative
        draw_relative_line_number_margin(app, view_id, buffer, face_id, text_layout_id, line_number_rect);
#endif
    }
    
    // NOTE(allen): draw the buffer
    vim_render_buffer(app, view_id, face_id, buffer, text_layout_id, region);
    
    text_layout_free(app, text_layout_id);
    draw_set_clip(app, prev_clip);
}

BUFFER_HOOK_SIG(vim_new_file){
    Scratch_Block scratch(app);
    String_Const_u8 file_name = push_buffer_base_name(app, scratch, buffer_id);
    if (!string_match(string_postfix(file_name, 2), string_u8_litexpr(".h"))) {
        return(0);
    }
    
    List_String_Const_u8 guard_list = {};
    for (u64 i = 0; i < file_name.size; ++i){
        u8 c[2] = {};
        u64 c_size = 1;
        u8 ch = file_name.str[i];
        if ('A' <= ch && ch <= 'Z'){
            c_size = 2;
            c[0] = '_';
            c[1] = ch;
        }
        else if ('0' <= ch && ch <= '9'){
            c[0] = ch;
        }
        else if ('a' <= ch && ch <= 'z'){
            c[0] = ch - ('a' - 'A');
        }
        else{
            c[0] = '_';
        }
        String_Const_u8 part = push_string_copy(scratch, SCu8(c, c_size));
        string_list_push(scratch, &guard_list, part);
    }
    String_Const_u8 guard = string_list_flatten(scratch, guard_list);
    
    Buffer_Insertion insert = begin_buffer_insertion_at_buffered(app, buffer_id, 0, scratch, KB(16));
    insertf(&insert,
            "#if !defined(%.*s)\n"
            "#define %.*s\n"
            "\n\n\n\n\n"
            "#endif // %.*s\n",
            string_expand(guard),
            string_expand(guard),
            string_expand(guard));
    end_buffer_insertion(&insert);
    
    return(0);
}


internal void
vim_set_all_hooks(Application_Links *app){
    set_custom_hook(app, HookID_BufferViewerUpdate, default_view_adjust);
    
    set_custom_hook(app, HookID_ViewEventHandler, vim_view_input_handler);
    set_custom_hook(app, HookID_Tick, default_tick);
    set_custom_hook(app, HookID_RenderCaller, vim_render_caller);
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
    
    set_custom_hook(app, HookID_BeginBuffer, default_begin_buffer);
    set_custom_hook(app, HookID_EndBuffer, end_buffer_close_jump_list);
    set_custom_hook(app, HookID_NewFile, vim_new_file);
    set_custom_hook(app, HookID_SaveFile, default_file_save);
    set_custom_hook(app, HookID_BufferEditRange, default_buffer_edit_range);
    set_custom_hook(app, HookID_BufferRegion, default_buffer_region);
    
    set_custom_hook(app, HookID_Layout, layout_unwrapped);
    //set_custom_hook(app, HookID_Layout, layout_wrap_anywhere);
    //set_custom_hook(app, HookID_Layout, layout_wrap_whitespace);
    //set_custom_hook(app, HookID_Layout, layout_virt_indent_unwrapped);
    //set_custom_hook(app, HookID_Layout, layout_unwrapped_small_blank_lines);
}
