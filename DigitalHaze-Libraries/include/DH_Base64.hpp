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

/* 
 * File:   DH_Base64.hpp
 * Author: Syed Ali <Syed.Ali@digital-haze.org>
 *
 * Created on September 16, 2017, 4:22 PM
 */

#ifndef DH_BASE64_HPP
#define DH_BASE64_HPP

#include <cstdlib>

namespace DigitalHaze {
	// Converts an int (first 6 bits only) to a base64 char.
	// If the int is less than 0 or above 63, then 0x00 is returned
	char IntToBase64Char(int num);

	// Converts a single base64 char to an int
	// If the char is invalid OR a '=' (which is valid), it returns zero
	int Base64CharToInt(char c);

	// Converts the src binary data (of length len) to a Base64 string stored in dest.
	// A null terminator is stored in dest as well. Returns the length of the string
	size_t Base64Encode(void* src, void* dest, size_t srclen);

	// Converts the src base64 data (of length len) to the decoded values, stored in dest.
	// A null terminator is stored in dest as well. Returns the length of the data
	size_t Base64Decode(void* src, void* dest, size_t srclen);
}

#endif /* DH_BASE64_HPP */

