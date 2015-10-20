#include <algorithm>
#include <array>
#include <cassert>
#include <fstream>
#include <iostream>
#include <string>
#include <utility>
#include <vector>

#include "directory.h"
#include "mp3encoder.h"
#include "pthread_wrapper.h"
#include "wavdecoder.h"

using namespace vscharf;

const int QUALITY = 2; // recommended (good) quality setting
const int NTHREADS = 4;

// Function for encoding a file taken from the (condition-protected)
// list of files-names available_files. After no file is left
// decreases the (again condition-protected) number of available
// threads.
namespace EncodeFiles {
  void* do_work(void* args)
  {
    using value_type = std::vector<std::string>;
    auto& available_files = *((mutex_protected<value_type>*)args);
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

      WavDecoder wav(infile);
      Mp3Encoder mp3(QUALITY);
      mp3.encode(wav, outfile);
    }
    // not reachable
    return nullptr;
  } // do_work
} // namespace EncodeFiles

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
  const auto& dir_entries = directory_entries(dir);
  std::copy_if(std::begin(dir_entries), std::end(dir_entries),
	       std::back_inserter(wav_files),
	       [](const std::string& entry) -> bool {
		 return entry.size() > 4 &&
		   entry.substr(entry.size() - 4) == ".wav";
	       });


  // mutex protected list of remaining files
  const std::size_t n_wav_files = wav_files.size();
  mutex_protected<std::vector<std::string>> available_files(wav_files);

  // do the work on (joinable) threads
  scoped_pthread_attr attr;
  pthread_attr_setdetachstate(attr.get(), PTHREAD_CREATE_JOINABLE);
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

  std::cout << "Successfully converted " << n_wav_files << " WAV files to mp3." << std::endl;

  return 0;
}
