/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
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

