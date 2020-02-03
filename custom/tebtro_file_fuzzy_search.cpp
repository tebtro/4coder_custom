// @todo @cleanup string mess

function Lister_Activation_Code
lister__write_character__fuzzy_find(Application_Links *app) {
    Lister_Activation_Code result = ListerActivation_Continue;
    
    View_ID view_id = get_this_ctx_view(app, Access_Always);
    Lister *lister = view_get_lister(view_id);
    if (!lister)  return result;
    
    User_Input in = get_current_input(app);
    String_Const_u8 string = to_writable(&in);
    if (!string.str || string.size <= 0)  return result;
    
    lister_append_text_field(lister, string);
    lister_set_key(lister, lister->text_field.string);
    
    lister->item_index = 0;
    lister_zero_scroll(lister);
    
    lister_update_filtered_list(app, lister);
    
    return result;
}

function void
lister__backspace_text_field__fuzzy_find(Application_Links *app) {
    View_ID view_id = get_this_ctx_view(app, Access_Always);
    Lister *lister = view_get_lister(view_id);
    if (!lister)  return;
    if (lister->text_field.size <= 0)  return;
    
    lister->text_field.string = backspace_utf8(lister->text_field.string);
    String_Const_u8 new_key = lister->text_field.string;
    lister_set_key(lister, new_key);
    
    lister->item_index = 0;
    lister_zero_scroll(lister);
    
    lister_update_filtered_list(app, lister);
}


function void
recursive_fill_fuzzy_file_list(Application_Links *app, Lister *lister, Scratch_Block *scratch, String_Const_u8 root_dir, String_Const_u8 sub_dir, 
                               Project_File_Pattern_Array whitelist, Project_File_Pattern_Array blacklist) {
    File_List file_list;
    {
        Temp_Memory temp = begin_temp(lister->arena);
        String_Const_u8 correct_dir = push_string_copy(lister->arena, sub_dir);
        if (!character_is_slash(string_get_character(correct_dir, correct_dir.size - 1))) {
            correct_dir = push_u8_stringf(lister->arena, "%.*s/%.*s/", string_expand(root_dir), string_expand(correct_dir));
        }
        else {
            correct_dir = push_u8_stringf(lister->arena, "%.*s/%.*s", string_expand(root_dir), string_expand(correct_dir));
        }
        file_list = system_get_file_list(scratch->arena, correct_dir);
        end_temp(temp);
    }
    File_Info **one_past_last = file_list.infos + file_list.count;
    push_align(lister->arena, 8);
    // @note recursively fill directories
    for (File_Info **info = file_list.infos; info < one_past_last; ++info) {
        if (!HasFlag((**info).attributes.flags, FileAttribute_IsDirectory))  continue;
        
        String_Const_u8 dir_name = (**info).file_name;
        // :whitelist_blacklist
        if (string_match(dir_name, SCu8(".git")) || 
            string_match(dir_name, SCu8(".vs"))) {
            continue;
        }
        if (match_in_pattern_array(dir_name, blacklist))  continue;
        
        
        Temp_Memory temp = begin_temp(scratch->arena);
        List_String_Const_u8 list = {};
        string_list_push(scratch->arena, &list, sub_dir);
        string_list_push_overlap(scratch->arena, &list, '/', dir_name);
        string_list_push_overlap(scratch->arena, &list, '/', SCu8(""));
        String_Const_u8 temp_full_sub_dir_path = string_list_flatten(scratch->arena, list);
        String_Const_u8 full_sub_dir_path = push_string_copy(lister->arena, temp_full_sub_dir_path);
        end_temp(temp);
        
        recursive_fill_fuzzy_file_list(app, lister, scratch, root_dir, full_sub_dir_path, whitelist, blacklist);
    }
    // @note fill files
    for (File_Info **info = file_list.infos; info < one_past_last; ++info) {
        if (HasFlag((**info).attributes.flags, FileAttribute_IsDirectory))  continue;
        
        String_Const_u8 file_name = (**info).file_name;
        // :whitelist_blacklist
        String_Const_u8 file_extension = string_file_extension(file_name);
        if (string_match(file_extension, SCu8("exe")) ||
            string_match(file_extension, SCu8("obj")) ||
            string_match(file_extension, SCu8("pdb")) ||
            string_match(file_extension, SCu8("rdbg"))) {
            continue;
        }
        if (!match_in_pattern_array(file_name, whitelist))  continue;
        if ( match_in_pattern_array(file_name, blacklist))  continue;
        
        
        Temp_Memory temp = begin_temp(scratch->arena);
        List_String_Const_u8 list = {};
        string_list_push(scratch->arena, &list, sub_dir);
        string_list_push_overlap(scratch->arena, &list, '/', file_name);
        String_Const_u8 temp_full_sub_file_path = string_list_flatten(scratch->arena, list);
        String_Const_u8 full_sub_file_path = push_string_copy(lister->arena, temp_full_sub_file_path);
        
        list = {};
        string_list_push(scratch->arena, &list, root_dir);
        string_list_push_overlap(scratch->arena, &list, '/', full_sub_file_path);
        String_Const_u8 temp_full_file_path = string_list_flatten(scratch->arena, list);
        String_Const_u8 full_file_path = push_string_copy(lister->arena, temp_full_file_path);
        end_temp(temp);
        
        char *is_loaded = "";
        char *status_flag = "";
        Buffer_ID buffer_id = get_buffer_by_file_name(app, full_file_path, Access_Always);
        if (buffer_id != 0) {
            is_loaded = "LOADED ";
            Dirty_State dirty = buffer_get_dirty_state(app, buffer_id);
            switch (dirty){
                case DirtyState_UnsavedChanges:  status_flag = "* "; break;
                case DirtyState_UnloadedChanges: status_flag = "! "; break;
                case DirtyState_UnsavedChangesAndUnloadedChanges: status_flag = "*! "; break;
            }
        }
        
#if 0
        String_Const_u8 text = push_u8_stringf(lister->arena, "%s%s%.*s", is_loaded, status_flag, string_expand(full_sub_file_path));
        Lister_Prealloced_String pre_full_file_path = lister_prealloced(full_file_path);
        lister_add_item(lister, lister_prealloced(file_name), lister_prealloced(text), pre_full_file_path.string.str, 0);
#else
        // remove leading slash
        String_Const_u8 sub_path = push_string_copy(lister->arena, sub_dir);
        if (sub_path.size > 0) {
            ++sub_path.str;
            --sub_path.size;
        }
        String_Const_u8 file_name_copy = push_string_copy(lister->arena, (**info).file_name);
        String_Const_u8 text = push_u8_stringf(lister->arena, "%s%s%.*s", is_loaded, status_flag, string_expand(sub_path));
        Lister_Prealloced_String pre_full_file_path = lister_prealloced(full_file_path);
        lister_add_item(lister, lister_prealloced(file_name_copy), lister_prealloced(text), pre_full_file_path.string.str, 0);
#endif
    }
}

function String_Const_u8
get_fuzzy_root_directory(Application_Links *app, Arena *arena) {
    String_Const_u8 root_dir = {};
    
    if (current_project.loaded) {
        root_dir = current_project.dir;
    }
    else {
        root_dir = push_hot_directory(app, arena);
    }
    
    return root_dir;
}

function void
generate_fuzzy_file_list(Application_Links *app, Lister *lister) {
    Scratch_Block scratch(app);
    
    Project_File_Pattern_Array whitelist;
    Project_File_Pattern_Array blacklist;
    if (current_project.loaded) {
        // @todo In 4coder set_current_project the parsed project is in temp_memory, so we can't access white and blacklist.
        Project_Parse_Result project_parse = parse_project__nearest_file(app, scratch);
        whitelist = project_parse.project->pattern_array;
        blacklist = project_parse.project->blacklist_pattern_array;
    }
    else {
        whitelist = {0};
        blacklist = get_standard_blacklist(scratch);
    }
    
    String_Const_u8 root_dir = get_fuzzy_root_directory(app, scratch);
    if (!character_is_slash(string_get_character(root_dir, root_dir.size - 1))) {
        root_dir = push_u8_stringf(lister->arena, "%.*s/", string_expand(root_dir));
    }
    String_Const_u8 sub_dir = push_string_copy(lister->arena, SCu8(""));
    
    lister_begin_new_item_set(app, lister);
    recursive_fill_fuzzy_file_list(app, lister, &scratch, root_dir, sub_dir, whitelist, blacklist);
}


function File_Name_Result
get_fuzzy_file_name_from_user(Application_Links *app, Arena *arena, String_Const_u8 query, View_ID view_id) {
    Lister_Handlers handlers = lister_get_default_handlers();
    handlers.refresh         = generate_fuzzy_file_list;
    handlers.write_character = lister__write_character__fuzzy_find;
    handlers.backspace       = lister__backspace_text_field__fuzzy_find;
    
    Lister_Result lister_result = run_lister_with_refresh_handler(app, arena, query, handlers);
    
    File_Name_Result result = {0};
    result.canceled = lister_result.canceled;
    if (result.canceled)  return result;
    
    result.clicked = lister_result.activated_by_click;
    if (!lister_result.user_data)  return result;
    
    String_Const_u8 path = SCu8((u8 *)lister_result.user_data);
    result.is_folder = character_is_slash(string_get_character(path, path.size - 1));
    if (result.is_folder)  return result;
    
    result.file_name_activated     = path;
    result.file_name_in_text_field = path;
    result.path_in_text_field      = path;
    
    return result;
}


CUSTOM_COMMAND_SIG(interactive_fuzzy_find) 
CUSTOM_DOC("Interactively fuzzy find a file in the other panel.") {
    View_ID view_id = get_this_ctx_view(app, Access_Always);
    
    Scratch_Block scratch(app);
    
    String_Const_u8 root_dir = get_fuzzy_root_directory(app, scratch);
    String_Const_u8 query = push_u8_stringf(scratch, "Fuzzy open: %.*s", string_expand(root_dir));
    
    File_Name_Result result = get_fuzzy_file_name_from_user(app, scratch, query, view_id);
    if (result.canceled || result.is_folder)  return;
    
    String_Const_u8 file_path = result.file_name_activated;
    if (file_path.size <= 0)  return;
    
    Buffer_ID buffer_id = create_buffer(app, file_path, 0);
    if (buffer_id != 0) {
        view_set_buffer(app, view_id, buffer_id, 0);
    }
}

CUSTOM_COMMAND_SIG(interactive_fuzzy_find_in_other) 
CUSTOM_DOC("Interactively fuzzy find a file recursively up the current directory.") {
    change_active_panel_send_command(app, interactive_fuzzy_find);
}


// @note Use this because we don't want to accidentally start a fuzzy search over the whole drive.
CUSTOM_COMMAND_SIG(interactive_open_or_new__or__fuzzy_find) 
CUSTOM_DOC("If a project is loaded a fuzzy search is started, otherwise a normal query is started.") {
    if (current_project.loaded) {
        interactive_fuzzy_find(app);
    }
    else {
        interactive_open_or_new(app);
    }
}
CUSTOM_COMMAND_SIG(interactive_open_or_new__or__fuzzy_find__in_other) 
CUSTOM_DOC("If a project is loaded a fuzzy search is started, otherwise a normal query is started, in the other view.") {
    if (current_project.loaded) {
        interactive_fuzzy_find_in_other(app);
    }
    else {
        interactive_open_or_new_in_other(app);
    }
}
