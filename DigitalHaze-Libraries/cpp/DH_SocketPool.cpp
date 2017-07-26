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

#include "DH_SocketPool.hpp"

#include <sys/poll.h>
#include <stdexcept>

DigitalHaze::SocketPool::SocketPool(size_t defaultPoolSize,
		size_t expandSlotSize)
	: ThreadLockedObject(),
	pollfdsBuffer(sizeof (pollfd) * (defaultPoolSize ? defaultPoolSize : DH_SOCKETPOOL_DEFAULTSIZE),
	sizeof (pollfd) * expandSlotSize),
	readListIndex(0), writeListIndex(0), errorListIndex(0) {
}

DigitalHaze::SocketPool::~SocketPool() {
}

void DigitalHaze::SocketPool::AddSocket(IOSocket* ptrSocket, void* ptrParam) {
	// Irresponsible value?
	if (!ptrSocket) return;
	// Duplicate?
	if (-1 != GetListIndexFromFD(ptrSocket->sockfd)) return;

	socketEntry entry;
	entry.pSocket = ptrSocket;
	entry.pParam = ptrParam;
	entry.passiveSocket = false;

	sockList.push_back(entry);
	AddSocketToPollList(ptrSocket->sockfd);
}

void DigitalHaze::SocketPool::AddPassiveSocket(Socket* ptrSocket, void* ptrParam) {
	if (!ptrSocket) return; // Bad pointer?
	if (-1 != GetListIndexFromFD(ptrSocket->sockfd)) return; // Duplicate?

	socketEntry entry;
	entry.pSocket = ptrSocket;
	entry.pParam = ptrParam;
	entry.passiveSocket = true;

	sockList.push_back(entry);
	AddSocketToPollList(ptrSocket->sockfd);
}

bool DigitalHaze::SocketPool::RemoveSocket(Socket* pSocket) {
	// Erase from our pollfds list, if we haven't already
	if (!RemoveSocketFromPollList(pSocket->sockfd)) {
		// This wasn't on our poll list??
		throw std::logic_error("Error socket not in fd list");
	}

	size_t temp = 0;
	if (RemoveSocketFromVector(pSocket, sockList, temp)) {
		// We found the socket in the main vector.
		// We can remove it from our write, read, and error list if it is
		// in them.
		RemoveSocketFromVector(pSocket, readList, readListIndex);
		RemoveSocketFromVector(pSocket, writeList, writeListIndex);
		RemoveSocketFromVector(pSocket, errorList, errorListIndex);

		return true;
	}

	// Did not find the fd
	return false;
}

bool DigitalHaze::SocketPool::PollSockets(int milliSeconds) {
	// Clear our output lists
	readList.clear();
	writeList.clear();
	errorList.clear();

	// Restart at position zero
	readListIndex = 0;
	writeListIndex = 0;
	errorListIndex = 0;

	// Do we even have sockets?
	if (!GetListSize()) return true; // Can't error if we did nothing.

	// This is our poll array
	pollfd* pollfdStartPtr = (pollfd*) pollfdsBuffer.GetBufferStart();

	// Check if we want to write only on sockets that have data in the buffer
	for (pollfd* fdptr = pollfdStartPtr;
		fdptr != pollfdsBuffer.GetBufferEnd(); ++fdptr) {
		ssize_t listIndex = GetListIndexFromFD(fdptr->fd);

		// Error finding the socket?
		if (listIndex < 0)
			throw std::logic_error("Error could not find socket in list");

		// If we're not a passive socket
		if (!sockList[listIndex].passiveSocket) {
			// non-passive sockets are IOSockets, so do the faster static cast.
			IOSocket* sockio = static_cast<IOSocket*> (sockList[listIndex].pSocket);

			// Do we have data in the buffer?
			if (sockio->GetEgressDataLen()) {
				// Then we need to check if we can write data
				fdptr->events |= POLLOUT;
			} else fdptr->events &= ~POLLOUT; // Don't check write capable
		}
	}

	int activeFDs = poll(pollfdStartPtr, GetListSize(), milliSeconds);

	// Error
	if (activeFDs == -1)
		return false;

	// No pending IO?
	if (!activeFDs)
		return true;

	// Go through our poll fd list
	for (pollfd* fdptr = pollfdStartPtr;
		fdptr != pollfdsBuffer.GetBufferEnd(); ++fdptr) {
		ssize_t listIndex = GetListIndexFromFD(fdptr->fd);

		// Checking for supposedly impossible things
		if (listIndex < 0)
			throw std::logic_error("Error could not find socket in list");

		// Read capable?
		if (fdptr->revents & POLLIN)
			readList.push_back(sockList[listIndex]);
		// Write capable?
		if (fdptr->revents & POLLOUT)
			writeList.push_back(sockList[listIndex]);
		// Error?
		if (fdptr->revents & POLLERR
			|| fdptr->revents & POLLNVAL
			|| fdptr->revents & POLLHUP)
			errorList.push_back(sockList[listIndex]);

		// Did we have an event? If so, that's one socket down
		if (fdptr->revents)
			--activeFDs;

		// If there are no more active FDs left, then don't waste cycles.
		if (!activeFDs)
			break;
	}

	return true;
}

DigitalHaze::Socket*
DigitalHaze::SocketPool::GetNextEntryFromList(std::vector<socketEntry>& list,
		size_t& index,
		void** pParam) {
	if (index >= list.size()) return nullptr;
	socketEntry se = list[index++];
	if (pParam) *pParam = se.pParam;
	return se.pSocket;
}

void DigitalHaze::SocketPool::AddSocketToPollList(int sockfd) {
	// This describes how we will be polling the socket
	pollfd socketpollfd;
	socketpollfd.fd = sockfd;
	socketpollfd.events = POLLIN;
	socketpollfd.revents = 0;

	// Store it in our poll list
	pollfdsBuffer.WriteVar(socketpollfd);
}

bool DigitalHaze::SocketPool::RemoveSocketFromPollList(int sockfd) {
	// Iterate through our list of pollfds
	for (pollfd* fdptr = (pollfd*) pollfdsBuffer.GetBufferStart();
		fdptr != pollfdsBuffer.GetBufferEnd(); ++fdptr) {
		if (fdptr->fd == sockfd) {
			// Found. This is the byte position of the structure in our list.
			size_t structPos = (size_t) fdptr - (size_t) pollfdsBuffer.GetBufferStart();

			// And we shift the contents of the buffer
			pollfdsBuffer.ShiftBufferAtOffset(sizeof (pollfd), structPos);
			return true;
		}
	}

	// Not found
	return false;
}

bool DigitalHaze::SocketPool::RemoveSocketFromVector(Socket* pSock,
		std::vector<socketEntry>& list,
		size_t& vectorIndex) {
	for (size_t i = 0; i < list.size(); ++i) {
		// Compare socketfds (not pointers because the Socket may be std::move'd)
		if (list[i].pSocket->sockfd == pSock->sockfd) {
			// We found the socket, erase it.
			list.erase(list.begin() + i);

			// Is this going to mess up what we're iterating through?
			if (vectorIndex > i) --vectorIndex; // fix it
			return true;
		}
	}

	return false;
}

ssize_t DigitalHaze::SocketPool::GetListIndexFromFD(int sockfd) const {
	for (size_t i = 0; i < sockList.size(); ++i) {
		if (sockList[i].pSocket->sockfd == sockfd) {
			// found it!
			return (ssize_t) i;
		}
	}

	return -1;
}