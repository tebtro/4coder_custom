#ifndef TEBTRO_H


// 
// @note Identifier list
// 

// @todo Use a Hash_Table

struct Identifier_Node {
    Identifier_Node *next;
    
    Code_Index_Note_Kind note_kind;
    String_Const_u8 text;    
};

struct Identifier_List {
    Identifier_Node *first;
    Identifier_Node *last;
    i32 count;
};

global Arena global_identifier_arena = {};
global Identifier_List global_identifier_list = {};


#define TEBTRO_H
#endif // TEBTRO_H
