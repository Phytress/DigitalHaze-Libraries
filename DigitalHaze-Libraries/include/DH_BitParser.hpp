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
 * File:   DH_BitParser.hpp
 * Author: Syed Ali <Syed.Ali@digital-haze.org>
 *
 * Created on September 15, 2017, 1:09 PM
 */

#ifndef DH_BITPARSER_HPP
#define DH_BITPARSER_HPP

#include <cstdlib>
#include <cstddef>

namespace DigitalHaze {

	struct SingleBitDescriptor;
	class BitBufferIterator;

	class BitParser {
	public:
		// Default Constructor
		BitParser(void* buffer, size_t lenInBits) noexcept;
		// Copy Constructor
		BitParser(const BitParser& other) noexcept;
		// Move Constructor
		BitParser(BitParser&& other) noexcept;
		// Destructor
		~BitParser() noexcept;
		// Copy assignment
		BitParser& operator=(const BitParser& other) noexcept;
		// Move assignment
		BitParser& operator=(BitParser&& other) noexcept;

		size_t getLengthInBits() const {
			return lengthInBits;
		}

		void setLengthInBits(size_t len) {
			lengthInBits = len;
		}

		void* getDataPtr() const {
			return (void*) dataPtr;
		}

		void setDataPtr(void* ptr, size_t lenInBits) {
			dataPtr = (unsigned char*) ptr;
			lengthInBits = lenInBits;
			startBitPos = 0;
		}
		void offsetBuffer(ptrdiff_t offsetInBits);

		template<class vType>
		vType ReadBits(size_t nBits = 0);
		template<class vType>
		vType PeekBits(size_t nBits = 0) const;
		template<class vType>
		void WriteBits(vType var, size_t nBits = 0);

		SingleBitDescriptor operator[](ptrdiff_t index) const;
		SingleBitDescriptor operator*() const;
		BitParser& operator++();

		BitBufferIterator begin() const;
		BitBufferIterator end() const;
	private:
		unsigned char* dataPtr;
		size_t lengthInBits;
		size_t startBitPos;
	};

	struct SingleBitDescriptor {
	public:
		bool operator==(bool rhs) const;
		bool operator!=(bool rhs) const;

		operator bool() const;
		operator int() const;

		SingleBitDescriptor& operator=(bool rhs);
		SingleBitDescriptor& operator=(int rhs);
		SingleBitDescriptor& operator=(SingleBitDescriptor& rhs) noexcept;

		SingleBitDescriptor(unsigned char* bPtr, unsigned char bit) noexcept;
	private:
		unsigned char* bytePtr;
		unsigned char bitPos;
	};

	class BitBufferIterator {
	public:
		BitBufferIterator(const BitParser* bptr, size_t bpos) noexcept;

		bool operator!=(BitBufferIterator& other) const;
		SingleBitDescriptor operator*() const;
		BitBufferIterator& operator++();
	private:
		size_t pos;
		const BitParser* bbPtr;
	};

	template<class vType>
	vType BitParser::ReadBits(size_t nBits) {
		if (!nBits || nBits > (sizeof (vType) * 8)) nBits = sizeof (vType) * 8;

		vType retVal;
		retVal = PeekBits<vType>(nBits);
		offsetBuffer(nBits);
		return retVal;
	}

	template<class vType>
	vType BitParser::PeekBits(size_t nBits) const {
		if (!nBits || nBits > (sizeof (vType) * 8)) nBits = sizeof (vType) * 8;

		vType retVal;
		unsigned char* destPtr = (unsigned char*) (void*) &retVal;

		for (std::size_t i = 0; i < sizeof (vType) && nBits; i++) {
			unsigned char byte = 0;
			int readMax = nBits > 7 ? 8 : (int) nBits;
			for (int bit = readMax - 1; bit >= 0; bit--) {
				byte <<= 1;
				byte |= (int) ((*this)[(i * 8) + bit]);
			}
			*destPtr = byte;
			destPtr++;

			nBits -= readMax;
		}

		return retVal;
	}

	template<class vType>
	void BitParser::WriteBits(vType var, size_t nBits) {
		if (!nBits || nBits > (sizeof (vType) * 8)) nBits = sizeof (vType) * 8;
		BitParser srcBuf(&var, 0);

		for (size_t i = 0; i < nBits; i++) {
			(*this)[i] = (bool)srcBuf[i];
		}

		offsetBuffer(nBits);
	}

}

#endif /* DH_BITPARSER_HPP */

