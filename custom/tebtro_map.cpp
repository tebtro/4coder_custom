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

