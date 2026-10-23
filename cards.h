/*
 * cards.h - small playing cards drawn into a pixel buffer.
 *
 * Plain ISO C with no hardware calls: a card is drawn into an array of RGB565
 * pixels and the caller puts that array on whatever screen it has. The
 * corners outside the rounded shape get the table colour.
 *
 * Author: Anthropic Claude
 */
#ifndef CARDS_H
#define CARDS_H

#include <stdint.h>

/* the largest card, in pixels (a buffer this big holds any card) */
#define CARDS_MAX_PIXELS (38 * 53)

/* number of card sizes, face styles and back designs */
int cards_size_count(void);
int cards_style_count(void);
int cards_back_count(void);

/* the pixel size of a card of the given size number */
int cards_width(int size);
int cards_height(int size);

/* The buffer must hold cards_width(size) * cards_height(size) pixels, row by
 * row. rank is 0 (ace) to 12 (king); suit is 0 spades, 1 hearts, 2 diamonds,
 * 3 clubs. */
void cards_draw_face(uint16_t *buf, int size, int style, int rank, int suit);
void cards_draw_back(uint16_t *buf, int size, int design);

/* A whole card drawn with a slim index (rank and suit side by side in the top
 * left corner) for a card that is covered by the next one: the caller puts
 * only its top rows on the screen. */
void cards_draw_strip(uint16_t *buf, int size, int rank, int suit);

/* For a card (drawn by any of the functions above) that lies on top of
 * another one: its rounded corners show that card, not the table. */
void cards_lay_on_card(uint16_t *buf, int size);

/* An empty spot: the outline of a card in the table colour; suit 0 to 3 draws
 * that suit in the middle (a foundation), -1 leaves it plain (a free cell). */
void cards_draw_slot(uint16_t *buf, int size, int suit);

#endif /* CARDS_H */
