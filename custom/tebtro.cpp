#include "tebtro.h"


//
// @note Zen / Focus mode
//
CUSTOM_COMMAND_SIG(toggle_focus_mode) {
    global_focus_mode_enabled = !global_focus_mode_enabled;
}

//
// @note Tebtro theme mode :tebtro_theme_mode
//
internal void
tebtro_set_theme_mode(Application_Links *app, Tebtro_Theme_Mode new_mode) {
    global_tebtro_theme_mode = new_mode;
    
    Color_Table *colors = get_color_table_by_name(SCu8(global_tebtro_theme_mode_names[new_mode]));
    set_active_color(colors);
    
    vim_setup_mode_and_chord_color_tables(app);
}
CUSTOM_COMMAND_SIG(tebtro_set_default_theme_mode) {tebtro_set_theme_mode(app, TEBTRO_THEME_MODE_default);}
CUSTOM_COMMAND_SIG(tebtro_set_darker_theme_mode)  {tebtro_set_theme_mode(app, TEBTRO_THEME_MODE_darker); }
CUSTOM_COMMAND_SIG(tebtro_set_light_theme_mode)   {tebtro_set_theme_mode(app, TEBTRO_THEME_MODE_light);  }


//
// @note Buffer commands
//

// @todo: The default clean all lines was updates in an update, repalce this with the default again?
// @todo: The default clean all lines was updates in an update, repalce this with the default again?
// @todo: The default clean all lines was updates in an update, repalce this with the default again?
internal void
tebtro_clean_all_lines(Application_Links *app, Buffer_ID buffer) {
    ProfileScope(app, "clean all lines");
    
    Scratch_Block scratch(app);
    Batch_Edit *batch_first = 0;
    Batch_Edit *batch_last = 0;
    
    i64 line_count = buffer_get_line_count(app, buffer);
    for (i64 line_number = 1; line_number <= line_count; line_number += 1) {
        i64 line_start = get_line_side_pos(app, buffer, line_number, Side_Min);
        i64 line_end = get_line_side_pos(app, buffer, line_number, Side_Max);
        u8 prev = buffer_get_char(app, buffer, line_end - 1);
        b32 has_cr_character = false;
        b32 has_tail_whitespace = false;
        if (prev == '\r') {
            has_cr_character = true;
            if (line_end - 2 >= line_start) {
                prev = buffer_get_char(app, buffer, line_end - 2);
                has_tail_whitespace = character_is_whitespace(prev);
            }
        }
        else {
            has_tail_whitespace = character_is_whitespace(prev);
        }
        if (has_tail_whitespace) {
            String_Const_u8 line = push_buffer_range(app, scratch, buffer, Ii64(line_start, line_end));
            if (line.size > 0) {
                // @note Dont remove whitespaces from :empty_lines
                {
                    b32 is_all_whitespace = true;
                    for (int i = 0; i < line.size; ++i) {
                        if (!character_is_whitespace(line.str[i])) {
                            is_all_whitespace = false;
                            break;
                        }
                    }
                    if (is_all_whitespace)  continue;
                }
                
                i64 end_offset = line.size;
                i64 i = line.size - 1;
                if (has_cr_character) {
                    end_offset -= 1;
                    i -= 1;
                }
                i64 start_offset = 0;
                for (; i >= 0; i -= 1) {
                    if (!character_is_whitespace(line.str[i])) {
                        start_offset = i + 1;
                        break;
                    }
                }
                
                i64 start = start_offset + line_start;
                i64 end   = end_offset   + line_start;
                
                Batch_Edit *batch = push_array(scratch, Batch_Edit, 1);
                sll_queue_push(batch_first, batch_last, batch);
                batch->edit.text = SCu8();
                batch->edit.range = Ii64(start, end);
            }
        }
    }
    
    if (batch_first != 0) {
        buffer_batch_edit(app, buffer, batch_first);
    }
}

CUSTOM_COMMAND_SIG(tebtro_clean_all_lines)
CUSTOM_DOC("Removes trailing whitespace from all lines in the current buffer.") {
    View_ID view_id = get_active_view(app, Access_ReadWriteVisible);
    Buffer_ID buffer_id = view_get_buffer(app, view_id, Access_ReadWriteVisible);
    
    tebtro_clean_all_lines(app, buffer_id);
}


//
// @note Identifier list
//

function u64
djb2(String_Const_u8 str) {
    u64 hash = 5381;
    int c;
    
    for (int i = 0; i < str.size; ++i) {
        c = (int)(str.str[i]);
        hash = ((hash << 5) + hash) + c;
        // hash * 33 + c
    }
    
    return hash;
}

function Code_Index_Identifier_Node *
code_index_identifier_table_lookup(Code_Index_Identifier_Hash_Table *table, u64 hash) {
    i32 bucket = hash % table->count;
    
    if (table->table[bucket] == 0) {
        return 0;
    }
    
    Code_Index_Identifier_Node *node = table->table[bucket];
    
    while (node) {
        if (node->hash == hash) {
            return node;
        }
        node = node->next;
    }
    
    return 0;
}

function Code_Index_Identifier_Hash_Table
code_index_identifier_table_from_array(Application_Links *app, Buffer_ID buffer_id, Arena *arena, Code_Index_Note_Ptr_Array note_array) {
    Code_Index_Identifier_Hash_Table result = {};
    
    result.count = 1000;
    result.table = push_array_zero(arena, Code_Index_Identifier_Node *, result.count);
    
    for (i32 i = 0; i < note_array.count; ++i) {
        u64 hash = djb2(note_array.ptrs[i]->text);
        
#if 0
        b32 skip = false;
        for (Buffer_ID buffer_it = get_buffer_next(app, 0, Access_Always);
             !skip && buffer_it != 0;
             buffer_it = get_buffer_next(app, buffer_it, Access_Always)) {
            Managed_Scope scope = buffer_get_managed_scope(app, buffer_it);
            Code_Index_Identifier_Hash_Table *identifier_table = scope_attachment(app, scope, attachment_code_index_identifier_table, Code_Index_Identifier_Hash_Table);
            if (!identifier_table)  continue;
            if (identifier_table->count == 0)  continue;
            Code_Index_Identifier_Node *found = code_index_identifier_table_lookup(identifier_table, hash);
            if (found != 0) {
                Code_Index_Note_Kind note_kind = note_array.ptrs[i]->note_kind;
                if (found->note_kind == CodeIndexNote_Function && note_kind == CodeIndexNote_Type) {
                    found->note_kind = note_kind;
                    skip = true;
                }
                else if (found->note_kind == CodeIndexNote_Function && note_kind == CodeIndexNote_Macro) {
                    found->note_kind = note_kind;
                    skip = true;
                }
            }
        }
        if (skip)  continue;
#endif
        
        Code_Index_Identifier_Node *node = push_array(arena, Code_Index_Identifier_Node, 1);
        node->hash = hash;
        node->note_kind = note_array.ptrs[i]->note_kind;
        node->next = 0;
        
        i32 bucket = node->hash % result.count;
        if (result.table[bucket] != 0) {
#if 0
            Code_Index_Identifier_Node *found = result.table[bucket];
            if (found->hash == node->hash) {
                if (found->note_kind == CodeIndexNote_Function && node->note_kind == CodeIndexNote_Type) {
                    found->note_kind = node->note_kind;
                }
                else if (found->note_kind == CodeIndexNote_Function && node->note_kind == CodeIndexNote_Macro) {
                    found->note_kind = node->note_kind;
                }
                continue;
            }
            else {
            }
#endif
            node->next = result.table[bucket];
        }
        result.table[bucket] = node;
    }
    
    return result;
}


//
// @note Project commands
//

// @copynpaste 4coder
function Project_Parse_Result
parse_project__file_name(Application_Links *app, Arena *arena, char *file_name) {
    Project_Parse_Result result = {0};
    if (!file_name)  return result;
    
    Temp_Memory restore_point = begin_temp(arena);
    
    File_Name_Data dump = dump_file(arena, SCu8(file_name));
    // File_Name_Data dump = dump_file_search_up_path(app, arena, project_path, string_u8_litexpr("project.4coder"));
    if (dump.data.data != 0) {
        String_Const_u8 project_root = string_remove_last_folder(dump.file_name);
        set_hot_directory(app, project_root);
        result = parse_project__data(app, arena, dump.file_name, dump.data, project_root);
    }
    else {
        List_String_Const_u8 list = {};
        string_list_push(arena, &list, string_u8_litexpr("Did not find project.4coder.  "));
        if (current_project.loaded) {
            string_list_push(arena, &list, string_u8_litexpr("Continuing with: "));
            if (current_project.name.size > 0){
                string_list_push(arena, &list, current_project.name);
            }
            else {
                string_list_push(arena, &list, current_project.dir);
            }
        }
        else {
            string_list_push(arena, &list, string_u8_litexpr("Continuing without a project."));
        }
        string_list_push(arena, &list, string_u8_litexpr("\n"));
        String_Const_u8 message = string_list_flatten(arena, list);
        print_message(app, message);
        end_temp(restore_point);
    }
    
    return result;
}

struct Project_List_Item {
    char *name;
    char *path;
};

// @todo: Move this out into a file. Like a master project file.
static Project_List_Item global_tebtro_project_list[3] = {
    { "quick", "D:\\_projects\\quick\\project.4coder" },
    { "handmade", "D:\\_research\\game_dev\\_projects\\handmade_hero\\handmade\\project.4coder" },
    { "4coder_experimental", "D:\\Programme\\4coder_experimental\\project.4coder" },
};

function Project_List_Item
get_project_from_user(Application_Links *app, Project_List_Item *project_list, i32 project_count, String_Const_u8 query) {
    Scratch_Block scratch(app, Scratch_Share);
    Lister *lister = begin_lister(app, scratch).current;
    lister_set_query(lister, query);
    lister->handlers = lister_get_default_handlers();
    {
        Project_List_Item *project = project_list;
        for (i32 i = 0; i < project_count; ++i, ++project) {
            lister_add_item(lister, SCu8(project->name), SCu8(project->path), project, 0);
        }
    }
    
    Lister_Result lister_result = run_lister(app, lister);
    if (lister_result.canceled)  return {0};
    
    Project_List_Item *project = (Project_List_Item *)lister_result.user_data;
    return *project;
}

CUSTOM_COMMAND_SIG(switch_project) {
    String_Const_u8 query = SCu8("Project: ");
    Project_List_Item result = get_project_from_user(app, global_tebtro_project_list, ArrayCount(global_tebtro_project_list), query);
    if (ArrayCount(result.path) <= 0)  return;
    
    Scratch_Block scratch(app, Scratch_Share);
    Project_Parse_Result project_parse = parse_project__file_name(app, scratch, result.path);
    set_current_project(app, project_parse.project, project_parse.parsed);
}


