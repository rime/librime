/*************************************************************************************************
 * Threading devices
 *                                                               Copyright (C) 2009-2012 FAL Labs
 * This file is part of Kyoto Cabinet.
 * This program is free software: you can redistribute it and/or modify it under the terms of
 * the GNU General Public License as published by the Free Software Foundation, either version
 * 3 of the License, or any later version.
 * This program is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 * without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 * You should have received a copy of the GNU General Public License along with this program.
 * If not, see <http://www.gnu.org/licenses/>.
 *************************************************************************************************/


#include "kcthread.h"
#include "myconf.h"

namespace kyotocabinet {                 // common namespace


/**
 * Constants for implementation.
 */
namespace {
const uint32_t LOCKBUSYLOOP = 8192;      ///< threshold of busy loop and sleep for locking
const size_t LOCKSEMNUM = 256;           ///< number of semaphores for locking
}


/**
 * Thread internal.
 */
struct ThreadCore {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  ::HANDLE th;                           ///< handle
#else
  ::pthread_t th;                        ///< identifier
  bool alive;                            ///< alive flag
#endif
};


/**
 * CondVar internal.
 */
struct CondVarCore {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  ::CRITICAL_SECTION mutex;              ///< mutex
  uint32_t wait;                         ///< wait count
  uint32_t wake;                         ///< wake count
  ::HANDLE sev;                          ///< signal event handle
  ::HANDLE fev;                          ///< finish event handle
#else
  ::pthread_cond_t cond;                 ///< condition
#endif
};


/**
 * Call the running thread.
 * @param arg the thread.
 * @return always NULL.
 */
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
static ::DWORD threadrun(::LPVOID arg);
#else
static void* threadrun(void* arg);
#endif


/**
 * Default constructor.
 */
Thread::Thread() : opq_(NULL) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  ThreadCore* core = new ThreadCore;
  core->th = NULL;
  opq_ = (void*)core;
#else
  _assert_(true);
  ThreadCore* core = new ThreadCore;
  core->alive = false;
  opq_ = (void*)core;
#endif
}


/**
 * Destructor.
 */
Thread::~Thread() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  ThreadCore* core = (ThreadCore*)opq_;
  if (core->th) join();
  delete core;
#else
  _assert_(true);
  ThreadCore* core = (ThreadCore*)opq_;
  if (core->alive) join();
  delete core;
#endif
}


/**
 * Start the thread.
 */
void Thread::start() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  ThreadCore* core = (ThreadCore*)opq_;
  if (core->th) throw std::invalid_argument("already started");
  ::DWORD id;
  core->th = ::CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)threadrun, this, 0, &id);
  if (!core->th) throw std::runtime_error("CreateThread");
#else
  _assert_(true);
  ThreadCore* core = (ThreadCore*)opq_;
  if (core->alive) throw std::invalid_argument("already started");
  if (::pthread_create(&core->th, NULL, threadrun, this) != 0)
    throw std::runtime_error("pthread_create");
  core->alive = true;
#endif
}


/**
 * Wait for the thread to finish.
 */
void Thread::join() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  ThreadCore* core = (ThreadCore*)opq_;
  if (!core->th) throw std::invalid_argument("not alive");
  if (::WaitForSingleObject(core->th, INFINITE) == WAIT_FAILED)
    throw std::runtime_error("WaitForSingleObject");
  ::CloseHandle(core->th);
  core->th = NULL;
#else
  _assert_(true);
  ThreadCore* core = (ThreadCore*)opq_;
  if (!core->alive) throw std::invalid_argument("not alive");
  core->alive = false;
  if (::pthread_join(core->th, NULL) != 0) throw std::runtime_error("pthread_join");
#endif
}


/**
 * Put the thread in the detached state.
 */
void Thread::detach() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
#else
  _assert_(true);
  ThreadCore* core = (ThreadCore*)opq_;
  if (!core->alive) throw std::invalid_argument("not alive");
  core->alive = false;
  if (::pthread_detach(core->th) != 0) throw std::runtime_error("pthread_detach");
#endif
}


/**
 * Terminate the running thread.
 */
void Thread::exit() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  ::ExitThread(0);
#else
  _assert_(true);
  ::pthread_exit(NULL);
#endif
}


/**
 * Yield the processor from the current thread.
 */
void Thread::yield() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  ::Sleep(0);
#else
  _assert_(true);
  ::sched_yield();
#endif
}


/**
 * Chill the processor by suspending execution for a quick moment.
 */
void Thread::chill() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  ::Sleep(21);
#else
  _assert_(true);
  struct ::timespec req;
  req.tv_sec = 0;
  req.tv_nsec = 21 * 1000 * 1000;
  ::nanosleep(&req, NULL);
#endif
}



/**
 * Suspend execution of the current thread.
 */
bool Thread::sleep(double sec) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(sec >= 0.0);
  if (sec <= 0.0) {
    yield();
    return true;
  }
  if (sec > INT32MAX) sec = INT32MAX;
  ::Sleep(sec * 1000);
  return true;
#else
  _assert_(sec >= 0.0);
  if (sec <= 0.0) {
    yield();
    return true;
  }
  if (sec > INT32MAX) sec = INT32MAX;
  double integ, fract;
  fract = std::modf(sec, &integ);
  struct ::timespec req, rem;
  req.tv_sec = (time_t)integ;
  req.tv_nsec = (long)(fract * 999999000);
  while (::nanosleep(&req, &rem) != 0) {
    if (errno != EINTR) return false;
    req = rem;
  }
  return true;
#endif
}


/**
 * Get the hash value of the current thread.
 */
int64_t Thread::hash() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  return ::GetCurrentThreadId();
#else
  _assert_(true);
  pthread_t tid = pthread_self();
  int64_t num;
  if (sizeof(tid) == sizeof(num)) {
    std::memcpy(&num, &tid, sizeof(num));
  } else if (sizeof(tid) == sizeof(int32_t)) {
    uint32_t inum;
    std::memcpy(&inum, &tid, sizeof(inum));
    num = inum;
  } else {
    num = hashmurmur(&tid, sizeof(tid));
  }
  return num & INT64MAX;
#endif
}


/**
 * Call the running thread.
 */
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
static ::DWORD threadrun(::LPVOID arg) {
  _assert_(true);
  Thread* thread = (Thread*)arg;
  thread->run();
  return NULL;
}
#else
static void* threadrun(void* arg) {
  _assert_(true);
  Thread* thread = (Thread*)arg;
  thread->run();
  return NULL;
}
#endif


/**
 * Default constructor.
 */
Mutex::Mutex() : opq_(NULL) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  ::CRITICAL_SECTION* mutex = new ::CRITICAL_SECTION;
  ::InitializeCriticalSection(mutex);
  opq_ = (void*)mutex;
#else
  _assert_(true);
  ::pthread_mutex_t* mutex = new ::pthread_mutex_t;
  if (::pthread_mutex_init(mutex, NULL) != 0) throw std::runtime_error("pthread_mutex_init");
  opq_ = (void*)mutex;
#endif
}


/**
 * Constructor with the specifications.
 */
Mutex::Mutex(Type type) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  ::CRITICAL_SECTION* mutex = new ::CRITICAL_SECTION;
  ::InitializeCriticalSection(mutex);
  opq_ = (void*)mutex;
#else
  _assert_(true);
  ::pthread_mutexattr_t attr;
  if (::pthread_mutexattr_init(&attr) != 0) throw std::runtime_error("pthread_mutexattr_init");
  switch (type) {
    case FAST: {
      break;
    }
    case ERRORCHECK: {
      if (::pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_ERRORCHECK) != 0)
        throw std::runtime_error("pthread_mutexattr_settype");
      break;
    }
    case RECURSIVE: {
      if (::pthread_mutexattr_settype(&attr, PTHREAD_MUTEX_RECURSIVE) != 0)
        throw std::runtime_error("pthread_mutexattr_settype");
      break;
    }
  }
  ::pthread_mutex_t* mutex = new ::pthread_mutex_t;
  if (::pthread_mutex_init(mutex, &attr) != 0) throw std::runtime_error("pthread_mutex_init");
  ::pthread_mutexattr_destroy(&attr);
  opq_ = (void*)mutex;
#endif
}


/**
 * Destructor.
 */
Mutex::~Mutex() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  ::CRITICAL_SECTION* mutex = (::CRITICAL_SECTION*)opq_;
  ::DeleteCriticalSection(mutex);
  delete mutex;
#else
  _assert_(true);
  ::pthread_mutex_t* mutex = (::pthread_mutex_t*)opq_;
  ::pthread_mutex_destroy(mutex);
  delete mutex;
#endif
}


/**
 * Get the lock.
 */
void Mutex::lock() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  ::CRITICAL_SECTION* mutex = (::CRITICAL_SECTION*)opq_;
  ::EnterCriticalSection(mutex);
#else
  _assert_(true);
  ::pthread_mutex_t* mutex = (::pthread_mutex_t*)opq_;
  if (::pthread_mutex_lock(mutex) != 0) throw std::runtime_error("pthread_mutex_lock");
#endif
}


/**
 * Try to get the lock.
 */
bool Mutex::lock_try() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  ::CRITICAL_SECTION* mutex = (::CRITICAL_SECTION*)opq_;
  if (!::TryEnterCriticalSection(mutex)) return false;
  return true;
#else
  _assert_(true);
  ::pthread_mutex_t* mutex = (::pthread_mutex_t*)opq_;
  int32_t ecode = ::pthread_mutex_trylock(mutex);
  if (ecode == 0) return true;
  if (ecode != EBUSY) throw std::runtime_error("pthread_mutex_trylock");
  return false;
#endif
}


/**
 * Try to get the lock.
 */
bool Mutex::lock_try(double sec) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_) || defined(_SYS_CYGWIN_) || defined(_SYS_MACOSX_)
  _assert_(sec >= 0.0);
  if (lock_try()) return true;
  double end = time() + sec;
  Thread::yield();
  uint32_t wcnt = 0;
  while (!lock_try()) {
    if (time() > end) return false;
    if (wcnt >= LOCKBUSYLOOP) {
      Thread::chill();
    } else {
      Thread::yield();
      wcnt++;
    }
  }
  return true;
#else
  _assert_(sec >= 0.0);
  ::pthread_mutex_t* mutex = (::pthread_mutex_t*)opq_;
  struct ::timeval tv;
  struct ::timespec ts;
  if (::gettimeofday(&tv, NULL) == 0) {
    double integ;
    double fract = std::modf(sec, &integ);
    ts.tv_sec = tv.tv_sec + (time_t)integ;
    ts.tv_nsec = (long)(tv.tv_usec * 1000.0 + fract * 999999000);
    if (ts.tv_nsec >= 1000000000) {
      ts.tv_nsec -= 1000000000;
      ts.tv_sec++;
    }
  } else {
    ts.tv_sec = std::time(NULL) + 1;
    ts.tv_nsec = 0;
  }
  int32_t ecode = ::pthread_mutex_timedlock(mutex, &ts);
  if (ecode == 0) return true;
  if (ecode != ETIMEDOUT) throw std::runtime_error("pthread_mutex_timedlock");
  return false;
#endif
}


/**
 * Release the lock.
 */
void Mutex::unlock() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  ::CRITICAL_SECTION* mutex = (::CRITICAL_SECTION*)opq_;
  ::LeaveCriticalSection(mutex);
#else
  _assert_(true);
  ::pthread_mutex_t* mutex = (::pthread_mutex_t*)opq_;
  if (::pthread_mutex_unlock(mutex) != 0) throw std::runtime_error("pthread_mutex_unlock");
#endif
}


/**
 * SlottedMutex internal.
 */
struct SlottedMutexCore {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  ::CRITICAL_SECTION* mutexes;           ///< primitives
  size_t slotnum;                        ///< number of slots
#else
  ::pthread_mutex_t* mutexes;            ///< primitives
  size_t slotnum;                        ///< number of slots
#endif
};


/**
 * Constructor.
 */
SlottedMutex::SlottedMutex(size_t slotnum) : opq_(NULL) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  SlottedMutexCore* core = new SlottedMutexCore;
  ::CRITICAL_SECTION* mutexes = new ::CRITICAL_SECTION[slotnum];
  for (size_t i = 0; i < slotnum; i++) {
    ::InitializeCriticalSection(mutexes + i);
  }
  core->mutexes = mutexes;
  core->slotnum = slotnum;
  opq_ = (void*)core;
#else
  _assert_(true);
  SlottedMutexCore* core = new SlottedMutexCore;
  ::pthread_mutex_t* mutexes = new ::pthread_mutex_t[slotnum];
  for (size_t i = 0; i < slotnum; i++) {
    if (::pthread_mutex_init(mutexes + i, NULL) != 0)
      throw std::runtime_error("pthread_mutex_init");
  }
  core->mutexes = mutexes;
  core->slotnum = slotnum;
  opq_ = (void*)core;
#endif
}


/**
 * Destructor.
 */
SlottedMutex::~SlottedMutex() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  SlottedMutexCore* core = (SlottedMutexCore*)opq_;
  ::CRITICAL_SECTION* mutexes = core->mutexes;
  size_t slotnum = core->slotnum;
  for (size_t i = 0; i < slotnum; i++) {
    ::DeleteCriticalSection(mutexes + i);
  }
  delete[] mutexes;
  delete core;
#else
  _assert_(true);
  SlottedMutexCore* core = (SlottedMutexCore*)opq_;
  ::pthread_mutex_t* mutexes = core->mutexes;
  size_t slotnum = core->slotnum;
  for (size_t i = 0; i < slotnum; i++) {
    ::pthread_mutex_destroy(mutexes + i);
  }
  delete[] mutexes;
  delete core;
#endif
}


/**
 * Get the lock of a slot.
 */
void SlottedMutex::lock(size_t idx) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  SlottedMutexCore* core = (SlottedMutexCore*)opq_;
  ::EnterCriticalSection(core->mutexes + idx);
#else
  _assert_(true);
  SlottedMutexCore* core = (SlottedMutexCore*)opq_;
  if (::pthread_mutex_lock(core->mutexes + idx) != 0)
    throw std::runtime_error("pthread_mutex_lock");
#endif
}


/**
 * Release the lock of a slot.
 */
void SlottedMutex::unlock(size_t idx) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  SlottedMutexCore* core = (SlottedMutexCore*)opq_;
  ::LeaveCriticalSection(core->mutexes + idx);
#else
  _assert_(true);
  SlottedMutexCore* core = (SlottedMutexCore*)opq_;
  if (::pthread_mutex_unlock(core->mutexes + idx) != 0)
    throw std::runtime_error("pthread_mutex_unlock");
#endif
}


/**
 * Get the locks of all slots.
 */
void SlottedMutex::lock_all() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  SlottedMutexCore* core = (SlottedMutexCore*)opq_;
  ::CRITICAL_SECTION* mutexes = core->mutexes;
  size_t slotnum = core->slotnum;
  for (size_t i = 0; i < slotnum; i++) {
    ::EnterCriticalSection(core->mutexes + i);
  }
#else
  _assert_(true);
  SlottedMutexCore* core = (SlottedMutexCore*)opq_;
  ::pthread_mutex_t* mutexes = core->mutexes;
  size_t slotnum = core->slotnum;
  for (size_t i = 0; i < slotnum; i++) {
    if (::pthread_mutex_lock(mutexes + i) != 0)
      throw std::runtime_error("pthread_mutex_lock");
  }
#endif
}


/**
 * Release the locks of all slots.
 */
void SlottedMutex::unlock_all() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  SlottedMutexCore* core = (SlottedMutexCore*)opq_;
  ::CRITICAL_SECTION* mutexes = core->mutexes;
  size_t slotnum = core->slotnum;
  for (size_t i = 0; i < slotnum; i++) {
    ::LeaveCriticalSection(mutexes + i);
  }
#else
  _assert_(true);
  SlottedMutexCore* core = (SlottedMutexCore*)opq_;
  ::pthread_mutex_t* mutexes = core->mutexes;
  size_t slotnum = core->slotnum;
  for (size_t i = 0; i < slotnum; i++) {
    if (::pthread_mutex_unlock(mutexes + i) != 0)
      throw std::runtime_error("pthread_mutex_unlock");
  }
#endif
}


/**
 * Default constructor.
 */
SpinLock::SpinLock() : opq_(NULL) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
#elif _KC_GCCATOMIC
  _assert_(true);
#else
  _assert_(true);
  ::pthread_spinlock_t* spin = new ::pthread_spinlock_t;
  if (::pthread_spin_init(spin, PTHREAD_PROCESS_PRIVATE) != 0)
    throw std::runtime_error("pthread_spin_init");
  opq_ = (void*)spin;
#endif
}


/**
 * Destructor.
 */
SpinLock::~SpinLock() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
#elif _KC_GCCATOMIC
  _assert_(true);
#else
  _assert_(true);
  ::pthread_spinlock_t* spin = (::pthread_spinlock_t*)opq_;
  ::pthread_spin_destroy(spin);
  delete spin;
#endif
}


/**
 * Get the lock.
 */
void SpinLock::lock() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  uint32_t wcnt = 0;
  while (::InterlockedCompareExchange((LONG*)&opq_, 1, 0) != 0) {
    if (wcnt >= LOCKBUSYLOOP) {
      Thread::chill();
    } else {
      Thread::yield();
      wcnt++;
    }
  }
#elif _KC_GCCATOMIC
  _assert_(true);
  uint32_t wcnt = 0;
  while (!__sync_bool_compare_and_swap(&opq_, 0, 1)) {
    if (wcnt >= LOCKBUSYLOOP) {
      Thread::chill();
    } else {
      Thread::yield();
      wcnt++;
    }
  }
#else
  _assert_(true);
  ::pthread_spinlock_t* spin = (::pthread_spinlock_t*)opq_;
  if (::pthread_spin_lock(spin) != 0) throw std::runtime_error("pthread_spin_lock");
#endif
}


/**
 * Try to get the lock.
 */
bool SpinLock::lock_try() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  return ::InterlockedCompareExchange((LONG*)&opq_, 1, 0) == 0;
#elif _KC_GCCATOMIC
  _assert_(true);
  return __sync_bool_compare_and_swap(&opq_, 0, 1);
#else
  _assert_(true);
  ::pthread_spinlock_t* spin = (::pthread_spinlock_t*)opq_;
  int32_t ecode = ::pthread_spin_trylock(spin);
  if (ecode == 0) return true;
  if (ecode != EBUSY) throw std::runtime_error("pthread_spin_trylock");
  return false;
#endif
}


/**
 * Release the lock.
 */
void SpinLock::unlock() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  ::InterlockedExchange((LONG*)&opq_, 0);
#elif _KC_GCCATOMIC
  _assert_(true);
  __sync_lock_release(&opq_);
#else
  _assert_(true);
  ::pthread_spinlock_t* spin = (::pthread_spinlock_t*)opq_;
  if (::pthread_spin_unlock(spin) != 0) throw std::runtime_error("pthread_spin_unlock");
#endif
}


/**
 * SlottedSpinLock internal.
 */
struct SlottedSpinLockCore {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_) || _KC_GCCATOMIC
  uint32_t* locks;                       ///< primitives
  size_t slotnum;                        ///< number of slots
#else
  ::pthread_spinlock_t* spins;           ///< primitives
  size_t slotnum;                        ///< number of slots
#endif
};


/**
 * Constructor.
 */
SlottedSpinLock::SlottedSpinLock(size_t slotnum) : opq_(NULL) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_) || _KC_GCCATOMIC
  _assert_(true);
  SlottedSpinLockCore* core = new SlottedSpinLockCore;
  uint32_t* locks = new uint32_t[slotnum];
  for (size_t i = 0; i < slotnum; i++) {
    locks[i] = 0;
  }
  core->locks = locks;
  core->slotnum = slotnum;
  opq_ = (void*)core;
#else
  _assert_(true);
  SlottedSpinLockCore* core = new SlottedSpinLockCore;
  ::pthread_spinlock_t* spins = new ::pthread_spinlock_t[slotnum];
  for (size_t i = 0; i < slotnum; i++) {
    if (::pthread_spin_init(spins + i, PTHREAD_PROCESS_PRIVATE) != 0)
      throw std::runtime_error("pthread_spin_init");
  }
  core->spins = spins;
  core->slotnum = slotnum;
  opq_ = (void*)core;
#endif
}


/**
 * Destructor.
 */
SlottedSpinLock::~SlottedSpinLock() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_) || _KC_GCCATOMIC
  _assert_(true);
  SlottedSpinLockCore* core = (SlottedSpinLockCore*)opq_;
  delete[] core->locks;
  delete core;
#else
  _assert_(true);
  SlottedSpinLockCore* core = (SlottedSpinLockCore*)opq_;
  ::pthread_spinlock_t* spins = core->spins;
  size_t slotnum = core->slotnum;
  for (size_t i = 0; i < slotnum; i++) {
    ::pthread_spin_destroy(spins + i);
  }
  delete[] spins;
  delete core;
#endif
}


/**
 * Get the lock of a slot.
 */
void SlottedSpinLock::lock(size_t idx) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  SlottedSpinLockCore* core = (SlottedSpinLockCore*)opq_;
  uint32_t* lock = core->locks + idx;
  uint32_t wcnt = 0;
  while (::InterlockedCompareExchange((LONG*)lock, 1, 0) != 0) {
    if (wcnt >= LOCKBUSYLOOP) {
      Thread::chill();
    } else {
      Thread::yield();
      wcnt++;
    }
  }
#elif _KC_GCCATOMIC
  _assert_(true);
  SlottedSpinLockCore* core = (SlottedSpinLockCore*)opq_;
  uint32_t* lock = core->locks + idx;
  uint32_t wcnt = 0;
  while (!__sync_bool_compare_and_swap(lock, 0, 1)) {
    if (wcnt >= LOCKBUSYLOOP) {
      Thread::chill();
    } else {
      Thread::yield();
      wcnt++;
    }
  }
#else
  _assert_(true);
  SlottedSpinLockCore* core = (SlottedSpinLockCore*)opq_;
  if (::pthread_spin_lock(core->spins + idx) != 0)
    throw std::runtime_error("pthread_spin_lock");
#endif
}


/**
 * Release the lock of a slot.
 */
void SlottedSpinLock::unlock(size_t idx) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  SlottedSpinLockCore* core = (SlottedSpinLockCore*)opq_;
  uint32_t* lock = core->locks + idx;
  ::InterlockedExchange((LONG*)lock, 0);
#elif _KC_GCCATOMIC
  _assert_(true);
  SlottedSpinLockCore* core = (SlottedSpinLockCore*)opq_;
  uint32_t* lock = core->locks + idx;
  __sync_lock_release(lock);
#else
  _assert_(true);
  SlottedSpinLockCore* core = (SlottedSpinLockCore*)opq_;
  if (::pthread_spin_unlock(core->spins + idx) != 0)
    throw std::runtime_error("pthread_spin_unlock");
#endif
}


/**
 * Get the locks of all slots.
 */
void SlottedSpinLock::lock_all() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  SlottedSpinLockCore* core = (SlottedSpinLockCore*)opq_;
  uint32_t* locks = core->locks;
  size_t slotnum = core->slotnum;
  for (size_t i = 0; i < slotnum; i++) {
    uint32_t* lock = locks + i;
    uint32_t wcnt = 0;
    while (::InterlockedCompareExchange((LONG*)lock, 1, 0) != 0) {
      if (wcnt >= LOCKBUSYLOOP) {
        Thread::chill();
      } else {
        Thread::yield();
        wcnt++;
      }
    }
  }
#elif _KC_GCCATOMIC
  _assert_(true);
  SlottedSpinLockCore* core = (SlottedSpinLockCore*)opq_;
  uint32_t* locks = core->locks;
  size_t slotnum = core->slotnum;
  for (size_t i = 0; i < slotnum; i++) {
    uint32_t* lock = locks + i;
    uint32_t wcnt = 0;
    while (!__sync_bool_compare_and_swap(lock, 0, 1)) {
      if (wcnt >= LOCKBUSYLOOP) {
        Thread::chill();
      } else {
        Thread::yield();
        wcnt++;
      }
    }
  }
#else
  _assert_(true);
  SlottedSpinLockCore* core = (SlottedSpinLockCore*)opq_;
  ::pthread_spinlock_t* spins = core->spins;
  size_t slotnum = core->slotnum;
  for (size_t i = 0; i < slotnum; i++) {
    if (::pthread_spin_lock(spins + i) != 0)
      throw std::runtime_error("pthread_spin_lock");
  }
#endif
}


/**
 * Release the locks of all slots.
 */
void SlottedSpinLock::unlock_all() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  SlottedSpinLockCore* core = (SlottedSpinLockCore*)opq_;
  uint32_t* locks = core->locks;
  size_t slotnum = core->slotnum;
  for (size_t i = 0; i < slotnum; i++) {
    uint32_t* lock = locks + i;
    ::InterlockedExchange((LONG*)lock, 0);
  }
#elif _KC_GCCATOMIC
  _assert_(true);
  SlottedSpinLockCore* core = (SlottedSpinLockCore*)opq_;
  uint32_t* locks = core->locks;
  size_t slotnum = core->slotnum;
  for (size_t i = 0; i < slotnum; i++) {
    uint32_t* lock = locks + i;
    __sync_lock_release(lock);
  }
#else
  _assert_(true);
  SlottedSpinLockCore* core = (SlottedSpinLockCore*)opq_;
  ::pthread_spinlock_t* spins = core->spins;
  size_t slotnum = core->slotnum;
  for (size_t i = 0; i < slotnum; i++) {
    if (::pthread_spin_unlock(spins + i) != 0)
      throw std::runtime_error("pthread_spin_unlock");
  }
#endif
}


/**
 * Default constructor.
 */
RWLock::RWLock() : opq_(NULL) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  SpinRWLock* rwlock = new SpinRWLock;
  opq_ = (void*)rwlock;
#else
  _assert_(true);
  ::pthread_rwlock_t* rwlock = new ::pthread_rwlock_t;
  if (::pthread_rwlock_init(rwlock, NULL) != 0) throw std::runtime_error("pthread_rwlock_init");
  opq_ = (void*)rwlock;
#endif
}


/**
 * Destructor.
 */
RWLock::~RWLock() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  SpinRWLock* rwlock = (SpinRWLock*)opq_;
  delete rwlock;
#else
  _assert_(true);
  ::pthread_rwlock_t* rwlock = (::pthread_rwlock_t*)opq_;
  ::pthread_rwlock_destroy(rwlock);
  delete rwlock;
#endif
}


/**
 * Get the writer lock.
 */
void RWLock::lock_writer() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  SpinRWLock* rwlock = (SpinRWLock*)opq_;
  rwlock->lock_writer();
#else
  _assert_(true);
  ::pthread_rwlock_t* rwlock = (::pthread_rwlock_t*)opq_;
  if (::pthread_rwlock_wrlock(rwlock) != 0) throw std::runtime_error("pthread_rwlock_lock");
#endif
}


/**
 * Try to get the writer lock.
 */
bool RWLock::lock_writer_try() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  SpinRWLock* rwlock = (SpinRWLock*)opq_;
  return rwlock->lock_writer_try();
#else
  _assert_(true);
  ::pthread_rwlock_t* rwlock = (::pthread_rwlock_t*)opq_;
  int32_t ecode = ::pthread_rwlock_trywrlock(rwlock);
  if (ecode == 0) return true;
  if (ecode != EBUSY) throw std::runtime_error("pthread_rwlock_trylock");
  return false;
#endif
}


/**
 * Get a reader lock.
 */
void RWLock::lock_reader() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  SpinRWLock* rwlock = (SpinRWLock*)opq_;
  rwlock->lock_reader();
#else
  _assert_(true);
  ::pthread_rwlock_t* rwlock = (::pthread_rwlock_t*)opq_;
  if (::pthread_rwlock_rdlock(rwlock) != 0) throw std::runtime_error("pthread_rwlock_lock");
#endif
}


/**
 * Try to get a reader lock.
 */
bool RWLock::lock_reader_try() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  SpinRWLock* rwlock = (SpinRWLock*)opq_;
  return rwlock->lock_reader_try();
#else
  _assert_(true);
  ::pthread_rwlock_t* rwlock = (::pthread_rwlock_t*)opq_;
  int32_t ecode = ::pthread_rwlock_tryrdlock(rwlock);
  if (ecode == 0) return true;
  if (ecode != EBUSY) throw std::runtime_error("pthread_rwlock_trylock");
  return false;
#endif
}


/**
 * Release the lock.
 */
void RWLock::unlock() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  SpinRWLock* rwlock = (SpinRWLock*)opq_;
  rwlock->unlock();
#else
  _assert_(true);
  ::pthread_rwlock_t* rwlock = (::pthread_rwlock_t*)opq_;
  if (::pthread_rwlock_unlock(rwlock) != 0) throw std::runtime_error("pthread_rwlock_unlock");
#endif
}


/**
 * SlottedRWLock internal.
 */
struct SlottedRWLockCore {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  RWLock* rwlocks;                       ///< primitives
  size_t slotnum;                        ///< number of slots
#else
  ::pthread_rwlock_t* rwlocks;           ///< primitives
  size_t slotnum;                        ///< number of slots
#endif
};


/**
 * Constructor.
 */
SlottedRWLock::SlottedRWLock(size_t slotnum) : opq_(NULL) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  SlottedRWLockCore* core = new SlottedRWLockCore;
  RWLock* rwlocks = new RWLock[slotnum];
  core->rwlocks = rwlocks;
  core->slotnum = slotnum;
  opq_ = (void*)core;
#else
  _assert_(true);
  SlottedRWLockCore* core = new SlottedRWLockCore;
  ::pthread_rwlock_t* rwlocks = new ::pthread_rwlock_t[slotnum];
  for (size_t i = 0; i < slotnum; i++) {
    if (::pthread_rwlock_init(rwlocks + i, NULL) != 0)
      throw std::runtime_error("pthread_rwlock_init");
  }
  core->rwlocks = rwlocks;
  core->slotnum = slotnum;
  opq_ = (void*)core;
#endif
}


/**
 * Destructor.
 */
SlottedRWLock::~SlottedRWLock() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  SlottedRWLockCore* core = (SlottedRWLockCore*)opq_;
  delete[] core->rwlocks;
  delete core;
#else
  _assert_(true);
  SlottedRWLockCore* core = (SlottedRWLockCore*)opq_;
  ::pthread_rwlock_t* rwlocks = core->rwlocks;
  size_t slotnum = core->slotnum;
  for (size_t i = 0; i < slotnum; i++) {
    ::pthread_rwlock_destroy(rwlocks + i);
  }
  delete[] rwlocks;
  delete core;
#endif
}


/**
 * Get the writer lock of a slot.
 */
void SlottedRWLock::lock_writer(size_t idx) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  SlottedRWLockCore* core = (SlottedRWLockCore*)opq_;
  core->rwlocks[idx].lock_writer();
#else
  _assert_(true);
  SlottedRWLockCore* core = (SlottedRWLockCore*)opq_;
  if (::pthread_rwlock_wrlock(core->rwlocks + idx) != 0)
    throw std::runtime_error("pthread_rwlock_wrlock");
#endif
}


/**
 * Get the reader lock of a slot.
 */
void SlottedRWLock::lock_reader(size_t idx) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  SlottedRWLockCore* core = (SlottedRWLockCore*)opq_;
  core->rwlocks[idx].lock_reader();
#else
  _assert_(true);
  SlottedRWLockCore* core = (SlottedRWLockCore*)opq_;
  if (::pthread_rwlock_rdlock(core->rwlocks + idx) != 0)
    throw std::runtime_error("pthread_rwlock_rdlock");
#endif
}


/**
 * Release the lock of a slot.
 */
void SlottedRWLock::unlock(size_t idx) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  SlottedRWLockCore* core = (SlottedRWLockCore*)opq_;
  core->rwlocks[idx].unlock();
#else
  _assert_(true);
  SlottedRWLockCore* core = (SlottedRWLockCore*)opq_;
  if (::pthread_rwlock_unlock(core->rwlocks + idx) != 0)
    throw std::runtime_error("pthread_rwlock_unlock");
#endif
}


/**
 * Get the writer locks of all slots.
 */
void SlottedRWLock::lock_writer_all() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  SlottedRWLockCore* core = (SlottedRWLockCore*)opq_;
  RWLock* rwlocks = core->rwlocks;
  size_t slotnum = core->slotnum;
  for (size_t i = 0; i < slotnum; i++) {
    rwlocks[i].lock_writer();
  }
#else
  _assert_(true);
  SlottedRWLockCore* core = (SlottedRWLockCore*)opq_;
  ::pthread_rwlock_t* rwlocks = core->rwlocks;
  size_t slotnum = core->slotnum;
  for (size_t i = 0; i < slotnum; i++) {
    if (::pthread_rwlock_wrlock(rwlocks + i) != 0)
      throw std::runtime_error("pthread_rwlock_wrlock");
  }
#endif
}


/**
 * Get the reader locks of all slots.
 */
void SlottedRWLock::lock_reader_all() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  SlottedRWLockCore* core = (SlottedRWLockCore*)opq_;
  RWLock* rwlocks = core->rwlocks;
  size_t slotnum = core->slotnum;
  for (size_t i = 0; i < slotnum; i++) {
    rwlocks[i].lock_reader();
  }
#else
  _assert_(true);
  SlottedRWLockCore* core = (SlottedRWLockCore*)opq_;
  ::pthread_rwlock_t* rwlocks = core->rwlocks;
  size_t slotnum = core->slotnum;
  for (size_t i = 0; i < slotnum; i++) {
    if (::pthread_rwlock_rdlock(rwlocks + i) != 0)
      throw std::runtime_error("pthread_rwlock_rdlock");
  }
#endif
}


/**
 * Release the locks of all slots.
 */
void SlottedRWLock::unlock_all() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  SlottedRWLockCore* core = (SlottedRWLockCore*)opq_;
  RWLock* rwlocks = core->rwlocks;
  size_t slotnum = core->slotnum;
  for (size_t i = 0; i < slotnum; i++) {
    rwlocks[i].unlock();
  }
#else
  _assert_(true);
  SlottedRWLockCore* core = (SlottedRWLockCore*)opq_;
  ::pthread_rwlock_t* rwlocks = core->rwlocks;
  size_t slotnum = core->slotnum;
  for (size_t i = 0; i < slotnum; i++) {
    if (::pthread_rwlock_unlock(rwlocks + i) != 0)
      throw std::runtime_error("pthread_rwlock_unlock");
  }
#endif
}


/**
 * SpinRWLock internal.
 */
struct SpinRWLockCore {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  LONG sem;                              ///< semaphore
  uint32_t cnt;                          ///< count of threads
#elif _KC_GCCATOMIC
  int32_t sem;                           ///< semaphore
  uint32_t cnt;                          ///< count of threads
#else
  ::pthread_spinlock_t sem;              ///< semaphore
  uint32_t cnt;                          ///< count of threads
#endif
};


/**
 * Lock the semephore of SpinRWLock.
 * @param core the internal fields.
 */
static void spinrwlocklock(SpinRWLockCore* core);


/**
 * Unlock the semephore of SpinRWLock.
 * @param core the internal fields.
 */
static void spinrwlockunlock(SpinRWLockCore* core);


/**
 * Default constructor.
 */
SpinRWLock::SpinRWLock() : opq_(NULL) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_) || _KC_GCCATOMIC
  _assert_(true);
  SpinRWLockCore* core = new SpinRWLockCore;
  core->sem = 0;
  core->cnt = 0;
  opq_ = (void*)core;
#else
  _assert_(true);
  SpinRWLockCore* core = new SpinRWLockCore;
  if (::pthread_spin_init(&core->sem, PTHREAD_PROCESS_PRIVATE) != 0)
    throw std::runtime_error("pthread_spin_init");
  core->cnt = 0;
  opq_ = (void*)core;
#endif
}


/**
 * Destructor.
 */
SpinRWLock::~SpinRWLock() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_) || _KC_GCCATOMIC
  _assert_(true);
  SpinRWLockCore* core = (SpinRWLockCore*)opq_;
  delete core;
#else
  _assert_(true);
  SpinRWLockCore* core = (SpinRWLockCore*)opq_;
  ::pthread_spin_destroy(&core->sem);
  delete core;
#endif
}


/**
 * Get the writer lock.
 */
void SpinRWLock::lock_writer() {
  _assert_(true);
  SpinRWLockCore* core = (SpinRWLockCore*)opq_;
  spinrwlocklock(core);
  uint32_t wcnt = 0;
  while (core->cnt > 0) {
    spinrwlockunlock(core);
    if (wcnt >= LOCKBUSYLOOP) {
      Thread::chill();
    } else {
      Thread::yield();
      wcnt++;
    }
    spinrwlocklock(core);
  }
  core->cnt = INT32MAX;
  spinrwlockunlock(core);
}


/**
 * Try to get the writer lock.
 */
bool SpinRWLock::lock_writer_try() {
  _assert_(true);
  SpinRWLockCore* core = (SpinRWLockCore*)opq_;
  spinrwlocklock(core);
  if (core->cnt > 0) {
    spinrwlockunlock(core);
    return false;
  }
  core->cnt = INT32MAX;
  spinrwlockunlock(core);
  return true;
}


/**
 * Get a reader lock.
 */
void SpinRWLock::lock_reader() {
  _assert_(true);
  SpinRWLockCore* core = (SpinRWLockCore*)opq_;
  spinrwlocklock(core);
  uint32_t wcnt = 0;
  while (core->cnt >= (uint32_t)INT32MAX) {
    spinrwlockunlock(core);
    if (wcnt >= LOCKBUSYLOOP) {
      Thread::chill();
    } else {
      Thread::yield();
      wcnt++;
    }
    spinrwlocklock(core);
  }
  core->cnt++;
  spinrwlockunlock(core);
}


/**
 * Try to get a reader lock.
 */
bool SpinRWLock::lock_reader_try() {
  _assert_(true);
  SpinRWLockCore* core = (SpinRWLockCore*)opq_;
  spinrwlocklock(core);
  if (core->cnt >= (uint32_t)INT32MAX) {
    spinrwlockunlock(core);
    return false;
  }
  core->cnt++;
  spinrwlockunlock(core);
  return true;
}


/**
 * Release the lock.
 */
void SpinRWLock::unlock() {
  _assert_(true);
  SpinRWLockCore* core = (SpinRWLockCore*)opq_;
  spinrwlocklock(core);
  if (core->cnt >= (uint32_t)INT32MAX) {
    core->cnt = 0;
  } else {
    core->cnt--;
  }
  spinrwlockunlock(core);
}


/**
 * Promote a reader lock to the writer lock.
 */
bool SpinRWLock::promote() {
  _assert_(true);
  SpinRWLockCore* core = (SpinRWLockCore*)opq_;
  spinrwlocklock(core);
  if (core->cnt > 1) {
    spinrwlockunlock(core);
    return false;
  }
  core->cnt = INT32MAX;
  spinrwlockunlock(core);
  return true;
}


/**
 * Demote the writer lock to a reader lock.
 */
void SpinRWLock::demote() {
  _assert_(true);
  SpinRWLockCore* core = (SpinRWLockCore*)opq_;
  spinrwlocklock(core);
  core->cnt = 1;
  spinrwlockunlock(core);
}


/**
 * Lock the semephore of SpinRWLock.
 */
static void spinrwlocklock(SpinRWLockCore* core) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(core);
  while (::InterlockedCompareExchange(&core->sem, 1, 0) != 0) {
    ::Sleep(0);
  }
#elif _KC_GCCATOMIC
  _assert_(core);
  while (!__sync_bool_compare_and_swap(&core->sem, 0, 1)) {
    ::sched_yield();
  }
#else
  _assert_(core);
  if (::pthread_spin_lock(&core->sem) != 0) throw std::runtime_error("pthread_spin_lock");
#endif
}


/**
 * Unlock the semephore of SpinRWLock.
 */
static void spinrwlockunlock(SpinRWLockCore* core) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(core);
  ::InterlockedExchange(&core->sem, 0);
#elif _KC_GCCATOMIC
  _assert_(core);
  __sync_lock_release(&core->sem);
#else
  _assert_(core);
  if (::pthread_spin_unlock(&core->sem) != 0) throw std::runtime_error("pthread_spin_unlock");
#endif
}


/**
 * SlottedRWLock internal.
 */
struct SlottedSpinRWLockCore {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  LONG sems[LOCKSEMNUM];                 ///< semaphores
#elif _KC_GCCATOMIC
  int32_t sems[LOCKSEMNUM];              ///< semaphores
#else
  ::pthread_spinlock_t sems[LOCKSEMNUM]; ///< semaphores
#endif
  uint32_t* cnts;                        ///< counts of threads
  size_t slotnum;                        ///< number of slots
};


/**
 * Lock the semephore of SlottedSpinRWLock.
 * @param core the internal fields.
 * @param idx the index of the semaphoe.
 */
static void slottedspinrwlocklock(SlottedSpinRWLockCore* core, size_t idx);


/**
 * Unlock the semephore of SlottedSpinRWLock.
 * @param core the internal fields.
 * @param idx the index of the semaphoe.
 */
static void slottedspinrwlockunlock(SlottedSpinRWLockCore* core, size_t idx);


/**
 * Constructor.
 */
SlottedSpinRWLock::SlottedSpinRWLock(size_t slotnum) : opq_(NULL) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  SlottedSpinRWLockCore* core = new SlottedSpinRWLockCore;
  LONG* sems = core->sems;
  uint32_t* cnts = new uint32_t[slotnum];
  for (size_t i = 0; i < LOCKSEMNUM; i++) {
    sems[i] = 0;
  }
  for (size_t i = 0; i < slotnum; i++) {
    cnts[i] = 0;
  }
  core->cnts = cnts;
  core->slotnum = slotnum;
  opq_ = (void*)core;
#elif _KC_GCCATOMIC
  SlottedSpinRWLockCore* core = new SlottedSpinRWLockCore;
  int32_t* sems = core->sems;
  uint32_t* cnts = new uint32_t[slotnum];
  for (size_t i = 0; i < LOCKSEMNUM; i++) {
    sems[i] = 0;
  }
  for (size_t i = 0; i < slotnum; i++) {
    cnts[i] = 0;
  }
  core->cnts = cnts;
  core->slotnum = slotnum;
  opq_ = (void*)core;
#else
  _assert_(true);
  SlottedSpinRWLockCore* core = new SlottedSpinRWLockCore;
  ::pthread_spinlock_t* sems = core->sems;
  uint32_t* cnts = new uint32_t[slotnum];
  for (size_t i = 0; i < LOCKSEMNUM; i++) {
    if (::pthread_spin_init(sems + i, PTHREAD_PROCESS_PRIVATE) != 0)
      throw std::runtime_error("pthread_spin_init");
  }
  for (size_t i = 0; i < slotnum; i++) {
    cnts[i] = 0;
  }
  core->cnts = cnts;
  core->slotnum = slotnum;
  opq_ = (void*)core;
#endif
}


/**
 * Destructor.
 */
SlottedSpinRWLock::~SlottedSpinRWLock() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_) || _KC_GCCATOMIC
  _assert_(true);
  SlottedSpinRWLockCore* core = (SlottedSpinRWLockCore*)opq_;
  delete[] core->cnts;
  delete core;
#else
  _assert_(true);
  SlottedSpinRWLockCore* core = (SlottedSpinRWLockCore*)opq_;
  ::pthread_spinlock_t* sems = core->sems;
  for (size_t i = 0; i < LOCKSEMNUM; i++) {
    ::pthread_spin_destroy(sems + i);
  }
  delete[] core->cnts;
  delete core;
#endif
}


/**
 * Get the writer lock of a slot.
 */
void SlottedSpinRWLock::lock_writer(size_t idx) {
  _assert_(true);
  SlottedSpinRWLockCore* core = (SlottedSpinRWLockCore*)opq_;
  size_t semidx = idx % LOCKSEMNUM;
  slottedspinrwlocklock(core, semidx);
  uint32_t wcnt = 0;
  while (core->cnts[idx] > 0) {
    slottedspinrwlockunlock(core, semidx);
    if (wcnt >= LOCKBUSYLOOP) {
      Thread::chill();
    } else {
      Thread::yield();
      wcnt++;
    }
    slottedspinrwlocklock(core, semidx);
  }
  core->cnts[idx] = INT32MAX;
  slottedspinrwlockunlock(core, semidx);
}


/**
 * Get the reader lock of a slot.
 */
void SlottedSpinRWLock::lock_reader(size_t idx) {
  _assert_(true);
  SlottedSpinRWLockCore* core = (SlottedSpinRWLockCore*)opq_;
  size_t semidx = idx % LOCKSEMNUM;
  slottedspinrwlocklock(core, semidx);
  uint32_t wcnt = 0;
  while (core->cnts[idx] >= (uint32_t)INT32MAX) {
    slottedspinrwlockunlock(core, semidx);
    if (wcnt >= LOCKBUSYLOOP) {
      Thread::chill();
    } else {
      Thread::yield();
      wcnt++;
    }
    slottedspinrwlocklock(core, semidx);
  }
  core->cnts[idx]++;
  slottedspinrwlockunlock(core, semidx);
}


/**
 * Release the lock of a slot.
 */
void SlottedSpinRWLock::unlock(size_t idx) {
  _assert_(true);
  SlottedSpinRWLockCore* core = (SlottedSpinRWLockCore*)opq_;
  size_t semidx = idx % LOCKSEMNUM;
  slottedspinrwlocklock(core, semidx);
  if (core->cnts[idx] >= (uint32_t)INT32MAX) {
    core->cnts[idx] = 0;
  } else {
    core->cnts[idx]--;
  }
  slottedspinrwlockunlock(core, semidx);
}


/**
 * Get the writer locks of all slots.
 */
void SlottedSpinRWLock::lock_writer_all() {
  _assert_(true);
  SlottedSpinRWLockCore* core = (SlottedSpinRWLockCore*)opq_;
  uint32_t* cnts = core->cnts;
  size_t slotnum = core->slotnum;
  for (size_t i = 0; i < slotnum; i++) {
    size_t semidx = i % LOCKSEMNUM;
    slottedspinrwlocklock(core, semidx);
    uint32_t wcnt = 0;
    while (cnts[i] > 0) {
      slottedspinrwlockunlock(core, semidx);
      if (wcnt >= LOCKBUSYLOOP) {
        Thread::chill();
      } else {
        Thread::yield();
        wcnt++;
      }
      slottedspinrwlocklock(core, semidx);
    }
    cnts[i] = INT32MAX;
    slottedspinrwlockunlock(core, semidx);
  }
}


/**
 * Get the reader locks of all slots.
 */
void SlottedSpinRWLock::lock_reader_all() {
  _assert_(true);
  SlottedSpinRWLockCore* core = (SlottedSpinRWLockCore*)opq_;
  uint32_t* cnts = core->cnts;
  size_t slotnum = core->slotnum;
  for (size_t i = 0; i < slotnum; i++) {
    size_t semidx = i % LOCKSEMNUM;
    slottedspinrwlocklock(core, semidx);
    uint32_t wcnt = 0;
    while (cnts[i] >= (uint32_t)INT32MAX) {
      slottedspinrwlockunlock(core, semidx);
      if (wcnt >= LOCKBUSYLOOP) {
        Thread::chill();
      } else {
        Thread::yield();
        wcnt++;
      }
      slottedspinrwlocklock(core, semidx);
    }
    cnts[i]++;
    slottedspinrwlockunlock(core, semidx);
  }
}


/**
 * Release the locks of all slots.
 */
void SlottedSpinRWLock::unlock_all() {
  _assert_(true);
  SlottedSpinRWLockCore* core = (SlottedSpinRWLockCore*)opq_;
  uint32_t* cnts = core->cnts;
  size_t slotnum = core->slotnum;
  for (size_t i = 0; i < slotnum; i++) {
    size_t semidx = i % LOCKSEMNUM;
    slottedspinrwlocklock(core, semidx);
    if (cnts[i] >= (uint32_t)INT32MAX) {
      cnts[i] = 0;
    } else {
      cnts[i]--;
    }
    slottedspinrwlockunlock(core, semidx);
  }
}


/**
 * Lock the semephore of SlottedSpinRWLock.
 */
static void slottedspinrwlocklock(SlottedSpinRWLockCore* core, size_t idx) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(core);
  while (::InterlockedCompareExchange(core->sems + idx, 1, 0) != 0) {
    ::Sleep(0);
  }
#elif _KC_GCCATOMIC
  _assert_(core);
  while (!__sync_bool_compare_and_swap(core->sems + idx, 0, 1)) {
    ::sched_yield();
  }
#else
  _assert_(core);
  if (::pthread_spin_lock(core->sems + idx) != 0) throw std::runtime_error("pthread_spin_lock");
#endif
}


/**
 * Unlock the semephore of SlottedSpinRWLock.
 */
static void slottedspinrwlockunlock(SlottedSpinRWLockCore* core, size_t idx) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(core);
  ::InterlockedExchange(core->sems + idx, 0);
#elif _KC_GCCATOMIC
  _assert_(core);
  __sync_lock_release(core->sems + idx);
#else
  _assert_(core);
  if (::pthread_spin_unlock(core->sems + idx) != 0)
    throw std::runtime_error("pthread_spin_unlock");
#endif
}


/**
 * Default constructor.
 */
CondVar::CondVar() : opq_(NULL) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  CondVarCore* core = new CondVarCore;
  ::InitializeCriticalSection(&core->mutex);
  core->wait = 0;
  core->wake = 0;
  core->sev = ::CreateEvent(NULL, true, false, NULL);
  if (!core->sev) throw std::runtime_error("CreateEvent");
  core->fev = ::CreateEvent(NULL, false, false, NULL);
  if (!core->fev) throw std::runtime_error("CreateEvent");
  opq_ = (void*)core;
#else
  _assert_(true);
  CondVarCore* core = new CondVarCore;
  if (::pthread_cond_init(&core->cond, NULL) != 0)
    throw std::runtime_error("pthread_cond_init");
  opq_ = (void*)core;
#endif
}


/**
 * Destructor.
 */
CondVar::~CondVar() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  CondVarCore* core = (CondVarCore*)opq_;
  ::CloseHandle(core->fev);
  ::CloseHandle(core->sev);
  ::DeleteCriticalSection(&core->mutex);
#else
  _assert_(true);
  CondVarCore* core = (CondVarCore*)opq_;
  ::pthread_cond_destroy(&core->cond);
  delete core;
#endif
}


/**
 * Wait for the signal.
 */
void CondVar::wait(Mutex* mutex) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(mutex);
  CondVarCore* core = (CondVarCore*)opq_;
  ::CRITICAL_SECTION* mymutex = (::CRITICAL_SECTION*)mutex->opq_;
  ::EnterCriticalSection(&core->mutex);
  core->wait++;
  ::LeaveCriticalSection(&core->mutex);
  ::LeaveCriticalSection(mymutex);
  while (true) {
    ::WaitForSingleObject(core->sev, INFINITE);
    ::EnterCriticalSection(&core->mutex);
    if (core->wake > 0) {
      core->wait--;
      core->wake--;
      if (core->wake < 1) {
        ::ResetEvent(core->sev);
        ::SetEvent(core->fev);
      }
      ::LeaveCriticalSection(&core->mutex);
      break;
    }
    ::LeaveCriticalSection(&core->mutex);
  }
  ::EnterCriticalSection(mymutex);
#else
  _assert_(mutex);
  CondVarCore* core = (CondVarCore*)opq_;
  ::pthread_mutex_t* mymutex = (::pthread_mutex_t*)mutex->opq_;
  if (::pthread_cond_wait(&core->cond, mymutex) != 0)
    throw std::runtime_error("pthread_cond_wait");
#endif
}


/**
 * Wait for the signal.
 */
bool CondVar::wait(Mutex* mutex, double sec) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(mutex && sec >= 0);
  if (sec <= 0) return false;
  CondVarCore* core = (CondVarCore*)opq_;
  ::CRITICAL_SECTION* mymutex = (::CRITICAL_SECTION*)mutex->opq_;
  ::EnterCriticalSection(&core->mutex);
  core->wait++;
  ::LeaveCriticalSection(&core->mutex);
  ::LeaveCriticalSection(mymutex);
  while (true) {
    if (::WaitForSingleObject(core->sev, sec * 1000) == WAIT_TIMEOUT) {
      ::EnterCriticalSection(&core->mutex);
      if (::WaitForSingleObject(core->sev, 0) == WAIT_TIMEOUT) {
        core->wait--;
        ::SetEvent(core->fev);
        ::LeaveCriticalSection(&core->mutex);
        ::EnterCriticalSection(mymutex);
        return false;
      }
      ::LeaveCriticalSection(&core->mutex);
    }
    ::EnterCriticalSection(&core->mutex);
    if (core->wake > 0) {
      core->wait--;
      core->wake--;
      if (core->wake < 1) {
        ::ResetEvent(core->sev);
        ::SetEvent(core->fev);
      }
      ::LeaveCriticalSection(&core->mutex);
      break;
    }
    ::LeaveCriticalSection(&core->mutex);
  }
  ::EnterCriticalSection(mymutex);
  return true;
#else
  _assert_(mutex && sec >= 0);
  if (sec <= 0) return false;
  CondVarCore* core = (CondVarCore*)opq_;
  ::pthread_mutex_t* mymutex = (::pthread_mutex_t*)mutex->opq_;
  struct ::timeval tv;
  struct ::timespec ts;
  if (::gettimeofday(&tv, NULL) == 0) {
    double integ;
    double fract = std::modf(sec, &integ);
    ts.tv_sec = tv.tv_sec + (time_t)integ;
    ts.tv_nsec = (long)(tv.tv_usec * 1000.0 + fract * 999999000);
    if (ts.tv_nsec >= 1000000000) {
      ts.tv_nsec -= 1000000000;
      ts.tv_sec++;
    }
  } else {
    ts.tv_sec = std::time(NULL) + 1;
    ts.tv_nsec = 0;
  }
  int32_t ecode = ::pthread_cond_timedwait(&core->cond, mymutex, &ts);
  if (ecode == 0) return true;
  if (ecode != ETIMEDOUT && ecode != EINTR)
    throw std::runtime_error("pthread_cond_timedwait");
  return false;
#endif
}


/**
 * Send the wake-up signal to another waiting thread.
 */
void CondVar::signal() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  CondVarCore* core = (CondVarCore*)opq_;
  ::EnterCriticalSection(&core->mutex);
  if (core->wait > 0) {
    core->wake = 1;
    ::SetEvent(core->sev);
    ::LeaveCriticalSection(&core->mutex);
    ::WaitForSingleObject(core->fev, INFINITE);
  } else {
    ::LeaveCriticalSection(&core->mutex);
  }
#else
  _assert_(true);
  CondVarCore* core = (CondVarCore*)opq_;
  if (::pthread_cond_signal(&core->cond) != 0)
    throw std::runtime_error("pthread_cond_signal");
#endif
}


/**
 * Send the wake-up signals to all waiting threads.
 */
void CondVar::broadcast() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  CondVarCore* core = (CondVarCore*)opq_;
  ::EnterCriticalSection(&core->mutex);
  if (core->wait > 0) {
    core->wake = core->wait;
    ::SetEvent(core->sev);
    ::LeaveCriticalSection(&core->mutex);
    ::WaitForSingleObject(core->fev, INFINITE);
  } else {
    ::LeaveCriticalSection(&core->mutex);
  }
#else
  _assert_(true);
  CondVarCore* core = (CondVarCore*)opq_;
  if (::pthread_cond_broadcast(&core->cond) != 0)
    throw std::runtime_error("pthread_cond_broadcast");
#endif
}


/**
 * Default constructor.
 */
TSDKey::TSDKey() : opq_(NULL) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  ::DWORD key = ::TlsAlloc();
  if (key == 0xFFFFFFFF) throw std::runtime_error("TlsAlloc");
  opq_ = (void*)key;
#else
  _assert_(true);
  ::pthread_key_t* key = new ::pthread_key_t;
  if (::pthread_key_create(key, NULL) != 0)
    throw std::runtime_error("pthread_key_create");
  opq_ = (void*)key;
#endif
}


/**
 * Constructor with the specifications.
 */
TSDKey::TSDKey(void (*dstr)(void*)) : opq_(NULL) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  ::DWORD key = ::TlsAlloc();
  if (key == 0xFFFFFFFF) throw std::runtime_error("TlsAlloc");
  opq_ = (void*)key;
#else
  _assert_(true);
  ::pthread_key_t* key = new ::pthread_key_t;
  if (::pthread_key_create(key, dstr) != 0)
    throw std::runtime_error("pthread_key_create");
  opq_ = (void*)key;
#endif
}


/**
 * Destructor.
 */
TSDKey::~TSDKey() {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  ::DWORD key = (::DWORD)opq_;
  ::TlsFree(key);
#else
  _assert_(true);
  ::pthread_key_t* key = (::pthread_key_t*)opq_;
  ::pthread_key_delete(*key);
  delete key;
#endif
}


/**
 * Set the value.
 */
void TSDKey::set(void* ptr) {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  ::DWORD key = (::DWORD)opq_;
  if (!::TlsSetValue(key, ptr)) std::runtime_error("TlsSetValue");
#else
  _assert_(true);
  ::pthread_key_t* key = (::pthread_key_t*)opq_;
  if (::pthread_setspecific(*key, ptr) != 0)
    throw std::runtime_error("pthread_setspecific");
#endif
}


/**
 * Get the value.
 */
void* TSDKey::get() const {
#if defined(_SYS_MSVC_) || defined(_SYS_MINGW_)
  _assert_(true);
  ::DWORD key = (::DWORD)opq_;
  return ::TlsGetValue(key);
#else
  _assert_(true);
  ::pthread_key_t* key = (::pthread_key_t*)opq_;
  return ::pthread_getspecific(*key);
#endif
}


/**
 * Set the new value.
 */
int64_t AtomicInt64::set(int64_t val) {
#if (defined(_SYS_MSVC_) || defined(_SYS_MINGW_)) && defined(_SYS_WIN64_)
  _assert_(true);
  return ::InterlockedExchange((uint64_t*)&value_, val);
#elif _KC_GCCATOMIC
  _assert_(true);
  int64_t oval = __sync_lock_test_and_set(&value_, val);
  __sync_synchronize();
  return oval;
#else
  _assert_(true);
  lock_.lock();
  int64_t oval = value_;
  value_ = val;
  lock_.unlock();
  return oval;
#endif
}


/**
 * Add a value.
 */
int64_t AtomicInt64::add(int64_t val) {
#if (defined(_SYS_MSVC_) || defined(_SYS_MINGW_)) && defined(_SYS_WIN64_)
  _assert_(true);
  return ::InterlockedExchangeAdd((uint64_t*)&value_, val);
#elif _KC_GCCATOMIC
  _assert_(true);
  int64_t oval = __sync_fetch_and_add(&value_, val);
  __sync_synchronize();
  return oval;
#else
  _assert_(true);
  lock_.lock();
  int64_t oval = value_;
  value_ += val;
  lock_.unlock();
  return oval;
#endif
}


/**
 * Perform compare-and-swap.
 */
bool AtomicInt64::cas(int64_t oval, int64_t nval) {
#if (defined(_SYS_MSVC_) || defined(_SYS_MINGW_)) && defined(_SYS_WIN64_)
  _assert_(true);
  return ::InterlockedCompareExchange((uint64_t*)&value_, nval, oval) == oval;
#elif _KC_GCCATOMIC
  _assert_(true);
  bool rv = __sync_bool_compare_and_swap(&value_, oval, nval);
  __sync_synchronize();
  return rv;
#else
  _assert_(true);
  bool rv = false;
  lock_.lock();
  if (value_ == oval) {
    value_ = nval;
    rv = true;
  }
  lock_.unlock();
  return rv;
#endif
}


/**
 * Get the current value.
 */
int64_t AtomicInt64::get() const {
#if (defined(_SYS_MSVC_) || defined(_SYS_MINGW_)) && defined(_SYS_WIN64_)
  _assert_(true);
  return ::InterlockedExchangeAdd((uint64_t*)&value_, 0);
#elif _KC_GCCATOMIC
  _assert_(true);
  return __sync_fetch_and_add((int64_t*)&value_, 0);
#else
  _assert_(true);
  lock_.lock();
  int64_t oval = value_;
  lock_.unlock();
  return oval;
#endif
}


}                                        // common namespace

// END OF FILE
