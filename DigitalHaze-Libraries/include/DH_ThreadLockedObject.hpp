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
 * File:   DH_ThreadedObject.hpp
 * Author: phytress
 *
 * Created on July 9, 2017, 1:22 PM
 */

#ifndef DH_THREADLOCKEDOBJECT_HPP
#define DH_THREADLOCKEDOBJECT_HPP

#include <pthread.h>

namespace DigitalHaze {
	class ThreadLockedObject {
	public:
		ThreadLockedObject() noexcept;
		~ThreadLockedObject();
		
		void LockObject();
		void WaitOnCondition(pthread_cond_t& cond);
		inline static void SignalCondition(pthread_cond_t& cond) {
			pthread_cond_signal(&cond);
		}
		inline static void BroadcastCondition(pthread_cond_t& cond) {
			pthread_cond_broadcast(&cond);
		}
		void UnlockObject();
	private:
		pthread_mutex_t cs_mutex;
	};
}

#endif /* DH_THREADEDOBJECT_HPP */

