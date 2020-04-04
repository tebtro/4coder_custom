function void
tebtro_setup_mapping(Mapping *mapping) {
    MappingScope();
    SelectMapping(mapping);
    
    SelectMap(mapid_vim_unbound); {
        BindCore(tebtro_startup, CoreCode_Startup);
        BindCore(default_try_exit, CoreCode_TryExit);
    }
    SelectMap(mapid_vim_escape_to_normal_mode); {
        ParentMap(mapid_vim_unbound);
    }
    
    SelectMap(mapid_vim_movements); {
        Bind(nop, KeyCode_4, KeyCode_Shift);
        Bind(vim_execute_command_count_add_predecimal_0, KeyCode_0);
        Bind(vim_move_to_line_start, KeyCode_H, KeyCode_Alt);
        Bind(vim_move_to_line_end,   KeyCode_L, KeyCode_Alt);
        
        // @note scope selection
        Bind(vim_select_next_scope_absolute,       KeyCode_N);
        Bind(vim_select_prev_scope_absolute,       KeyCode_Minus);
        Bind(vim_select_next_scope_after_current,  KeyCode_N, KeyCode_Shift);
        Bind(vim_select_prev_top_most_scope,       KeyCode_Minus, KeyCode_Shift);
        // Bind(vim_select_surrounding_scope_maximal, KeyCode_N, KeyCode_Control);
        // Bind(vim_select_surrounding_scope,         KeyCode_M, KeyCode_Control);
        
        Bind(nop, KeyCode_Period);
        Bind(nop, KeyCode_Period, KeyCode_Shift);
        
        Bind(vim_toggle_build_panel_height, KeyCode_Period);
    }
    
    SelectMap(mapid_vim_mode_normal); {
        Bind(copy,         KeyCode_Y, KeyCode_Shift); // KeyCode_Control);
        Bind(delete_range, KeyCode_D, KeyCode_Shift); // KeyCode_Control);
        Bind(cut,          KeyCode_X, KeyCode_Shift); // KeyCode_Control);
        
        Bind(vim_toggle_mouse_suppression, KeyCode_Space, KeyCode_Alt);
    }
    
    SelectMap(mapid_vim_mode_insert); {
        // :autocomplete
        Bind(word_complete,               KeyCode_Tab, KeyCode_Shift);
        Bind(word_complete_drop_down,     KeyCode_Tab, KeyCode_Control);
        
        Bind(toggle_show_function_helper, KeyCode_Space, KeyCode_Control);
        Bind(toggle_show_function_helper, KeyCode_Space, KeyCode_Alt);
    }
}


function void
tebtro_setup_mapping_dvorak_programmer(Mapping *mapping) {
    MappingScope();
    SelectMapping(mapping);
    
    
    //
    // @note: Vim keybindings
    //
    SelectMap(mapid_vim_unbound);
    {
        BindCore(tebtro_startup, CoreCode_Startup);
        BindCore(default_try_exit, CoreCode_TryExit);
        
        BindMouseWheel(mouse_wheel_scroll);
        BindMouseWheel(mouse_wheel_change_face_size, KeyCode_Control);
        // @todo not working BindMouse(vim_reset_face_size, MouseCode_Middle);
        Bind(vim_reset_face_size, KeyCode_Home);
        Bind(vim_set_small_face_size, KeyCode_PageDown);
        Bind(vim_set_big_face_size, KeyCode_PageUp);
        
        // @todo Command to reset face_size to the one specified in the theme file
        
        // @todo cursor click, maybe we don't want that, maybe just to change view
        //
        // @todo
        // This commands are for updating the selection range.
        // @note But I think we don't want them in every mode. Just in visual mode,
        //       so that you don't accidentaly change the cursor pos while typing in insert mode.
        // But maybe we want it in normal mode, so we should probably just unbind it in insert mode.
        //
        // @todo Or we could set suppressing_mouse to true when entering insert mode!
        // Use set_mouse_suppression(b32 suppress);
        //
        // bind(context, key_mouse_left, MDFR_NONE, vim_move_click);
        // bind(context, key_mouse_wheel, MDFR_NONE, vim_move_scroll);
    }
    
    SelectMap(mapid_vim_escape_to_normal_mode);
    {
        ParentMap(mapid_vim_unbound);
        
        Bind(vim_enter_mode_normal, KeyCode_Escape);
    }
    
    //
    // @note Mode maps
    //
    
    /*
            // @note Also has bindings like search, and status_command_input for example.
            // These are all bindings which should be awailable in all modes which include this command_map.
            // Because we probably want to use a search_command in visual mode for example.
    */
    SelectMap(mapid_vim_movements);
    {
        ParentMap(mapid_vim_escape_to_normal_mode);
        
        // @note execute command count
        Bind(vim_execute_command_count_add_predecimal_0, KeyCode_0);
        Bind(vim_execute_command_count_add_predecimal_1, KeyCode_1);
        Bind(vim_execute_command_count_add_predecimal_2, KeyCode_2);
        Bind(vim_execute_command_count_add_predecimal_3, KeyCode_3);
        Bind(vim_execute_command_count_add_predecimal_4, KeyCode_4);
        Bind(vim_execute_command_count_add_predecimal_5, KeyCode_5);
        Bind(vim_execute_command_count_add_predecimal_6, KeyCode_6);
        Bind(vim_execute_command_count_add_predecimal_7, KeyCode_7);
        Bind(vim_execute_command_count_add_predecimal_8, KeyCode_8);
        Bind(vim_execute_command_count_add_predecimal_9, KeyCode_9);
        
        // :move_commands
        
        // @note move hjkl
        Bind(vim_move_up,    KeyCode_K);
        Bind(vim_move_down,  KeyCode_J);
        Bind(vim_move_left,  KeyCode_H);
        Bind(vim_move_right, KeyCode_L);
        
        Bind(vim_move_to_file_start, KeyCode_K, KeyCode_Control);
        Bind(vim_move_to_file_end,   KeyCode_J, KeyCode_Control);
        
        Bind(vim_move_to_line_start, KeyCode_H, KeyCode_Alt);
        Bind(vim_move_to_line_end,   KeyCode_L, KeyCode_Alt);
        // @todo Should these be just in normal mode
        // @todo Remove whitespaces from the combined lines indentation
        // Bind(vim_combine_with_previous_line, KeyCode_H, KeyCode_Control);
        // Bind(vim_combine_with_next_line,     KeyCode_L, KeyCode_Control);
        
        Bind(vim_move_up_by_whitespace,   KeyCode_K, KeyCode_Alt);
        Bind(vim_move_down_by_whitespace, KeyCode_J, KeyCode_Alt);
        
        Bind(vim_move_up_by_page,   KeyCode_K, KeyCode_Shift);
        Bind(vim_move_down_by_page, KeyCode_J, KeyCode_Shift);
        
        Bind(vim_move_up_by_page_half,   KeyCode_K, KeyCode_Shift, KeyCode_Alt);
        Bind(vim_move_down_by_page_half, KeyCode_J, KeyCode_Shift, KeyCode_Alt);
        
        
        // @note Move webg
        Bind(vim_move_right_word_start, KeyCode_W);
        Bind(vim_move_right_word_end,   KeyCode_E);
        Bind(vim_move_left_word_start,  KeyCode_B);
        Bind(vim_move_left_word_end,    KeyCode_G);
        
        Bind(vim_move_right_token_start, KeyCode_W, KeyCode_Shift);
        Bind(vim_move_right_token_end,   KeyCode_E, KeyCode_Shift);
        Bind(vim_move_left_token_start,  KeyCode_B, KeyCode_Shift);
        Bind(vim_move_left_token_end,    KeyCode_G, KeyCode_Shift);
        
        Bind(vim_move_right_one_after_whitespace,  KeyCode_W, KeyCode_Alt);
        Bind(vim_move_right_one_before_whitespace, KeyCode_E, KeyCode_Alt);
        Bind(vim_move_left_one_before_whitespace,  KeyCode_B, KeyCode_Alt);
        Bind(vim_move_left_one_after_whitespace,   KeyCode_G, KeyCode_Alt);
        
        // @note Find/Till character search commands
        Bind(vim_enter_chord_move_right_to_found,     KeyCode_F);
        Bind(vim_enter_chord_move_right_before_found, KeyCode_T);
        Bind(vim_enter_chord_move_left_to_found,      KeyCode_F, KeyCode_Shift);
        Bind(vim_enter_chord_move_left_before_found,  KeyCode_T, KeyCode_Shift);
        
        Bind(vim_enter_chord_z, KeyCode_Z);
        Bind(vim_enter_chord_z, KeyCode_Z, KeyCode_Control);
        
        Bind(vim_goto_line, KeyCode_G, KeyCode_Control);
        // :avy
        Bind(vim_avy_goto_line, KeyCode_G, KeyCode_Alt);
        // Bind(vim_enter_chord_g, KeyCode_G, KeyCode_Control);
        
        // @note :cursor_mark
        // @todo Proper vim marks
        Bind(vim_set_mark,         KeyCode_M); // Should we be able to set mark in visual mode
        Bind(vim_cursor_mark_swap, KeyCode_S);
        Bind(vim_cursor_mark_swap_scope_range, KeyCode_S, KeyCode_Shift);
        // @todo
        // Bind(vim_save_cursor_mark_pos,    KeyCode_S, KeyCode_Alt);
        // Bind(vim_restore_cursor_mark_pos, KeyCode_S, KeyCode_Shift);
        
        // :selection
        Bind(vim_select_token_or_word_under_cursor, KeyCode_T, KeyCode_Alt);
        Bind(vim_visual_select_token_or_word_under_cursor, KeyCode_T, KeyCode_Alt, KeyCode_Shift);
        
        // @note scope selection
        Bind(vim_select_next_scope_absolute,       KeyCode_N);
        Bind(vim_select_prev_scope_absolute,       KeyCode_Minus);
        Bind(vim_select_next_scope_after_current,  KeyCode_N, KeyCode_Shift);
        Bind(vim_select_prev_top_most_scope,       KeyCode_Minus, KeyCode_Shift);
        // Bind(vim_select_surrounding_scope_maximal, KeyCode_N, KeyCode_Control);
        // Bind(vim_select_surrounding_scope,         KeyCode_M, KeyCode_Control);
        
        // @note :search
        // @todo
        // Bind(vim_search, KeyCode_ForwardSlash);
        // Bind(vim_search_reverse, KeyCode_?); @keycode_missing
        // Bind(vim_search_next,     KeyCode_N);
        // Bind(vim_search_previous, KeyCode_N, KeyCode_Shift);
        // Bind(vim_search_identifier_under_cursor, KeyCode_*); @keycode_missing
        Bind(search,         KeyCode_S, KeyCode_Control);
        Bind(reverse_search, KeyCode_R, KeyCode_Control);
        // Bind(search_identifier,         KeyCode_Equal, KeyCode_Shift);
        // Bind(reverse_search_identifier, KeyCode_Equal, KeyCode_Shift, KeyCode_Alt);
        Bind(vim_search_token_or_word,         KeyCode_4); // KeyCode_Plus
        Bind(vim_reverse_search_token_or_word, KeyCode_4, KeyCode_Alt); // KeyCode_Plus
        // Bind(nop, KeyCode_Equal, KeyCode_Shift);
        
        // @note :avy
        // Bind(avy_search, KeyCode_A, KeyCode_Alt);
        Bind(vim_avy_goto_string, KeyCode_S, KeyCode_Alt);
        Bind(vim_avy_goto_char, KeyCode_C, KeyCode_Alt);
        
        // @note query_replace
        Bind(query_replace,     KeyCode_Q);
        Bind(replace_in_range,  KeyCode_Q, KeyCode_Shift);
        Bind(replace_in_buffer, KeyCode_Q, KeyCode_Shift, KeyCode_Control);
        Bind(query_replace_identifier,     KeyCode_Q, KeyCode_Control);
        
        
#if USE_MULTIPLE_CURSORS
        // :multiple_cursors
        Bind(vim_add_multiple_cursor_down,  KeyCode_J, KeyCode_Control, KeyCode_Alt, KeyCode_Shift);
        Bind(vim_remove_multiple_cursor_up, KeyCode_K, KeyCode_Control, KeyCode_Alt, KeyCode_Shift);
        Bind(vim_multiple_cursor_search, KeyCode_S, KeyCode_Control, KeyCode_Alt);
#endif
        
        
        Bind(vim_toggle_mouse_suppression, KeyCode_Space, KeyCode_Alt);
        Bind(vim_toggle_build_panel_height, KeyCode_Period);
    }
    
    SelectMap(mapid_vim_mode_normal);
    {
        ParentMap(mapid_vim_movements);
        
        Bind(exit_4coder, KeyCode_F4, KeyCode_Alt);
        
        // @note: Project commands
        Bind(project_fkey_command, KeyCode_F1);
        Bind(project_fkey_command, KeyCode_F2);
        Bind(project_fkey_command, KeyCode_F3);
        Bind(project_fkey_command, KeyCode_F4);
        Bind(project_fkey_command, KeyCode_F5);
        Bind(project_fkey_command, KeyCode_F6);
        Bind(project_fkey_command, KeyCode_F7);
        Bind(project_fkey_command, KeyCode_F8);
        Bind(project_fkey_command, KeyCode_F9);
        Bind(project_fkey_command, KeyCode_F10);
        Bind(project_fkey_command, KeyCode_F11);
        Bind(project_fkey_command, KeyCode_F12);
        Bind(project_fkey_command, KeyCode_F13);
        Bind(project_fkey_command, KeyCode_F14);
        Bind(project_fkey_command, KeyCode_F15);
        Bind(project_fkey_command, KeyCode_F16);
        
        // @note Lister commands
        Bind(if_read_only_goto_position_same_panel, KeyCode_Return);
        Bind(if_read_only_goto_position,            KeyCode_Return, KeyCode_Alt);
        Bind(view_jump_list_with_lister,            KeyCode_Return, KeyCode_Control);
        
#if CALC_PLOT
        // @note Calc commands
        Bind(vim_calc_write_result, KeyCode_Return, KeyCode_Control, KeyCode_Alt);
#endif
        
        // @note :paste
        Bind(vim_paste_after_and_indent, KeyCode_P);
        Bind(vim_paste_before_and_indent, KeyCode_P, KeyCode_Shift);
        Bind(vim_paste_next_and_indent, KeyCode_P, KeyCode_Alt);
        
        // @note delete
        Bind(vim_enter_chord_delete, KeyCode_D);
        //Bind(vim_delete_line, KeyCode_D, KeyCode_Shift);
        
        // @note Jumps
        Bind(vim_tebtro_jump_to_definition, KeyCode_D, KeyCode_Control);
        Bind(vim_tebtro_jump_to_definition, KeyCode_D, KeyCode_Alt);
        Bind(vim_tebtro_jump_to_definition_under_cursor, KeyCode_D, KeyCode_Control, KeyCode_Shift);
        Bind(vim_tebtro_jump_to_definition_under_cursor, KeyCode_D, KeyCode_Alt, KeyCode_Shift);
        
        // @note copy/yank
        Bind(vim_enter_chord_yank, KeyCode_Y);
        Bind(vim_yank_line, KeyCode_Y, KeyCode_Shift);
        
        // @note cut
        Bind(vim_delete_char, KeyCode_X);
        
        Bind(copy,         KeyCode_Y, KeyCode_Shift); // KeyCode_Control);
        Bind(delete_range, KeyCode_D, KeyCode_Shift); // KeyCode_Control);
        Bind(cut,          KeyCode_X, KeyCode_Shift); // KeyCode_Control);
        
        // @note Modes and chords
        Bind(vim_enter_mode_insert,            KeyCode_I);
        Bind(vim_enter_mode_insert_after,      KeyCode_A);
        Bind(vim_enter_mode_insert_line_start, KeyCode_I, KeyCode_Shift);
        Bind(vim_enter_mode_insert_line_end,   KeyCode_A, KeyCode_Shift);
        Bind(vim_newline_and_enter_mode_insert_after,  KeyCode_O);
        Bind(vim_newline_and_enter_mode_insert_before, KeyCode_O, KeyCode_Shift);
        
        Bind(vim_enter_chord_change, KeyCode_C);
        Bind(vim_enter_chord_replace_single, KeyCode_R);
        Bind(vim_enter_mode_replace,         KeyCode_R, KeyCode_Shift);
        
        Bind(vim_enter_mode_visual,      KeyCode_V);
        Bind(vim_enter_mode_visual_line, KeyCode_V, KeyCode_Shift);
        Bind(vim_enter_mode_visual_with_cursor_mark_range,      KeyCode_V, KeyCode_Alt);
        Bind(vim_enter_mode_visual_line_with_cursor_mark_range, KeyCode_V, KeyCode_Shift, KeyCode_Alt);
        
        Bind(vim_enter_chord_z, KeyCode_Z);
        
        Bind(vim_enter_chord_window, KeyCode_W, KeyCode_Control);
        
        // @keycode_missing Bind(vim_enter_chord_switch_register, KeyCode_");
        
        // @keycode_missing Bind(vim_enter_chord_format, KeyCode_=);
        // @keycode_missing Bind(vim_enter_chord_indent_right, KeyCode_>);
        // @keycode_missing Bind(vim_enter_chord_indent_left,  KeyCode_<);
        
        Bind(undo, KeyCode_U);
        Bind(redo, KeyCode_R, KeyCode_Control);
        
        
        Bind(auto_indent_range, KeyCode_Tab);
        Bind(auto_indent_line_at_cursor, KeyCode_Tab, KeyCode_Shift);
#if CODE_PEEK
        // @note Code peek
        Bind(vim_code_peek_open__or__next,     KeyCode_Tab);
        Bind(vim_code_peek_open__or__previous, KeyCode_Tab, KeyCode_Shift);
        Bind(vim_close_code_peek, KeyCode_Escape);
        Bind(vim_code_peek_go_same_panel, KeyCode_Backspace);
        Bind(vim_code_peek_go,            KeyCode_Backspace, KeyCode_Alt);
#endif
        
        // @note :avy
        // Bind(avy_search, KeyCode_A, KeyCode_Alt);
        Bind(vim_avy_goto_string, KeyCode_S, KeyCode_Alt);
        Bind(vim_avy_goto_char, KeyCode_C, KeyCode_Alt);
        
        // @note Build and Debug
        Bind(vim_save_all_dirty_buffers_and_build, KeyCode_M, KeyCode_Alt);
        Bind(goto_next_jump,  KeyCode_N, KeyCode_Alt);
        Bind(goto_prev_jump,  KeyCode_N, KeyCode_Alt, KeyCode_Shift);
        Bind(goto_first_jump, KeyCode_N, KeyCode_Alt, KeyCode_Control);
        
        Bind(vim_toggle_hide_build_panel,   KeyCode_Comma);
        Bind(vim_toggle_build_panel_height, KeyCode_Comma, KeyCode_Shift);
        Bind(vim_change_to_build_panel,     KeyCode_Comma, KeyCode_Control);
        
        // @note: Macro
        Bind(keyboard_macro_start_recording , KeyCode_Minus, KeyCode_Control);
        Bind(keyboard_macro_finish_recording, KeyCode_Minus, KeyCode_Control, KeyCode_Shift);
        Bind(keyboard_macro_replay,           KeyCode_Minus, KeyCode_Alt);
        
        
        Bind(vim_toggle_mouse_suppression, KeyCode_Space, KeyCode_Control);
        Bind(vim_toggle_mouse_suppression, KeyCode_Space, KeyCode_Alt);
    }
    
    SelectMap(mapid_vim_mode_visual);
    {
        ParentMap(mapid_vim_movements);
        
        // :selection
        Bind(vim_visual_select_token_or_word_under_cursor, KeyCode_T, KeyCode_Alt);
        
        // :search
        Bind(vim_search_visual_selection,         KeyCode_4); // KeyCode_Plus
        Bind(vim_reverse_search_visual_selection, KeyCode_4, KeyCode_Alt); // KeyCode_Plus
        
        // @todo bind(context, '"', MDFR_NONE, enter_chord_switch_registers);
        
        Bind(vim_visual_mode_delete, KeyCode_D);
        Bind(vim_visual_mode_delete, KeyCode_X);
        Bind(vim_visual_mode_yank,   KeyCode_Y);
        Bind(vim_visual_mode_change, KeyCode_C);
        
        // @note :avy
        Bind(vim_avy_goto_char,   KeyCode_C, KeyCode_Alt);
        Bind(vim_avy_goto_string, KeyCode_S, KeyCode_Alt);
        Bind(vim_avy_goto_line,   KeyCode_G, KeyCode_Alt);
        
        // :paste
        Bind(vim_visual_paste_and_indent, KeyCode_P);
        Bind(vim_visual_paste_and_indent, KeyCode_P, KeyCode_Shift);
        Bind(vim_paste_next_and_indent, KeyCode_P, KeyCode_Alt);
        
        /* @todo
                query_replace_selection
                vim_query_replace_in_visual_selection_range

        // :move_selection
        bind(context, 'H', MDFR_ALT, vim_move_visual_selection_left);
        bind(context, 'L', MDFR_ALT, vim_move_visual_selection_right);
        bind(context, 'K', MDFR_ALT, vim_move_visual_selection_up);
        bind(context, 'J', MDFR_ALT, vim_move_visual_selection_down);
        */
        
        // :surround
        Bind(vim_visual_surround_with_parenthesis, KeyCode_1); // KeyCode_OpenParenthesis
        Bind(vim_visual_remove_surrounding_parenthesis, KeyCode_2); // KeyCode_CloseParenthesis
        
        Bind(vim_visual_surround_with_braces,      KeyCode_5); // KeyCode_OpenBrace
        Bind(vim_visual_remove_surrounding_braces, KeyCode_3); // KeyCode_CloseBrace
        
        Bind(vim_visual_surround_with_brackets,      KeyCode_7); // KeyCode_OpenBracket
        Bind(vim_visual_remove_surrounding_brackets, KeyCode_6); // KeyCode_CloseBracket
        
        Bind(vim_visual_surround_with_static_if,      KeyCode_Ex1); // KeyCode_Hashtag
        Bind(vim_visual_remove_surrounding_static_if, KeyCode_Ex1, KeyCode_Control); // KeyCode_Hashtag, KeyCode_Control
        
        Bind(vim_visual_surround_with_double_quotes,        KeyCode_Quote, KeyCode_Shift); // KeyCode_DoubleQuote
        Bind(vim_visual_remove_surround_with_double_quotes, KeyCode_Quote, KeyCode_Shift, KeyCode_Control); // KeyCode_DoubleQuote, KeyCode_Control
        
        Bind(vim_visual_surround_with_single_quotes,        KeyCode_Quote); // KeyCode_SingleQuote
        Bind(vim_visual_remove_surround_with_single_quotes, KeyCode_Quote, KeyCode_Control); // KeyCode_SingleQuote, KeyCode_Control
        
        Bind(vim_visual_comment_line_range,   KeyCode_ForwardSlash); // KeyCode_ForwardSlash
        Bind(vim_visual_uncomment_line_range, KeyCode_ForwardSlash, KeyCode_Control); // KeyCode_BackwardSlash, KeyCode_Control
        Bind(vim_visual_surround_with_comment,        KeyCode_0); // KeyCode_Star
        Bind(vim_visual_remove_surround_with_comment, KeyCode_0, KeyCode_Control); // KeyCode_Star, KeyCode_Control
    }
    
    SelectMap(mapid_vim_mode_insert);
    {
        ParentMap(mapid_vim_escape_to_normal_mode);
        // BindTextInput(write_text_input); // @todo Maybe use write_text_input_and_auto_indent for non virtual whitespace files.
        BindTextInput(vim_write_text_and_maybe_auto_close_and_auto_indent);
        
        Bind(vim_mode_insert_start_escape_sequence,  KeyCode_J);
        Bind(vim_mode_insert_finish_escape_sequence, KeyCode_K);
        
        // @note newline is done inside vim_write_text_and_maybe_auto_close_and_auto_indent
        // Bind(vim_newline, KeyCode_Return);
        Bind(backspace_char, KeyCode_Backspace);
        Bind(delete_char, KeyCode_Delete);
        
        // @note other commands
        Bind(vim_enter_chord_z, KeyCode_Z, KeyCode_Control);
        
        // :autocomplete
        Bind(word_complete,               KeyCode_Tab, KeyCode_Shift);
        Bind(word_complete_drop_down,     KeyCode_Tab, KeyCode_Control);
        
        Bind(auto_indent_range,          KeyCode_Tab, KeyCode_Alt);
        Bind(auto_indent_line_at_cursor, KeyCode_Tab, KeyCode_Alt, KeyCode_Shift);
        
        Bind(toggle_show_function_helper, KeyCode_Space, KeyCode_Control);
        Bind(toggle_show_function_helper, KeyCode_Space, KeyCode_Alt);
        
        
        // :move_commands
        // @note move hjkl
        Bind(vim_move_up,    KeyCode_K, KeyCode_Alt);
        Bind(vim_move_down,  KeyCode_J, KeyCode_Alt);
        Bind(vim_move_left,  KeyCode_H, KeyCode_Alt);
        Bind(vim_move_right, KeyCode_L, KeyCode_Alt);
        
        Bind(vim_move_to_file_start, KeyCode_K, KeyCode_Control);
        Bind(vim_move_to_file_end,   KeyCode_J, KeyCode_Control);
        
        Bind(vim_move_to_line_start, KeyCode_H, KeyCode_Alt, KeyCode_Control);
        Bind(vim_move_to_line_end,   KeyCode_L, KeyCode_Alt, KeyCode_Control);
        
        Bind(vim_move_up_by_whitespace,   KeyCode_K, KeyCode_Alt, KeyCode_Control);
        Bind(vim_move_down_by_whitespace, KeyCode_J, KeyCode_Alt, KeyCode_Control);
        
        Bind(vim_move_up_by_page,   KeyCode_K, KeyCode_Shift, KeyCode_Control);
        Bind(vim_move_down_by_page, KeyCode_J, KeyCode_Shift, KeyCode_Control);
        
        Bind(vim_move_up_by_page_half,   KeyCode_K, KeyCode_Shift, KeyCode_Alt, KeyCode_Control);
        Bind(vim_move_down_by_page_half, KeyCode_J, KeyCode_Shift, KeyCode_Alt, KeyCode_Control);
        
        // @note move webg
        Bind(vim_move_right_word_start, KeyCode_W, KeyCode_Alt);
        Bind(vim_move_right_word_end,   KeyCode_E, KeyCode_Alt);
        Bind(vim_move_left_word_start,  KeyCode_B, KeyCode_Alt);
        Bind(vim_move_left_word_end,    KeyCode_G, KeyCode_Alt);
        
        Bind(vim_move_right_token_start, KeyCode_W, KeyCode_Shift, KeyCode_Control);
        Bind(vim_move_right_token_end,   KeyCode_E, KeyCode_Shift, KeyCode_Control);
        Bind(vim_move_left_token_start,  KeyCode_B, KeyCode_Shift, KeyCode_Control);
        Bind(vim_move_left_token_end,    KeyCode_G, KeyCode_Shift, KeyCode_Control);
        
        Bind(vim_move_right_one_after_whitespace,  KeyCode_W, KeyCode_Alt, KeyCode_Control);
        Bind(vim_move_right_one_before_whitespace, KeyCode_E, KeyCode_Alt, KeyCode_Control);
        Bind(vim_move_left_one_before_whitespace,  KeyCode_B, KeyCode_Alt, KeyCode_Control);
        Bind(vim_move_left_one_after_whitespace,   KeyCode_G, KeyCode_Alt, KeyCode_Control);
        
        
        Bind(vim_enter_chord_move_right_to_found,     KeyCode_F, KeyCode_Control);
        Bind(vim_enter_chord_move_right_before_found, KeyCode_T, KeyCode_Control);
        Bind(vim_enter_chord_move_left_to_found,      KeyCode_F, KeyCode_Shift, KeyCode_Control);
        Bind(vim_enter_chord_move_left_before_found,  KeyCode_T, KeyCode_Shift, KeyCode_Control);
    }
    
#if 0
    SelectMap(mapid_vim_insert_chord_single_quote);
    {
        ParentMap(mapid_vim_escape_to_normal_mode);
    }
    
    SelectMap(mapid_vim_insert_chord_double_quote);
    {
        ParentMap(mapid_vim_escape_to_normal_mode);
    }
#endif
    
    SelectMap(mapid_vim_mode_replace);
    {
        ParentMap(mapid_vim_escape_to_normal_mode);
        BindTextInput(vim_replace_character_and_move_right);
        
        Bind(vim_mode_replace_start_escape_sequence,  KeyCode_J);
        Bind(vim_mode_replace_finish_escape_sequence, KeyCode_K);
        
        Bind(move_left,  KeyCode_Backspace);
        Bind(move_right, KeyCode_Delete);
    }
    
    //
    // @note Chord maps
    //
    
    // @note Chord replace single
    SelectMap(mapid_vim_chord_replace_single);
    {
        ParentMap(mapid_vim_escape_to_normal_mode);
        BindTextInput(vim_replace_character_and_enter_mode_normal);
    }
    
    // @note Chord delete
    SelectMap(mapid_vim_chord_delete);
    {
        ParentMap(mapid_vim_escape_to_normal_mode);
        ParentMap(mapid_vim_movements);
        
        Bind(vim_exec_pending_action_on_line_range, KeyCode_D);
        // @todo Bind(vim_visual_change_line, KeyCode_C);
    }
    
    // @note Chord yank
    SelectMap(mapid_vim_chord_yank);
    {
        ParentMap(mapid_vim_escape_to_normal_mode);
        ParentMap(mapid_vim_movements);
        
        Bind(vim_exec_pending_action_on_line_range, KeyCode_Y);
    }
    
    // @note Chord change
    SelectMap(mapid_vim_chord_change);
    {
        ParentMap(mapid_vim_escape_to_normal_mode);
        ParentMap(mapid_vim_movements);
        
        Bind(vim_exec_pending_action_on_line_range, KeyCode_C);
    }
    
    SelectMap(mapid_vim_chord_choose_register);
    {
        ParentMap(mapid_vim_escape_to_normal_mode);
    }
    
    SelectMap(mapid_vim_chord_move_right_to_found);
    {
        ParentMap(mapid_vim_escape_to_normal_mode);
        BindTextInput(vim_move_right_to_found);
    }
    
    SelectMap(mapid_vim_chord_move_right_before_found);
    {
        ParentMap(mapid_vim_escape_to_normal_mode);
        BindTextInput(vim_move_right_before_found);
    }
    
    SelectMap(mapid_vim_chord_move_left_to_found);
    {
        ParentMap(mapid_vim_escape_to_normal_mode);
        BindTextInput(vim_move_left_to_found);
    }
    
    SelectMap(mapid_vim_chord_move_left_before_found);
    {
        ParentMap(mapid_vim_escape_to_normal_mode);
        BindTextInput(vim_move_left_before_found);
    }
    
    SelectMap(mapid_vim_chord_indent_left);
    {
        ParentMap(mapid_vim_escape_to_normal_mode);
        
        // @todo @keycode_missing Bind(vim_exec_pending_action_on_line_range, KeyCode_<);
    }
    
    SelectMap(mapid_vim_chord_indent_right);
    {
        ParentMap(mapid_vim_escape_to_normal_mode);
        
        // @todo @keycode_missing Bind(vim_exec_pending_action_on_line_range, KeyCode_>);
    }
    
    SelectMap(mapid_vim_chord_format);
    {
        ParentMap(mapid_vim_escape_to_normal_mode);
        
        // @todo @keycode_missing Bind(vim_exec_pending_action_on_line_range, KeyCode_=);
    }
    
    SelectMap(mapid_vim_chord_g);
    {
        ParentMap(mapid_vim_escape_to_normal_mode);
        
#if 0 // :vim_listers
        //
        // @note Jumps
        //
        Bind(vim_goto_line, KeyCode_G);
        Bind(vim_jump_to_definition, KeyCode_J);
        // @todo jump_to_definition_under_cursor
        
        //
        // @note Listers
        //
        Bind(vim_list_all_functions_all_buffers, KeyCode_F);
        Bind(vim_list_all_substring_locations_case_insensitive, KeyCode_S);
        
        Bind(vim_list_all_locations, KeyCode_L);
        Bind(vim_list_all_locations_of_selection, KeyCode_V);
        Bind(vim_list_all_locations_of_identifier, KeyCode_I);
        Bind(vim_list_all_locations_of_type_definition, KeyCode_D);
        Bind(vim_list_all_locations_of_type_definition_of_identifier, KeyCode_T);
#endif
    }
    
    SelectMap(mapid_vim_chord_z);
    {
        ParentMap(mapid_vim_escape_to_normal_mode);
        
        Bind(vim_scroll_cursor_line_to_view_center, KeyCode_Z);
        Bind(vim_scroll_cursor_line_to_view_top,    KeyCode_T);
        Bind(vim_scroll_cursor_line_to_view_bottom, KeyCode_B);
    }
    
    SelectMap(mapid_vim_chord_window);
    {
        ParentMap(mapid_vim_escape_to_normal_mode);
        
        Bind(vim_cycle_view_focus, KeyCode_W);
        Bind(vim_cycle_view_focus, KeyCode_W, KeyCode_Control);
        Bind(vim_rotate_view_buffers, KeyCode_R);
        Bind(vim_rotate_view_buffers, KeyCode_R, KeyCode_Control);
        
        Bind(vim_open_view_duplicate_split_vertical,   KeyCode_V);
        Bind(vim_open_view_duplicate_split_vertical,   KeyCode_V, KeyCode_Control);
        Bind(vim_open_view_duplicate_split_horizontal, KeyCode_S);
        Bind(vim_open_view_duplicate_split_horizontal, KeyCode_S, KeyCode_Control);
        
        Bind(vim_open_view_split_horizontal, KeyCode_N);
        Bind(vim_open_view_split_horizontal, KeyCode_N, KeyCode_Control);
        // Bind(vim_open_view_split_vertical, KeyCode_);
        // Bind(vim_open_view_split_vertical, KeyCode_, KeyCode_Control);
        
        Bind(vim_close_view, KeyCode_Q);
        Bind(vim_close_view, KeyCode_Q, KeyCode_Control);
        
        Bind(vim_focus_view_left,  KeyCode_H);
        Bind(vim_focus_view_left,  KeyCode_H, KeyCode_Control);
        Bind(vim_focus_view_down,  KeyCode_J);
        Bind(vim_focus_view_down,  KeyCode_J, KeyCode_Control);
        Bind(vim_focus_view_up,    KeyCode_K);
        Bind(vim_focus_view_up,    KeyCode_K, KeyCode_Control);
        Bind(vim_focus_view_right, KeyCode_L);
        Bind(vim_focus_view_right, KeyCode_L, KeyCode_Control);
        
        Bind(vim_swap_view_left,  KeyCode_H, KeyCode_Alt);
        Bind(vim_swap_view_down,  KeyCode_J, KeyCode_Alt);
        Bind(vim_swap_view_up,    KeyCode_K, KeyCode_Alt);
        Bind(vim_swap_view_right, KeyCode_L, KeyCode_Alt);
    }
}
