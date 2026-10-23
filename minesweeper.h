/*
 * minesweeper.h - the rules of Minesweeper. Plain ISO C, no screen and no
 * keys.
 *
 * A square is at column x (0 is the left) and row y (0 is the top). The mines
 * are laid when the board is made; nothing is moved when the first square is
 * opened, so the first square can be a mine.
 *
 * Author: Anthropic Claude
 */
#ifndef MINESWEEPER_H
#define MINESWEEPER_H

#include <stdbool.h>
#include <stdint.h>

#include "minesweeper_const.h"

typedef struct {
    int      w, h, mines;
    uint8_t  cell[MS_MAX_CELLS];    /* MS_MINE, MS_OPEN, MS_FLAG, MS_QUESTION */
    uint8_t  near[MS_MAX_CELLS];    /* mines in the 8 squares round a square  */
    int      opened;                /* squares opened (never a mine)          */
    int      flags;                 /* flags on the board                     */
    int      state;                 /* MS_PLAYING, MS_WON or MS_LOST          */
    int      hit;                   /* the mine that was opened, or -1        */
    bool     started;               /* a square has been opened               */
    unsigned long seed;             /* the number the layout came from        */
} ms_t;

/* Make a board of w x h squares with the given number of mines, laid from
 * seed (the same seed gives the same layout). w, h and mines are limited to
 * what fits: at least 1 mine, and at least one square without one. */
void ms_new(ms_t *g, int w, int h, int mines, unsigned long seed);

/* Open a square. On an open square that shows a number, and has as many
 * flags round it as the number, it opens all the squares round it that are
 * not flagged. Returns an MS_RESULT_ code. */
int ms_open(ms_t *g, int x, int y);

/* Step a square through no mark, a flag and a question mark, and back to no
 * mark. Returns false when nothing could be done (an open square, or the game
 * is over). */
bool ms_mark(ms_t *g, int x, int y);

/* Solve the board at once: every square without a mine is opened, the mines
 * are flagged and the game is won. */
void ms_solve(ms_t *g);

/* mines minus flags: what the counter shows (it can go below 0) */
int ms_mines_left(const ms_t *g);

#endif /* MINESWEEPER_H */
