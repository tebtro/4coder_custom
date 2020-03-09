//
// @note: Avy search  :avy_search
//
//
// @note Usage:
// Funtions:
// - avy_goto_string
//
// Options: (Before including define those)
//
// - #define AVY_CLOSE_FIRST_QUERY_BAR_AFTER_INPUT  true
// - #define AVY_HIDE_SECOND_QUERY_BAR_DURING_SELECTION  true
// - #define AVY_KEY_LIST  "asdfghjkl"
//

#ifndef AVY_CLOSE_FIRST_QUERY_BAR_AFTER_INPUT
#define AVY_CLOSE_FIRST_QUERY_BAR_AFTER_INPUT  true
#endif

#ifndef AVY_HIDE_SECOND_QUERY_BAR_DURING_SELECTION
#define AVY_HIDE_SECOND_QUERY_BAR_DURING_SELECTION  true
#endif

// char *input_chars = "aoeuhtns";
// char *input_chars = "abcdefghijklmnopqrstuvwxyz";
// char *input_chars = "abcd";
#undef AVY_KEY_LIST
#ifndef AVY_KEY_LIST
#define AVY_KEY_LIST  "asdf"
#endif



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

// global int global_generated_count = 0;

// @note: For input_characters "abcd", the prefixes start at the first char.
//        And not at the end, as it would be in emacs avy.
// @todo: For that we would need to order the keys after generation
//        and for removing prefixes we would need to do that in the middle of the keys array.
//        Could store that in a bit_array!
function String_Const_u8 *
avy_generate_keys(Arena *arena, int key_count) {
    char *input_chars = AVY_KEY_LIST;
    u64 input_chars_count = cstring_length(input_chars);
    if (input_chars_count < 4) {
        AssertMessageAlways("You should define at least 4 characters in AVY_KEY_LIST!");
    }
    
    u64 extra_count = key_count / input_chars_count;
    extra_count *= extra_count;
    String_Const_u8 *keys = push_array(arena, String_Const_u8, key_count + extra_count);
    
    int remaining = key_count;
    int generated_count = 0;
    
    for (int i = 0; (i < input_chars_count && remaining > 0); ++i) {
        String_Const_u8 *key = keys + i;
        key->str = push_array(arena, u8, 1);
        key->size = 1;
        key->str[0] = input_chars[i];
        
        --remaining;
        ++generated_count;
    }
    if (remaining <= 0) {
        return keys;
    }
    
    int prefix_count = generated_count;
    for (int i = 0; i < generated_count; ++i) {
        if (i >= prefix_count) {
            i = prefix_count;
            prefix_count = generated_count;
        }
        
        String_Const_u8 *prefix = keys + i;
        ++remaining;
        
        for (int c = 0; c < input_chars_count; ++c) {
            Assert(generated_count < key_count + extra_count);
            String_Const_u8 *key = keys + (generated_count);
            key->size = prefix->size + 1;
            key->str = push_array(arena, u8, key->size);
            for (int x = 0; x < prefix->size; ++x) {
                key->str[x] = prefix->str[x];
            }
            key->str[key->size - 1] = input_chars[c];
            
            --remaining;
            ++generated_count;
            
            if (remaining <= 0) {
                goto loop_end;
            }
        }
    }
    loop_end:;
    
    // global_generated_count = generated_count;
    int prefixes_used = generated_count - key_count;
    keys += prefixes_used;
    
    return keys;
}

function Avy_Pair *
avy_get_selection_from_user(Application_Links *app, Avy_State *avy_state, String_Match_List matches) {
    Avy_Pair *result_pair = 0;
    
    // @todo We maybe also need to save this to the views scope attachment.
    //       To blur out the already written characters.
    Query_Bar key_bar = {};
    u8 key_space[1024];
    key_bar.prompt = string_u8_litexpr("Avy-Jump: ");
    key_bar.string = SCu8(key_space, (u64)0);
    key_bar.string_capacity = sizeof(key_space);
#if 0
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
#else
    
#if !AVY_HIDE_SECOND_QUERY_BAR_DURING_SELECTION
    if (start_query_bar(app, &key_bar, 0) == 0) {
        return;
    }
    defer {
        end_query_bar(app, &key_bar, 0);
    };
#endif
    User_Input in = {};
    for (;;) {
        in = get_next_input(app, EventPropertyGroup_AnyKeyboardEvent, EventProperty_Escape | EventProperty_ViewActivation);
        if (in.abort){
            break;
        }
        
        String_Const_u8 string = to_writable(&in);
        
        b32 string_change = false;
        if (string.str != 0 && string.size > 0) {
            // @note Validate that the string contains only valid characters from the AVY_KEY_LIST.
            b32 is_valid = true;
            String_Const_u8 avy_key_list = SCu8(AVY_KEY_LIST);
            for (int c = 0; c < string.size; ++c) {
                b32 has_match = false;
                for (int k = 0; k < avy_key_list.size; ++k) {
                    if (avy_key_list.str[k] == string.str[c]) {
                        has_match = true;
                        break;
                    }
                }
                if (!has_match) {
                    is_valid = false;
                    break;
                }
            }
            
            if (is_valid) {
                String_u8 bar_string = Su8(key_bar.string, sizeof(key_space));
                string_append(&bar_string, string);
                key_bar.string = bar_string.string;
                string_change = true;
            }
        }
        else if (match_key_code(&in, KeyCode_Backspace)) {
            if (is_unmodified_key(&in.event)) {
                u64 old_bar_string_size = key_bar.string.size;
                key_bar.string = backspace_utf8(key_bar.string);
                string_change = (key_bar.string.size < old_bar_string_size);
            }
            else if (has_modifier(&in.event.key.modifiers, KeyCode_Control)) {
                if (key_bar.string.size > 0) {
                    string_change = true;
                    key_bar.string.size = 0;
                }
            }
        }
        
        if (string_change) {
            for (i32 i = 0; i < matches.count; ++i) {
                Avy_Pair *pair = avy_state->pairs + i;
                if (string_match(pair->key, key_bar.string)) {
                    result_pair = pair;
                }
            }
            if (result_pair != 0) {
                break;
            }
        }
        else {
            leave_current_input_unhandled(app);
        }
    }
#endif
    
    return result_pair;
}


CUSTOM_COMMAND_SIG(avy_goto_string) {
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
    
#if AVY_CLOSE_FIRST_QUERY_BAR_AFTER_INPUT
    end_query_bar(app, &needle_bar, 0);
#else
    defer {
        end_query_bar(app, &needle_bar, 0);
    };
#endif
    
    Avy_Pair *result_pair = avy_get_selection_from_user(app, avy_state, matches);
    if (result_pair != 0) {
        i64 jump_pos = result_pair->range.first;
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
