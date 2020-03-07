function void
space_mode_setup_mapping(Mapping *mapping) {
    MappingScope();
    SelectMapping(mapping);
    
    // @note set vim normal mode bind to enter space mode
    SelectMap(mapid_vim_mode_normal); {
        Bind(enter_space_mode, KeyCode_Space);
        Bind(toggle_filebar,   KeyCode_Space, KeyCode_Shift);
    }
    
    
    // @note main space mode chord map
    SelectMap(mapid_space_mode); {
        ParentMap(mapid_vim_escape_to_normal_mode);
        
        // Bind(command_lister, KeyCode_Space);
        Bind(whichkey_command_lister, KeyCode_Space);
        
        Bind(enter_space_mode_chord_b, KeyCode_B); // buffer
        Bind(enter_space_mode_chord_f, KeyCode_F); // file
        Bind(enter_space_mode_chord_p, KeyCode_P); // project
        Bind(enter_space_mode_chord_q, KeyCode_Q); // quit
        Bind(enter_space_mode_chord_w, KeyCode_W); // window
        
        
        // @todo Chord for git commands
        Bind(magit_status, KeyCode_G);
    }
    
    // @note buffer chord map
    SelectMap(mapid_space_mode_chord_b); {
        ParentMap(mapid_vim_escape_to_normal_mode);
        
        Bind(vim_interactive_switch_buffer, KeyCode_B);
        Bind(vim_interactive_kill_buffer,   KeyCode_K);
        Bind(vim_kill_current_buffer,       KeyCode_D);
    }
    
    // @note file chord map
    SelectMap(mapid_space_mode_chord_f); {
        ParentMap(mapid_vim_escape_to_normal_mode);
        
#if 0
        Bind(vim_interactive_open_or_new__or__fuzzy_find,           KeyCode_F);
        Bind(vim_interactive_open_or_new__or__fuzzy_find__in_other, KeyCode_F, KeyCode_Alt);
#else
        Bind(vim_interactive_open_or_new__or__switch_buffer,           KeyCode_F);
        Bind(vim_interactive_open_or_new__or__switch_buffer__in_other, KeyCode_F, KeyCode_Alt);
        
        Bind(vim_interactive_open_or_new__or__fuzzy_find,           KeyCode_Z);
        Bind(vim_interactive_open_or_new__or__fuzzy_find__in_other, KeyCode_Z, KeyCode_Alt);
#endif
        Bind(vim_interactive_fuzzy_find,          KeyCode_F, KeyCode_Control);
        Bind(vim_interactive_fuzzy_find__in_other, KeyCode_F, KeyCode_Alt, KeyCode_Control);
        Bind(vim_interactive_open_or_new,          KeyCode_O);
        Bind(vim_interactive_open_or_new__in_other, KeyCode_O, KeyCode_Alt);
        
        // Bind(vim_open_matching_file_cpp, KeyCode_C);
        Bind(vim_open_matching_file_cpp__in_other,   KeyCode_C, KeyCode_Alt);
        
        // Bind(vim_open_file_in_quotes, KeyCode_Q);
        Bind(vim_open_file_in_quotes__in_other, KeyCode_Q, KeyCode_Alt);
        
        Bind(vim_reopen,               KeyCode_R);
        Bind(vim_reopen__in_other,     KeyCode_R, KeyCode_Alt);
        
        
        Bind(vim_save_buffer, KeyCode_S);
        Bind(vim_save_all_dirty_buffers, KeyCode_S, KeyCode_Shift);
        
        
        Bind(vim_rename_file_query,    KeyCode_N, KeyCode_Control);
        Bind(vim_delete_file_query,    KeyCode_D, KeyCode_Control);
        Bind(vim_make_directory_query, KeyCode_M, KeyCode_Control);
    }
    
    // @note project chord map
    SelectMap(mapid_space_mode_chord_p); {
        ParentMap(mapid_vim_escape_to_normal_mode);
        
        Bind(vim_project_command_lister, KeyCode_Space);
        
        Bind(vim_switch_project, KeyCode_S);
        
        Bind(vim_load_project, KeyCode_L);
        Bind(vim_setup_new_project, KeyCode_C);
        Bind(vim_project_go_to_root_directory, KeyCode_H);
        
        Bind(vim_clean_save_all_dirty_buffers_and_build, KeyCode_M);
        Bind(vim_goto_next_jump,  KeyCode_N);
        Bind(vim_goto_prev_jump,  KeyCode_P);
        Bind(vim_goto_first_jump, KeyCode_0);
    }
    
    // @note quit chord map
    SelectMap(mapid_space_mode_chord_q); {
        ParentMap(mapid_vim_escape_to_normal_mode);
        Bind(vim_exit_4coder, KeyCode_Q);
    }
    
    // @note window chord map
    SelectMap(mapid_space_mode_chord_w); {
        ParentMap(mapid_vim_escape_to_normal_mode);
        
        Bind(vim_close_view, KeyCode_D);
        Bind(vim_open_view_duplicate_split_horizontal, KeyCode_Minus);
        Bind(vim_open_view_duplicate_split_vertical,   KeyCode_7, KeyCode_Shift);
        
        
        Bind(vim_cycle_view_focus,    KeyCode_W);
        Bind(vim_rotate_view_buffers, KeyCode_R);
        
        Bind(vim_open_view_duplicate_split_vertical,   KeyCode_V);
        Bind(vim_open_view_duplicate_split_horizontal, KeyCode_S);
        
        Bind(vim_open_view_split_horizontal, KeyCode_N);
        // Bind(vim_open_view_split_vertical,   KeyCode_);
        
        Bind(vim_close_view, KeyCode_Q);
        
        Bind(vim_focus_view_left,  KeyCode_H);
        Bind(vim_focus_view_down,  KeyCode_J);
        Bind(vim_focus_view_up,    KeyCode_K);
        Bind(vim_focus_view_right, KeyCode_L);
        
        Bind(vim_swap_view_left,  KeyCode_H, KeyCode_Alt);
        Bind(vim_swap_view_down,  KeyCode_J, KeyCode_Alt);
        Bind(vim_swap_view_up,    KeyCode_K, KeyCode_Alt);
        Bind(vim_swap_view_right, KeyCode_L, KeyCode_Alt);
    }
}