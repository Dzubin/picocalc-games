/*
 * ui.c - the text rows and the keyboard, shared by the screens of the games
 * (see ui.h). Keys are read straight from the south bridge's FIFO.
 *
 * Author: Anthropic Claude
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "lcd.h"          /* WIDTH, GLYPH_HEIGHT, lcd_*()          */
#include "southbridge.h"  /* sb_read_keyboard()                    */
#include "keyboard.h"     /* KEY_* codes, KEY_STATE_* values       */

#include "platform.h"
#include "ui.h"
#include "games_const.h"

static uint16_t screen_bg = COL_BG;
static bool     repeat_on;          /* arrow keys repeat while held         */
static uint8_t  held;               /* the arrow key held down, or 0        */
static uint32_t next_repeat;        /* when it repeats next                 */

/* forward declarations (the public functions come first) */
static bool is_mod_key(uint8_t code);
static bool is_arrow_key(uint8_t code);
static _Noreturn void go_bootsel(void);

void ui_set_background(uint16_t colour)
{
    screen_bg = colour;
    lcd_set_background(colour);
}

void ui_clear_row(int row)
{
    lcd_solid_rectangle(screen_bg, 0, (uint16_t)(row * GLYPH_HEIGHT), WIDTH,
                        GLYPH_HEIGHT);
}

void ui_put_row(int row, const char *str, uint16_t fg)
{
    int len = (int)strlen(str);
    int col = (TCOLS - len) / 2;

    ui_clear_row(row);
    if (col < 0)
        col = 0;
    lcd_set_background(screen_bg);
    lcd_set_foreground(fg);
    lcd_putstr((uint8_t)col, (uint8_t)row, str);
}

void ui_put_text(int row, int col, const char *str, uint16_t fg)
{
    ui_clear_row(row);
    lcd_set_background(screen_bg);
    lcd_set_foreground(fg);
    lcd_putstr((uint8_t)col, (uint8_t)row, str);
}

void ui_drain_keys(void)
{
    while (sb_read_keyboard() != 0)
        ;                     /* reading the FIFO empties it */
    held = 0;                 /* a release may have been thrown away */
}

void ui_set_repeat(bool on)
{
    repeat_on = on;
    held = 0;
}

/* A key pressed now. A held arrow key (when repeat is on) comes again after
 * UI_REPEAT_DELAY_MS and then every UI_REPEAT_MS, until its release arrives. */
bool ui_poll_key(uint8_t *code)
{
    uint16_t e = sb_read_keyboard();
    uint8_t c = e & 0xFF;
    uint8_t state = e >> 8;
    uint32_t now = plat_now_ms();

    if (e) {
        if (state == KEY_STATE_RELEASED) {
            if (c == held)
                held = 0;
            return false;
        }
        if (state != KEY_STATE_PRESSED || is_mod_key(c))
            return false;
        if (c == '~')
            go_bootsel();
        held = (repeat_on && is_arrow_key(c)) ? c : 0;
        next_repeat = now + UI_REPEAT_DELAY_MS;
        *code = c;
        return true;
    }
    if (held && (int32_t)(now - next_repeat) >= 0) {
        next_repeat = now + UI_REPEAT_MS;
        *code = held;
        return true;
    }
    return false;
}

uint8_t ui_wait_key(void)
{
    uint8_t code;

    ui_drain_keys();
    for (;;) {
        if (ui_poll_key(&code)) {
            ui_drain_keys();
            return code;
        }
        plat_sleep_ms(POLL_MS);
    }
}

static bool is_arrow_key(uint8_t code)
{
    return code == KEY_UP || code == KEY_DOWN || code == KEY_LEFT || code == KEY_RIGHT;
}

long ui_ask_deal(const char *title, const char *extra_help,
                 const char *option_label, const char *const *option_names,
                 int option_count, int *option)
{
    char digits[DEAL_DIGITS + 1];
    char line[TCOLS + 1];
    int n = 0;
    uint8_t code;

    digits[0] = '\0';
    ui_set_background(COL_BG);
    lcd_clear_screen();
    ui_put_row(ASK_ROW_TITLE, title, COL_CYAN);
    ui_put_row(ASK_ROW_PROMPT, ASK_TEXT_PROMPT, COL_WHITE);
    ui_put_row(ASK_ROW_HELP, ASK_TEXT_HELP_1, COL_WHITE);
    ui_put_row(ASK_ROW_HELP + 1, ASK_TEXT_HELP_2, COL_WHITE);
    ui_put_row(ASK_ROW_HELP + 2, ASK_TEXT_HELP_3, COL_WHITE);
    if (extra_help)
        ui_put_row(ASK_ROW_HELP + 3, extra_help, COL_WHITE);
    ui_drain_keys();

    for (;;) {
        snprintf(line, sizeof line, "%s_", digits);
        ui_put_row(ASK_ROW_ENTRY, line, COL_YELLOW);
        if (option_label) {
            snprintf(line, sizeof line, "%s: %s   (T: change)", option_label,
                     option_names[*option]);
            ui_put_row(ASK_ROW_OPTION, line, COL_GREEN);
        }
        while (!ui_poll_key(&code))
            plat_sleep_ms(POLL_MS);

        if (code == KEY_ESC)
            return -1;
        if (code >= '0' && code <= '9' && n < DEAL_DIGITS) {
            digits[n++] = (char)code;
            digits[n] = '\0';
        } else if (code == KEY_BACKSPACE && n > 0) {
            digits[--n] = '\0';
        } else if ((code == ASK_KEY_OPTION || code == ASK_KEY_OPTION - 32) && option_label) {
            *option = (*option + 1) % option_count;
        } else if (code == KEY_ENTER || code == KEY_RETURN) {
            long v = atol(digits);

            if (n == 0)
                return (long)(((plat_now_ms() * DEAL_RANDOM_MUL) >> DEAL_RANDOM_SHIFT) %
                              DEAL_MAX) + 1;
            if (v >= DEAL_MIN && v <= DEAL_MAX)
                return v;
        }
    }
}

int ui_ask_number(const char *title, const char *prompt, int min, int max, int def)
{
    char digits[ASK_NUMBER_DIGITS + 1];
    char line[TCOLS + 1];
    int n = 0;
    uint8_t code;

    digits[0] = '\0';
    ui_set_background(COL_BG);
    lcd_clear_screen();
    ui_put_row(ASK_ROW_TITLE, title, COL_CYAN);
    snprintf(line, sizeof line, "%s (%d TO %d)", prompt, min, max);
    ui_put_row(ASK_ROW_PROMPT, line, COL_WHITE);
    snprintf(line, sizeof line, "ENTER alone: %d", def);
    ui_put_row(ASK_ROW_HELP, line, COL_WHITE);
    ui_put_row(ASK_ROW_HELP + 1, ASK_TEXT_HELP_3, COL_WHITE);
    ui_drain_keys();

    for (;;) {
        snprintf(line, sizeof line, "%s_", digits);
        ui_put_row(ASK_ROW_ENTRY, line, COL_YELLOW);
        while (!ui_poll_key(&code))
            plat_sleep_ms(POLL_MS);

        if (code == KEY_ESC)
            return -1;
        if (code >= '0' && code <= '9' && n < ASK_NUMBER_DIGITS) {
            digits[n++] = (char)code;
            digits[n] = '\0';
        } else if (code == KEY_BACKSPACE && n > 0) {
            digits[--n] = '\0';
        } else if (code == KEY_ENTER || code == KEY_RETURN) {
            int v = atoi(digits);

            if (n == 0)
                return def;
            if (v >= min && v <= max)
                return v;
        }
    }
}

/* true for a bare SHIFT / CTRL / ALT / SYM modifier key code */
static bool is_mod_key(uint8_t code)
{
    return code == KEY_MOD_SHL || code == KEY_MOD_SHR || code == KEY_MOD_CTRL ||
           code == KEY_MOD_ALT || code == KEY_MOD_SYM;
}

/* reboot into BOOTSEL (USB drive) mode; does not return */
static _Noreturn void go_bootsel(void)
{
    ui_put_row(ROW_NOTICE, "REBOOTING TO BOOTSEL...", COL_YELLOW);
    plat_sleep_ms(REBOOT_MS);
    plat_bootsel();
}

/* leave for the UF2 Loader menu; does not return */
_Noreturn void ui_exit_to_loader(void)
{
    ui_put_row(SPLASH_ROW_EXIT, "BACK TO THE LOADER...", COL_YELLOW);
    plat_sleep_ms(REBOOT_MS);
    plat_exit_to_loader();
}
