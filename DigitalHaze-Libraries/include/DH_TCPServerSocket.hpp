/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   DH_TCPServerSocket.hpp
 * Author: phytress
 *
 * Created on July 11, 2017, 4:01 PM
 */

#ifndef DH_TCPSERVERSOCKET_HPP
#define DH_TCPSERVERSOCKET_HPP

#include <pthread.h>
#include <signal.h>

#include "DH_Socket.hpp"
#include "DH_TCPSocket.hpp"
#include "DH_ThreadLockedObject.hpp"

namespace DigitalHaze {

	class TCPServerSocket : public Socket, public ThreadLockedObject {
	public:
		TCPServerSocket();
		~TCPServerSocket();

		// Creates our listener
		bool CreateListener(unsigned short port);
		// Creates our listener on a separate thread
		bool CreateThreadedListener(unsigned short port);
		// Retrieves a new client connection from the seperated thread.
		// Optionally, pass a sockaddr structure or socklen_t* to retrieve
		// the new connection's address info. Either or both can be null
		// to be disregarded. Returns null if there is no waiting connection.
		// Also returns null if there is no thread running.
		TCPSocket* GetNewConnectionFromThread(TCPAddressStorage* newAddr = nullptr,
											socklen_t* newAddrLen = nullptr);
		// Retrieves a new client connection.
		// Optionally, pass a sockaddr structure or socklen_t* to retrieve
		// the new connection's address info. Either or both can be null
		// to be disregarded. Returns null if there is no waiting connection.
		// If there is a thread running, the function will check the thread.
		// If there is no thread running, the function will block until
		// there is a new connection present. If an error occurs during blocking,
		// null is returned.
		TCPSocket* GetNewConnection(TCPAddressStorage* newAddr = nullptr,
									socklen_t* newAddrLen = nullptr);

		// Override of closing a TCP socket. Need to make sure
		// all threads are closed too.
		virtual void CloseSocket() override;

		// We could use a macro, but this works better with code parsing

		inline bool isListening() {
			return Socket::sockfd != -1;
		}

		// For TCPServerSocket, a socket that only listens for new connections:
		// We override our read and write functions to always return false
		// and not do anything. There is no buffer that can be used.
		/*virtual bool PerformSocketRead(size_t len = 0, bool flush = false) override;
		virtual bool PerformSocketWrite(bool flush = false) override;
		virtual bool Read(void* outBuffer, size_t len) override;
		virtual bool Peek(void* outBuffer, size_t len) override;
		virtual void Write(void* inBuffer, size_t len) override;*/

		// Our thread needs access to our internals
		friend void* ListenerThread(void*);
	private:
		// Thread status
		volatile sig_atomic_t listenerThreadStatus;
		// Thread
		pthread_t listenerThread;
		// Condition variable for new connections
		pthread_cond_t waitNewConnectionCond;

		// When our thread gets a new socket, we store it until another
		// thread retrieves it
		int newsockfd;
		TCPAddressStorage remoteAddr;
		socklen_t remoteAddrLen;

		// Notify object for when a new connection was retrieved
		void Thread_NotifyNewClient(int newfd, TCPAddressStorage* addr, socklen_t addrLen);

		// Closes the listening thread
		void CloseCurrentThreads();

		// Give our thread access to private parent functions

		inline void Thread_RecordErrno() {
			Socket::RecordErrno();
		}

		inline void Thread_RecordErrno(int setErrno) {
			Socket::RecordErrno(setErrno);
		}

		inline int Thread_GetSockFD() {
			return Socket::sockfd;
		}
	};

	void* ListenerThread(void*);
}

#endif /* DH_TCPSERVERSOCKET_HPP */

