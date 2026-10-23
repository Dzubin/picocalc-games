/*
 * mine_cells.h - the squares of the Minesweeper board drawn into a pixel
 * buffer. Plain ISO C with no hardware calls: the caller puts the buffer on
 * whatever screen it has.
 *
 * Author: Anthropic Claude
 */
#ifndef MINE_CELLS_H
#define MINE_CELLS_H

#include <stdbool.h>
#include <stdint.h>

/* what a square looks like */
#define MINE_CELL_COVERED   0   /* not opened, no mark          */
#define MINE_CELL_FLAG      1
#define MINE_CELL_QUESTION  2
#define MINE_CELL_OPEN      3   /* opened; number is 0 to 8     */
#define MINE_CELL_MINE      4   /* a mine shown at the end      */
#define MINE_CELL_HIT       5   /* the mine that was opened     */
#define MINE_CELL_WRONG     6   /* a flag where there is no mine, shown at the end */

/* Draw one square of size x size pixels into buf (size * size pixels, row by
 * row). number matters only for MINE_CELL_OPEN. cursor adds the frame of the
 * square the player is on. */
void mine_draw_cell(uint16_t *buf, int size, int kind, int number, bool cursor);

#endif /* MINE_CELLS_H */
