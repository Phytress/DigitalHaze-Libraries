/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   DH_Socket.hpp
 * Author: phytress
 *
 * Created on July 6, 2017, 11:26 PM
 */

#ifndef DH_SOCKET_HPP
#define DH_SOCKET_HPP

#include <stdlib.h>

#include "DH_Buffer.hpp"

#ifndef DHSOCKETBUFSIZE
#define DHSOCKETBUFSIZE 4096
#endif
#ifndef DHSOCKETBUFRESIZE
#define DHSOCKETBUFRESIZE 4096
#endif

namespace DigitalHaze {

	class Socket {
		// We friend these classes so they can either:
		//  * Use our file descriptor directly
		//  * Access or set errors
		// No one else needs this access since others may mess with our sockfd.
		friend class TCPSocket;
		friend class TCPClientSocket;
		friend class TCPServerSocket;
		friend class SocketPool;
	public:
		Socket();
		virtual ~Socket();

		// Retrieve the last errno caused by a socket operation.

		inline int GetLastError() const {
			return lasterrno;
		}

		virtual void CloseSocket();

		// Compare sockfd. Useful if we moved the fd to another object.

		inline bool operator==(const Socket& rhs) const {
			return sockfd == rhs.sockfd;
		}
	private:
		int sockfd;
		int lasterrno;

		// Record errno in our local variable
		void RecordErrno();
		void RecordErrno(int specificerrno);
	public:
		// Rule of 5

		Socket(const Socket& rhs); // copy constructor
		Socket(Socket&& rhs) noexcept; // move constructor
		Socket& operator=(const Socket& rhs); // assignment
		Socket& operator=(Socket&& rhs) noexcept; // move
	};

	class IOSocket : public Socket {
		// Derived types work with our buffers.
		friend class TCPSocket;
	public:
		IOSocket();
		virtual ~IOSocket();

		// Perform a read into our internal read buffer.
		// If len is zero, any amount of data is read in.
		// If flush is true, then this function will block until
		// the specified amount of bytes is read.
		// If len is zero and flush is true, then the call will block
		// until the read buffer is full.
		// Returns false on error, true for success.
		virtual bool PerformSocketRead(size_t len = 0, bool flush = false) = 0;

		// Perform a write from our outgoing buffer.
		// If flush is set to true, then the function will
		// block until all data in our outgoing buffer is sent.
		// Returns false on error, true on success.
		virtual bool PerformSocketWrite(bool flush = false) = 0;

		// Read data into caller provided buffer.
		// Data is discarded from internal buffer.
		// If there is not enough data in our buffer,
		// this becomes a blocking operation until there
		// is enough data or an error occurs.
		// Returns false on error, true on success.
		// If len is zero, then the size of the entire internal
		virtual bool Read(void* outBuffer, size_t len);

		// Read data into caller provided buffer.
		// Data is not discarded from internal buffer
		// as if it were never read.
		// Returns false on error, true on success.
		virtual bool Peek(void* outBuffer, size_t len);

		// Write data from caller provided buffer
		// into internal outgoing buffer.
		virtual void Write(void* inBuffer, size_t len);

		// Read a variable and remove it from internal buffer.
		// Blocking operation if not enough data in buffer.
		template<class vType>
		inline bool ReadVar(vType& var);

		// Read a variable but do not discard it from internal buffer
		// as if it were never read. Blocking operation if not enough
		// data in buffer.
		template<class vType>
		inline bool PeekVar(vType& var);

		// Write variable into internal outgoing buffer.
		template<class vType>
		inline void WriteVar(vType var);

		// Reads a string from the read buffer and stores it into
		// the passed buffer up to specified number of bytes.
		// If a string terminating character (only \n and \0) is not
		// found within the read bytes, then 0 is returned.
		// Returns the length of the string. If there is not enough
		// space to store the string, maxLen+1 is returned.
		inline size_t ReadString(char* outString, size_t maxLen);

		// Reads a string from the internal buffer and stores it into
		// the passed buffer up to specified number of bytes. Does not
		// remove it from the internal buffer, as if it had never been read.
		// If a string terminating character (only \n and \0) is not
		// found within the read bytes, then 0 is returned.
		// Returns the length of the string. If there is not enough
		// space to store the string, maxLen+1 is returned.
		// null terminators and line breaks do not count towards the length.
		inline size_t PeekString(char* outString, size_t maxLen) const;

		// Writes a formatted string into the buffer.
		// Does not write a null terminator. That must be specified
		// as a \0 at the end of the string. null terminators and line
		// breaks count towards the length returned.
		size_t WriteString(const char* fmtStr, ...);

		// Returns how many bytes we have pending in our write buffer.

		inline size_t GetEgressDataLen() const {
			return writeBuffer.GetBufferDataLen();
		}

		// Returns how many bytes we have pending in our read buffer.

		inline size_t GetIngressDataLen() const {
			return readBuffer.GetBufferDataLen();
		}

		// When we close our connection, we can reset our buffers too.
		virtual void CloseSocket() override;
	private:
		// We use our own buffers and we do not increase the size
		// of the send and recv buffer because of several reasons.
		// The system already optimizes stuff like this for performance,
		// so we don't want to use defaults. If the system wants us to not
		// write data, we should be okay with that and just buffer ourselves.
		Buffer readBuffer;
		Buffer writeBuffer;
	public:
		// Rule of 5

		IOSocket(const IOSocket& rhs); // copy constructor
		IOSocket(IOSocket&& rhs) noexcept; // move constructor
		IOSocket& operator=(const IOSocket& rhs); // assignment
		IOSocket& operator=(IOSocket&& rhs) noexcept; // move
	};

	template<class vType>
	inline bool IOSocket::ReadVar(vType& var) {
		return Read(&var, sizeof (vType));
	}

	template<class vType>
	inline bool IOSocket::PeekVar(vType& var) {
		return Peek(&var, sizeof (vType));
	}

	template<class vType>
	inline void IOSocket::WriteVar(vType var) {
		Write(&var, sizeof (vType));
	}

	inline size_t IOSocket::ReadString(char* outString, size_t maxLen) {
		return readBuffer.ReadString(outString, maxLen);
	}

	inline size_t IOSocket::PeekString(char* outString, size_t maxLen) const {
		return readBuffer.PeekString(outString, maxLen);
	}
}

#endif /* SOCKET_HPP */