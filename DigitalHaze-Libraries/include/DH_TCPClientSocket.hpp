/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
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

namespace DigitalHaze
{
	class TCPClientSocket : public TCPSocket, public ThreadLockedObject {
	public:
		TCPClientSocket();
		virtual ~TCPClientSocket();
		
		// A blocking connect attempt. Will return true if successful.
		// False if connection did not go through.
		bool AttemptConnect( const char* hostname, unsigned short port );
		
		// A non-blocking connect attempt.
		// The return is true if everything is going fine (not if you're
		// connected). If it is false, the thread or some other part
		// failed.
		bool AttemptThreadedConnect( const char* hostname, unsigned short port );
		
		// If a threaded attempt to connect was attempted
		// this function will let you know if the thread
		// is still in process of connecting.
		bool isConnecting() const;
		
		// Returns what address is currently being used, whether the
		// connection is still connecting or already connected.
		void GetConnectedAddress( sockaddr* addrOut ) const;
		void GetConnectedAddressLen( socklen_t& addrLen ) const;
		
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
		void SetConnectedAddress( sockaddr* cAddress, socklen_t cAddressLen );
		
		// Give our thread access to private parent functions
		inline void Thread_RecordErrno() {
			Socket::RecordErrno();
		}
		
		// When there's a lot of system calls going on, thread might
		// need to cache errors and set them in our object later.
		inline void Thread_RecordErrno(int setErrno) {
			Socket::RecordErrno(setErrno);
		}
		
		inline void Thread_SetSockFD( int newfd ) {
			Socket::sockfd = newfd;
		}
	};
	
	void* ConnectThread(void*);
}

#endif /* DH_TCPCLIENTSOCKET_HPP */

