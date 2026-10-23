/*
 * chooser.c - the screen for choosing the look of the playing cards (see
 * chooser.h).
 *
 * Author: Anthropic Claude
 */
#include <stdbool.h>
#include <stdint.h>

#include "lcd.h"          /* WIDTH, RGB(), lcd_*()  */
#include "keyboard.h"     /* KEY_* codes            */

#include "platform.h"
#include "cards.h"
#include "ui.h"
#include "settings.h"
#include "chooser.h"
#include "games_const.h"

/* forward declarations (the public function comes first) */
static void draw_chooser(void);
static void draw_marks(int marker);
static void draw_row(int style, int y);

/* ===================================================================== */
/*  Public                                                                */
/* ===================================================================== */
void card_chooser(void)
{
    int marker = settings_face_style();
    uint8_t code;

    ui_set_repeat(false);                   /* one step for each press */
    draw_chooser();
    draw_marks(marker);
    ui_drain_keys();

    for (;;) {
        if (!ui_poll_key(&code)) {
            plat_sleep_ms(POLL_MS);
            continue;
        }
        if (code == KEY_ESC)
            break;
        if (code == KEY_UP)
            marker = (marker + CHOOSER_FACES - 1) % CHOOSER_FACES;
        else if (code == KEY_DOWN)
            marker = (marker + 1) % CHOOSER_FACES;
        else if (code == KEY_ENTER || code == KEY_RETURN || code == ' ')
            settings_set_face_style(marker);
        else
            continue;
        draw_marks(marker);
    }
    ui_set_repeat(true);
}

/* ===================================================================== */
/*  Drawing                                                               */
/* ===================================================================== */

/* one row of eight cards in a face layout, its top edge at y */
static void draw_row(int style, int y)
{
    static uint16_t pixels[CARDS_MAX_PIXELS];
    int w = cards_width(CHOOSER_SIZE);
    int h = cards_height(CHOOSER_SIZE);
    int gap = (WIDTH - CHOOSER_ACROSS * w) / (CHOOSER_ACROSS + 1);
    int i;

    for (i = 0; i < CHOOSER_ACROSS; i++) {
        cards_draw_face(pixels, CHOOSER_SIZE, style, CHOOSER_CARDS[i].rank,
                        CHOOSER_CARDS[i].suit);
        lcd_blit(pixels, (uint16_t)(gap + i * (w + gap)), (uint16_t)y, (uint16_t)w,
                 (uint16_t)h);
    }
}

static void draw_chooser(void)
{
    int h = cards_height(CHOOSER_SIZE), i;

    ui_set_background(COL_TABLE);
    lcd_clear_screen();
    ui_put_row(CHOOSER_ROW_TITLE, CHOOSER_TEXT_TITLE, COL_YELLOW);
    ui_put_row(CHOOSER_ROW_LEGEND, CHOOSER_TEXT_LEGEND, COL_WHITE);
    for (i = 0; i < CHOOSER_FACES; i++)
        draw_row(i, CHOOSER_TOP + i * (h + CHOOSER_ROW_GAP));
    ui_put_row(CHOOSER_ROW_HINT, CHOOSER_TEXT_HINT, COL_CYAN);
}

/* a frame round each of the rows: yellow on the row the marker is on, green on
 * the one used in the games, otherwise the colour of the table (which takes an
 * old frame away) */
static void draw_marks(int marker)
{
    int w = cards_width(CHOOSER_SIZE), h = cards_height(CHOOSER_SIZE);
    int gap = (WIDTH - CHOOSER_ACROSS * w) / (CHOOSER_ACROSS + 1);
    int span = CHOOSER_ACROSS * w + (CHOOSER_ACROSS - 1) * gap;
    int t = CHOOSER_FRAME, pad = CHOOSER_FRAME_PAD, i;

    for (i = 0; i < CHOOSER_FACES; i++) {
        int x = gap - pad - t, y = CHOOSER_TOP + i * (h + CHOOSER_ROW_GAP) - pad - t;
        int fw = span + 2 * (pad + t), fh = h + 2 * (pad + t);
        uint16_t colour = i == marker ? COL_MARK_CURSOR
                        : i == settings_face_style() ? COL_MARK_CHOSEN
                        : COL_TABLE;

        lcd_solid_rectangle(colour, (uint16_t)x, (uint16_t)y, (uint16_t)fw, (uint16_t)t);
        lcd_solid_rectangle(colour, (uint16_t)x, (uint16_t)(y + fh - t), (uint16_t)fw, (uint16_t)t);
        lcd_solid_rectangle(colour, (uint16_t)x, (uint16_t)y, (uint16_t)t, (uint16_t)fh);
        lcd_solid_rectangle(colour, (uint16_t)(x + fw - t), (uint16_t)y, (uint16_t)t, (uint16_t)fh);
    }
}
