/*
 * cards.c - small playing cards drawn into a pixel buffer (see cards.h).
 *
 * A card is a rounded rectangle with a one-pixel edge. Faces are built from
 * the rank (a tiny bitmap font) and the suit (a shape sampled at the size
 * wanted), and every piece can be drawn turned half way round so that the
 * second corner of a card reads upside down, as on a real one.
 *
 * Author: Anthropic Claude
 */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "cards.h"
#include "cards_const.h"

_Static_assert(CARD_MAX_W * CARD_MAX_H <= CARDS_MAX_PIXELS,
               "CARDS_MAX_PIXELS in cards.h is too small");

/* The card being drawn */
static uint16_t *buf;
static int cw, ch;

/* ===================================================================== */
/*  Public                                                                */
/* ===================================================================== */
int cards_size_count(void)  { return CARD_SIZES; }
int cards_style_count(void) { return CARD_STYLES; }
int cards_back_count(void)  { return CARD_BACKS; }
int cards_width(int size)   { return CARD_SIZE[size].w; }
int cards_height(int size)  { return CARD_SIZE[size].h; }

/* forward declarations (the public functions are at the top) */
static void begin(uint16_t *b, int size, uint16_t face, uint16_t edge);
static void put(int x, int y, uint16_t colour, bool turned);
static bool inside(int x, int y, int radius);
static int  draw_rank(int rank, int x, int y, int font, int scale,
                      uint16_t colour, bool turned);
static void draw_suit(int suit, int size, int x, int y, uint16_t colour,
                      bool turned);
static bool suit_hit(int suit, int u, int v);
static bool heart_hit(int u, int v);
static bool in_circle(int u, int v, int cx, int cy, int r);
static bool in_stem(int u, int v, int top);
static void draw_back_art(int design, int x0, int y0, int x1, int y1);

void cards_draw_face(uint16_t *b, int size, int style, int rank, int suit)
{
    const card_size_t *s = &CARD_SIZE[size];
    uint16_t ink = (suit == CARD_HEARTS || suit == CARD_DIAMONDS)
                       ? CARD_INK_RED : CARD_INK_BLACK;
    int font_h = (s->font == CARD_FONT_5X7) ? CARD_FONT5_H : CARD_FONT3_H;
    int rank_w, suit_size, scale, turn;

    begin(b, size, CARD_FACE, CARD_EDGE);

    if (style == CARD_STYLE_SLIM) {
        int w = draw_rank(rank, s->margin, SLIM_TOP, s->font, 1, ink, false);
        int fh = (s->font == CARD_FONT_5X7) ? CARD_FONT5_H : CARD_FONT3_H;

        draw_suit(suit, s->suit_small, s->margin + w + SLIM_GAP,
                  SLIM_TOP + (fh - s->suit_small) / 2, ink, false);
        return;
    }
    if (style == CARD_STYLE_INDEX) {
        scale = 2;
        suit_size = s->suit_big;
    } else {
        scale = 1;
        suit_size = s->suit_small;
    }

    /* the corner index; the two-corner style draws it a second time turned */
    for (turn = 0; turn < ((style == CARD_STYLE_TWO) ? 2 : 1); turn++) {
        bool t = (turn == 1);

        rank_w = draw_rank(rank, s->margin, s->margin, s->font, scale, ink, t);
        draw_suit(suit, suit_size, s->margin + (rank_w - suit_size) / 2,
                  s->margin + font_h * scale + 2, ink, t);
    }

    if (style == CARD_STYLE_PIP)
        draw_suit(suit, s->pip_big, (cw - s->pip_big) / 2,
                  (ch - s->pip_big) / 2 + s->margin + 1, ink, false);
    else if (style == CARD_STYLE_TWO)
        draw_suit(suit, s->pip_mid, (cw - s->pip_mid) / 2,
                  (ch - s->pip_mid) / 2, ink, false);
}

void cards_draw_strip(uint16_t *b, int size, int rank, int suit)
{
    cards_draw_face(b, size, CARD_STYLE_SLIM, rank, suit);
}

void cards_lay_on_card(uint16_t *b, int size)
{
    /* the rounded top corners lie on the card above: show its face and its
     * side edge there instead of the table */
    int r = CARD_SIZE[size].radius, w = CARD_SIZE[size].w, x, y;

    for (y = 0; y < r; y++)
        for (x = 0; x < w; x++)
            if (b[y * w + x] == CARD_FELT)
                b[y * w + x] = (x == 0 || x == w - 1) ? CARD_EDGE : CARD_FACE;
}

void cards_draw_slot(uint16_t *b, int size, int suit)
{
    const card_size_t *s = &CARD_SIZE[size];

    begin(b, size, CARD_FELT, CARD_SLOT_EDGE);
    if (suit >= 0)
        draw_suit(suit, s->pip_mid, (cw - s->pip_mid) / 2,
                  (ch - s->pip_mid) / 2, CARD_SLOT_EDGE, false);
}

void cards_draw_back(uint16_t *b, int size, int design)
{
    begin(b, size, CARD_FACE, CARD_EDGE);
    draw_back_art(design, BACK_INSET, BACK_INSET, cw - 1 - BACK_INSET,
                  ch - 1 - BACK_INSET);
}

/* ===================================================================== */
/*  The card shape                                                        */
/* ===================================================================== */

/* fill the buffer: table colour outside, edge colour on the rim, the face
 * colour inside */
static void begin(uint16_t *b, int size, uint16_t face, uint16_t edge_colour)
{
    int radius = CARD_SIZE[size].radius;
    int x, y;

    buf = b;
    cw = CARD_SIZE[size].w;
    ch = CARD_SIZE[size].h;
    for (y = 0; y < ch; y++) {
        for (x = 0; x < cw; x++) {
            uint16_t c;

            if (!inside(x, y, radius))
                c = CARD_FELT;
            else if (!inside(x - 1, y, radius) || !inside(x + 1, y, radius) ||
                     !inside(x, y - 1, radius) || !inside(x, y + 1, radius))
                c = edge_colour;
            else
                c = face;
            buf[y * cw + x] = c;
        }
    }
}

/* put a pixel; a turned piece lands at the opposite corner, upside down */
static void put(int x, int y, uint16_t colour, bool turned)
{
    if (turned) {
        x = cw - 1 - x;
        y = ch - 1 - y;
    }
    if (x >= 0 && x < cw && y >= 0 && y < ch)
        buf[y * cw + x] = colour;
}

/* is the pixel within the rounded rectangle? Only the four corners differ
 * from a plain rectangle. Doubled coordinates keep it in whole numbers. */
static bool inside(int x, int y, int radius)
{
    int dx, dy;

    if (x < 0 || y < 0 || x >= cw || y >= ch)
        return false;
    dx = (x < radius) ? radius - x : (x >= cw - radius) ? x - (cw - 1 - radius) : 0;
    dy = (y < radius) ? radius - y : (y >= ch - radius) ? y - (ch - 1 - radius) : 0;
    if (dx == 0 || dy == 0)
        return true;
    /* the centre of the pixel against the circle of the corner */
    return (2 * dx - 1) * (2 * dx - 1) + (2 * dy - 1) * (2 * dy - 1) <=
           4 * radius * radius - 2 * radius;
}

/* ===================================================================== */
/*  Rank text                                                             */
/* ===================================================================== */

/* draw the rank at x, y; returns the width drawn */
static int draw_rank(int rank, int x, int y, int font, int scale,
                     uint16_t colour, bool turned)
{
    const char *text = CARD_RANK_TEXT[rank];
    int gw = (font == CARD_FONT_5X7) ? CARD_FONT5_W : CARD_FONT3_W;
    int gh = (font == CARD_FONT_5X7) ? CARD_FONT5_H : CARD_FONT3_H;
    int pen = x;

    for (; *text; text++) {
        int g, row, col, sx, sy;

        switch (*text) {
        case 'A': g = CARD_GLYPH_A; break;
        case 'J': g = CARD_GLYPH_J; break;
        case 'Q': g = CARD_GLYPH_Q; break;
        case 'K': g = CARD_GLYPH_K; break;
        default:  g = *text - '0';  break;
        }
        for (row = 0; row < gh; row++) {
            uint8_t bits = (font == CARD_FONT_5X7) ? CARD_FONT5[g][row]
                                                  : CARD_FONT3[g][row];

            for (col = 0; col < gw; col++) {
                if (!(bits & (1 << (gw - 1 - col))))
                    continue;
                for (sy = 0; sy < scale; sy++)
                    for (sx = 0; sx < scale; sx++)
                        put(pen + col * scale + sx, y + row * scale + sy,
                            colour, turned);
            }
        }
        pen += (gw + 1) * scale;
    }
    return pen - x - scale;
}

/* ===================================================================== */
/*  Suits                                                                 */
/* ===================================================================== */

/* draw a suit into a size x size square at x, y */
static void draw_suit(int suit, int size, int x, int y, uint16_t colour,
                      bool turned)
{
    int px, py;

    for (py = 0; py < size; py++) {
        for (px = 0; px < size; px++) {
            int hits = 0, sx, sy;

            for (sy = 0; sy < SUIT_SAMPLES; sy++) {
                for (sx = 0; sx < SUIT_SAMPLES; sx++) {
                    int u = (2 * (px * SUIT_SAMPLES + sx) + 1) * SUIT_GRID /
                            (2 * size * SUIT_SAMPLES);
                    int v = (2 * (py * SUIT_SAMPLES + sy) + 1) * SUIT_GRID /
                            (2 * size * SUIT_SAMPLES);

                    if (suit_hit(suit, u, v))
                        hits++;
                }
            }
            if (hits >= SUIT_SAMPLES_MIN)
                put(x + px, y + py, colour, turned);
        }
    }
}

static int iabs(int v) { return v < 0 ? -v : v; }

static bool in_circle(int u, int v, int cx, int cy, int r)
{
    return (u - cx) * (u - cx) + (v - cy) * (v - cy) <= r * r;
}

/* the stem under a spade or a club: narrow at the top, wide at the foot */
static bool in_stem(int u, int v, int top)
{
    int span = STEM_BOTTOM - STEM_TOP;

    if (v < top || v > STEM_BOTTOM)
        return false;
    return iabs(u - 50) * span <=
           STEM_HALF_TOP * span + (v - STEM_TOP) * (STEM_HALF_BOTTOM - STEM_HALF_TOP);
}

static bool heart_hit(int u, int v)
{
    if (in_circle(u, v, HEART_LEFT_X, HEART_CIRCLE_Y, HEART_CIRCLE_R) ||
        in_circle(u, v, HEART_RIGHT_X, HEART_CIRCLE_Y, HEART_CIRCLE_R))
        return true;
    return v >= HEART_TRI_TOP && v <= HEART_TRI_TIP &&
           iabs(u - 50) * (HEART_TRI_TIP - HEART_TRI_TOP) <=
               HEART_TRI_HALF * (HEART_TRI_TIP - v);
}

static bool suit_hit(int suit, int u, int v)
{
    switch (suit) {
    case CARD_HEARTS:
        return heart_hit(u, v);
    case CARD_SPADES:
        if (v < SPADE_BODY_H && heart_hit(u, SUIT_GRID - 1 - v * SUIT_GRID / SPADE_BODY_H))
            return true;
        return in_stem(u, v, STEM_TOP);
    case CARD_DIAMONDS:
        return iabs(u - 50) * DIAMOND_HALF_H + iabs(v - 50) * DIAMOND_HALF_W <=
               DIAMOND_HALF_W * DIAMOND_HALF_H;
    default:
        if (in_circle(u, v, CLUB_TOP_X, CLUB_TOP_Y, CLUB_R) ||
            in_circle(u, v, CLUB_LEFT_X, CLUB_SIDE_Y, CLUB_R) ||
            in_circle(u, v, CLUB_RIGHT_X, CLUB_SIDE_Y, CLUB_R))
            return true;
        return in_stem(u, v, CLUB_STEM_TOP);
    }
}

/* ===================================================================== */
/*  Backs                                                                 */
/* ===================================================================== */

/* the pattern, inside the rectangle x0, y0 to x1, y1 (both included) */
static void draw_back_art(int design, int x0, int y0, int x1, int y1)
{
    uint16_t base = (design == 0) ? BACK_BLUE : (design == 1) ? BACK_RED :
                    (design == 2) ? BACK_NAVY : BACK_PURPLE;
    int cx2 = x0 + x1, cy2 = y0 + y1;       /* twice the centre */
    int x, y;

    for (y = y0; y <= y1; y++) {
        for (x = x0; x <= x1; x++) {
            uint16_t c = base;
            int rx = x - x0, ry = y - y0;

            switch (design) {
            case 0:                                 /* diamond lattice */
                if ((rx + ry) % BACK_LATTICE_STEP == 0 ||
                    (rx - ry + 1000 * BACK_LATTICE_STEP) % BACK_LATTICE_STEP == 0)
                    c = BACK_LINE;
                break;
            case 1:                                 /* dots */
                if (rx % BACK_DOT_STEP == 1 && ry % BACK_DOT_STEP == 1)
                    c = BACK_LINE;
                break;
            case 2: {                               /* frame and diamond */
                int fx = rx - BACK_FRAME_GAP, fy = ry - BACK_FRAME_GAP;
                int fw = x1 - x0 - BACK_FRAME_GAP, fh = y1 - y0 - BACK_FRAME_GAP;
                bool on_frame = fx >= 0 && fy >= 0 && fx <= fw - BACK_FRAME_GAP &&
                                fy <= fh - BACK_FRAME_GAP &&
                                (fx == 0 || fy == 0 || fx == fw - BACK_FRAME_GAP ||
                                 fy == fh - BACK_FRAME_GAP);
                int dx = iabs(2 * x - cx2), dy = iabs(2 * y - cy2);

                if (on_frame || (dx * (y1 - y0) + dy * (x1 - x0)) / 2 <=
                                    ((x1 - x0) * (y1 - y0)) / 3)
                    c = BACK_LINE;
                break;
            }
            default:                                /* diagonal stripes */
                if (((rx + ry) / BACK_STRIPE_WIDTH) % 2 == 0)
                    c = BACK_LINE;
                break;
            }
            put(x, y, c, false);
        }
    }
}
