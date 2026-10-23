/*
 * klondike_const.h - the constants of the Klondike rules: the spots of the
 * table, the scoring and the deal. Nothing here depends on the machine. The
 * picture, the keys and the texts are in klondike_play_const.h.
 *
 * Author: Anthropic Claude
 */
#ifndef KLONDIKE_CONST_H
#define KLONDIKE_CONST_H

/* ------------------------------------------------------------------ */
/*  The table: stock, waste, 4 foundations and 7 columns                 */
/* ------------------------------------------------------------------ */
#define KL_FOUNDATIONS  4
#define KL_COLUMNS      7
#define KL_SPOT_STOCK   0
#define KL_SPOT_WASTE   1
#define KL_SPOT_FOUND0  2       /* one pile per suit, in the order of cards.h */
#define KL_SPOT_COL0    6
#define KL_SPOTS       (KL_SPOT_COL0 + KL_COLUMNS)

#define KL_MAX_COLUMN  20       /* 6 turned down and a king down to an ace */
#define KL_STOCK_MAX   24       /* the cards left after the columns        */
#define KL_NO_CARD     0xFF
#define KL_UNDO_MAX    2048     /* moves that can be taken back            */
#define KL_UNDO_DROP    512     /* oldest moves forgotten when that is full */

/* the kinds of move in the undo list */
#define KL_MOVE         0
#define KL_DRAW         1
#define KL_RECYCLE      2

/* ------------------------------------------------------------------ */
/*  Scoring, as in the Windows game (standard scoring, no clock)         */
/* ------------------------------------------------------------------ */
#define KL_SCORE_TO_COLUMN        5     /* waste to a column               */
#define KL_SCORE_TO_FOUNDATION   10     /* from the waste or a column      */
#define KL_SCORE_TURN_OVER        5     /* a card turned face up           */
#define KL_SCORE_FROM_FOUNDATION (-15)  /* a card taken back to a column   */
#define KL_SCORE_RECYCLE_ONE    (-100)  /* waste back to stock, draw 1     */
#define KL_SCORE_RECYCLE_THREE   (-20)  /* the same, draw 3, after 2 free  */
#define KL_FREE_RECYCLES_THREE     2

/* ------------------------------------------------------------------ */
/*  Deals: numbers 1 to 32000, shuffled with the generator of the        */
/*  classic Windows games (the same number is always the same deal)      */
/* ------------------------------------------------------------------ */
#define KL_RAND_MUL   214013UL
#define KL_RAND_ADD  2531011UL
#define KL_RAND_SHIFT     16
#define KL_RAND_MASK  0x7FFFUL

#endif /* KLONDIKE_CONST_H */
