function void
tebtro_setup_mapping(Mapping *mapping) {
    MappingScope();
    SelectMapping(mapping);
    
    SelectMap(mapid_vim_unbound);
    {
        BindCore(tebtro_startup, CoreCode_Startup);
        BindCore(default_try_exit, CoreCode_TryExit);
    }
    
    SelectMap(mapid_vim_movements);
    {
        Bind(nop, KeyCode_Period);
        Bind(nop, KeyCode_Period, KeyCode_Shift);
        
        Bind(vim_toggle_build_panel_height, KeyCode_Period);
    }
    
    SelectMap(mapid_vim_mode_normal);
    {
        Bind(copy,         KeyCode_Y, KeyCode_Shift); // KeyCode_Control);
        Bind(delete_range, KeyCode_D, KeyCode_Shift); // KeyCode_Control);
        Bind(cut,          KeyCode_X, KeyCode_Shift); // KeyCode_Control);
    }
    
    SelectMap(mapid_vim_mode_insert);
    {
        // :autocomplete
        Bind(word_complete,              KeyCode_Tab, KeyCode_Shift);
        Bind(word_complete_drop_down,    KeyCode_Tab, KeyCode_Control);
    }
}

