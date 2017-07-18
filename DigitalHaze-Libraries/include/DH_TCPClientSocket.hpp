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
 * File:   DH_TCPClientSocket.hpp
 * Author: phytress
 *
 * Created on July 9, 2017, 12:59 AM
 */

#ifndef DH_TCPCLIENTSOCKET_HPP
#define DH_TCPCLIENTSOCKET_HPP

#include "DH_TCPSocket.hpp"
#include "DH_ThreadLockedObject.hpp"

#include <signal.h>
#include <pthread.h>

#include <sys/socket.h>
#include <sys/types.h>

#include <netdb.h>

namespace DigitalHaze {

	class TCPClientSocket : public TCPSocket, public ThreadLockedObject {
	public:
		TCPClientSocket();
		virtual ~TCPClientSocket();

		// A blocking connect attempt. Will return true if successful.
		// False if connection did not go through.
		bool AttemptConnect(const char* hostname, unsigned short port);

		// A non-blocking connect attempt.
		// The return is true if everything is going fine (not if you're
		// connected). If it is false, the thread or some other part
		// failed.
		bool AttemptThreadedConnect(const char* hostname, unsigned short port);

		// If a threaded attempt to connect was attempted
		// this function will let you know if the thread
		// is still in process of connecting.
		bool isConnecting() const;

		// Returns what address is currently being used, whether the
		// connection is still connecting or already connected.
		void GetConnectedAddress(sockaddr* addrOut) const;
		void GetConnectedAddressLen(socklen_t& addrLen) const;

		// Override of closing a TCP socket. Zero more data from this class.
		virtual void CloseSocket() override;

		// Our thread needs access to our internals
		friend void* ConnectThread(void*);
	private:
		// Thread status
		volatile sig_atomic_t connectThreadStatus;
		// Thread pointer
		pthread_t connectThread;

		// Resolved connected address
		TCPAddressStorage connectedAddress;
		socklen_t connectedAddressLen;

		void CloseCurrentThreads();
		void SetConnectedAddress(sockaddr* cAddress, socklen_t cAddressLen);

		// Give our thread access to private parent functions

		inline void Thread_RecordErrno() {
			Socket::RecordErrno();
		}

		// When there's a lot of system calls going on, thread might
		// need to cache errors and set them in our object later.

		inline void Thread_RecordErrno(int setErrno) {
			Socket::RecordErrno(setErrno);
		}

		inline void Thread_SetSockFD(int newfd) {
			Socket::sockfd = newfd;
		}
	};

	void* ConnectThread(void*);
}

#endif /* DH_TCPCLIENTSOCKET_HPP */

