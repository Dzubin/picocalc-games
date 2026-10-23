/*
 * klondike_play.c - the Klondike screen and keys (see klondike_play.h).
 *
 * The stock, the waste and the 4 foundations are in the top row and the 7
 * columns are below. A cursor (a yellow frame) moves over the spots; ENTER on
 * the stock draws, ENTER on any other spot picks up the card under the cursor
 * (cyan frame) and ENTER again drops it on the spot the cursor has moved to.
 * The cards that fit are found by themselves: dropping on a column takes the
 * card of the face-up run that goes there, so no count has to be chosen.
 *
 *   arrows ......... move the cursor
 *   1 to 7 ......... jump to a column;  D: stock;  W: waste
 *   ENTER, SPACE ... draw from the stock; pick up / drop
 *   F .............. send the card under the cursor (or the one picked up)
 *                    to its foundation
 *   S .............. step through the animation speeds (off, fast, normal, slow)
 *   C .............. choose the look of the cards
 *   U .............. undo;  R: same deal again;  N: another deal
 *   F4 ............. end the game as a win, with the victory show
 *   ESC ............ drop what is picked up; with nothing picked, leave
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
#include "cards.h"
#include "cardfly.h"
#include "victory.h"
#include "chooser.h"
#include "settings.h"
#include "ui.h"
#include "klondike.h"
#include "klondike_play.h"
#include "games_const.h"
#include "klondike_play_const.h"

static kl_t     game;
static int      draw_choice;        /* index into KL_DRAW_CARDS            */
static int      cursor;             /* the spot the cursor is on           */
static int      picked;             /* the spot picked up from, or -1      */
static bool     won;
static int      bad_drops;          /* refused drops in a row              */
static char     speed_text[TCOLS + 1];
static const char *message;         /* shown on the hint row until a key   */
static bool     dirty[KL_SPOTS];
static uint16_t pixels[CARDS_MAX_PIXELS];
static uint16_t part[CARDS_MAX_PIXELS]; /* the left part of a fanned card  */
static int      card_w, card_h, gap;
static bool     animating;          /* a card is flying: no frames drawn   */
static int      hide_spot = -1;     /* a spot drawn without its last cards */
static int      hide_count;

/* the spots of the top row, in the order the cursor goes along them */
static const int TOP_SPOTS[] = {
    KL_SPOT_STOCK, KL_SPOT_WASTE, KL_SPOT_FOUND0, KL_SPOT_FOUND0 + 1,
    KL_SPOT_FOUND0 + 2, KL_SPOT_FOUND0 + 3
};
#define TOP_SPOT_COUNT (int)(sizeof TOP_SPOTS / sizeof TOP_SPOTS[0])

/* forward declarations (the public function is first) */
static void start_deal(long deal, int draw);
static void draw_everything(void);
static void draw_spot(int spot);
static void draw_text(void);
static void flush(void);
static void mark(int spot);
static void mark_all(void);
static int  slot_of(int spot);
static int  spot_x(int spot);
static int  spot_width(int spot);
static void column_steps(int n, int hidden, int *down, int *up);
static int  card_y(int n, int hidden, int index);
static int  waste_shown(void);
static int  waste_card_x(int shown);
static void draw_frame(int x, int y, int w, int h, uint16_t colour);
static bool handle_key(uint8_t code);
static void move_cursor(int dx, int dy);
static void pick_or_drop(void);
static void send_to_foundation(void);
static void after_move(void);
static void celebrate(void);
static void end_game(void);
static void animate(int from, int to, int count);
static void redraw_near(int x, int y, int w, int h);
static int  shown_size(int spot);
static uint8_t shown_top(int spot);

/* ===================================================================== */
/*  Public                                                                */
/* ===================================================================== */
void klondike_play(void)
{
    long deal = ui_ask_deal(KL_TEXT_TITLE, KL_TEXT_ASK_EXTRA, KL_TEXT_DRAW_LABEL,
                            KL_DRAW_NAMES, KL_DRAW_CHOICES, &draw_choice);
    uint8_t code;

    if (deal < 0)
        return;
    ui_set_repeat(true);
    card_w = cards_width(KL_CARD_SIZE);
    card_h = cards_height(KL_CARD_SIZE);
    gap = (WIDTH - KL_SLOTS * card_w) / (KL_SLOTS + 1);
    start_deal(deal, KL_DRAW_CARDS[draw_choice]);
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

static void start_deal(long deal, int draw)
{
    kl_deal(&game, deal, draw);
    cursor = KL_SPOT_STOCK;
    picked = -1;
    bad_drops = 0;
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
    if (code == KL_KEY_NEW || code == KL_KEY_NEW - 32) {
        long deal = ui_ask_deal(KL_TEXT_TITLE, KL_TEXT_ASK_EXTRA, KL_TEXT_DRAW_LABEL,
                                KL_DRAW_NAMES, KL_DRAW_CHOICES, &draw_choice);

        if (deal >= 0)
            start_deal(deal, KL_DRAW_CARDS[draw_choice]);
        draw_everything();
        return true;
    }
    if (code == KL_KEY_RESTART || code == KL_KEY_RESTART - 32) {
        start_deal(game.deal, game.draw);
        draw_everything();
        return true;
    }
    if (code == KL_KEY_SPEED || code == KL_KEY_SPEED - 32) {
        snprintf(speed_text, sizeof speed_text, "Animation: %s", cardfly_next_speed());
        message = speed_text;
        return true;
    }
    if (won)
        return true;

    switch (code) {
    case KEY_F4:    end_game();         break;
    case KL_KEY_LOOK:
    case KL_KEY_LOOK - 32:
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
    case KL_KEY_FOUND:
    case KL_KEY_FOUND - 32:
        send_to_foundation();
        break;
    case KL_KEY_UNDO:
    case KL_KEY_UNDO - 32:
        if (kl_undo(&game)) {
            picked = -1;
            mark_all();
        }
        break;
    default:
        if (code >= '1' && code <= '0' + KL_COLUMNS)
            cursor = KL_SPOT_COL0 + (code - '1');
        else if (code == KL_KEY_STOCK || code == KL_KEY_STOCK - 32)
            cursor = KL_SPOT_STOCK;
        else if (code == KL_KEY_WASTE || code == KL_KEY_WASTE - 32)
            cursor = KL_SPOT_WASTE;
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
    int i;

    if (dy < 0 && cursor >= KL_SPOT_COL0) {         /* up into the top row */
        int slot = cursor - KL_SPOT_COL0;

        cursor = slot == 0 ? KL_SPOT_STOCK
               : slot < KL_SLOT_FOUND0 ? KL_SPOT_WASTE
               : KL_SPOT_FOUND0 + (slot - KL_SLOT_FOUND0);
    } else if (dy > 0 && cursor < KL_SPOT_COL0) {   /* down into the columns */
        cursor = KL_SPOT_COL0 + slot_of(cursor);
    } else if (dx != 0 && cursor >= KL_SPOT_COL0) {
        cursor = KL_SPOT_COL0 + (cursor - KL_SPOT_COL0 + dx + KL_COLUMNS) % KL_COLUMNS;
    } else if (dx != 0) {
        for (i = 0; i < TOP_SPOT_COUNT; i++)
            if (TOP_SPOTS[i] == cursor)
                break;
        cursor = TOP_SPOTS[(i + dx + TOP_SPOT_COUNT) % TOP_SPOT_COUNT];
    }
}

static void pick_or_drop(void)
{
    int count, from, to;

    if (picked < 0) {
        if (cursor == KL_SPOT_STOCK) {
            int n = kl_draw(&game);

            if (n == 0)
                message = KL_TEXT_NO_STOCK;
            mark(KL_SPOT_STOCK);
            mark(KL_SPOT_WASTE);
        } else if (kl_size(&game, cursor) == 0 ||
                   (cursor >= KL_SPOT_COL0 && kl_faceup(&game, cursor) == 0)) {
            message = KL_TEXT_NO_PICK;
        } else {
            picked = cursor;
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
    from = picked;
    to = cursor;
    if (to >= KL_SPOT_FOUND0 && to < KL_SPOT_COL0)      /* its own suit's pile */
        to = KL_SPOT_FOUND0 + KL_SUIT(kl_top(&game, from));
    count = kl_move(&game, from, cursor, 0);
    if (count == 0) {
        if (++bad_drops >= KL_BAD_DROPS_LIMIT) {        /* maybe a different card */
            mark(picked);
            picked = -1;
            message = KL_TEXT_PUT_BACK;
        } else {
            message = KL_TEXT_NO_MOVE;
        }
        return;
    }
    picked = -1;
    animate(from, to, count);
    after_move();
}

/* F: the card under the cursor, or the one picked up, goes to its foundation */
static void send_to_foundation(void)
{
    int from = picked >= 0 ? picked : cursor, to, count;

    if (from == KL_SPOT_STOCK || kl_size(&game, from) == 0) {
        message = KL_TEXT_NO_PICK;
        return;
    }
    to = KL_SPOT_FOUND0 + KL_SUIT(kl_top(&game, from));
    count = kl_move(&game, from, to, 0);
    if (count == 0) {
        message = KL_TEXT_NO_MOVE;
        return;
    }
    mark(picked >= 0 ? picked : cursor);
    picked = -1;
    animate(from, to, count);
    after_move();
}

static void after_move(void)
{
    int from, to;

    if (kl_can_finish(&game)) {                 /* every card is face up: finish it */
        message = KL_TEXT_FINISHING;
        draw_text();
        while (kl_finish_step(&game, &from, &to))
            animate(from, to, 1);
    }
    won = kl_won(&game);
    if (won)
        celebrate();
    mark_all();
}

/* the show for a win: the cards fall from the foundations */
static void celebrate(void)
{
    int start_x[KL_FOUNDATIONS], s;

    for (s = 0; s < KL_FOUNDATIONS; s++)
        start_x[s] = spot_x(KL_SPOT_FOUND0 + s);
    victory_cascade(KL_CARD_SIZE, settings_face_style(), start_x, KL_TOP_Y);
    draw_everything();
}

/* F4: end the game as a win at once, with the show: every card goes to its
 * foundation */
static void end_game(void)
{
    int s;

    picked = -1;
    memset(game.length, 0, sizeof game.length);
    memset(game.hidden, 0, sizeof game.hidden);
    game.stock_n = 0;
    game.waste_n = 0;
    for (s = 0; s < KL_FOUNDATIONS; s++)
        game.found[s] = 13;
    won = true;
    celebrate();
}

/* ===================================================================== */
/*  Where things are on the screen                                        */
/* ===================================================================== */
static void mark(int spot)  { dirty[spot] = true; }

static void mark_all(void)
{
    int i;

    for (i = 0; i < KL_SPOTS; i++)
        dirty[i] = true;
}

static void flush(void)
{
    int i;

    for (i = 0; i < KL_SPOTS; i++) {
        if (dirty[i]) {
            draw_spot(i);
            dirty[i] = false;
        }
    }
    draw_text();
}

/* the place across the table that a spot is over */
static int slot_of(int spot)
{
    if (spot == KL_SPOT_STOCK)
        return 0;
    if (spot == KL_SPOT_WASTE)
        return KL_SLOT_WASTE;
    if (spot < KL_SPOT_COL0)
        return KL_SLOT_FOUND0 + (spot - KL_SPOT_FOUND0);
    return spot - KL_SPOT_COL0;
}

static int spot_x(int spot)
{
    return gap + slot_of(spot) * (card_w + gap);
}

/* The width a spot draws over, with the gap round it. The waste takes the
 * empty place beside it as well (a fan of cards lies there), so that nothing
 * is left behind when a card flies across the table. */
static int spot_width(int spot)
{
    if (spot == KL_SPOT_WASTE)
        return spot_x(KL_SPOT_FOUND0) - spot_x(KL_SPOT_WASTE);
    return card_w + gap;
}

/* the distances between the cards of a column of n cards, hidden of them face
 * down: a long column is squeezed so that it still ends above the text rows */
static void column_steps(int n, int hidden, int *down, int *up)
{
    int room = KL_BOTTOM_Y - KL_COLUMN_Y - card_h;
    int down_gaps = n - 1 < hidden ? n - 1 : hidden;
    int up_gaps = n - 1 - hidden > 0 ? n - 1 - hidden : 0;

    *down = KL_STEP_DOWN;
    *up = KL_STEP_UP;
    if (up_gaps > 0 && down_gaps * *down + up_gaps * *up > room) {
        *up = (room - down_gaps * *down) / up_gaps;
        if (*up < KL_STEP_MIN)
            *up = KL_STEP_MIN;
    }
}

/* the top edge of card number index (0 is the first) in such a column */
static int card_y(int n, int hidden, int index)
{
    int down, up, i, y = KL_COLUMN_Y;

    column_steps(n, hidden, &down, &up);
    for (i = 0; i < index; i++)
        y += i < hidden ? down : up;
    return y;
}

/* the waste cards on show (the last ones, fanned out when drawing 3) */
static int waste_shown(void)
{
    int n = kl_size(&game, KL_SPOT_WASTE);

    if (game.draw != 3 || n < 2)
        return n > 0 ? 1 : 0;
    return n < KL_FAN_CARDS ? n : KL_FAN_CARDS;
}

/* the left edge of the top card of the waste */
static int waste_card_x(int shown)
{
    return spot_x(KL_SPOT_WASTE) + (shown > 1 ? (shown - 1) * KL_FAN : 0);
}

static void draw_frame(int x, int y, int w, int h, uint16_t colour)
{
    lcd_solid_rectangle(colour, (uint16_t)(x - KL_FRAME), (uint16_t)(y - KL_FRAME),
                        (uint16_t)(w + 2 * KL_FRAME), KL_FRAME);
    lcd_solid_rectangle(colour, (uint16_t)(x - KL_FRAME), (uint16_t)(y + h),
                        (uint16_t)(w + 2 * KL_FRAME), KL_FRAME);
    lcd_solid_rectangle(colour, (uint16_t)(x - KL_FRAME), (uint16_t)y, KL_FRAME,
                        (uint16_t)h);
    lcd_solid_rectangle(colour, (uint16_t)(x + w), (uint16_t)y, KL_FRAME, (uint16_t)h);
}

/* ===================================================================== */
/*  Drawing                                                               */
/* ===================================================================== */

/* what is on a spot as drawn: a spot may be drawn without the cards that are
 * still flying to it */
static int shown_size(int spot)
{
    int n = kl_size(&game, spot);

    return spot == hide_spot ? n - hide_count : n;
}

static uint8_t shown_top(int spot)
{
    if (spot == hide_spot && spot >= KL_SPOT_FOUND0 && spot < KL_SPOT_COL0) {
        int suit = spot - KL_SPOT_FOUND0;
        int cards = game.found[suit] - hide_count;

        return cards > 0 ? (uint8_t)((cards - 1) * 4 + suit) : KL_NO_CARD;
    }
    return kl_top(&game, spot);
}

/* draw again every spot that the rectangle touches */
static void redraw_near(int x, int y, int w, int h)
{
    int spot;

    for (spot = 0; spot < KL_SPOTS; spot++) {
        int sx = spot_x(spot) - gap / 2;
        int sw = spot_width(spot);
        int top = spot < KL_SPOT_COL0 ? KL_TOP_Y - KL_FRAME : KL_COLUMN_Y - KL_FRAME;
        int bottom = spot < KL_SPOT_COL0 ? KL_COLUMN_Y : KL_ROW_HINT * GLYPH_HEIGHT;

        if (x < sx + sw && x + w > sx && y < bottom && y + h > top)
            draw_spot(spot);
    }
}

/* draw one spot with whatever lies on it, and the frame if it has one */
static void draw_spot(int spot)
{
    int x = spot_x(spot);
    int clear_x = x - gap / 2;
    int clear_w = spot_width(spot);
    int frame_x = x, frame_y, frame_h = card_h;

    if (spot < KL_SPOT_COL0) {
        uint8_t c;

        frame_y = KL_TOP_Y;
        lcd_solid_rectangle(KL_COL_FELT, (uint16_t)clear_x, (uint16_t)(KL_TOP_Y - KL_FRAME),
                            (uint16_t)clear_w,
                            (uint16_t)(KL_COLUMN_Y - KL_TOP_Y));
        if (spot == KL_SPOT_STOCK) {
            if (kl_size(&game, spot) > 0)
                cards_draw_back(pixels, KL_CARD_SIZE, KL_BACK_DESIGN);
            else
                cards_draw_slot(pixels, KL_CARD_SIZE, -1);
            lcd_blit(pixels, (uint16_t)x, KL_TOP_Y, (uint16_t)card_w, (uint16_t)card_h);
        } else if (spot == KL_SPOT_WASTE) {
            int shown = waste_shown(), n = kl_size(&game, spot), i;

            if (shown == 0) {
                cards_draw_slot(pixels, KL_CARD_SIZE, -1);
                lcd_blit(pixels, (uint16_t)x, KL_TOP_Y, (uint16_t)card_w, (uint16_t)card_h);
            }
            for (i = 0; i < shown; i++) {
                uint8_t card = game.waste[n - shown + i];
                int cx = x + i * KL_FAN, r;

                /* a card that is partly covered shows the small index whatever
                 * face was chosen */
                cards_draw_face(pixels, KL_CARD_SIZE,
                                i < shown - 1 ? 0 : settings_face_style(),
                                KL_RANK(card), KL_SUIT(card));
                if (i < shown - 1) {            /* only the left part shows */
                    for (r = 0; r < card_h; r++)
                        memcpy(&part[r * KL_FAN], &pixels[r * card_w],
                               (size_t)KL_FAN * sizeof part[0]);
                    lcd_blit(part, (uint16_t)cx, KL_TOP_Y, KL_FAN, (uint16_t)card_h);
                } else {
                    lcd_blit(pixels, (uint16_t)cx, KL_TOP_Y, (uint16_t)card_w,
                             (uint16_t)card_h);
                }
            }
            frame_x = waste_card_x(shown);
        } else {
            c = shown_top(spot);
            if (c == KL_NO_CARD)
                cards_draw_slot(pixels, KL_CARD_SIZE, spot - KL_SPOT_FOUND0);
            else
                cards_draw_face(pixels, KL_CARD_SIZE, settings_face_style(), KL_RANK(c), KL_SUIT(c));
            lcd_blit(pixels, (uint16_t)x, KL_TOP_Y, (uint16_t)card_w, (uint16_t)card_h);
        }
    } else {
        int col = spot - KL_SPOT_COL0;
        int n = shown_size(spot), hidden = game.hidden[col], i, down, up;
        const uint8_t *cards = game.column[col];

        lcd_solid_rectangle(KL_COL_FELT, (uint16_t)clear_x,
                            (uint16_t)(KL_COLUMN_Y - KL_FRAME), (uint16_t)clear_w,
                            (uint16_t)(KL_ROW_HINT * GLYPH_HEIGHT - (KL_COLUMN_Y - KL_FRAME)));
        column_steps(n, hidden, &down, &up);
        if (n == 0) {
            cards_draw_slot(pixels, KL_CARD_SIZE, -1);
            lcd_blit(pixels, (uint16_t)x, KL_COLUMN_Y, (uint16_t)card_w, (uint16_t)card_h);
        }
        for (i = 0; i < n; i++) {
            int cy = card_y(n, hidden, i);
            int rows = card_h;

            if (i < hidden) {
                cards_draw_back(pixels, KL_CARD_SIZE, KL_BACK_DESIGN);
                rows = down;
            } else if (i < n - 1) {
                cards_draw_strip(pixels, KL_CARD_SIZE, KL_RANK(cards[i]), KL_SUIT(cards[i]));
                rows = up;
            } else {
                cards_draw_face(pixels, KL_CARD_SIZE, settings_face_style(), KL_RANK(cards[i]),
                                KL_SUIT(cards[i]));
            }
            if (i > 0)
                cards_lay_on_card(pixels, KL_CARD_SIZE);
            lcd_blit(pixels, (uint16_t)x, (uint16_t)cy, (uint16_t)card_w, (uint16_t)rows);
        }
        frame_y = n > 0 ? card_y(n, hidden, n - 1) : KL_COLUMN_Y;
    }

    if (animating)
        return;
    if (spot == picked)
        draw_frame(frame_x, frame_y, card_w, frame_h, KL_COL_PICKED);
    else if (spot == cursor)
        draw_frame(frame_x, frame_y, card_w, frame_h, KL_COL_CURSOR);
}

static void draw_text(void)
{
    char line[TCOLS + 1];

    snprintf(line, sizeof line, "DEAL %ld  SCORE %d  MOVES %d  DRAW %d", game.deal,
             game.score, game.moves, game.draw);
    ui_put_row(KL_ROW_STATUS, line, COL_WHITE);

    if (message)
        ui_put_row(KL_ROW_HINT, message, COL_YELLOW);
    else if (won)
        ui_put_row(KL_ROW_HINT, KL_HINT_WON, COL_CYAN);
    else
        ui_put_row(KL_ROW_HINT, picked >= 0 ? KL_HINT_PICKED : KL_HINT_PLAY, COL_CYAN);
    if (won)
        ui_put_row(KL_ROW_WIN, KL_TEXT_WIN, COL_YELLOW);
}

static void draw_everything(void)
{
    ui_set_background(KL_COL_FELT);
    lcd_clear_screen();
    mark_all();
    flush();
}

/* ===================================================================== */
/*  A card or a run flying to its place                                   */
/* ===================================================================== */

/* Show count cards flying from one spot to another. The move has already been
 * made in the game, so the cards are the last ones of the destination; the
 * screen still shows them on the source, and the destination is drawn without
 * them until they land. */
static void animate(int from, int to, int count)
{
    uint8_t moved[KL_MAX_COLUMN];
    int dest_n = kl_size(&game, to), src_n = kl_size(&game, from);
    int pitch = KL_STEP_UP, sx, sy, ex, ey, i;

    if (!cardfly_enabled())
        return;
    if (to >= KL_SPOT_COL0) {
        for (i = 0; i < count; i++)
            moved[i] = game.column[to - KL_SPOT_COL0][dest_n - count + i];
        ex = spot_x(to);
        ey = card_y(dest_n, game.hidden[to - KL_SPOT_COL0], dest_n - count);
    } else {
        moved[0] = kl_top(&game, to);
        ex = spot_x(to);
        ey = KL_TOP_Y;
    }
    if (from >= KL_SPOT_COL0) {
        int hidden_before = game.hidden[from - KL_SPOT_COL0] + game.last_flipped;
        int down;

        column_steps(src_n + count, hidden_before, &down, &pitch);
        sx = spot_x(from);
        sy = card_y(src_n + count, hidden_before, src_n);
    } else if (from == KL_SPOT_WASTE) {
        int before = src_n + 1, shown = game.draw == 3 && before > 1
                        ? (before < KL_FAN_CARDS ? before : KL_FAN_CARDS) : 1;

        sx = waste_card_x(shown);
        sy = KL_TOP_Y;
    } else {
        sx = spot_x(from);
        sy = KL_TOP_Y;
    }
    animating = true;
    hide_spot = to;
    hide_count = count;
    draw_spot(from);
    draw_spot(to);
    cardfly(moved, count, KL_CARD_SIZE, settings_face_style(), pitch, sx, sy, ex, ey, false,
            redraw_near);
    hide_spot = -1;
    animating = false;
    draw_spot(from);
    draw_spot(to);
}
