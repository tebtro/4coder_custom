
CUSTOM_ID(attachment, tebtro_marker_test_id);

CUSTOM_COMMAND_SIG(tebtro_test_markers_remove) {
    View_ID view_id = get_active_view(app, Access_Always);
    Managed_Scope scope = view_get_managed_scope(app, view_id);
    Managed_Object *markers_object = scope_attachment(app, scope, tebtro_marker_test_id, Managed_Object);
    
    if (*markers_object != 0) {
        Marker marker_range[2];
        if (managed_object_load_data(app, *markers_object, 0, 2, &marker_range)) {
            Range_i64 range = Ii64(marker_range[0].pos, marker_range[1].pos);
        }
        
        
        managed_object_free(app, *markers_object);
    }
    managed_scope_attachment_erase(app, scope, tebtro_marker_test_id);
}

CUSTOM_COMMAND_SIG(tebtro_test_markers_set) {
    View_ID view_id = get_active_view(app, Access_Always);
    Buffer_ID buffer_id = view_get_buffer(app, view_id, Access_Always);
    Managed_Scope scope = view_get_managed_scope(app, view_id);
    Managed_Object *markers_object = scope_attachment(app, scope, tebtro_marker_test_id, Managed_Object);
    *markers_object = alloc_buffer_markers_on_buffer(app, buffer_id, 2, &scope);
    Marker marker_range[2] = {};
    marker_range[0].pos = view_get_cursor_pos(app, view_id);
    marker_range[1].pos = view_get_cursor_pos(app, view_id);
    marker_range[0].lean_right = true;
    marker_range[1].lean_right = true;
    managed_object_store_data(app, *markers_object, 0, 2, marker_range);
}


//
// @note Needs to be called in buffer_edit_range set by:
// set_custom_hook(app, HookID_BufferEditRange, default_buffer_edit_range);
void origami_buffer_edit_range_update(Range_i64 old_range, i64 new_range_size) {
    // @todo Remove the marker if he is inside a range beeing deleted.
}


//
// @note Origami / Code Folding
//
// Called origami because of an emacs mode.
//

//
// @todo Code folding only works for buffers which have a code_index_file and virtual_indent enabled.
// result = layout_virt_indent_literal(app, arena, buffer, range, face, width, kind);
// result = layout_basic(app, arena, buffer, range, face, width, kind);
//

// @todo Needs to be a marker list stored on the buffer.
//       So that when the buffer is edited, the positions are updated.
// @todo But what happens to a marker, which is inside a range that gets deleted.
//
// @todo But we would want to also store a reference to those markers on a view scope,
//       so that we could make it possible to have something hidden on one view but not on another.
//       @note This is not possible right now, because I think the layout set by buffer_set_layout
//             is only called on buffer_edits.
//             So this happens just on a per buffer basis and is the same output for all views.
//
static Range_i64 temp_range_array[2] = {
    Ii64(19, 177),
    Ii64(309, 362)
};
global Range_i64_Array global_code_folding_ranges = {
    &temp_range_array[0],
    2
};

function Layout_Item_List
origami_layout_index__inner(Application_Links *app, Arena *arena, Buffer_ID buffer, Range_i64 range, Face_ID face, f32 width, Code_Index_File *file, Layout_Wrap_Kind kind){
    Scratch_Block scratch(app);
    
    Token_Array tokens = get_token_array_from_buffer(app, buffer);
    Token_Array *tokens_ptr = &tokens;
    
    Layout_Item_List list = get_empty_item_list(range);
    String_Const_u8 text = push_buffer_range(app, scratch, buffer, range);
    
    Face_Advance_Map advance_map = get_face_advance_map(app, face);
    Face_Metrics metrics = get_face_metrics(app, face);
    LefRig_TopBot_Layout_Vars pos_vars = get_lr_tb_layout_vars(&advance_map, &metrics, width);
    
    f32 regular_indent = metrics.space_advance*global_config.virtual_whitespace_regular_indent;
    f32 wrap_align_x = width - metrics.normal_advance;
    
    Layout_Reflex reflex = get_layout_reflex(&list, buffer, width, face);
    
    // :code_folding
    Range_i64 folded_range = {};
    b32 found_folded_range = false;
    for (u64 i = 0; i < global_code_folding_ranges.count; i++) {
        Range_i64 *it = global_code_folding_ranges.ranges + i;
        if (range.start >= it->start && range.end <= it->end) {
            found_folded_range = true;
            folded_range = *it;
        }
    }
    if (found_folded_range) {
        if (range.start == folded_range.start) {
            u8 *ptr = text.str;
            i64 index = layout_index_from_ptr(ptr, text.str, range.first);
            lr_tb_write_with_advance(&pos_vars, face, 0.0f, arena, &list, index, ' ');
#if 0
            lr_tb_write_ghost(&pos_vars, face, arena, &list, index, '\\');
#else
            // @todo Look if this could be done with one call.
            lr_tb_write_with_advance(&pos_vars, face, metrics.normal_advance, arena, &list, index, '.');
            lr_tb_write_with_advance(&pos_vars, face, metrics.normal_advance, arena, &list, index, '.');
            lr_tb_write_with_advance(&pos_vars, face, metrics.normal_advance, arena, &list, index, '.');
#endif
            
            //lr_tb_next_line(&pos_vars);
            pos_vars.line_to_text_shift = 0;
            pos_vars.width = 0;
        }
        else {
            u8 *ptr = text.str;
            //i64 index = layout_index_from_ptr(ptr, text.str, range.first);
            //lr_tb_write_with_advance(&pos_vars, face, 0.0f, arena, &list, index, ' ');
            //lr_tb_next_line(&pos_vars);
            //lr_tb_write_blank_dim(&pos_vars, face, {0, 0}, arena, &list, index);
            
            pos_vars.line_to_text_shift = 0;
            pos_vars.width = 0;
            //layout_item_list_finish(&list, 0);
            //return list;
        }
    }
    else if (text.size == 0){
        lr_tb_write_blank(&pos_vars, face, arena, &list, range.start);
    }
    else{
        b32 first_of_the_line = true;
        Newline_Layout_Vars newline_vars = get_newline_layout_vars();
        
        u8 *ptr = text.str;
        u8 *end_ptr = ptr + text.size;
        u8 *word_ptr = ptr;
        
        u8 *pending_wrap_ptr = ptr;
        f32 pending_wrap_x = 0.f;
        i32 pending_wrap_paren_nest_count = 0;
        i32 pending_wrap_token_score = 0;
        f32 pending_wrap_accumulated_w = 0.f;
        
        start:
        if (ptr == end_ptr){
            i64 index = layout_index_from_ptr(ptr, text.str, range.first);
            f32 shift = layout_index_x_shift(app, &reflex, file, index, regular_indent);
            lr_tb_advance_x_without_item(&pos_vars, shift);
            goto finish;
        }
        
        if (!character_is_whitespace(*ptr)){
            i64 index = layout_index_from_ptr(ptr, text.str, range.first);
            f32 shift = layout_index_x_shift(app, &reflex, file, index, regular_indent);
            lr_tb_advance_x_without_item(&pos_vars, shift);
            goto consuming_non_whitespace;
        }
        
        {
            for (;ptr < end_ptr; ptr += 1){
                if (!character_is_whitespace(*ptr)){
                    pending_wrap_ptr = ptr;
                    word_ptr = ptr;
                    i64 index = layout_index_from_ptr(ptr, text.str, range.first);
                    f32 shift = layout_index_x_shift(app, &reflex, file, index, regular_indent);
                    lr_tb_advance_x_without_item(&pos_vars, shift);
                    goto consuming_non_whitespace;
                }
                if (*ptr == '\r'){
                    i64 index = layout_index_from_ptr(ptr, text.str, range.first);
                    newline_layout_consume_CR(&newline_vars, index);
                }
                else if (*ptr == '\n'){
                    pending_wrap_ptr = ptr;
                    i64 index = layout_index_from_ptr(ptr, text.str, range.first);
                    f32 shift = layout_index_x_shift(app, &reflex, file, index, regular_indent);
                    lr_tb_advance_x_without_item(&pos_vars, shift);
                    goto consuming_normal_whitespace;
                }
            }
            
            if (ptr == end_ptr){
                pending_wrap_ptr = ptr;
                i64 index = layout_index_from_ptr(ptr - 1, text.str, range.first);
                f32 shift = layout_index_x_shift(app, &reflex, file, index, regular_indent);
                lr_tb_advance_x_without_item(&pos_vars, shift);
                goto finish;
            }
        }
        
        consuming_non_whitespace:
        {
            for (;ptr <= end_ptr; ptr += 1){
                if (ptr == end_ptr || character_is_whitespace(*ptr)){
                    break;
                }
            }
            
            // NOTE(allen): measure this word
            newline_layout_consume_default(&newline_vars);
            String_Const_u8 word = SCu8(word_ptr, ptr);
            u8 *word_end = ptr;
            {
                f32 word_advance = 0.f;
                ptr = word.str;
                for (;ptr < word_end;){
                    Character_Consume_Result consume = utf8_consume(ptr, (u64)(word_end - ptr));
                    if (consume.codepoint != max_u32){
                        word_advance += lr_tb_advance(&pos_vars, face, consume.codepoint);
                    }
                    else{
                        word_advance += lr_tb_advance_byte(&pos_vars);
                    }
                    ptr += consume.inc;
                }
                pending_wrap_accumulated_w += word_advance;
            }
            
            if (!first_of_the_line && (kind == Layout_Wrapped) && lr_tb_crosses_width(&pos_vars, pending_wrap_accumulated_w)){
                i64 index = layout_index_from_ptr(pending_wrap_ptr, text.str, range.first);
                lr_tb_align_rightward(&pos_vars, wrap_align_x);
                lr_tb_write_ghost(&pos_vars, face, arena, &list, index, '\\');
                
                lr_tb_next_line(&pos_vars);
#if 0
                f32 shift = layout_index_x_shift(app, &reflex, file, index, regular_indent);
                lr_tb_advance_x_without_item(&pos_vars, shift);
#endif
                
                ptr = pending_wrap_ptr;
                pending_wrap_accumulated_w = 0.f;
                first_of_the_line = true;
                goto start;
            }
        }
        
        consuming_normal_whitespace:
        for (; ptr < end_ptr; ptr += 1){
            if (!character_is_whitespace(*ptr)){
                u8 *new_wrap_ptr = ptr;
                
                i64 index = layout_index_from_ptr(new_wrap_ptr, text.str, range.first);
                Code_Index_Nest *new_wrap_nest = code_index_get_nest(file, index);
                b32 invalid_wrap_x = false;
                f32 new_wrap_x = layout_index_x_shift(app, &reflex, new_wrap_nest, index, regular_indent, &invalid_wrap_x);
                if (invalid_wrap_x){
                    new_wrap_x = max_f32;
                }
                
                i32 new_wrap_paren_nest_count = 0;
                for (Code_Index_Nest *nest = new_wrap_nest;
                     nest != 0;
                     nest = nest->parent){
                    if (nest->kind == CodeIndexNest_Paren){
                        new_wrap_paren_nest_count += 1;
                    }
                }
                
                Token_Pair new_wrap_token_pair = layout_token_pair(tokens_ptr, index);
                
                // TODO(allen): pull out the token scoring part and make it replacable for other
                // language's token based wrap scoring needs.
                i32 token_score = 0;
                if (new_wrap_token_pair.a.kind == TokenBaseKind_Keyword){
                    if (new_wrap_token_pair.b.kind == TokenBaseKind_ParentheticalOpen ||
                        new_wrap_token_pair.b.kind == TokenBaseKind_Keyword){
                        token_score -= 2;
                    }
                }
                token_score += layout_token_score_wrap_token(&new_wrap_token_pair, TokenCppKind_Eq);
                token_score += layout_token_score_wrap_token(&new_wrap_token_pair, TokenCppKind_PlusEq);
                token_score += layout_token_score_wrap_token(&new_wrap_token_pair, TokenCppKind_MinusEq);
                token_score += layout_token_score_wrap_token(&new_wrap_token_pair, TokenCppKind_StarEq);
                token_score += layout_token_score_wrap_token(&new_wrap_token_pair, TokenCppKind_DivEq);
                token_score += layout_token_score_wrap_token(&new_wrap_token_pair, TokenCppKind_ModEq);
                token_score += layout_token_score_wrap_token(&new_wrap_token_pair, TokenCppKind_LeftLeftEq);
                token_score += layout_token_score_wrap_token(&new_wrap_token_pair, TokenCppKind_RightRightEq);
                token_score += layout_token_score_wrap_token(&new_wrap_token_pair, TokenCppKind_Comma);
                token_score += layout_token_score_wrap_token(&new_wrap_token_pair, TokenCppKind_AndAnd);
                token_score += layout_token_score_wrap_token(&new_wrap_token_pair, TokenCppKind_OrOr);
                token_score += layout_token_score_wrap_token(&new_wrap_token_pair, TokenCppKind_Ternary);
                token_score += layout_token_score_wrap_token(&new_wrap_token_pair, TokenCppKind_Colon);
                token_score += layout_token_score_wrap_token(&new_wrap_token_pair, TokenCppKind_Semicolon);
                
                i32 new_wrap_token_score = token_score;
                
                b32 new_wrap_ptr_is_better = false;
                if (first_of_the_line){
                    new_wrap_ptr_is_better = true;
                }
                else{
                    if (new_wrap_token_score > pending_wrap_token_score){
                        new_wrap_ptr_is_better = true;
                    }
                    else if (new_wrap_token_score == pending_wrap_token_score){
                        f32 new_score = new_wrap_paren_nest_count*10.f + new_wrap_x;
                        f32 old_score = pending_wrap_paren_nest_count*10.f + pending_wrap_x + metrics.normal_advance*4.f + pending_wrap_accumulated_w*0.5f;
                        
                        if (new_score < old_score){
                            new_wrap_ptr_is_better = true;
                        }
                    }
                }
                
                if (new_wrap_ptr_is_better){
                    layout_index__emit_chunk(&pos_vars, face, arena, text.str, range.first, pending_wrap_ptr, new_wrap_ptr, &list);
                    first_of_the_line = false;
                    
                    pending_wrap_ptr = new_wrap_ptr;
                    pending_wrap_paren_nest_count = new_wrap_paren_nest_count;
                    pending_wrap_x = layout_index_x_shift(app, &reflex, new_wrap_nest, index, regular_indent);
                    pending_wrap_paren_nest_count = new_wrap_paren_nest_count;
                    pending_wrap_token_score = new_wrap_token_score;
                    pending_wrap_accumulated_w = 0.f;
                }
                
                word_ptr = ptr;
                goto consuming_non_whitespace;
            }
            
            i64 index = layout_index_from_ptr(ptr, text.str, range.first);
            switch (*ptr){
                default:
                {
                    newline_layout_consume_default(&newline_vars);
                    pending_wrap_accumulated_w += lr_tb_advance(&pos_vars, face, *ptr);
                }break;
                
                case '\r':
                {
                    newline_layout_consume_CR(&newline_vars, index);
                }break;
                
                case '\n':
                {
                    layout_index__emit_chunk(&pos_vars, face, arena, text.str, range.first, pending_wrap_ptr, ptr, &list);
                    pending_wrap_ptr = ptr + 1;
                    pending_wrap_accumulated_w = 0.f;
                    
                    u64 newline_index = newline_layout_consume_LF(&newline_vars, index);
                    lr_tb_write_blank(&pos_vars, face, arena, &list, newline_index);
                    lr_tb_next_line(&pos_vars);
                    first_of_the_line = true;
                    ptr += 1;
                    goto start;
                }break;
            }
        }
        
        finish:
        if (newline_layout_consume_finish(&newline_vars)){
            layout_index__emit_chunk(&pos_vars, face, arena, text.str, range.first, pending_wrap_ptr, ptr, &list);
            i64 index = layout_index_from_ptr(ptr, text.str, range.first);
            lr_tb_write_blank(&pos_vars, face, arena, &list, index);
        }
    }
    
    layout_item_list_finish(&list, -pos_vars.line_to_text_shift);
    
    return(list);
}

function Layout_Item_List
origami_layout_virt_indent_index(Application_Links *app, Arena *arena, Buffer_ID buffer, Range_i64 range, Face_ID face, f32 width, Layout_Wrap_Kind kind){
    Layout_Item_List result = {};
    
    if (global_config.enable_virtual_whitespace){
        code_index_lock();
        Code_Index_File *file = code_index_get_file(buffer);
        if (file != 0){
            result = origami_layout_index__inner(app, arena, buffer, range, face, width, file, kind);
        }
        code_index_unlock();
        if (file == 0) {
            result = layout_virt_indent_literal(app, arena, buffer, range, face, width, kind);
        }
    }
    else{
        result = layout_basic(app, arena, buffer, range, face, width, kind);
    }
    
    return(result);
}

function Layout_Item_List
origami_layout_virt_indent_index_unwrapped(Application_Links *app, Arena *arena, Buffer_ID buffer, Range_i64 range, Face_ID face, f32 width){
    return(origami_layout_virt_indent_index(app, arena, buffer, range, face, width, Layout_Unwrapped));
}

function Layout_Item_List
origami_layout_virt_indent_index_wrapped(Application_Links *app, Arena *arena, Buffer_ID buffer, Range_i64 range, Face_ID face, f32 width){
    return(origami_layout_virt_indent_index(app, arena, buffer, range, face, width, Layout_Wrapped));
}

function Layout_Item_List
origami_layout_virt_indent_index_generic(Application_Links *app, Arena *arena, Buffer_ID buffer, Range_i64 range, Face_ID face, f32 width){
    Managed_Scope scope = buffer_get_managed_scope(app, buffer);
    b32 *wrap_lines_ptr = scope_attachment(app, scope, buffer_wrap_lines, b32);
    b32 wrap_lines = (wrap_lines_ptr != 0 && *wrap_lines_ptr);
    return(origami_layout_virt_indent_index(app, arena, buffer, range, face, width, wrap_lines?Layout_Wrapped:Layout_Unwrapped));
}

