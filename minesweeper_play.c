/*
 * minesweeper_play.c - the Minesweeper screen and keys (see
 * minesweeper_play.h).
 *
 * A menu offers the three classic boards and a custom one. The squares are as
 * big as the screen allows for the board. A yellow frame is the cursor.
 *
 *   arrows ......... move the cursor (held down, they repeat)
 *   ENTER .......... open the square; on an open number with as many flags
 *                    round it as the number, open the squares round it
 *   SPACE or F ..... step the square through a flag, a question mark and none
 *   N .............. a new board of the same size;  R: the same board again
 *   F4 ............. end the game as a win, with the victory show (no best time)
 *   ESC ............ back to the menu of boards
 *
 * The timer starts with the first square opened. The best time of each of the
 * classic boards is kept until the PicoCalc is switched off.
 *
 * Author: Anthropic Claude
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "lcd.h"          /* WIDTH, GLYPH_HEIGHT, lcd_*()   */
#include "keyboard.h"     /* KEY_* codes                    */

#include "platform.h"
#include "mine_cells.h"
#include "minesweeper.h"
#include "minesweeper_play.h"
#include "ui.h"
#include "victory.h"
#include "games_const.h"
#include "minesweeper_play_const.h"

static ms_t     game;
static int      level;                  /* menu number of the board in play     */
static int      custom_w = MP_CUSTOM_DEFAULT_W;
static int      custom_h = MP_CUSTOM_DEFAULT_H;
static int      custom_mines;           /* 0 until the player has chosen         */
static int      best[MS_LEVELS];        /* best times in seconds, 0 for none     */
static int      cursor_x, cursor_y;
static int      cell_size, board_x, board_y;
static uint32_t start_ms;               /* when the first square was opened      */
static int      seconds;                /* the time shown                        */
static bool     counting;
static const char *status_note;         /* "NEW BEST TIME" after a win           */
static uint16_t pixels[MP_CELL_MAX * MP_CELL_MAX];
static uint16_t drawn[MS_MAX_CELLS];    /* what each square looked like last drawn */
static uint32_t seed_counter;

/* forward declarations (the public function is first) */
static int  choose_board(void);
static bool set_board(int choice);
static void start_board(unsigned long seed);
static void draw_everything(void);
static void draw_cell(int index, bool force);
static void draw_changes(void);
static void draw_text(void);
static uint16_t look_of(int index, int *kind, int *number);
static bool handle_key(uint8_t code);
static void move_cursor(int dx, int dy);
static unsigned long new_seed(void);

/* ===================================================================== */
/*  Public                                                                */
/* ===================================================================== */
void minesweeper_play(void)
{
    uint8_t code;

    for (;;) {
        int choice = choose_board();

        if (choice < 0)
            return;
        if (!set_board(choice))
            continue;
        ui_set_repeat(true);
        start_board(new_seed());
        draw_everything();
        ui_drain_keys();

        for (;;) {
            if (!ui_poll_key(&code)) {
                if (counting && (int)((plat_now_ms() - start_ms) / 1000) != seconds) {
                    seconds = (int)((plat_now_ms() - start_ms) / 1000);
                    if (seconds > 999)
                        seconds = 999;
                    draw_text();
                }
                plat_sleep_ms(POLL_MS);
                continue;
            }
            if (!handle_key(code))
                break;
            draw_changes();
            draw_text();
        }
        ui_set_repeat(false);
    }
}

/* a seed for the mines from the clock and a counter, so two boards made in the
 * same millisecond differ */
static unsigned long new_seed(void)
{
    seed_counter++;
    return (unsigned long)(plat_now_ms() * 2654435761UL) + seed_counter * 40503UL;
}

/* ===================================================================== */
/*  The menu of boards                                                    */
/* ===================================================================== */

/* returns 0 to 3 for the board chosen, or -1 when the player backs out */
static int choose_board(void)
{
    int sel = 0, i;
    uint8_t code;
    char line[TCOLS + 1];

    ui_set_background(COL_BG);
    for (;;) {
        lcd_clear_screen();
        ui_put_row(MP_ROW_MENU_TITLE, MP_TEXT_TITLE, COL_CYAN);
        for (i = 0; i < MP_MENU_ITEMS; i++) {
            int row = MP_ROW_MENU_FIRST + i * MP_MENU_ROW_STEP;
            uint16_t colour = i == sel ? COL_YELLOW : COL_WHITE;

            if (i < MS_LEVELS) {
                snprintf(line, sizeof line, "%d  %s %d x %d, %d mines", i + 1,
                         MS_LEVEL[i].name, MS_LEVEL[i].w, MS_LEVEL[i].h,
                         MS_LEVEL[i].mines);
                ui_put_row(row, line, colour);
                if (best[i] > 0)
                    snprintf(line, sizeof line, "best time %d seconds", best[i]);
                else
                    snprintf(line, sizeof line, "no best time yet");
                ui_put_row(row + 1, line, COL_CYAN);
            } else {
                snprintf(line, sizeof line, "%d  %s", i + 1, MP_TEXT_CUSTOM);
                ui_put_row(row, line, colour);
            }
        }
        ui_put_row(MP_ROW_MENU_HELP, MP_TEXT_MENU_HELP, COL_WHITE);
        ui_put_row(MP_ROW_MENU_HELP_2, MP_TEXT_MENU_HELP_2, COL_WHITE);
        ui_drain_keys();

        for (;;) {
            while (!ui_poll_key(&code))
                plat_sleep_ms(POLL_MS);
            if (code == KEY_ESC)
                return -1;
            if (code >= '1' && code < '1' + MP_MENU_ITEMS)
                return code - '1';
            if (code == KEY_ENTER || code == KEY_RETURN || code == ' ')
                return sel;
            if (code == KEY_UP || code == KEY_DOWN) {
                sel = (sel + (code == KEY_UP ? MP_MENU_ITEMS - 1 : 1)) % MP_MENU_ITEMS;
                break;                  /* draw the menu again */
            }
        }
    }
}

/* make ready the board of a menu choice; false when the custom board was
 * backed out of */
static bool set_board(int choice)
{
    level = choice;
    if (choice < MS_LEVELS)
        return true;
    custom_w = ui_ask_number(MP_TEXT_CUSTOM, MP_TEXT_ASK_W, MS_MIN_W, MS_MAX_W, custom_w);
    if (custom_w < 0)
        return false;
    custom_h = ui_ask_number(MP_TEXT_CUSTOM, MP_TEXT_ASK_H, MS_MIN_H, MS_MAX_H, custom_h);
    if (custom_h < 0)
        return false;
    {
        int max = (custom_w - 1) * (custom_h - 1);
        int def = custom_mines > 0 && custom_mines <= max ? custom_mines
                                                           : custom_w * custom_h / MP_CUSTOM_MINES_DIV;

        custom_mines = ui_ask_number(MP_TEXT_CUSTOM, MP_TEXT_ASK_MINES, 1, max, def);
    }
    return custom_mines > 0;
}

/* ===================================================================== */
/*  Starting a board                                                      */
/* ===================================================================== */
static void start_board(unsigned long seed)
{
    int w = level < MS_LEVELS ? MS_LEVEL[level].w : custom_w;
    int h = level < MS_LEVELS ? MS_LEVEL[level].h : custom_h;
    int m = level < MS_LEVELS ? MS_LEVEL[level].mines : custom_mines;
    int by_width = MP_BOARD_W / w, by_height = MP_BOARD_H / h;

    ms_new(&game, w, h, m, seed);
    cell_size = by_width < by_height ? by_width : by_height;
    if (cell_size > MP_CELL_MAX)
        cell_size = MP_CELL_MAX;
    board_x = (WIDTH - w * cell_size) / 2;
    board_y = (MP_BOARD_H - h * cell_size) / 2;
    cursor_x = w / 2;
    cursor_y = h / 2;
    counting = false;
    seconds = 0;
    status_note = NULL;
}

/* ===================================================================== */
/*  Keys                                                                  */
/* ===================================================================== */

/* returns false when the player leaves the board */
static bool handle_key(uint8_t code)
{
    int result;

    if (code == KEY_ESC)
        return false;
    if (code == MP_KEY_NEW || code == MP_KEY_NEW - 32) {
        start_board(new_seed());
        draw_everything();
        return true;
    }
    if (code == MP_KEY_RESTART || code == MP_KEY_RESTART - 32) {
        start_board(game.seed);
        draw_everything();
        return true;
    }
    switch (code) {
    case KEY_F4:                        /* end the game as a win, with the show */
        if (game.state == MS_PLAYING) {
            ms_solve(&game);
            counting = false;
            victory_flash(board_x, board_y, game.w * cell_size, game.h * cell_size);
            draw_everything();
        }
        break;
    case KEY_LEFT:  move_cursor(-1, 0); break;
    case KEY_RIGHT: move_cursor(1, 0);  break;
    case KEY_UP:    move_cursor(0, -1); break;
    case KEY_DOWN:  move_cursor(0, 1);  break;
    case ' ':
    case MP_KEY_FLAG:
    case MP_KEY_FLAG - 32:
        ms_mark(&game, cursor_x, cursor_y);
        break;
    case KEY_ENTER:
    case KEY_RETURN:
        result = ms_open(&game, cursor_x, cursor_y);
        if (game.started && !counting && game.state == MS_PLAYING) {
            counting = true;
            start_ms = plat_now_ms();
        }
        if (result == MS_RESULT_WON || result == MS_RESULT_LOST) {
            if (counting) {
                seconds = (int)((plat_now_ms() - start_ms) / 1000);
                if (seconds > 999)
                    seconds = 999;
            }
            counting = false;
            if (result == MS_RESULT_WON && level < MS_LEVELS &&
                (best[level] == 0 || seconds < best[level])) {
                best[level] = seconds > 0 ? seconds : 1;
                status_note = "NEW BEST TIME";
            }
            if (result == MS_RESULT_WON) {
                victory_flash(board_x, board_y, game.w * cell_size, game.h * cell_size);
                draw_everything();
            }
        }
        break;
    default:
        break;
    }
    return true;
}

static void move_cursor(int dx, int dy)
{
    int nx = cursor_x + dx, ny = cursor_y + dy;

    if (nx >= 0 && nx < game.w)
        cursor_x = nx;
    if (ny >= 0 && ny < game.h)
        cursor_y = ny;
}

/* ===================================================================== */
/*  Drawing                                                               */
/* ===================================================================== */

/* what a square looks like now: a code that changes whenever its picture does */
static uint16_t look_of(int index, int *kind, int *number)
{
    uint8_t c = game.cell[index];
    bool over = game.state == MS_LOST;

    *number = 0;
    if (over && index == game.hit) {
        *kind = MINE_CELL_HIT;
    } else if (over && (c & MS_MINE) && !(c & MS_FLAG)) {
        *kind = MINE_CELL_MINE;
    } else if (over && (c & MS_FLAG) && !(c & MS_MINE)) {
        *kind = MINE_CELL_WRONG;
    } else if (c & MS_OPEN) {
        *kind = MINE_CELL_OPEN;
        *number = game.near[index];
    } else if (c & MS_FLAG) {
        *kind = MINE_CELL_FLAG;
    } else if (c & MS_QUESTION) {
        *kind = MINE_CELL_QUESTION;
    } else {
        *kind = MINE_CELL_COVERED;
    }
    return (uint16_t)(*kind * 16 + *number + 1);
}

/* draw one square if it looks different from when it was last drawn */
static void draw_cell(int index, bool force)
{
    int kind, number, x = index % game.w, y = index / game.w;
    bool cursor = x == cursor_x && y == cursor_y;
    uint16_t code = (uint16_t)(look_of(index, &kind, &number) * 2 + (cursor ? 1 : 0));

    if (!force && drawn[index] == code)
        return;
    drawn[index] = code;
    mine_draw_cell(pixels, cell_size, kind, number, cursor);
    lcd_blit(pixels, (uint16_t)(board_x + x * cell_size),
             (uint16_t)(board_y + y * cell_size), (uint16_t)cell_size,
             (uint16_t)cell_size);
}

/* draw the squares that changed */
static void draw_changes(void)
{
    int i;

    for (i = 0; i < game.w * game.h; i++)
        draw_cell(i, false);
}

static void draw_text(void)
{
    char line[TCOLS + 1];

    if (status_note)
        snprintf(line, sizeof line, "MINES %d   TIME %d   %s", ms_mines_left(&game),
                 seconds, status_note);
    else
        snprintf(line, sizeof line, "MINES %d   TIME %d   %d x %d", ms_mines_left(&game),
                 seconds, game.w, game.h);
    ui_put_row(MP_ROW_STATUS, line, COL_WHITE);
    ui_put_row(MP_ROW_HINT, game.state == MS_WON ? MP_HINT_WON
                          : game.state == MS_LOST ? MP_HINT_LOST : MP_HINT_PLAY,
               game.state == MS_PLAYING ? COL_CYAN : COL_YELLOW);
}

static void draw_everything(void)
{
    int i;

    ui_set_background(COL_BG);
    lcd_clear_screen();
    for (i = 0; i < game.w * game.h; i++)
        draw_cell(i, true);
    draw_text();
}
