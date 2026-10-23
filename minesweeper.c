/*
 * minesweeper.c - the rules of Minesweeper (see minesweeper.h).
 *
 * Author: Anthropic Claude
 */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "minesweeper.h"

/* forward declarations (the public functions come first) */
static int  neighbours(const ms_t *g, int index, int *out);
static void flood(ms_t *g, int index);
static int  open_one(ms_t *g, int index);
static void finish_win(ms_t *g);
static void finish_loss(ms_t *g, int index);

/* ===================================================================== */
/*  Making a board                                                        */
/* ===================================================================== */
void ms_new(ms_t *g, int w, int h, int mines, unsigned long seed)
{
    static uint16_t order[MS_MAX_CELLS];
    unsigned long s = seed;
    int n, i, left;

    if (w < 1) w = 1;
    if (h < 1) h = 1;
    if (w > MS_MAX_W) w = MS_MAX_W;
    if (h > MS_MAX_H) h = MS_MAX_H;
    n = w * h;
    if (mines < 1) mines = 1;
    if (mines > n - 1) mines = n - 1;

    memset(g, 0, sizeof *g);
    g->w = w;
    g->h = h;
    g->mines = mines;
    g->hit = -1;
    g->seed = seed;

    /* pick the squares of the mines by shuffling the numbers of all squares */
    for (i = 0; i < n; i++)
        order[i] = (uint16_t)i;
    left = n;
    for (i = 0; i < mines; i++) {
        int j;

        s = (s * MS_RAND_MUL + MS_RAND_ADD) & 0xFFFFFFFFUL;
        j = (int)(((s >> MS_RAND_SHIFT) & MS_RAND_MASK) % (unsigned long)left);
        g->cell[order[j]] |= MS_MINE;
        order[j] = order[--left];
    }
    for (i = 0; i < n; i++) {
        int around[8], count = neighbours(g, i, around), k, m = 0;

        for (k = 0; k < count; k++)
            if (g->cell[around[k]] & MS_MINE)
                m++;
        g->near[i] = (uint8_t)m;
    }
}

/* the squares next to a square (up to 8); returns how many */
static int neighbours(const ms_t *g, int index, int *out)
{
    int x = index % g->w, y = index / g->w, dx, dy, n = 0;

    for (dy = -1; dy <= 1; dy++) {
        for (dx = -1; dx <= 1; dx++) {
            int nx = x + dx, ny = y + dy;

            if ((dx || dy) && nx >= 0 && nx < g->w && ny >= 0 && ny < g->h)
                out[n++] = ny * g->w + nx;
        }
    }
    return n;
}

void ms_solve(ms_t *g)
{
    int i;

    if (g->state != MS_PLAYING)
        return;
    for (i = 0; i < g->w * g->h; i++) {
        if (!(g->cell[i] & MS_MINE) && !(g->cell[i] & MS_OPEN)) {
            g->cell[i] &= (uint8_t)~(MS_FLAG | MS_QUESTION);
            g->cell[i] |= MS_OPEN;
            g->opened++;
        }
    }
    g->started = true;
    finish_win(g);
}

int ms_mines_left(const ms_t *g)
{
    return g->mines - g->flags;
}

/* ===================================================================== */
/*  Opening squares                                                       */
/* ===================================================================== */

/* the board is won when every square without a mine is open: the mines left
 * are then flagged */
static void finish_win(ms_t *g)
{
    int i;

    g->state = MS_WON;
    g->flags = 0;
    for (i = 0; i < g->w * g->h; i++) {
        if (g->cell[i] & MS_MINE) {
            g->cell[i] = (uint8_t)((g->cell[i] & ~MS_QUESTION) | MS_FLAG);
            g->flags++;
        }
    }
}

static void finish_loss(ms_t *g, int index)
{
    g->state = MS_LOST;
    g->hit = index;
}

/* open one square that is covered, without flood fill; returns the result */
static int open_one(ms_t *g, int index)
{
    g->cell[index] &= (uint8_t)~(MS_FLAG | MS_QUESTION);
    g->cell[index] |= MS_OPEN;
    if (g->cell[index] & MS_MINE) {
        finish_loss(g, index);
        return MS_RESULT_LOST;
    }
    g->opened++;
    return MS_RESULT_OK;
}

/* open a square with no mines round it and everything it uncovers */
static void flood(ms_t *g, int start)
{
    static uint16_t stack[MS_MAX_CELLS];
    int top = 0;

    stack[top++] = (uint16_t)start;
    while (top > 0) {
        int index = stack[--top], around[8], count, k;

        if (g->near[index] != 0)
            continue;
        count = neighbours(g, index, around);
        for (k = 0; k < count; k++) {
            int c = around[k];

            if (g->cell[c] & (MS_OPEN | MS_FLAG | MS_MINE))
                continue;
            open_one(g, c);
            if (g->near[c] == 0)
                stack[top++] = (uint16_t)c;
        }
    }
}

int ms_open(ms_t *g, int x, int y)
{
    int index, result = MS_RESULT_NONE;

    if (g->state != MS_PLAYING || x < 0 || x >= g->w || y < 0 || y >= g->h)
        return MS_RESULT_NONE;
    index = y * g->w + x;

    if (g->cell[index] & MS_OPEN) {                 /* a number: open round it */
        int around[8], count, k, flags = 0;

        if (g->near[index] == 0)
            return MS_RESULT_NONE;
        count = neighbours(g, index, around);
        for (k = 0; k < count; k++)
            if (g->cell[around[k]] & MS_FLAG)
                flags++;
        if (flags != g->near[index])
            return MS_RESULT_NONE;
        for (k = 0; k < count; k++) {
            int c = around[k];

            if (g->cell[c] & (MS_OPEN | MS_FLAG))
                continue;
            if (open_one(g, c) == MS_RESULT_LOST)
                result = MS_RESULT_LOST;
            else if (g->near[c] == 0)
                flood(g, c);
        }
        if (result == MS_RESULT_NONE)
            result = MS_RESULT_OK;
    } else if (g->cell[index] & MS_FLAG) {
        return MS_RESULT_NONE;                      /* a flag protects the square */
    } else {
        g->started = true;
        result = open_one(g, index);
        if (result == MS_RESULT_OK && g->near[index] == 0)
            flood(g, index);
    }

    if (g->state == MS_LOST)
        return MS_RESULT_LOST;
    if (g->opened == g->w * g->h - g->mines) {
        finish_win(g);
        return MS_RESULT_WON;
    }
    return result;
}

bool ms_mark(ms_t *g, int x, int y)
{
    int index;
    uint8_t *c;

    if (g->state != MS_PLAYING || x < 0 || x >= g->w || y < 0 || y >= g->h)
        return false;
    index = y * g->w + x;
    c = &g->cell[index];
    if (*c & MS_OPEN)
        return false;
    if (*c & MS_FLAG) {                 /* flag -> question mark */
        *c = (uint8_t)((*c & ~MS_FLAG) | MS_QUESTION);
        g->flags--;
    } else if (*c & MS_QUESTION) {      /* question mark -> nothing */
        *c &= (uint8_t)~MS_QUESTION;
    } else {                            /* nothing -> flag */
        *c |= MS_FLAG;
        g->flags++;
    }
    return true;
}
