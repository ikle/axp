/*
 * AXP opcode 11 test
 *
 * Copyright (c) 2021-2022 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdint.h>
#include <stdio.h>

/* opcode 11, bitwise operations and conditional move (w/o amask/implver) */

static inline uint64_t axp_logic (int f, uint64_t a, uint64_t b, uint64_t c)
{
	uint64_t bb  = (f & 0x08) ? ~b : b;		/* 08 - invert	*/

	/* in xor branch: add amask (x61) and implver (x6c) */

	uint64_t bitop = (f & 0x40) ? a ^ bb :		/* 40 - xor	*/
			 (f & 0x20) ? a | bb : a & bb;	/* 20 - or	*/

	int lt   = (f & 0x40) && ((int64_t) a < 0);	/* 40 - lt	*/
	int eq   = (f & 0x20) && (a == 0);		/* 20 - eq	*/
	int lbs  = (f & 0x10) ? (a & 1) : (eq || lt);	/* 10 - lbs	*/
	int inv  = (f & 2) && ((f & 4) || !(f & 0x10));
	int cond = inv ? !lbs : lbs;

	uint64_t cmov = cond ? b : c;

	return (f & 0x06) ? cmov : bitop;		/* 02/04 - cmov	*/
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
