/*
 * victory.h - what happens when the player wins: a fanfare, and either cards
 * falling and bouncing across the screen (the card games) or the board
 * flashing (Minesweeper). Any key skips the show.
 *
 * Author: Anthropic Claude
 */
#ifndef VICTORY_H
#define VICTORY_H

/* All 52 cards fall from the four foundations, kings first, and bounce across
 * the screen leaving a trail. start_x gives the left edge of each foundation
 * (in the order of the suits in cards.h) and start_y their top edge. The
 * screen is left covered with the trails: the caller draws it again. */
void victory_cascade(int size, int face_style, const int *start_x, int start_y);

/* Flash the rectangle between two colours. The caller draws it again. */
void victory_flash(int x, int y, int w, int h);

#endif /* VICTORY_H */
