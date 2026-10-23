/*
 * minesweeper_const.h - the constants of the Minesweeper rules: the sizes of
 * the board and the classic levels. Nothing here depends on the machine. The
 * picture, the keys and the texts are in minesweeper_play_const.h.
 *
 * Author: Anthropic Claude
 */
#ifndef MINESWEEPER_CONST_H
#define MINESWEEPER_CONST_H

/* ------------------------------------------------------------------ */
/*  The board                                                           */
/* ------------------------------------------------------------------ */
#define MS_MIN_W       8        /* smallest custom board                   */
#define MS_MIN_H       8
#define MS_MAX_W      30        /* the widest board is the classic expert  */
#define MS_MAX_H      24
#define MS_MAX_CELLS  (MS_MAX_W * MS_MAX_H)

/* the state of a square: the mine is part of the layout, the rest is what the
 * player has done */
#define MS_MINE       0x01
#define MS_OPEN       0x02
#define MS_FLAG       0x04
#define MS_QUESTION   0x08

/* how the game stands */
#define MS_PLAYING    0         /* also before the first square is opened  */
#define MS_WON        1
#define MS_LOST       2

/* what opening a square did */
#define MS_RESULT_NONE   0      /* nothing: a flag, or an open square without a match */
#define MS_RESULT_OK     1
#define MS_RESULT_LOST   2
#define MS_RESULT_WON    3

/* ------------------------------------------------------------------ */
/*  The classic levels                                                  */
/* ------------------------------------------------------------------ */
typedef struct {
    const char *name;
    int w, h, mines;
} ms_level_t;

#define MS_LEVELS 3
static const ms_level_t MS_LEVEL[MS_LEVELS] = {
    { "BEGINNER",     9,  9, 10 },
    { "INTERMEDIATE", 16, 16, 40 },
    { "EXPERT",       30, 16, 99 },
};

/* ------------------------------------------------------------------ */
/*  Laying the mines: the generator of the classic Windows games         */
/* ------------------------------------------------------------------ */
#define MS_RAND_MUL   214013UL
#define MS_RAND_ADD  2531011UL
#define MS_RAND_SHIFT     16
#define MS_RAND_MASK  0x7FFFUL

#endif /* MINESWEEPER_CONST_H */
