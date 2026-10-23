/*
 * test_freecell.c - checks of the FreeCell rules (freecell.c) on a PC.
 *
 * Author: Anthropic Claude
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "freecell.h"

static int checks, failures;

#define CHECK(cond) do { checks++; if (!(cond)) { failures++; \
    printf("FAIL line %d: %s\n", __LINE__, #cond); } } while (0)

static fc_t g, start;

/* every card exactly once, wherever it lies */
static bool all_cards_once(const fc_t *t)
{
    int seen[52] = { 0 }, i, j, total = 0;

    for (i = 0; i < FC_COLUMNS; i++)
        for (j = 0; j < t->length[i]; j++) {
            seen[t->column[i][j]]++;
            total++;
        }
    for (i = 0; i < FC_CELLS; i++)
        if (t->cell[i] != FC_NO_CARD) {
            seen[t->cell[i]]++;
            total++;
        }
    for (i = 0; i < FC_FOUNDATIONS; i++)
        for (j = 0; j < t->found[i]; j++) {
            seen[j * 4 + i]++;
            total++;
        }
    for (i = 0; i < 52; i++)
        if (seen[i] != 1)
            return false;
    return total == 52;
}

static bool same_table(const fc_t *a, const fc_t *b)
{
    int i;

    for (i = 0; i < FC_COLUMNS; i++)
        if (a->length[i] != b->length[i] ||
            memcmp(a->column[i], b->column[i], a->length[i]) != 0)
            return false;
    return memcmp(a->cell, b->cell, sizeof a->cell) == 0 &&
           memcmp(a->found, b->found, sizeof a->found) == 0;
}

static void test_deal(void)
{
    static const uint8_t row0[8] = { 42, 6, 33, 43, 18, 25, 27, 17 };  /* JD 2D 9H JC 5D 7H 7C 5H */
    static const uint8_t row1[8] = { 50, 51, 32, 16, 2, 47, 49, 9 };   /* KD KC 9S 5S AD QC KH 3H */
    long d;
    int i;

    fc_deal(&g, 1);
    for (i = 0; i < 8; i++) {
        CHECK(g.column[i][0] == row0[i]);
        CHECK(g.column[i][1] == row1[i]);
    }
    for (d = 1; d <= 500; d++) {
        fc_deal(&g, d);
        CHECK(all_cards_once(&g));
        CHECK(g.length[0] == 7 && g.length[3] == 7 && g.length[4] == 6 && g.length[7] == 6);
    }
    fc_deal(&g, 32000);
    CHECK(all_cards_once(&g));
}

static void test_rules(void)
{
    fc_deal(&g, 1);
    memset(g.length, 0, sizeof g.length);
    memset(g.found, 0, sizeof g.found);
    /* column 0: 9 of spades (8*4+0=32); column 1: 8 of hearts (7*4+1=29) */
    g.column[0][0] = 32; g.length[0] = 1;
    g.column[1][0] = 29; g.length[1] = 1;
    CHECK(fc_move(&g, FC_SPOT_COL0 + 1, FC_SPOT_COL0 + 0, 0) == 1);   /* red 8 on black 9 */
    CHECK(g.length[0] == 2 && g.length[1] == 0);
    CHECK(fc_run_length(&g, FC_SPOT_COL0 + 0) == 2);
    /* a run of two moves to an empty column when allowed */
    CHECK(fc_capacity(&g, true) == 5 << 6);            /* 4 cells free, 6 other empty columns */
    CHECK(fc_capacity(&g, false) == 5 << 7);
    CHECK(fc_move(&g, FC_SPOT_COL0 + 0, FC_SPOT_COL0 + 1, 2) == 2);
    CHECK(g.length[1] == 2 && g.length[0] == 0);
    CHECK(fc_undo(&g));
    CHECK(g.length[0] == 2 && g.length[1] == 0);
    /* same colour or wrong rank is refused */
    g.column[2][0] = 21; g.length[2] = 1;              /* 6 of hearts (5*4+1) */
    CHECK(fc_move(&g, FC_SPOT_COL0 + 2, FC_SPOT_COL0 + 0, 0) == 0);
    /* cells */
    CHECK(fc_move(&g, FC_SPOT_COL0 + 2, FC_SPOT_CELL0, 0) == 1);
    CHECK(fc_move(&g, FC_SPOT_COL0 + 0, FC_SPOT_CELL0, 0) == 0);      /* occupied */
    CHECK(fc_size(&g, FC_SPOT_CELL0) == 1);
}

static void test_foundation_and_auto(void)
{
    fc_deal(&g, 1);
    memset(g.length, 0, sizeof g.length);
    memset(g.found, 0, sizeof g.found);
    memset(g.cell, FC_NO_CARD, sizeof g.cell);
    /* ace of hearts (1) on top of a 2 of hearts (5) in column 3 */
    g.column[3][0] = 5; g.column[3][1] = 1; g.length[3] = 2;
    CHECK(fc_move(&g, FC_SPOT_COL0 + 3, FC_SPOT_FOUND0 + 0, 0) == 1);  /* any pile: goes to its suit */
    CHECK(g.found[1] == 1);
    CHECK(fc_move(&g, FC_SPOT_COL0 + 3, FC_SPOT_FOUND0 + 2, 0) == 1);
    CHECK(g.found[1] == 2);
    CHECK(fc_undo(&g) && g.found[1] == 1 && g.length[3] == 1);
    /* the ace of spades is safe at once */
    g.column[4][0] = 0; g.length[4] = 1;
    CHECK(fc_auto(&g) == 1 && g.found[0] == 1);
    CHECK(fc_undo(&g) == true && g.found[0] == 0 && g.length[4] == 1);
}

static void test_playouts(void)
{
    long d;

    srand(7);
    for (d = 1; d <= 300; d++) {
        int step, i;

        fc_deal(&g, d);
        start = g;
        for (step = 0; step < 400; step++) {
            int from = rand() % FC_SPOTS, to = rand() % FC_SPOTS;
            int count = rand() % 4;

            if (fc_move(&g, from, to, count))
                fc_auto(&g);
            if (step % 50 == 0 && !all_cards_once(&g)) {
                CHECK(0);
                break;
            }
        }
        CHECK(all_cards_once(&g));
        for (i = 0; i < 100000 && fc_undo(&g); i++)
            ;
        CHECK(same_table(&g, &start));
        CHECK(g.moves == 0);
    }
}

int main(void)
{
    test_deal();
    test_rules();
    test_foundation_and_auto();
    test_playouts();
    printf("%d checks, %d failed\n", checks, failures);
    return failures != 0;
}
