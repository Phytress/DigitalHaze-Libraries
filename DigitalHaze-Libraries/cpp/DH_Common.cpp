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

#include "DH_Common.hpp"

#include <stdarg.h>
#include <string.h>

#include "zlib.h"

std::string DigitalHaze::stringprintf(const char* fmtStr, ...) {
	char* message = nullptr;
	int msgLen;

	va_list list;
	va_start(list, fmtStr);
	msgLen = vasprintf(&message, fmtStr, list); // God bless GNU
	va_end(list);

	if (msgLen == -1) return std::string(""); // allocation error
	if (!message) return std::string(""); // I think only *BSD does this

	std::string retStr = message;

	free(message); // vasprintf requires free

	return retStr;
}

void DigitalHaze::displayformatted(void* buf, size_t len) {
	char readableBuf[17];
	unsigned char* packet = (unsigned char*) buf;

	for (size_t pos = 0; pos < len; pos += 16) {
		int i;
		memset(readableBuf, 0, sizeof ( readableBuf));

		printf("%.4zX| ", pos);
		for (i = 0; i < 16 && pos + i < len; ++i) {
			readableBuf[i] = packet[ pos + i ] < 0x20 ? '.' : (char) packet[ pos + i ];
			printf("%.2hhX ", packet[ pos + i ]);
		}
		for (; i < 16; ++i) printf("   ");
		printf("|%s\n", readableBuf);
	}
}

int DigitalHaze::stringcasecmp(std::string str1, std::string str2) {
	return strcasecmp(str1.c_str(), str2.c_str());
}

std::string DigitalHaze::zliberr(int errCode) {
	switch (errCode) {
		case Z_BUF_ERROR: return "Destination buffer not large enough";
			break;
		case Z_MEM_ERROR: return "insufficient memory";
			break;
		case Z_DATA_ERROR: return "compressed data was corrupted";
			break;
	};
	
	return stringprintf("Unknown error code: %d", errCode);
}