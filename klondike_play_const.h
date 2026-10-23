/*
 * klondike_play_const.h - the constants of the Klondike screen: positions,
 * colours, rows of text, the texts and the keys.
 *
 * Author: Anthropic Claude
 */
#ifndef KLONDIKE_PLAY_CONST_H
#define KLONDIKE_PLAY_CONST_H

/* ------------------------------------------------------------------ */
/*  The picture: all positions are in pixels, the card size is the       */
/*  34 x 48 size of cards.c (size number 1). 7 columns share the width;  */
/*  the stock is over the first, the waste over the second (fanned over  */
/*  the third when 3 cards are drawn) and the foundations over the last  */
/*  four.                                                                */
/* ------------------------------------------------------------------ */
#define KL_CARD_SIZE        1
#define KL_BACK_DESIGN      2   /* the back of the cards that are face down */
#define KL_SLOTS            7   /* places across the table                 */
#define KL_SLOT_WASTE       1
#define KL_SLOT_FOUND0      3
#define KL_TOP_Y            5   /* top of the row with stock and foundations */
#define KL_COLUMN_Y        62   /* top of the columns                      */
#define KL_BOTTOM_Y       298   /* nothing of the table is drawn below this */
#define KL_STEP_UP         11   /* part of a covered face-up card that shows */
#define KL_STEP_DOWN        6   /* part of a face-down card that shows     */
#define KL_STEP_MIN         6   /* a long column is squeezed to this at most */
#define KL_FAN              15  /* distance between the waste cards when drawing 3 */
#define KL_FAN_CARDS        3
#define KL_FRAME            2   /* thickness of the cursor frame           */

#define KL_COL_FELT     RGB(  0, 110,  40)
#define KL_COL_CURSOR   RGB(255, 225,  70)    /* yellow                   */
#define KL_COL_PICKED   RGB(120, 225, 255)    /* cyan                     */

/* ------------------------------------------------------------------ */
/*  Rows of text and the texts                                          */
/* ------------------------------------------------------------------ */
#define KL_ROW_HINT        30
#define KL_ROW_STATUS      31
#define KL_ROW_WIN         14

#define KL_TEXT_TITLE   "KLONDIKE"
#define KL_TEXT_ASK_EXTRA "In the game: S speed  C card look"
#define KL_TEXT_DRAW_LABEL "DRAW"
#define KL_DRAW_CHOICES    2    /* 1 card, 3 cards                         */
static const char *const KL_DRAW_NAMES[KL_DRAW_CHOICES] = { "1 CARD", "3 CARDS" };
static const int KL_DRAW_CARDS[KL_DRAW_CHOICES] = { 1, 3 };

#define KL_HINT_PLAY    "ENTER pick/draw  F foundation  U undo"
#define KL_HINT_PICKED  "ENTER drop  F foundation  ESC cancel"
#define KL_HINT_WON     "N new deal   R same deal   ESC back"
#define KL_TEXT_WIN     "YOU WIN!"
#define KL_TEXT_NO_MOVE "That move is not allowed"
#define KL_TEXT_PUT_BACK "Not allowed again: card put back"
#define KL_TEXT_NO_PICK "Nothing to pick up there"
#define KL_TEXT_FINISHING "Every card is face up: finishing"
#define KL_TEXT_NO_STOCK "Stock and waste are empty"
#define KL_BAD_DROPS_LIMIT 2    /* refused drops in a row that put the card back */

/* keys */
#define KL_KEY_LOOK     'c'     /* the screen for choosing the card faces  */
#define KL_KEY_SPEED    's'     /* steps through the animation speeds      */
#define KL_KEY_UNDO     'u'
#define KL_KEY_NEW      'n'
#define KL_KEY_RESTART  'r'
#define KL_KEY_FOUND    'f'     /* send the card to its foundation          */
#define KL_KEY_STOCK    'd'     /* jump to the stock                       */
#define KL_KEY_WASTE    'w'     /* jump to the waste                       */

#endif /* KLONDIKE_PLAY_CONST_H */
