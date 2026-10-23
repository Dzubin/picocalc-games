/*
 * freecell.h - the rules of FreeCell. Plain ISO C, no screen and no keys.
 *
 * A card is one byte: rank * 4 + suit, with rank 0 (ace) to 12 (king) and the
 * suit numbers of cards.h (0 spades, 1 hearts, 2 diamonds, 3 clubs). A spot is
 * a place on the table: 0 to 3 the free cells, 4 to 7 the foundations (the
 * spade, heart, diamond and club piles) and 8 to 15 the columns.
 *
 * Author: Anthropic Claude
 */
#ifndef FREECELL_H
#define FREECELL_H

#include <stdbool.h>
#include <stdint.h>

#include "freecell_const.h"

typedef struct {
    uint8_t from, to, count;
    uint8_t chain;              /* 1: made by the game after the move before */
} fc_move_t;

typedef struct {
    uint8_t   column[FC_COLUMNS][FC_MAX_COLUMN];
    uint8_t   length[FC_COLUMNS];
    uint8_t   cell[FC_CELLS];           /* FC_NO_CARD when empty              */
    uint8_t   found[FC_FOUNDATIONS];    /* cards on each pile, 0 to 13        */
    fc_move_t undo[FC_UNDO_MAX];
    int       undo_count;
    int       moves;                    /* moves made by the player           */
    long      deal;
} fc_t;

#define FC_RANK(c)  ((c) >> 2)
#define FC_SUIT(c)  ((c) & 3)

/* Shuffle and deal game number deal (the classic numbers, 1 to 32000). */
void fc_deal(fc_t *g, long deal);

/* the card at the bottom of a column or in a cell, FC_NO_CARD if none */
uint8_t fc_top(const fc_t *g, int spot);

/* how many cards a spot holds (a cell 0 or 1, a foundation 0 to 13) */
int fc_size(const fc_t *g, int spot);

/* the cards at the bottom of a column that form a run (alternating colours,
 * each one lower than the one before): the most that could be picked up */
int fc_run_length(const fc_t *g, int column_spot);

/* how many cards can be moved at once to a column that is not empty (or to an
 * empty one, with to_empty true) given the free cells and empty columns */
int fc_capacity(const fc_t *g, bool to_empty);

/* Move count cards from a cell or column spot to another spot. A count of 0
 * means: find out how many (for a column that is not empty, the cards that
 * fit on it; otherwise one). Returns the number of cards moved, 0 when the
 * move is not allowed. Cards moving to a foundation always go to the pile of
 * their own suit, whichever foundation spot was named. */
int fc_move(fc_t *g, int from, int to, int count);

/* Make one automatic move: the first card that is safe goes to its foundation
 * (see fc_auto). Returns false when there is none; otherwise from, to and
 * count say what moved, so that the screen can show it. */
bool fc_auto_step(fc_t *g, int *from, int *to, int *count);

/* Move every card that is safe to the foundations (a card nothing of the
 * other colour still needs). Returns how many moved. */
int fc_auto(fc_t *g);

/* take back the last move, with the automatic moves that followed it */
bool fc_undo(fc_t *g);

bool fc_won(const fc_t *g);

#endif /* FREECELL_H */
