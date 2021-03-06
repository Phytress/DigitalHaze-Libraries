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

#define _GNU_SOURCE

#include "DH_Buffer.hpp"
#include "DH_Common.hpp"

#include <exception>
#include <stdexcept>
#include <cstring>
#include <string>

#include <stdarg.h>
#include <stdio.h>

DigitalHaze::Buffer::Buffer(size_t sizeInBytes, size_t reallocSize, size_t maxSize)
	: bufferSize(0), bufferMaxSize(maxSize), buffer(nullptr) {
	// Our recreate function will allocate us.
	Recreate(sizeInBytes, reallocSize);
}

DigitalHaze::Buffer::~Buffer() {
	// Free any data
	if (buffer != nullptr)
		free(buffer);
}

bool DigitalHaze::Buffer::Read(void* outBuffer, size_t len, size_t offset) {
	// Peek the data, if available.
	if (!Peek(outBuffer, len, offset))
		return false;

	// Discard it
	ShiftBufferAtOffset(len, offset);
	return true;
}

bool DigitalHaze::Buffer::Peek(void* outBuffer, size_t len, size_t offset) const {
	// Invalid offset / not enough data available?
	if (len + offset > bufferLen || !buffer) return false;
	// Copy
	memcpy(outBuffer, (void*) ((size_t) buffer + offset), len);
	return true;
}

void DigitalHaze::Buffer::Write(void* inBuffer, size_t len, ssize_t insertOffset) {
	if (!buffer) return;

	// If -1, then we insert data to the back of the buffer
	if (insertOffset == -1) insertOffset = (ssize_t) bufferLen;

	// Is this offset valid?
	if ((size_t) insertOffset > bufferLen) {
		// Can't write past our end point
		throw
		std::overflow_error(
				stringprintf("DigitalHaze::Buffer::Write cannot write %zu bytes at offset %zd in a buffer of length %zu",
				len, insertOffset, bufferLen)
				);
	}

	// Do we have enough space?
	if (bufferLen + len > bufferSize) {
		// We need more space
		if (!bufferReallocSize) {
			// If we're not allowed to realloc more data, then this would
			// cause a buffer overflow
			throw
			std::overflow_error(
					stringprintf("DigitalHaze::Buffer::Write ran out of space to store %zu bytes in a %zu length buffer",
					len, bufferSize)
					);
		}

		// expand in chunks of bufferReallocSize
		ExpandBufferAligned(bufferLen + len - bufferSize);
	}

	// The address at which we're inserting data
	size_t insertionPoint = (size_t) buffer + insertOffset;
	// The ending address after data has been written
	size_t destinationPoint = insertionPoint + len;
	// The number of bytes to shift forward if inserting data.
	size_t bytesToForwardShift = bufferLen - insertOffset;
	// Shift bytes forward
	memmove((void*) destinationPoint, (void*) insertionPoint, bytesToForwardShift);

	// Copy the new bytes in
	memcpy((void*) insertionPoint, inBuffer, len);
	bufferLen += len;
}

void DigitalHaze::Buffer::NotifyWrite(size_t len) {
	if (bufferLen + len > bufferSize || !buffer) {
		throw
		std::overflow_error(
				stringprintf("DigitalHaze::Buffer::NotifyWrite notified of a write of %zu bytes which resulted in an overflow in a %zu sized buffer",
				len, bufferSize)
				);
	}

	bufferLen += len;
}

void DigitalHaze::Buffer::ExpandBuffer(size_t additionalBytes) {
	if (!additionalBytes) additionalBytes = bufferReallocSize;
	if (!additionalBytes)
		throw std::invalid_argument("DigitalHaze::Buffer::ExpandBuffer cannot expand by 0");
	if (bufferMaxSize && bufferSize + additionalBytes > bufferMaxSize)
		throw std::overflow_error("DigitalHaze::Buffer::ExpandBuffer cannot expand buffer past max size.");

	bufferSize += additionalBytes;
	buffer = realloc(buffer, bufferSize);

	if (!buffer) {
		throw std::bad_alloc();
	}
}

void DigitalHaze::Buffer::ExpandBufferAligned(size_t additionalBytes) {
	if (!bufferReallocSize) {
		ExpandBuffer(additionalBytes);
		return;
	}

	// We always expand at least by one multiple of the reallocSize.
	size_t numBytesExpand = (bufferReallocSize *
			(((bufferSize + additionalBytes) / bufferReallocSize) + 1))
			- bufferSize;
	ExpandBuffer(numBytesExpand);
}

size_t DigitalHaze::Buffer::ReadString(char* outString, size_t maxLen, size_t offset) {
	// Peek the string
	size_t readLen = PeekString(outString, maxLen, offset);

	// Remove from the buffer
	if (readLen && readLen <= maxLen)
		ShiftBufferAtOffset(readLen + 1, offset); //Remove terminating character

	return readLen;
}

size_t DigitalHaze::Buffer::PeekString(char* outString, size_t maxLen, size_t offset) const {
	size_t tokenPos;

	// Find a line break or null terminator
	for (tokenPos = offset; tokenPos < bufferLen; ++tokenPos) {
		unsigned char byte =
				*(unsigned char*) ((size_t) buffer + tokenPos);
		if (byte == 0x00 || byte == 0x0A)
			break;
	}

	// Did we find it in time?
	if (tokenPos > offset && tokenPos < bufferLen) {
		size_t stringLength = tokenPos - offset;
		if (stringLength + 1 > maxLen) // include null terminator
			return stringLength + 1;

		// Copy the string
		memcpy(outString, (void*) ((size_t) buffer + offset), stringLength + 1);

		// Write a null terminator in case the string's provided terminator
		// wasn't a \0.
		outString[stringLength + 1] = 0x00;
		return stringLength;
	}

	// Did not find a string
	return 0;
}

size_t DigitalHaze::Buffer::WriteString(const char* fmtStr, ...) {
	char* message = nullptr;
	int msgLen;

	va_list list;
	va_start(list, fmtStr);
	msgLen = vasprintf(&message, fmtStr, list); // God bless GNU
	va_end(list);

	if (msgLen == -1) return 0; // allocation error
	if (!message) return 0; // I think only *BSD does this

	Write(message, (size_t) msgLen);

	free(message); // vasprintf requires free

	return (size_t) msgLen;
}

size_t DigitalHaze::Buffer::WriteStringAtOffset(size_t offset, const char* fmtStr, ...) {
	char* message = nullptr;
	int msgLen;

	va_list list;
	va_start(list, fmtStr);
	msgLen = vasprintf(&message, fmtStr, list);
	va_end(list);

	if (msgLen == -1) return 0;
	if (!message) return 0;

	Write(message, (size_t) msgLen, offset);

	free(message); // vasprintf requires free

	return (size_t) msgLen;
}

void DigitalHaze::Buffer::ShiftBufferAtOffset(size_t bytesToShift, size_t offset) {
	if (bytesToShift + offset > bufferLen) {
		throw
		std::out_of_range(
				stringprintf("DigitalHaze::Buffer cannot shift %zu bytes at %zu offset, only %zu bytes in buffer.",
				bytesToShift, offset, bufferLen)
				);
	}

	bufferLen -= bytesToShift;

	// Data will be copied to this position/pointer
	size_t shiftDestPos = (size_t) buffer + offset;
	// The source is specified by this pointer
	size_t shiftStartPos = shiftDestPos + bytesToShift;
	// The number of bytes to shift
	size_t remainingBytes = bufferLen - offset;

	memmove((void*) shiftDestPos, (void*) shiftStartPos, remainingBytes);
}

void DigitalHaze::Buffer::Recreate(size_t newBufferSize, size_t newBufferReallocSize,
		size_t maxSize) {
	if (!newBufferSize)
		throw std::bad_array_new_length();
	if (maxSize && newBufferSize < maxSize)
		throw std::invalid_argument("DigitalHaze::Buffer::Recreate newBufferSize is larger than maxSize");

	// Reallocate only if we have to. If the size is the same, then don't bother.
	if (newBufferSize != bufferSize) {
		buffer = realloc(buffer, newBufferSize);

		if (!buffer)
			throw std::bad_alloc();

		bufferSize = newBufferSize;
	}

	// Reset variables.
	bufferLen = 0;
	bufferReallocSize = newBufferReallocSize;
	bufferMaxSize = maxSize;
}

void* DigitalHaze::Buffer::ExportBuffer(size_t& bufLen, size_t& bufSize) {
	bufLen = bufferLen;
	bufSize = bufferSize;
	
	void* retVal = buffer;
	
	buffer = nullptr;
	bufferSize = 0;
	bufferLen = 0;
	bufferReallocSize = 0;
	bufferMaxSize = 0;
	
	return retVal;
}

// Begin rule of 5

DigitalHaze::Buffer::Buffer(const Buffer& rhs)
	: Buffer(rhs.bufferSize, rhs.bufferReallocSize, rhs.bufferMaxSize) {
	// Not too many things other than memory corruption can cause this
	if (!rhs.buffer)
		throw std::invalid_argument("DigitalHaze::Buffer::operator= rhs.buffer is nullptr");

	// Copy the contents of the other buffer
	memcpy(buffer, rhs.buffer, rhs.bufferLen);
	bufferLen = rhs.bufferLen;
}

DigitalHaze::Buffer::Buffer(Buffer&& rhs) noexcept
: bufferLen(rhs.bufferLen), bufferSize(rhs.bufferSize),
bufferReallocSize(rhs.bufferReallocSize),
bufferMaxSize(rhs.bufferMaxSize), buffer(rhs.buffer) {
	rhs.buffer = nullptr;
	rhs.bufferSize = 0;
	rhs.bufferLen = 0;
	rhs.bufferReallocSize = 0;
	rhs.bufferMaxSize = 0;
}

DigitalHaze::Buffer& DigitalHaze::Buffer::operator=(const Buffer& rhs) {
	// Not too many things other than memory corruption can cause this
	if (!rhs.buffer)
		throw std::invalid_argument("DigitalHaze::Buffer::operator= rhs.buffer is nullptr");

	Recreate(rhs.bufferSize, rhs.bufferReallocSize, rhs.bufferMaxSize);
	bufferLen = rhs.bufferLen;
	memcpy(buffer, rhs.buffer, bufferLen);

	return *this;
}

DigitalHaze::Buffer& DigitalHaze::Buffer::operator=(Buffer&& rhs) noexcept {
	if (buffer)
		free(buffer);

	// Copy
	buffer = rhs.buffer;
	bufferLen = rhs.bufferLen;
	bufferSize = rhs.bufferSize;
	bufferReallocSize = rhs.bufferReallocSize;
	bufferMaxSize = rhs.bufferMaxSize;

	// Remove rhs from existance
	rhs.buffer = nullptr;
	rhs.bufferSize = 0;
	rhs.bufferLen = 0;
	rhs.bufferReallocSize = 0;
	rhs.bufferMaxSize = 0;

	return *this;
}