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
 * File:   DH_Buffer.hpp
 * Author: phytress
 *
 * Created on July 7, 2017, 7:05 PM
 */

#ifndef DH_BUFFER_HPP
#define DH_BUFFER_HPP

#include <stdlib.h>

namespace DigitalHaze {

	class Buffer {
	public:
		// sizeInBytes: The length of the buffer in bytes.
		// reallocSize:
		//  If the buffer overflows, we reallocate a larger
		//  buffer by allocating this many more bytes. If reallocSize
		//  is zero, std::overflow_error is thrown on overflow.
		// maxSize:
		//  If the buffer is given additional bytes, how big is it allowed
		//  to get? If this value is non-zero and is exceeded, then
		//  std::overflow_error is thrown when trying to make the buffer
		//  too large.
		// throws:
		//   bad_array_new_length on zero sizeInBytes.
		//   bad_alloc on allocation errors.
		//   invalid_argument on non-zero maxSize less than sizeInBytes
		explicit Buffer(size_t sizeInBytes, size_t reallocSize = 0, size_t maxSize = 0);
		~Buffer();

		// Read data into specified buffer.
		// outBuffer: Data is output to this buffer.
		// len: The length of the data to read into the output buffer.
		// offset: An offset from the beginning of the buffer to read from.
		// return: On success, returns true. Returns false if not enough data.
		// throws:
		//   out_of_range if specified offset is invalid.
		bool Read(void* outBuffer, size_t len, size_t offset = 0);

		// Peek data into specified buffer.
		// See Read parameters.
		bool Peek(void* outBuffer, size_t len, size_t offset = 0) const;

		// Write data to the end of the buffer.
		// inBuffer: data from this buffer will be stored.
		// len: the length of data to grab from the input buffer.
		// insertOffset: if not -1, then data is inserted at a specified
		//   position. Otherwise, data is inserted to the end of the buffer.
		// throws:
		//   overflow_error if insertOffset is invalid.
		//   overflow_error if this would result in an overflow and reallocating
		//     more data is not allowed.
		//   bad_alloc on reallocation errors
		void Write(void* inBuffer, size_t len, ssize_t insertOffset = -1);

		// Notify that we wrote to the buffer provided by GetBufferEnd.
		// This will increase the size of the buffer.
		// len: number of bytes written to the buffer.
		// throws: overflow_error if the number of bytes exceeded the length
		//   of the buffer. Ensure space is available. And if not, then expand.
		void NotifyWrite(size_t len);

		// Notify that we want to expand the buffer by this many bytes.
		// additionalBytes: if zero, we use the realloc size provided in initialization.
		// throws:
		//   bad_alloc if there is an allocation failure.
		//   invalid_argument if additionalBytes AND realloc size are zero.
		//   overflow_error if the buffer's maximum allowed size is reached.
		void ExpandBuffer(size_t additionalBytes = 0);

		// Notify that we want to expand the buffer by this many bytes.
		// That number of bytes is rounded up to a multiple of the realloc size.
		// If no realloc size was provided, this function is identical to
		// ExpandBuffer.
		// additionalBytes: if zero, we use the realloc size provided in initialization.
		// throws:
		//   bad_alloc if there is an allocation failure.
		//   invalid_argument if additionalBytes AND realloc size are zero.
		//   overflow_error if the buffer's maximum allowed size is reached.
		void ExpandBufferAligned(size_t additionalBytes = 0);

		// See: Read
		template<class vType>
		inline bool ReadVar(vType& var, size_t offset = 0);

		// See: Peek
		template<class vType>
		inline bool PeekVar(vType& var, size_t offset = 0) const;

		// See: Write
		template<class vType>
		inline void WriteVar(vType var, ssize_t offset = -1);

		// Reads a string from the internal buffer. A string, for this function,
		// is terminated by either a \0 or a \n.
		// outString: where the string will be stored.
		// maxLen: the maximum number of bytes to read, including null terminator.
		// offset: specifies where to begin reading data from.
		// returns:
		//   The length in bytes of the string, OR
		//   0: if there is no string (delimiter not found)
		//   maxLen+1 or larger: if the string is too large, no read occurs.
		size_t ReadString(char* outString, size_t maxLen, size_t offset = 0);

		// Peeks a string from the internal buffer. A string, for this function,
		// is terminated by either a \0 or a \n.
		// See: ReadString
		size_t PeekString(char* outString, size_t maxLen, size_t offset = 0) const;

		// Writes a formatted string into the buffer. Does not write any
		// string terminators (such as \0 or \n) UNLESS specified in fmtStr.
		// fmtStr: Formatted string
		// returns:
		//   number of bytes written
		//   0: allocation or some other error.
		size_t WriteString(const char* fmtStr, ...);

		// Writes a formatted string and inserts it at the specified offset
		// position in our buffer.
		// See: WriteString
		size_t WriteStringAtOffset(size_t offset, const char* fmtStr, ...);

		// Get the maximum capacity of our buffer.

		inline size_t GetBufferSize() const {
			return bufferSize;
		}

		// Get the amount of data we're holding.

		inline size_t GetBufferDataLen() const {
			return bufferLen;
		}

		// Get the amount of bytes we will reallocate to prevent overflows.

		inline size_t GetBufferReallocSize() const {
			return bufferReallocSize;
		}

		// Get the amount of space, in bytes, left in the buffer.

		inline size_t GetRemainingBufferLength() const {
			return bufferSize - bufferLen;
		}

		// Get a pointer to the end of the buffer where new writes happen.

		inline void* GetBufferEnd() const {
			return (void*) ((size_t) buffer + bufferLen);
		}

		// Get a pointer to the start of the buffer where new reads happen.

		inline void* GetBufferStart() const {
			return buffer;
		}

		// Removes bytes from the front of the buffer as if it had been read.
		// bytesToShift: number of bytes to remove.

		inline void ShiftBufferFromFront(size_t bytesToShift) {
			ShiftBufferAtOffset(bytesToShift, 0);
		}

		// Removes bytes from a specified position in the buffer as if it
		// had been read.
		// bytesToShift: number of bytes to remove.
		// offset: position in our buffer.
		// throws: out_of_range on bad offset.
		void ShiftBufferAtOffset(size_t bytesToShift, size_t offset);

		// Recreates AND RESETS the buffer.
		// If a resize is not necessary, then it won't happen, but the length
		// of the buffer is reset to zero (as if the data is no longer there).
		// newBufferSize: Size of the new buffer.
		// newBufferReallocSize:
		//  If the buffer overflows, we reallocate a larger
		//  buffer by allocating this many more bytes. If reallocSize
		//  is zero, std::overflow_error is thrown on overflow.
		// maxSize:
		//  If the buffer is given additional bytes, how big is it allowed
		//  to get? If this value is non-zero and is exceeded, then
		//  std::overflow_error is thrown when trying to make the buffer
		//  too large.
		// throws:
		//  bad_alloc on allocation errors.
		//  bad_array_new_length if the specified size is invalid (0).
		//  invalid_argument if non-zero maxSize is less than newBufferSize
		void Recreate(size_t newBufferSize, size_t newBufferReallocSize = 0,
			size_t maxSize = 0);
		
		inline void ClearData() {
			bufferLen = 0;
		}
		
		void* ExportBuffer(size_t& bufLen, size_t& bufSize);
	private:
		// Length of data active in buffer
		size_t bufferLen;
		// Our maximum size for our buffer
		size_t bufferSize;
		// How many bytes to allocate on overflows (can be zero)
		size_t bufferReallocSize;
		// How large is a buffer allowed to get? (can be zero)
		size_t bufferMaxSize;
		// A malloc'd buffer.
		void* buffer;
	public:
		// Rule of 5

		Buffer(const Buffer& rhs); // copy constructor
		Buffer(Buffer&& rhs) noexcept; // move constructor
		Buffer& operator=(const Buffer& rhs); // assignment
		Buffer& operator=(Buffer&& rhs) noexcept; // move
	};

	template<class vType>
	inline bool Buffer::ReadVar(vType& var, size_t offset) {
		return Read(&var, sizeof (var), offset);
	}

	template<class vType>
	inline bool Buffer::PeekVar(vType& var, size_t offset) const {
		return Peek(&var, sizeof (var), offset);
	}

	template<class vType>
	inline void Buffer::WriteVar(vType var, ssize_t offset) {
		Write(&var, sizeof (var), offset);
	}
}

#endif /* DH_BUFFER_HPP */

