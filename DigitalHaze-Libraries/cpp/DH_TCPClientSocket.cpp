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

#include "DH_TCPClientSocket.hpp"

#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <errno.h>

#include <cstring>
#include <stdio.h>
#include <pthread.h>

enum ConnectThreadStatusCode {
	UNKNOWN = 0,
	STARTED,
	CONNECTED,
	FAILED
};

DigitalHaze::TCPClientSocket::TCPClientSocket()
	: TCPSocket(), ThreadLockedObject(),
	connectThreadStatus(ConnectThreadStatusCode::UNKNOWN) {
}

DigitalHaze::TCPClientSocket::~TCPClientSocket() {
	CloseCurrentThreads();
	// non-virtual Socket::CloseSocket is called on Socket destructor
}

bool DigitalHaze::TCPClientSocket::AttemptConnect(const char* hostname,
		unsigned short port) {
	// Close any threads going on right now
	// CloseCurrentThreads(); // CloseSocket takes care of this
	// Close any connection we already have
	this->CloseSocket();

	// Non-threaded call, so change the thread status code
	// connectThreadStatus = ConnectThreadStatusCode::UNKNOWN;
	// This is also done by CloseSocket / CloseCurrentThreads

	// Resolve IP address
	addrinfo hints, *servinfo;
	memset(&hints, 0, sizeof (hints));
	hints.ai_family = AF_UNSPEC; // IPv4 or v6
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_NUMERICSERV;

	char portNumber[8];
	snprintf(portNumber, sizeof (portNumber), "%hu", port);

	if (0 != getaddrinfo(hostname, portNumber, &hints, &servinfo)) {
		// Host does not exist
		Socket::RecordErrno();
		return false;
	}

	// Set up our connection
	int newsockfd = -1;

	for (addrinfo* ipResults = servinfo; ipResults; ipResults = ipResults->ai_next) {
		newsockfd = socket(ipResults->ai_family,
				ipResults->ai_socktype,
				ipResults->ai_protocol);

		if (newsockfd == -1) {
			// An annoying time to be getting this error
			Socket::RecordErrno();
			continue; // try again?
		}

		SetConnectedAddress(ipResults->ai_addr, ipResults->ai_addrlen);

		// Returns -1 on error, but we check for success
		if (0 == connect(newsockfd, ipResults->ai_addr, ipResults->ai_addrlen)) {
			// Successfully connected!
			break;
		}

		// We were not successful in connecting.
		Socket::RecordErrno();
		close(newsockfd);
		newsockfd = -1;
	}

	freeaddrinfo(servinfo);

	// Did we get a successful connect?
	if (newsockfd != -1) {
		// Store the socket
		IOSocket::sockfd = newsockfd;
		return true;
	}

	SetConnectedAddress(nullptr, 0);
	return false;
}

struct threadedconnectdata {
	char* hostname;
	unsigned short port;
	DigitalHaze::TCPClientSocket* parent;
};

// Cleanup function to free memory passed to a connect thread.

void CleanupConnectThread(void* data) {
	if (data) {
		threadedconnectdata* tcd = (threadedconnectdata*) data;
		delete[] tcd->hostname;
		delete tcd;
	}
}

// Cleanup to free our addrinfo returned from looking up the hostname

void CleanupAddrInfo(void* data) {
	if (data) {
		addrinfo* servinfo = (addrinfo*) data;
		freeaddrinfo(servinfo);
	}
}

// Cleanup in case we got a valid connection, but the thread was canceled.

void CleanupConnectedSocket(void* data) {
	if (data) {
		int fd = *(int*) data; // We don't mind losing precision
		if (fd != -1) {
			close(fd);
		}
	}
}

void* DigitalHaze::ConnectThread(void* data) {
	if (!data) {
		// Someone tried to start this thread improperly.
		pthread_exit(nullptr);
		return nullptr;
	}

	// We're doing some complex networking, opening file descriptors, and
	// data has been allocated to us. This thread needs to ensure it will
	// be executed and canceled properly.
	// Since these are the default values, the lines are commented, but
	// in the future it may be useful.
	// pthread_setcanceltype(PTHREAD_CANCEL_DEFERRED,nullptr);
	// pthread_setcancelstate(PTHREAD_CANCEL_ENABLE,nullptr);

	// There is allocated data passed to us.
	// If the thread gets canceled, exits prematurely, or exits properly,
	// we need to clean it up with the cleanup function. Since it was allocated
	// out of the function, we're going to clean it up out of the function.
	pthread_cleanup_push(CleanupConnectThread, data);

	// Grab our parameters
	threadedconnectdata* tcd = (threadedconnectdata*) data;

	// Resolve IP address
	addrinfo hints, *servinfo;
	memset(&hints, 0, sizeof (hints));
	hints.ai_family = AF_UNSPEC; // IPv4 or v6
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = IPPROTO_TCP;
	hints.ai_flags = AI_NUMERICSERV;

	char portNumber[8];
	snprintf(portNumber, sizeof (portNumber), "%hu", tcd->port);

	// Before we begin a lookup, give a chance to be immediately canceled
	pthread_testcancel();

	if (0 != getaddrinfo(tcd->hostname, portNumber, &hints, &servinfo)) {
		// Host does not exist
		tcd->parent->LockObject();
		tcd->parent->connectThreadStatus = ConnectThreadStatusCode::FAILED;
		tcd->parent->Thread_RecordErrno();
		tcd->parent->UnlockObject();
		pthread_exit(nullptr); // Call our cleanup handler
		return nullptr; // Just in case for undefined behavior
	}

	// Will use this to get a connected socket
	int newsockfd = -1;
	int currentErrno; // cache variable for errno

	// We have newly allocated info in servinfo, so we get a second
	// cleanup handler.
	pthread_cleanup_push(CleanupAddrInfo, servinfo);

	// Now that we're done with the lookup, give another chance
	pthread_testcancel();

	for (addrinfo* ipResults = servinfo; ipResults; ipResults = ipResults->ai_next) {
		newsockfd = socket(ipResults->ai_family,
				ipResults->ai_socktype,
				ipResults->ai_protocol);

		if (newsockfd == -1) {
			// An annoying time to be getting this error.
			// Could be something with the OS system call. Good time
			// to check if the thread was canceled.
			currentErrno = errno; // cache our errno
			pthread_testcancel();

			// Not canceled, record the error and keep going.
			tcd->parent->LockObject();
			tcd->parent->Thread_RecordErrno(currentErrno);
			tcd->parent->UnlockObject();
			continue; // try again?
		}

		// Tell our parent to whom we are trying to connect
		tcd->parent->LockObject();
		tcd->parent->SetConnectedAddress(ipResults->ai_addr, ipResults->ai_addrlen);
		tcd->parent->UnlockObject();

		if (-1 != connect(newsockfd, ipResults->ai_addr, ipResults->ai_addrlen)) {
			// Successfully connected!
			break;
		}

		// We cache errno here so we can close the FD and testcancel first
		currentErrno = errno;

		// First, immediately close our FD because it is bad
		close(newsockfd);
		newsockfd = -1;

		// Second, test for cancellation now that we're in a safe spot
		pthread_testcancel();

		// We are not successful in connecting, so record it!
		tcd->parent->LockObject();
		tcd->parent->Thread_RecordErrno(currentErrno);
		tcd->parent->UnlockObject();
	}

	// Free lookup info via freeaddrinfo(servinfo)
	// in our cleanup handler
	pthread_cleanup_pop(1);

	// We might have a valid socket FD right now, but the thread might also
	// be canceled, so we create a cleanup handler that will close our socket.
	pthread_cleanup_push(CleanupConnectedSocket, &newsockfd);
	pthread_testcancel(); // Do we need to close our socket?
	pthread_cleanup_pop(0); // Do not execute cleanup if we don't have to.

	// Set if we connected or not
	tcd->parent->LockObject();
	if (newsockfd == -1) {
		// Set the status to failed
		tcd->parent->SetConnectedAddress(nullptr, 0);
		tcd->parent->connectThreadStatus = ConnectThreadStatusCode::FAILED;
	} else {
		tcd->parent->Thread_SetSockFD(newsockfd);
		tcd->parent->connectThreadStatus = ConnectThreadStatusCode::CONNECTED;
	}
	tcd->parent->UnlockObject();

	// Call our thread parameters cleanup with a non-zero execute parameter
	pthread_cleanup_pop(1);
	return nullptr;
}

bool DigitalHaze::TCPClientSocket::AttemptThreadedConnect(const char* hostname,
		unsigned short port) {
	// Close any other threads
	// CloseCurrentThreads(); // CloseSocket takes care of this
	// Close any current connection
	this->CloseSocket();

	// Allocate thread parameters
	size_t hostnameLen = strlen(hostname);
	threadedconnectdata* tcd = new threadedconnectdata;

	tcd->port = port;
	tcd->parent = this;
	tcd->hostname = new char[hostnameLen + 1];
	snprintf(tcd->hostname, hostnameLen + 1, "%s", hostname);

	// Init
	connectThreadStatus = ConnectThreadStatusCode::STARTED;
	
	// The connect thread will be detached because it will
	// leave data in a state in which we can tell if it has
	// already exited or not.
	pthread_attr_t threadAttr;
	pthread_attr_init( &threadAttr );
	pthread_attr_setdetachstate( &threadAttr, PTHREAD_CREATE_DETACHED );

	// create thread
	if (0 != pthread_create(&connectThread, &threadAttr,
		DigitalHaze::ConnectThread, (void*) tcd)) {
		// failed to create thread
		Socket::RecordErrno();
		connectThreadStatus = ConnectThreadStatusCode::FAILED;
		delete[] tcd->hostname;
		delete tcd;
		return false;
	}

	return true;
}

bool DigitalHaze::TCPClientSocket::isConnecting() const {
	switch (connectThreadStatus) {
		case ConnectThreadStatusCode::UNKNOWN:
		case ConnectThreadStatusCode::CONNECTED:
		case ConnectThreadStatusCode::FAILED:
			return false;
		case ConnectThreadStatusCode::STARTED:
			return true;
	};

	return false;
}

void DigitalHaze::TCPClientSocket::GetConnectedAddress(sockaddr* addrOut) const {
	if (addrOut)
		memcpy(addrOut, &connectedAddress, connectedAddressLen);
}

void DigitalHaze::TCPClientSocket::GetConnectedAddressLen(socklen_t& addrLen) const {
	addrLen = connectedAddressLen;
}

void DigitalHaze::TCPClientSocket::CloseSocket() {
	CloseCurrentThreads();
	Socket::CloseSocket();
	SetConnectedAddress(nullptr, 0);
}

void DigitalHaze::TCPClientSocket::CloseCurrentThreads() {
	switch (connectThreadStatus) {
		case ConnectThreadStatusCode::STARTED:
			pthread_cancel(connectThread);
			break;
	};

	connectThreadStatus = ConnectThreadStatusCode::UNKNOWN;
}

void DigitalHaze::TCPClientSocket::SetConnectedAddress(sockaddr* cAddress,
		socklen_t cAddressLen) {
	if (cAddress)
		memcpy(&connectedAddress, cAddress, cAddressLen);
	else {
		memset(&connectedAddress, 0, sizeof (connectedAddress));
		cAddressLen = 0;
	}

	connectedAddressLen = cAddressLen;
}