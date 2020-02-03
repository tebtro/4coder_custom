#include "space_mode.h"


//
// @note space mode commands
//

CUSTOM_COMMAND_SIG(enter_space_mode) {
    vim_enter_chord(app, mapid_space_mode, 0);
    // push_to_chord_bar(app, lit("<space>"));
}


CUSTOM_COMMAND_SIG(enter_space_mode_chord_b) {
    vim_enter_chord(app, mapid_space_mode_chord_b, 0);
    // push_to_chord_bar(app, lit("b"));
}

CUSTOM_COMMAND_SIG(enter_space_mode_chord_f) {
    vim_enter_chord(app, mapid_space_mode_chord_f, 0);
    // push_to_chord_bar(app, lit("f"));
}

CUSTOM_COMMAND_SIG(enter_space_mode_chord_p) {
    vim_enter_chord(app, mapid_space_mode_chord_p, 0);
    // push_to_chord_bar(app, lit("p"));
}

CUSTOM_COMMAND_SIG(enter_space_mode_chord_q) {
    vim_enter_chord(app, mapid_space_mode_chord_q, 0);
    // push_to_chord_bar(app, lit("q"));
}

CUSTOM_COMMAND_SIG(enter_space_mode_chord_w){
    vim_enter_chord(app, mapid_space_mode_chord_w, 0);
    // push_to_chord_bar(app, lit("w"));
}


//
// @note vim commands
//
#define vim_nop  vim_chord_command<nop>

#define vim_project_command_lister  vim_chord_command<project_command_lister>

#define vim_switch_project  vim_chord_command<switch_project>

#define vim_load_project                  vim_chord_command<load_project>
#define vim_setup_new_project             vim_chord_command<setup_new_project>
#define vim_project_go_to_root_directory  vim_chord_command<project_go_to_root_directory>

#define vim_goto_next_jump   vim_window_command<goto_next_jump>
#define vim_goto_prev_jump   vim_window_command<goto_prev_jump>
#define vim_goto_first_jump  vim_window_command<goto_first_jump>


// @note file chords
#define vim_interactive_fuzzy_find           vim_chord_command<interactive_fuzzy_find>
#define vim_interactive_fuzzy_find_in_other  vim_chord_command<interactive_fuzzy_find_in_other>
#define vim_interactive_open_or_new__or__fuzzy_find  vim_chord_command<interactive_open_or_new__or__fuzzy_find>
#define vim_interactive_open_or_new__or__fuzzy_find__in_other  vim_chord_command<interactive_open_or_new__or__fuzzy_find__in_other>

#define vim_interactive_open_or_new_in_other  vim_chord_command<interactive_open_or_new_in_other>


#define vim_open_file_in_quotes  vim_chord_command<open_file_in_quotes>
#define vim_reopen  vim_chord_command<reopen>
#define vim_rename_file_query  vim_chord_command<rename_file_query>
#define vim_make_directory_query  vim_chord_command<make_directory_query>


//
// @note Space mode commands lister
//

struct Whichkey_Command {
    char *name;
    char *text;
    Custom_Command_Function *proc;
    
    Whichkey_Command *sub_commands;
    i32 sub_command_count;
};

// :buffer_commands
static Whichkey_Command whichkey_buffer_commands[2] = {
    {
        "switch buffer", "B",
        vim_interactive_switch_buffer, 0, 0
    },
    {
        "kill buffer", "D",
        vim_kill_current_buffer, 0, 0
    },
};
// :filecommands
static Whichkey_Command whichkey_file_commands[11] = {
    {
        "fuzzy find file", "F",
        vim_interactive_open_or_new__or__fuzzy_find, 0, 0
    },
    {
        "fuzzy find file in other view", "ALT + F",
        vim_interactive_open_or_new__or__fuzzy_find__in_other, 0, 0
    },
    {
        "open or create file", "O",
        vim_interactive_open_or_new, 0, 0
    },
    {
        "open or create file in other view", "ALT + O",
        vim_interactive_open_or_new_in_other, 0, 0
    },
    {
        "open corresponding file (cpp / h)", "C",
        vim_open_matching_file_cpp, 0, 0
    },
    {
        "open file in quotes", "*",
        vim_open_file_in_quotes, 0, 0
    },
    {
        "reopen current file without asking", "CTRL + R",
        vim_reopen, 0, 0
    },
    {
        "save file", "S",
        vim_save_buffer, 0, 0
    },
    {
        "save all dirty buffers", "SHIFT + S",
        vim_save_all_dirty_buffers, 0, 0
    },
    {
        "rename file query", "CTRL + N",
        vim_rename_file_query, 0, 0
    },
    {
        "make directory query", "CTRL + M",
        vim_make_directory_query, 0, 0
    },
};
// :project_commands
static Whichkey_Command whichkey_project_commands[9] = {
    {
        "list project commands", "SPACE",
        vim_project_command_lister, 0, 0
    },
    {
        "switch project", "S",
        vim_switch_project, 0, 0
    },
    {
        "load project", "L",
        vim_load_project, 0, 0
    },
    {
        "create project", "C",
        vim_setup_new_project, 0, 0
    },
    {
        "goto project home directory", "H",
        vim_project_go_to_root_directory, 0, 0
    },
    {
        "save and make", "M || ALT-M",
        vim_clean_save_all_dirty_buffers_and_build, 0, 0
    },
    {
        "goto next error", "N",
        vim_goto_next_jump, 0, 0
    },
    {
        "goto previous error", "P",
        vim_goto_prev_jump, 0, 0
    },
    {
        "goto first error", "0",
        vim_goto_first_jump, 0, 0
    },
};
// :q_commands
static Whichkey_Command whichkey_q_commands[1] = {
    {
        "quit 4coder", "Q",
        vim_exit_4coder, 0, 0
    },
};
// :window_commands
static Whichkey_Command whichkey_window_commands[10] = {
    {
        "delete window", "D",
        vim_close_view, 0, 0
    },
    {
        "cycle window focus", "W",
        vim_cycle_view_focus, 0, 0
    },
    {
        "rotate view buffers", "R",
        vim_rotate_view_buffers, 0, 0
    },
    {
        "split vertical window duplicate", "V",
        vim_open_view_duplicate_split_vertical, 0, 0
    },
    {
        "split horizontal window duplicate", "S",
        vim_open_view_duplicate_split_horizontal, 0, 0
    },
    {
        "split horizontal new window", "N",
        vim_open_view_split_horizontal, 0, 0
    },
    {
        "focus window left", "H",
        vim_focus_view_left, 0, 0
    },
    {
        "focus window down", "J",
        vim_focus_view_down, 0, 0
    },
    {
        "focus window up", "K",
        vim_focus_view_up, 0, 0
    },
    {
        "focus window right", "L",
        vim_focus_view_right, 0, 0
    },
};
static Whichkey_Command whichkey_commands[12] = {
    {
        "switch project", "S",
        vim_switch_project, 0, 0
    },
    {
        "toggle_fullscreen",
        "Toggle fullscreen mode on or off. The change(s) do not take effect until the next frame.",
        toggle_fullscreen, 0, 0
    },
    { " ", " ", vim_nop, 0, 0},
    {
        "list_all_functions_all_buffers_lister",
        "Creates a lister of locations that look like function definitions and declarations all buffers.",
        list_all_functions_all_buffers_lister, 0, 0
    },
    {
        "project_command_lister",
        "Open a list of all commands in the currently loaded project.",
        project_command_lister, 0, 0
    },
    {
        "command_lister",
        "Opens a list of all registered commands.",
        command_lister, 0, 0
    },
    
    { " ", " ", vim_nop, 0, 0},
    //
    // @note sub command listers
    //
    
    // :buffer_commands
    {
        "+buffer", "B", 0,
        whichkey_buffer_commands, ArrayCount(whichkey_buffer_commands)
    },
    
    // :filecommands
    {
        "+file", "F", 0,
        whichkey_file_commands, ArrayCount(whichkey_file_commands)
    },
    
    // :project_commands
    {
        "+project", "P", 0,
        whichkey_project_commands, ArrayCount(whichkey_project_commands)
    },
    
    // :q_commands
    {
        "+quit", "Q", 0,
        whichkey_q_commands, ArrayCount(whichkey_q_commands)
    },
    
    // :window_commands
    {
        "+window", "W", 0,
        whichkey_window_commands, ArrayCount(whichkey_window_commands)
    },
};

function Lister_Result
get_whichkey_from_user(Application_Links *app, Whichkey_Command *whichkeys, i32 whichkey_count, String_Const_u8 query) {
    Scratch_Block scratch(app, Scratch_Share);
    Lister *lister = begin_lister(app, scratch);
    lister_set_query(lister, query);
    lister->handlers = lister_get_default_handlers();
    
    Whichkey_Command *whichkey = whichkeys;
    for (i32 i = 0; i < whichkey_count; ++i, ++whichkey) {
        lister_add_item(lister, SCu8(whichkey->name), SCu8(whichkey->text), whichkey, 0);
    }
    
    Lister_Result lister_result = run_lister(app, lister);
    return lister_result;
}

CUSTOM_UI_COMMAND_SIG(whichkey_command_lister) {
    View_ID view_id = get_this_ctx_view(app, Access_ReadWrite);
    if (view_id == 0)  return;
    
    Whichkey_Command *commands_list = whichkey_commands;
    i32 command_count = ArrayCount(whichkey_commands);
    
    Custom_Command_Function *proc = 0;
    while (proc == 0) {
        Lister_Result lister_result = get_whichkey_from_user(app, commands_list, command_count,
                                                             SCu8("WhichKey: "));
        if (lister_result.canceled)  break;
        
        Whichkey_Command *whichkey = (Whichkey_Command *)lister_result.user_data;
        if (whichkey->sub_command_count > 0) {
            commands_list = whichkey->sub_commands;
            command_count = whichkey->sub_command_count;
        }
        else if (whichkey->proc != 0) {
            proc = whichkey->proc;
            break;
        }
    }
    
    if (proc != 0) {
        view_enqueue_command_function(app, view_id, proc);
    }
    
    vim_enter_mode_normal(app);
}
