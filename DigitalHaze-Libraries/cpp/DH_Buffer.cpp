/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "DH_Buffer.hpp"

#include <exception>
#include <stdexcept>
#include <cstring>
#include <string>

#include <stdarg.h>

DigitalHaze::Buffer::Buffer(size_t sizeInBytes, size_t reallocSize)
	: bufferSize(0), buffer(nullptr) {
	Recreate(sizeInBytes, reallocSize);
}

DigitalHaze::Buffer::~Buffer() {
	// Free any data
	if (buffer != nullptr)
		free(buffer);
}

bool DigitalHaze::Buffer::Read(void* outBuffer, size_t len, size_t offset) {
	if (len + offset > bufferLen || !buffer) return false;
	Peek(outBuffer, len, offset);
	ShiftBufferAtOffset(len, offset);
	return true;
}

bool DigitalHaze::Buffer::Peek(void* outBuffer, size_t len, size_t offset) const {
	if (len + offset > bufferLen || !buffer) return false;
	memcpy(outBuffer, (void*) ((size_t) buffer + offset), len);
	return true;
}

void DigitalHaze::Buffer::Write(void* inBuffer, size_t len, ssize_t insertOffset) {
	if (!buffer) return;

	// If -1, then we insert data to the back of the buffer
	if (insertOffset == -1) insertOffset = (ssize_t) bufferLen;
	
	// Is this offset valid?
	if ((size_t)insertOffset > bufferLen) {
		throw std::overflow_error(
								std::string("DigitalHaze::Buffer::Write cannot write ") + std::to_string(len) +
								std::string(" bytes at offset ") + std::to_string(insertOffset) +
								std::string(" in a buffer of size ") + std::to_string(bufferSize)
								);
	}

	// Do we have enough space?
	if (bufferLen + len > bufferSize) {
		// We need more space
		if (!bufferReallocSize) {
			throw std::overflow_error(
									std::string("DigitalHaze::Buffer::Write ran out of space to store ") + std::to_string(len) +
									std::string(" bytes in a ") + std::to_string(bufferSize) + std::string(" length buffer"));
		}

		// expand in chunks of bufferReallocSize
		ExpandBufferAligned(bufferLen + len - bufferSize);
	}

	// Move current bytes forward
	size_t insertionPoint = (size_t) buffer + insertOffset;
	size_t destinationPoint = insertionPoint + len;
	size_t bytesToForwardShift = bufferLen - insertOffset;
	memmove((void*) destinationPoint, (void*) insertionPoint, bytesToForwardShift);

	// Copy the new bytes in
	memcpy((void*) insertionPoint, inBuffer, len);
	bufferLen += len;
}

void DigitalHaze::Buffer::NotifyWrite(size_t len) {
	if (bufferLen + len > bufferSize || !buffer) {
		throw
		std::overflow_error(
							std::string("DigitalHaze::Buffer notified of a write of ") + std::to_string(len) +
							std::string(" bytes which resulted in an overflow in a ") + std::to_string(bufferSize) +
							std::string(" length buffer")
							);
	}

	bufferLen += len;
}

void DigitalHaze::Buffer::ExpandBuffer(size_t additionalBytes) {
	if (!additionalBytes) additionalBytes = bufferReallocSize;
	if (!additionalBytes)
		throw std::invalid_argument("DigitalHaze::Buffer::NotifyExpand cannot expand by 0");

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

	size_t numBytesExpand = (bufferReallocSize *
							(((bufferSize + additionalBytes) / bufferReallocSize) + 1))
			- bufferSize;
	ExpandBuffer(numBytesExpand);
}

size_t DigitalHaze::Buffer::ReadString(char* outString, size_t maxLen, size_t offset) {
	size_t readLen = PeekString(outString, maxLen, offset);

	if (readLen && readLen != maxLen + 1)
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
		if (stringLength > maxLen)
			return maxLen + 1;
		memcpy(outString, (void*)((size_t)buffer + offset), stringLength + 1);
		outString[stringLength + 1] = 0x00; // This doesn't override the line break
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

	free(message);

	return (size_t) msgLen;
}

size_t DigitalHaze::Buffer::WriteStringAtOffset(size_t offset, const char* fmtStr, ...) {
	char* message = nullptr;
	int msgLen;
	
	va_list list;
	va_start(list, fmtStr);
	msgLen = vasprintf(&message, fmtStr, list);
	va_end(list);
	
	if(msgLen == -1) return 0;
	if(!message) return 0;
	
	Write(message, (size_t)msgLen, offset);
	
	free(message);
	
	return (size_t) msgLen;
}

void DigitalHaze::Buffer::ShiftBufferAtOffset(size_t bytesToShift, size_t offset) {
	if (bytesToShift + offset > bufferLen) {
		throw std::out_of_range(std::string("DigitalHaze::Buffer Cannot shift ") +
								std::to_string(bytesToShift) + std::string(" bytes, at ") +
								std::to_string(offset) + std::string(" offset, only ") +
								std::to_string(bufferLen) + std::string(" bytes in buffer"));
	}

	bufferLen -= bytesToShift;

	size_t shiftDestPos = (size_t) buffer + offset;
	size_t shiftStartPos = shiftDestPos + bytesToShift;
	size_t remainingBytes = bufferLen - offset;

	memmove((void*) shiftDestPos, (void*) shiftStartPos, remainingBytes);
}

void DigitalHaze::Buffer::Recreate(size_t newBufferSize, size_t newBufferReallocSize) {
	if (!newBufferSize)
		throw std::bad_array_new_length();

	if (newBufferSize != bufferSize) {
		buffer = realloc(buffer, newBufferSize);

		if (!buffer)
			throw std::bad_alloc();

		bufferSize = newBufferSize;
	}
	bufferLen = 0;
	bufferReallocSize = newBufferReallocSize;
}

DigitalHaze::Buffer::Buffer(const Buffer& rhs)
	: Buffer(rhs.bufferSize, rhs.bufferReallocSize) {
	// Not too many things other than memory corruption can cause this
	if (!rhs.buffer)
		throw std::invalid_argument("DigitalHaze::Buffer::operator= rhs.buffer is nullptr");

	// Copy the contents of the other buffer
	memcpy(buffer, rhs.buffer, rhs.bufferLen);
	bufferLen = rhs.bufferLen;
}

DigitalHaze::Buffer::Buffer(Buffer&& rhs) noexcept
: bufferLen(rhs.bufferLen), bufferSize(rhs.bufferSize),
bufferReallocSize(rhs.bufferReallocSize), buffer(rhs.buffer) {
	rhs.buffer = nullptr;
	rhs.bufferSize = 0;
	rhs.bufferLen = 0;
	rhs.bufferReallocSize = 0;
}

DigitalHaze::Buffer& DigitalHaze::Buffer::operator=(const Buffer& rhs) {
	// Not too many things other than memory corruption can cause this
	if (!rhs.buffer)
		throw std::invalid_argument("DigitalHaze::Buffer::operator= rhs.buffer is nullptr");

	Recreate(rhs.bufferSize, rhs.bufferReallocSize);
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

	// Remove rhs from existance
	rhs.buffer = nullptr;
	rhs.bufferSize = 0;
	rhs.bufferLen = 0;
	rhs.bufferReallocSize = 0;

	return *this;
}