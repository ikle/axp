/*
 * AXP opcode 12 test
 *
 * Copyright (c) 2021-2024 Alexei A. Smekalkine <ikle@ikle.ru>
 *
 * SPDX-License-Identifier: BSD-2-Clause
 */

#include <stdint.h>
#include <stdio.h>

static uint64_t axp_zap (uint64_t x, unsigned mask)
{
	uint64_t m, bm;
	int i;

	for (m = 0, bm = 0xff, i = 1; i < 0x100; bm <<= 8, i <<= 1)
		if ((mask & i) != 0)
			m |= bm;

	return x & ~m;
}

static unsigned axp_byte_mask (int f)
{
	switch (f & 0x4f) {
	case 0x46:  return 0x00ff;	// WTF?
	}

	switch (f & 0x30) {
	case 0x00:  return 0x0001;
	case 0x10:  return 0x0003;
	case 0x20:  return 0x000f;
	default:    return 0x00ff;
	}
}

uint64_t axp_msk_0 (int f, uint64_t a, uint64_t b)
{
#if 0
	unsigned bs = b & 7;
	unsigned m  = axp_byte_mask (f) << bs;
	unsigned sm = (f & 0x40) ? m >> 8 : m;
#else
	unsigned m  = axp_byte_mask (f);
	unsigned bs = b & 7;
	unsigned bn = (f & 0x40) ? 8 - bs      : bs;
	unsigned sm = (f & 0x40) ? m >> bn     : m << bn;
#endif
	return axp_zap (a, sm);
}

uint64_t axp_ins_0 (int f, uint64_t a, uint64_t b)
{
#if 0
	unsigned bs = b & 7;
	unsigned m  = axp_byte_mask (f) << bs;
	unsigned sm = (f & 0x40) ? m >> 8      : m;
	unsigned bl = (f & 0x40) ? 64 - bs * 8 : bs * 8;
	uint64_t as = (f & 0x40) ? a >> bl     : a << bl;
#else
	unsigned m  = axp_byte_mask (f);
	unsigned bs = b & 7;
	unsigned bn = (f & 0x40) ? 8 - bs      : bs;
	unsigned sm = (f & 0x40) ? m >> bn     : m << bn;
	uint64_t as = (f & 0x40) ? a >> bn * 8 : a << bn * 8;
#endif
	return axp_zap (as, ~sm);
}

uint64_t axp_ext_0 (int f, uint64_t a, uint64_t b)
{
	unsigned m  = axp_byte_mask (f);
	unsigned bs = b & 7;
#if 0
	unsigned bl = (f & 0x40) ? 64 - bs * 8 : bs * 8;
	uint64_t as = (f & 0x40) ? a << bl     : a >> bl;
#else
	unsigned bn = (f & 0x40) ? 8 - bs      : bs;
	uint64_t as = (f & 0x40) ? a << bn * 8 : a >> bn * 8;
#endif
	return axp_zap (as, ~m);
}

#define F(x)	((f >> (x)) & 1)

#define F0	F (0)
#define F1	F (1)
#define F2	F (2)
#define F3	F (3)
#define F4	F (4)
#define F5	F (5)
#define F6	F (6)

static inline uint64_t axp_srn (int h, int l, int s, uint64_t a, uint64_t b)
{
	const uint64_t al = l ? a : 0;
	const uint64_t ah = s ? (int64_t) a >> 63 : h ? a : 0;

	return ah << (-b & 63) | al >> (b & 63);  /* 128-bit right shift */
}

static inline uint64_t axp_sr (int f, uint64_t a, uint64_t b, int pass)
{
	const int l = F2;		/* use Al = A	*/
	const int h = F3;		/* use Ah = A	*/
	const int s = F2 & F3;		/* sign extend	*/

	return pass ? a : axp_srn (h, l, s, a, b);
}

static inline uint64_t axp_ins_pm (int f, uint64_t a, uint64_t b)
{
	return axp_sr (f, axp_zap (a, ~axp_byte_mask (f)), b * 8, 0);
}

static uint8_t axp_ms (int f, uint64_t b, int p)
{
	const int n = F0 &  p;		/* preinvert byte mask		*/
	const int h = F6 & !F3;		/* select high mask		*/
	const int s = F0 |  p;		/* shift byte mask		*/
	const int x = F1;		/* do msk/ext/ins		*/
	const int z = F0 | !p;		/* postinvert byte mask		*/

	uint16_t m  = axp_byte_mask (f);
	uint16_t mn = n  ? m ^ 0x00ff : m;  /* invert whole 8-bit mask */
	uint8_t  mh = h  ? mn << (b & 7) >> 8 : mn << (b & 7);
	uint8_t  ms = s  ? mh : m;
	uint8_t  mx = x  ? ms : p ? b : ~0;

	return z ? ~mx : mx;
}

static inline uint64_t axp_mei (int f, uint64_t a, uint64_t bs, uint64_t bm)
{
	const int p = !(F2 | F3);	/* pass A (zap/msk block)	*/

	const uint64_t as = axp_sr (f, a, F1 ? bs * 8 : bs, p);
	const uint8_t  ms = axp_ms (f, bm, p);

	return axp_zap (as, ms);
}

static uint64_t axp_shift (int f, uint64_t a, uint64_t b)
{
	const int dc = b;  /* don't care */

	switch (f & 0x0f) {
	case 0x00:  return axp_mei (f, a, dc    ,  b    );	// zap
	case 0x01:  return axp_mei (f, a, dc    ,  b    );	// zapnot
	case 0x02:  return axp_mei (f, a,  b    ,  b    );	// msk
	case 0x03:  return axp_mei (f, a,  b    ,  b    );	// -
	}

	switch (f & 0x4f) {
	case 0x04:  return axp_mei (f, a,  b    , dc    );	// srl
	case 0x05:  return axp_mei (f, a, -b    , dc    );	// sll.high
	case 0x06:  return axp_mei (f, a,  b    ,  b    );	// extl
	case 0x07:  return axp_mei (f, a, -b    , -b    );	// -

	case 0x08:  return axp_mei (f, a,  b    , dc    );	// srl.frac
	case 0x09:  return axp_mei (f, a, -b    , dc    );	// sll
	case 0x0a:  return axp_mei (f, a,  b    , -b    );	// -
	case 0x0b:  return axp_mei (f, a, -b    ,  b    );	// insl

	case 0x0c:  return axp_mei (f, a,  b    , dc    );	// sra
	case 0x0d:  return axp_mei (f, a, -b    , dc    );	// sra.frac = srl.frac
	case 0x0e:  return axp_mei (f, a,  b    ,  b    );	// -
	case 0x0f:  return axp_mei (f, a, -b    ,  b    );	// -

	case 0x44:  return axp_mei (f, a,  b + 1, dc    );	// -
	case 0x45:  return axp_mei (f, a, -b    , dc    );	// -
	case 0x46:  return axp_mei (f, a,  b    ,  b    );	// -
	case 0x47:  return axp_mei (f, a, -b    ,  b    );	// insh

	case 0x48:  return axp_mei (f, a,  b    , dc    );	// -
	case 0x49:  return axp_mei (f, a, ~b    , dc    );	// -
	case 0x4a:  return axp_mei (f, a,  b    ,  b    );	// exth
	case 0x4b:  return axp_mei (f, a, -b    ,  b    );	// -

	case 0x4c:  return axp_mei (f, a,  b    , dc    );	// -
	case 0x4d:  return axp_mei (f, a, ~b    , dc    );	// -
	case 0x4e:  return axp_mei (f, a,  b    ,  b    );	// -
	case 0x4f:  return axp_mei (f, a, -b    ,  b    );	// -, 7F broken, mask = 0
	}

	return 0;
}

int main (int argc, char *argv[])
{
//	const uint64_t a = 0x0fedcba987654321ull, b = 3;
	const uint64_t a = 0x8fedcba987654321ull, b = 11;
	unsigned op;

	printf ("# a = %016lx, b = %ld\n\n", a, b);

	for (op = 0; op < 0x80; ++op)
		printf ("%2X %016lx\n", op, axp_shift (op, a, b));

	return 0;
}
