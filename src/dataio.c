/* Extended Module Player
 * Copyright (C) 1996-2015 Claudio Matsuoka and Hipolito Carraro Jr
 *
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include <errno.h>
#include "common.h"


inline uint8 read8(FILE *f, int *err)
{
	uint8 x = 0xff;

	if (fread(&x, 1, 1, f) != 1 && err != NULL) {
		*err = errno;
	} else if (err != NULL) {
		*err = 0;
	}

	return x;
}

int8 read8s(FILE *f, int *err)
{
	int8 x = 0;

	if (fread(&x, 1, 1, f) != 1 && err != NULL) {
		*err = errno;
	} else if (err != NULL) {
		*err = 0;
	}

	return x;
}

uint16 read16l(FILE *f, int *err)
{
	uint8 x[2] = { 0xff, 0xff };

	if (fread(&x, 1, 2, f) != 2 && err != NULL) {
		*err = errno;
	} else if (err != NULL) {
		*err = 0;
	}

	return ((uint16)x[1] << 8) | x[0];
}

uint16 read16b(FILE *f, int *err)
{
	uint8 x[2] = { 0xff, 0xff };

	if (fread(&x, 1, 2, f) != 2 && err != NULL) {
		*err = errno;
	} else if (err != NULL) {
		*err = 0;
	}

	return ((uint16)x[0] << 8) | x[1];
}

uint32 read24l(FILE *f, int *err)
{
	uint8 x[3] = { 0xff, 0xff, 0xff };

	if (fread(&x, 1, 3, f) != 3 && err != NULL) {
		*err = errno;
	} else if (err != NULL) {
		*err = 0;
	}

	return ((uint32)x[2] << 16) | ((uint32)x[1] << 8) | x[0];
}

uint32 read24b(FILE *f, int *err)
{
	uint8 x[3] = { 0xff, 0xff, 0xff };

	if (fread(&x, 1, 3, f) != 3 && err != NULL) {
		*err = errno;
	} else if (err != NULL) {
		*err = 0;
	}

	return ((uint32)x[0] << 16) | ((uint32)x[1] << 8) | x[2];
}

uint32 read32l(FILE *f, int *err)
{
	uint8 x[4] = { 0xff, 0xff, 0xff, 0xff };

	if (fread(&x, 1, 4, f) != 4 && err != NULL) {
		*err = errno;
	} else if (err != NULL) {
		*err = 0;
	}

	return ((uint32)x[3] << 24) | ((uint32)x[2] << 16) |
					((uint32)x[1] << 8) | x[0];
}

uint32 read32b(FILE *f, int *err)
{
	uint8 x[4] = { 0xff, 0xff, 0xff, 0xff };

	if (fread(&x, 1, 4, f) != 4 && err != NULL) {
		*err = errno;
	} else if (err != NULL) {
		*err = 0;
	}

	return ((uint32)x[0] << 24) | ((uint32)x[1] << 16) |
					((uint32)x[2] << 8) | x[3];
}

uint16 readmem16l(uint8 *m)
{
	uint32 a, b;

	a = m[0];
	b = m[1];

	return (b << 8) | a;
}

uint16 readmem16b(uint8 *m)
{
	uint32 a, b;

	a = m[0];
	b = m[1];

	return (a << 8) | b;
}

uint32 readmem24l(uint8 *m)
{
	uint32 a, b, c;

	a = m[0];
	b = m[1];
	c = m[2];

	return (c << 16) | (b << 8) | a;
}

uint32 readmem24b(uint8 *m)
{
	uint32 a, b, c;

	a = m[0];
	b = m[1];
	c = m[2];

	return (a << 16) | (b << 8) | c;
}

uint32 readmem32l(uint8 *m)
{
	uint32 a, b, c, d;

	a = m[0];
	b = m[1];
	c = m[2];
	d = m[3];

	return (d << 24) | (c << 16) | (b << 8) | a;
}

uint32 readmem32b(uint8 *m)
{
	uint32 a, b, c, d;

	a = m[0];
	b = m[1];
	c = m[2];
	d = m[3];

	return (a << 24) | (b << 16) | (c << 8) | d;
}

#ifndef LIBXMP_CORE_PLAYER

inline void write8(FILE *f, uint8 b)
{
	fputc(b, f);
}

void write16l(FILE *f, uint16 w)
{
	write8(f, w & 0x00ff);
	write8(f, (w & 0xff00) >> 8);
}

void write16b(FILE *f, uint16 w)
{
	write8(f, (w & 0xff00) >> 8);
	write8(f, w & 0x00ff);
}

void write32l(FILE *f, uint32 w)
{
	write8(f, w & 0x000000ff);
	write8(f, (w & 0x0000ff00) >> 8);
	write8(f, (w & 0x00ff0000) >> 16);
	write8(f, (w & 0xff000000) >> 24);
}

void write32b(FILE *f, uint32 w)
{
	write8(f, (w & 0xff000000) >> 24);
	write8(f, (w & 0x00ff0000) >> 16);
	write8(f, (w & 0x0000ff00) >> 8);
	write8(f, w & 0x000000ff);
}

int move_data(FILE *out, FILE *in, int len)
{
	uint8 buf[1024];
	int l;

	do {
		l = fread(buf, 1, len > 1024 ? 1024 : len, in);
		fwrite(buf, 1, l, out);
		len -= l;
	} while (l > 0 && len > 0);

	return 0;
}

#endif
