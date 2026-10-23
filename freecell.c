/*
 * freecell.c - the rules of FreeCell (see freecell.h).
 *
 * Author: Anthropic Claude
 */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "freecell.h"

/* forward declarations (the public functions come first) */
static bool is_cell(int spot);
static bool is_found(int spot);
static bool is_column(int spot);
static bool is_red(uint8_t c);
static bool fits_on(uint8_t card, uint8_t base);
static int  empty_columns(const fc_t *g);
static int  free_cells(const fc_t *g);
static void push_undo(fc_t *g, int from, int to, int count, int chain);
static void take(fc_t *g, int spot, int count, uint8_t *out);
static void put(fc_t *g, int spot, int count, const uint8_t *cards);
static bool safe_to_found(const fc_t *g, uint8_t c);

/* ===================================================================== */
/*  Deal                                                                  */
/* ===================================================================== */
void fc_deal(fc_t *g, long deal)
{
    uint8_t deck[52];
    unsigned long seed = (unsigned long)deal;
    int left = 52, i;

    memset(g, 0, sizeof *g);
    g->deal = deal;
    memset(g->cell, FC_NO_CARD, sizeof g->cell);

    /* the card numbers of the classic game: rank * 4 + suit, suits clubs,
     * diamonds, hearts, spades; ours are flipped to match cards.h */
    for (i = 0; i < 52; i++)
        deck[i] = (uint8_t)(FC_RANK(i) * 4 + (FC_SUIT_FLIP - FC_SUIT(i)));

    for (i = 0; i < 52; i++) {
        int j, col;

        seed = (seed * FC_RAND_MUL + FC_RAND_ADD) & 0xFFFFFFFFUL;
        j = (int)(((seed >> FC_RAND_SHIFT) & FC_RAND_MASK) % (unsigned long)left);
        col = i % FC_COLUMNS;
        g->column[col][g->length[col]++] = deck[j];
        deck[j] = deck[--left];
    }
}

/* ===================================================================== */
/*  Looking at the table                                                  */
/* ===================================================================== */
static bool is_cell(int spot)   { return spot >= FC_SPOT_CELL0 && spot < FC_SPOT_FOUND0; }
static bool is_found(int spot)  { return spot >= FC_SPOT_FOUND0 && spot < FC_SPOT_COL0; }
static bool is_column(int spot) { return spot >= FC_SPOT_COL0 && spot < FC_SPOTS; }
static bool is_red(uint8_t c)   { return FC_SUIT(c) == 1 || FC_SUIT(c) == 2; }

/* may card sit on base in a column? */
static bool fits_on(uint8_t card, uint8_t base)
{
    return FC_RANK(card) + 1 == FC_RANK(base) && is_red(card) != is_red(base);
}

static int empty_columns(const fc_t *g)
{
    int i, n = 0;

    for (i = 0; i < FC_COLUMNS; i++)
        if (g->length[i] == 0)
            n++;
    return n;
}

static int free_cells(const fc_t *g)
{
    int i, n = 0;

    for (i = 0; i < FC_CELLS; i++)
        if (g->cell[i] == FC_NO_CARD)
            n++;
    return n;
}

uint8_t fc_top(const fc_t *g, int spot)
{
    if (is_cell(spot))
        return g->cell[spot - FC_SPOT_CELL0];
    if (is_column(spot)) {
        int col = spot - FC_SPOT_COL0;

        return g->length[col] ? g->column[col][g->length[col] - 1] : FC_NO_CARD;
    }
    if (is_found(spot)) {
        int suit = spot - FC_SPOT_FOUND0;

        return g->found[suit] ? (uint8_t)((g->found[suit] - 1) * 4 + suit)
                              : FC_NO_CARD;
    }
    return FC_NO_CARD;
}

int fc_size(const fc_t *g, int spot)
{
    if (is_cell(spot))
        return g->cell[spot - FC_SPOT_CELL0] != FC_NO_CARD;
    if (is_found(spot))
        return g->found[spot - FC_SPOT_FOUND0];
    if (is_column(spot))
        return g->length[spot - FC_SPOT_COL0];
    return 0;
}

int fc_run_length(const fc_t *g, int column_spot)
{
    int col = column_spot - FC_SPOT_COL0;
    int n = g->length[col], run = 0;

    if (n == 0)
        return 0;
    run = 1;
    while (run < n && fits_on(g->column[col][n - run], g->column[col][n - run - 1]))
        run++;
    return run;
}

int fc_capacity(const fc_t *g, bool to_empty)
{
    int empties = empty_columns(g) - (to_empty ? 1 : 0);

    if (empties < 0)
        empties = 0;
    return (free_cells(g) + 1) << empties;
}

bool fc_won(const fc_t *g)
{
    int i;

    for (i = 0; i < FC_FOUNDATIONS; i++)
        if (g->found[i] != 13)
            return false;
    return true;
}

/* ===================================================================== */
/*  Moving cards                                                          */
/* ===================================================================== */
static void push_undo(fc_t *g, int from, int to, int count, int chain)
{
    fc_move_t *m;

    if (g->undo_count == FC_UNDO_MAX) {
        memmove(g->undo, g->undo + FC_UNDO_DROP,
                (size_t)(FC_UNDO_MAX - FC_UNDO_DROP) * sizeof g->undo[0]);
        g->undo_count -= FC_UNDO_DROP;
        g->undo[0].chain = 0;       /* the oldest kept move stands alone */
    }
    m = &g->undo[g->undo_count++];
    m->from = (uint8_t)from;
    m->to = (uint8_t)to;
    m->count = (uint8_t)count;
    m->chain = (uint8_t)chain;
}

/* remove count cards from the end of a spot; out gets them in the order they
 * lay (the first one is the one deepest in the column) */
static void take(fc_t *g, int spot, int count, uint8_t *out)
{
    int i;

    if (is_cell(spot)) {
        out[0] = g->cell[spot - FC_SPOT_CELL0];
        g->cell[spot - FC_SPOT_CELL0] = FC_NO_CARD;
    } else if (is_column(spot)) {
        int col = spot - FC_SPOT_COL0;

        g->length[col] = (uint8_t)(g->length[col] - count);
        for (i = 0; i < count; i++)
            out[i] = g->column[col][g->length[col] + i];
    } else {
        out[0] = fc_top(g, spot);
        g->found[spot - FC_SPOT_FOUND0]--;
    }
}

static void put(fc_t *g, int spot, int count, const uint8_t *cards)
{
    int i;

    if (is_cell(spot)) {
        g->cell[spot - FC_SPOT_CELL0] = cards[0];
    } else if (is_column(spot)) {
        int col = spot - FC_SPOT_COL0;

        for (i = 0; i < count; i++)
            g->column[col][g->length[col]++] = cards[i];
    } else {
        g->found[FC_SUIT(cards[0])]++;
    }
}

int fc_move(fc_t *g, int from, int to, int count)
{
    uint8_t cards[FC_MAX_COLUMN] = { 0 };
    uint8_t first;
    int run;

    if (from == to || !(is_cell(from) || is_column(from)) || fc_size(g, from) == 0)
        return 0;
    run = is_column(from) ? fc_run_length(g, from) : 1;

    if (is_found(to)) {
        uint8_t c = fc_top(g, from);

        if (count > 1 || FC_RANK(c) != g->found[FC_SUIT(c)])
            return 0;
        to = FC_SPOT_FOUND0 + FC_SUIT(c);
        count = 1;
    } else if (is_cell(to)) {
        if ((count > 1) || g->cell[to - FC_SPOT_CELL0] != FC_NO_CARD)
            return 0;
        count = 1;
    } else if (is_column(to)) {
        int col = to - FC_SPOT_COL0;

        if (g->length[col] == 0) {
            if (count == 0)
                count = 1;
            if (count > run || count > fc_capacity(g, true))
                return 0;
        } else {
            uint8_t base = fc_top(g, to);
            int k;

            if (count == 0) {                   /* find the card that fits */
                for (k = 1; k <= run; k++) {
                    uint8_t c = is_column(from)
                        ? g->column[from - FC_SPOT_COL0][g->length[from - FC_SPOT_COL0] - k]
                        : fc_top(g, from);

                    if (fits_on(c, base)) {
                        count = k;
                        break;
                    }
                }
                if (count == 0)
                    return 0;
            }
            if (count > run || count > fc_capacity(g, false))
                return 0;
            first = is_column(from)
                ? g->column[from - FC_SPOT_COL0][g->length[from - FC_SPOT_COL0] - count]
                : fc_top(g, from);
            if (!fits_on(first, base))
                return 0;
        }
    } else {
        return 0;
    }
    if (count > run)
        return 0;

    take(g, from, count, cards);
    put(g, to, count, cards);
    push_undo(g, from, to, count, 0);
    g->moves++;
    return count;
}

/* ===================================================================== */
/*  Automatic moves and undo                                              */
/* ===================================================================== */

/* a card is safe to send home when nothing of the other colour could still
 * want to sit on it: both piles of the other colour hold the rank below */
static bool safe_to_found(const fc_t *g, uint8_t c)
{
    int rank = FC_RANK(c);
    int a = is_red(c) ? 0 : 1, b = is_red(c) ? 3 : 2;   /* other colour's suits */

    if (g->found[FC_SUIT(c)] != rank)
        return false;
    return rank == 0 || (g->found[a] >= rank && g->found[b] >= rank);
}

bool fc_auto_step(fc_t *g, int *from, int *to, int *count)
{
    int spot;

    for (spot = FC_SPOT_CELL0; spot < FC_SPOTS; spot++) {
        uint8_t c;
        uint8_t cards[1];

        if (is_found(spot))
            continue;
        c = fc_top(g, spot);
        if (c == FC_NO_CARD || !safe_to_found(g, c))
            continue;
        take(g, spot, 1, cards);
        put(g, FC_SPOT_FOUND0 + FC_SUIT(c), 1, cards);
        push_undo(g, spot, FC_SPOT_FOUND0 + FC_SUIT(c), 1, 1);
        *from = spot;
        *to = FC_SPOT_FOUND0 + FC_SUIT(c);
        *count = 1;
        return true;
    }
    return false;
}

int fc_auto(fc_t *g)
{
    int moved = 0, from, to, count;

    while (fc_auto_step(g, &from, &to, &count))
        moved++;
    return moved;
}

bool fc_undo(fc_t *g)
{
    bool more = true;

    if (g->undo_count == 0)
        return false;
    while (more && g->undo_count > 0) {
        const fc_move_t *m = &g->undo[--g->undo_count];
        uint8_t cards[FC_MAX_COLUMN];

        more = m->chain != 0;
        take(g, m->to, m->count, cards);
        put(g, m->from, m->count, cards);
        if (!m->chain && g->moves > 0)
            g->moves--;
    }
    return true;
}
