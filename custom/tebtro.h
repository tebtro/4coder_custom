#ifndef TEBTRO_H


//
// @note Zen / Focus mode
//
global b32 global_focus_mode_enabled = false;


//
// @note: Fonts / Faces
//
global Face_ID global_underlined_face_id = 0;
global Face_ID global_strikethrough_face_id = 0;
global Face_ID global_bold_face_id = 0;
global Face_ID global_italic_face_id = 0;


//
// @note Identifier list
//

struct Code_Index_Identifier_Node {
    Code_Index_Identifier_Node *next;
    
    u64 hash;
    Code_Index_Note_Kind note_kind;
};

struct Code_Index_Identifier_Hash_Table {
    Code_Index_Identifier_Node **table;
    i32 count;
};

CUSTOM_ID(attachment, attachment_code_index_identifier_table);


#define TEBTRO_H
#endif // TEBTRO_H
