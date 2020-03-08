#include "tebtro_general.h"


function void
scroll_cursor_line(Application_Links *app, int view_pos, View_ID view_id = -1) {
    ProfileScope(app, "scroll_cursor_line");
    
    if (view_id == -1) {
        view_id = get_active_view(app, Access_Always);
    }
    f32 line_height = 0.f;
    if (view_pos != 0) {
        Buffer_ID buffer_id = view_get_buffer(app, view_id, Access_ReadVisible);
        Face_ID face_id = get_face_id(app, buffer_id);
        Face_Metrics metrics = get_face_metrics(app, face_id);
        line_height = metrics.line_height;
    }
    
    Rect_f32 region = view_get_buffer_region(app, view_id);
    i64 pos = view_get_cursor_pos(app, view_id);
    Buffer_Cursor cursor = view_compute_cursor(app, view_id, seek_pos(pos));
    f32 view_height = rect_height(region);
    Buffer_Scroll scroll = view_get_buffer_scroll(app, view_id);
    scroll.target.line_number = cursor.line;
    if (view_pos == 0) {
        scroll.target.pixel_shift.y = -view_height*0.5f;
    }
    else if (view_pos == -1) {
        scroll.target.pixel_shift.y =  -line_height;
    }
    else if (view_pos == 1) {
        scroll.target.pixel_shift.y = -(view_height - 2.0f*line_height);
        // @todo Not quite right when the line to scroll to is a wrapped line!
    }
    
    view_set_buffer_scroll(app, view_id, scroll, SetBufferScroll_SnapCursorIntoView);
}

CUSTOM_COMMAND_SIG(scroll_cursor_line_to_view_center) {
    scroll_cursor_line(app, 0);
}

CUSTOM_COMMAND_SIG(scroll_cursor_line_to_view_top) {
    scroll_cursor_line(app, -1);
}

CUSTOM_COMMAND_SIG(scroll_cursor_line_to_view_bottom) {
    scroll_cursor_line(app, 1);
}

