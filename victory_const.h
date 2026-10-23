/*
 * victory_const.h - the constants of the victory celebration: the fanfare, the
 * flashing of Minesweeper and the falling cards of the card games.
 *
 * Author: Anthropic Claude
 */
#ifndef VICTORY_CONST_H
#define VICTORY_CONST_H

/* ------------------------------------------------------------------ */
/*  The fanfare: a rising chord and a long top note. A pitch of 0 is a   */
/*  rest.                                                                */
/* ------------------------------------------------------------------ */
typedef struct {
    int hz;
    int ms;
} victory_note_t;

#define VICTORY_NOTE_COUNT 7
static const victory_note_t VICTORY_NOTES[VICTORY_NOTE_COUNT] = {
    { 523, 110 },       /* C5 */
    { 659, 110 },       /* E5 */
    { 784, 110 },       /* G5 */
    {   0,  40 },
    { 784,  90 },       /* G5 */
    {   0,  30 },
    { 1047, 520 },      /* C6, held */
};

/* ------------------------------------------------------------------ */
/*  Flashing (the whole board changes colour in turn)                    */
/* ------------------------------------------------------------------ */
#define VICTORY_FLASHES      8
#define VICTORY_FLASH_MS   110
#define VICTORY_FLASH_COL_A  RGB( 90, 235, 130)     /* green  */
#define VICTORY_FLASH_COL_B  RGB(255, 225,  70)     /* yellow */

/* ------------------------------------------------------------------ */
/*  The falling cards: each one starts on its foundation, falls, bounces */
/*  and leaves a trail as it crosses the screen                          */
/* ------------------------------------------------------------------ */
#define VICTORY_CARDS        52
#define VICTORY_FRAME_MS      6     /* wait after each frame, so the cascade is slow enough to watch */
#define VICTORY_GRAVITY       1     /* pixels a frame, added to the fall each frame */
#define VICTORY_BOUNCE_NUM    3     /* a bounce keeps 3/4 of the speed          */
#define VICTORY_BOUNCE_DEN    4
#define VICTORY_BOUNCE_MIN    3     /* least speed back up, so a card never stops */
#define VICTORY_VX_MIN        2     /* sideways pixels a frame: this much...    */
#define VICTORY_VX_SPAN       4     /* ...plus up to this much less one          */
#define VICTORY_KEY_EVERY     8     /* frames between looks at the keyboard     */
#define VICTORY_RAND_MUL  1664525UL
#define VICTORY_RAND_ADD  1013904223UL
#define VICTORY_RAND_SHIFT    16

#endif /* VICTORY_CONST_H */
