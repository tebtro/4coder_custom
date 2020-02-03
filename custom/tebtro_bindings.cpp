#if !defined(TEBTRO_BINDINGS_CPP)
#define TEBTRO_BINDINGS_CPP


#include "tebtro_include.cpp"


void
custom_layer_init(Application_Links *app) {
    Thread_Context *tctx = get_thread_context(app);
    
    // @note Setup for default framework
    async_task_handler_init(app, &global_async_system);
    code_index_init();
    buffer_modified_set_init();
    Profile_Global_List *list = get_core_profile_list(app);
    ProfileThreadName(tctx, list, string_u8_litexpr("main"));
    initialize_managed_id_metadata(app);
    set_default_color_scheme(app); // @note(tebtro): Called to initialize globa_theme_arena and default_color_table
    
    mapping_init(tctx, &framework_mapping);
    setup_default_mapping(&framework_mapping, mapid_global, mapid_file, mapid_code);
    
    //
    // @note Custom framework
    //
    tebtro_set_all_hooks(app);
    
    vim_setup_mapping(&framework_mapping);
    tebtro_setup_mapping(&framework_mapping);
    space_mode_setup_mapping(&framework_mapping);
}

#endif // TEBTRO_BINDINGS_CPP