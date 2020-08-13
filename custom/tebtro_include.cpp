#if !defined(TEBTRO_INCLUDE_CPP)


#define VIM_USE_DEFAULT_RUN_LISTER 0
#define VIM_USE_DEFAULT_ISEARCH 0
#define VIM_USE_DEFAULT_QUERY_REPLACE_BASE 0


#if VIM_WINDOWS_AUTO_DISABLE_CAPSLOCK
#pragma comment(lib, "user32")
#pragma warning(push)
#pragma warning(disable: 4042)
#define WIN32_LEAN_AND_MEAN
#include "windows.h"
#pragma warning(pop)
#endif


#define FCODER_TRANSITION_TO 4001004
#if !defined(FCODER_TRANSITION_TO)
#define FCODER_TRANSITION_TO 0
#endif


#include "4coder_base_types.h"
#include "4coder_version.h"
#include "4coder_table.h"
#include "4coder_events.h"
#include "4coder_types.h"
#include "4coder_doc_content_types.h"
// #include "4coder_default_colors.h"
#include "vim_default_colors.h"
#define DYNAMIC_LINK_API
#include "generated/custom_api.h"
#include "4coder_system_types.h"
#define DYNAMIC_LINK_API
#include "generated/system_api.h"
#include "generated/command_metadata.h"

#include "4coder_profile.h"
#include "4coder_async_tasks.h"
#include "4coder_token.h"
#include "4coder_string_match.h"
#include "4coder_helper.h"
#include "4coder_delta_rule.h"
#include "4coder_layout_rule.h"
#include "4coder_code_index.h"
#include "4coder_draw.h"
#include "4coder_insertion.h"
#include "4coder_command_map.h"
#include "4coder_lister_base.h"
#include "4coder_clipboard.h"
#include "4coder_default_framework.h"
#include "4coder_config.h"
#include "4coder_auto_indent.h"
#include "4coder_search.h"
#include "4coder_build_commands.h"
#include "4coder_jumping.h"
#include "4coder_jump_sticky.h"
#include "4coder_jump_lister.h"
#include "4coder_project_commands.h"
#include "4coder_function_list.h"
#include "4coder_scope_commands.h"
#include "4coder_combined_write_commands.h"
#include "4coder_log_parser.h"
#include "4coder_profile_inspect.h"
#include "4coder_tutorial.h"

////////////////////////////////

#include "4coder_base_types.cpp"
#include "4coder_stringf.cpp"
#include "4coder_app_links_allocator.cpp"
#include "4coder_system_allocator.cpp"
#include "generated/lexer_cpp.h"

#define DYNAMIC_LINK_API
#include "generated/custom_api.cpp"
#define DYNAMIC_LINK_API
#include "generated/system_api.cpp"
#include "4coder_system_helpers.cpp"
#include "4coder_layout.cpp"
#include "4coder_profile.cpp"
#include "4coder_profile_static_enable.cpp"
#include "4coder_events.cpp"
#include "4coder_custom.cpp"
#include "4coder_log.cpp"
#include "4coder_hash_functions.cpp"
#include "4coder_table.cpp"
#include "4coder_codepoint_map.cpp"
#include "4coder_async_tasks.cpp"
#include "4coder_string_match.cpp"
#include "4coder_buffer_seek_constructors.cpp"
#include "4coder_token.cpp"
#include "4coder_command_map.cpp"

#include "generated/lexer_cpp.cpp"

#include "4coder_default_map.cpp"
#include "4coder_mac_map.cpp"

#include "4coder_default_framework_variables.cpp"
#include "4coder_default_colors.cpp"
#include "4coder_helper.cpp"
#include "4coder_delta_rule.cpp"
#include "4coder_layout_rule.cpp"
#include "4coder_code_index.cpp"
#include "4coder_fancy.cpp"
#include "4coder_draw.cpp"
#include "4coder_font_helper.cpp"
#include "4coder_config.cpp"
#include "4coder_default_framework.cpp"
#include "4coder_clipboard.cpp"
#include "4coder_lister_base.cpp"
#include "4coder_base_commands.cpp"
#include "4coder_insertion.cpp"
#include "4coder_eol.cpp"
#include "4coder_lists.cpp"
#include "4coder_auto_indent.cpp"
#include "4coder_search.cpp"
#include "4coder_jumping.cpp"
#include "4coder_jump_sticky.cpp"
#include "4coder_jump_lister.cpp"
#include "4coder_code_index_listers.cpp"
#include "4coder_log_parser.cpp"
#include "4coder_keyboard_macro.cpp"
#include "4coder_cli_command.cpp"
#include "4coder_build_commands.cpp"
#include "4coder_project_commands.cpp"
#include "4coder_function_list.cpp"
#include "4coder_scope_commands.cpp"
#include "4coder_combined_write_commands.cpp"
#include "4coder_miblo_numbers.cpp"
#include "4coder_profile_inspect.cpp"
#include "4coder_tutorial.cpp"
#include "4coder_doc_content_types.cpp"
#include "4coder_doc_commands.cpp"
#include "4coder_docs.cpp"

#include "4coder_examples.cpp"

#include "4coder_default_hooks.cpp"


//
// MODULES
//

// vertical_scope_annotation
#include "vertical_scope_annotation.cpp"

// scope_highlight
#include "scope_highlight.cpp"

// primitive_highlight
function FColor tebtro_get_token_color_cpp(Application_Links *app, Buffer_ID buffer_id, Token token);
#include "primitive_highlight.cpp"


//
// Fleury
//
// Source: https://github.com/ryanfleury/4coder_fleury
//
#include <cstdlib>  // rand
#include "fleury_ubiquitous.cpp"
#include "fleury_plot.cpp"
#include "fleury_calc.cpp"

// quick_calc
#define CALC_PLOT 1
#include "quick_calc.cpp"

// Code Peek
#define CODE_PEEK 0
#if CODE_PEEK
#include "vim_code_peek.cpp"
#endif

//
// My MODULES
//

// relative_line_number_mode
#include "tebtro_relative_line_number_mode.cpp"

//
// vim
//
#include "vim.h"

//
// general
//
#include "tebtro_general.cpp"

//
// magit
//
#include "magit.cpp"

//
// avy
//
// "aoeuhtns"
#define AVY_KEY_LIST  "aoeuidhtns"
#define AVY_VIEW_SELECTION_CENTER_KEY  true
#define AVY_VIEW_SELECTION_BIGGER_KEY  true
#include "avy.cpp"

//
// code folding
//
#include "code_folding.cpp"

//
// vim
//
#include "vim_function_helper.cpp"
#include "vim_window_movement.cpp"
#include "vim.cpp"
#include "vim_draw.cpp"
#include "vim_hooks.cpp"
#include "vim_map.cpp"

//
// tebtro
//
#include "tebtro.cpp"
#include "tebtro_colors.cpp"
#include "tebtro_draw.cpp"
#include "tebtro_hooks.cpp"
#include "tebtro_file_fuzzy_search.cpp"
#include "tebtro_jump_to_definition.cpp"
#include "tebtro_map.cpp"

//
// space mode
//
#include "space_mode.cpp"
#include "space_mode_map.cpp"


// @note Must be the last file to include.
#include "generated/managed_id_metadata.cpp"


#define TEBTRO_INCLUDE_CPP
#endif // TEBTRO_INCLUDE_CPP
