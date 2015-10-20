// -*- C++ -*-
#ifndef ALAMEMP3ENCODER_PTHREAD_WRAPPER_H
#define ALAMEMP3ENCODER_PTHREAD_WRAPPER_H

#include <pthread.h>

namespace vscharf {
// A scoped lock which allows access to an object of type T when
// locked.
template<typename T>
class scoped_lock {
public:
  scoped_lock(pthread_mutex_t* m, T& t) : m_(m), t_(t) {
    while(pthread_mutex_lock(m_) != 0) {}
  }
  scoped_lock(const scoped_lock&) = delete;
  scoped_lock(scoped_lock&& other) noexcept : m_(other.m_), t_(other.t_) {
    other.m_ = nullptr;
  }
  scoped_lock& operator=(const scoped_lock&) = delete;
  scoped_lock& operator=(scoped_lock&& other) {
    std::swap(m_, other.m_);
    return *this;
  }
  ~scoped_lock() { if(m_) pthread_mutex_unlock(m_); }
  
  void unlock() { assert(m_); pthread_mutex_unlock(m_); m_ = nullptr; }
  T& get() {
    assert(m_);
    return t_;
  }
  
  bool operator==(const scoped_lock& other) { return m_ == other.m_; }
  bool operator==(const pthread_mutex_t* other) { return m_ == other; }
  bool operator!=(const scoped_lock& other) { return !(*this == other); }
  bool operator!=(const pthread_mutex_t* other) { return !(*this == other); }
private:
  pthread_mutex_t* m_ = nullptr;
  T& t_;
};

// Wrapper for a mutex that protects an object of type T
template<typename T>
class mutex_protected {
public:
  mutex_protected(T& t) : condmutex_(PTHREAD_MUTEX_INITIALIZER), t_(t) {}
  ~mutex_protected() { pthread_mutex_destroy(&condmutex_); }

  scoped_lock<T> acquire() {
    return scoped_lock<T>(&condmutex_, t_);
  }
private:
  pthread_mutex_t condmutex_;
  T& t_;
};

// This class provides RAII for the paired calls to
// pthread_attr_init/pthread_attr_destroy
class scoped_pthread_attr
{
public:
  scoped_pthread_attr() {
    // fails only in case of ENOMEM and from this case can't recover ...
    if(pthread_attr_init(&attr_)) std::terminate();
  }
  ~scoped_pthread_attr() { pthread_attr_destroy(&attr_); }
  pthread_attr_t* get() { return &attr_; }
private:
  pthread_attr_t attr_;
};

// // Encapsulates a condition using pthread condition variables.
// template<typename T>
// class condition_protected {
// public:

//   condition_protected(T& cond)
//     : condition_(cond)
//     , condvar_(PTHREAD_COND_INITIALIZER)
//     , condmutex_(PTHREAD_MUTEX_INITIALIZER)
//   {}
//   ~condition_protected() {
//     pthread_mutex_destroy(&condmutex_);
//     pthread_cond_destroy(&condvar_);
//   }

//   T& condition() { return condition_; }
//   void signal() { pthread_cond_signal(&condvar_); }
//   void broadcast() { pthread_cond_broadcast(&condvar_); }
//   scoped_lock wait(scoped_lock&& lock) {
//     assert(lock == &condmutex_); // check that the correct mutex is locked
//     pthread_cond_wait(&condvar_);
//     return lock;
//   }
//   scoped_lock lock() {
//     scoped_lock l{&condmutex_};
//     return l;
//   }

// private:
//   T& condition_;
//   pthread_cond_t condvar_;
//   pthread_mutex_t condmutex_;
// };
} // namespace vscharf

#endif // ALAMEMP3ENCODER_PTHREAD_WRAPPER_H
