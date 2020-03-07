#if 0
Head:     master STB TrueType test Changes made in cmder.bat ...
Merge:    origin/master STB TrueType test Changes made in cmder.bat ...

Unstaged changes (4)
modified   misc/cmder.bat
modified   src/quickcpp.cpp

Recent commits
49852d6 master origin/master STB TrueType test Changes made in cmder.bat ...
6a17807 Add asm building to quickcpp Not standalone asm to executable yet.
c355472 Win32 OpenGL example
ced9b3c new repository, initial commit
#endif

#if 0
// porcelain=v2
# branch.oid 49852d6bb62609c44fddc7545e95859c2a4f47cf
# branch.head master
# branch.upstream origin/master
# branch.ab +0 -0
1 .M N... 100644 100644 100644 b169e52eb942551b466ecf3cd7fa5f35156664b2 b169e52eb942551b466ecf3cd7fa5f35156664b2 misc/cmder.bat
1 .M N... 100644 100644 100644 3ad78134b6873842aceb5a1310b168e9753a7bea 3ad78134b6873842aceb5a1310b168e9753a7bea misc/quick.rdbg
1 .M N... 100644 100644 100644 5688b3bc7ba2694eff79fe042b4ff8b4b83a2ffd 5688b3bc7ba2694eff79fe042b4ff8b4b83a2ffd src/build.bat
1 .M N... 100644 100644 100644 96c4bb0ed8849e14ac3851e5f2cf13b85f7a8eeb 96c4bb0ed8849e14ac3851e5f2cf13b85f7a8eeb src/quickcpp.cpp
exited with code 0
#endif

#if 0
// porcelain
## master...origin/master
M misc/cmder.bat
M misc/quick.rdbg
M src/build.bat
M src/quickcpp.cpp
exited with code 0
#endif


global String_Const_u8 global_magit_buffer_name = SCu8("*git*");
global String_Const_u8 global_magit_dump_buffer_name = SCu8("*dump*");

global Arena global_magit_arena = {};

CUSTOM_COMMAND_SIG(magit_status) {
    ProfileScope(app, "magit_status");
    
    Scratch_Block scratch(app);
    if (global_magit_arena.base_allocator == 0) {
        global_magit_arena = make_arena_system();
    }
    Arena *arena = &global_magit_arena;
    
    View_ID view_id = get_active_view(app, Access_Always);
    Buffer_ID magit_buffer_id = get_buffer_by_name(app, global_magit_buffer_name, Access_Always);
    if (!magit_buffer_id) {
        magit_buffer_id = create_buffer(app, global_magit_buffer_name, BufferCreate_AlwaysNew);
        
        // BufferSetting_ReadOnly
        // BufferSetting_Unimportant
        // BufferSetting_Unkillable
        buffer_set_setting(app, magit_buffer_id, BufferSetting_Unimportant, true);
    }
    Assert(magit_buffer_id);
    
    // Buffer_Identifier id = buffer_identifier(magit_buffer_id);
    Buffer_ID dump_buffer_id = get_buffer_by_name(app, global_magit_dump_buffer_name, Access_Always);
    if (!dump_buffer_id) {
        dump_buffer_id = create_buffer(app, global_magit_dump_buffer_name, BufferCreate_AlwaysNew);
        buffer_set_setting(app, dump_buffer_id, BufferSetting_Unimportant, true);
    }
    else {
        clear_buffer(app, dump_buffer_id);
    }
    Assert(dump_buffer_id);
    Buffer_Identifier id = buffer_identifier(global_magit_dump_buffer_name);
    defer {
        if (dump_buffer_id) {
            // @crash
            // buffer_kill(app, dump_buffer_id, BufferKill_AlwaysKill);
        }
    };
    
    String_Const_u8 hot_directory = push_hot_directory(app, scratch);
    exec_system_command(app, view_id, id, hot_directory, SCu8("git status --porcelain --branch"), CLI_OverlapWithConflict | CLI_CursorAtEnd | CLI_SendEndSignal);
    
    String_Const_u8 src = push_whole_buffer(app, arena, dump_buffer_id);
    
    i64 buffer_size = buffer_get_size(app, magit_buffer_id);
    buffer_replace_range(app, magit_buffer_id, Ii64(0, buffer_size-1), src);
}
