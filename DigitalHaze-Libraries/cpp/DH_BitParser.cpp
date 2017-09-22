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

#include "DH_BitParser.hpp"

namespace DigitalHaze {

	BitParser::BitParser(void* buffer, size_t lenInBits) noexcept
	: dataPtr((unsigned char*) buffer), lengthInBits(lenInBits), startBitPos(0) {
	}

	BitParser::BitParser(const BitParser& other) noexcept
	: dataPtr(other.dataPtr), lengthInBits(other.lengthInBits), startBitPos(other.startBitPos) {
	}

	BitParser::BitParser(BitParser&& other) noexcept
	: dataPtr(other.dataPtr), lengthInBits(other.lengthInBits), startBitPos(other.startBitPos) {
	}

	BitParser::~BitParser() noexcept {
	}

	BitParser& BitParser::operator=(const BitParser& other) noexcept {
		dataPtr = other.dataPtr;
		lengthInBits = other.lengthInBits;
		startBitPos = other.startBitPos;
		return *this;
	}

	BitParser& BitParser::operator=(BitParser&& other) noexcept {
		dataPtr = other.dataPtr;
		lengthInBits = other.lengthInBits;
		startBitPos = other.startBitPos;
		return *this;
	}

	void BitParser::offsetBuffer(ptrdiff_t offsetInBits) {
		startBitPos = (ptrdiff_t) startBitPos + offsetInBits;

		if (startBitPos < 0) {
			// Move back appropriate number of bytes
			dataPtr -= 1 + ((startBitPos - 1) / -8);
			startBitPos &= 7; // Clear negative value
		} else if (startBitPos > 7) {
			// Move forward appropriate number of bytes
			dataPtr += startBitPos / 8;
			startBitPos &= 7; // Remainder after /8
		}

		lengthInBits = (ptrdiff_t) lengthInBits - offsetInBits;
	}

	SingleBitDescriptor BitParser::operator[](ptrdiff_t index) const {
		ptrdiff_t bytePos;
		int bitPos;

		if (index > 0) {
			bytePos = index / 8;
			bitPos = (int) ((index % 8) + startBitPos);
		} else if (index < 0) {
			bytePos = -1 + ((index + 1) / 8);
			bitPos = (int) ((index & 7) + startBitPos); // Remainder after /8 and clear negative
		} else // index == 0
		{
			bytePos = 0;
			bitPos = (int) startBitPos;
		}

		if (bitPos > 7) {
			bitPos -= 8;
			bytePos++;
		}

		return SingleBitDescriptor(dataPtr + bytePos, bitPos);
	}

	SingleBitDescriptor BitParser::operator*() const {
		return SingleBitDescriptor(dataPtr, (unsigned char) startBitPos);
	}

	BitParser& BitParser::operator++() {
		offsetBuffer(1);
		return *this;
	}

	SingleBitDescriptor::SingleBitDescriptor(unsigned char* bptr, unsigned char bit) noexcept
	: bytePtr(bptr), bitPos(bit) {
	}

	SingleBitDescriptor& SingleBitDescriptor::operator=(bool rhs) {
		if (rhs) *bytePtr |= 1 << bitPos;
		else *bytePtr &= ~(1 << bitPos);
		return *this;
	}

	SingleBitDescriptor& SingleBitDescriptor::operator=(int rhs) {
		*this = (bool)!!rhs;
		return *this;
	}

	SingleBitDescriptor& SingleBitDescriptor::operator=(SingleBitDescriptor& rhs) noexcept {
		*this = (bool)rhs;
	}

	bool SingleBitDescriptor::operator==(bool rhs) const {
		return (bool) * this == rhs;
	}

	bool SingleBitDescriptor::operator!=(bool rhs) const {
		return !(*this == rhs);
	}

	SingleBitDescriptor::operator int() const {
		return !!(*bytePtr & (1 << bitPos));
	}

	SingleBitDescriptor::operator bool() const {
		return !!(*bytePtr & (1 << bitPos));
	}

	BitBufferIterator BitParser::begin() const {
		return BitBufferIterator(this, 0);
	}

	BitBufferIterator BitParser::end() const {
		return BitBufferIterator(this, lengthInBits);
	}

	BitBufferIterator::BitBufferIterator(const BitParser* bptr, size_t bpos) noexcept
	: bbPtr(bptr), pos(bpos) {
	}

	bool BitBufferIterator::operator!=(BitBufferIterator& rhs) const {
		return pos != rhs.pos;
	}

	SingleBitDescriptor BitBufferIterator::operator*() const {
		return (*bbPtr)[pos];
	}

	BitBufferIterator& BitBufferIterator::operator++() {
		pos++;
		return *this;
	}

}