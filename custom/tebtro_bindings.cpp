#if !defined(TEBTRO_BINDINGS_CPP)
#define TEBTRO_BINDINGS_CPP


#include "tebtro_include.cpp"


void
custom_layer_init(Application_Links *app) {
    Thread_Context *tctx = get_thread_context(app);
    
    default_framework_init(app);
    
    // @note Setup for default framework
    set_default_color_scheme(app); // @note(tebtro): Called to initialize globa_theme_arena and default_color_table
    mapping_init(tctx, &framework_mapping);
    setup_default_mapping(&framework_mapping, mapid_global, mapid_file, mapid_code);
    
    //
    // @note Custom framework
    //
    tebtro_set_all_hooks(app);
    
    vim_setup_mapping(&framework_mapping);
    space_mode_setup_mapping(&framework_mapping);
    // tebtro_setup_mapping(&framework_mapping);
    tebtro_setup_mapping_dvorak_programmer(&framework_mapping);
}

#endif // TEBTRO_BINDINGS_CPP