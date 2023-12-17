/*
 * AXP opcode 10 test
 *
 * Copyright (c) 2021-2023 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdint.h>
#include <stdio.h>

static inline char mp_digit_add (uint64_t *r, uint64_t x, uint64_t y)
{
	*r = x + y;
	return *r < x;
}

static inline char mp_digit_adc (uint64_t *r, uint64_t x, uint64_t y, int c)
{
	uint64_t a;

	c = mp_digit_add (&a, y, c);
	return c + mp_digit_add (r, x, a);
}

static inline int axp_byte_carry (int a, int b, int ci)
{
	return ((a & 0xff) + (b & 0xff) + ci) >> 8;
}

static inline uint64_t axp_cmpbge (int f, uint64_t a, uint64_t bb, int ci)
{
	return	axp_byte_carry (a >>  0, bb >>  0, ci) << 0 |
		axp_byte_carry (a >>  8, bb >>  8,  1) << 1 |
		axp_byte_carry (a >> 16, bb >> 16,  1) << 2 |
		axp_byte_carry (a >> 24, bb >> 24,  1) << 3 |
		axp_byte_carry (a >> 32, bb >> 32,  1) << 4 |
		axp_byte_carry (a >> 40, bb >> 40,  1) << 5 |
		axp_byte_carry (a >> 48, bb >> 48,  1) << 6 |
		axp_byte_carry (a >> 56, bb >> 56,  1) << 7;
}

/* opcode 10, adder and comparator (with cmpbge) */

static inline uint64_t axp_adder (int f, uint64_t a, uint64_t b)
{
	int      cin = (f & 0x01);			/* 01 - carry	*/
	uint64_t as  = (f & 0x10) ? (a  << 1) : a;	/* 10 - shift 1	*/
	uint64_t ass = (f & 0x02) ? (as << 2) : a;	/* 02 - shift 2	*/
	uint64_t bb  = (f & 0x08) ? ~b : b;		/* 08 - invert	*/

	uint64_t sum;
	int     co = mp_digit_adc (&sum, ass, bb, cin);
	int64_t ss = (f & 0x20) ? sum : (int32_t) sum;	/* 20 - !sext	*/

	int lt  = (f & 0x40) && (sum <  0);		/* 40 - lt	*/
	int eq  = (f & 0x20) && (sum == 0);		/* 20 - eq	*/
	int ult = (f & 0x10) && (!co);			/* 10 - ult	*/

	int cmp = (f & 0x02) ? axp_cmpbge (f, a, bb, cin) : (ult | eq | lt);

	return (f & 0x04) ? cmp : ss;			/* 04 - cmp	*/
}

int main (int argc, char *argv[])
{
	const uint64_t a = 0x0fedcba987654321ull, b = 3;
	unsigned f;
	uint64_t c;

	printf ("# a = %016lx, b = %ld\n\n", a, b);

	for (f = 0; f < 0x80; ++f) {
		c = axp_adder (f, a, b);

		printf ("%2X %016lx\n", f, c);
	}

	return 0;
}
