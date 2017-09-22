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

#include "DH_TCPSocket.hpp"

#include <unistd.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <errno.h>

DigitalHaze::TCPSocket::TCPSocket() : IOSocket() {
}

DigitalHaze::TCPSocket::TCPSocket(int connectedfd) : IOSocket() {
	IOSocket::sockfd = connectedfd;
}

DigitalHaze::TCPSocket::~TCPSocket() {
}

bool DigitalHaze::TCPSocket::PerformSocketRead(size_t len, bool flush) {
	// Check how many bytes to read.
	// If not specified, then read as many as we can!
	if (!len) {
		len = IOSocket::readBuffer.GetRemainingBufferLength();
		// If we have no more space left, then read how much we'll allocate
		if (!len) {
			len = IOSocket::readBuffer.GetBufferReallocSize();

			// Expand by our realloc size. If realloclen ends up being zero,
			// this will throw an error.
			IOSocket::readBuffer.ExpandBuffer();
		}
	}

	// Make sure we have enough space.
	if (len > IOSocket::readBuffer.GetRemainingBufferLength()) {
		// We don't have enough space, expand
		IOSocket::readBuffer
				.ExpandBuffer(len - IOSocket::readBuffer.GetRemainingBufferLength());
	}

	// read
	ssize_t nBytes = recv(Socket::sockfd,
			readBuffer.GetBufferEnd(), len,
			flush ? MSG_WAITALL : MSG_DONTWAIT);

	// error?
	if (nBytes <= 0) {
		Socket::RecordErrno();
		if (!flush && (Socket::lasterrno == EAGAIN ||
			Socket::lasterrno == EWOULDBLOCK)) {
			// If we're not flushing, then these errors are okay.
			// We just accomplished nothing instead.
			//return true;
		}
		return false;
	}

	// If we're flushing, we better have gotten everything we wanted
	if (flush && (size_t) nBytes != len) {
		Socket::RecordErrno();
		return false;
	}

	// Read successful
	IOSocket::readBuffer.NotifyWrite((size_t) nBytes);

	return true;
}

bool DigitalHaze::TCPSocket::PerformSocketWrite(bool flush) {
	// Can't write to the socket if we have no data
	if (!IOSocket::writeBuffer.GetBufferDataLen()) return false;

	size_t totalWritten = 0;

	do {
		// Attempt send
		ssize_t nBytes = send(
				IOSocket::sockfd,
				(void*) ((size_t) writeBuffer.GetBufferStart() + totalWritten),
				IOSocket::writeBuffer.GetBufferDataLen() - totalWritten,
				flush ? 0 : MSG_DONTWAIT);

		if (nBytes <= 0) {
			Socket::RecordErrno();
			return false;
		}

		totalWritten += (size_t) nBytes;

		// Repeat until fully sent if flushing
	} while (flush && totalWritten < IOSocket::writeBuffer.GetBufferDataLen());

	// Remove the data we just wrote.
	IOSocket::writeBuffer.ShiftBufferFromFront(totalWritten);

	return true;
}

bool DigitalHaze::TCPSocket::isConnected() const {
	return IOSocket::sockfd != -1;
}

bool DigitalHaze::TCPSocket::ConvertAddrToText(TCPAddressStorage& addr,
		socklen_t len,
		char* outText) {
	void* paddr;

	switch (addr.sa.sa_family) {
		case AF_INET:
			paddr = &addr.sa_ip4.sin_addr;
			break;
		case AF_INET6:
			paddr = &addr.sa_ip6.sin6_addr;
			break;
		default: // Don't know what kind of address this is.
			return false;
	};

	return nullptr != inet_ntop(addr.sa.sa_family, paddr, outText, len);
}