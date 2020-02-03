function void
tebtro_set_color_scheme(Application_Links *app, Color_Table_List *color_table_list = 0) {
    if (global_theme_arena.base_allocator == 0){
        global_theme_arena = make_arena_system();
    }
    
    Arena *arena = &global_theme_arena;
    
    Color_Table color_table = make_color_table(app, arena);
    
    color_table.arrays[0] = make_colors(arena, 0xFF90B080);
    color_table.arrays[defcolor_bar] = make_colors(arena, 0xFF888888);
    color_table.arrays[defcolor_base] = make_colors(arena, 0xFF000000);
    color_table.arrays[defcolor_pop1] = make_colors(arena, 0xFF3C57DC);
    color_table.arrays[defcolor_pop2] = make_colors(arena, 0xFFFF0000);
    color_table.arrays[defcolor_back] = make_colors(arena, 0xFF0C0C0C);
    color_table.arrays[defcolor_margin] = make_colors(arena, 0xFF181818);
    color_table.arrays[defcolor_margin_hover] = make_colors(arena, 0xFF252525);
    color_table.arrays[defcolor_margin_active] = make_colors(arena, 0xFF323232);
    color_table.arrays[defcolor_list_item] = make_colors(arena, 0xFF181818);
    color_table.arrays[defcolor_list_item_hover] = make_colors(arena, 0xFF252525);
    color_table.arrays[defcolor_list_item_active] = make_colors(arena, 0xFF323232);
    color_table.arrays[defcolor_cursor] = make_colors(arena, 0xFF00EE00);
    color_table.arrays[defcolor_at_cursor] = make_colors(arena, 0xFF0C0C0C);
    color_table.arrays[defcolor_highlight_cursor_line] = make_colors(arena, 0xFF1E1E1E);
    color_table.arrays[defcolor_highlight] = make_colors(arena, 0xFFDDEE00);
    color_table.arrays[defcolor_at_highlight] = make_colors(arena, 0xFFFF44DD);
    color_table.arrays[defcolor_mark] = make_colors(arena, 0xFF494949);
    color_table.arrays[defcolor_text_default] = make_colors(arena, 0xFF90B080);
    color_table.arrays[defcolor_comment] = make_colors(arena, 0xFF2090F0);
    color_table.arrays[defcolor_comment_pop] = make_colors(arena, 0xFF00A000, 0xFFA00000);
    color_table.arrays[defcolor_keyword] = make_colors(arena, 0xFFD08F20);
    color_table.arrays[defcolor_str_constant] = make_colors(arena, 0xFF50FF30);
    color_table.arrays[defcolor_char_constant] = make_colors(arena, 0xFF50FF30);
    color_table.arrays[defcolor_int_constant] = make_colors(arena, 0xFF50FF30);
    color_table.arrays[defcolor_float_constant] = make_colors(arena, 0xFF50FF30);
    color_table.arrays[defcolor_bool_constant] = make_colors(arena, 0xFF50FF30);
    color_table.arrays[defcolor_preproc] = make_colors(arena, 0xFFA0B8A0);
    color_table.arrays[defcolor_include] = make_colors(arena, 0xFF50FF30);
    color_table.arrays[defcolor_special_character] = make_colors(arena, 0xFFFF0000);
    color_table.arrays[defcolor_ghost_character] = make_colors(arena, 0xFF4E5E46);
    color_table.arrays[defcolor_highlight_junk] = make_colors(arena, 0xFF3A0000);
    color_table.arrays[defcolor_highlight_white] = make_colors(arena, 0xFF003A3A);
    color_table.arrays[defcolor_paste] = make_colors(arena, 0xFFDDEE00);
    color_table.arrays[defcolor_undo] = make_colors(arena, 0xFF00DDEE);
    color_table.arrays[defcolor_back_cycle] = make_colors(arena, 0xFF130707, 0xFF071307, 0xFF070713, 0xFF131307);
    color_table.arrays[defcolor_text_cycle] = make_colors(arena, 0xFFA00000, 0xFF00A000, 0xFF0030B0, 0xFFA0A000);
    color_table.arrays[defcolor_line_numbers_back] = make_colors(arena, 0xFF101010);
    color_table.arrays[defcolor_line_numbers_text] = make_colors(arena, 0xFF404040);
    
    active_color_table = color_table;
}
