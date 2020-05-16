/*
* Default color slots
*/

#if !defined(VIM_DEFAULT_COLORS_H)


CUSTOM_ID(colors, defcolor_bar);
CUSTOM_ID(colors, defcolor_bar_active);
CUSTOM_ID(colors, defcolor_base);
CUSTOM_ID(colors, defcolor_pop1);
CUSTOM_ID(colors, defcolor_pop2);
CUSTOM_ID(colors, defcolor_back);
CUSTOM_ID(colors, defcolor_margin);
CUSTOM_ID(colors, defcolor_margin_hover);
CUSTOM_ID(colors, defcolor_margin_active);
CUSTOM_ID(colors, defcolor_margin_keyboard_macro_is_recording);
CUSTOM_ID(colors, defcolor_list_item);
CUSTOM_ID(colors, defcolor_list_item_hover);
CUSTOM_ID(colors, defcolor_list_item_active);
CUSTOM_ID(colors, defcolor_cursor);
CUSTOM_ID(colors, defcolor_at_cursor);
CUSTOM_ID(colors, defcolor_highlight_cursor_line);
CUSTOM_ID(colors, defcolor_highlight_token_under_cursor);
CUSTOM_ID(colors, defcolor_highlight_whitespaces);
CUSTOM_ID(colors, defcolor_highlight_range_vertical_line);
CUSTOM_ID(colors, defcolor_highlight);
CUSTOM_ID(colors, defcolor_at_highlight);
CUSTOM_ID(colors, defcolor_search_highlight);
CUSTOM_ID(colors, defcolor_at_search_highlight);
CUSTOM_ID(colors, defcolor_search_occurance_highlight);
CUSTOM_ID(colors, defcolor_at_search_occurance_highlight);
CUSTOM_ID(colors, defcolor_mark);
CUSTOM_ID(colors, defcolor_text_default);
CUSTOM_ID(colors, defcolor_comment);
enum Comment_Pop_Sub_Kind {
    COMMENT_POP_note = 0,
    COMMENT_POP_todo,
    COMMENT_POP_study,
    COMMENT_POP_tag,
    COMMENT_POP_correlation_tag,
    COMMENT_POP_username,
};
CUSTOM_ID(colors, defcolor_comment_pop);
CUSTOM_ID(colors, defcolor_keyword);
CUSTOM_ID(colors, defcolor_str_constant);
CUSTOM_ID(colors, defcolor_char_constant);
CUSTOM_ID(colors, defcolor_int_constant);
CUSTOM_ID(colors, defcolor_float_constant);
CUSTOM_ID(colors, defcolor_bool_constant);
CUSTOM_ID(colors, defcolor_preproc);
CUSTOM_ID(colors, defcolor_include);
CUSTOM_ID(colors, defcolor_special_character);
CUSTOM_ID(colors, defcolor_ghost_character);
CUSTOM_ID(colors, defcolor_highlight_junk);
CUSTOM_ID(colors, defcolor_highlight_white);
CUSTOM_ID(colors, defcolor_paste);
CUSTOM_ID(colors, defcolor_undo);
CUSTOM_ID(colors, defcolor_back_cycle);
CUSTOM_ID(colors, defcolor_text_cycle);
CUSTOM_ID(colors, defcolor_line_numbers_back);
CUSTOM_ID(colors, defcolor_line_numbers_text);

CUSTOM_ID(colors, defcolor_type);
CUSTOM_ID(colors, defcolor_type_function);
CUSTOM_ID(colors, defcolor_type_macro);
CUSTOM_ID(colors, defcolor_operator);
CUSTOM_ID(colors, defcolor_square_bracket);

CUSTOM_ID(colors, defcolor_parenthesis_cycle);
CUSTOM_ID(colors, defcolor_scope_background_cycle);
CUSTOM_ID(colors, defcolor_scope_vertical_line_cycle);
CUSTOM_ID(colors, defcolor_scope_brace_cycle);
CUSTOM_ID(colors, defcolor_scope_close_brace_annotation_cycle);


struct Color_Table_Node{
    Color_Table_Node *next;
    String_Const_u8 name;
    Color_Table table;
};

struct Color_Table_List{
    Color_Table_Node *first;
    Color_Table_Node *last;
    i32 count;
};

global Color_Table active_color_table = {};
global Color_Table default_color_table = {};

global Arena global_theme_arena = {};
global Color_Table_List global_theme_list = {};


#define VIM_DEFAULT_COLORS_H
#endif
