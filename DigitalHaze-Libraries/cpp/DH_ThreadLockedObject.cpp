/*
 * To change this license header, choose License Headers in Project Properties.
 * To change this template file, choose Tools | Templates
 * and open the template in the editor.
 */

#include "DH_ThreadLockedObject.hpp"

#include <pthread.h>
#include <time.h>

DigitalHaze::ThreadLockedObject::ThreadLockedObject() noexcept
: cs_mutex(PTHREAD_RECURSIVE_MUTEX_INITIALIZER_NP) {
}

DigitalHaze::ThreadLockedObject::~ThreadLockedObject() {
	pthread_mutex_destroy(&cs_mutex);
}

void DigitalHaze::ThreadLockedObject::LockObject() {
	pthread_mutex_lock(&cs_mutex);
}

void DigitalHaze::ThreadLockedObject::WaitOnCondition(pthread_cond_t& cond) {
	pthread_cond_wait(&cond, &cs_mutex);
}

void DigitalHaze::ThreadLockedObject::UnlockObject() {
	pthread_mutex_unlock(&cs_mutex);
}