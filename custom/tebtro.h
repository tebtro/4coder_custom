#ifndef TEBTRO_H


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
