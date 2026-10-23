/*
 * ui.h - the text rows and the keyboard, shared by the screens of the games.
 *
 * Author: Anthropic Claude
 */
#ifndef UI_H
#define UI_H

#include <stdbool.h>
#include <stdint.h>

/* the colour behind the text rows (black, or the green of the card table) */
void ui_set_background(uint16_t colour);

void ui_clear_row(int row);
void ui_put_row(int row, const char *str, uint16_t fg);   /* centred */
void ui_put_text(int row, int col, const char *str, uint16_t fg);   /* from a column */

/* Keyboard. '~' goes to BOOTSEL from every one of these. */
void    ui_drain_keys(void);
bool    ui_poll_key(uint8_t *code);   /* a key pressed now, or false */
void    ui_set_repeat(bool on);       /* arrow keys repeat while held */
uint8_t ui_wait_key(void);            /* waits for a key press       */

/* Ask for a deal number: the digits typed, 1 to 32000, or a random one for
 * ENTER alone; -1 when ESC is pressed. title heads the screen and extra_help
 * is one more line of help (or NULL). A game with a choice before the deal
 * passes its name, the names of the choices and a place for the answer: the
 * T key steps through them. */
long ui_ask_deal(const char *title, const char *extra_help,
                 const char *option_label, const char *const *option_names,
                 int option_count, int *option);

/* Ask for a number from min to max: the digits typed, or def for ENTER alone;
 * -1 when ESC is pressed. title heads the screen and prompt names the number. */
int ui_ask_number(const char *title, const char *prompt, int min, int max, int def);

_Noreturn void ui_exit_to_loader(void);

#endif /* UI_H */
