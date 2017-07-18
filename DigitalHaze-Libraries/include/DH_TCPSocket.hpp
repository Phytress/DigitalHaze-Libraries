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
 * File:   DH_TCPSocket.hpp
 * Author: phytress
 *
 * Created on July 7, 2017, 5:42 PM
 */

#ifndef DH_TCPSOCKET_HPP
#define DH_TCPSOCKET_HPP

#include <stdlib.h>
#include <netdb.h>

#include "DH_Socket.hpp"

namespace DigitalHaze {

	union TCPAddressStorage {
		sockaddr sa;
		sockaddr_in sa_ip4;
		sockaddr_in6 sa_ip6;
		sockaddr_storage sa_storage;
	};

	class TCPSocket : public IOSocket {
	public:
		TCPSocket();
		explicit TCPSocket(int connectedfd);
		virtual ~TCPSocket();

		// Perform a read into our internal read buffer.
		// If len is zero, any amount of data is read in.
		// If flush is true, then this function will block until
		// the specified amount of bytes is read.
		// If len is zero and flush is true, then the call will block
		// until the read buffer is full.
		virtual bool PerformSocketRead(size_t len = 0, bool flush = false) override;

		// Perform a write from our outgoing buffer.
		// If flush is set to true, then the function will
		// block until all data in our outgoing buffer is sent.
		// Returns false on error, true on success.
		virtual bool PerformSocketWrite(bool flush = false) override;

		// Let's you know if you're connected or not.
		bool isConnected() const;

		// Converts a TCPAddressStorage structure to a text address.
		// Returns false if the address cannot be parsed.
		static bool ConvertAddrToText(TCPAddressStorage& addr,
									socklen_t len,
									char* outText);
	private:
	};
}

#endif /* DH_TCPSOCKET_HPP */

