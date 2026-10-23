/*
 * test_klondike.c - checks of the Klondike rules (klondike.c) on a PC.
 *
 * Author: Anthropic Claude
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "klondike.h"

static int checks, failures;

#define CHECK(cond) do { checks++; if (!(cond)) { failures++; \
    printf("FAIL line %d: %s\n", __LINE__, #cond); } } while (0)

static kl_t g, start;

/* every card exactly once, wherever it lies */
static bool all_cards_once(const kl_t *t)
{
    int seen[52] = { 0 }, i, j, total = 0;

    for (i = 0; i < KL_COLUMNS; i++)
        for (j = 0; j < t->length[i]; j++) {
            seen[t->column[i][j]]++;
            total++;
        }
    for (i = 0; i < t->stock_n; i++) {
        seen[t->stock[i]]++;
        total++;
    }
    for (i = 0; i < t->waste_n; i++) {
        seen[t->waste[i]]++;
        total++;
    }
    for (i = 0; i < KL_FOUNDATIONS; i++)
        for (j = 0; j < t->found[i]; j++) {
            seen[j * 4 + i]++;
            total++;
        }
    for (i = 0; i < 52; i++)
        if (seen[i] != 1)
            return false;
    return total == 52;
}

static bool same_table(const kl_t *a, const kl_t *b)
{
    int i;

    for (i = 0; i < KL_COLUMNS; i++)
        if (a->length[i] != b->length[i] || a->hidden[i] != b->hidden[i] ||
            memcmp(a->column[i], b->column[i], a->length[i]) != 0)
            return false;
    return a->stock_n == b->stock_n && a->waste_n == b->waste_n &&
           memcmp(a->stock, b->stock, (size_t)a->stock_n) == 0 &&
           memcmp(a->waste, b->waste, (size_t)a->waste_n) == 0 &&
           memcmp(a->found, b->found, sizeof a->found) == 0 &&
           a->score == b->score && a->passes == b->passes;
}

static void test_deal(void)
{
    long d;
    int i;

    kl_deal(&g, 1, 1);
    start = g;
    kl_deal(&g, 1, 1);
    CHECK(same_table(&g, &start));              /* the same number, the same deal */
    kl_deal(&g, 2, 1);
    CHECK(!same_table(&g, &start));
    for (d = 1; d <= 300; d++) {
        kl_deal(&g, d, 3);
        CHECK(all_cards_once(&g));
        CHECK(g.stock_n == 24 && g.waste_n == 0);
        for (i = 0; i < KL_COLUMNS; i++)
            CHECK(g.length[i] == i + 1 && g.hidden[i] == i);
    }
}

/* an empty table to set up by hand */
static void clear_table(void)
{
    kl_deal(&g, 1, 1);
    memset(g.length, 0, sizeof g.length);
    memset(g.hidden, 0, sizeof g.hidden);
    memset(g.found, 0, sizeof g.found);
    g.stock_n = g.waste_n = 0;
    g.score = 0;
    g.undo_count = 0;
}

static void test_draw(void)
{
    int i;
    uint8_t first;

    kl_deal(&g, 5, 3);
    CHECK(kl_draw(&g) == 3 && g.stock_n == 21 && g.waste_n == 3);
    CHECK(kl_undo(&g) && g.stock_n == 24 && g.waste_n == 0);

    kl_deal(&g, 5, 1);
    for (i = 0; i < 24; i++)
        CHECK(kl_draw(&g) == 1);
    CHECK(g.stock_n == 0 && g.waste_n == 24);
    CHECK(kl_draw(&g) == -24 && g.stock_n == 24 && g.waste_n == 0);
    CHECK(g.passes == 1 && g.score == 0);               /* the score stays at 0 */
    g.score = 150;
    for (i = 0; i < 24; i++)
        kl_draw(&g);
    CHECK(kl_draw(&g) == -24 && g.score == 50);         /* draw 1: -100 a recycle */
    CHECK(kl_undo(&g) && g.score == 150 && g.passes == 1 && g.waste_n == 24);

    /* the first card drawn after a recycle is the first one drawn before */
    kl_deal(&g, 9, 1);
    first = kl_top(&g, KL_SPOT_STOCK);
    for (i = 0; i < 24; i++)
        kl_draw(&g);
    kl_draw(&g);
    CHECK(kl_top(&g, KL_SPOT_STOCK) == first);

    /* draw 3: the first two recycles are free, then -20 */
    kl_deal(&g, 9, 3);
    g.score = 100;
    for (i = 0; i < 3; i++) {
        while (g.stock_n)
            kl_draw(&g);
        kl_draw(&g);
    }
    CHECK(g.passes == 3 && g.score == 80);
}

static void test_moves(void)
{
    /* column 0: a hidden 7 of clubs and the 9 of spades up; column 1: 8 of hearts */
    clear_table();
    g.column[0][0] = 6 * 4 + 3;
    g.column[0][1] = 8 * 4 + 0;
    g.length[0] = 2;
    g.hidden[0] = 1;
    g.column[1][0] = 7 * 4 + 1;
    g.length[1] = 1;
    CHECK(kl_faceup(&g, KL_SPOT_COL0) == 1);
    CHECK(kl_move(&g, KL_SPOT_COL0 + 1, KL_SPOT_COL0 + 0, 0) == 1);   /* red 8 on black 9 */
    CHECK(g.length[0] == 3 && g.length[1] == 0 && kl_faceup(&g, KL_SPOT_COL0) == 2);
    CHECK(g.score == 0);
    /* a run goes to an empty column only if it starts with a king */
    CHECK(kl_move(&g, KL_SPOT_COL0, KL_SPOT_COL0 + 1, 0) == 0);
    CHECK(kl_undo(&g) && g.length[1] == 1 && g.length[0] == 2);

    /* turning a card over scores 5, a card to the foundation 10 */
    clear_table();
    g.column[2][0] = 5 * 4 + 1;
    g.column[2][1] = 0 * 4 + 2;                                       /* 6H down, AD up */
    g.length[2] = 2;
    g.hidden[2] = 1;
    CHECK(kl_move(&g, KL_SPOT_COL0 + 2, KL_SPOT_FOUND0, 0) == 1);     /* any pile: its own suit's */
    CHECK(g.found[2] == 1 && g.hidden[2] == 0 && g.score == 15);
    CHECK(kl_undo(&g) && g.hidden[2] == 1 && g.score == 0 && g.found[2] == 0);

    /* waste to the foundation 10, waste to a column 5, back from the foundation -15 */
    clear_table();
    g.waste[0] = 0 * 4 + 0;
    g.waste_n = 1;                                                    /* AS */
    CHECK(kl_move(&g, KL_SPOT_WASTE, KL_SPOT_FOUND0 + 3, 0) == 1 && g.score == 10);
    g.waste[0] = 12 * 4 + 2;
    g.waste_n = 1;                                                    /* KD */
    CHECK(kl_move(&g, KL_SPOT_WASTE, KL_SPOT_COL0 + 4, 0) == 1 && g.score == 15);
    g.column[5][0] = 11 * 4 + 0;
    g.length[5] = 1;                                                  /* QS */
    g.found[1] = 1;                                                   /* AH on its pile */
    CHECK(kl_move(&g, KL_SPOT_FOUND0 + 1, KL_SPOT_COL0 + 6, 0) == 0); /* an ace needs no empty column */
    /* a card taken back from a foundation goes on a card one higher of the other colour */
    g.column[3][0] = 1 * 4 + 1;
    g.length[3] = 1;
    g.found[0] = 1;                                                   /* AS on its pile */
    CHECK(kl_move(&g, KL_SPOT_FOUND0 + 0, KL_SPOT_COL0 + 3, 0) == 1); /* AS on the red 2 */
    CHECK(g.score == 0);                                               /* 15 - 15 */
}

static void test_playouts(void)
{
    long d;

    srand(11);
    for (d = 1; d <= 400; d++) {
        int step, i;

        kl_deal(&g, d, (d & 1) ? 1 : 3);
        start = g;
        for (step = 0; step < 600; step++) {
            int r = rand() % 6;

            if (r == 0)
                kl_draw(&g);
            else
                kl_move(&g, rand() % KL_SPOTS, rand() % KL_SPOTS, rand() % 3);
            if (step % 50 == 0)
                CHECK(all_cards_once(&g));
        }
        CHECK(all_cards_once(&g));
        for (i = 0; i < 100000 && kl_undo(&g); i++)
            ;
        CHECK(same_table(&g, &start));
        CHECK(g.moves == 0);
    }
}

static void test_finish(void)
{
    static const int order[4] = { 0, 1, 3, 2 };   /* spades, hearts, clubs, diamonds */
    int col, j, from, to, steps = 0;

    clear_table();
    /* four runs of K down to A, the colours alternating, every card once */
    for (col = 0; col < 4; col++)
        for (j = 0; j < 13; j++)
            g.column[col][g.length[col]++] = (uint8_t)((12 - j) * 4 + order[(j + col) % 4]);
    CHECK(all_cards_once(&g));
    CHECK(kl_can_finish(&g));
    while (kl_finish_step(&g, &from, &to))
        steps++;
    CHECK(steps == 52 && kl_won(&g) && !kl_can_finish(&g));
    /* a card still face down, or one in the stock, stops it */
    clear_table();
    kl_deal(&g, 3, 1);
    CHECK(!kl_can_finish(&g));
}

int main(void)
{
    test_deal();
    test_draw();
    test_moves();
    test_playouts();
    test_finish();
    printf("%d checks, %d failed\n", checks, failures);
    return failures != 0;
}
