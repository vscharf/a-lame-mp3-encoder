#include <algorithm>
#include <array>
#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include <pthread.h>

#include "directory.h"
#include "mp3encoder.h"
#include "wavdecoder.h"

const int QUALITY = 2;
const int NTHREADS = 4;

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
class pthread_mutex {
public:
  pthread_mutex(T& t) : condmutex_(PTHREAD_MUTEX_INITIALIZER), t_(t) {}
  ~pthread_mutex() { pthread_mutex_destroy(&condmutex_); }

  scoped_lock<T> acquire() {
    return scoped_lock<T>(&condmutex_, t_);
  }
private:
  pthread_mutex_t condmutex_;
  T& t_;
};


// // Encapsulates a condition using pthread condition variables.
// template<typename T>
// class pthread_condition {
// public:

//   pthread_condition(T& cond)
//     : condition_(cond)
//     , condvar_(PTHREAD_COND_INITIALIZER)
//     , condmutex_(PTHREAD_MUTEX_INITIALIZER)
//   {}
//   ~pthread_condition() {
//     pthread_mutex_destroy(&condmutex_);
//     pthread_cond_destroy(&condvar_);
//   }

//   T& condition() { return condition_; }
//   void signal() { pthread_cond_signal(&condvar_); }
//   void broadcast() { pthread_cond_broadcast(&condvar_); }
//   scoped_lock&& wait(scoped_lock&& lock) {
//     assert(lock == &condmutex_); // check that the correct mutex is locked
//     pthread_cond_wait(&condvar_);
//     return std::move(lock);
//   }
//   scoped_lock&& lock() {
//     scoped_lock l{&condmutex_};
//     return std::move(l);
//   }

// private:
//   T& condition_;
//   pthread_cond_t condvar_;
//   pthread_mutex_t condmutex_;
// };

// Function for encoding a file taken from the (condition-protected)
// list of files-names available_files. After no file is left
// decreases the (again condition-protected) number of available
// threads.
namespace EncodeFiles {
  void* do_work(void* args)
  {
    using value_type = std::vector<std::string>;
    pthread_mutex<value_type>& available_files = *((pthread_mutex<value_type>*)args);
    while(1) {
      scoped_lock<value_type> lock = available_files.acquire();
      if(lock.get().empty()) return nullptr; // no more work to do
      
      auto infilename = lock.get().back();
      lock.get().pop_back();
      lock.unlock();

      // do the encoding
      std::string outfilename(infilename);
      outfilename.replace(outfilename.size() - 3, 3, "mp3");
      
      std::ifstream infile(infilename);
      std::ofstream outfile(outfilename);

      vscharf::WavDecoder wav(infile);
      vscharf::Mp3Encoder mp3(QUALITY);
      mp3.encode(wav, outfile);
    }
    // not reachable
    return nullptr;
  } // do_work
} // namespace EncodeFiles

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

int main(int argc, char* argv[])
{
  if(argc < 2) {
    std::cerr << argv[0] << ": missing directory operand" << std::endl;
    return 1;
  } else if(argc > 2) {
    std::cerr << argv[0] << ": too many directory operands" << std::endl;
    return 2;
  }

  std::string dir(argv[1]);
  std::vector<std::string> wav_files;
  const auto& dir_entries = vscharf::directory_entries(dir);
  std::copy_if(std::begin(dir_entries), std::end(dir_entries),
	       std::back_inserter(wav_files),
	       [](const std::string& entry) -> bool {
		 return entry.size() > 4 &&
		   entry.substr(entry.size() - 4) == ".wav";
	       });

  scoped_pthread_attr attr;
  pthread_attr_setdetachstate(attr.get(), PTHREAD_CREATE_JOINABLE);

  pthread_mutex<std::vector<std::string>> available_files(wav_files);

  // do the work on (joinable) threads
  std::array<pthread_t, NTHREADS> threads;
  for(auto& t : threads) {
    int rc = pthread_create(&t, attr.get(), EncodeFiles::do_work, (void*)&available_files);
    if(rc) {
      std::cerr << "Couldn't create thread with error " << rc << std::endl;
      return 3;
    }
  }

  // wait for all threads to finish
  for(auto& t : threads) {
    pthread_join(t, nullptr); // ignore error code as we will exit anyways
  }
  return 0;
}
