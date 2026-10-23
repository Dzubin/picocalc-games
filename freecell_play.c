/*
 * freecell_play.c - the FreeCell screen and keys (see freecell_play.h).
 *
 * The table has the 4 free cells and the 4 foundations in the top row and the
 * 8 columns below. A cursor (a yellow frame) moves over the 16 spots; ENTER
 * picks up the card or run under it (cyan frame) and ENTER again drops it on
 * the spot the cursor has moved to.
 *
 *   arrows ......... move the cursor
 *   1 to 8 ......... jump to a column;  A: the first free cell that is empty;
 *                    F: the foundations
 *   C .............. choose the look of the cards
 *   ENTER, SPACE ... pick up / drop
 *   + and - ........ one more / one fewer card in the run picked up (used
 *                    for an empty column; on a column that is not empty the
 *                    cards that fit are found by themselves)
 *   S .............. step through the animation speeds (off, fast, normal, slow)
 *   U .............. undo;  R: same deal again;  N: another deal
 *   F4 ............. end the game as a win, with the victory show
 *   ESC ............ drop what is picked up; with nothing picked, leave
 *
 * Author: Anthropic Claude
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lcd.h"          /* WIDTH, GLYPH_HEIGHT, lcd_*()   */
#include "keyboard.h"     /* KEY_* codes                    */

#include "platform.h"
#include "cards.h"
#include "cardfly.h"
#include "victory.h"
#include "chooser.h"
#include "settings.h"
#include "ui.h"
#include "freecell.h"
#include "freecell_play.h"
#include "games_const.h"
#include "freecell_play_const.h"

static fc_t     game;
static int      cursor;             /* the spot the cursor is on            */
static int      picked;             /* the spot picked up from, or -1       */
static int      pick_count;         /* cards picked up                      */
static bool     won;
static int      bad_drops;          /* refused drops in a row               */
static char     speed_text[TCOLS + 1];
static const char *message;         /* shown on the hint row until a key    */
static bool     dirty[FC_SPOTS];
static uint16_t pixels[CARDS_MAX_PIXELS];
static int      card_w, card_h, gap;
static bool     animating;          /* a card is flying: no frames drawn    */
static int      hide_spot = -1;     /* a spot drawn without its last cards  */
static int      hide_count;

/* forward declarations (the public function is first) */
static void start_deal(long deal);
static void draw_everything(void);
static void draw_spot(int spot);
static void draw_text(void);
static void flush(void);
static void mark(int spot);
static void mark_all(void);
static int  slot_of(int spot);
static int  spot_x(int spot);
static int  column_step(int count);
static void draw_frame(int x, int y, int w, int h, uint16_t colour);
static void move_cursor(int dx, int dy);
static void pick_or_drop(void);
static void celebrate(void);
static int  first_empty_cell(void);
static void end_game(void);
static void change_run(int delta);
static bool handle_key(uint8_t code);
static void animate(int from, int to, int count, bool automatic);
static void redraw_near(int x, int y, int w, int h);
static int  shown_size(int spot);
static uint8_t shown_top(int spot);

/* ===================================================================== */
/*  Public                                                                */
/* ===================================================================== */
void freecell_play(void)
{
    long deal = ui_ask_deal(FC_TEXT_TITLE, FC_TEXT_ASK_EXTRA, NULL, NULL, 0, NULL);
    uint8_t code;

    if (deal < 0)
        return;
    ui_set_repeat(true);
    card_w = cards_width(FC_CARD_SIZE);
    card_h = cards_height(FC_CARD_SIZE);
    gap = (WIDTH - FC_COLUMNS * card_w) / (FC_COLUMNS + 1);
    start_deal(deal);
    draw_everything();
    ui_drain_keys();

    for (;;) {
        if (!ui_poll_key(&code)) {
            plat_sleep_ms(POLL_MS);
            continue;
        }
        message = NULL;
        if (!handle_key(code)) {
            ui_set_repeat(false);
            return;
        }
        flush();
    }
}

/* ===================================================================== */
/*  Starting a deal                                                       */
/* ===================================================================== */

static void start_deal(long deal)
{
    fc_deal(&game, deal);
    cursor = FC_SPOT_COL0;
    picked = -1;
    pick_count = 0;
    won = false;
    message = NULL;
}

/* ===================================================================== */
/*  Keys                                                                  */
/* ===================================================================== */

/* returns false when the game is to be left */
static bool handle_key(uint8_t code)
{
    int old = cursor;

    if (code == KEY_ESC) {
        if (picked < 0)
            return false;
        mark(picked);
        picked = -1;
        return true;
    }
    if (code == FC_KEY_NEW || code == FC_KEY_NEW - 32) {
        long deal = ui_ask_deal(FC_TEXT_TITLE, FC_TEXT_ASK_EXTRA, NULL, NULL, 0, NULL);

        if (deal >= 0)
            start_deal(deal);
        draw_everything();
        return true;
    }
    if (code == FC_KEY_RESTART || code == FC_KEY_RESTART - 32) {
        start_deal(game.deal);
        draw_everything();
        return true;
    }
    if (code == FC_KEY_SPEED || code == FC_KEY_SPEED - 32) {
        snprintf(speed_text, sizeof speed_text, "Animation: %s", cardfly_next_speed());
        message = speed_text;
        return true;
    }
    if (won)
        return true;

    switch (code) {
    case KEY_F4:    end_game();         break;
    case FC_KEY_LOOK:
    case FC_KEY_LOOK - 32:
        card_chooser();
        draw_everything();
        break;
    case KEY_LEFT:  move_cursor(-1, 0); break;
    case KEY_RIGHT: move_cursor(1, 0);  break;
    case KEY_UP:    move_cursor(0, -1); break;
    case KEY_DOWN:  move_cursor(0, 1);  break;
    case KEY_ENTER:
    case KEY_RETURN:
    case ' ':       pick_or_drop();     break;
    case FC_KEY_MORE:
    case FC_KEY_MORE_2: change_run(1);  break;
    case FC_KEY_LESS:   change_run(-1); break;
    case FC_KEY_UNDO:
    case FC_KEY_UNDO - 32:
        if (fc_undo(&game)) {
            picked = -1;
            mark_all();
        }
        break;
    default:
        if (code >= '1' && code <= '0' + FC_COLUMNS)
            cursor = FC_SPOT_COL0 + (code - '1');
        else if (code == FC_KEY_CELLS || code == FC_KEY_CELLS - 32)
            cursor = first_empty_cell();
        else if (code == FC_KEY_FOUND)
            cursor = FC_SPOT_FOUND0;
        break;
    }
    if (cursor != old) {
        mark(old);
        mark(cursor);
    }
    return true;
}

static void move_cursor(int dx, int dy)
{
    int slot = slot_of(cursor);
    bool top = cursor < FC_SPOT_COL0;

    slot = (slot + dx + FC_COLUMNS) % FC_COLUMNS;
    if (dy < 0)
        top = true;
    else if (dy > 0)
        top = false;
    cursor = top ? slot : FC_SPOT_COL0 + slot;
}

static void pick_or_drop(void)
{
    int count, from, to;

    if (picked < 0) {
        if (cursor >= FC_SPOT_FOUND0 && cursor < FC_SPOT_COL0) {
            message = "Cards on a foundation stay there";
        } else if (fc_size(&game, cursor) == 0) {
            message = "Nothing to pick up there";
        } else {
            picked = cursor;
            pick_count = 1;
            bad_drops = 0;
            mark(picked);
        }
        return;
    }
    if (cursor == picked) {                     /* the same spot: put it back */
        mark(picked);
        picked = -1;
        return;
    }
    count = (cursor >= FC_SPOT_COL0 && fc_size(&game, cursor) > 0) ? 0 : pick_count;
    from = picked;
    to = cursor;
    if (to >= FC_SPOT_FOUND0 && to < FC_SPOT_COL0)      /* its own suit's pile */
        to = FC_SPOT_FOUND0 + FC_SUIT(fc_top(&game, from));
    count = fc_move(&game, from, cursor, count);
    if (count == 0) {
        if (++bad_drops >= FC_BAD_DROPS_LIMIT) {    /* maybe a different card */
            mark(picked);
            picked = -1;
            message = FC_TEXT_PUT_BACK;
        } else {
            message = FC_TEXT_NO_MOVE;
        }
        return;
    }
    picked = -1;
    animate(from, to, count, false);
    while (fc_auto_step(&game, &from, &to, &count))
        animate(from, to, count, true);
    won = fc_won(&game);
    if (won)
        celebrate();
    mark_all();
}

/* the first free cell with nothing in it (the first cell when all are full) */
static int first_empty_cell(void)
{
    int i;

    for (i = 0; i < FC_CELLS; i++)
        if (fc_size(&game, FC_SPOT_CELL0 + i) == 0)
            return FC_SPOT_CELL0 + i;
    return FC_SPOT_CELL0;
}

/* the show for a win: the cards fall from the foundations */
static void celebrate(void)
{
    int start_x[FC_FOUNDATIONS], s;

    for (s = 0; s < FC_FOUNDATIONS; s++)
        start_x[s] = spot_x(FC_SPOT_FOUND0 + s);
    victory_cascade(FC_CARD_SIZE, settings_face_style(), start_x, FC_TOP_Y);
    draw_everything();
}

/* F4: end the game as a win at once, with the show: every card goes to its
 * foundation */
static void end_game(void)
{
    int s;

    picked = -1;
    memset(game.length, 0, sizeof game.length);
    memset(game.cell, FC_NO_CARD, sizeof game.cell);
    for (s = 0; s < FC_FOUNDATIONS; s++)
        game.found[s] = 13;
    won = true;
    celebrate();
}

static void change_run(int delta)
{
    int run;

    if (picked < FC_SPOT_COL0)
        return;
    run = fc_run_length(&game, picked);
    if (pick_count + delta >= 1 && pick_count + delta <= run) {
        pick_count += delta;
        mark(picked);
    }
}

/* ===================================================================== */
/*  Drawing                                                               */
/* ===================================================================== */
static void mark(int spot)  { dirty[spot] = true; }

static void mark_all(void)
{
    int i;

    for (i = 0; i < FC_SPOTS; i++)
        dirty[i] = true;
}

static void flush(void)
{
    int i;

    for (i = 0; i < FC_SPOTS; i++) {
        if (dirty[i]) {
            draw_spot(i);
            dirty[i] = false;
        }
    }
    draw_text();
}

/* the cells and foundations share the 8 places of the top row with the
 * 8 columns below them */
static int slot_of(int spot)
{
    return spot < FC_SPOT_COL0 ? spot : spot - FC_SPOT_COL0;
}

static int spot_x(int spot)
{
    return gap + slot_of(spot) * (card_w + gap);
}

/* the distance from one card of a column to the next: a long column is
 * squeezed so that it still ends above the text rows */
static int column_step(int count)
{
    int room = FC_BOTTOM_Y - FC_COLUMN_Y - card_h;
    int step = FC_STRIP_HEIGHT;

    if (count > 1 && (count - 1) * step > room)
        step = room / (count - 1);
    return step < FC_STRIP_MIN ? FC_STRIP_MIN : step;
}

static void draw_frame(int x, int y, int w, int h, uint16_t colour)
{
    lcd_solid_rectangle(colour, (uint16_t)(x - FC_FRAME), (uint16_t)(y - FC_FRAME),
                        (uint16_t)(w + 2 * FC_FRAME), FC_FRAME);
    lcd_solid_rectangle(colour, (uint16_t)(x - FC_FRAME), (uint16_t)(y + h),
                        (uint16_t)(w + 2 * FC_FRAME), FC_FRAME);
    lcd_solid_rectangle(colour, (uint16_t)(x - FC_FRAME), (uint16_t)y, FC_FRAME,
                        (uint16_t)h);
    lcd_solid_rectangle(colour, (uint16_t)(x + w), (uint16_t)y, FC_FRAME,
                        (uint16_t)h);
}

/* draw one spot with whatever lies on it, and the frame if it has one */
static void draw_spot(int spot)
{
    int x = spot_x(spot);
    int margin = FC_FRAME + FC_FRAME_PAD;
    int y, last_y, first_y;
    int n = shown_size(spot);

    if (spot < FC_SPOT_COL0) {
        uint8_t c = shown_top(spot);

        y = last_y = first_y = FC_TOP_Y;
        lcd_solid_rectangle(FC_COL_FELT, (uint16_t)(x - gap / 2), (uint16_t)(y - margin),
                            (uint16_t)(card_w + gap),
                            (uint16_t)(FC_COLUMN_Y - y));
        if (c == FC_NO_CARD)
            cards_draw_slot(pixels, FC_CARD_SIZE,
                            spot >= FC_SPOT_FOUND0 ? spot - FC_SPOT_FOUND0 : -1);
        else
            cards_draw_face(pixels, FC_CARD_SIZE, settings_face_style(), FC_RANK(c), FC_SUIT(c));
        lcd_blit(pixels, (uint16_t)x, (uint16_t)y, (uint16_t)card_w, (uint16_t)card_h);
    } else {
        const uint8_t *cards = game.column[spot - FC_SPOT_COL0];
        int step = column_step(n), i;

        y = FC_COLUMN_Y;
        lcd_solid_rectangle(FC_COL_FELT, (uint16_t)(x - gap / 2), (uint16_t)(y - margin),
                            (uint16_t)(card_w + gap),
                            (uint16_t)(FC_ROW_HINT * GLYPH_HEIGHT - (y - margin)));
        if (n == 0) {
            cards_draw_slot(pixels, FC_CARD_SIZE, -1);
            lcd_blit(pixels, (uint16_t)x, (uint16_t)y, (uint16_t)card_w, (uint16_t)card_h);
        }
        for (i = 0; i < n; i++) {
            int cy = FC_COLUMN_Y + i * step;

            if (i < n - 1) {
                cards_draw_strip(pixels, FC_CARD_SIZE, FC_RANK(cards[i]), FC_SUIT(cards[i]));
                if (i > 0)
                    cards_lay_on_card(pixels, FC_CARD_SIZE);
                lcd_blit(pixels, (uint16_t)x, (uint16_t)cy, (uint16_t)card_w, (uint16_t)step);
            } else {
                cards_draw_face(pixels, FC_CARD_SIZE, settings_face_style(), FC_RANK(cards[i]),
                                FC_SUIT(cards[i]));
                if (i > 0)
                    cards_lay_on_card(pixels, FC_CARD_SIZE);
                lcd_blit(pixels, (uint16_t)x, (uint16_t)cy, (uint16_t)card_w, (uint16_t)card_h);
            }
        }
        last_y = n ? FC_COLUMN_Y + (n - 1) * step : FC_COLUMN_Y;
        first_y = (spot == picked && n >= pick_count)
                      ? FC_COLUMN_Y + (n - pick_count) * step : last_y;
    }

    if (animating)
        return;
    if (spot == picked)
        draw_frame(x, first_y, card_w, last_y + card_h - first_y, FC_COL_PICKED);
    else if (spot == cursor)
        draw_frame(x, last_y, card_w, card_h, FC_COL_CURSOR);
}

/* what is on a spot as drawn: a spot may be drawn without the cards that are
 * still flying to it */
static int shown_size(int spot)
{
    int n = fc_size(&game, spot);

    return spot == hide_spot ? n - hide_count : n;
}

static uint8_t shown_top(int spot)
{
    if (spot == hide_spot && spot >= FC_SPOT_FOUND0 && spot < FC_SPOT_COL0) {
        int suit = spot - FC_SPOT_FOUND0;
        int cards = game.found[suit] - hide_count;

        return cards > 0 ? (uint8_t)((cards - 1) * 4 + suit) : FC_NO_CARD;
    }
    if (spot == hide_spot)
        return FC_NO_CARD;
    return fc_top(&game, spot);
}

/* draw again every spot that the rectangle touches */
static void redraw_near(int x, int y, int w, int h)
{
    int spot, margin = FC_FRAME + FC_FRAME_PAD;

    for (spot = 0; spot < FC_SPOTS; spot++) {
        int sx = spot_x(spot) - gap / 2;
        int top = spot < FC_SPOT_COL0 ? FC_TOP_Y - margin : FC_COLUMN_Y - margin;
        int bottom = spot < FC_SPOT_COL0 ? FC_COLUMN_Y : FC_ROW_HINT * GLYPH_HEIGHT;

        if (x < sx + card_w + gap && x + w > sx && y < bottom && y + h > top)
            draw_spot(spot);
    }
}

/* Show count cards flying from one spot to another. The move has already been
 * made in the game, so the cards are the last ones of the destination; the
 * screen still shows them on the source, and the spots are drawn without them
 * until they land. */
static void animate(int from, int to, int count, bool automatic)
{
    uint8_t moved[FC_MAX_COLUMN];
    int dest_n = fc_size(&game, to), src_n = fc_size(&game, from);
    int pitch, sx, sy, ex, ey, i;

    if (!cardfly_enabled())
        return;
    if (to >= FC_SPOT_COL0) {
        for (i = 0; i < count; i++)
            moved[i] = game.column[to - FC_SPOT_COL0][dest_n - count + i];
    } else {
        moved[0] = fc_top(&game, to);
    }
    pitch = from >= FC_SPOT_COL0 ? column_step(src_n + count) : FC_STRIP_HEIGHT;
    sx = spot_x(from);
    sy = from >= FC_SPOT_COL0 ? FC_COLUMN_Y + src_n * pitch : FC_TOP_Y;
    ex = spot_x(to);
    ey = to >= FC_SPOT_COL0 ? FC_COLUMN_Y + (dest_n - count) * column_step(dest_n)
                            : FC_TOP_Y;
    animating = true;
    hide_spot = to;
    hide_count = count;
    draw_spot(from);
    draw_spot(to);
    cardfly(moved, count, FC_CARD_SIZE, settings_face_style(), pitch, sx, sy, ex, ey,
            automatic, redraw_near);
    hide_spot = -1;
    animating = false;
    draw_spot(from);
    draw_spot(to);
}

static void draw_text(void)
{
    char line[TCOLS + 1];
    int i, free_cells = 0;

    for (i = 0; i < FC_CELLS; i++)
        if (fc_size(&game, FC_SPOT_CELL0 + i) == 0)
            free_cells++;
    snprintf(line, sizeof line, "DEAL %ld   MOVES %d   FREE CELLS %d", game.deal,
             game.moves, free_cells);
    ui_put_row(FC_ROW_STATUS, line, COL_WHITE);

    if (message)
        ui_put_row(FC_ROW_HINT, message, COL_YELLOW);
    else if (won)
        ui_put_row(FC_ROW_HINT, FC_HINT_WON, COL_CYAN);
    else
        ui_put_row(FC_ROW_HINT, picked >= 0 ? FC_HINT_PICKED : FC_HINT_PLAY, COL_CYAN);
    if (won)
        ui_put_row(FC_ROW_WIN, FC_TEXT_WIN, COL_YELLOW);
}

static void draw_everything(void)
{
    ui_set_background(FC_COL_FELT);
    lcd_clear_screen();
    mark_all();
    flush();
}
