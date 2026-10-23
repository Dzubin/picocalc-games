#!/bin/sh
# Builds and runs the host tests (on a PC). Run from the project folder:
#     sh tests/run_tests.sh
# On Windows put the MSYS2 ucrt64 bin folder first in PATH.
set -e
T=tests
OUT=${TMPDIR:-/tmp}/games-tests
mkdir -p "$OUT"
gcc -std=c11 -Wall -Wextra -I. $T/test_freecell.c freecell.c -o "$OUT/test_freecell"
gcc -std=c11 -Wall -Wextra -I. $T/test_klondike.c klondike.c -o "$OUT/test_klondike"
"$OUT/test_klondike"
"$OUT/test_freecell"
gcc -std=c11 -Wall -Wextra -I. $T/test_minesweeper.c minesweeper.c -o "$OUT/test_minesweeper"
"$OUT/test_minesweeper"
