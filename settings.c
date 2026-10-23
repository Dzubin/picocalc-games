/*
 * settings.c - the choices the games share (see settings.h).
 *
 * Author: Anthropic Claude
 */
#include "settings.h"
#include "games_const.h"

static int face_style = SETTING_FACE_STYLE_DEFAULT;

int settings_face_style(void)
{
    return face_style;
}

void settings_set_face_style(int style)
{
    face_style = style;
}
