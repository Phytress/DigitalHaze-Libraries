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

#include "DH_TCPServerSocket.hpp"
#include "DH_Buffer.hpp"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>

#include <cstring>
#include <utility>

#include <stdio.h>
#include <errno.h>
#include <pthread.h>

enum ListenerThreadStatusCode {
	UNKNOWN = 0,
	STARTED,
	NEWCONNECTION,
	FAILED
};

DigitalHaze::TCPServerSocket::TCPServerSocket()
	: Socket(), ThreadLockedObject(),
	listenerThreadStatus(ListenerThreadStatusCode::UNKNOWN),
	waitNewConnectionCond(PTHREAD_COND_INITIALIZER) {
}

DigitalHaze::TCPServerSocket::~TCPServerSocket() {
	CloseCurrentThreads();
}

void DigitalHaze::TCPServerSocket::CloseSocket() {
	CloseCurrentThreads();
	Socket::CloseSocket();
}

bool DigitalHaze::TCPServerSocket::CreateListener(unsigned short port) {
	this->CloseSocket(); // Close current threads and socket

	// We need an address to bind on. So we getaddrinfo on NULL to retrieve
	// what we can bind on. Unfortunately it will list ipv4 before ipv6 as
	// of this writing, so it may be better to change it to AF_INET6 depending
	// on what we're intending to do.
	addrinfo hints, *servinfo;
	memset(&hints, 0, sizeof (hints));
	hints.ai_family = AF_UNSPEC; // IPv4 or v6
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_NUMERICSERV | AI_PASSIVE;

	// Gotta have that port number
	char portNumber[8];
	snprintf(portNumber, sizeof (portNumber), "%hu", port);

	// Look us up
	if (0 != getaddrinfo(nullptr, portNumber, &hints, &servinfo)) {
		// We don't exist?
		Socket::RecordErrno();
		return false;
	}

	// Get a new listener
	int newsockfd = -1;

	// We need to try to bind to an address provided
	for (addrinfo* ipResult = servinfo; ipResult; ipResult = ipResult->ai_next) {
		newsockfd = socket(ipResult->ai_family, // could be v4 or v6
				ipResult->ai_socktype, // should be SOCK_STREAM
				ipResult->ai_protocol); // Should be TCP

		if (newsockfd == -1) {
			Socket::RecordErrno();
			continue; // Lets try again
		}

		// Returns -1 on error, but we're checking for success
		if (0 == bind(newsockfd, ipResult->ai_addr, ipResult->ai_addrlen)) {
			// Successful on bind
			// Attempt to listen
			if (0 == listen(newsockfd, SOMAXCONN)) {
				// Successful!
				break;
			}
		}

		// Error binding
		Socket::RecordErrno();
		close(newsockfd);
		newsockfd = -1;
	}

	freeaddrinfo(servinfo);

	// Were we successful?
	if (newsockfd != -1) {
		// Store
		sockfd = newsockfd;
		return true;
	}

	return false; // No address worked
}

struct threadedacceptdata {
	DigitalHaze::TCPServerSocket* parent;
};

// Cleanup to free our thread's allocated parameters

void CleanupListenerThread(void* data) {
	if (data) {
		threadedacceptdata* tad = (threadedacceptdata*) data;
		delete tad;
	}
}

// Cleanup in case we got a valid connection, but the thread was canceled.

void CleanupNewConnectionSocket(void* data) {
	if (data) {
		int fd = *(int*) data; // We don't mind losing precision
		if (fd != -1) {
			close(fd);
		}
	}
}

void* DigitalHaze::ListenerThread(void* data) {
	if (!data) {
		// Someone tried to start this thread improperly
		pthread_exit(nullptr);
		return nullptr;
	}

	// These are the default values for a thread. However, it may prove
	// to be useful in the future.
	// pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,nullptr);
	// pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,nullptr);

	// There is allocated data passed to us.
	// If the thread is canceled, exits prematurely, or exits properly,
	// we clean up the passed data using our cleanup function outside
	// this function. (Since the data came from outside of this function).
	pthread_cleanup_push(CleanupListenerThread, data);

	threadedacceptdata* tad = (threadedacceptdata*) data;
	int listenerfd;

	// Get our listener
	tad->parent->LockObject();
	listenerfd = tad->parent->Thread_GetSockFD();

	// Is the listener invalid for whatever reason?
	if (listenerfd == -1) {
		tad->parent->listenerThreadStatus = ListenerThreadStatusCode::FAILED;
		tad->parent->UnlockObject();
		pthread_exit(nullptr);
		return nullptr; // Just in case for undefined behavior
	}
	tad->parent->UnlockObject();

	for (;;) {
		TCPAddressStorage remoteAddr;
		socklen_t remoteAddrLen = sizeof (remoteAddr);

		int newSocketfd = accept(listenerfd, (sockaddr*) & remoteAddr, &remoteAddrLen);

		// Did we error?
		if (newSocketfd == -1) {
			// Double check to make sure we should still be running..
			int currentErrno = errno; // cache errno
			pthread_testcancel();

			// Record the error in our parent
			tad->parent->LockObject();
			tad->parent->Thread_RecordErrno(currentErrno);
			tad->parent->listenerThreadStatus = ListenerThreadStatusCode::FAILED;
			tad->parent->UnlockObject();
			pthread_exit(nullptr);
			return nullptr;
		}

		// New connection!!

		// First, check if our thread was canceled. If it was, we will
		// need to close this new connection.
		pthread_cleanup_push(CleanupNewConnectionSocket, &newSocketfd);
		pthread_testcancel();
		pthread_cleanup_pop(0); // Do not execute the cleanup if we can help it

		// Notify the parent about the new connection
		tad->parent->LockObject();
		tad->parent->Thread_NotifyNewClient(newSocketfd, &remoteAddr, remoteAddrLen);
		tad->parent->listenerThreadStatus = ListenerThreadStatusCode::NEWCONNECTION;

		// Halt our thread until the parent thread retrieves new client info.
		// Then we can begin working on another client
		while (tad->parent->listenerThreadStatus
			== ListenerThreadStatusCode::NEWCONNECTION) {
			// Release the lock and wait on a signal.
			tad->parent->WaitOnCondition(tad->parent->waitNewConnectionCond);

			// We may have been woken up and canceled.
			// Release the lock, then make sure.
			tad->parent->UnlockObject();
			pthread_testcancel();

			// Reacquire since we're not canceled.
			tad->parent->LockObject();
		}

		tad->parent->UnlockObject();

		// Now that we're done with our wait condition, test again
		pthread_testcancel();
	}

	// Call our thread parameters cleanup with a non-zero execute parameter
	pthread_cleanup_pop(1);
	return nullptr;
}

bool DigitalHaze::TCPServerSocket::CreateThreadedListener(unsigned short port) {
	// We're actually going to bind to ports here and not in the thread.
	// Plus binding is not a blocking operation.
	// While we do repeat code in the client,
	//  we don't want that to become a habit.
	if (!CreateListener(port))
		return false;

	// Set the parameters
	volatile threadedacceptdata* tad = new threadedacceptdata;
	tad->parent = this;

	// Init
	listenerThreadStatus = ListenerThreadStatusCode::STARTED;

	// Create thread
	if (0 != pthread_create(&listenerThread, nullptr,
		DigitalHaze::ListenerThread, (void*) tad)) {
		Socket::RecordErrno();
		listenerThreadStatus = ListenerThreadStatusCode::FAILED;
		delete tad;
		return false;
	}

	return true;
}

DigitalHaze::TCPSocket*
DigitalHaze::TCPServerSocket::GetNewConnectionFromThread(TCPAddressStorage* newAddr,
		socklen_t* newAddrLen) {
	// Check our thread status code
	if (listenerThreadStatus != ListenerThreadStatusCode::NEWCONNECTION)
		return nullptr;

	socklen_t copyAddrLen;

	// If we have a length given to us..
	if (newAddrLen) // Copy the smaller
	{
		// Use the smaller size for memcpy
		copyAddrLen = *newAddrLen < remoteAddrLen ? *newAddrLen : remoteAddrLen;
		*newAddrLen = remoteAddrLen; // Copy correct address size
	} else
		copyAddrLen = sizeof (TCPAddressStorage); // Otherwise assume it's big enough

	// If address info is requested, fill it in
	if (newAddr) memcpy(newAddr, &remoteAddr, copyAddrLen);

	// Create our new socket
	TCPSocket* newClient = new TCPSocket(newsockfd);

	// Signal our thread that it can start accepting new connections again
	listenerThreadStatus = ListenerThreadStatusCode::STARTED;
	SignalCondition(waitNewConnectionCond);

	if (newAddrLen) *newAddrLen = copyAddrLen;

	// And we have retrieved all info. The caller should unlock the mutex
	// and then the thread will reacquire it.
	return newClient;
}

DigitalHaze::TCPSocket*
DigitalHaze::TCPServerSocket::GetNewConnection(TCPAddressStorage* newAddr,
		socklen_t* newAddrLen) {
	// Check if we're a thread or blocking
	if (listenerThreadStatus == ListenerThreadStatusCode::UNKNOWN) {
		// Not in a threaded state, so block until new connection.
		if (!newAddr) newAddr = &remoteAddr; // Have to store new address somewhere
		if (!newAddrLen) {
			newAddrLen = &remoteAddrLen;
			remoteAddrLen = sizeof (remoteAddr); // Assume it's big enough
		}

		int newfd = accept(sockfd, (sockaddr*) newAddr, newAddrLen);

		if (newfd == -1) {
			// Error
			Socket::RecordErrno();
			return nullptr;
		}

		return new TCPSocket(newfd);
	}

	// In a threaded listen state
	return GetNewConnectionFromThread(newAddr, newAddrLen);
}

void DigitalHaze::TCPServerSocket::Thread_NotifyNewClient(int newfd,
		TCPAddressStorage* addr,
		socklen_t addrLen) {
	newsockfd = newfd;
	memcpy(&remoteAddr, addr, addrLen);
	remoteAddrLen = addrLen;
}

void DigitalHaze::TCPServerSocket::CloseCurrentThreads() {
	switch (listenerThreadStatus) {
		case ListenerThreadStatusCode::STARTED:
		case ListenerThreadStatusCode::NEWCONNECTION:
			pthread_cancel(listenerThread);
			break;
	}

	listenerThreadStatus = ListenerThreadStatusCode::UNKNOWN;
}