/*
 * freecell_play_const.h - the constants of the FreeCell screen: positions,
 * colours, rows of text, the texts and the keys.
 *
 * Author: Anthropic Claude
 */
#ifndef FREECELL_PLAY_CONST_H
#define FREECELL_PLAY_CONST_H

/* ------------------------------------------------------------------ */
/*  The picture: all positions are in pixels, the card size is the       */
/*  34 x 48 size of cards.c (size number 1)                              */
/* ------------------------------------------------------------------ */
#define FC_CARD_SIZE        1
#define FC_TOP_Y            5   /* top of the row of cells and foundations */
#define FC_COLUMN_Y        62   /* top of the columns                      */
#define FC_BOTTOM_Y       298   /* nothing of the table is drawn below this */
#define FC_STRIP_HEIGHT    11   /* part of a covered card that shows       */
#define FC_STRIP_MIN        6   /* a long column is squeezed to this at most */
#define FC_FRAME            2   /* thickness of the cursor frame           */
#define FC_FRAME_PAD        2   /* frame distance from the card            */

#define FC_COL_FELT     RGB(  0, 110,  40)
#define FC_COL_CURSOR   RGB(255, 225,  70)    /* yellow                   */
#define FC_COL_PICKED   RGB(120, 225, 255)    /* cyan                     */
#define FC_COL_SLOT     RGB(255, 255, 255)

/* ------------------------------------------------------------------ */
/*  Rows of text and the texts                                          */
/* ------------------------------------------------------------------ */
#define FC_ROW_HINT        30
#define FC_ROW_STATUS      31
#define FC_ROW_WIN         14

#define FC_TEXT_TITLE   "FREECELL"
#define FC_TEXT_ASK_EXTRA "In the game: S speed  C card look"

#define FC_HINT_PLAY    "ENTER pick  1-8 a f jump  U undo  C look"
#define FC_HINT_PICKED  "ENTER drop  ESC cancel  + - run size"
#define FC_HINT_WON     "N new deal   R same deal   ESC back"
#define FC_TEXT_WIN     "YOU WIN!"
#define FC_TEXT_NO_MOVE "That move is not allowed"
#define FC_TEXT_PUT_BACK "Not allowed again: card put back"
#define FC_BAD_DROPS_LIMIT 2    /* refused drops in a row that put the card back */

/* keys */
#define FC_KEY_SPEED    's'     /* steps through the animation speeds      */
#define FC_KEY_UNDO     'u'
#define FC_KEY_NEW      'n'
#define FC_KEY_RESTART  'r'
#define FC_KEY_MORE     '+'
#define FC_KEY_MORE_2   '='
#define FC_KEY_LESS     '-'
#define FC_KEY_CELLS    'a'     /* jumps to the first empty free cell      */
#define FC_KEY_LOOK     'c'     /* the screen for choosing the card faces  */
#define FC_KEY_FOUND    'f'     /* jumps to the foundations                */

#endif /* FREECELL_PLAY_CONST_H */
