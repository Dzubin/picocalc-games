/*
 * chooser.h - the screen where the player chooses how the faces of the
 * playing cards look. The choice is kept in settings.h and used by the card
 * games.
 *
 * Author: Anthropic Claude
 */
#ifndef CHOOSER_H
#define CHOOSER_H

/* Show the three face layouts and let the player choose one: UP and DOWN move
 * a marker, ENTER chooses, ESC goes back. The screen is left covered, so the
 * caller draws its own screen again. The arrow keys repeat again afterwards
 * (the card games have them repeating). */
void card_chooser(void);

#endif /* CHOOSER_H */
