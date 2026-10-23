/*
 * cardfly.h - a card or a run of cards flying across the screen.
 *
 * The caller says where the cards start and end and gets called to draw the
 * table again under and around the cards at every frame; this module only
 * builds the picture of the cards and puts it on the screen.
 *
 * A card is one byte: rank * 4 + suit, as in freecell.h.
 *
 * Author: Anthropic Claude
 */
#ifndef CARDFLY_H
#define CARDFLY_H

#include <stdbool.h>
#include <stdint.h>

/* Draw again whatever lies in the rectangle (the cards have left it). */
typedef void (*cardfly_redraw_t)(int x, int y, int w, int h);

/* Fly count cards (the first is the topmost of the run, each one pitch pixels
 * below the one before, the last one whole) from (sx, sy) to (ex, ey), at the
 * speed the player chose. An automatic move (one the game made by itself)
 * waits a moment first and may fly in fewer frames. face_style is a face style
 * of cards.h for the last card. Does nothing when the animation is off. */
void cardfly(const uint8_t *cards, int count, int size, int face_style,
             int pitch, int sx, int sy, int ex, int ey, bool automatic,
             cardfly_redraw_t redraw);

/* true unless the player turned the animation off */
bool cardfly_enabled(void);

/* Step to the next speed (off, fast, normal, slow, off, ...) and return its
 * name. The speed is shared by all the games. */
const char *cardfly_next_speed(void);

#endif /* CARDFLY_H */
