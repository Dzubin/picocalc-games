/*
 *  PicoCalc Games
 *  ===========================================
 *
 *  Minesweeper, Klondike Solitaire and FreeCell for the 320 x 320 screen of
 *  the ClockworkPi PicoCalc. All three are playable. In the card games the C
 *  key chooses the face layout of the cards.
 *
 *  Hardware used (all through the vendored picocalc-text-starter drivers by
 *  Blair Leduc, in picocalc-text-starter-main/):
 *      - lcd_init(), lcd_putstr(), lcd_solid_rectangle(), lcd_blit()
 *      - sb_init(), sb_read_keyboard()                     (south bridge)
 *      - audio_init(), audio_play_sound(), audio_stop()    (the victory fanfare)
 *
 *  Where things are
 *      this file ....... main and the splash menu
 *      freecell.c ...... the rules of FreeCell (no screen, tested on a PC)
 *      freecell_play.c . the FreeCell screen and keys
 *      klondike.c ...... the rules of Klondike (no screen, tested on a PC)
 *      klondike_play.c . the Klondike screen and keys
 *      minesweeper.c ... the rules of Minesweeper (no screen, tested on a PC)
 *      minesweeper_play.c the Minesweeper screen and keys
 *      mine_cells.c .... the squares of the board drawn into a pixel buffer
 *      cardfly.c ....... a card or a run of cards flying across the screen
 *      victory.c ....... the fanfare and the show when the player wins
 *      cards.c ......... the playing cards, drawn into a pixel buffer
 *      chooser.c ....... the screen for choosing the look of the card faces (C in a card game)
 *      ui.c ............ text rows and the keyboard, shared by the screens
 *
 *  Controls (the FreeCell keys are listed in freecell_play.c)
 *      F .............. splash: play FreeCell;  K: Klondike;  M: Minesweeper
 *      ESC or Q ....... splash: leave for the PicoCalc UF2 Loader menu
 *      ESC ............ games: back to the splash
 *      ~ (SHIFT+`) .... reboot into BOOTSEL (USB drive) mode, from anywhere
 *
 *  Constants are in games_const.h (this file), cards_const.h, freecell_const.h
 *  and freecell_play_const.h; platform.h is the whole interface to the machine
 *  (clock, sleep, BOOTSEL, exit to the UF2 Loader), implemented in
 *  platform_pico.c for the PicoCalc and in desktop/platform_desktop.c for
 *  Windows and Linux.
 *
 *  Author: Anthropic Claude
 */
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "lcd.h"          /* WIDTH, HEIGHT, RGB(), lcd_*()                    */
#include "southbridge.h"  /* sb_init()                                        */
#include "audio.h"        /* audio_init(): the victory fanfare                */
#include "keyboard.h"     /* KEY_* codes                                      */

#include "platform.h"     /* clock, sleep, BOOTSEL, loader exit (per platform) */
#include "ui.h"           /* text rows and the keyboard                       */
#include "freecell_play.h"
#include "klondike_play.h"
#include "minesweeper_play.h"
#include "games_const.h"  /* every constant and screen position */

/* ===================================================================== */
/*  Forward declarations (so main() can sit at the top)                   */
/* ===================================================================== */
static void splash_screen(void);

/* ===================================================================== */
/*  main: set up the hardware once, then the splash menu                  */
/* ===================================================================== */
int main(void)
{
    plat_init();

    sb_init();                /* keyboard / south-bridge I2C (no bg poll) */
    lcd_init();               /* ST7365P LCD */
    lcd_enable_cursor(false); /* no blinking text cursor */
    audio_init();             /* PIO PWM audio on GP26 / GP27 */

    for (;;) {
        uint8_t key;

        splash_screen();
        key = ui_wait_key();
        if (key == KEY_ESC || key == 'q' || key == 'Q')
            ui_exit_to_loader();      /* does not return */
        if (key == 'f' || key == 'F')
            freecell_play();          /* returns when ESC is pressed */
        else if (key == 'k' || key == 'K')
            klondike_play();
        else if (key == 'm' || key == 'M')
            minesweeper_play();
    }
}

/* ===================================================================== */
/*  Screens                                                               */
/* ===================================================================== */
static void splash_screen(void)
{
    int i;

    ui_set_background(COL_BG);
    lcd_clear_screen();
    ui_put_row(SPLASH_ROW_TITLE, "PICOCALC GAMES", COL_CYAN);
    ui_put_row(SPLASH_ROW_VERSION, VERSION, COL_CYAN);
    ui_put_row(SPLASH_ROW_AUTHOR, SPLASH_TEXT_AUTHOR, COL_WHITE);
    for (i = 0; i < SPLASH_MENU_CHOICES; i++)
        ui_put_text(SPLASH_ROW_MENU + i * SPLASH_MENU_STEP, SPLASH_MENU_COL,
                    SPLASH_MENU_TEXT[i], COL_WHITE);
    ui_put_row(SPLASH_ROW_EXIT_HINT, "ESC or Q: exit", COL_WHITE);
}
