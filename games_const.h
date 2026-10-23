/*
 * games_const.h - every constant of PicoCalc Games: version, colours, limits,
 * screen positions and tables. games.c holds only code, so a new constant or
 * a moved row goes here. The constants of the card drawing are in
 * cards_const.h.
 *
 * Author: Anthropic Claude
 */
#ifndef GAMES_CONST_H
#define GAMES_CONST_H

/* Shown on the splash screen; the firmware build reads it from this line, so
 * keep it in the form  #define VERSION "V0.01A"  on one line. */
#define VERSION "V0.01A"

/* ------------------------------------------------------------------ */
/*  Colours (full-strength only: dim greys are hard to read on the LCD) */
/* ------------------------------------------------------------------ */
#define COL_BG      RGB(  0,   0,   0)
#define COL_TABLE   RGB(  0, 110,  40)  /* the green of the card table     */
#define COL_WHITE   RGB(255, 255, 255)
#define COL_GREEN   RGB( 90, 235, 130)
#define COL_YELLOW  RGB(255, 225,  70)
#define COL_CYAN    RGB(120, 225, 255)

/* ------------------------------------------------------------------ */
/*  Screen geometry: the LCD is a 40 x 32 grid of 8 x 10 pixel cells    */
/* ------------------------------------------------------------------ */
#define TCOLS    40

/* ------------------------------------------------------------------ */
/*  Splash screen rows                                                  */
/* ------------------------------------------------------------------ */
#define SPLASH_ROW_TITLE     8
#define SPLASH_ROW_VERSION  10
#define SPLASH_ROW_AUTHOR   12
#define SPLASH_TEXT_AUTHOR  "Author: Anthropic CLAUDE"
#define SPLASH_ROW_MENU     16      /* the first choice; they stack downwards  */
#define SPLASH_MENU_STEP     2      /* rows from one choice to the next        */
#define SPLASH_MENU_COL      9      /* the choices start at this column        */
#define SPLASH_MENU_CHOICES  3
static const char *const SPLASH_MENU_TEXT[SPLASH_MENU_CHOICES] = {
    "F: FREECELL",
    "K: KLONDIKE SOLITAIRE",
    "M: MINESWEEPER",
};
#define SPLASH_ROW_EXIT_HINT 25
#define SPLASH_ROW_EXIT     27
#define ROW_NOTICE          31  /* the BOOTSEL notice                      */

/* ------------------------------------------------------------------ */
/*  The screen that asks for a deal number (ui.c)                       */
/* ------------------------------------------------------------------ */
#define ASK_ROW_TITLE      8
#define ASK_ROW_PROMPT    12
#define ASK_ROW_ENTRY     14
#define ASK_ROW_OPTION    16
#define ASK_ROW_HELP      18    /* three rows of help, then the extra one */
#define ASK_TEXT_PROMPT   "DEAL NUMBER (1 TO 32000)"
#define ASK_TEXT_HELP_1   "Type digits, then ENTER"
#define ASK_TEXT_HELP_2   "ENTER alone: a random deal"
#define ASK_TEXT_HELP_3   "ESC: back"
#define ASK_NUMBER_DIGITS 3     /* digits ui_ask_number takes               */
#define ASK_KEY_OPTION    't'   /* steps through the options of the game  */
#define DEAL_MIN          1
#define DEAL_MAX      32000
#define DEAL_DIGITS       5
#define DEAL_RANDOM_MUL   2654435761UL  /* spreads the clock into a number */
#define DEAL_RANDOM_SHIFT 8

/* ------------------------------------------------------------------ */
/*  The card chooser: one row of eight cards for each of the three face  */
/*  layouts of the 34 x 48 size. UP and DOWN move a marker over the rows */
/*  and ENTER chooses the one the games use.                             */
/* ------------------------------------------------------------------ */
#define CHOOSER_SIZE         1      /* card size number: 34 x 48                */
#define CHOOSER_ACROSS       8      /* cards side by side                      */
#define CHOOSER_FACES        3      /* rows: the face styles 0, 1 and 2 of cards.h */
#define CHOOSER_TOP         70      /* y of the first row of cards             */
#define CHOOSER_ROW_GAP     16      /* pixels between two rows of cards        */
#define CHOOSER_FRAME        2      /* thickness of a frame round a row        */
#define CHOOSER_FRAME_PAD    2      /* distance of the frame from the cards    */
#define CHOOSER_ROW_TITLE    0
#define CHOOSER_ROW_LEGEND   1
#define CHOOSER_ROW_HINT    31
#define CHOOSER_TEXT_TITLE  "CHOOSE THE CARD FACE"
#define CHOOSER_TEXT_LEGEND "Yellow: marker   Green: used in games"
#define CHOOSER_TEXT_HINT   "UP/DOWN + ENTER: choose   ESC: back"
#define SETTING_FACE_STYLE_DEFAULT 0    /* the face used until one is chosen */
#define COL_MARK_CURSOR   RGB(255, 225,  70)    /* yellow: the marker       */
#define COL_MARK_CHOSEN   RGB( 90, 235, 130)    /* green: used in the games */

/* The eight cards shown in every row: rank 0 (ace) to 12 (king), suit 0
 * spades, 1 hearts, 2 diamonds, 3 clubs. */
typedef struct { int rank, suit; } card_t;

static const card_t CHOOSER_CARDS[CHOOSER_ACROSS] = {
    { 0, 0 }, { 12, 1 }, { 11, 2 }, { 10, 3 },
    { 9, 0 }, {  8, 1 }, {  6, 2 }, {  1, 3 },
};

/* ------------------------------------------------------------------ */
/*  Animation speeds. Shared by the games; the player steps through them with the S key.      */
/*  steps: frames of a card flying to its place (0: none); auto_steps:   */
/*  the same for a card that goes home by itself; frame_ms: wait after   */
/*  each frame; pause_ms: wait before such a card starts to fly.         */
/* ------------------------------------------------------------------ */
typedef struct {
    const char *name;
    int steps, auto_steps, frame_ms, pause_ms;
} anim_speed_t;

#define ANIM_SPEEDS         4
#define ANIM_SPEED_DEFAULT  1             /* the one used at the start (FAST) */
static const anim_speed_t ANIM_SPEED[ANIM_SPEEDS] = {
    { "OFF",     0,  0,  0,   0 },
    { "FAST",    6,  4,  0,   0 },
    { "NORMAL", 10,  9, 20, 150 },
    { "SLOW",   14, 13, 40, 300 },
};

/* the picture of a run of cards in flight: room for 13 cards of the biggest
 * size with a strip of 11 pixels each (cardfly.c) */
#define CARDFLY_SPRITE_PIXELS  (38 * (53 + 12 * 11))

/* ------------------------------------------------------------------ */
/*  Timing                                                              */
/* ------------------------------------------------------------------ */
#define POLL_MS         3       /* pause between keyboard polls          */
#define UI_REPEAT_DELAY_MS 400  /* an arrow key starts repeating after this */
#define UI_REPEAT_MS    100     /* then repeats at this interval           */
#define REBOOT_MS     300       /* time the BOOTSEL notice stays visible */

#endif /* GAMES_CONST_H */
