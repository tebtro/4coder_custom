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

static Project_List_Item global_tebtro_project_list[3] = {
    { "quick", "D:\\_projects\\quick\\project.4coder" },
    { "handmade", "D:\\_research\\game_dev\\_projects\\handmade_hero\\handmade\\project.4coder" },
    { "4coder_experimental", "D:\\Programme\\4coder_experimental\\project.4coder" },
};

function Project_List_Item 
get_project_from_user(Application_Links *app, Project_List_Item *project_list, i32 project_count, String_Const_u8 query) {
    Scratch_Block scratch(app, Scratch_Share);
    Lister *lister = begin_lister(app, scratch);
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


//
// @note File commands
//

CUSTOM_COMMAND_SIG(interactive_open_or_new_in_other) {
    change_active_panel_send_command(app, interactive_open_or_new);
}
