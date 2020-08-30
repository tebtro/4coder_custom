//
// @note: Wrapper around Ryan Feury's calc and plot.
// https://github.com/ryanfleury/4coder_fleury/
//


global_const String_Const_u8 quick_calc_buffer_name = string_u8_litexpr("*calc*");


CUSTOM_COMMAND_SIG(quick_calc_write_result) {
#if CALC_PLOT
    
#endif
}


function void
quick_calc_startup(Application_Links *app) {
    ProfileScope(app, "quick_calc_startup");
    
#if CALC_PLOT
    // @note: Set up fonts
    Face_ID default_face_id = get_face_id(app, 0);
    global_styled_title_face = default_face_id;
    global_styled_label_face = default_face_id;
    
    // @note: Open calc buffer
    Buffer_ID calc_buffer = create_buffer(app, quick_calc_buffer_name, BufferCreate_NeverAttachToFile | BufferCreate_AlwaysNew);
    buffer_set_setting(app, calc_buffer, BufferSetting_Unimportant, true);
    buffer_set_setting(app, calc_buffer, BufferSetting_Unkillable, true);
#endif
}

function void
quick_calc_tick(Application_Links *app, Frame_Info frame_info) {
    ProfileScope(app, "quick_calc_tick");
    
#if CALC_PLOT
    static i32 last_frame_index = -1;
    if (last_frame_index != frame_info.index) {
        CalcUpdateOncePerFrame(frame_info);
    }
    last_frame_index = frame_info.index;
#endif
}

function void
quick_calc_draw(Application_Links *app, Buffer_ID buffer_id, View_ID view_id, Text_Layout_ID text_layout_id, Frame_Info frame_info) {
    ProfileScope(app, "quick_calc_draw");
    
#if CALC_PLOT
    //~ @note: Interpret buffer as calc code, if it's the calc buffer.
    Buffer_ID calc_buffer_id = get_buffer_by_name(app, quick_calc_buffer_name, AccessFlag_Read);
    if(calc_buffer_id == buffer_id) {
        F4_RenderCalcBuffer(app, buffer_id, view_id, text_layout_id, frame_info);
    }
    
    // @note: Render calc comments
    F4_RenderCalcComments(app, buffer_id, view_id, text_layout_id, frame_info);
#endif
}
