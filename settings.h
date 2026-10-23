/*
 * settings.h - the choices the player makes that all the games share. They
 * last until the PicoCalc is switched off.
 *
 * Author: Anthropic Claude
 */
#ifndef SETTINGS_H
#define SETTINGS_H

/* The layout of the card faces in the games: 0 a corner index and one big
 * suit, 1 a big corner index only, 2 an index in two corners and a small suit
 * (the face styles of cards.h). Chosen on the card samples screen. */
int  settings_face_style(void);
void settings_set_face_style(int style);

#endif /* SETTINGS_H */
