/*
 * freecell_const.h - the constants of the FreeCell rules: the spots of the
 * table, the deal numbers and the limits. Nothing here depends on the machine.
 * The picture, the keys and the texts are in freecell_play_const.h.
 *
 * Author: Anthropic Claude
 */
#ifndef FREECELL_CONST_H
#define FREECELL_CONST_H

/* ------------------------------------------------------------------ */
/*  The table: 4 free cells, 4 foundations, 8 columns                    */
/* ------------------------------------------------------------------ */
#define FC_CELLS        4
#define FC_FOUNDATIONS  4
#define FC_COLUMNS      8
#define FC_SPOTS       (FC_CELLS + FC_FOUNDATIONS + FC_COLUMNS)

/* spot numbers: the cells first, then the foundations (one per suit, in the
 * suit order of cards.h), then the columns */
#define FC_SPOT_CELL0   0
#define FC_SPOT_FOUND0  4
#define FC_SPOT_COL0    8

#define FC_MAX_COLUMN  30       /* cards one column can ever hold          */
#define FC_NO_CARD     0xFF
#define FC_UNDO_MAX    4096     /* moves that can be taken back            */
#define FC_UNDO_DROP   1024     /* oldest moves forgotten when that is full */

/* ------------------------------------------------------------------ */
/*  Deal numbers: the classic 1 to 32000 shuffle of the Windows game     */
/* ------------------------------------------------------------------ */
#define FC_DEAL_MIN        1
#define FC_DEAL_MAX    32000
#define FC_DEAL_DIGITS     5
#define FC_RAND_MUL   214013UL
#define FC_RAND_ADD  2531011UL
#define FC_RAND_SHIFT     16
#define FC_RAND_MASK  0x7FFFUL
/* the classic game numbers the suits 0 clubs, 1 diamonds, 2 hearts, 3 spades;
 * cards.h numbers them 0 spades, 1 hearts, 2 diamonds, 3 clubs */
#define FC_SUIT_FLIP        3

#endif /* FREECELL_CONST_H */
