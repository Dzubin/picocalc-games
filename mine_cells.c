/*
 * mine_cells.c - the squares of the Minesweeper board drawn into a pixel
 * buffer (see mine_cells.h).
 *
 * Author: Anthropic Claude
 */
#include <stdbool.h>
#include <stdint.h>

#include "mine_cells.h"
#include "mine_cells_const.h"

/* the square being drawn */
static uint16_t *buf;
static int       sz;

/* forward declarations (the public function comes first) */
static void fill_covered(void);
static void fill_open(uint16_t colour);
static void draw_digit(int glyph, uint16_t colour);
static void draw_shape(int shape, bool with_accent_red);
static int  shape_class(int shape, int u, int v);
static int  iabs(int v);
static bool in_circle(int u, int v, int cx, int cy, int r);
static void draw_cross(void);
static void draw_frame(void);

#define SHAPE_MINE 0
#define SHAPE_FLAG 1

/* ===================================================================== */
/*  Public                                                                */
/* ===================================================================== */
void mine_draw_cell(uint16_t *b, int size, int kind, int number, bool cursor)
{
    buf = b;
    sz = size;

    switch (kind) {
    case MINE_CELL_FLAG:
        fill_covered();
        draw_shape(SHAPE_FLAG, true);
        break;
    case MINE_CELL_QUESTION:
        fill_covered();
        draw_digit(MINE_GLYPH_QUESTION, MINE_COL_INK);
        break;
    case MINE_CELL_OPEN:
        fill_open(MINE_COL_OPEN);
        if (number > 0)
            draw_digit(number, MINE_COL_NUMBER[number]);
        break;
    case MINE_CELL_MINE:
        fill_open(MINE_COL_OPEN);
        draw_shape(SHAPE_MINE, false);
        break;
    case MINE_CELL_HIT:
        fill_open(MINE_COL_HIT);
        draw_shape(SHAPE_MINE, false);
        break;
    case MINE_CELL_WRONG:
        fill_open(MINE_COL_OPEN);
        draw_shape(SHAPE_MINE, false);
        draw_cross();
        break;
    default:
        fill_covered();
        break;
    }
    if (cursor)
        draw_frame();
}

/* ===================================================================== */
/*  Backgrounds and the cursor                                            */
/* ===================================================================== */

/* a raised square: lit edge above and left, shaded edge below and right */
static void fill_covered(void)
{
    int bevel = sz >= MINE_BEVEL_BIG_FROM ? 2 : 1, x, y;

    for (y = 0; y < sz; y++) {
        for (x = 0; x < sz; x++) {
            uint16_t c = MINE_COL_COVERED;

            if (x < bevel || y < bevel)
                c = (x >= sz - bevel || y >= sz - bevel) ? MINE_COL_DARK : MINE_COL_LIGHT;
            if (x >= sz - bevel || y >= sz - bevel)
                c = MINE_COL_DARK;
            buf[y * sz + x] = c;
        }
    }
}

/* a flat square with a line along its right and bottom edges */
static void fill_open(uint16_t colour)
{
    int x, y;

    for (y = 0; y < sz; y++)
        for (x = 0; x < sz; x++)
            buf[y * sz + x] = (x == sz - 1 || y == sz - 1) ? MINE_COL_GRID : colour;
}

static void draw_frame(void)
{
    int t = sz >= MINE_CURSOR_BIG_FROM ? 2 : 1, x, y;

    for (y = 0; y < sz; y++)
        for (x = 0; x < sz; x++)
            if (x < t || y < t || x >= sz - t || y >= sz - t)
                buf[y * sz + x] = MINE_COL_CURSOR;
}

/* ===================================================================== */
/*  Digits and the question mark                                          */
/* ===================================================================== */

/* the glyph, as big as fits and in the middle of the square */
static void draw_digit(int glyph, uint16_t colour)
{
    int room = sz - 2 * MINE_MARGIN;
    int scale = room / MINE_FONT_W < room / MINE_FONT_H ? room / MINE_FONT_W
                                                        : room / MINE_FONT_H;
    int x0, y0, row, col, sx, sy;

    if (scale < 1)
        scale = 1;
    x0 = (sz - MINE_FONT_W * scale) / 2;
    y0 = (sz - MINE_FONT_H * scale) / 2;
    for (row = 0; row < MINE_FONT_H; row++) {
        for (col = 0; col < MINE_FONT_W; col++) {
            if (!(MINE_GLYPH[glyph][row] & (1 << (MINE_FONT_W - 1 - col))))
                continue;
            for (sy = 0; sy < scale; sy++)
                for (sx = 0; sx < scale; sx++) {
                    int x = x0 + col * scale + sx, y = y0 + row * scale + sy;

                    if (x >= 0 && x < sz && y >= 0 && y < sz)
                        buf[y * sz + x] = colour;
                }
        }
    }
}

/* ===================================================================== */
/*  Shapes                                                                */
/* ===================================================================== */
static int iabs(int v) { return v < 0 ? -v : v; }

static bool in_circle(int u, int v, int cx, int cy, int r)
{
    return (u - cx) * (u - cx) + (v - cy) * (v - cy) <= r * r;
}

/* what the point (u, v) of a shape is part of */
static int shape_class(int shape, int u, int v)
{
    if (shape == SHAPE_MINE) {
        int du = u - 50, dv = v - 50;

        if (in_circle(u, v, MINE_SHINE_X, MINE_SHINE_Y, MINE_SHINE_R))
            return SHAPE_ACCENT;
        if (in_circle(u, v, 50, 50, MINE_DISC_R))
            return SHAPE_INK;
        if ((iabs(du) <= MINE_BAR_HALF && iabs(dv) <= MINE_BAR_LENGTH) ||
            (iabs(dv) <= MINE_BAR_HALF && iabs(du) <= MINE_BAR_LENGTH))
            return SHAPE_INK;
        if ((iabs(du - dv) <= MINE_DIAG_HALF && iabs(du) <= MINE_DIAG_LENGTH) ||
            (iabs(du + dv) <= MINE_DIAG_HALF && iabs(du) <= MINE_DIAG_LENGTH))
            return SHAPE_INK;
        return SHAPE_NONE;
    }
    /* the flag: the triangle points left from the top of the pole */
    if (u >= FLAG_TRI_TIP_X && u <= FLAG_POLE_X0 && v >= FLAG_TRI_TOP && v <= FLAG_TRI_BOTTOM) {
        int span = FLAG_POLE_X0 - FLAG_TRI_TIP_X;
        int half_h = (FLAG_TRI_BOTTOM - FLAG_TRI_TOP) / 2;
        int mid = (FLAG_TRI_BOTTOM + FLAG_TRI_TOP) / 2;

        /* the height of the triangle at u grows from 0 at the tip */
        if (iabs(v - mid) * span <= half_h * (u - FLAG_TRI_TIP_X))
            return SHAPE_ACCENT;
    }
    if (u >= FLAG_POLE_X0 && u <= FLAG_POLE_X1 && v >= FLAG_POLE_TOP && v <= FLAG_POLE_BOTTOM)
        return SHAPE_INK;
    if (u >= FLAG_BASE_X0 && u <= FLAG_BASE_X1 && v >= FLAG_BASE_TOP && v <= FLAG_POLE_BOTTOM + 6)
        return SHAPE_INK;
    return SHAPE_NONE;
}

/* draw a shape into the square, inside a margin: the ink is black, the accent
 * red on the flag and a white spot of light on the mine */
static void draw_shape(int shape, bool with_accent_red)
{
    int inset = sz / MINE_SHAPE_INSET_DIV, box = sz - 2 * inset, px, py;

    for (py = 0; py < box; py++) {
        for (px = 0; px < box; px++) {
            int ink = 0, accent = 0, sx, sy;

            for (sy = 0; sy < SHAPE_SAMPLES; sy++) {
                for (sx = 0; sx < SHAPE_SAMPLES; sx++) {
                    int u = (2 * (px * SHAPE_SAMPLES + sx) + 1) * SHAPE_GRID /
                            (2 * box * SHAPE_SAMPLES);
                    int v = (2 * (py * SHAPE_SAMPLES + sy) + 1) * SHAPE_GRID /
                            (2 * box * SHAPE_SAMPLES);
                    int c = shape_class(shape, u, v);

                    if (c == SHAPE_INK)
                        ink++;
                    else if (c == SHAPE_ACCENT)
                        accent++;
                }
            }
            if (accent >= SHAPE_SAMPLES_MIN)
                buf[(inset + py) * sz + inset + px] =
                    with_accent_red ? MINE_COL_RED : MINE_COL_SHINE;
            else if (ink + accent >= SHAPE_SAMPLES_MIN)
                buf[(inset + py) * sz + inset + px] = MINE_COL_INK;
        }
    }
}

/* the red cross over a wrong flag, corner to corner */
static void draw_cross(void)
{
    int x, y, half = sz * CROSS_HALF / SHAPE_GRID;

    if (half < 1)
        half = 0;
    for (y = 0; y < sz - 1; y++)
        for (x = 0; x < sz - 1; x++)
            if (iabs(x - y) <= half || iabs(x + y - (sz - 2)) <= half)
                buf[y * sz + x] = MINE_COL_RED;
}
