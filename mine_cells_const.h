/*
 * mine_cells_const.h - every constant of the Minesweeper squares: colours,
 * the digit font and the shapes of the flag and the mine.
 *
 * Author: Anthropic Claude
 */
#ifndef MINE_CELLS_CONST_H
#define MINE_CELLS_CONST_H

#include <stdint.h>

/* RGB565 from 8-bit channels (the same packing the LCD driver uses) */
#define MINE_RGB(r, g, b) \
    ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))

/* ------------------------------------------------------------------ */
/*  Colours (strong ones: dim greys are hard to read on the LCD)         */
/* ------------------------------------------------------------------ */
#define MINE_COL_COVERED   MINE_RGB(110, 160, 255)   /* a square not yet opened */
#define MINE_COL_LIGHT     MINE_RGB(220, 235, 255)   /* its lit edge            */
#define MINE_COL_DARK      MINE_RGB(  0,  40, 140)   /* its shaded edge         */
#define MINE_COL_OPEN      MINE_RGB(250, 250, 250)   /* an open square          */
#define MINE_COL_GRID      MINE_RGB(150, 160, 190)   /* the line between them   */
#define MINE_COL_INK       MINE_RGB(  0,   0,   0)   /* mine, pole, question mark */
#define MINE_COL_RED       MINE_RGB(230,   0,   0)   /* flag, the mine hit, a wrong flag */
#define MINE_COL_HIT       MINE_RGB(255, 225,  70)   /* background of the mine that was hit */
#define MINE_COL_SHINE     MINE_RGB(255, 255, 255)   /* the spot of light on a mine */
#define MINE_COL_CURSOR    MINE_RGB(255, 225,  70)   /* frame of the square under the cursor */

/* the colour of the number 1 to 8 */
static const uint16_t MINE_COL_NUMBER[9] = {
    MINE_RGB(  0,   0,   0),    /* 0 is not drawn */
    MINE_RGB(  0,  50, 255),    /* 1 blue         */
    MINE_RGB(  0, 140,   0),    /* 2 green        */
    MINE_RGB(230,   0,   0),    /* 3 red          */
    MINE_RGB(  0,   0, 130),    /* 4 navy         */
    MINE_RGB(150,   0,   0),    /* 5 maroon       */
    MINE_RGB(  0, 140, 140),    /* 6 teal         */
    MINE_RGB(  0,   0,   0),    /* 7 black        */
    MINE_RGB(150,   0, 150),    /* 8 purple       */
};

/* ------------------------------------------------------------------ */
/*  The look of a square, by its size in pixels                         */
/* ------------------------------------------------------------------ */
#define MINE_BEVEL_BIG_FROM   14    /* a square this big or bigger has a 2 pixel edge */
#define MINE_CURSOR_BIG_FROM  16    /* ... and a 2 pixel cursor frame                 */
#define MINE_SHAPE_INSET_DIV   8    /* a flag or mine keeps size / this off the edge  */
#define MINE_MARGIN            2    /* room left round a digit                        */

/* ------------------------------------------------------------------ */
/*  Digits 1 to 8 and the question mark, 5 x 7 dots, one row per byte     */
/*  with the leftmost dot in the highest of the 5 bits                    */
/* ------------------------------------------------------------------ */
#define MINE_FONT_W  5
#define MINE_FONT_H  7
#define MINE_GLYPH_QUESTION 9
static const uint8_t MINE_GLYPH[10][MINE_FONT_H] = {
    { 0, 0, 0, 0, 0, 0, 0 },                            /* 0 is not drawn */
    { 0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E },       /* 1 */
    { 0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F },       /* 2 */
    { 0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E },       /* 3 */
    { 0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02 },       /* 4 */
    { 0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E },       /* 5 */
    { 0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E },       /* 6 */
    { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08 },       /* 7 */
    { 0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E },       /* 8 */
    { 0x0E, 0x11, 0x01, 0x02, 0x04, 0x00, 0x04 },       /* ? */
};

/* ------------------------------------------------------------------ */
/*  The shapes, drawn on a 0 to 99 grid and sampled for any size         */
/* ------------------------------------------------------------------ */
#define SHAPE_GRID        100
#define SHAPE_SAMPLES     3     /* sub-samples across and down one pixel   */
#define SHAPE_SAMPLES_MIN 4     /* of 9 that must hit for a lit pixel      */

/* what a sampled point is part of */
#define SHAPE_NONE   0
#define SHAPE_INK    1
#define SHAPE_ACCENT 2          /* red on the flag, the spot of light on the mine */

/* the mine: a disc with spikes (bars through the middle) and a spot of light */
#define MINE_DISC_R        30
#define MINE_BAR_HALF       6
#define MINE_BAR_LENGTH    44
#define MINE_DIAG_HALF      8
#define MINE_DIAG_LENGTH   34
#define MINE_SHINE_X       40
#define MINE_SHINE_Y       40
#define MINE_SHINE_R        7

/* the flag: a red triangle on a pole, with a base */
#define FLAG_POLE_X0       46
#define FLAG_POLE_X1       54
#define FLAG_POLE_TOP      10
#define FLAG_POLE_BOTTOM   84
#define FLAG_BASE_X0       28
#define FLAG_BASE_X1       74
#define FLAG_BASE_TOP      78
#define FLAG_TRI_TIP_X      8
#define FLAG_TRI_TOP       10
#define FLAG_TRI_BOTTOM    58

/* the cross over a flag on a square with no mine */
#define CROSS_HALF          7

#endif /* MINE_CELLS_CONST_H */
