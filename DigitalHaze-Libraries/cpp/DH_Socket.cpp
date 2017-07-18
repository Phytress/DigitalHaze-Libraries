/*
 * The MIT License
 *
 * Copyright 2017 phytress.
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

#include "DH_Socket.hpp"

#include <errno.h>
#include <unistd.h>
#include <utility>

#include <string>
#include <stdarg.h>

DigitalHaze::Socket::Socket() : sockfd(-1), lasterrno(0) {
}

DigitalHaze::Socket::~Socket() {
	// Do not call virtual functions in constructors/destructors
	Socket::CloseSocket();
}

void DigitalHaze::Socket::CloseSocket() {
	if (sockfd != -1) {
		close(sockfd);
	}
	sockfd = -1;
}

void DigitalHaze::Socket::RecordErrno() {
	lasterrno = errno;
}

void DigitalHaze::Socket::RecordErrno(int specificerrno) {
	lasterrno = specificerrno;
}

DigitalHaze::IOSocket::IOSocket() : Socket(),
	readBuffer(DHSOCKETBUFSIZE, DHSOCKETBUFRESIZE),
	writeBuffer(DHSOCKETBUFSIZE, DHSOCKETBUFRESIZE) {
}

DigitalHaze::IOSocket::~IOSocket() {
}

bool DigitalHaze::IOSocket::Read(void* outBuffer, size_t len) {
	if (!Peek(outBuffer, len))
		return false;
	readBuffer.ShiftBufferFromFront(len);
	return true;
}

bool DigitalHaze::IOSocket::Peek(void* outBuffer, size_t len) {
	// If we have data in our buffer, then read directly from it
	if (readBuffer.Peek(outBuffer, len))
		return true;

	// Otherwise we need to block until we get enough data
	if (!PerformSocketRead(len - readBuffer.GetBufferDataLen(), true)) {
		// Error!
		return false;
	}

	// There is no reason for this to return false now, but pass
	// any errors anyway
	return readBuffer.Peek(outBuffer, len);
}

void DigitalHaze::IOSocket::Write(void* inBuffer, size_t len) {
	writeBuffer.Write(inBuffer, len);
}

size_t DigitalHaze::IOSocket::WriteString(const char* fmtStr, ...) {
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

void DigitalHaze::IOSocket::CloseSocket() {
	// Close the fd
	Socket::CloseSocket();

	// We can keep our buffers allocated (until destructed), but
	// we need to tell them that theres no need to keep the old data.
	readBuffer.ShiftBufferFromFront(readBuffer.GetBufferDataLen());
	writeBuffer.ShiftBufferFromFront(writeBuffer.GetBufferDataLen());
}

// copy

DigitalHaze::Socket::Socket(const Socket& rhs)
	: sockfd(rhs.sockfd), lasterrno(rhs.lasterrno) {
}

// move

DigitalHaze::Socket::Socket(Socket&& rhs) noexcept
: sockfd(rhs.sockfd), lasterrno(rhs.lasterrno) {
	rhs.sockfd = -1;
}

// copy

DigitalHaze::Socket& DigitalHaze::Socket::operator=(const Socket& rhs) {
	// Close our current IO fd, if any
	this->CloseSocket();

	sockfd = rhs.sockfd;
	lasterrno = rhs.lasterrno;
	return *this;
}

// move

DigitalHaze::Socket& DigitalHaze::Socket::operator=(Socket&& rhs) noexcept {
	// Close our current IO fd, if any
	this->CloseSocket();

	sockfd = rhs.sockfd;
	lasterrno = rhs.lasterrno;
	rhs.sockfd = -1;

	return *this;
}

DigitalHaze::IOSocket::IOSocket(const IOSocket& rhs)
	: Socket(rhs), readBuffer(rhs.readBuffer), writeBuffer(rhs.writeBuffer) {
}

DigitalHaze::IOSocket::IOSocket(IOSocket&& rhs) noexcept
: Socket(rhs),
readBuffer(std::move(rhs.readBuffer)), writeBuffer(std::move(rhs.writeBuffer)) {
}

DigitalHaze::IOSocket& DigitalHaze::IOSocket::operator=(const IOSocket& rhs) {
	// Copy our socketfd over
	Socket::operator=(rhs);

	// copy buffers
	readBuffer = rhs.readBuffer;
	writeBuffer = rhs.writeBuffer;
	return *this;
}

DigitalHaze::IOSocket& DigitalHaze::IOSocket::operator=(IOSocket&& rhs) noexcept {
	// Move our socketfd over
	Socket::operator=(rhs);

	// move buffers
	readBuffer = std::move(rhs.readBuffer);
	writeBuffer = std::move(rhs.writeBuffer);

	return *this;
}