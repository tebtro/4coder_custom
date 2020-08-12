#ifndef TEBTRO_H


//
// @note Zen / Focus mode
//
global b32 global_focus_mode_enabled = false;


//
// @note Theme mode :tebtro_theme_mode
//
enum Tebtro_Theme_Mode {
    TEBTRO_THEME_MODE_default,
    TEBTRO_THEME_MODE_darker,
    TEBTRO_THEME_MODE_light,
};
global char *global_tebtro_theme_mode_names[3] = {
    "theme-tebtro",
    "theme-tebtro-dark",
    "theme-tebtro-light"
};
global Tebtro_Theme_Mode global_tebtro_theme_mode = TEBTRO_THEME_MODE_default;


//
// @note: Fonts / Faces
//
global Face_ID global_underlined_face_id = 0;
global Face_ID global_strikethrough_face_id = 0;
global Face_ID global_bold_face_id = 0;
global Face_ID global_italic_face_id = 0;


#define TEBTRO_H
#endif // TEBTRO_H
