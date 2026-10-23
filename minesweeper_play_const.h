/*
 * minesweeper_play_const.h - the constants of the Minesweeper screen:
 * positions, rows of text, the texts and the keys.
 *
 * Author: Anthropic Claude
 */
#ifndef MINESWEEPER_PLAY_CONST_H
#define MINESWEEPER_PLAY_CONST_H

/* ------------------------------------------------------------------ */
/*  The board: the squares are as big as the screen allows, in the area  */
/*  above the two rows of text                                          */
/* ------------------------------------------------------------------ */
#define MP_BOARD_W        320   /* pixels of width the board may use       */
#define MP_BOARD_H        300   /* pixels of height the board may use      */
#define MP_CELL_MAX        40   /* a square never gets bigger than this    */

/* ------------------------------------------------------------------ */
/*  Rows of text                                                        */
/* ------------------------------------------------------------------ */
#define MP_ROW_HINT        30
#define MP_ROW_STATUS      31

/* the menu of boards */
#define MP_ROW_MENU_TITLE   5
#define MP_ROW_MENU_FIRST   9   /* each board takes 3 rows: the board, its best time, a blank */
#define MP_MENU_ROW_STEP    3
#define MP_ROW_MENU_HELP   22
#define MP_ROW_MENU_HELP_2 23

/* ------------------------------------------------------------------ */
/*  The custom board: what the player may ask for                       */
/* ------------------------------------------------------------------ */
#define MP_CUSTOM          3    /* the menu number of the custom board     */
#define MP_MENU_ITEMS      4
#define MP_CUSTOM_DEFAULT_W 16
#define MP_CUSTOM_DEFAULT_H 16
#define MP_CUSTOM_MINES_DIV  6  /* the mines asked for start at squares / this */

/* ------------------------------------------------------------------ */
/*  Texts                                                               */
/* ------------------------------------------------------------------ */
#define MP_TEXT_TITLE       "MINESWEEPER"
#define MP_TEXT_CUSTOM      "CUSTOM BOARD"
#define MP_TEXT_MENU_HELP   "UP/DOWN + ENTER, or 1 to 4"
#define MP_TEXT_MENU_HELP_2 "ESC: back"
#define MP_TEXT_ASK_W       "WIDTH"
#define MP_TEXT_ASK_H       "HEIGHT"
#define MP_TEXT_ASK_MINES   "MINES"

#define MP_HINT_PLAY    "ENTER open  SPACE/F flag  N new  ESC"
#define MP_HINT_WON     "YOU WIN!   N new  R same  ESC menu"
#define MP_HINT_LOST    "YOU LOST!  N new  R same  ESC menu"

/* keys */
#define MP_KEY_FLAG     'f'     /* flags a square, as SPACE does           */
#define MP_KEY_NEW      'n'     /* a new board of the same size            */
#define MP_KEY_RESTART  'r'     /* the same board again                    */

#endif /* MINESWEEPER_PLAY_CONST_H */
