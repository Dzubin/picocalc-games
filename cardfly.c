/*
 * cardfly.c - a card or a run of cards flying across the screen (see
 * cardfly.h).
 *
 * Author: Anthropic Claude
 */
#include <stdbool.h>
#include <stdint.h>
#include <string.h>

#include "lcd.h"          /* lcd_blit() */

#include "platform.h"
#include "cards.h"
#include "cardfly.h"
#include "games_const.h"

static uint16_t sprite[CARDFLY_SPRITE_PIXELS];  /* the run of cards          */
static uint16_t pixels[CARDS_MAX_PIXELS];       /* one card while building   */
static int      speed = ANIM_SPEED_DEFAULT;     /* index into ANIM_SPEED     */

bool cardfly_enabled(void)
{
    return ANIM_SPEED[speed].steps > 0;
}

const char *cardfly_next_speed(void)
{
    speed = (speed + 1) % ANIM_SPEEDS;
    return ANIM_SPEED[speed].name;
}

void cardfly(const uint8_t *cards, int count, int size, int face_style,
             int pitch, int sx, int sy, int ex, int ey, bool automatic,
             cardfly_redraw_t redraw)
{
    const anim_speed_t *sp = &ANIM_SPEED[speed];
    int steps = automatic ? sp->auto_steps : sp->steps;
    int w = cards_width(size), h = cards_height(size);
    int sprite_h = h + (count - 1) * pitch;
    int i, t, old_x = sx, old_y = sy;

    if (steps <= 0)
        return;
    if (automatic)
        plat_sleep_ms((uint32_t)sp->pause_ms);

    /* the cards as one picture: a strip of each and the last one whole */
    for (i = 0; i < count; i++) {
        int rows = i < count - 1 ? pitch : h, r;

        if (i < count - 1)
            cards_draw_strip(pixels, size, cards[i] >> 2, cards[i] & 3);
        else
            cards_draw_face(pixels, size, face_style, cards[i] >> 2, cards[i] & 3);
        if (i > 0)
            cards_lay_on_card(pixels, size);
        for (r = 0; r < rows; r++)
            memcpy(&sprite[(i * pitch + r) * w], &pixels[r * w],
                   (size_t)w * sizeof sprite[0]);
    }

    for (t = 1; t <= steps; t++) {
        int x = sx + (ex - sx) * t / steps;
        int y = sy + (ey - sy) * t / steps;

        redraw(old_x, old_y, w, sprite_h);
        redraw(x, y, w, sprite_h);
        lcd_blit(sprite, (uint16_t)x, (uint16_t)y, (uint16_t)w, (uint16_t)sprite_h);
        old_x = x;
        old_y = y;
        plat_sleep_ms((uint32_t)sp->frame_ms);
    }
}
