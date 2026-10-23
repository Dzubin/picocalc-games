/*
 * klondike.h - the rules of Klondike Solitaire. Plain ISO C, no screen and no
 * keys.
 *
 * A card is one byte: rank * 4 + suit, with rank 0 (ace) to 12 (king) and the
 * suit numbers of cards.h (0 spades, 1 hearts, 2 diamonds, 3 clubs). A spot is
 * a place on the table: 0 the stock, 1 the waste, 2 to 5 the foundations (one
 * per suit) and 6 to 12 the columns.
 *
 * Author: Anthropic Claude
 */
#ifndef KLONDIKE_H
#define KLONDIKE_H

#include <stdbool.h>
#include <stdint.h>

#include "klondike_const.h"

typedef struct {
    uint8_t type;               /* KL_MOVE, KL_DRAW or KL_RECYCLE             */
    uint8_t from, to, count;
    uint8_t flipped;            /* a card was turned face up by the move      */
    uint8_t passes;             /* recycles done before the move              */
    int16_t score;              /* the score before the move                  */
} kl_move_t;

typedef struct {
    uint8_t   stock[KL_STOCK_MAX];      /* the top of the pile is the last    */
    uint8_t   waste[KL_STOCK_MAX];
    int       stock_n, waste_n;
    uint8_t   found[KL_FOUNDATIONS];    /* cards on each pile, 0 to 13        */
    uint8_t   column[KL_COLUMNS][KL_MAX_COLUMN];
    uint8_t   length[KL_COLUMNS];
    uint8_t   hidden[KL_COLUMNS];       /* cards face down, at the top        */
    int       draw;                     /* 1 or 3 cards at a time             */
    int       passes;                   /* times the waste went back          */
    int       score;
    int       moves;
    int       last_flipped;             /* the last move turned a card face up */
    long      deal;
    kl_move_t undo[KL_UNDO_MAX];
    int       undo_count;
} kl_t;

#define KL_RANK(c)  ((c) >> 2)
#define KL_SUIT(c)  ((c) & 3)

/* Shuffle and deal game number deal (1 to 32000), drawing 1 or 3 cards. */
void kl_deal(kl_t *g, long deal, int draw);

/* the card on top of a spot (for a column, the last one; for the stock, the
 * next one to be drawn), KL_NO_CARD if the spot is empty */
uint8_t kl_top(const kl_t *g, int spot);

/* cards on a spot; for a column, all of them (see kl_faceup) */
int kl_size(const kl_t *g, int spot);

/* cards of a column that are face up (the run that can be picked up) */
int kl_faceup(const kl_t *g, int column_spot);

/* Draw cards from the stock to the waste. With the stock empty, turn the
 * waste over into the stock again. Returns the cards drawn, minus the cards
 * turned over for a recycle, 0 when there is nothing to do. */
int kl_draw(kl_t *g);

/* Move cards from the waste, a foundation or a column to a foundation or a
 * column. A count of 0 means: find out how many (the cards of a column run
 * that fit where they are going). Returns the number of cards moved, 0 when
 * the move is not allowed. Cards moving to a foundation always go to the pile
 * of their own suit, whichever foundation spot was named. A column left with
 * only face-down cards has its last one turned up. */
int kl_move(kl_t *g, int from, int to, int count);

/* True when every card can be sent home without more thought: the stock and
 * the waste are empty and no card in a column is face down. */
bool kl_can_finish(const kl_t *g);

/* One step of finishing the game: the lowest card that is at the foot of a
 * column goes to its foundation. Returns false when there is nothing to move.
 * from and to say what moved, so that the screen can show it. */
bool kl_finish_step(kl_t *g, int *from, int *to);

/* take back the last move */
bool kl_undo(kl_t *g);

bool kl_won(const kl_t *g);

#endif /* KLONDIKE_H */
