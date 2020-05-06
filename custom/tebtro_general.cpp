#include "tebtro_general.h"


//
// @note scroll_cursor_line / center_view
//
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


//
// @note token_or_word_under_cursor
//

// character_predicate_whitespace && !character_predicate_alpha_numeric_underscore
internal i64
boundary_no_whitespace_alpha_numeric_underscore(Application_Links *app, Buffer_ID buffer, Side side, Scan_Direction direction, i64 pos){
    local_persist Character_Predicate a = character_predicate_not(&character_predicate_alpha_numeric_underscore);
    local_persist Character_Predicate b = character_predicate_not(&character_predicate_whitespace);
    local_persist Character_Predicate c = character_predicate_and(&a, &b);
    //Character_Predicate d = character_predicate_not(&b);
    return(boundary_predicate(app, buffer, side, direction, pos, &c));
}
internal Range_i64
enclose_no_whitespace_alpha_numeric_underscore(Application_Links *app, Buffer_ID buffer, i64 pos){
    return(enclose_boundary(app, buffer, Ii64(pos), boundary_no_whitespace_alpha_numeric_underscore));
}

function Range_i64
get_token_or_word_under_cursor_range(Application_Links *app, Buffer_ID buffer_id, i64 cursor_pos, Token *token) {
    Range_i64 range = {}; // gets set to {cursor_pos, cursor_pos} if no good range gets found.
    
    auto do_boundary_search = [app, buffer_id, cursor_pos]() -> Range_i64 {
        u8 ch = buffer_get_char(app, buffer_id, cursor_pos);
        if (character_is_alpha_numeric(ch)) { // || ch == '_'  // character_is_alpha_numeric already includes this.
            return enclose_pos_alpha_numeric_underscore(app, buffer_id, cursor_pos);
        }
        else {
            return enclose_no_whitespace_alpha_numeric_underscore(app, buffer_id, cursor_pos);
            //range = enclose_pos_non_whitespace(app, buffer_id, cursor_pos);
        }
    };
    
    if (token) {
        // @note: Token
        if (token->kind == TokenBaseKind_Comment) {
            if (cursor_pos < token->pos+2) {
                range.start = token->pos;
                range.end   = range.start + 2;
            }
            else if (token->sub_kind == TokenCppKind_BlockComment && (cursor_pos >= token->pos+token->size-2)) {
                range.end   = token->pos+token->size;
                range.start = range.end - 2;
            }
            else {
                range = do_boundary_search();
                
                if (range.start < token->pos+2) {
                    range.start = token->pos + 2;
                }
                else if (token->sub_kind == TokenCppKind_BlockComment) {
                    if (range.end >= token->pos+token->size-2) {
                        range.end = token->pos+token->size - 2;
                    }
                }
            }
        }
        else if (token->kind == TokenBaseKind_LiteralString) {
            if (cursor_pos == token->pos || cursor_pos == token->pos+token->size-1) {
                range.start = cursor_pos;
                range.end   = range.start + 1;
            }
            else {
                range = do_boundary_search();
                
                range.start = clamp(token->pos+1, range.start, token->pos+token->size-1);
                range.end   = clamp(token->pos+1, range.end,   token->pos+token->size-1);
            }
        }
        else if (token->kind == TokenBaseKind_Whitespace || token->kind == TokenBaseKind_EOF) {
            range = {cursor_pos, cursor_pos}; // Return range of size 0 at cursor pos
        }
        else {
            range = Ii64_size(token->pos, token->size);
        }
    }
    else {
        // @note: Word
        
        u8 ch = buffer_get_char(app, buffer_id, cursor_pos);
        if (character_is_whitespace(ch)) {
            range = {cursor_pos, cursor_pos};
        }
        else {
            range = do_boundary_search();
        }
    }
    
    Assert(range.start >= 0 && range.end >= 0);
    return range;
}

function Range_i64
get_token_or_word_under_cursor_range(Application_Links *app, Buffer_ID buffer_id, i64 cursor_pos, Token_Array *token_array) {
    Token *token = 0;
    if (token_array && (token_array->tokens != 0)) {
        Token_Iterator_Array it = token_iterator_pos(0, token_array, cursor_pos);
        token = token_it_read(&it);
    }
    Range_i64 result = get_token_or_word_under_cursor_range(app, buffer_id, cursor_pos, token);
    return result;
}

