/*
 * klondike.c - the rules of Klondike Solitaire (see klondike.h).
 *
 * Author: Anthropic Claude
 */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "klondike.h"

/* forward declarations (the public functions come first) */
static bool is_found(int spot);
static bool is_column(int spot);
static bool is_red(uint8_t c);
static bool fits_on(uint8_t card, uint8_t base);
static uint8_t card_at(const kl_t *g, int spot, int depth);
static void add_score(kl_t *g, int delta);
static void push_undo(kl_t *g, int type, int from, int to, int count, int flipped);
static void take(kl_t *g, int spot, int count, uint8_t *out);
static void put(kl_t *g, int spot, int count, const uint8_t *cards);

/* ===================================================================== */
/*  Deal                                                                  */
/* ===================================================================== */
void kl_deal(kl_t *g, long deal, int draw)
{
    uint8_t deck[52], order[52];
    unsigned long seed = (unsigned long)deal;
    int left = 52, i, col, row, next = 0;

    memset(g, 0, sizeof *g);
    g->deal = deal;
    g->draw = draw;
    for (i = 0; i < 52; i++)
        deck[i] = (uint8_t)i;
    for (i = 0; i < 52; i++) {
        int j;

        seed = (seed * KL_RAND_MUL + KL_RAND_ADD) & 0xFFFFFFFFUL;
        j = (int)(((seed >> KL_RAND_SHIFT) & KL_RAND_MASK) % (unsigned long)left);
        order[i] = deck[j];
        deck[j] = deck[--left];
    }
    for (col = 0; col < KL_COLUMNS; col++) {
        for (row = 0; row <= col; row++)
            g->column[col][g->length[col]++] = order[next++];
        g->hidden[col] = (uint8_t)col;          /* only the last one is up */
    }
    while (next < 52)
        g->stock[g->stock_n++] = order[next++];
}

/* ===================================================================== */
/*  Looking at the table                                                  */
/* ===================================================================== */
static bool is_found(int spot)  { return spot >= KL_SPOT_FOUND0 && spot < KL_SPOT_COL0; }
static bool is_column(int spot) { return spot >= KL_SPOT_COL0 && spot < KL_SPOTS; }
static bool is_red(uint8_t c)   { return KL_SUIT(c) == 1 || KL_SUIT(c) == 2; }

/* may card sit on base in a column? */
static bool fits_on(uint8_t card, uint8_t base)
{
    return KL_RANK(card) + 1 == KL_RANK(base) && is_red(card) != is_red(base);
}

uint8_t kl_top(const kl_t *g, int spot)
{
    return card_at(g, spot, 1);
}

/* the card depth places from the end of a spot (1 is the last one) */
static uint8_t card_at(const kl_t *g, int spot, int depth)
{
    if (spot == KL_SPOT_STOCK)
        return depth <= g->stock_n ? g->stock[g->stock_n - depth] : KL_NO_CARD;
    if (spot == KL_SPOT_WASTE)
        return depth <= g->waste_n ? g->waste[g->waste_n - depth] : KL_NO_CARD;
    if (is_found(spot)) {
        int suit = spot - KL_SPOT_FOUND0;

        return depth == 1 && g->found[suit]
                   ? (uint8_t)((g->found[suit] - 1) * 4 + suit) : KL_NO_CARD;
    }
    if (is_column(spot)) {
        int col = spot - KL_SPOT_COL0;

        return depth <= g->length[col] ? g->column[col][g->length[col] - depth]
                                       : KL_NO_CARD;
    }
    return KL_NO_CARD;
}

int kl_size(const kl_t *g, int spot)
{
    if (spot == KL_SPOT_STOCK)
        return g->stock_n;
    if (spot == KL_SPOT_WASTE)
        return g->waste_n;
    if (is_found(spot))
        return g->found[spot - KL_SPOT_FOUND0];
    if (is_column(spot))
        return g->length[spot - KL_SPOT_COL0];
    return 0;
}

int kl_faceup(const kl_t *g, int column_spot)
{
    int col = column_spot - KL_SPOT_COL0;

    return g->length[col] - g->hidden[col];
}

bool kl_won(const kl_t *g)
{
    int i;

    for (i = 0; i < KL_FOUNDATIONS; i++)
        if (g->found[i] != 13)
            return false;
    return true;
}

/* ===================================================================== */
/*  Score and undo list                                                   */
/* ===================================================================== */
static void add_score(kl_t *g, int delta)
{
    g->score += delta;
    if (g->score < 0)
        g->score = 0;
}

/* note what a move is about to change; call it before changing anything */
static void push_undo(kl_t *g, int type, int from, int to, int count, int flipped)
{
    kl_move_t *m;

    if (g->undo_count == KL_UNDO_MAX) {
        memmove(g->undo, g->undo + KL_UNDO_DROP,
                (size_t)(KL_UNDO_MAX - KL_UNDO_DROP) * sizeof g->undo[0]);
        g->undo_count -= KL_UNDO_DROP;
    }
    m = &g->undo[g->undo_count++];
    m->type = (uint8_t)type;
    m->from = (uint8_t)from;
    m->to = (uint8_t)to;
    m->count = (uint8_t)count;
    m->flipped = (uint8_t)flipped;
    m->passes = (uint8_t)g->passes;
    m->score = (int16_t)g->score;
}

/* ===================================================================== */
/*  Drawing from the stock                                                */
/* ===================================================================== */
int kl_draw(kl_t *g)
{
    int n, i;

    if (g->stock_n > 0) {
        n = g->stock_n < g->draw ? g->stock_n : g->draw;
        push_undo(g, KL_DRAW, KL_SPOT_STOCK, KL_SPOT_WASTE, n, 0);
        for (i = 0; i < n; i++)
            g->waste[g->waste_n++] = g->stock[--g->stock_n];
        g->moves++;
        return n;
    }
    if (g->waste_n > 0) {
        n = g->waste_n;
        push_undo(g, KL_RECYCLE, KL_SPOT_WASTE, KL_SPOT_STOCK, n, 0);
        while (g->waste_n > 0)
            g->stock[g->stock_n++] = g->waste[--g->waste_n];
        g->passes++;
        if (g->draw == 1)
            add_score(g, KL_SCORE_RECYCLE_ONE);
        else if (g->passes > KL_FREE_RECYCLES_THREE)
            add_score(g, KL_SCORE_RECYCLE_THREE);
        g->moves++;
        return -n;
    }
    return 0;
}

/* ===================================================================== */
/*  Moving cards                                                          */
/* ===================================================================== */

/* remove count cards from the end of a spot; out gets them in the order they
 * lay (the first one is the one deepest in the column) */
static void take(kl_t *g, int spot, int count, uint8_t *out)
{
    int i;

    if (spot == KL_SPOT_WASTE) {
        out[0] = g->waste[--g->waste_n];
    } else if (is_found(spot)) {
        out[0] = kl_top(g, spot);
        g->found[spot - KL_SPOT_FOUND0]--;
    } else {
        int col = spot - KL_SPOT_COL0;

        g->length[col] = (uint8_t)(g->length[col] - count);
        for (i = 0; i < count; i++)
            out[i] = g->column[col][g->length[col] + i];
    }
}

static void put(kl_t *g, int spot, int count, const uint8_t *cards)
{
    int i;

    if (spot == KL_SPOT_WASTE) {
        g->waste[g->waste_n++] = cards[0];
    } else if (is_found(spot)) {
        g->found[KL_SUIT(cards[0])]++;
    } else {
        int col = spot - KL_SPOT_COL0;

        for (i = 0; i < count; i++)
            g->column[col][g->length[col]++] = cards[i];
    }
}

int kl_move(kl_t *g, int from, int to, int count)
{
    uint8_t cards[KL_MAX_COLUMN];
    int avail, k, flipped = 0, delta = 0;

    if (from == to)
        return 0;
    if (from == KL_SPOT_WASTE)
        avail = g->waste_n ? 1 : 0;
    else if (is_found(from))
        avail = g->found[from - KL_SPOT_FOUND0] ? 1 : 0;
    else if (is_column(from))
        avail = kl_faceup(g, from);
    else
        return 0;
    if (avail == 0)
        return 0;

    if (is_found(to)) {
        uint8_t c = card_at(g, from, 1);

        if (count > 1 || is_found(from) || KL_RANK(c) != g->found[KL_SUIT(c)])
            return 0;
        to = KL_SPOT_FOUND0 + KL_SUIT(c);
        count = 1;
    } else if (is_column(to)) {
        int col = to - KL_SPOT_COL0;

        if (g->length[col] == 0) {                      /* only a king fits */
            if (count == 0) {
                for (k = 1; k <= avail; k++)
                    if (KL_RANK(card_at(g, from, k)) == 12) {
                        count = k;
                        break;
                    }
                if (count == 0)
                    return 0;
            } else if (count > avail || KL_RANK(card_at(g, from, count)) != 12) {
                return 0;
            }
            if (is_column(from) && g->length[from - KL_SPOT_COL0] == count)
                return 0;                   /* already at the foot of its column */
        } else {
            uint8_t base = kl_top(g, to);

            if (count == 0) {
                for (k = 1; k <= avail; k++)
                    if (fits_on(card_at(g, from, k), base)) {
                        count = k;
                        break;
                    }
                if (count == 0)
                    return 0;
            } else if (count > avail || !fits_on(card_at(g, from, count), base)) {
                return 0;
            }
        }
    } else {
        return 0;
    }

    if (is_column(from)) {
        int col = from - KL_SPOT_COL0;
        int left = g->length[col] - count;

        flipped = left > 0 && g->hidden[col] == left;
    }
    push_undo(g, KL_MOVE, from, to, count, flipped);
    g->last_flipped = flipped;
    take(g, from, count, cards);
    put(g, to, count, cards);
    if (flipped)
        g->hidden[from - KL_SPOT_COL0]--;

    if (from == KL_SPOT_WASTE)
        delta = is_found(to) ? KL_SCORE_TO_FOUNDATION : KL_SCORE_TO_COLUMN;
    else if (is_found(from))
        delta = KL_SCORE_FROM_FOUNDATION;
    else if (is_found(to))
        delta = KL_SCORE_TO_FOUNDATION;
    if (flipped)
        delta += KL_SCORE_TURN_OVER;
    add_score(g, delta);
    g->moves++;
    return count;
}

/* ===================================================================== */
/*  Finishing                                                             */
/* ===================================================================== */
bool kl_can_finish(const kl_t *g)
{
    int i;

    if (g->stock_n > 0 || g->waste_n > 0 || kl_won(g))
        return false;
    for (i = 0; i < KL_COLUMNS; i++)
        if (g->hidden[i] > 0)
            return false;
    return true;
}

/* With every card face up the cards of a column are one run, each lower than
 * the one above, so the card of lowest rank still on the table is at the foot of
 * its column and is the next one its foundation wants. */
bool kl_finish_step(kl_t *g, int *from, int *to)
{
    int col;

    for (col = 0; col < KL_COLUMNS; col++) {
        uint8_t c = kl_top(g, KL_SPOT_COL0 + col);

        if (c != KL_NO_CARD && KL_RANK(c) == g->found[KL_SUIT(c)]) {
            *from = KL_SPOT_COL0 + col;
            *to = KL_SPOT_FOUND0 + KL_SUIT(c);
            return kl_move(g, *from, *to, 0) == 1;
        }
    }
    return false;
}

/* ===================================================================== */
/*  Undo                                                                  */
/* ===================================================================== */
bool kl_undo(kl_t *g)
{
    const kl_move_t *m;
    uint8_t cards[KL_MAX_COLUMN];
    int i;

    if (g->undo_count == 0)
        return false;
    m = &g->undo[--g->undo_count];
    if (m->type == KL_MOVE) {
        take(g, m->to, m->count, cards);
        put(g, m->from, m->count, cards);
        if (m->flipped)
            g->hidden[m->from - KL_SPOT_COL0]++;
    } else if (m->type == KL_DRAW) {
        for (i = 0; i < m->count; i++)
            g->stock[g->stock_n++] = g->waste[--g->waste_n];
    } else {
        for (i = 0; i < m->count; i++)
            g->waste[g->waste_n++] = g->stock[--g->stock_n];
    }
    g->passes = m->passes;
    g->score = m->score;
    if (g->moves > 0)
        g->moves--;
    return true;
}
