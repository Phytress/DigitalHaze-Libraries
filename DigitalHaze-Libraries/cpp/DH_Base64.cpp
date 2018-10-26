/*
 * The MIT License
 *
 * Copyright 2017 Syed Ali <Syed.Ali@digital-haze.org>.
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
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

#include "DH_Base64.hpp"
#include "DH_BitParser.hpp"

#include <cstring>

unsigned char reverse8(unsigned char b);

size_t DigitalHaze::Base64Encode(void* src, void* dest, size_t srclen) {
	if (!src) return 0;
	if (!dest) return 0;
	if (!srclen) return 0;

	char* destBuf = (char*) dest;
	size_t destPos = 0;
	size_t i;

	// i is incremented at the bottom depending on how many
	// bytes we're reading
	for (i = 0; i < srclen;) {
		unsigned char threebytes[3];
		DigitalHaze::BitParser bytebuf(threebytes, sizeof (threebytes) * 8);

		// Read up to 3 bytes = 24 bits
		// depending on how much data we have left
		int readLen = sizeof (threebytes);
		if (srclen - i < sizeof (threebytes)) {
			// read 1 or 2 bytes
			readLen = (int) (srclen - i);
			// zero the memory as well to prevent memory corruption
			memset(threebytes, 0, sizeof (threebytes));
		}

		// Read
		// We have to flip the bits so they're in order.
		// Base64 reads bits left to right. DigitalHaze::BitParser reads bits right to left
		// So we reverse these bits so they can be read sequentially in DigitalHaze::BitParser
		for (int x = 0; x < readLen; ++x)
			threebytes[x] = reverse8(*((unsigned char*) src + i + x));

		// Read up to 4 sets of 6 bits = 24 bits
		for (int x = 0; x < readLen + 1; ++x) {
			// These 6 bits have to be re-reversed because we had put them
			// out of order when reading it sequentially
			int readVal = reverse8(bytebuf.ReadBits<int>(6)) >> 2;
			destBuf[destPos++] = IntToBase64Char(readVal);
		}

		i += readLen;
	}

	// If we're not divisble by a length of 3, add a padding of = signs
	for (; i % 3; ++i) destBuf[destPos++] = '=';
	destBuf[destPos] = '\0';

	return destPos;
}

size_t DigitalHaze::Base64Decode(void* src, void* dest, size_t srclen) {
	if (!src) return 0;
	if (!dest) return 0;
	if (!srclen) return 0;

	DigitalHaze::BitParser destBBuf(dest, 0);

	for (size_t i = 0; i < srclen; ++i) {
		unsigned char curLetter = *((unsigned char*) src + i);
		//if (curLetter != '=') {
			unsigned char curValue = Base64CharToInt(curLetter);

			curValue = reverse8(curValue) >> 2;

			destBBuf.WriteBits(curValue, 6);
		//}
	}

	size_t len = (unsigned char*) destBBuf.getDataPtr() - (unsigned char*) dest;

	// All of the bits are in reverse because of how Base64 decodes
	for (size_t i = 0; i < len; ++i)
		*((unsigned char*) dest + i) = reverse8(*((unsigned char*) dest + i));

	return len;
}

char DigitalHaze::IntToBase64Char(int num) {
	if (num < 0 || num > 63) return '\0';
	char retChar;

	if (num < 26) retChar = 'A' + num;
	else if (num < 52) retChar = 'a' + (num - 26);
	else if (num < 62) retChar = '0' + (num - 52);
	else if (num == 62) retChar = '+';
	else retChar = '/';

	return retChar;
}

int DigitalHaze::Base64CharToInt(char c) {
	if (c == '+') return 62;
	else if (c == '/') return 63;
	else if (c == '=') return 0;
	else if (c >= 'A' && c <= 'Z')
		return (c - 'A');
	else if (c >= 'a' && c <= 'z')
		return (c - 'a') + 26;
	else if (c >= '0' && c <= '9')
		return (c - '0') + 52;
	return 0;
}

unsigned char reverse8(unsigned char b) {
	b = (b & 0xF0) >> 4 | (b & 0x0F) << 4;
	b = (b & 0xCC) >> 2 | (b & 0x33) << 2;
	b = (b & 0xAA) >> 1 | (b & 0x55) << 1;
	return b;
}