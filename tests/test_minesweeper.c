/*
 * test_minesweeper.c - checks of the Minesweeper rules (minesweeper.c) on a
 * PC.
 *
 * Author: Anthropic Claude
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "minesweeper.h"

static int checks, failures;

#define CHECK(cond) do { checks++; if (!(cond)) { failures++; \
    printf("FAIL line %d: %s\n", __LINE__, #cond); } } while (0)

static ms_t g;

static int count_flag(const ms_t *t, int flag)
{
    int i, n = 0;

    for (i = 0; i < t->w * t->h; i++)
        if (t->cell[i] & flag)
            n++;
    return n;
}

/* the mines round a square, counted the slow way */
static int slow_near(const ms_t *t, int x, int y)
{
    int dx, dy, n = 0;

    for (dy = -1; dy <= 1; dy++)
        for (dx = -1; dx <= 1; dx++) {
            int nx = x + dx, ny = y + dy;

            if ((dx || dy) && nx >= 0 && nx < t->w && ny >= 0 && ny < t->h &&
                (t->cell[ny * t->w + nx] & MS_MINE))
                n++;
        }
    return n;
}

static void test_layout(void)
{
    ms_t other;
    int i, x, y;

    ms_new(&g, 9, 9, 10, 1);
    CHECK(count_flag(&g, MS_MINE) == 10);
    for (y = 0; y < 9; y++)
        for (x = 0; x < 9; x++)
            CHECK(g.near[y * 9 + x] == slow_near(&g, x, y));
    ms_new(&other, 9, 9, 10, 1);
    CHECK(memcmp(g.cell, other.cell, 81) == 0);            /* the same seed, the same board */
    ms_new(&other, 9, 9, 10, 2);
    CHECK(memcmp(g.cell, other.cell, 81) != 0);
    for (i = 1; i <= 200; i++) {
        ms_new(&g, 30, 16, 99, (unsigned long)i * 7919UL);
        CHECK(count_flag(&g, MS_MINE) == 99);
        ms_new(&g, 16, 16, 40, (unsigned long)i);
        CHECK(count_flag(&g, MS_MINE) == 40);
    }
    /* limits */
    ms_new(&g, 99, 99, 5, 3);
    CHECK(g.w == MS_MAX_W && g.h == MS_MAX_H);
    ms_new(&g, 8, 8, 1000, 3);
    CHECK(g.mines == 63);
    ms_new(&g, 8, 8, 0, 3);
    CHECK(g.mines == 1);
}

/* find a covered square that is not a mine and has no mine round it */
static int find_zero(const ms_t *t)
{
    int i;

    for (i = 0; i < t->w * t->h; i++)
        if (!(t->cell[i] & MS_MINE) && t->near[i] == 0)
            return i;
    return -1;
}

static void test_open(void)
{
    int z, i, opened_ok = 1;

    ms_new(&g, 16, 16, 40, 5);
    z = find_zero(&g);
    CHECK(z >= 0);
    CHECK(ms_open(&g, z % 16, z / 16) == MS_RESULT_OK);
    CHECK(g.opened > 1);
    for (i = 0; i < 256; i++) {
        if ((g.cell[i] & MS_OPEN) && (g.cell[i] & MS_MINE))
            opened_ok = 0;
        /* a square next to an open zero is open too */
        if ((g.cell[i] & MS_OPEN) && g.near[i] == 0) {
            int x = i % 16, y = i / 16, dx, dy;

            for (dy = -1; dy <= 1; dy++)
                for (dx = -1; dx <= 1; dx++) {
                    int nx = x + dx, ny = y + dy;

                    if (nx >= 0 && nx < 16 && ny >= 0 && ny < 16 &&
                        !(g.cell[ny * 16 + nx] & MS_OPEN))
                        opened_ok = 0;
                }
        }
    }
    CHECK(opened_ok);
    CHECK(g.opened == count_flag(&g, MS_OPEN));

    /* a flag protects a square */
    ms_new(&g, 9, 9, 10, 8);
    CHECK(ms_mark(&g, 0, 0) && (g.cell[0] & MS_FLAG));
    CHECK(ms_open(&g, 0, 0) == MS_RESULT_NONE && !(g.cell[0] & MS_OPEN));
}

static void test_lose_and_win(void)
{
    int i, last;

    /* a mine ends the game */
    ms_new(&g, 9, 9, 10, 11);
    for (i = 0; i < 81 && !(g.cell[i] & MS_MINE); i++)
        ;
    CHECK(ms_open(&g, i % 9, i / 9) == MS_RESULT_LOST);
    CHECK(g.state == MS_LOST && g.hit == i);
    CHECK(ms_open(&g, 0, 0) == MS_RESULT_NONE);                 /* nothing after the end */
    CHECK(!ms_mark(&g, 0, 0));

    /* opening every square without a mine wins, and the mines get flags */
    ms_new(&g, 9, 9, 10, 12);
    last = -1;
    for (i = 0; i < 81; i++)
        if (!(g.cell[i] & MS_MINE))
            last = i;
    for (i = 0; i < 81; i++) {
        int r;

        if (g.cell[i] & (MS_MINE | MS_OPEN))
            continue;
        r = ms_open(&g, i % 9, i / 9);
        if (i == last || g.state == MS_WON)
            CHECK(r == MS_RESULT_WON || g.state == MS_WON);
        else
            CHECK(r == MS_RESULT_OK || r == MS_RESULT_WON);
    }
    CHECK(g.state == MS_WON);
    CHECK(g.flags == 10 && count_flag(&g, MS_FLAG) == 10 && ms_mines_left(&g) == 0);
}

static void test_solve(void)
{
    ms_new(&g, 16, 16, 40, 30);
    ms_open(&g, 0, 0);
    ms_mark(&g, 5, 5);
    ms_solve(&g);
    CHECK(g.state == MS_WON);
    CHECK(g.opened == 256 - 40);
    CHECK(g.flags == 40 && ms_mines_left(&g) == 0);
}

static void test_marks_and_chord(void)
{
    int i, m = -1, x, y, dx, dy;

    ms_new(&g, 9, 9, 10, 20);
    CHECK(ms_mines_left(&g) == 10);
    CHECK(ms_mark(&g, 3, 3) && g.flags == 1 && ms_mines_left(&g) == 9);
    CHECK(ms_mark(&g, 3, 3) && g.flags == 0 && (g.cell[3 * 9 + 3] & MS_QUESTION));
    CHECK(ms_mark(&g, 3, 3) && !(g.cell[3 * 9 + 3] & (MS_FLAG | MS_QUESTION)));
    CHECK(!ms_mark(&g, -1, 0));

    /* chord: a number with the right flags opens the squares round it */
    ms_new(&g, 16, 16, 40, 21);
    for (i = 0; i < 256 && m < 0; i++) {
        if (!(g.cell[i] & MS_MINE) && g.near[i] == 1)
            m = i;
    }
    CHECK(m >= 0);
    x = m % 16;
    y = m / 16;
    CHECK(ms_open(&g, x, y) == MS_RESULT_OK);                   /* the number is open */
    CHECK(ms_open(&g, x, y) == MS_RESULT_NONE);                 /* no flag yet: nothing */
    for (dy = -1; dy <= 1; dy++)
        for (dx = -1; dx <= 1; dx++) {
            int nx = x + dx, ny = y + dy;

            if ((dx || dy) && nx >= 0 && nx < 16 && ny >= 0 && ny < 16 &&
                (g.cell[ny * 16 + nx] & MS_MINE))
                ms_mark(&g, nx, ny);
        }
    CHECK(ms_open(&g, x, y) != MS_RESULT_LOST);                 /* the flag is on the mine */
    CHECK(g.state != MS_LOST);

    /* a wrong flag makes the chord open a mine */
    ms_new(&g, 16, 16, 40, 21);
    ms_open(&g, x, y);
    for (dy = -1; dy <= 1; dy++)
        for (dx = -1; dx <= 1; dx++) {
            int nx = x + dx, ny = y + dy;

            if ((dx || dy) && nx >= 0 && nx < 16 && ny >= 0 && ny < 16 &&
                !(g.cell[ny * 16 + nx] & (MS_MINE | MS_OPEN)) && !(g.flags))
                ms_mark(&g, nx, ny);                            /* one wrong flag */
        }
    CHECK(ms_open(&g, x, y) == MS_RESULT_LOST);
}

static void test_playouts(void)
{
    int round;

    srand(3);
    for (round = 0; round < 300; round++) {
        int step, i;

        ms_new(&g, 8 + rand() % 23, 8 + rand() % 17, 5 + rand() % 60, (unsigned long)round);
        for (step = 0; step < 400; step++) {
            if (rand() % 3 == 0)
                ms_mark(&g, rand() % g.w, rand() % g.h);
            else
                ms_open(&g, rand() % g.w, rand() % g.h);
        }
        {
            int open_safe = 0;

            for (i = 0; i < g.w * g.h; i++)
                if ((g.cell[i] & MS_OPEN) && !(g.cell[i] & MS_MINE))
                    open_safe++;
            CHECK(g.opened == open_safe);
            CHECK(g.flags == count_flag(&g, MS_FLAG));
            CHECK(g.state != MS_WON || g.opened == g.w * g.h - g.mines);
        }
    }
}

int main(void)
{
    test_layout();
    test_open();
    test_lose_and_win();
    test_marks_and_chord();
    test_solve();
    test_playouts();
    printf("%d checks, %d failed\n", checks, failures);
    return failures != 0;
}
