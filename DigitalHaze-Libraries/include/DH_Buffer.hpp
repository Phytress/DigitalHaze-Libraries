/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

/* 
 * File:   DH_Buffer.hpp
 * Author: phytress
 *
 * Created on July 7, 2017, 7:05 PM
 */

#ifndef DH_BUFFER_HPP
#define DH_BUFFER_HPP

#include <stdlib.h>

namespace DigitalHaze {

	class Buffer {
	public:
		// sizeInBytes: The length of the buffer in bytes.
		// reallocSize: If the buffer overflows, we reallocate a larger
		//  buffer by allocating this many more bytes. If reallocSize
		//  is zero, std::overflow_error is thrown on overflow.
		// Throws bad_array_new_length on zero sizeInBytes
		explicit Buffer(size_t sizeInBytes, size_t reallocSize = 0);
		~Buffer();

		// Read data from the position in the buffer.
		// On success, returns true and discards the read
		// data from the buffer.
		// Returns false if there is not enough data.
		bool Read(void* outBuffer, size_t len, size_t offset = 0);

		// Peek data from the beginning of the buffer.
		// On success, returns true and discards the read
		// data from the buffer.
		// Returns false if there is not enough data.
		bool Peek(void* outBuffer, size_t len, size_t offset = 0) const;

		// Write data to the end of the buffer.
		// If there is not enough space in the buffer,
		// more space is allocated in the buffer. If the reallocSize
		// was specified as zero, then overflow_error is thrown.
		// If insertOffset is -1, the write will occur at the
		// end the buffer as normal.
		void Write(void* inBuffer, size_t len, ssize_t insertOffset = -1);

		// Notify that we wrote to the buffer provided by GetBufferEnd
		void NotifyWrite(size_t len);

		// Notify that we want to expand the buffer by this many bytes.
		// If parameter is zero, we use the realloc size provided in initialization.
		// This will throw bad_alloc exceptions if there is an allocation failure.
		void ExpandBuffer(size_t additionalBytes = 0);

		// Notify that we want to expand the buffer by this many bytes.
		// The actual amount of bytes additionally allocated may be higher, and
		// the total size will align to the nearest multiple of our realloc size.
		// If realloc size is not set, then this function works the same
		// as NotifyExpand. If parameter is zero, then realloc size provided
		// in initialization is used. Will throw bad_alloc if allocation failure.
		void ExpandBufferAligned(size_t additionalBytes = 0);

		// Reads from the buffer and copies the memory into the variable
		// and removes that memory from the buffer.
		// Returns false if there is not enough data in the buffer.
		template<class vType>
		inline bool ReadVar(vType& var, size_t offset = 0);

		// Reads from the buffer and copies the memory into the variable.
		// Does not remove the data from buffer as if it had never been read.
		// Returns false if there is not enough data in the buffer.
		template<class vType>
		inline bool PeekVar(vType& var, size_t offset = 0) const;

		// Writes variable into the buffer.
		template<class vType>
		inline void WriteVar(vType var, ssize_t offset = -1);

		// Reads a string from the internal buffer and stores it into
		// the passed buffer up to specified number of bytes.
		// If a string terminating character (only \n and \0) is not
		// found within the read bytes, then 0 is returned.
		// Returns the length of the string. If there is not enough
		// space to store the string, maxLen+1 is returned.
		size_t ReadString(char* outString, size_t maxLen, size_t offset = 0);

		// Reads a string from the internal buffer and stores it into
		// the passed buffer up to specified number of bytes. Does not
		// remove it from the internal buffer, as if it had never been read.
		// If a string terminating character (only \n and \0) is not
		// found within the read bytes, then 0 is returned.
		// Returns the length of the string. If there is not enough
		// space to store the string, maxLen+1 is returned.
		size_t PeekString(char* outString, size_t maxLen, size_t offset = 0) const;

		// Writes a formatted string into the buffer.
		// Does not write a null terminator. That must be specified
		// as a \0 at the end of the string.
		size_t WriteString(const char* fmtStr, ...);
		
		// Writes a formatted string and inserts it at the specified
		// position in our buffer. Does not write null terminator,
		// that must be specified as a \0.
		size_t WriteStringAtOffset(size_t offset, const char* fmtStr, ...);

		inline size_t GetBufferSize() const {
			return bufferSize;
		}

		inline size_t GetBufferDataLen() const {
			return bufferLen;
		}

		inline size_t GetBufferReallocSize() const {
			return bufferReallocSize;
		}

		inline size_t GetRemainingBufferLength() const {
			return bufferSize - bufferLen;
		}

		inline void* GetBufferEnd() const {
			return (void*) ((size_t) buffer + bufferLen);
		}

		inline void* GetBufferStart() const {
			return buffer;
		}

		// Removes bytes from the front of the buffer

		inline void ShiftBufferFromFront(size_t bytesToShift) {
			ShiftBufferAtOffset(bytesToShift, 0);
		}

		// Removes bytes from the middle of a buffer
		void ShiftBufferAtOffset(size_t bytesToShift, size_t offset);

		// May remove any data in the buffer. Resets the buffer
		// with (or without) a resize.
		void Recreate(size_t newBufferSize, size_t newBufferReallocSize = 0);
	private:
		// Length of data in buffer
		size_t bufferLen;
		// Our maximum size for our buffer
		size_t bufferSize;
		// How many bytes to allocate on overflows (can be zero)
		size_t bufferReallocSize;
		// A malloc'd buffer.
		void* buffer;
	public:
		// Rule of 5

		Buffer(const Buffer& rhs); // copy constructor
		Buffer(Buffer&& rhs) noexcept; // move constructor
		Buffer& operator=(const Buffer& rhs); // assignment
		Buffer& operator=(Buffer&& rhs) noexcept; // move
	};

	template<class vType>
	inline bool Buffer::ReadVar(vType& var, size_t offset) {
		return Read(&var, sizeof (var), offset);
	}

	template<class vType>
	inline bool Buffer::PeekVar(vType& var, size_t offset) const {
		return Peek(&var, sizeof (var), offset);
	}

	template<class vType>
	inline void Buffer::WriteVar(vType var, ssize_t offset) {
		Write(&var, sizeof (var), offset);
	}
}

#endif /* DH_BUFFER_HPP */

