//
// @note: Avy  :avy
//
//
// @note Usage:
// Funtions:
// - avy_goto_char
// - avy_goto_string
// - avy_goto_line
//
// - avy_goto_view
// - avy_goto_view_and_swap_buffers
// - avy_close_view
//
// - avy_render  Needs to be called in your render_buffer function.
//
//
// Optional settings (Define those before including):
// - #define AVY_KEY_LIST  "asdfghjkl"
// - #define AVY_CLOSE_FIRST_QUERY_BAR_AFTER_INPUT  true
// - #define AVY_HIDE_SECOND_QUERY_BAR_DURING_SELECTION  true
// - #define AVY_VIEW_THRESHOLD 2
//
// - #define AVY_VIEW_IGNORE_UNIMPORTANT_BUFFERS  false
// - #define AVY_VIEW_IGNORE_COMPILATION_BUFFERS  true
//
// - #define AVY_VIEW_SELECTION_CENTER_KEY  false
// - #define AVY_VIEW_SELECTION_BIGGER_KEY  false
//
//

// char *input_chars = "aoeuhtns";
// char *input_chars = "abcdefghijklmnopqrstuvwxyz";
// char *input_chars = "abcd";
#ifndef AVY_KEY_LIST
#define AVY_KEY_LIST  "asdfghjkl"
#endif

#ifndef AVY_CLOSE_FIRST_QUERY_BAR_AFTER_INPUT
#define AVY_CLOSE_FIRST_QUERY_BAR_AFTER_INPUT  true
#endif

#ifndef AVY_HIDE_SECOND_QUERY_BAR_DURING_SELECTION
#define AVY_HIDE_SECOND_QUERY_BAR_DURING_SELECTION  true
#endif

#ifndef AVY_VIEW_THRESHOLD
#define AVY_VIEW_THRESHOLD  2
#endif


#ifndef AVY_VIEW_IGNORE_UNIMPORTANT_BUFFERS
#define AVY_VIEW_IGNORE_UNIMPORTANT_BUFFERS  false
#endif
#ifndef AVY_VIEW_IGNORE_COMPILATION_BUFFERS
#define AVY_VIEW_IGNORE_COMPILATION_BUFFERS  true
#endif


// @todo Position the bigger key correctly in the view if it is no centered.
// @todo Center the text inside the background rect correcly, when its bigger and not centered.
#ifndef AVY_VIEW_SELECTION_CENTER_KEY
#define AVY_VIEW_SELECTION_CENTER_KEY  false
#endif
#ifndef AVY_VIEW_SELECTION_BIGGER_KEY
#define AVY_VIEW_SELECTION_BIGGER_KEY  false
#endif



CUSTOM_ID(attachment, view_visible_range);
CUSTOM_ID(attachment, view_avy_state);

enum Avy_Action {
    avy_action_none = 0,
    
    avy_action_goto_string,
    avy_action_goto_line,
    
    avy_action_goto_view,
};

struct Avy_Pair {
    String_Const_u8 key;
    
    union {
        // @note: For actions goto_string, goto_line
        Range_i64 range;
        // @note: For actions goto_view
        View_ID view_id;
    };
};

struct Avy_View_State {
    Avy_Action action;
    
    union {
        // @note: For actions goto_string, goto_line
        struct {
            int count;
            Avy_Pair *pairs;
        };
        // @note: For actions goto_view
        struct {
            String_Const_u8 key;
            View_ID view_id;
        };
    };
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

struct Avy_Selection_Result {
    b32 abort;
    char character;
};

function Avy_Selection_Result
avy_get_single_character_selection_from_user(Application_Links *app, b32 allow_only_avy_key_list_chars) {
    Avy_Selection_Result result = {};
    
    User_Input in = {};
    for (;;) {
        in = get_next_input(app, EventPropertyGroup_AnyKeyboardEvent, EventProperty_Escape | EventProperty_ViewActivation);
        if (in.abort){
            break;
        }
        
        String_Const_u8 string = to_writable(&in);
        
        if (string.str != 0 && string.size > 0) {
            result.abort = false;
            result.character = string.str[0];
            return result;
        }
        else {
            leave_current_input_unhandled(app);
        }
    }
    
    return result;
}

function Avy_Pair *
avy_get_key_selection_from_user(Application_Links *app, Avy_Pair *pairs, int count) {
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
#else
    
#if !AVY_HIDE_SECOND_QUERY_BAR_DURING_SELECTION
    if (start_query_bar(app, &key_bar, 0) == 0) {
        return 0;
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
            for (i32 i = 0; i < count; ++i) {
                Avy_Pair *pair = pairs + i;
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


//
// @note: Avy buffer/text commands
//

function void
avy_goto_text(Application_Links *app, b32 single_character) {
    Scratch_Block scratch(app);
    Query_Bar_Group group(app);
    
    String_Const_u8 needle = {};
    Query_Bar needle_bar = {};
    if (single_character) {
        Avy_Selection_Result result = avy_get_single_character_selection_from_user(app, false);
        if (result.abort)  return;
        needle = push_string_copy(scratch, SCu8(&result.character, 1));
    }
    else {
        u8 needle_space[1024];
        needle_bar.prompt = string_u8_litexpr("Avy-Search: ");
        needle_bar.string = SCu8(needle_space, (u64)0);
        needle_bar.string_capacity = sizeof(needle_space);
        // @todo Would be nice to already highlight the occurances while typing.
        if (!query_user_string(app, &needle_bar)) {
            return;
        }
        needle = needle_bar.string;
    }
    
    View_ID view_id = get_active_view(app, Access_Always);
    Buffer_ID buffer_id = view_get_buffer(app, view_id, Access_Always);
    
    // @todo @robustness What is the correct and safe way to get the visible_range outside the render loop?
    Managed_Scope view_scope = view_get_managed_scope(app, view_id);
    Range_i64 visible_range = *scope_attachment(app, view_scope, view_visible_range, Range_i64);
    if (range_size(visible_range) <= 0) {
        return;
    }
    
    
    String_Match_List matches = buffer_find_all_matches(app, scratch, buffer_id, 0, visible_range, needle, &character_predicate_alpha_numeric_underscore_utf8, Scan_Forward);
    string_match_list_filter_flags(&matches, StringMatch_CaseSensitive, 0);
    if (matches.count <= 0) {
        return;
    }
    
    Avy_View_State *avy_state = scope_attachment(app, view_scope, view_avy_state, Avy_View_State);
    avy_state->action = avy_action_goto_string;
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
    
    Avy_Pair *result_pair = avy_get_key_selection_from_user(app, avy_state->pairs, avy_state->count);
    if (result_pair != 0) {
        i64 jump_pos = result_pair->range.first;
        view_set_cursor_and_preferred_x(app, view_id, seek_pos(jump_pos));
    }
    
    // @note Center view
    scroll_cursor_line(app, 0, view_id);
}

CUSTOM_COMMAND_SIG(avy_goto_char) {
    avy_goto_text(app, true);
}

CUSTOM_COMMAND_SIG(avy_goto_string) {
    avy_goto_text(app, false);
}

CUSTOM_COMMAND_SIG(avy_goto_line) {
    Scratch_Block scratch(app);
    
    View_ID view_id = get_active_view(app, Access_Always);
    Buffer_ID buffer_id = view_get_buffer(app, view_id, Access_Always);
    
    // @todo @robustness What is the correct and safe way to get the visible_range outside the render loop?
    Managed_Scope view_scope = view_get_managed_scope(app, view_id);
    Range_i64 visible_range = *scope_attachment(app, view_scope, view_visible_range, Range_i64);
    if (range_size(visible_range) <= 0) {
        return;
    }
    Range_i64 line_range = get_line_range_from_pos_range(app, buffer_id, visible_range);
    int line_count = (int)range_size(line_range);
    
    Avy_View_State *avy_state = scope_attachment(app, view_scope, view_avy_state, Avy_View_State);
    avy_state->action = avy_action_goto_line;
    avy_state->count = line_count;
    avy_state->pairs = push_array(scratch, Avy_Pair, line_count);
    defer {
        *avy_state = {0};
    };
    String_Const_u8 *keys = avy_generate_keys(scratch, line_count);
    i64 line = line_range.start;
    for (int i = 0; i < line_count; ++i, ++line) {
        Avy_Pair *pair = avy_state->pairs + i;
        pair->key = keys[i];
        pair->range = Ii64(line);
    }
    
    
    Avy_Pair *result_pair = avy_get_key_selection_from_user(app, avy_state->pairs, avy_state->count);
    if (result_pair != 0) {
        i64 jump_line_number = result_pair->range.first;
        view_set_cursor_and_preferred_x(app, view_id, seek_line_col(jump_line_number, 0));
    }
    
    // @note Center view
    scroll_cursor_line(app, 0, view_id);
}

//
// @note Vim
//
#ifdef VIM
VIM_MOVE_COMMAND_EXECUTE_ONCE(vim_avy_goto_char,   avy_goto_char);
VIM_MOVE_COMMAND_EXECUTE_ONCE(vim_avy_goto_string, avy_goto_string);
VIM_MOVE_COMMAND_EXECUTE_ONCE(vim_avy_goto_line,   avy_goto_line);
#endif


//
// @note: Avy view commands
//

#define avy_for_views(app, it) \
for (View_ID it = get_view_next((app), 0, Access_Always); \
it != 0;                                           \
it = get_view_next((app), it, Access_Always))

function View_ID
avy_get_view_selection_from_user(Application_Links *app) {
    Scratch_Block scratch(app);
    View_ID active_view_id = get_active_view(app, Access_Always);
    View_ID target_view_id = 0;
    b32 active_view_is_hidden_build_buffer = false;
    
    int view_count = 0;
    // @todo: Use a linked_list maybe, so we don't have to loop twice.?
    avy_for_views(app, it) {
        ++view_count;
    }
    View_ID *view_ids = push_array(scratch, View_ID, view_count);
    View_ID *dest = view_ids;
    avy_for_views(app, it) {
        b32 valid_view = true;
        
        Buffer_ID buffer_id = view_get_buffer(app, it, Access_Always);
#if AVY_VIEW_IGNORE_UNIMPORTANT_BUFFERS
        i64 unimportant;
        buffer_get_setting(app, buffer_id, BufferSetting_Unimportant, &unimportant);
        if (unimportant)  valid_view = false;
#endif
        
#if AVY_VIEW_IGNORE_COMPILATION_BUFFERS
        String_Const_u8 buffer_name = push_buffer_base_name(app, scratch, buffer_id);
        if (string_match(buffer_name, string_u8_litexpr("*compilation*"))) {
#ifdef VIM
            if (vim_is_build_panel_hidden)
#endif
            {
                valid_view = false;
                if (active_view_id == it) {
                    active_view_is_hidden_build_buffer = true;
                }
            }
        }
        if (it == build_footer_panel_view_id) {
            valid_view = false;
        }
#endif
        if (!valid_view) {
            *dest = 0;
            --view_count;
            continue;
        }
        *dest = it;
        ++dest;
    }
    
    if (view_count > AVY_VIEW_THRESHOLD || active_view_is_hidden_build_buffer) {
        Avy_Pair *pairs = push_array(scratch, Avy_Pair, view_count);
        String_Const_u8 *keys = avy_generate_keys(scratch, view_count);
        for (int i = 0; i < view_count; ++i) {
            View_ID it = view_ids[i];
            
            // @note: We are itrating over the views backwards I guess, so we reverse the keys here.
            String_Const_u8 *key = keys + (view_count - 1) - i;
            
            Managed_Scope view_scope = view_get_managed_scope(app, it);
            Avy_View_State *avy_state = scope_attachment(app, view_scope, view_avy_state, Avy_View_State);
            avy_state->action = avy_action_goto_view;
            avy_state->key = *key;
            avy_state->view_id = it;
            
            Avy_Pair *pair = pairs + i;
            pair->key = *key;
            pair->view_id = it;
        }
        defer {
            for (int i = 0; i < view_count; ++i) {
                View_ID it = view_ids[i];
                
                Managed_Scope view_scope = view_get_managed_scope(app, it);
                Avy_View_State *avy_state = scope_attachment(app, view_scope, view_avy_state, Avy_View_State);
                *avy_state = {0};
            }
        };
        
        Avy_Pair *result_pair = avy_get_key_selection_from_user(app, pairs, view_count);
        if (result_pair != 0) {
            target_view_id = result_pair->view_id;
        }
    }
    else {
        target_view_id = get_next_view_looped_primary_panels(app, active_view_id, Access_Always);
    }
    return target_view_id;
}

function void
avy_goto_view(Application_Links *app, b32 do_buffer_swap) {
    View_ID active_view_id = get_active_view(app, Access_Always);
    View_ID target_view_id = avy_get_view_selection_from_user(app);
    if (target_view_id != 0) {
        if (do_buffer_swap) {
            Buffer_ID active_buffer_id = view_get_buffer(app, active_view_id, Access_Always);
            Buffer_ID target_buffer_id = view_get_buffer(app, target_view_id, Access_Always);
            if (active_buffer_id != target_buffer_id) {
                view_set_buffer(app, target_view_id, active_buffer_id, Access_Always);
                view_set_buffer(app, active_view_id, target_buffer_id, Access_Always);
            }
            else {
                Buffer_Scroll active_buffer_scroll = view_get_buffer_scroll(app, active_view_id);
                Buffer_Scroll target_buffer_scroll = view_get_buffer_scroll(app, target_view_id);
                Set_Buffer_Scroll_Rule rule = SetBufferScroll_SnapCursorIntoView;
                view_set_buffer_scroll(app, active_view_id, target_buffer_scroll, rule);
                view_set_buffer_scroll(app, target_view_id, active_buffer_scroll, rule);
            }
        }
        view_set_active(app, target_view_id);
    }
}

CUSTOM_COMMAND_SIG(avy_goto_view) {
    avy_goto_view(app, false);
}
CUSTOM_COMMAND_SIG(avy_goto_view_and_swap_buffers) {
    avy_goto_view(app, true);
}
CUSTOM_COMMAND_SIG(avy_close_view) {
    View_ID target_view_id = avy_get_view_selection_from_user(app);
    view_close(app, target_view_id);
}

#ifdef VIM
VIM_VIEW_COMMAND(vim_avy_goto_view, avy_goto_view, true);
VIM_VIEW_COMMAND(vim_avy_goto_view_and_swap_buffers, avy_goto_view_and_swap_buffers, true);
VIM_VIEW_COMMAND(vim_avy_close_view, avy_close_view, true);
#endif


//
// @note: Avy rendering
//
function void
avy_render(Application_Links *app, View_ID view_id, Buffer_ID buffer_id, Text_Layout_ID text_layout_id, Face_ID face_id, Rect_f32 view_region, f32 roundness, ARGB_Color argb_background, ARGB_Color argb_foreground) {
    Face_Metrics face_metrics = get_face_metrics(app, face_id);
    
    Managed_Scope view_scope = view_get_managed_scope(app, view_id);
    Range_i64 *avy_visible_range = scope_attachment(app, view_scope, view_visible_range, Range_i64);
    *avy_visible_range = text_layout_get_visible_range(app, text_layout_id);
    
    Avy_View_State *avy_state = scope_attachment(app, view_scope, view_avy_state, Avy_View_State);
    if (avy_state->action == avy_action_goto_string || avy_state->action == avy_action_goto_line) {
        i32 count = avy_state->count;
        for (i32 i = 0; i < count; ++i) {
            Avy_Pair *pair = avy_state->pairs + i;
            i64 pos = pair->range.first;
            Rect_f32 rect = {};
            if (avy_state->action == avy_action_goto_line) {
                pos = get_line_start_pos(app, buffer_id, pos);
                pos = view_get_character_legal_pos_from_pos(app, view_id, pos);
                rect = text_layout_character_on_screen(app, text_layout_id, pos);
                rect.x0 = view_region.x0;
                rect.x1 = rect.x0 + face_metrics.normal_advance;
            }
            else {
                rect = text_layout_character_on_screen(app, text_layout_id, pos);
            }
            rect.x1 += face_metrics.normal_advance * (pair->key.size - 1);
            draw_rectangle(app, rect, roundness, argb_background);
            Vec2_f32 point = {};
            point.x = rect.x0;
            point.y = rect.y0;
            draw_string(app, face_id, pair->key, point, argb_foreground);
        }
    }
    else if (avy_state->action == avy_action_goto_view) {
        Rect_f32 rect = view_region;
        
#if AVY_VIEW_SELECTION_CENTER_KEY
        Vec2_f32 half_view_dim = rect_dim(view_region) / 2.0f;
        rect.x0 += half_view_dim.x;
        rect.y0 += half_view_dim.y;
#endif
        
        f32 key_widch  = face_metrics.normal_advance * avy_state->key.size;
        f32 key_height = face_metrics.line_height;
        rect.y1 = rect.y0 + face_metrics.line_height;
        rect.x1 = rect.x0 + (face_metrics.normal_advance * avy_state->key.size);
        
        Rect_f32 rect_background = rect;
#if AVY_VIEW_SELECTION_BIGGER_KEY
        rect_background.x0 -= face_metrics.normal_advance * 2;
        rect_background.x1 += face_metrics.normal_advance * 2;
        rect_background.y0 -= face_metrics.line_height * 1;
        rect_background.y1 += face_metrics.line_height * 1;
#endif
        draw_rectangle(app, rect_background, roundness, argb_background);
        Vec2_f32 point = {};
        point.x = rect.x0;
        point.y = rect.y0;
        draw_string(app, face_id, avy_state->key, point, argb_foreground);
    }
}

function void
avy_render(Application_Links *app, View_ID view_id, Text_Layout_ID text_layout_id, Rect_f32 view_region) {
    ARGB_Color argb_background = 0xFFFFFF00;
    ARGB_Color argb_foreground = 0xFF000000;
    
    Buffer_ID buffer_id = view_get_buffer(app, view_id, Access_Always);
    Face_ID face_id = get_face_id(app, buffer_id);
    
    Face_Metrics face_metrics = get_face_metrics(app, face_id);
    f32 roundness = (face_metrics.normal_advance*0.5f)*0.9f;
    
    avy_render(app, view_id, buffer_id, text_layout_id, face_id, view_region, roundness, argb_background, argb_foreground);
}
