/*
 * AXP opcode 11 test
 *
 * Copyright (c) 2021-2024 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdint.h>
#include <stdio.h>

static inline int axp_cond (int lbs, int eq, int lt, int inv, uint64_t a)
{
	const int e = eq & (a == 0);
	const int l = lt & ((a >> 63) & 1);
	const int c = lbs ? (a & 1) : (e | l);

	return inv ^ c;
}

/* opcode 11, bitwise operations and conditional move */

static inline uint64_t axp_logic (int f, uint64_t a, uint64_t b, uint64_t c)
{
	const uint64_t amask = 1;			/* bwx		*/
	const uint64_t impl  = 1;			/* 21164*	*/

	uint64_t bb  = (f & 0x08) ? ~b : b;		/* 08 - invert	*/

	uint64_t bitop = (f & 0x40) ? a ^ bb :		/* 40 - xor	*/
			 (f & 0x20) ? a | bb : a & bb;	/* 20 - or	*/

	const int lbs = (f & 0x10) != 0;		/* 10 - lbs	*/
	const int eq  = (f & 0x20) != 0;		/* 20 - eq	*/
	const int lt  = (f & 0x40) != 0;		/* 40 - lt	*/

	int inv  = (f & 2) && ((f & 4) || !(f & 0x10));
	int cmov = (f & 4) || (f & 2);			/* 02/04 - cmov	*/

	const int cond = axp_cond (lbs, eq, lt, inv, a) || !cmov;

	uint64_t r  = cmov ? b : bitop;
	uint64_t rm = (f & 0x61) == 0x61 ? r & ~amask : r;
	uint64_t cb = (f & 0x08) ? impl : c;		/* 08 - implver	*/

	return cond ? rm : cb;
}

int main (int argc, char *argv[])
{
	const uint64_t a = 0x0fedcba987654321ull, b = 3, c = 1;
	unsigned f;
	uint64_t r;

	printf ("# a = %016lx, b = %ld\n\n", a, b);

	for (f = 0; f < 0x80; ++f) {
		r = axp_logic (f, a, b, c);

		printf ("%2X %016lx\n", f, r);
	}

	return 0;
}
