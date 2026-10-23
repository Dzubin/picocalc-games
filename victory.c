/*
 * victory.c - the victory celebration (see victory.h).
 *
 * Author: Anthropic Claude
 */
#include <stdbool.h>
#include <stdint.h>

#include "lcd.h"          /* WIDTH, HEIGHT, RGB(), lcd_blit(), lcd_solid_rectangle() */
#include "audio.h"        /* audio_play_sound(), audio_stop() */

#include "platform.h"
#include "cards.h"
#include "ui.h"
#include "victory.h"
#include "victory_const.h"
#include "games_const.h"

static uint32_t sound_start_ms;
static int      sound_note;         /* the note playing, -1 before the first   */
static bool     sound_on;
static uint32_t rand_state = 12345u;

/* forward declarations (the public functions come first) */
static void     sound_begin(void);
static void     sound_update(void);
static void     sound_finish(void);
static uint32_t next_rand(void);
static bool     key_pressed(void);

/* ===================================================================== */
/*  Public                                                                */
/* ===================================================================== */
void victory_cascade(int size, int face_style, const int *start_x, int start_y)
{
    static uint16_t pixels[CARDS_MAX_PIXELS];
    int w = cards_width(size), h = cards_height(size);
    int floor_y = HEIGHT - h, n, frame = 0;
    bool stop = false;

    sound_begin();
    for (n = 0; n < VICTORY_CARDS && !stop; n++) {
        int rank = 12 - n / 4, suit = n % 4;
        int x = start_x[suit], y = start_y, vy = 0;
        int vx = VICTORY_VX_MIN + (int)(next_rand() % VICTORY_VX_SPAN);

        if (next_rand() & 1)
            vx = -vx;
        cards_draw_face(pixels, size, face_style, rank, suit);
        while (x > -w && x < WIDTH) {
            if (x >= 0 && x + w <= WIDTH)           /* the LCD cannot clip for us */
                lcd_blit(pixels, (uint16_t)x, (uint16_t)y, (uint16_t)w, (uint16_t)h);
            x += vx;
            vy += VICTORY_GRAVITY;
            y += vy;
            if (y >= floor_y) {                     /* a bounce */
                y = floor_y;
                vy = -(vy * VICTORY_BOUNCE_NUM) / VICTORY_BOUNCE_DEN;
                if (vy > -VICTORY_BOUNCE_MIN)
                    vy = -VICTORY_BOUNCE_MIN;
            }
            sound_update();
            plat_sleep_ms(VICTORY_FRAME_MS);
            if (++frame % VICTORY_KEY_EVERY == 0 && key_pressed()) {
                stop = true;
                break;
            }
        }
    }
    sound_finish();
}

void victory_flash(int x, int y, int w, int h)
{
    int i;

    sound_begin();
    for (i = 0; i < VICTORY_FLASHES; i++) {
        lcd_solid_rectangle((i & 1) ? VICTORY_FLASH_COL_B : VICTORY_FLASH_COL_A,
                            (uint16_t)x, (uint16_t)y, (uint16_t)w, (uint16_t)h);
        plat_sleep_ms(VICTORY_FLASH_MS);
        sound_update();
        if (key_pressed())
            break;
    }
    sound_finish();
}

/* ===================================================================== */
/*  The fanfare, played as the show goes on                               */
/* ===================================================================== */
static void sound_begin(void)
{
    sound_start_ms = plat_now_ms();
    sound_note = -1;
    sound_on = true;
}

/* start the note that the time calls for; stop at the end */
static void sound_update(void)
{
    uint32_t t = plat_now_ms() - sound_start_ms, end = 0;
    int i;

    if (!sound_on)
        return;
    for (i = 0; i < VICTORY_NOTE_COUNT; i++) {
        end += (uint32_t)VICTORY_NOTES[i].ms;
        if (t < end)
            break;
    }
    if (i >= VICTORY_NOTE_COUNT) {
        audio_stop();
        sound_on = false;
        return;
    }
    if (i != sound_note) {
        sound_note = i;
        if (VICTORY_NOTES[i].hz > 0)
            audio_play_sound((uint32_t)VICTORY_NOTES[i].hz, (uint32_t)VICTORY_NOTES[i].hz);
        else
            audio_stop();
    }
}

/* let the fanfare play to its end unless a key was pressed */
static void sound_finish(void)
{
    while (sound_on && !key_pressed()) {
        sound_update();
        plat_sleep_ms(POLL_MS);
    }
    if (sound_on)
        audio_stop();
    sound_on = false;
}

/* ===================================================================== */
/*  Helpers                                                               */
/* ===================================================================== */
static uint32_t next_rand(void)
{
    rand_state = rand_state * (uint32_t)VICTORY_RAND_MUL + (uint32_t)VICTORY_RAND_ADD;
    return rand_state >> VICTORY_RAND_SHIFT;
}

static bool key_pressed(void)
{
    uint8_t code;

    return ui_poll_key(&code);
}
