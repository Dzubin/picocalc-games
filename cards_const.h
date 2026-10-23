/*
 * cards_const.h - every constant of the playing-card drawing code: colours,
 * the sizes of the cards, the two small fonts for the rank, and the patterns
 * of the card backs.
 *
 * Author: Anthropic Claude
 */
#ifndef CARDS_CONST_H
#define CARDS_CONST_H

#include <stdint.h>

/* RGB565 from 8-bit channels (the same packing the LCD driver uses) */
#define CARD_RGB(r, g, b) \
    ((uint16_t)((((r) >> 3) << 11) | (((g) >> 2) << 5) | ((b) >> 3)))

/* ------------------------------------------------------------------ */
/*  Colours (full strength; dim greys are hard to read on the LCD)      */
/* ------------------------------------------------------------------ */
#define CARD_FELT      CARD_RGB(  0, 110,  40)  /* shows at rounded corners */
#define CARD_FACE      CARD_RGB(255, 255, 255)
#define CARD_EDGE      CARD_RGB(  0,   0,   0)
#define CARD_INK_BLACK CARD_RGB(  0,   0,   0)
#define CARD_INK_RED   CARD_RGB(225,   0,   0)
#define CARD_SLOT_EDGE CARD_RGB(255, 255, 255)  /* outline of an empty spot */

/* ------------------------------------------------------------------ */
/*  Names                                                               */
/* ------------------------------------------------------------------ */
#define CARD_SPADES    0
#define CARD_HEARTS    1
#define CARD_DIAMONDS  2
#define CARD_CLUBS     3
#define CARD_SUITS     4
#define CARD_RANKS     13

/* How a face is laid out */
#define CARD_STYLE_PIP    0   /* corner index and one big suit in the middle */
#define CARD_STYLE_INDEX  1   /* big corner index only, nothing else         */
#define CARD_STYLE_TWO    2   /* index in two corners and a small middle suit */
#define CARD_STYLES       3
#define CARD_STYLE_SLIM   3   /* rank and suit side by side, for covered cards */
#define SLIM_TOP          2   /* distance of the slim index from the top edge  */
#define SLIM_GAP          2   /* between the rank and the suit                 */

/* Rank text as drawn on the card */
static const char *const CARD_RANK_TEXT[CARD_RANKS] = {
    "A", "2", "3", "4", "5", "6", "7", "8", "9", "10", "J", "Q", "K"
};

/* ------------------------------------------------------------------ */
/*  Sizes. All close to the 5:7 shape of a real card (2.5 x 3.5 in).    */
/* ------------------------------------------------------------------ */
#define CARD_FONT_5X7  0
#define CARD_FONT_3X5  1

#define CARD_MAX_W  38
#define CARD_MAX_H  53

typedef struct {
    int w, h;          /* size in pixels                                    */
    int radius;        /* rounding of the corners                           */
    int font;          /* font of the rank                                  */
    int margin;        /* distance of the index from the edge               */
    int suit_small;    /* suit under the rank (pip and two-corner styles)   */
    int suit_big;      /* suit under the big rank (index style)             */
    int pip_big;       /* the one big suit in the pip style                 */
    int pip_mid;       /* the small middle suit in the two-corner style     */
} card_size_t;

#define CARD_SIZES 4

static const card_size_t CARD_SIZE[CARD_SIZES] = {
    /*  w   h  r  font           mg  ss  sb  big mid */
    { 38, 53, 4, CARD_FONT_5X7,  3,  7, 11, 17, 11 },
    { 34, 48, 4, CARD_FONT_5X7,  3,  7, 11, 15, 11 },
    { 30, 42, 3, CARD_FONT_5X7,  2,  5,  9, 13,  9 },
    { 26, 36, 3, CARD_FONT_3X5,  2,  5,  7, 11,  7 },
};

/* ------------------------------------------------------------------ */
/*  Fonts for the rank. One row per byte, the leftmost dot is the       */
/*  highest bit used. Order: 0 to 9, A, J, Q, K.                        */
/* ------------------------------------------------------------------ */
#define CARD_GLYPHS 14
#define CARD_GLYPH_DIGIT0 0
#define CARD_GLYPH_A     10
#define CARD_GLYPH_J     11
#define CARD_GLYPH_Q     12
#define CARD_GLYPH_K     13

#define CARD_FONT5_W 5
#define CARD_FONT5_H 7
static const uint8_t CARD_FONT5[CARD_GLYPHS][CARD_FONT5_H] = {
    { 0x0E, 0x11, 0x13, 0x15, 0x19, 0x11, 0x0E },   /* 0 */
    { 0x04, 0x0C, 0x04, 0x04, 0x04, 0x04, 0x0E },   /* 1 */
    { 0x0E, 0x11, 0x01, 0x02, 0x04, 0x08, 0x1F },   /* 2 */
    { 0x1E, 0x01, 0x01, 0x0E, 0x01, 0x01, 0x1E },   /* 3 */
    { 0x02, 0x06, 0x0A, 0x12, 0x1F, 0x02, 0x02 },   /* 4 */
    { 0x1F, 0x10, 0x1E, 0x01, 0x01, 0x11, 0x0E },   /* 5 */
    { 0x06, 0x08, 0x10, 0x1E, 0x11, 0x11, 0x0E },   /* 6 */
    { 0x1F, 0x01, 0x02, 0x04, 0x08, 0x08, 0x08 },   /* 7 */
    { 0x0E, 0x11, 0x11, 0x0E, 0x11, 0x11, 0x0E },   /* 8 */
    { 0x0E, 0x11, 0x11, 0x0F, 0x01, 0x02, 0x0C },   /* 9 */
    { 0x0E, 0x11, 0x11, 0x1F, 0x11, 0x11, 0x11 },   /* A */
    { 0x07, 0x02, 0x02, 0x02, 0x02, 0x12, 0x0C },   /* J */
    { 0x0E, 0x11, 0x11, 0x11, 0x15, 0x12, 0x0D },   /* Q */
    { 0x11, 0x12, 0x14, 0x18, 0x14, 0x12, 0x11 },   /* K */
};

#define CARD_FONT3_W 3
#define CARD_FONT3_H 5
static const uint8_t CARD_FONT3[CARD_GLYPHS][CARD_FONT3_H] = {
    { 7, 5, 5, 5, 7 },   /* 0 */
    { 2, 6, 2, 2, 7 },   /* 1 */
    { 7, 1, 7, 4, 7 },   /* 2 */
    { 7, 1, 7, 1, 7 },   /* 3 */
    { 5, 5, 7, 1, 1 },   /* 4 */
    { 7, 4, 7, 1, 7 },   /* 5 */
    { 7, 4, 7, 5, 7 },   /* 6 */
    { 7, 1, 2, 2, 2 },   /* 7 */
    { 7, 5, 7, 5, 7 },   /* 8 */
    { 7, 5, 7, 1, 7 },   /* 9 */
    { 2, 5, 7, 5, 5 },   /* A */
    { 1, 1, 1, 5, 2 },   /* J */
    { 2, 5, 5, 7, 3 },   /* Q */
    { 5, 5, 6, 5, 5 },   /* K */
};

/* ------------------------------------------------------------------ */
/*  Suit shapes, drawn on a 0 to 99 grid and sampled for any size       */
/* ------------------------------------------------------------------ */
#define SUIT_GRID        100
#define SUIT_SAMPLES     3      /* sub-samples across and down one pixel */
#define SUIT_SAMPLES_MIN 4      /* of 9 that must hit for a lit pixel    */

/* hearts: two circles on a triangle */
#define HEART_CIRCLE_Y   32
#define HEART_CIRCLE_R   27
#define HEART_LEFT_X     27
#define HEART_RIGHT_X    73
#define HEART_TRI_TOP    45
#define HEART_TRI_TIP    98
#define HEART_TRI_HALF   48     /* half the triangle's width at the top  */

/* spades: the heart upside down on a stem */
#define SPADE_BODY_H     82     /* the height the flipped heart is fitted to */
#define STEM_TOP         66
#define STEM_BOTTOM      98
#define STEM_HALF_TOP    2
#define STEM_HALF_BOTTOM 16

/* clubs: three circles on a stem */
#define CLUB_TOP_X       50
#define CLUB_TOP_Y       27
#define CLUB_SIDE_Y      60
#define CLUB_LEFT_X      26
#define CLUB_RIGHT_X     74
#define CLUB_R           24
#define CLUB_STEM_TOP    60

/* diamonds */
#define DIAMOND_HALF_W   42
#define DIAMOND_HALF_H   50

/* ------------------------------------------------------------------ */
/*  Backs                                                               */
/* ------------------------------------------------------------------ */
#define CARD_BACKS       4
#define BACK_INSET       3      /* white margin between the edge and the art */

#define BACK_BLUE        CARD_RGB(  0,  70, 220)
#define BACK_RED         CARD_RGB(210,   0,   0)
#define BACK_NAVY        CARD_RGB(  0,   0, 190)
#define BACK_PURPLE      CARD_RGB(170,   0, 210)
#define BACK_LINE        CARD_RGB(255, 255, 255)

#define BACK_LATTICE_STEP 4     /* distance between lattice lines        */
#define BACK_DOT_STEP     4     /* distance between dots                 */
#define BACK_STRIPE_WIDTH 3     /* width of one diagonal stripe          */
#define BACK_FRAME_GAP    2     /* frame distance from the art's edge    */

#endif /* CARDS_CONST_H */
