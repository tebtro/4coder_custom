
struct Tebtro_Tiny_Jump {
    Buffer_ID buffer_id;
    i64 pos;
};

function void
tebtro_jump_to_definition(Application_Links *app, bool use_ident_under_cursor = false) {
    char *query = "Definition:";
    
    Scratch_Block scratch(app);
    Lister_Block lister(app, scratch);
    lister_set_query(lister, query);
    lister_set_default_handlers(lister);
    
    if (use_ident_under_cursor) {
        String_Const_u8 ident = push_token_or_word_under_active_cursor(app, scratch);
        lister_set_text_field(lister, ident);
        lister_set_key(lister, ident);
    }
    
    code_index_lock();
    for (Buffer_ID buffer_id = get_buffer_next(app, 0, Access_Always);
         buffer_id != 0;
         buffer_id = get_buffer_next(app, buffer_id, Access_Always)) {
        Code_Index_File *file = code_index_get_file(buffer_id);
        if (file != 0) {
            for (i32 i = 0; i < file->note_array.count; i += 1) {
                Code_Index_Note *note = file->note_array.ptrs[i];
                Tebtro_Tiny_Jump *jump = push_array(scratch, Tebtro_Tiny_Jump, 1);
                jump->buffer_id = buffer_id;
                jump->pos = note->pos.first;
                
                String_Const_u8 sort = {};
                switch (note->note_kind) {
                    case CodeIndexNote_Type: {
                        sort = string_u8_litexpr("type");
                    } break;
                    case CodeIndexNote_Function: {
                        sort = string_u8_litexpr("function");
                    } break;
                    case CodeIndexNote_Macro:{
                        sort = string_u8_litexpr("macro");
                    } break;
                }
                
                String_Const_u8 buffer_name = push_buffer_unique_name(app, scratch, buffer_id);
                
                String_Const_u8 text = push_u8_stringf(scratch, "%.*s - <%.*s>", string_expand(sort), string_expand(buffer_name));
                lister_add_item(lister, note->text, text, jump, 0);
            }
        }
    }
    code_index_unlock();
    
    Lister_Result l_result = run_lister(app, lister);
    Tebtro_Tiny_Jump result = {};
    if (!l_result.canceled && l_result.user_data != 0) {
        block_copy_struct(&result, (Tebtro_Tiny_Jump*)l_result.user_data);
    }
    
    if (result.buffer_id != 0) {
        View_ID view_id = get_this_ctx_view(app, Access_Always);
        jump_to_location(app, view_id, result.buffer_id, result.pos);
    }
}


CUSTOM_UI_COMMAND_SIG(tebtro_jump_to_definition)
CUSTOM_DOC("List all definitions in the code index and jump to one chosen by the user.") {
    tebtro_jump_to_definition(app, false);
}

CUSTOM_UI_COMMAND_SIG(tebtro_jump_to_definition_under_cursor)
CUSTOM_DOC("List all definitions in the code index and jump to one chosen by the user.") {
    tebtro_jump_to_definition(app, true);
}


#ifdef VIM
#define vim_tebtro_jump_to_definition vim_chord_command<tebtro_jump_to_definition>
#define vim_tebtro_jump_to_definition_under_cursor vim_chord_command<tebtro_jump_to_definition_under_cursor>
#endif
