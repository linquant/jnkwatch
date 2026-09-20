#pragma once
#include <stdint.h>

/* 5x7 dot-matrix glyph lookup (Casio-style LCD text).
 * Fills rows[0..6] top-to-bottom; each row is a 5-bit mask where bit 4 (0x10)
 * is the leftmost column and bit 0 the rightmost.
 * Supports A-Z, 0-9, space and '-'. Unknown characters -> blank (all zero). */
void wf_dot5x7(char c, uint8_t rows[7]);
