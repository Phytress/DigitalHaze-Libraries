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

/* 
 * File:   DH_SocketPool.hpp
 * Author: phytress
 *
 * Created on July 14, 2017, 4:15 PM
 */

#ifndef DH_SOCKETPOOL_HPP
#define DH_SOCKETPOOL_HPP

#include "DH_ThreadLockedObject.hpp"

#include "DH_Socket.hpp"
#include "DH_Buffer.hpp"

#include <sys/poll.h>
#include <vector>

#define DH_SOCKETPOOL_DEFAULTSIZE 50

namespace DigitalHaze {

	struct socketEntry {
		Socket* pSocket;
		void* pParam;
		bool passiveSocket;
	};

	class SocketPool : public ThreadLockedObject {
	public:
		// The default pool size is how many clients we're anticipating.
		// If that maximum is reached, we allocate more space for
		// a number of clients specified by expandSlotsSize.
		// If expandSlotsSize is zero, then no additional space is allocated
		// and an overflow error is thrown.
		// If the pool size is 0, then a default size of 50 is used.
		// This is defined by DH_SOCKETPOOL_DEFAULTSIZE.
		explicit SocketPool(size_t defaultPoolSize = DH_SOCKETPOOL_DEFAULTSIZE,
							size_t expandSlotsSize = DH_SOCKETPOOL_DEFAULTSIZE);
		~SocketPool();

		// Adds a socket to the list. pParam is an optional parameter.
		// This pointer is given along with the socket when any activity
		// is detected.
		void AddSocket(IOSocket* pSocket, void* pParam = nullptr);
		// Adds a passive socket to the list.
		void AddPassiveSocket(Socket* pSocket, void* pParam = nullptr);

		// Removes a socket from the list.
		bool RemoveSocket(Socket* pSocket);

		// Returns false on error. Returns true if sockets are ready
		// to be worked with. Blocks for the specified number of milliseconds.
		// If the milliseconds is negative, then the function will block
		// until interrupted or until something in our list is ready for IO.
		// A value of zero will not block.
		bool PollSockets(int milliSeconds = 0);

		// Returns the next readable socket after polling.
		// Returns null if there are no more readable sockets.
		// If pParam is not null, then the socket's associated pointer is filled.

		inline Socket* GetNextReadableSocket(void** pParam = nullptr) {
			return GetNextEntryFromList(readList, readListIndex, pParam);
		}

		// Returns the next writable socket after polling.
		// Returns null if there are no more writable sockets.
		// If pParam is not null, then the socket's associated pointer is filled.
		// Writable sockets are only IOSockets and not passive.

		inline IOSocket* GetNextWritableSocket(void** pParam = nullptr) {
			return static_cast<IOSocket*> (GetNextEntryFromList(writeList, writeListIndex, pParam));
		}

		// Returns the next socket that had an error on it after polling.
		// Returns null if there are no more error'd sockets.
		// If pParam is not null, then the socket's associated pointer is filled.

		inline Socket* GetNextErroredSocket(void** pParam = nullptr) {
			return GetNextEntryFromList(errorList, errorListIndex, pParam);
		}

		// Returns the number of sockets in this pool

		inline size_t GetListSize() const {
			return sockList.size();
		}
	private:
		// Our list of sockets
		std::vector<socketEntry> sockList;

		// Our array that we poll with.
		// We keep this allocated and updated so we can poll without
		// having to allocate every time.
		Buffer pollfdsBuffer;

		// List that contains what sockets can be read from
		std::vector<socketEntry> readList;
		size_t readListIndex;

		// List that contains what sockets we can write to
		std::vector<socketEntry> writeList;
		size_t writeListIndex;

		// List that contains what sockets have errored.
		std::vector<socketEntry> errorList;
		size_t errorListIndex;

		// Add a socket to the poll list
		void AddSocketToPollList(int sockfd);

		// Remove a socket from the poll list.
		// Returns false if the socket file descriptor is not found
		bool RemoveSocketFromPollList(int sockfd);

		// Remove a socket from the specified list.
		static bool RemoveSocketFromVector(Socket* pSock,
										std::vector<socketEntry>& list,
										size_t& vectorIndex);

		// Gets the next socketEntry from the specified list
		static Socket* GetNextEntryFromList(std::vector<socketEntry>& list,
											size_t& index,
											void** pParam = nullptr);

		// Returns the index inside sockList where sockfd occurs.
		// If it is not found, -1 is returned.
		ssize_t GetListIndexFromFD(int sockfd) const;
	};
}

#endif /* DH_SOCKETPOOL_HPP */

