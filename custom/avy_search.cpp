//
// @note: Avy search  :avy_search
//

CUSTOM_ID(attachment, view_visible_range);
CUSTOM_ID(attachment, view_avy_state);

struct Avy_Pair {
    String_Const_u8 key;
    Range_i64 range;
};

struct Avy_State {
    i32 count;
    Avy_Pair *pairs;
};

function String_Const_u8 *
avy_generate_keys(Arena *arena, int key_count) {
    String_Const_u8 *keys = push_array(arena, String_Const_u8, key_count);
    
    // char *input_chars = "aoeuhtns";
    char *input_chars = "abcdefghijklmnopqrstuvwxyz";
    // @note k
    u64 input_chars_count = cstring_length(input_chars);
    // @note n
    u64 key_length = key_count / input_chars_count;
    
    if (key_count <= input_chars_count) {
        for (int i = 0; i < key_count; ++i) {
            String_Const_u8 *key = keys + i;
            key->str = push_array(arena, u8, 1);
            key->size = 1;
            
            key->str[0] = input_chars[i];
        }
    }
    else {
        // @todo @robustness We only support one char, so just one abc.
        for (int i = 0; i < key_count; ++i) {
            String_Const_u8 *key = keys + i;
            key->str = push_array(arena, u8, 1);
            key->size = 1;
            
            key->str[0] = input_chars[i % input_chars_count];
        }
    }
    
    return keys;
}

// @todo Make it more interactive, so that you dont have to press enter every time.
CUSTOM_COMMAND_SIG(avy_search) {
    Scratch_Block scratch(app);
    Query_Bar_Group group(app);
    
    Query_Bar needle_bar = {};
    u8 needle_space[1024];
    needle_bar.prompt = string_u8_litexpr("Avy-Search: ");
    needle_bar.string = SCu8(needle_space, (u64)0);
    needle_bar.string_capacity = sizeof(needle_space);
    // @todo Would be nice to already highlight the occurances while typing.
    if (!query_user_string(app, &needle_bar)) {
        return;
    }
    
    View_ID view_id = get_active_view(app, Access_Always);
    Buffer_ID buffer_id = view_get_buffer(app, view_id, Access_Always);
    
    // @todo @robustness What is the correct and safe way to get the visible_range outside the render loop?
    Managed_Scope view_scope = view_get_managed_scope(app, view_id);
    Range_i64 visible_range = *scope_attachment(app, view_scope, view_visible_range, Range_i64);
    if (range_size(visible_range) <= 0) {
        return;
    }
    
    
    String_Const_u8 needle = needle_bar.string;
    String_Match_List matches = buffer_find_all_matches(app, scratch, buffer_id, 0, visible_range, needle, &character_predicate_alpha_numeric_underscore_utf8, Scan_Forward);
    string_match_list_filter_flags(&matches, StringMatch_CaseSensitive, 0);
    if (matches.count <= 0) {
        return;
    }
    
    Avy_State *avy_state = scope_attachment(app, view_scope, view_avy_state, Avy_State);
    avy_state->count = matches.count;
    avy_state->pairs = push_array(scratch, Avy_Pair, matches.count);
    defer {
        *avy_state = {0};
    };
    String_Match *match = matches.first;
    String_Const_u8 *keys = avy_generate_keys(scratch, matches.count);
    for (i32 i = 0; i < matches.count; ++i) {
        Avy_Pair *pair = avy_state->pairs + i;
        pair->key = keys[i];
        pair->range = match->range;
        match = match->next;
    }
    
    // @todo We maybe also need to save this to the views scope attachment.
    Query_Bar key_bar = {};
    u8 key_space[1024];
    key_bar.prompt = string_u8_litexpr("Avy-Jump: ");
    key_bar.string = SCu8(key_space, (u64)0);
    key_bar.string_capacity = sizeof(key_space);
    if (!query_user_string(app, &key_bar)) {
        return;
    }
    
    i64 jump_pos = -1;
    for (i32 i = 0; i < matches.count; ++i) {
        Avy_Pair *pair = avy_state->pairs + i;
        if (string_match(pair->key, key_bar.string)) {
            jump_pos = pair->range.first;
        }
    }
    if (jump_pos >= 0) {
        view_set_cursor_and_preferred_x(app, view_id, seek_pos(jump_pos));
    }
    
    
    // @note Center view
    scroll_cursor_line(app, 0, view_id);
}

//
// @note: Avy rendering
//
function void
avy_search_render(Application_Links *app, View_ID view_id, Text_Layout_ID text_layout_id, Face_ID face_id, f32 roundness, ARGB_Color argb_background, ARGB_Color argb_foreground) {
    Managed_Scope view_scope = view_get_managed_scope(app, view_id);
    Range_i64 *avy_visible_range = scope_attachment(app, view_scope, view_visible_range, Range_i64);
    *avy_visible_range = text_layout_get_visible_range(app, text_layout_id);
    
    Avy_State *avy_state = scope_attachment(app, view_scope, view_avy_state, Avy_State);
    i32 count = avy_state->count;
    for (i32 i = 0; i < count; ++i) {
        Avy_Pair *pair = avy_state->pairs + i;
        // @todo change to draw rect
        // draw_character_block(app, text_layout_id, pair->range, cursor_roundness, argb_color);
        draw_character_block(app, text_layout_id, Ii64(pair->range.first, pair->range.first + pair->key.size), roundness, argb_background);
        Rect_f32 rect = text_layout_character_on_screen(app, text_layout_id, pair->range.first);
        Vec2_f32 point = {};
        point.x = rect.x0;
        point.y = rect.y0;
        draw_string(app, face_id, pair->key, point, argb_foreground);
    }
}

function void
avy_search_render(Application_Links *app, View_ID view_id, Text_Layout_ID text_layout_id) {
    ARGB_Color argb_background = 0xFFFFFF00;
    ARGB_Color argb_foreground = 0xFF000000;
    
    Buffer_ID buffer_id = view_get_buffer(app, view_id, Access_Always);
    Face_ID face_id = get_face_id(app, buffer_id);
    
    Face_Metrics face_metrics = get_face_metrics(app, face_id);
    f32 roundness = (face_metrics.normal_advance*0.5f)*0.9f;
    
    avy_search_render(app, view_id, text_layout_id, face_id, roundness, argb_background, argb_foreground);
}
